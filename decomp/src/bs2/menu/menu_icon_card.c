// Memory Card icon (PAL 1.0 BS2, 0x813218C8..0x813230F0): a 6 x 6 grid of
// small cubes behind the Memory Card submenu. The 24 cubes of the outer ring
// fly in to their grid places at random times, flip over in diagonal waves
// and swing the grid around, while the 12 inner cubes fade in from the top
// left corner.

#include "bs2/menu.h"

#define CARDICON_CUBES 36

static J3DModelInstance sCardIconInst[CARDICON_CUBES];
static CardIconCube sCardCube[CARDICON_CUBES];

static s32 sCardIconFrame;      /* runs 0..490, then loops from 210 */
static s16 sCardIconRotY;       /* swing of the whole grid */
static GXColorS10 sCardIconTint;
static BOOL sCardIconSettled;

/* 0x813218C8 */
void CardIcon_Init(J3DModel* model, BOOL first) {
    s16 i;

    if (first) {
        for (i = 0; i < CARDICON_CUBES; i++) {
            sCardIconInst[i].model = model;
            J3D_InitModel(&sCardIconInst[i], 0);
        }
    }
    for (i = 0; i < CARDICON_CUBES; i++) {
        sCardIconInst[i].alpha = 0;
    }
    sCardIconFrame = 0;
    sCardIconRotY = 0;
    sCardIconSettled = FALSE;
    CardIcon_InitGrid();
}

/* 0x813219F4: cube i flies from the centre to its place in the 6 x 6 grid. */
/* each cube starts flying out at a random frame 0..31 and takes 20 frames */
static inline void CardIcon_RandomStarts(void) {
    s16 i;
    s16 s;

    for (i = 0; i < CARDICON_CUBES; i++) {
        s = (s16)(s32)(Rand() & 0x1F);
        sCardCube[i].start = s;
        sCardCube[i].end = s + 20;
    }
}

static inline void CardIcon_ResetSpin(void) {
    s16 i;

    for (i = 0; i < CARDICON_CUBES; i++) {
        sCardCube[i].spin = 0;
    }
}

void CardIcon_InitGrid(void) {
    s16 i;

    for (i = 0; i < CARDICON_CUBES; i++) {
        sCardCube[i].from[0] = 0;
        sCardCube[i].from[1] = 0;
        sCardCube[i].from[2] = 0;
        sCardCube[i].to[0] = (i % 6) * 20 - 50;
        sCardCube[i].to[1] = 50 - (i / 6) * 20;
        sCardCube[i].to[2] = 0;
        sCardCube[i].flag18 = 0;
        sCardCube[i].flag19 = 0x80;
        sCardCube[i].pos[0] = sCardCube[i].from[0];
        sCardCube[i].pos[1] = sCardCube[i].from[1];
        sCardCube[i].pos[2] = sCardCube[i].from[2];
        sCardIconFrame = 0;
    }
    CardIcon_RandomStarts();
    CardIcon_ResetSpin();
}

/* 0x81321B98: the 4 x 3 block in the middle of the grid. */
BOOL CardIcon_IsInner(s16 i) {
    if ((i >= 7 && i <= 10) || (i >= 13 && i <= 16) || (i >= 19 && i <= 22)) {
        return TRUE;
    }
    return FALSE;
}

/* 0x81321BE4: diagonal of an outer cube, counted from the top left corner. */
s16 CardIcon_Diagonal(s16 i) {
    switch (i) {
    case 0:
        return 0;
    case 1:
    case 6:
        return 1;
    case 2:
    case 12:
        return 2;
    case 3:
    case 18:
        return 3;
    case 4:
    case 24:
        return 4;
    case 5:
    case 25:
    case 30:
        return 5;
    case 11:
    case 26:
    case 31:
        return 6;
    case 17:
    case 27:
    case 32:
        return 7;
    case 23:
    case 28:
    case 33:
        return 8;
    case 29:
    case 34:
        return 9;
    case 35:
        return 10;
    default:
        return 0;
    }
}

