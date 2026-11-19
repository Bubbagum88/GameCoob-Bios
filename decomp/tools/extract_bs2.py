#!/usr/bin/env python3
"""Turn a GameCube IPL ROM dump into the DOL that decomp-toolkit analyzes.

The IPL ROM holds BS1 (a small boot stub) and BS2 (the IPL proper, a normal
Dolphin SDK program). Both are XOR-scrambled. BS1 copies BS2 from ROM 0x820
to 0x81300000; BS2's own startup code then carries a table of its sections
(the `__rom_copy_info` / `__bss_init_info` pair the SDK linker emits). We read
that table and write a DOL with the same sections, so the DOL is a faithful
container for exactly the bytes BS2 runs, and nothing from the ROM is ever
stored in this repository.

Usage: tools/extract_bs2.py orig/PAL10/ipl.bin orig/PAL10/bs2.dol
"""

import hashlib
import struct
import sys

SCRAMBLE_START = 0x100
SCRAMBLE_END = 0x1AFF00  # the fonts after this are stored in the clear
BS2_ROM = 0x820
BS2_ADDR = 0x81300000

# Known dumps: md5 -> (version id, section table address). The table is the
# operand of the `lis/addi` pair in __init_data; find_table() finds it in any
# dump, so a new revision only needs its id added here.
KNOWN = {
    "0cdda509e2da83c85bfe423dd87346cc": ("PAL10", 0x8130030C),
}


def keystream(size):
    """Three 16-bit LFSRs, as documented by segher and used by Dolphin/MAME."""
    out = bytearray(size)
    acc = nacc = 0
    t, u, v = 0x2953, 0xD9C2, 0x3FF1
    x = 1
    i = 0
    while i < size:
        t0, t1 = t & 1, (t >> 1) & 1
        u0, u1 = u & 1, (u >> 1) & 1
        v0 = v & 1
        x ^= t1 ^ v0
        x ^= u0 | u1
        x ^= (t0 ^ u1 ^ v0) & (t0 ^ u0)
        if t0 == u0:
            v >>= 1
            if v0:
                v ^= 0xB3D0
        if t0 == 0:
            u >>= 1
            if u0:
                u ^= 0xFB10
        t >>= 1
        if t0:
            t ^= 0xA740
        nacc += 1
        acc = ((acc << 1) | x) & 0xFF
        if nacc == 8:
            out[i] = acc
            i += 1
            nacc = 0
    return bytes(out)


def descramble(rom):
    size = SCRAMBLE_END - SCRAMBLE_START
    region = int.from_bytes(rom[SCRAMBLE_START:SCRAMBLE_END], "big")
    ks = int.from_bytes(keystream(size), "big")
    return rom[:SCRAMBLE_START] + (region ^ ks).to_bytes(size, "big") + rom[SCRAMBLE_END:]


def table_ok(image, a):
    """True if a holds a plausible __rom_copy_info table: unrelocated BS2 sections."""
    def word(addr):
        return struct.unpack_from(">I", image, addr - BS2_ADDR)[0]
    n = 0
    while a - BS2_ADDR + 12 <= len(image):
        rom_addr, addr, size = word(a), word(a + 4), word(a + 8)
        if size == 0:
            return n >= 3
        if rom_addr != addr or not BS2_ADDR <= addr < 0x81800000 or size > 0x400000:
            return False
        a += 12
        n += 1
    return False


def find_table(image):
    """Find the section table from the lis/addi pair in the startup code."""
    words = struct.unpack_from(">%dI" % (0x400 // 4), image, 0)
    for i, w in enumerate(words):
        if w >> 26 != 15 or (w >> 16) & 0x1F != 0:  # lis rD
            continue
        rd, hi = (w >> 21) & 0x1F, w & 0xFFFF
        for w2 in words[i + 1:i + 5]:
            if w2 >> 26 == 14 and (w2 >> 16) & 0x1F == rd:  # addi rX, rD, lo
                lo = w2 & 0xFFFF
                a = ((hi << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
                if BS2_ADDR <= a < BS2_ADDR + 0x400 and table_ok(image, a):
                    return a
    sys.exit("could not find the BS2 section table")


def main():
    src, dst = sys.argv[1], sys.argv[2]
    rom = open(src, "rb").read()
    md5 = hashlib.md5(rom).hexdigest()
    rom = descramble(rom)
    image = rom[BS2_ROM:SCRAMBLE_END]
    table = find_table(image)
    if md5 in KNOWN:
        version, known = KNOWN[md5]
        assert table == known, f"section table at {table:#x}, expected {known:#x}"
    else:
        # Still extract, so a new revision can be looked at before it has a config.
        version = "UNKNOWN"
        print(f"{src}: unknown IPL dump (md5 {md5}); add it to KNOWN in {__file__} "
              f"(section table {table:#x})")

    def word(addr):
        return struct.unpack_from(">I", image, addr - BS2_ADDR)[0]

    sections = []
    a = table
    while True:
        rom_addr, addr, size = word(a), word(a + 4), word(a + 8)
        a += 12
        if size == 0:
            break
        assert rom_addr == addr, "BS2 sections are not relocated"
        sections.append((addr, image[addr - BS2_ADDR:addr - BS2_ADDR + size]))
    bss = []
    while True:
        addr, size = word(a), word(a + 4)
        a += 8
        if size == 0:
            break
        bss.append((addr, size))

    # A section is text if it is .init or .text: the first two in the table.
    text, data = sections[:2], sections[2:]
    assert len(data) <= 11
    bss_start = min(b[0] for b in bss)
    bss_end = max(b[0] + b[1] for b in bss)

    hdr = bytearray(0x100)
    off = 0x100
    body = bytearray()
    for i, (addr, blob) in enumerate(text):
        struct.pack_into(">III", hdr, 0, *([0] * 3))  # placeholder, filled below
    slots = [(0x00, 0x48, 0x90, i) for i in range(7)] + [(0x1C, 0x64, 0xAC, i) for i in range(11)]
    for (o_off, o_addr, o_size, i), (addr, blob) in zip(
        [s for s in slots if s[0] == 0x00][: len(text)] + [s for s in slots if s[0] == 0x1C][: len(data)],
        text + data,
    ):
        struct.pack_into(">I", hdr, o_off + i * 4, off + len(body))
        struct.pack_into(">I", hdr, o_addr + i * 4, addr)
        # Sizes are rounded up to 0x20 like dtk elf2dol, so the build check matches.
        struct.pack_into(">I", hdr, o_size + i * 4, len(blob) + (-len(blob) % 0x20))
        body += blob
        body += b"\0" * (-len(body) % 0x20)
    struct.pack_into(">III", hdr, 0xD8, bss_start, bss_end - bss_start, BS2_ADDR)
    out = bytes(hdr + body)
    open(dst, "wb").write(out)
    print(f"{version}: wrote {dst} ({len(text)} text, {len(data)} data sections, "
          f"bss {bss_start:#x}..{bss_end:#x}), sha1 {hashlib.sha1(out).hexdigest()}")


if __name__ == "__main__":
    main()
