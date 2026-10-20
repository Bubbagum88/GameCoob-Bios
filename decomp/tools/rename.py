#!/usr/bin/env python3
"""Apply config/<VER>/names.txt to config/<VER>/symbols.txt (rename by address)."""
import re, sys
ver = sys.argv[1] if len(sys.argv) > 1 else "PAL10"
names = {}
for line in open(f"config/{ver}/names.txt"):
    line = line.split("#")[0].split()
    if len(line) == 2:
        names[int(line[0], 16)] = line[1]
out = []
for line in open(f"config/{ver}/symbols.txt"):
    m = re.match(r"(\S+) = (\.\w+):0x([0-9A-Fa-f]+);(.*)", line)
    if m and int(m.group(3), 16) in names:
        line = f"{names[int(m.group(3), 16)]} = {m.group(2)}:0x{m.group(3)};{m.group(4)}\n"
    out.append(line)
open(f"config/{ver}/symbols.txt", "w").writelines(out)
