#!/usr/bin/env python3
"""Port function symbols and .text splits from one IPL version to another.

Every function in the source version's symbols.txt is fingerprinted with the
parts that move between builds masked out (branch targets, and the 16-bit
immediates of lis/addi/ori and loads/stores that can hold addresses or small
data offsets). The same fingerprint is then looked for in the target bs2.dol.
A function found exactly once, or at the same shift as its neighbours, keeps
its name at the new address. Functions that are not found are the code that
really changed between the versions; they and every non-function symbol are
listed in the report for hand work (dtk detects the rest on its own).

Usage:
  tools/port_symbols.py PAL10 PAL12
    reads orig/PAL10/bs2.dol, config/PAL10/{symbols,splits}.txt, orig/PAL12/bs2.dol
    writes config/PAL12/symbols.txt, config/PAL12/splits.txt, config/PAL12/port_report.txt

Nothing from either dump is written, only names, addresses and sizes.
"""

import argparse
import re
import struct
import sys
from pathlib import Path

SYM_RE = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+); //(.*)$")
WINDOW = 4  # words used to index candidate positions


class Dol:
    def __init__(self, path):
        d = Path(path).read_bytes()
        self.sections = []  # (addr, bytes)
        for i in range(18):
            off, addr, size = (struct.unpack_from(">I", d, base + i * 4)[0]
                               for base in (0x00, 0x48, 0x90))
            if size:
                self.sections.append((addr, d[off:off + size]))
        self.text = self.sections[:2]  # .init and .text, as tools/extract_bs2.py writes them

    def words(self, addr, n):
        for base, blob in self.sections:
            if base <= addr and addr + n * 4 <= base + len(blob):
                return struct.unpack_from(">%dI" % n, blob, addr - base)
        return None


def mask(w):
    op = w >> 26
    if op == 18:  # b / bl: target moves
        return w & 0xFC000003
    ra = (w >> 16) & 0x1F
    if op == 15 or op == 24 or op == 25:  # lis / ori / oris: address halves
        return w & 0xFFFF0000
    if op == 14 and ra != 0 and ra != 1:  # addi off a register (not li, not stack)
        return w & 0xFFFF0000
    if 32 <= op <= 55 and ra != 1:  # loads/stores not off the stack pointer
        return w & 0xFFFF0000
    return w


def read_symbols(path):
    syms = []
    for line in Path(path).read_text().splitlines():
        m = SYM_RE.match(line)
        if not m:
            continue
        name, sec, addr, attrs = m.group(1), m.group(2), int(m.group(3), 16), m.group(4)
        size = re.search(r"size:0x([0-9A-Fa-f]+)", attrs)
        syms.append({
            "name": name, "sec": sec, "addr": addr, "attrs": attrs.strip(),
            "size": int(size.group(1), 16) if size else 0,
            "func": "type:function" in attrs,
        })
    return syms