/* 0x81321C68: fly the cubes in and fade the outer ones up on the way. */
static inline f32 CardIcon_Ease(f32 t, f32 t0, f32 p0, f32 t1, f32 p1) {
    return Hermite(t, t0, p0, (p1 - p0) / (t1 - t0), t1, p1, 0.0f);
}

void CardIcon_UpdateFly(void) {
    s16 i;
    s16 start;
    f32 dur;
    f32 x0;
    f32 y0;
    f32 x1;
    f32 t0;
    f32 y1;
    f32 t1;

    for (i = 0; i < CARDICON_CUBES; i++) {
        if (sCardIconFrame <= sCardCube[i].start) {
            sCardCube[i].pos[0] = sCardCube[i].from[0];
            sCardCube[i].pos[1] = sCardCube[i].from[1];
            sCardCube[i].pos[2] = sCardCube[i].from[2];
        }
        if (sCardIconFrame >= sCardCube[i].end) {
            sCardCube[i].pos[0] = sCardCube[i].to[0];
            sCardCube[i].pos[1] = sCardCube[i].to[1];
            sCardCube[i].pos[2] = sCardCube[i].to[2];
        }
        if (sCardIconFrame > sCardCube[i].start) {
            if (sCardIconFrame < sCardCube[i].end) {
                x0 = sCardCube[i].from[0];
                y0 = sCardCube[i].from[1];
                x1 = sCardCube[i].to[0];
                y1 = sCardCube[i].to[1];
                t0 = sCardCube[i].start;
                t1 = sCardCube[i].end;
                dur = t1 - t0;
                sCardCube[i].pos[0] = Hermite(sCardIconFrame, t0, x0, (x1 - x0) / dur, t1, x1, 0.0f);
                sCardCube[i].pos[1] = Hermite(sCardIconFrame, t0, y0, (y1 - y0) / dur, t1, y1, 0.0f);
                sCardCube[i].pos[2] = 0;
            }
        }
    }
    for (i = 0; i < CARDICON_CUBES; i++) {
        if (!CardIcon_IsInner(i)) {
            start = sCardCube[i].start;
            if (sCardIconFrame > start && sCardIconFrame < sCardCube[i].end) {
                sCardIconInst[i].alpha = (sCardIconFrame - start) * 255 / (sCardCube[i].end - start);
            } else if (sCardIconFrame <= start) {
                sCardIconInst[i].alpha = 0;
            } else if (sCardIconFrame >= sCardCube[i].end) {
                sCardIconInst[i].alpha = 255;
            }
        }
    }
}

static inline void CardIcon_AddAlpha(int i, s16 step) {
    s16 alpha;

    sCardIconInst[i].alpha += step;
    alpha = sCardIconInst[i].alpha;
    if (alpha > 255) {
        alpha = 255;
    }
    if (alpha < 0) {
        alpha = 0;
    }
    sCardIconInst[i].alpha = alpha;
}

/* 0x81321F60: the inner block fades in one diagonal after the other, and
   back out all at once. */
void CardIcon_FadeInner(BOOL in) {
    if (in) {
        CardIcon_AddAlpha(7, 30);
        if (sCardIconInst[7].alpha > 100) {
            CardIcon_AddAlpha(8, 30);
            CardIcon_AddAlpha(13, 30);
        }
        if (sCardIconInst[8].alpha > 100) {
            CardIcon_AddAlpha(9, 30);
            CardIcon_AddAlpha(14, 30);
            CardIcon_AddAlpha(19, 30);
        }
        if (sCardIconInst[9].alpha > 100) {
            CardIcon_AddAlpha(10, 30);
            CardIcon_AddAlpha(15, 30);
            CardIcon_AddAlpha(20, 30);
        }
        if (sCardIconInst[10].alpha > 100) {
            CardIcon_AddAlpha(16, 30);
            CardIcon_AddAlpha(21, 30);
        }
        if (sCardIconInst[16].alpha > 100) {
            CardIcon_AddAlpha(22, 30);
        }
    } else {
        CardIcon_AddAlpha(7, -15);
        CardIcon_AddAlpha(8, -15);
        CardIcon_AddAlpha(13, -15);
        CardIcon_AddAlpha(9, -15);
        CardIcon_AddAlpha(14, -15);
        CardIcon_AddAlpha(19, -15);
        CardIcon_AddAlpha(10, -15);
        CardIcon_AddAlpha(15, -15);
        CardIcon_AddAlpha(20, -15);
        CardIcon_AddAlpha(16, -15);
        CardIcon_AddAlpha(21, -15);
        CardIcon_AddAlpha(22, -15);
    }
}

