#include "bs2/menu.h"
#include "bs2/gbase.h"
#include <dolphin/os.h>
#include <string.h>

/*
 * IPL font (0x81307FB8..0x81309620): the boot ROM font is read over EXI,
 * unpacked and its 2 bpp sheet expanded once, then text boxes of a GLH0
 * string table are laid out and drawn one textured quad per character.
 * Positions and sizes are 12.4 fixed point.
 */

/* Same layout as the SDK's OSFontHeader */
typedef struct FontHeader {
    u16 fontType;      /* 0x00 */
    u16 firstChar;     /* 0x02 */
    u16 lastChar;      /* 0x04 */
    u16 invalChar;     /* 0x06 */
    u16 ascent;        /* 0x08 */
    u16 descent;       /* 0x0A */
    u16 width;         /* 0x0C */
    u16 leading;       /* 0x0E */
    u16 cellWidth;     /* 0x10 */
    u16 cellHeight;    /* 0x12 */
    u32 sheetSize;     /* 0x14 */
    u16 sheetFormat;   /* 0x18 GX_TF_I4 or GX_TF_IA4 */
    u16 sheetColumn;   /* 0x1A */
    u16 sheetRow;      /* 0x1C */
    u16 sheetWidth;    /* 0x1E */
    u16 sheetHeight;   /* 0x20 */
    u16 widthTable;    /* 0x22 offset from the header */
    u32 sheetImage;    /* 0x24 offset from the header */
    u32 sheetFullSize; /* 0x28 */
    u8 c0, c1, c2, c3; /* 0x2C 2 bpp -> 4 bpp intensity */
} FontHeader;

/* A text box of a GLH0 string table (STH0) entry */
typedef struct FontBox {
    u32 tag;           /* 0x00 */
    u16 x;             /* 0x04 */
    u16 y;             /* 0x06 */
    u16 w;             /* 0x08 */
    u16 h;             /* 0x0A */
    u8 padC[0x14 - 0xC];
    u8 halign;         /* 0x14 bit 7 used elsewhere */
    u8 valign;         /* 0x15 */
    s16 charSpace;     /* 0x16 pixels */
    s16 lineSpace;     /* 0x18 pixels */
    u16 size;          /* 0x1A */
    u8 pad1C[0x20 - 0x1C];
    s32 ofs;           /* 0x20 string, from the box */
} FontBox;

#define FONT_MAX_LINES 64
#define FONT_STR(box) ((u8 *)(box) + (box)->ofs)

BOOL __OSReadROM(void *buf, s32 length, s32 offset);

void Font_DrawChar(u16 x, u16 y, s32 size, s32 index);

static u8 *sSheet;
static u8 *sWidth;
static s32 sCharsInSheet;
static s32 sLoadedSheet;
static u32 sColorTop;
static u32 sColorBottom;
static u32 sShadowTop;
static u32 sShadowBottom;

static FontHeader sFont;
static GXTexObj sFontTex;

#define READ_BE32(p) ((p)[0] << 24 | (p)[1] << 16 | (p)[2] << 8 | (p)[3])

/* 0x81307FB8: same Yay0 decoder as lang.c */
static void Font_Decode(u8 *src, u8 *dst) {
    s32 size;
    s32 codeOfs;
    s32 dstOfs;
    s32 linkOfs;
    s32 chunkOfs;
    u32 bits;
    u32 code;

    dstOfs = 0;
    bits = 0;
    size = READ_BE32(src + 4);
    linkOfs = READ_BE32(src + 8);
    chunkOfs = READ_BE32(src + 12);
    codeOfs = 16;

    do {
        if (bits == 0) {
            code = READ_BE32(src + codeOfs);
            codeOfs += 4;
            bits = 32;
        }
        if (code & 0x80000000) {
            dst[dstOfs++] = src[chunkOfs++];
        } else {
            s32 link = src[linkOfs] << 8 | src[linkOfs + 1];
            s32 back;
            s32 hi;
            s32 n;
            s32 i;

            linkOfs += 2;
            back = dstOfs - (link & 0xFFF);
            hi = link >> 12;
            if (hi == 0) {
                n = src[chunkOfs++] + 0x12;
            } else {
                n = hi + 2;
            }
            for (i = n; i > 0; i--) {
                dst[dstOfs] = dst[back - 1];
                dstOfs++;
                back++;
            }
        }
        code <<= 1;
        bits--;
    } while (dstOfs < size);
}

