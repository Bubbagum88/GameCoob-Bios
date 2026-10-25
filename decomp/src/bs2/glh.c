#define BS2_GLH_C
#include "bs2/menu.h"
#include "bs2/glh.h"

/*
 * GLH0 layout drawer (0x81309620..0x8130A86C): picture panes, 9-slice
 * frames and STH0 strings in the text panes, all from the current TXH0
 * texture bank.
 */

void J3D_LoadTexture(s32 map, void *tex); /* 0x81305F58 */

void fn_81308DB8(GlhText *box, u8 *str);
void Font_DrawBoxN(GlhText *box, u8 *str, s32 maxChars);
void Font_DrawBoxLine1(GlhText *box, u8 *str, s32 maxChars);
void Font_DrawBoxLine2(GlhText *box, u8 *str, s32 maxChars);
void fn_813090A4(GlhText *box, u8 *str);
void Glh_MeasureString(GlhText *box, u8 *str, u16 *w, u16 *h);
void Font_SetColorU32(u32 top, u32 bottom);
void fn_81309554(GXColor *top, GXColor *bottom);
void fn_813095A0(GXColor *top, GXColor *bottom);
void Font_Setup(void);

static u8 *sTxh;

#define TXH_TEX(i) ((TexHeader *)(sTxh + (i) * 0x20))

/* 0x81309620 */
void Txh_SetBank(void *txh) {
    u8 *bank = (u8 *)txh + 0x20;

    if (*(u32 *)txh == TXH_MAGIC) {
        sTxh = bank;
    }
}

/* 0x8130963C */
void GX_Setup2DTex(void) {
    GX_Setup2D(1, 0, 0);
}

/* 0x81309668 */
void GX_Setup2DText(void) {
    Font_Setup();
    GX_Setup2D(1, 1, 0);
}

/* 0x81309698: one frame corner */
void Glh_DrawCorner(s16 x, s16 y, s16 w, s16 h, u16 tex, u8 flip) {
    s16 t0;
    s16 t1;
    s16 s0;
    s16 s1;

    if (flip & 1) {
        t0 = 0x4000;
        t1 = 0;
    } else {
        t0 = 0;
        t1 = 0x4000;
    }
    if (flip & 2) {
        s0 = 0x4000;
        s1 = 0;
    } else {
        s0 = 0;
        s1 = 0x4000;
    }
    J3D_LoadTexture(0, TXH_TEX(tex));
    GXBegin(GX_QUADS, GX_VTXFMT1, 4);
    GXPosition3s16(x, y, 0);
    GXTexCoord2s16(s0, t0);
    GXPosition3s16(x + w, y, 0);
    GXTexCoord2s16(s1, t0);
    GXPosition3s16(x + w, y + h, 0);
    GXTexCoord2s16(s1, t1);
    GXPosition3s16(x, y + h, 0);
    GXTexCoord2s16(s0, t1);
    GXEnd();
}

/* 0x813097A4: one frame edge, the corner texture's first texel row or column stretched */
void Glh_DrawEdge(s16 x, s16 y, s16 w, s16 h, u16 tex, u8 flip, u8 horiz) {
    s16 t0;
    s16 t1;
    s16 s0;
    s16 s1;

    if (flip & 1) {
        t0 = 0x4000;
        t1 = 0;
    } else {
        t0 = 0;
        t1 = 0x4000;
    }
    if (flip & 2) {
        s0 = 0x4000;
        s1 = 0;
    } else {
        s0 = 0;
        s1 = 0x4000;
    }
    J3D_LoadTexture(0, TXH_TEX(tex));
    if (horiz) {
        GXBegin(GX_QUADS, GX_VTXFMT1, 4);
        GXPosition3s16(x, y, 0);
        GXTexCoord2s16(s0, t0);
        GXPosition3s16(x + w, y, 0);
        GXTexCoord2s16(s0, t0);
        GXPosition3s16(x + w, y + h, 0);
        GXTexCoord2s16(s0, t1);
        GXPosition3s16(x, y + h, 0);
        GXTexCoord2s16(s0, t1);
        GXEnd();
    } else {
        GXBegin(GX_QUADS, GX_VTXFMT1, 4);
        GXPosition3s16(x, y, 0);
        GXTexCoord2s16(s0, t0);
        GXPosition3s16(x + w, y, 0);
        GXTexCoord2s16(s1, t0);
        GXPosition3s16(x + w, y + h, 0);
        GXTexCoord2s16(s1, t0);
        GXPosition3s16(x, y + h, 0);
        GXTexCoord2s16(s0, t0);
        GXEnd();
    }
}

