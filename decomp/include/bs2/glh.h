#ifndef BS2_GLH_H
#define BS2_GLH_H

#include "types.h"
#include <dolphin/gx.h>

/*
 * IPL 2D layout data (big-endian, positions and sizes in 12.4 fixed point).
 *
 * GLH0: a screen layout of picture panes, text panes and 9-slice frames.
 * STH0: a string table; each entry names the text pane it is drawn in.
 * TXH0: a texture bank, BTI headers 0x20 apart from offset 0x20.
 */

#define GLH_MAGIC 0x474C4830 /* "GLH0" */
#define STH_MAGIC 0x53544830 /* "STH0" */
#define TXH_MAGIC 0x54584830 /* "TXH0" */

typedef struct GlhPic {
    u32 tag;           /* 0x00 */
    u16 x;             /* 0x04 centre */
    u16 y;             /* 0x06 */
    u16 w;             /* 0x08 */
    u16 h;             /* 0x0A */
    u16 tex;           /* 0x0C TXH0 index */
    u8 fit;            /* 0x0E how the image fills the pane */
    u8 flags;          /* 0x0F bit 0 mirrors t, bit 1 mirrors s, bit 2 turns 90 degrees */
} GlhPic;

typedef struct GlhText {
    u32 tag;           /* 0x00 */
    u16 x;             /* 0x04 */
    u16 y;             /* 0x06 */
    u16 w;             /* 0x08 */
    u16 h;             /* 0x0A */
    GXColor top;       /* 0x0C */
    GXColor bottom;    /* 0x10 */
    u8 halign;         /* 0x14 0 centre, 1 right, 2 left */
    u8 valign;         /* 0x15 0 centre, 1 bottom, 2 top */
    s16 charSpace;     /* 0x16 pixels */
    s16 lineSpace;     /* 0x18 pixels */
    u16 size;          /* 0x1A */
    u16 bgPane;        /* 0x1C frame drawn behind the text, 0xFFFF none */
    u16 maxLen;        /* 0x1E */
    s32 ofs;           /* 0x20 default string, from the pane */
} GlhText;

typedef struct GlhFrame {
    u32 tag;           /* 0x00 */
    u16 x;             /* 0x04 centre */
    u16 y;             /* 0x06 */
    u16 w;             /* 0x08 */
    u16 h;             /* 0x0A */
    u16 cornerW;       /* 0x0C */
    u16 cornerH;       /* 0x0E */
    u16 innerW;        /* 0x10 */
    u16 innerH;        /* 0x12 */
    u16 tex[4];        /* 0x14 TL, TR, BL, BR */
    u8 flips;          /* 0x1C two bits per corner, TL in the top bits */
    u8 pad1D[3];
    u32 color[4];      /* 0x20 RGBA TL, TR, BL, BR */
} GlhFrame;

typedef struct Glh {
    u32 magic;         /* 0x00 GLH_MAGIC */
    s32 pics;          /* 0x04 offsets from the header */
    s32 texts;         /* 0x08 */
    s32 frames;        /* 0x0C */
    u16 nPics;         /* 0x10 */
    u16 nTexts;        /* 0x12 */
    u16 nFrames;       /* 0x14 */
} Glh;

typedef struct SthEntry {
    u16 pane;          /* 0x00 text pane */
    u16 length;        /* 0x02 */
    s32 ofs;           /* 0x04 string, from the entry */
} SthEntry;

typedef struct Sth {
    u32 magic;         /* 0x00 STH_MAGIC */
    u16 count;         /* 0x04 */
    u16 pad6;
    SthEntry entry[1]; /* 0x08 */
} Sth;

#define GLH_PICS(g) ((GlhPic *)((u8 *)(g) + ((Glh *)(g))->pics))
#define GLH_TEXTS(g) ((GlhText *)((u8 *)(g) + ((Glh *)(g))->texts))
#define GLH_FRAMES(g) ((GlhFrame *)((u8 *)(g) + ((Glh *)(g))->frames))

#endif