/* 0x81308184 */
static void Font_ReadROM(void *buf, s32 length, s32 offset) {
    u8 *p = buf;
    s32 i;
    s32 len;

    for (i = 0; i < length; i += 0x100) {
        len = length - i;
        __OSReadROM(p, len < 0x100 ? len : 0x100, offset + i);
        p += 0x100;
    }
}

static void Font_ExpandSheet(u8 *src, u8 *dst);

static inline u8 *Font_Read(void) {
    u8 *raw;
    u8 *font;
    s32 size;
    u32 alloc;

    if (Lang_GetCode() == 2) {
        raw = HeapAlloc(0x4D000);
        Font_ReadROM(raw, 0x4D000, 0x1AFF00);
    } else {
        raw = HeapAlloc(0x3000);
        Font_ReadROM(raw, 0x3000, 0x1FCF00);
    }
    if (raw[0] == 'Y' && raw[1] == 'a' && raw[2] == 'y') {
        size = READ_BE32(raw + 4);
    } else {
        size = 0;
    }
    if (size > 0) {
        alloc = (size + 0x1F) & ~0x1F;
        font = HeapAlloc(alloc);
        Font_Decode(raw, font);
        DCStoreRange(font, alloc);
        HeapFree(raw);
    }
    return font;
}

/* 0x81308208 */
void Font_Load(void) {
    u8 *font = Font_Read();

    sFont = *(FontHeader *)font;
    if (sSheet == NULL) {
        sSheet = HeapAlloc(sFont.sheetFullSize);
        Font_ExpandSheet(font + sFont.sheetImage, sSheet);
    }
    sCharsInSheet = sFont.sheetColumn * sFont.sheetRow;
    if (sWidth == NULL) {
        s32 n = sFont.sheetImage - sFont.widthTable;
        sWidth = HeapAlloc((n + 0x1F) & ~0x1F);
        memset(sWidth, 0, (n + 0x1F) & ~0x1F);
        memcpy(sWidth, font + sFont.widthTable, n);
    }
    HeapFree(font);
}

/* 0x8130841C: 2 bpp sheet to I4 or IA4 */
static void Font_ExpandSheet(u8 *src, u8 *dst) {
    u8 *c = &sFont.c0;
    u32 i;

    if (sFont.sheetFormat == GX_TF_I4) {
        for (i = 0; i < sFont.sheetFullSize / 2; i++) {
            dst[i * 2 + 0] = c[src[i] >> 6 & 3] & 0xF0 | c[src[i] >> 4 & 3] & 0x0F;
            dst[i * 2 + 1] = c[src[i] >> 2 & 3] & 0xF0 | c[src[i] >> 0 & 3] & 0x0F;
        }
    } else if (sFont.sheetFormat == GX_TF_IA4) {
        for (i = 0; i < sFont.sheetFullSize / 4; i++) {
            dst[i * 4 + 0] = c[src[i] >> 6 & 3];
            dst[i * 4 + 1] = c[src[i] >> 4 & 3];
            dst[i * 4 + 2] = c[src[i] >> 2 & 3];
            dst[i * 4 + 3] = c[src[i] >> 0 & 3];
        }
    }
    DCStoreRange(dst, sFont.sheetFullSize);
}

s32 Font_Index(s32 code);

static inline void Font_PutChar(u16 x, u16 y, s32 size, s32 code) {
    Font_DrawChar(x, y, size, Font_Index((u16)code));
}

static inline u16 Font_LineX(FontBox *box, u16 *lineW, s32 line) {
    u16 x0;

    switch (box->halign & 0x7F) {
    case 0:
        x0 = box->x - lineW[line] / 2;
        break;
    case 2:
        x0 = box->x - box->w / 2;
        break;
    case 1:
        x0 = box->x + box->w / 2 - lineW[line];
        break;
    }
    return x0;
}