/* 0x81309948: a 9-slice frame around the rectangle x, y, w, h */
static inline void Glh_DrawSlices(GlhFrame *f, s16 x, s16 y, s16 w, s16 h, s16 right, s16 bottom) {
    TexHeader *tex = TXH_TEX(f->tex[0]);
    s16 cw = tex->width * 16;
    s16 ch = tex->height * 16;
    s32 left = x - cw;
    s32 top = y - ch;

    Glh_DrawCorner(left, top, cw, ch, f->tex[0], f->flips >> 6);
    Glh_DrawCorner(right, top, cw, ch, f->tex[1], f->flips >> 4);
    Glh_DrawCorner(left, bottom, cw, ch, f->tex[2], f->flips >> 2);
    Glh_DrawCorner(right, bottom, cw, ch, f->tex[3], f->flips);
    Glh_DrawEdge(x, top, w, ch, f->tex[1], f->flips >> 4, TRUE);
    Glh_DrawEdge(x, bottom, w, ch, f->tex[3], f->flips, TRUE);
    Glh_DrawEdge(left, y, cw, h, f->tex[2], f->flips >> 2, FALSE);
    Glh_DrawEdge(right, y, cw, h, f->tex[3], f->flips, FALSE);
}

void Glh_DrawPaneRect(u16 pane, void *glh, GXColor *c, s32 x, s32 y, s32 w, s32 h) {
    GlhFrame *frames = GLH_FRAMES(glh);
    u8 alpha = c->a;
    u32 c0 = frames[pane].color[0];
    u32 c1 = frames[pane].color[1];
    u32 c3 = frames[pane].color[3];
    u32 c2 = frames[pane].color[2];
    u32 a0 = alpha * (c0 & 0xFF) / 255;
    u32 a1 = alpha * (c1 & 0xFF) / 255;
    u32 a2 = alpha * (c2 & 0xFF) / 255;
    u32 a3 = alpha * (c3 & 0xFF) / 255;

    if (a0 > 0xFF) {
        a0 = 0xFF;
    }
    if (a1 > 0xFF) {
        a1 = 0xFF;
    }
    if (a2 > 0xFF) {
        a2 = 0xFF;
    }
    if (a3 > 0xFF) {
        a3 = 0xFF;
    }
    GX_Setup2D(0, 1, 0);
    GXBegin(GX_QUADS, GX_VTXFMT1, 4);
    GXPosition3s16(x, y, 0);
    GXColor1u32(a0 | (c0 & 0xFFFFFF00));
    GXPosition3s16(x + w, y, 0);
    GXColor1u32(a1 | (c1 & 0xFFFFFF00));
    GXPosition3s16(x + w, y + h, 0);
    GXColor1u32(a3 | (c3 & 0xFFFFFF00));
    GXPosition3s16(x, y + h, 0);
    GXColor1u32(a2 | (c2 & 0xFFFFFF00));
    GXEnd();

    GX_Setup2D(1, 0, 0);
    GXSetChanMatColor(GX_COLOR0A0, *c);
    Glh_DrawSlices(&frames[pane], x, y, w, h, x + w, y + h);
}

static inline void Glh_DrawTextBg(u16 pane, void *glh, GXColor *c, s32 dx, s32 dy, s32 w, s32 h) {
    GlhFrame *f = GLH_FRAMES(glh);
    s32 x = dx + (f[pane].x - w / 2);
    s32 y = dy + (f[pane].y - h / 2);

    Glh_DrawPaneRect(pane, glh, c, x, y, w, h);
}

/* 0x81309C3C: every frame with this tag at its own size */
void Glh_DrawFrame(u32 tag, void *glh, GXColor *c) {
    GlhFrame *f = GLH_FRAMES(glh);
    u16 i;

    for (i = 0; i < ((Glh *)glh)->nFrames; f++, i++) {
        if (tag == f->tag) {
            Glh_DrawTextBg(i, glh, c, 0, 0, f->innerW, f->innerH);
        }
    }
}

/* 0x81309CEC: place an image of the given size along one axis of a pane */
void Glh_Fit(s32 fit, u8 flags, s32 lo, s32 mid, s32 hi, s32 size, s32 maskLo, s32 maskHi, s32 *outLo,
             s32 *outHi, s32 *texLo, s32 *texHi) {
    if ((fit & maskLo) && (fit & maskHi)) {
        *outLo = lo;
        *texLo = 0;
        *outHi = hi;
        *texHi = 0x4000;
    } else if (fit & maskLo) {
        *outLo = lo;
        *texLo = 0;
        if (!(flags & 0x18) && hi - lo > size) {
            *outHi = size;
            *texHi = 0x4000;
        } else {
            *outHi = hi;
            *texHi = ((hi - lo) << 14) / size;
        }
    } else if (fit & maskHi) {
        *outHi = hi;
        *texHi = 0x4000;
        if (!(flags & 0x18) && hi - lo > size) {
            *outLo = hi - size;
            *texLo = 0;
        } else {
            *outLo = lo;
            *texLo = ((lo + (size - hi)) << 14) / size;
        }
    } else if (!(flags & 0x18) && hi - lo > size) {
        *outLo = mid - size / 2;
        *texLo = 0;
        *outHi = *outLo + size;
        *texHi = 0x4000;
    } else {
        s32 d;

        *outLo = lo;
        d = ((hi - lo) << 13) / size;
        *texLo = 0x2000 - d;
        *outHi = hi;
        *texHi = d + 0x2000;
    }
}

