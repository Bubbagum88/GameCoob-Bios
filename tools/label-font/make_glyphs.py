#!/usr/bin/env python3
"""Rasterize the fallback letters of the menu's label font.

The cube's side labels and face titles are pictures in the IPL, one per
word, so GameCoob builds new words by cutting letters out of those
pictures at boot (src/bs2/menu/label_font.rs). Letters no IPL word has
(B, F, k, v, w and so on) come from M PLUS 1p ExtraBold (SIL Open Font
License, see src/bs2/menu/label_glyphs.OFL.txt), drawn here at the size
and baseline of the IPL's letters and saved as src/bs2/menu/label_glyphs.bin.

Usage: make_glyphs.py MPLUS1p-ExtraBold.ttf src/bs2/menu/label_glyphs.bin
(the Google Fonts latin subset of the font is enough). Needs Pillow.

Format: b"GCLF", height u8, count u8, then per glyph: char u8, width u8,
width * height alpha bytes, row by row.
"""
import sys
from PIL import Image, ImageDraw, ImageFont

H = 32          # height of the IPL's title pictures
CAP_TOP = 3     # first row of an IPL capital ('O' of "Options")
BASE = 27       # last row of an IPL capital
PEAK = 238      # the IPL pictures' full coverage
CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"


def rows(im):
    # rows with solid ink, as the IPL's were measured
    bb = im.point(lambda v: 255 if v > 100 else 0).getbbox()
    return (bb[1], bb[3] - 1) if bb else (0, 0)


def glyph(font, ch):
    im = Image.new("L", (font.size * 2, H), 0)
    ImageDraw.Draw(im).text((2, BASE + 1), ch, font=font, fill=255, anchor="ls")
    bb = im.getbbox()
    im = im.crop((max(bb[0] - 1, 0), 0, bb[2] + 1, H))
    return im.point(lambda v: v * PEAK // 255)


def main():
    path, out = sys.argv[1], sys.argv[2]
    size = next(s for s in range(16, 60)
                if (lambda r: r[1] - r[0])(rows(glyph(ImageFont.truetype(path, s), "O"))) >= BASE - CAP_TOP)
    font = ImageFont.truetype(path, size)
    data = bytearray(b"GCLF") + bytes([H, len(CHARS)])
    for ch in CHARS:
        g = glyph(font, ch)
        data += bytes([ord(ch), g.width]) + g.tobytes()
    open(out, "wb").write(data)
    print(f"size {size}, {len(CHARS)} glyphs, {len(data)} bytes")


main()