/* 0x81308538: draw the lines of a box measured by Font_Measure */
u16 Font_DrawLines(FontBox *box, u8 *str, u16 *lineW, s32 height, s32 maxChars, s32 maxLines, BOOL wrap,
                   BOOL stopAtWrap) {
    s32 lines = 0;
    s32 chars = 0;
    u16 x = 0;
    s32 w = box->w & 0xFFFF;
    u16 code;
    u16 x0;
    s32 y;
    u16 nx;
    BOOL over;

    switch (box->valign) {
    case 0:
        y = box->y - height / 2;
        break;
    case 2:
        y = box->y - box->h / 2;
        break;
    case 1:
        y = box->y + box->h / 2 - height;
        break;
    }
    x0 = Font_LineX(box, lineW, 0);
    y += sFont.ascent * box->size * 16 / sFont.cellHeight;

    while ((code = *str++) != 0) {
        if (Lang_GetCode() == 2 && ((code >= 0x80 && code <= 0x9F) || (code >= 0xE0 && code <= 0xFF))) {
            if (*str != 0) {
                code = code << 8 | *str;
                chars++;
                str++;
            }
        }
        if (code == '\n') {
            if (wrap) {
                lines++;
                x0 = Font_LineX(box, lineW, lines);
                y += box->lineSpace * 16;
                x = 0;
                if (maxLines != 0 && lines >= maxLines) {
                    break;
                }
            }
        } else {
            nx = box->charSpace * 16 + box->size * (sWidth[Font_Index(code)] * 16) / sFont.cellWidth + x;
            if (box->charSpace > 0) {
                if (nx - box->charSpace * 16 > w) {
                    over = TRUE;
                } else {
                    goto fits2;
                }
            } else if (nx > w) {
                over = TRUE;
            } else {
            fits2:
                over = FALSE;
            }
            if (over) {
                if (wrap) {
                    lines++;
                    x0 = Font_LineX(box, lineW, lines);
                    x = 0;
                    y += box->lineSpace * 16;
                    if (*str == '\n') {
                        str++;
                    }
                }
                if (stopAtWrap) {
                    break;
                }
                if (maxLines != 0 && lines >= maxLines) {
                    break;
                }
            }
            Font_PutChar(x0 + x, y, box->size, code);
            chars++;
            x += box->charSpace * 16 + box->size * (sWidth[Font_Index(code)] * 16) / sFont.cellWidth;
        }
        if (maxChars != 0 && chars >= maxChars) {
            break;
        }
        if (maxLines != 0 && lines >= maxLines) {
            break;
        }
    }
    return x;
}

/* 0x81308978: close a line; a trailing SJIS comma or period hangs out */
void Font_EndLine(FontBox *box, u16 last, u16 *curW, u16 *lineW, s32 *line, s32 *height, BOOL next) {
    DispSettings *disp = Disp_GetSettings();
    u16 w = *curW;

    if (w != 0) {
        w -= box->charSpace * 16;
        if ((u16)(last - 0x8141) <= 3) {
            w -= box->size * (disp->punctHang * sWidth[Font_Index(last)]) / sFont.cellWidth;
        }
    }
    lineW[*line] = w;
    if (next) {
        (*line)++;
        if (*line >= FONT_MAX_LINES) {
            *height = 0;
        } else {
            *height += box->lineSpace * 16;
        }
    }
    *curW = 0;
}

/* 0x81308A78: line widths and total height of a box's text */
s32 Font_Measure(FontBox *box, u8 *str, u16 *lineW, s32 *linesOut, BOOL wrap, BOOL newline, BOOL stopAtWrap,
                 s32 maxChars, s32 maxLines) {
    u16 size = box->size;
    s32 height = sFont.ascent * size * 16 / sFont.cellHeight + sFont.descent * size * 16 / sFont.cellHeight;
    s32 cs;
    s32 line = 0;
    u16 curW = 0;
    s32 w = box->w & 0xFFFF;
    u16 code;
    u16 last = 0;
    s32 chars = 0;
    u16 nw;
    BOOL over;
    u16 pad;

    while ((code = *str++) != 0) {
        if (Lang_GetCode() == 2 && ((code >= 0x80 && code <= 0x9F) || (code >= 0xE0 && code <= 0xFF))) {
            if (*str != 0) {
                code = code << 8 | *str;
                chars++;
                str++;
            }
        }
        if (code == '\n') {
            if (newline) {
                Font_EndLine(box, last, &curW, lineW, &line, &height, TRUE);
                if (height == 0) {
                    return 0;
                }
                if (maxLines != 0 && line >= maxLines) {
                    break;
                }
            }
        } else {
            last = code;
            nw = box->charSpace * 16 + box->size * (sWidth[Font_Index(code)] * 16) / sFont.cellWidth + curW;
            cs = box->charSpace;
            if (cs > 0) {
                if (nw - cs * 16 > w) {
                    over = TRUE;
                } else {
                    goto fits;
                }
            } else if (nw > w) {
                over = TRUE;
            } else {
            fits:
                over = FALSE;
            }
            if (over) {
                if (wrap) {
                    if (*str == '\n') {
                        str++;
                    }
                    Font_EndLine(box, code, &curW, lineW, &line, &height, TRUE);
                    if (height == 0) {
                        return 0;
                    }
                }
                if (stopAtWrap) {
                    break;
                }
                if (maxLines != 0 && line >= maxLines) {
                    break;
                }
            }
            curW = box->charSpace * 16 + box->size * (sWidth[Font_Index(code)] * 16) / sFont.cellWidth + curW;
            chars++;
        }
        last = code;
        if (maxChars != 0 && chars >= maxChars) {
            break;
        }
        if (maxLines != 0 && line >= maxLines) {
            line--;
            height -= box->lineSpace * 16;
            break;
        }
    }
    if (maxLines != 0 && line >= maxLines) {
        line--;
        height -= box->lineSpace * 16;
    } else {
        Font_EndLine(box, last, &curW, lineW, &line, &height, FALSE);
    }
    if (linesOut != NULL) {
        *linesOut = line;
    }
    return height;
}

