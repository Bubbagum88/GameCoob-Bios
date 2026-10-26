#include "bs2/menu.h"

/* 0x8130A86C */
void Fader_Reset(Fader *f) {
    f->frame = 0;
    f->ticks = 0;
}

/* 0x8130A87C */
void Fader_Step(Fader *f, u8 mode) {
    switch (mode) {
    case FADE_IN:
        f->frame++;
        if (f->frame > f->length + f->delay) {
            f->frame = f->length + f->delay;
        }
        break;
    case FADE_OUT:
        f->frame--;
        if (f->frame < f->delay) {
            f->frame = 0;
        }
        break;
    case FADE_HALF:
        if (f->frame >= f->delay) {
            if (f->frame - f->delay > f->length / 2) {
                f->frame--;
            } else if (f->frame - f->delay < f->length / 2) {
                f->frame++;
            } else {
                f->frame = f->length / 2 + f->delay;
            }
        } else {
            f->frame++;
        }
        break;
    }
    f->ticks++;
}

/* 0x8130A964: alpha 0..maxAlpha, and the 12.4 slide offset */
void Fader_GetAlpha(Fader *f, s16 *alpha, s32 *slide) {
    if (f->frame >= f->delay) {
        *alpha = f->maxAlpha * (f->frame - f->delay) / f->length;
    } else {
        *alpha = 0;
    }
    if (slide != NULL) {
        if (f->frame >= f->delay) {
            *slide = f->slide * 16 * (f->delay + (f->length - f->frame)) / f->length;
        } else {
            *slide = f->slide * 16;
        }
    }
}

/* 0x8130A9E8: draw string f->id faded; returns the alpha used */
u8 Fader_DrawString(Fader *f, void *sth, void *glh, GXColor *c) {
    void *s;
    void *g;
    s32 slide;
    u8 a;
    s16 alpha;
    u8 pad[4];

    Fader_GetAlpha(f, &alpha, &slide);
    a = c->a;
    c->a = a * alpha / 255;
    if (c->a != 0) {
        /* late copies of sth/glh: shift the vreg order so c, alpha, a land in r29-r31 */
        Glh_DrawString(f->id, s = sth, g = glh, c, 0, 0, slide);
    }
    c->a = a;
    return alpha;
}

/* 0x8130AAEC */
void Fader_Init(Fader *f) {
    f->delay = 0;
    f->frame = 0;
    f->length = 20;
    f->maxAlpha = 255;
    f->slide = 0;
    f->id = 0;
}