/* 0x81309E54 */
void Glh_DrawPicture(u16 idx, void *glh, GXColor *c, TexHeader *tex, s16 dx, s16 dy) {
    GlhPic *pics = GLH_PICS(glh);
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 tw;
    s32 th;
    s32 s0;
    s32 t0;
    s32 s1;
    s32 t1;
    s32 px0;
    s32 py0;
    s32 px1;
    s32 py1;
    s32 tmp;

    GXSetChanMatColor(GX_COLOR0A0, *c);
    x0 = dx + pics[idx].x - pics[idx].w / 2;
    y0 = dy + pics[idx].y - pics[idx].h / 2;
    x1 = x0 + pics[idx].w;
    y1 = y0 + pics[idx].h;
    if (tex != NULL) {
        if (pics[idx].flags & 4) {
            tw = tex->height;
            th = tex->width;
        } else {
            tw = tex->width;
            th = tex->height;
        }
    } else {
        tw = 0x60;
        th = 0x20;
    }
    Glh_Fit(pics[idx].fit, pics[idx].flags, y0, dy + pics[idx].y, y1, th, 2, 1, &py0, &py1, &t0, &t1);
    Glh_Fit(pics[idx].fit, pics[idx].flags, x0, dx + pics[idx].x, x1, tw, 8, 4, &px0, &px1, &s0, &s1);
    if (pics[idx].flags & 1) {
        tmp = t0;
        t0 = t1;
        t1 = tmp;
    }
    if (pics[idx].flags & 2) {
        tmp = s0;
        s0 = s1;
        s1 = tmp;
    }
    if (tex != NULL) {
        J3D_LoadTexture(0, tex);
    }
    GXSetChanMatColor(GX_COLOR0A0, *c);
    if (pics[idx].flags & 4) {
        GXBegin(GX_QUADS, GX_VTXFMT1, 4);
        GXPosition3s16(px0, py0, 0);
        GXTexCoord2u16(s0, t1);
        GXPosition3s16(px1, py0, 0);
        GXTexCoord2u16(s0, t0);
        GXPosition3s16(px1, py1, 0);
        GXTexCoord2u16(s1, t0);
        GXPosition3s16(px0, py1, 0);
        GXTexCoord2u16(s1, t1);
        GXEnd();
    } else {
        GXBegin(GX_QUADS, GX_VTXFMT1, 4);
        GXPosition3s16(px0, py0, 0);
        GXTexCoord2u16(s0, t0);
        GXPosition3s16(px1, py0, 0);
        GXTexCoord2u16(s1, t0);
        GXPosition3s16(px1, py1, 0);
        GXTexCoord2u16(s1, t1);
        GXPosition3s16(px0, py1, 0);
        GXTexCoord2u16(s0, t1);
        GXEnd();
    }
}

/* 0x8130A158 */
void Glh_DrawAll(void *glh, GXColor *c) {
    GlhPic *pics = GLH_PICS(glh);
    GlhPic *p;
    u16 i;

    for (i = 0, p = &pics[i]; i < ((Glh *)glh)->nPics; p++, i++) {
        Glh_DrawPicture(i, glh, c, TXH_TEX(pics[i].tex), 0, 0);
    }
}

/* 0x8130A1F8 */
void Glh_DrawPane(u32 tag, void *glh, GXColor *c) {
    GlhPic *p = GLH_PICS(glh);
    u16 i;

    for (i = 0; i < ((Glh *)glh)->nPics; p++, i++) {
        if (tag == p->tag) {
            Glh_DrawPicture(i, glh, c, TXH_TEX(p->tex), 0, 0);
        }
    }
}

/* 0x8130A284 */
void Glh_DrawPaneOfs(u32 tag, void *glh, GXColor *c, s16 dx, s16 dy) {
    GlhPic *p = GLH_PICS(glh);
    u16 i;
    u8 pad[8];

    for (i = 0; i < ((Glh *)glh)->nPics; p++, i++) {
        if (tag == p->tag) {
            Glh_DrawPicture(i, glh, c, TXH_TEX(p->tex), dx, dy);
        }
    }
}