#pragma auto_inline on

/* 0x81308DB8 */
void fn_81308DB8(FontBox *box, u8 *str) {
    u16 lineW[FONT_MAX_LINES];
    s32 lines;
    u16 height;

    if (str == NULL) {
        str = FONT_STR(box);
    }
    height = Font_Measure(box, str, lineW, &lines, TRUE, TRUE, FALSE, 0, 0);
    if (height != 0) {
        Font_DrawLines(box, str, lineW, height, 0, 0, TRUE, FALSE);
    }
}

/* 0x81308E50 */
void Font_DrawBoxN(FontBox *box, u8 *str, s32 maxChars) {
    u16 lineW[FONT_MAX_LINES];
    s32 lines;
    u16 height;

    if (str == NULL) {
        str = FONT_STR(box);
    }
    height = Font_Measure(box, str, lineW, &lines, TRUE, TRUE, FALSE, maxChars, 0);
    if (height != 0) {
        Font_DrawLines(box, str, lineW, height, maxChars, 0, TRUE, FALSE);
    }
}

/* 0x81308EF4 */
void Font_DrawBoxLine1(FontBox *box, u8 *str, s32 maxChars) {
    u16 lineW[FONT_MAX_LINES];
    s32 lines;
    u16 height;

    if (str == NULL) {
        str = FONT_STR(box);
    }
    height = Font_Measure(box, str, lineW, &lines, TRUE, TRUE, FALSE, maxChars, 1);
    if (height != 0) {
        Font_DrawLines(box, str, lineW, height, maxChars, 1, TRUE, FALSE);
    }
}

/* 0x81308F98 */
void Font_DrawBoxLine2(FontBox *box, u8 *str, s32 maxChars) {
    u16 lineW[FONT_MAX_LINES];
    s32 lines;
    u16 height;

    if (str == NULL) {
        str = FONT_STR(box);
    }
    height = Font_Measure(box, str, lineW, &lines, TRUE, TRUE, FALSE, maxChars, 2);
    if (height != 0) {
        Font_DrawLines(box, str, lineW, height, maxChars, 2, TRUE, FALSE);
    }
}

/* 0x8130903C: step the shadow pulse */
void Font_StepPulse(void) {
    DispSettings *disp = Disp_GetSettings();

    disp->pulse += disp->pulseStep;
    if (disp->pulse > disp->pulseLen) {
        disp->pulse = disp->pulseLen;
        disp->pulseStep = -1;
    }
    if (disp->pulse < 0) {
        disp->pulse = 0;
        disp->pulseStep = 1;
    }
}

static inline u32 Font_GlowColor(u32 c, u8 alpha) {
    return (c & 0xFFFFFF00) | (u8)((c & 0xFF) * alpha / 255) / 6;
}

