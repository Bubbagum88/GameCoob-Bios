#!/usr/bin/env python3
"""Progress graphics in the style of decomp.dev, made from objdiff's report.json.

Writes into progress/:
  progress.svg           treemap of every unit, sized by code bytes
  badge-total.svg        shields.io style badges (total = code + data bytes)
  badge-code.svg
  badge-functions.svg
  history.csv            one row per day: date, code %, matched / total functions
  history.svg            line chart of history.csv

Only unit names, sizes and percentages go into these files, never code or data.
Standard library only. Run `python tools/progress_art.py [report.json] [out_dir]`;
`python configure.py progress` (and the ninja PROGRESS step) also calls it.
"""

import csv
import datetime
import json
import os
import sys
from xml.sax.saxutils import escape

TITLE = "BS2 PAL 1.0"
WIDTH = 960
HEADER = 44
MAP_HEIGHT = 540

GREEN = (46, 160, 67)
YELLOW = (210, 153, 34)
GREY = "#6e7681"
BG = "#0d1117"
FG = "#e6edf3"


def num(v):
    return float(v) if v is not None else 0.0


def load_report(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def group_of(name, meta):
    """Group shown in the treemap: SDK library, BS2 area, or unsplit asm."""
    if meta.get("auto_generated"):
        return "unsplit (asm)"
    path = name.split("/", 1)[1] if "/" in name else name
    parts = path.split("/")
    if parts[0] == "dolphin" and len(parts) > 2:
        return "SDK " + parts[1]
    if parts[0] == "bs2" and len(parts) > 2:
        return "BS2 " + parts[1]
    return parts[0]


def units_of(report):
    out = []
    for u in report["units"]:
        m = u.get("measures", {})
        size = num(m.get("total_code"))
        if size <= 0:
            continue
        meta = u.get("metadata", {})
        label = u["name"].split("/", 1)[1] if "/" in u["name"] else u["name"]
        out.append({
            "name": label.rsplit("/", 1)[-1],
            "path": label,
            "group": group_of(u["name"], meta),
            "size": size,
            "matched": num(m.get("matched_code_percent")),
            "fuzzy": num(m.get("fuzzy_match_percent")),
            "funcs": (int(m.get("matched_functions", 0)), int(m.get("total_functions", 0))),
            "asm": bool(meta.get("auto_generated")),
        })
    return out


def color(u):
    if u["asm"]:
        return GREY
    if u["matched"] >= 100.0:
        return "#%02x%02x%02x" % GREEN
    t = max(0.0, min(1.0, u["matched"] / 100.0))
    c = tuple(round(a + (b - a) * t) for a, b in zip(YELLOW, GREEN))
    return "#%02x%02x%02x" % c


def squarify(items, x, y, w, h):
    """Squarified treemap: items are (weight, payload), returns (x, y, w, h, payload)."""
    items = [it for it in items if it[0] > 0]
    total = sum(it[0] for it in items)
    if not items or w <= 0 or h <= 0:
        return []
    scale = w * h / total
    rest = [(wt * scale, p) for wt, p in sorted(items, key=lambda it: -it[0])]
    out = []

    def worst(row, side):
        s = sum(r for r, _ in row)
        return max(max(side * side * r / (s * s), (s * s) / (side * side * r)) for r, _ in row)

    while rest:
        side = min(w, h)
        row = [rest.pop(0)]
        while rest and worst(row + [rest[0]], side) <= worst(row, side):
            row.append(rest.pop(0))
        s = sum(r for r, _ in row)
        if w >= h:
            cw = s / h
            cy = y
            for r, p in row:
                out.append((x, cy, cw, r / cw, p))
                cy += r / cw
            x, w = x + cw, w - cw
        else:
            ch = s / w
            cx = x
            for r, p in row:
                out.append((cx, y, r / ch, ch, p))
                cx += r / ch
            y, h = y + ch, h - ch
    return out


def text_width(s, px=11):
    return len(s) * px * 0.6


def total_percent(m):
    """Matched bytes of code and data together, out of all bytes."""
    total = num(m.get("total_code")) + num(m.get("total_data"))
    return 100.0 * (num(m.get("matched_code")) + num(m.get("matched_data"))) / total if total else 0.0


def treemap_svg(report, units):
    m = report["measures"]
    head = "%s: %.2f%% total, %.2f%% code, %.2f%% data, %d/%d functions" % (
        TITLE, total_percent(m), num(m.get("matched_code_percent")), num(m.get("matched_data_percent")),
        int(m.get("matched_functions", 0)), int(m.get("total_functions", 0)))
    out = [
        '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" font-family="Verdana,DejaVu Sans,sans-serif">'
        % (WIDTH, HEADER + MAP_HEIGHT),
        '<rect width="100%%" height="100%%" fill="%s"/>' % BG,
    ]
    pct = num(m.get("matched_code_percent")) / 100.0
    out.append('<rect x="0" y="0" width="%d" height="%d" fill="#161b22"/>' % (WIDTH, HEADER))
    out.append('<rect x="0" y="%d" width="%.1f" height="4" fill="#%02x%02x%02x"/>' % (HEADER - 4, WIDTH * pct, *GREEN))
    out.append('<text x="12" y="27" font-size="16" fill="%s">%s</text>' % (FG, escape(head)))

    groups = {}
    for u in units:
        groups.setdefault(u["group"], []).append(u)
    blocks = squarify([(sum(u["size"] for u in g), name) for name, g in groups.items()], 0, HEADER, WIDTH, MAP_HEIGHT)
    for gx, gy, gw, gh, gname in blocks:
        inner = squarify([(u["size"], u) for u in groups[gname]], gx + 2, gy + 16, gw - 4, gh - 18)
        out.append('<g><title>%s</title>' % escape(gname))
        out.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="#21262d" stroke="%s"/>'
                   % (gx, gy, gw, gh, BG))
        if gw > text_width(gname) + 8:
            out.append('<text x="%.1f" y="%.1f" font-size="11" fill="%s">%s</text>' % (gx + 4, gy + 12, FG, escape(gname)))
        out.append("</g>")
        for x, y, w, h, u in inner:
            tip = "%s: %d bytes, %.1f%% matched, %.1f%% fuzzy, %d/%d functions" % (
                u["path"], u["size"], u["matched"], u["fuzzy"], u["funcs"][0], u["funcs"][1])
            out.append('<g><title>%s</title>' % escape(tip))
            out.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s" stroke="%s" stroke-width="1"/>'
                       % (x, y, w, h, color(u), BG))
            if w > text_width(u["name"], 10) + 6 and h > 14:
                out.append('<text x="%.1f" y="%.1f" font-size="10" fill="#0d1117">%s</text>' % (x + 3, y + 12, escape(u["name"])))
            out.append("</g>")
    out.append("</svg>")
    return "\n".join(out) + "\n"