void CardIcon_SpinY(s32 t) {
    s16 t0;
    s16 t1;

    if (t < 50) {
        t1 = 50;
        sCardIconRotY = Hermite(t, 0.0f, 0.0f, 0.0f, t1, -8000.0f, 0.0f);
    } else if (t < 90) {
        t1 = 90;
        t0 = 50;
        sCardIconRotY = Hermite(t, t0, -8000.0f, 0.0f, t1, 32768.0f, 0.0f);
    } else if (t < 140) {
        sCardIconRotY = -0x8000;
    } else if (t < 190) {
        t1 = 190;
        t0 = 140;
        sCardIconRotY = Hermite(t, t0, 32768.0f, 0.0f, t1, 40768.0f, 0.0f);
    } else if (t < 230) {
        t1 = 230;
        t0 = 190;
        sCardIconRotY = Hermite(t, t0, 40768.0f, 0.0f, t1, 0.0f, 0.0f);
    } else if (t < 280) {
        sCardIconRotY = 0;
    }
}

static inline void CardIcon_Flip(s32 t) {
    s16 i;

    for (i = 0; i < CARDICON_CUBES; i++) {
        if (!CardIcon_IsInner(i) && t >= CardIcon_Diagonal(i) * 3 && t <= CardIcon_Diagonal(i) * 3 + 15) {
            sCardCube[i].spin = ((t - CardIcon_Diagonal(i) * 3) << 14) / 15;
        }
    }
}

/* 0x81322650 */

void CardIcon_UpdateFace(void) {
    s16 i;

    Card_GetParams();
    if (sCardIconFrame == 0) {
        CardIcon_RandomStarts();
    }
    if (sCardIconFrame <= 113) {
        CardIcon_UpdateFly();
    }
    if (sCardIconFrame >= 150 && sCardIconFrame <= 195) {
        CardIcon_Flip(sCardIconFrame - 150);
    } else if (sCardIconFrame >= 300 && sCardIconFrame <= 345) {
        CardIcon_Flip(sCardIconFrame - 300);
    } else if (sCardIconFrame >= 440 && sCardIconFrame <= 485) {
        CardIcon_Flip(sCardIconFrame - 430);
    } else {
        CardIcon_ResetSpin();
    }
    if (sCardIconFrame >= 340) {
        CardIcon_FadeInner(TRUE);
    } else {
        CardIcon_FadeInner(FALSE);
    }
    if (sCardIconFrame >= 210 && sCardIconFrame <= 490) {
        CardIcon_SpinY(sCardIconFrame - 210);
    }
    sCardIconFrame++;
    if (sCardIconFrame > 490) {
        sCardIconFrame = 210;
    }
}

/* Passing the scale through VecRef gives the IPL's stack layout for the
 * by-value Vec argument (rodata copy, then the argument copy above it). */
static inline Vec* VecRef(Vec* v) {
    (void)v;
    return v;
}

static inline void CardIcon_SetInnerMtx(J3DModelInstance* inst, Mtx m, CardParams* p) {
    Vec scale = { 0.0f, 0.0f, 0.0f };

    scale.z = scale.y = scale.x = p->cubeScale / 2.0f;
    J3D_SetModelMtx(inst, m, *VecRef(&scale));
}

static inline void CardIcon_SetOuterMtx(J3DModelInstance* inst, Mtx m, CardParams* p) {
    Vec scale = { 0.0f, 0.0f, 0.0f };

    scale.z = scale.y = scale.x = p->cubeScale;
    J3D_SetModelMtx(inst, m, *VecRef(&scale));
}