/* 0x813090A4: glow under the text: 5x5 faint copies, then the text */
void fn_813090A4(FontBox *box, u8 *str) {
    FontBox tmp = *box;
    u32 top = sColorTop;
    u32 bottom = sColorBottom;
    DispSettings *disp = Disp_GetSettings();
    u8 alpha = disp->glowMin + disp->pulse * (disp->glowMax - disp->glowMin) / disp->pulseLen;
    s32 i;
    s32 j;
    u16 x;
    s32 y;

    sColorTop = (sShadowTop & 0xFFFFFF00) | (u8)((sShadowTop & 0xFF) * alpha / 255) / 6;
    sColorBottom = Font_GlowColor(sShadowBottom, alpha);
    x = tmp.x;
    y = tmp.y;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            tmp.x = x - 0x40 + i * 0x20;
            tmp.y = y - 0x40 + j * 0x20;
            fn_81308DB8(&tmp, str);
        }
    }
    tmp.x = x;
    tmp.y = y;
    sColorTop = top;
    sColorBottom = bottom;
    fn_81308DB8(&tmp, str);
}

#pragma auto_inline reset

/* 0x813092D8 */
void Glh_MeasureString(FontBox *box, u8 *str, u16 *w, u16 *h) {
    u16 lineW[FONT_MAX_LINES];
    s32 lines;
    s32 i;

    *w = 0;
    if (str == NULL) {
        str = FONT_STR(box);
    }
    *h = Font_Measure(box, str, lineW, &lines, FALSE, TRUE, FALSE, 0, 0);
    for (i = 0; i <= lines; i++) {
        if (*w < lineW[i]) {
            *w = lineW[i];
        }
    }
}

/* 0x81309380 */
void Font_DrawChar(u16 x, u16 y, s32 size, s32 index) {
    s32 left;
    s32 sheet;
    s32 right;
    s32 top;
    s32 bottom;
    s32 s0;
    s32 t0;
    s32 s1;
    s32 t1;
    s32 n;
    s32 tx;
    s32 ty;

    sheet = index / sCharsInSheet;
    n = index % sCharsInSheet;
    tx = n % sFont.sheetColumn * sFont.cellWidth;
    ty = n / sFont.sheetColumn * sFont.cellHeight;
    top = y - sFont.ascent * size * 16 / sFont.cellHeight;
    s0 = (tx << 14) / sFont.sheetWidth;
    t0 = (ty << 14) / sFont.sheetHeight;
    s1 = ((tx + sFont.cellWidth) << 14) / sFont.sheetWidth;
    t1 = ((ty + sFont.cellHeight) << 14) / sFont.sheetHeight;
    bottom = y + sFont.descent * size * 16 / sFont.cellHeight;
    left = x;
    right = left + size * 16;

    if (sheet != sLoadedSheet) {
        GXInitTexObj(&sFontTex, sSheet + sFont.sheetSize * sheet, sFont.sheetWidth, sFont.sheetHeight,
                     sFont.sheetFormat, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXInitTexObjLOD(&sFontTex, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
        sLoadedSheet = sheet;
    }
    GXLoadTexObj(&sFontTex, GX_TEXMAP0);
    GXBegin(GX_QUADS, GX_VTXFMT1, 4);
    GXPosition3s16(left, top, 0);
    GXColor1u32(sColorTop);
    GXTexCoord2u16(s0, t0);
    GXPosition3s16(right, top, 0);
    GXColor1u32(sColorTop);
    GXTexCoord2u16(s1, t0);
    GXPosition3s16(right, bottom, 0);
    GXColor1u32(sColorBottom);
    GXTexCoord2u16(s1, t1);
    GXPosition3s16(left, bottom, 0);
    GXColor1u32(sColorBottom);
    GXTexCoord2u16(s0, t1);
    GXEnd();
}

/* 0x81309548 */
void Font_SetColorU32(u32 top, u32 bottom) {
    sColorTop = top;
    sColorBottom = bottom;
}

#define COLOR_U32(c) ((c)->r << 24 | (c)->g << 16 | (c)->b << 8 | (c)->a)

/* 0x81309554 */
void fn_81309554(GXColor *top, GXColor *bottom) {
    sColorTop = COLOR_U32(top);
    sColorBottom = COLOR_U32(bottom);
}

/* 0x813095A0 */
void fn_813095A0(GXColor *top, GXColor *bottom) {
    sShadowTop = COLOR_U32(top);
    sShadowBottom = COLOR_U32(bottom);
}

/* 0x813095EC */
void Font_Setup(void) {
    sLoadedSheet = -1;
    sColorTop = 0xFFFFFFFF;
    sColorBottom = 0xFFFFFFFF;
}

/* 0x81309600: ASCII/Latin-1 code to sheet index */
s32 Font_Index(s32 code) {
    if (code > 0x20 && code <= 0xFF) {
        return code - 0x20;
    }
    return 0;
}