def badge_svg(label, value, fill):
    lw = int(text_width(label, 11)) + 12
    vw = int(text_width(value, 11)) + 12
    w = lw + vw
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="20" role="img" aria-label="{l}: {v}">'
        '<linearGradient id="s" x2="0" y2="100%"><stop offset="0" stop-color="#bbb" stop-opacity=".1"/>'
        '<stop offset="1" stop-opacity=".1"/></linearGradient>'
        '<clipPath id="r"><rect width="{w}" height="20" rx="3" fill="#fff"/></clipPath>'
        '<g clip-path="url(#r)"><rect width="{lw}" height="20" fill="#555"/>'
        '<rect x="{lw}" width="{vw}" height="20" fill="{f}"/><rect width="{w}" height="20" fill="url(#s)"/></g>'
        '<g fill="#fff" text-anchor="middle" font-family="Verdana,Geneva,DejaVu Sans,sans-serif" font-size="11">'
        '<text x="{lx}" y="15" fill="#010101" fill-opacity=".3">{l}</text><text x="{lx}" y="14">{l}</text>'
        '<text x="{vx}" y="15" fill="#010101" fill-opacity=".3">{v}</text><text x="{vx}" y="14">{v}</text></g></svg>\n'
    ).format(w=w, lw=lw, vw=vw, f=fill, l=escape(label), v=escape(value), lx=lw / 2, vx=lw + vw / 2)


def badge_color(p):
    if p >= 100:
        return "#4c1"
    if p >= 50:
        return "#97ca00"
    if p >= 10:
        return "#dfb317"
    return "#fe7d37"


