#!/usr/bin/env python3
"""Write Nintendo data blobs that matching sources need as C include files.

The repository holds no Nintendo data. Some units still embed binary data
(the JAudio DSP microcode), so this pulls those bytes out of your own
bs2.dol into build/<version>/include, where the sources #include them.

usage: extract_blobs.py <bs2.dol> <out include dir> <version>
"""
import struct
import sys
from pathlib import Path

# version -> [(include path, address, size, element size)]
BLOBS = {
    "PAL10": [
        ("jaudio/jdsp.inc", 0x813F4BA0, 0x1760, 2),  # jdsp, JAudio DSP microcode (dspboot.c)
    ],
}


def dol_read(dol, addr, size):
    for i in range(18):
        off, start, length = (struct.unpack_from(">I", dol, base + i * 4)[0] for base in (0x00, 0x48, 0x90))
        if length and start <= addr and addr + size <= start + length:
            return dol[off + addr - start:off + addr - start + size]
    sys.exit(f"{addr:#x}+{size:#x} is not in the DOL")


def main():
    dol = Path(sys.argv[1]).read_bytes()
    out_dir = Path(sys.argv[2])
    for path, addr, size, width in BLOBS.get(sys.argv[3], []):
        data = dol_read(dol, addr, size)
        fmt = {1: "0x{:02X}", 2: "0x{:04X}", 4: "0x{:08X}"}[width]
        vals = [fmt.format(int.from_bytes(data[i:i + width], "big")) for i in range(0, size, width)]
        lines = [", ".join(vals[i:i + 16]) + "," for i in range(0, len(vals), 16)]
        dst = out_dir / path
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