/* 0x8130A318 */
void Glh_DrawPaneTex(u32 tag, void *glh, GXColor *c, void *tex) {
    GlhPic *p = GLH_PICS(glh);
    u16 i;
    u8 pad[8];

    for (i = 0; i < ((Glh *)glh)->nPics; p++, i++) {
        if (tag == p->tag) {
            Glh_DrawPicture(i, glh, c, tex, 0, 0);
        }
    }
}

/* 0x8130A39C: STH0 string idx in its text pane, moved by dx, dy; glow != NULL draws it glowing */
void Glh_DrawString(u16 idx, void *sth, void *glh, GXColor *c, GXColor *glow, s32 dx, s32 dy) {
    SthEntry *entries = ((Sth *)sth)->entry;
    GlhText *texts = GLH_TEXTS(glh);
    DispSettings *disp = Disp_GetSettings();
    u16 w;
    u16 h;

    if (((Sth *)sth)->magic == STH_MAGIC) {
        Glh_MeasureString(&texts[entries[idx].pane], (u8 *)&entries[idx] + entries[idx].ofs, &w, &h);
        if (texts[entries[idx].pane].bgPane != 0xFFFF) {
            Glh_DrawTextBg(texts[entries[idx].pane].bgPane, glh, c, dx, dy, w + disp->textPad * 32, h + disp->textPad * 32);
        }
        texts[entries[idx].pane].x += dx;
        texts[entries[idx].pane].y += dy;
        if ((texts[entries[idx].pane].halign & 0x7F) == 0) {
            texts[entries[idx].pane].w = w + 0x100;
            texts[entries[idx].pane].h = h + 0x100;
        }
        GX_Setup2DText();
        fn_81309554(c, c);
        if (glow != NULL) {
            fn_813095A0(glow, glow);
            fn_813090A4(&texts[entries[idx].pane], (u8 *)&entries[idx] + entries[idx].ofs);
        } else {
            fn_81308DB8(&texts[entries[idx].pane], (u8 *)&entries[idx] + entries[idx].ofs);
        }
        texts[entries[idx].pane].x -= dx;
        texts[entries[idx].pane].y -= dy;
    }
}

/* 0x8130A5A4 */
void Glh_DrawStringOfs(u16 idx, void *sth, void *glh, GXColor *c, s32 dx, s32 dy) {
    Glh_DrawString(idx, sth, glh, c, NULL, dx, dy);
}

/* 0x8130A5D0 */
void Glh_DrawText(u16 idx, void *sth, void *glh, GXColor *c) {
    Glh_DrawString(idx, sth, glh, c, NULL, 0, 0);
}

/* 0x8130A5FC: str typed into text pane idx, up to maxChars characters */
void Glh_DrawBannerText(u16 idx, void *glh, GXColor *c, const void *str, s32 maxChars, s32 lines) {
    u32 color = c->r << 24 | c->g << 16 | c->b << 8 | c->a;
    DispSettings *disp = Disp_GetSettings();
    GlhText *texts = GLH_TEXTS(glh);
    u16 w;
    u16 h;

    if (texts[idx].bgPane != 0xFFFF) {
        Glh_MeasureString(&texts[idx], NULL, &w, &h);
        Glh_DrawTextBg(texts[idx].bgPane, glh, c, 0, 0, w + disp->textPad * 32, h + disp->textPad * 32);
    }
    GX_Setup2DText();
    Font_SetColorU32(color, color);
    if (lines == 0) {
        Font_DrawBoxN(&texts[idx], (u8 *)str, maxChars);
    } else if (lines == 1) {
        Font_DrawBoxLine1(&texts[idx], (u8 *)str, maxChars);
    } else if (lines == 2) {
        Font_DrawBoxLine2(&texts[idx], (u8 *)str, maxChars);
    }
}

/* 0x8130A75C */
void Glh_DrawBannerLine(u32 tag, void *glh, GXColor *c, const void *str, s32 maxChars) {
    GlhText *t = GLH_TEXTS(glh);
    u16 i;

    for (i = 0; i < ((Glh *)glh)->nTexts; t++, i++) {
        if (tag == t->tag) {
            Glh_DrawBannerText(i, glh, c, str, maxChars, 1);
        }
    }
}

/* 0x8130A7E4 */
void Glh_DrawBannerBox(u32 tag, void *glh, GXColor *c, const void *str, s32 maxChars) {
    GlhText *t = GLH_TEXTS(glh);
    u16 i;

    for (i = 0; i < ((Glh *)glh)->nTexts; t++, i++) {
        if (tag == t->tag) {
            Glh_DrawBannerText(i, glh, c, str, maxChars, 2);
        }
    }
}