def update_history(path, report):
    m = report["measures"]
    today = datetime.date.today().isoformat()
    row = [today, "%.4f" % num(m.get("matched_code_percent")), str(int(m.get("matched_functions", 0))),
           str(int(m.get("total_functions", 0)))]
    rows = []
    if os.path.exists(path):
        with open(path, newline="", encoding="utf-8") as f:
            rows = [r for r in csv.reader(f) if r and r[0] != "date"]
    rows = [r for r in rows if r[0] != today] + [row]
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["date", "code_percent", "matched_functions", "total_functions"])
        w.writerows(rows)
    return rows


def history_svg(rows):
    w, h, pad = 640, 220, 36
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" font-family="Verdana,DejaVu Sans,sans-serif">' % (w, h),
           '<rect width="100%%" height="100%%" fill="%s"/>' % BG,
           '<text x="%d" y="20" font-size="13" fill="%s">%s progress</text>' % (pad, FG, TITLE)]
    pts_code, pts_func = [], []
    top = max([float(r[1]) for r in rows] + [100.0 * int(r[2]) / max(1, int(r[3])) for r in rows] + [1.0])
    top = min(100.0, max(5.0, top * 1.2))
    n = len(rows)
    for i, r in enumerate(rows):
        x = pad + (w - 2 * pad) * (i / (n - 1) if n > 1 else 0.5)
        fy = lambda v: h - pad - (h - 2 * pad) * v / top
        pts_code.append((x, fy(float(r[1]))))
        pts_func.append((x, fy(100.0 * int(r[2]) / max(1, int(r[3])))))
    out.append('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="#30363d"/>' % (pad, h - pad, w - pad, h - pad))
    out.append('<text x="4" y="%d" font-size="10" fill="#8b949e">%.0f%%</text>' % (pad + 4, top))
    out.append('<text x="4" y="%d" font-size="10" fill="#8b949e">0%%</text>' % (h - pad))
    for pts, col, name in ((pts_func, "#58a6ff", "functions"), (pts_code, "#%02x%02x%02x" % GREEN, "code")):
        if len(pts) > 1:
            out.append('<polyline fill="none" stroke="%s" stroke-width="2" points="%s"/>'
                       % (col, " ".join("%.1f,%.1f" % p for p in pts)))
        for x, y in pts:
            out.append('<circle cx="%.1f" cy="%.1f" r="3" fill="%s"/>' % (x, y, col))
    out.append('<text x="%d" y="%d" font-size="10" fill="#%02x%02x%02x">code %%</text>' % (w - 150, 20, *GREEN))
    out.append('<text x="%d" y="%d" font-size="10" fill="#58a6ff">functions %%</text>' % (w - 90, 20))
    out.append('<text x="%d" y="%d" font-size="10" fill="#8b949e">%s</text>' % (pad, h - 12, rows[0][0]))
    out.append('<text x="%d" y="%d" font-size="10" fill="#8b949e" text-anchor="end">%s</text>' % (w - pad, h - 12, rows[-1][0]))
    out.append("</svg>")
    return "\n".join(out) + "\n"


def main(report_path="build/PAL10/report.json", out_dir="progress"):
    report = load_report(report_path)
    os.makedirs(out_dir, exist_ok=True)
    m = report["measures"]
    code = num(m.get("matched_code_percent"))
    funcs = (int(m.get("matched_functions", 0)), int(m.get("total_functions", 0)))
    total = total_percent(m)
    files = {
        "badge-total.svg": badge_svg("total", "%.2f%%" % total, badge_color(total)),
        "progress.svg": treemap_svg(report, units_of(report)),
        "badge-code.svg": badge_svg("code", "%.2f%%" % code, badge_color(code)),
        "badge-functions.svg": badge_svg("functions", "%d/%d" % funcs, badge_color(100.0 * funcs[0] / max(1, funcs[1]))),
    }
    for name, body in files.items():
        with open(os.path.join(out_dir, name), "w", encoding="utf-8") as f:
            f.write(body)
    rows = update_history(os.path.join(out_dir, "history.csv"), report)
    with open(os.path.join(out_dir, "history.svg"), "w", encoding="utf-8") as f:
        f.write(history_svg(rows))
    print("progress art: %.2f%% total, %.2f%% code, %d/%d functions -> %s" % (total, code, funcs[0], funcs[1], out_dir))


if __name__ == "__main__":
    main(*sys.argv[1:3])