/* 0x81322A18 */
void CardIcon_CalcMtx(void) {
    CardParams* p = Card_GetParams();
    Mtx m;
    Mtx rot;
    Mtx tilt;
    s16 i;

    MtxRotTrans(0x4000, 0, 0, tilt, 0.0f, 0.0f, 0.0f);
    for (i = 0; i < CARDICON_CUBES; i++) {
        MtxRotTrans(sCardCube[i].spin, sCardCube[i].spin, 0, m, sCardCube[i].pos[0], sCardCube[i].pos[1],
                    sCardCube[i].pos[2]);
        MtxRot(0, sCardIconRotY, 0, rot);
        PSMTXConcat(rot, m, m);
        PSMTXConcat(tilt, m, m);
        PSMTXConcat(Menu_GetCubeMtx(), m, m);
        if (CardIcon_IsInner(i)) {
            CardIcon_SetInnerMtx(&sCardIconInst[i], m, p);
        } else {
            CardIcon_SetOuterMtx(&sCardIconInst[i], m, p);
        }
        J3D_SetViewMtx(&sCardIconInst[i], Camera_GetView());
        J3D_CalcJoints(&sCardIconInst[i]);
    }
}

/* 0x81322C8C */
void CardIcon_Draw(void) {
    CardParams* p = Card_GetParams();
    s16 i;
    void* tev;

    sCardIconTint.r = p->tint[0];
    sCardIconTint.g = p->tint[1];
    sCardIconTint.b = p->tint[2];
    sCardIconTint.a = p->tint[3];
    tev = sCardIconInst[0].model->material->tevColor;
    sCardIconInst[0].model->material->tevColor = &sCardIconTint;

    for (i = 0; i < CARDICON_CUBES; i++) {
        if (sCardIconInst[i].alpha > 0) {
            J3D_Draw(&sCardIconInst[i]);
        }
    }

    sCardIconInst[0].model->material->tevColor = tev;
}

/* 0x81322D58: fade every cube out while the submenu closes. */
void CardIcon_UpdateHide(void) {
    s32 i;
    s32 hidden = 0; /* counted, but never used */

    for (i = 0; i < CARDICON_CUBES; i++) {
        sCardIconInst[i].alpha -= 10;
        if (sCardIconInst[i].alpha < 0) {
            sCardIconInst[i].alpha = 0;
            hidden++;
        }
    }
    sCardIconFrame = 0;
    sCardIconRotY = 0;
}

/* 0x81322EE8: pull every cube back to the centre of the grid. */
void CardIcon_UpdateEnter(void) {
    s32 i;
    s32 settled = 0;
    CardParams* p = Card_GetParams();
    s16 v;

#define CARD_SHRINK(n)                                                  \
    v = sCardCube[i].pos[n];                                            \
    p2 = Card_GetParams();                                              \
    if (v > 0) {                                                        \
        v -= p2->collapseSpeed;                                         \
        if (v < 0) {                                                    \
            v = 0;                                                      \
        }                                                               \
    } else if (v < 0) {                                                 \
        v += p2->collapseSpeed;                                         \
        if (v > 0) {                                                    \
            v = 0;                                                      \
        }                                                               \
    }                                                                   \
    sCardCube[i].pos[n] = v;                                            \
    if ((v == 0 ? TRUE : FALSE) && sCardIconInst[i].alpha == 0) {       \
        settled++;                                                      \
    }

    for (i = 0; i < CARDICON_CUBES; i++) {
        CardParams* p2;

        sCardIconInst[i].alpha -= p->fadeOutSpeed;
        if (sCardIconInst[i].alpha < 0) {
            sCardIconInst[i].alpha = 0;
        }
        CARD_SHRINK(0)
        CARD_SHRINK(1)
        CARD_SHRINK(2)
    }
#undef CARD_SHRINK
    if (settled == CARDICON_CUBES * 3) {
        sCardIconSettled = TRUE;
    } else {
        sCardIconSettled = FALSE;
    }
    sCardIconFrame = 0;
    sCardIconRotY = 0;
}

/* 0x813230E8 */
BOOL CardIcon_IsSettled(void) {
    return sCardIconSettled;
}