def read_splits(path):
    """Return (header lines, [(unit, unit attr line, [(sec, start, end, rest)])])."""
    header, units, cur = [], [], None
    for line in Path(path).read_text().splitlines():
        if line and not line[0].isspace() and line.endswith(":") and line != "Sections:":
            cur = (line[:-1], [])
            units.append(cur)
        elif cur is None:
            header.append(line)
        else:
            m = re.match(r"\s+(\.\w+)\s+start:0x([0-9A-Fa-f]+) end:0x([0-9A-Fa-f]+)(.*)", line)
            if m:
                cur[1].append((m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4)))
    return header, units


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("src", help="version to port from, e.g. PAL10")
    ap.add_argument("dst", help="version to port to, e.g. PAL12")
    ap.add_argument("--src-dol", help="default orig/<src>/bs2.dol")
    ap.add_argument("--dst-dol", help="default orig/<dst>/bs2.dol")
    ap.add_argument("--out", help="default config/<dst>")
    a = ap.parse_args()
    src_dol = Dol(a.src_dol or f"orig/{a.src}/bs2.dol")
    dst_dol = Dol(a.dst_dol or f"orig/{a.dst}/bs2.dol")
    out = Path(a.out or f"config/{a.dst}")
    out.mkdir(parents=True, exist_ok=True)
    syms = read_symbols(f"config/{a.src}/symbols.txt")
    funcs = [s for s in syms if s["func"] and s["size"] >= 4]

    # Index every masked WINDOW-word run in the target's code.
    index, dst_masked = {}, {}
    for base, blob in dst_dol.text:
        ws = [mask(w) for w in struct.unpack(">%dI" % (len(blob) // 4), blob)]
        dst_masked[base] = ws
        for i in range(len(ws)):
            index.setdefault(tuple(ws[i:i + WINDOW]), []).append(base + i * 4)

    def dst_match(addr, fp):
        for base, ws in dst_masked.items():
            i = (addr - base) // 4
            if 0 <= i and i + len(fp) <= len(ws) and base <= addr:
                return ws[i:i + len(fp)] == fp
        return False

    # Pass 1: unique exact fingerprints anchor the map.
    cands, new_addr = {}, {}
    for f in funcs:
        fp = [mask(w) for w in src_dol.words(f["addr"], f["size"] // 4)]
        f["fp"] = fp
        hits = ([p for p in index.get(tuple(fp[:WINDOW]), []) if dst_match(p, fp)]
                if len(fp) >= WINDOW else [])
        cands[f["addr"]] = hits
        if len(hits) == 1:
            new_addr[f["addr"]] = hits[0]

    # Pass 2: duplicates and functions shorter than the index window take the
    # shift of the nearest anchored function before them, if they match there.
    shift = 0
    for f in funcs:
        if f["addr"] in new_addr:
            shift = new_addr[f["addr"]] - f["addr"]
        elif dst_match(f["addr"] + shift, f["fp"]):
            new_addr[f["addr"]] = f["addr"] + shift
    # ...then the shift of the nearest one after them (code inserted just before).
    shift = 0
    for f in reversed(funcs):
        if f["addr"] in new_addr:
            shift = new_addr[f["addr"]] - f["addr"]
        elif dst_match(f["addr"] + shift, f["fp"]):
            new_addr[f["addr"]] = f["addr"] + shift

    # Symbols: mapped functions only.
    mapped = [f for f in funcs if f["addr"] in new_addr]
    lines = [f'{f["name"]} = {f["sec"]}:0x{new_addr[f["addr"]]:08X}; // {f["attrs"]}'
             for f in sorted(mapped, key=lambda f: new_addr[f["addr"]])]
    (out / "symbols.txt").write_text("\n".join(lines) + "\n")

    # Splits: a unit's code range ports when its first and last functions both moved
    # by the same shift and everything between them did too.
    by_addr = {f["addr"]: f for f in funcs}
    header, units = read_splits(f"config/{a.src}/splits.txt")
    split_out, manual = header[:], []
    for unit, ranges in units:
        kept = []
        for sec, start, end, rest in ranges:
            if sec not in (".init", ".text"):
                continue
            inside = [f for f in funcs if start <= f["addr"] < end and f["sec"] == sec]
            shifts = {new_addr[f["addr"]] - f["addr"] if f["addr"] in new_addr else None
                      for f in inside}
            if inside and len(shifts) == 1 and None not in shifts and start in by_addr:
                d = shifts.pop()
                kept.append(f"\t{sec:<11} start:0x{start + d:08X} end:0x{end + d:08X}{rest}")
            else:
                manual.append(f"{unit} {sec} 0x{start:08X}-0x{end:08X}")
        if kept:
            split_out += [f"{unit}:"] + kept + [""]
    (out / "splits.txt").write_text("\n".join(split_out).rstrip() + "\n")

    missing = [f for f in funcs if f["addr"] not in new_addr]
    data = [s for s in syms if not s["func"]]
    rep = [f"port {a.src} -> {a.dst}",
           f"functions mapped: {len(mapped)}/{len(funcs)}",
           f"units with code ranges ported: {sum(1 for u in split_out if u.endswith(':'))}/{len(units)}",
           "", f"## functions not found ({len(missing)}): changed code, port by hand"]
    rep += [f'{f["name"]} {f["sec"]}:0x{f["addr"]:08X} size:0x{f["size"]:X} '
            f'({len(cands[f["addr"]])} exact hits)' for f in missing]
    rep += ["", f"## code ranges not ported ({len(manual)})"] + manual
    rep += ["", f"## non-function symbols left for dtk/hand ({len(data)})"]
    rep += [f'{s["name"]} {s["sec"]}:0x{s["addr"]:08X}' for s in data]
    (out / "port_report.txt").write_text("\n".join(rep) + "\n")
    print("\n".join(rep[:3]))
    print(f"wrote {out}/symbols.txt, splits.txt, port_report.txt")


if __name__ == "__main__":
    sys.exit(main())
