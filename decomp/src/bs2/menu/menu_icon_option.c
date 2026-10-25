// Options icon (PAL 1.0 BS2, 0x81323C30..0x81326114): two groups of small
// cubes behind the Options submenu. 30 "ring" cubes form the outline of a
// 9 x 8 square; 31 "shape" cubes form a picture taken from a 9 x 8 bitmap.
// UpdateFace runs a looping timeline: the four corners fly in and draw the
// outline, the outline cubes flip one after the other, then the square is
// wiped away horizontally to reveal the shape, which bounces row by row and
// is wiped away vertically to bring the square back.

#include "bs2/menu.h"

#define OPT_RING 30
#define OPT_SHAPE 31

typedef struct OptCube {
    f32 x, y;       /* 0x00 home position */
    f32 dx, dy;     /* 0x08 drawn position */
    s16 rotX, rotY; /* 0x10 flip angles */
} OptCube;

static J3DModelInstance sOptRing[OPT_RING];
static J3DModelInstance sOptShape[OPT_SHAPE];
static OptCube sOptRingCube[OPT_RING];
static OptCube sOptShapeCube[OPT_SHAPE];

/* the picture drawn by the shape cubes, 9 columns x 8 rows */
static s8 sOptShapeBits[8 * 9] = {
    0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 1, 1,
    0, 0, 0, 0, 1, 0, 0, 1, 1,
    0, 1, 1, 1, 1, 0, 1, 1, 0,
    1, 1, 1, 1, 1, 0, 1, 0, 0,
    0, 1, 1, 1, 0, 0, 0, 0, 0,
};

static s16 sOptFrame;
static GXColorS10 sOptTint;
static BOOL sOptSettled;

void OptionIcon_ResetShape(void);

/* 0x81323C30 */
void OptionIcon_Init(J3DModel* model, BOOL first) {
    s16 i;

    if (first) {
        for (i = 0; i < OPT_RING; i++) {
            sOptRing[i].model = model;
            J3D_InitModel(&sOptRing[i], 0);
        }
        for (i = 0; i < OPT_SHAPE; i++) {
            sOptShape[i].model = model;
            J3D_InitModel(&sOptShape[i], 0);
        }
    }
    for (i = 0; i < OPT_RING; i++) {
        sOptRing[i].alpha = 0;
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        sOptShape[i].alpha = 0;
    }
    sOptFrame = 0;
    sOptSettled = FALSE;
    OptionIcon_ResetShape();
}

/* 0x81323E20: home positions. Ring cube 0, 7, 15 and 22 are the corners. */
void OptionIcon_ResetShape(void) {
    s16 i;
    s16 k = 0;
    u8 unused[4]; /* never used, but still takes a stack slot */

    for (i = 0; i < OPT_RING; i++) {
        if (i >= 1 && i <= 6) {
            sOptRingCube[i].x = 80.0f;
            sOptRingCube[i].y = 70 - i * 20;
        } else if (i >= 8 && i <= 14) {
            sOptRingCube[i].x = (15 - i) * 20 - 80;
            sOptRingCube[i].y = -70.0f;
        } else if (i >= 16 && i <= 21) {
            sOptRingCube[i].x = -80.0f;
            sOptRingCube[i].y = 70 - (22 - i) * 20;
        } else if (i >= 23 && i <= 29) {
            sOptRingCube[i].x = (i - 22) * 20 - 80;
            sOptRingCube[i].y = 70.0f;
        } else {
            sOptRingCube[i].x = 0.0f;
            sOptRingCube[i].y = 0.0f;
        }
        sOptRingCube[i].rotX = 0;
        sOptRingCube[i].rotY = 0;
        sOptRing[i].alpha = 0;
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        while (sOptShapeBits[k++] == 0 && k < 8 * 9) {
        }
        sOptShapeCube[i].x = ((k - 1) % 9) * 20 - 80;
        sOptShapeCube[i].y = 70 - ((k - 1) / 9) * 20;
        sOptShape[i].alpha = 0;
    }
}

static inline void OptionIcon_ClampAlpha(s16* alpha, s32 add) {
    s16 a = *alpha;
    a += add;

    if (a > 255) {
        a = 255;
    }
    if (a < 0) {
        a = 0;
    }
    *alpha = a;
}

static inline void OptionIcon_CopyHome(void) {
    s16 i;

    for (i = 0; i < OPT_RING; i++) {
        sOptRingCube[i].dx = sOptRingCube[i].x;
        sOptRingCube[i].dy = sOptRingCube[i].y;
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        sOptShapeCube[i].dx = sOptShapeCube[i].x;
        sOptShapeCube[i].dy = sOptShapeCube[i].y;
    }
}

/* 0x8132406C */
static void OptionIcon_AddAlpha(s16* alpha, s32 add) {
    s16 a = *alpha;
    a += add;

    if (a > 255) {
        a = 255;
    }
    if (a < 0) {
        a = 0;
    }
    *alpha = a;
}

/* 0x81324098: draw every cube at its home position. */
static void OptionIcon_ShowHome(void) {
    s16 i;

    for (i = 0; i < OPT_RING; i++) {
        sOptRingCube[i].dx = sOptRingCube[i].x;
        sOptRingCube[i].dy = sOptRingCube[i].y;
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        sOptShapeCube[i].dx = sOptShapeCube[i].x;
        sOptShapeCube[i].dy = sOptShapeCube[i].y;
    }
}

/* the four corners fly out from the centre to the corners of the square */
static inline void OptionIcon_FlyOut(s16 t) {
    u8 corner[4] = { 0, 7, 22, 15 };
    s16 j;
    s16 toX, toY;

    for (j = 0; j < 4; j++) {
        toX = (j % 2) * 160 - 80;
        toY = 70 - (j / 2) * 140;
        sOptRingCube[corner[j]].x = Hermite(t, 0.0f, 0.0f, toX / 20, 20.0f, toX, 0.0f);
        sOptRingCube[corner[j]].y = Hermite(t, 0.0f, 0.0f, toY / 20, 20.0f, toY, 0.0f);
        OptionIcon_ClampAlpha(&sOptRing[corner[j]].alpha, 6);
    }
}

/* 0x81324268: the corners move round to the next corner, and the outline
   cubes they pass light up. */
static void OptionIcon_SlideCorners(s16 t, s16 dur) {
    u8 corner[4] = { 22, 0, 15, 7 };
    u8 from[4] = { 2, 0, 3, 1 };
    s16 j;
    s16 k;
    s16 fromX, fromY, toX, toY;

    for (j = 0; j < 4; j++) {
        fromX = (from[j] % 2) * 160 - 80;
        fromY = 70 - (from[j] / 2) * 140;
        toX = (j % 2) * 160 - 80;
        toY = 70 - (j / 2) * 140;
        sOptRingCube[corner[j]].x = Hermite(t, 0.0f, fromX, (toX - fromX) / dur, dur, toX, 0.0f);
        sOptRingCube[corner[j]].y = Hermite(t, 0.0f, fromY, (toY - fromY) / dur, dur, toY, 0.0f);
        OptionIcon_ClampAlpha(&sOptRing[corner[j]].alpha, 6);
        switch (j) {
        case 0:
            for (k = 7; k > (s16)((70.0f - sOptRingCube[corner[j]].y) / 20.0f); k--) {
                sOptRing[22 - k].alpha = 255;
            }
            break;
        case 1:
            for (k = 0; k < (s16)((sOptRingCube[corner[j]].x - -80.0f) / 20.0f); k++) {
                sOptRing[23 + k].alpha = 255;
            }
            break;
        case 2:
            for (k = 8; k > (s16)((sOptRingCube[corner[j]].x - -80.0f) / 20.0f); k--) {
                sOptRing[15 - k].alpha = 255;
            }
            break;
        case 3:
            for (k = 0; k < (s16)((70.0f - sOptRingCube[corner[j]].y) / 20.0f); k++) {
                sOptRing[1 + k].alpha = 255;
            }
            break;
        }
    }
    OptionIcon_CopyHome();
}

/* 0x81324938: the outline cubes flip a quarter turn, 5 frames apart. */
static void OptionIcon_FlipRing(s16 t) {
    s16 i;

    for (i = 0; i < OPT_RING; i++) {
        if (t >= i * 5 && t <= i * 5 + 10) {
            if (i < 7) {
                sOptRingCube[i].rotX = ((t - i * 5) << 14) / 10;
                sOptRingCube[i].rotY = 0;
            } else if (i < 15) {
                sOptRingCube[i].rotX = 0;
                sOptRingCube[i].rotY = -((t - i * 5) << 14) / 10;
            } else if (i < 22) {
                sOptRingCube[i].rotX = -((t - i * 5) << 14) / 10;
                sOptRingCube[i].rotY = 0;
            } else {
                sOptRingCube[i].rotX = 0;
                sOptRingCube[i].rotY = ((t - i * 5) << 14) / 10;
            }
        }
    }
    OptionIcon_CopyHome();
}

/* hide whichever group is shown and show the other one */
static inline void OptionIcon_Swap(void) {
    s16 i;

    for (i = 0; i < OPT_RING; i++) {
        sOptRing[i].alpha ^= 255;
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        sOptShape[i].alpha ^= 255;
    }
}

/* 0x81324C24: open a vertical slit from the centre that shows the shape. */
static void OptionIcon_WipeToShape(s16 t, s16 dur) {
    s16 i;
    s16 r = Hermite(t, 0.0f, 0.0f, 0.0f, dur, 80.0f, 0.0f);
    f32 x;

    if (t == 0) {
        OptionIcon_Swap();
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        x = sOptShapeCube[i].x;
        if (x > 0.0f && x > r) {
            sOptShapeCube[i].dx = r;
        } else if (x < 0.0f && x < -r) {
            sOptShapeCube[i].dx = -r;
        } else {
            sOptShapeCube[i].dx = x;
        }
    }
}

/* 0x81324EEC: each row of the shape hops twice, top row first. */
static void OptionIcon_BounceShape(s16 t) {
    s16 i;
    s16 row;

    for (i = 0; i < OPT_SHAPE; i++) {
        row = 7.0f - (70.0f - sOptShapeCube[i].y) / 20.0f;
        if (t >= row * 4 && t < row * 4 + 16) {
            sOptShapeCube[i].dy = sOptShapeCube[i].y + Hermite(t - row * 4, 0.0f, 0.0f, 0.0f, 16.0f, 10.0f, 0.0f);
        } else if (t >= row * 4 + 16 && t <= row * 4 + 32) {
            sOptShapeCube[i].dy = sOptShapeCube[i].y + Hermite(t - row * 4 - 16, 0.0f, 10.0f, 0.0f, 16.0f, 0.0f, 0.0f);
        } else if (t >= (row + 8) * 4 && t < (row + 8) * 4 + 16) {
            sOptShapeCube[i].dy = sOptShapeCube[i].y + Hermite(t - (row + 8) * 4, 0.0f, 0.0f, 0.0f, 16.0f, 10.0f, 0.0f);
        } else if (t >= (row + 8) * 4 + 16 && t < (row + 8) * 4 + 32) {
            sOptShapeCube[i].dy = sOptShapeCube[i].y + Hermite(t - (row + 8) * 4 - 16, 0.0f, 10.0f, 0.0f, 16.0f, 0.0f, 0.0f);
        } else {
            sOptShapeCube[i].dy = sOptShapeCube[i].y;
        }
    }
}

/* 0x81325140: squeeze the shape vertically into a line. */
static void OptionIcon_SqueezeShape(s16 t, s16 dur) {
    s16 i;
    s16 r = Hermite(t, 0.0f, 70.0f, 0.0f, dur, 0.0f, 0.0f);
    f32 y;

    for (i = 0; i < OPT_SHAPE; i++) {
        y = sOptShapeCube[i].y;
        if (y > 0.0f && y > r) {
            sOptShapeCube[i].dy = r;
        } else if (y < 0.0f && y < -r) {
            sOptShapeCube[i].dy = -r;
        } else {
            sOptShapeCube[i].dy = y;
        }
        sOptShapeCube[i].dx = sOptShapeCube[i].x;
    }
}

/* 0x8132527C: open a horizontal slit from the centre that shows the square. */
static void OptionIcon_WipeToRing(s16 t, s16 dur) {
    s16 i;
    s16 r = Hermite(t, 0.0f, 0.0f, 0.0f, dur, 80.0f, 0.0f);
    f32 y;

    if (t == 0) {
        OptionIcon_Swap();
    }
    for (i = 0; i < OPT_RING; i++) {
        y = sOptRingCube[i].y;
        if (y > 0.0f && y > r) {
            sOptRingCube[i].dy = r;
        } else if (y < 0.0f && y < -r) {
            sOptRingCube[i].dy = -r;
        } else {
            sOptRingCube[i].dy = y;
        }
        sOptRingCube[i].dx = sOptRingCube[i].x;
    }
}

/* 0x8132554C */
/* squeeze the square horizontally into a line */
static inline void OptionIcon_SqueezeRing(s16 t) {
    s16 i;
    u8 unused[4];
    s16 r;
    f32 x;

    (void)&unused; /* dead, but it takes the stack slot below FlyOut's corner[] */
    r = Hermite(t, 0.0f, 80.0f, 0.0f, 40.0f, 0.0f, 0.0f);
    for (i = 0; i < OPT_RING; i++) {
        x = sOptRingCube[i].x;
        if (x > 0.0f && x > r) {
            sOptRingCube[i].dx = r;
        } else if (x < 0.0f && x < -r) {
            sOptRingCube[i].dx = -r;
        } else {
            sOptRingCube[i].dx = x;
        }
    }
}

void OptionIcon_UpdateFace(void) {
    OptionParams* p;

    p = Option_GetParams();
    if (sOptFrame == 0) {
        OptionIcon_ResetShape();
    }
    if (sOptFrame < 20) {
        /* the four corners fade in at the centre */
        sOptRingCube[0].x = 0.0f;
        sOptRingCube[0].y = 0.0f;
        OptionIcon_AddAlpha(&sOptRing[0].alpha, 6);
        sOptRingCube[7].x = 0.0f;
        sOptRingCube[7].y = 0.0f;
        OptionIcon_AddAlpha(&sOptRing[7].alpha, 6);
        sOptRingCube[15].x = 0.0f;
        sOptRingCube[15].y = 0.0f;
        OptionIcon_AddAlpha(&sOptRing[15].alpha, 6);
        sOptRingCube[22].x = 0.0f;
        sOptRingCube[22].y = 0.0f;
        OptionIcon_AddAlpha(&sOptRing[22].alpha, 6);
        OptionIcon_ShowHome();
    } else if (sOptFrame < 40) {
        OptionIcon_FlyOut(sOptFrame - 20);
        OptionIcon_ShowHome();
    } else if (sOptFrame < 70) {
        OptionIcon_SlideCorners(sOptFrame - 40, 30);
    } else if (sOptFrame < 225) {
        OptionIcon_FlipRing(sOptFrame - 70);
    } else if (sOptFrame < 265) {
        OptionIcon_SqueezeRing(sOptFrame - 225);
    } else if (sOptFrame < 305) {
        OptionIcon_WipeToShape(sOptFrame - 265, 40);
    } else if (sOptFrame < 397) {
        OptionIcon_BounceShape(sOptFrame - 305);
    } else if (sOptFrame < 519) {
        OptionIcon_BounceShape(sOptFrame - 397);
    } else if (sOptFrame < 559) {
        OptionIcon_SqueezeShape(sOptFrame - 519, 40);
    } else if (sOptFrame < 599) {
        OptionIcon_WipeToRing(sOptFrame - 559, 40);
    }
    sOptFrame++;
    if (sOptFrame > 599) {
        sOptFrame = 70;
    }
}

/* Passing the scale through VecRef gives the IPL's stack layout for the
 * by-value Vec argument (rodata copy, then the argument copy above it). */
static inline Vec* VecRef(Vec* v) {
    (void)v;
    return v;
}

/* ring and shape each get their own copy of the 0.67 literal in .rodata */
static inline void OptionIcon_SetRingMtx(J3DModelInstance* inst, Mtx m) {
    Vec scale = { 0.67f, 0.67f, 0.67f };

    J3D_SetModelMtx(inst, m, *VecRef(&scale));
}

static inline void OptionIcon_SetShapeMtx(J3DModelInstance* inst, Mtx m) {
    Vec scale = { 0.67f, 0.67f, 0.67f };

    J3D_SetModelMtx(inst, m, *VecRef(&scale));
}

/* 0x81325960 */
void OptionIcon_CalcMtx(void) {
    Mtx m;
    Mtx rot;
    Mtx t;
    Mtx base;
    s16 i;

    MtxRotTrans(0, -0x4000, 0, rot, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(Menu_GetCubeMtx(), rot, base);
    for (i = 0; i < OPT_RING; i++) {
        MtxRotTrans(sOptRingCube[i].rotX, sOptRingCube[i].rotY, 0, t, sOptRingCube[i].dx, sOptRingCube[i].dy, 0.0f);
        PSMTXConcat(base, t, m);
        OptionIcon_SetRingMtx(&sOptRing[i], m);
        J3D_SetViewMtx(&sOptRing[i], Camera_GetView());
        J3D_CalcJoints(&sOptRing[i]);
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        PSMTXTrans(t, sOptShapeCube[i].dx, sOptShapeCube[i].dy, 0.0f);
        PSMTXConcat(base, t, m);
        OptionIcon_SetShapeMtx(&sOptShape[i], m);
        J3D_SetViewMtx(&sOptShape[i], Camera_GetView());
        J3D_CalcJoints(&sOptShape[i]);
    }
}

/* 0x81325B48 */
void OptionIcon_Draw(void) {
    OptionParams* p = Option_GetParams();
    void* tev;
    s16 i;

    sOptTint.r = p->tint[0];
    sOptTint.g = p->tint[1];
    sOptTint.b = p->tint[2];
    sOptTint.a = p->tint[3];
    tev = sOptRing[0].model->material->tevColor;
    sOptRing[0].model->material->tevColor = &sOptTint;
    for (i = 0; i < OPT_RING; i++) {
        J3D_Draw(&sOptRing[i]);
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        J3D_Draw(&sOptShape[i]);
    }
    sOptRing[0].model->material->tevColor = tev;
}

/* 0x81325C30: fade every cube out while the submenu closes. */
void OptionIcon_UpdateHide(void) {
    s32 i;

    for (i = 0; i < OPT_RING; i++) {
        OptionIcon_ClampAlpha(&sOptRing[i].alpha, -10);
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        OptionIcon_ClampAlpha(&sOptShape[i].alpha, -10);
    }
    sOptFrame = 0;
}

static inline BOOL OptionIcon_Shrink(f32* pv) {
    f32 v = *pv;
    OptionParams* p = Option_GetParams();

    if (v > 0.0f) {
        v -= p->s1E;
        if (v < 0.0f) {
            v = 0.0f;
        }
    } else if (v < 0.0f) {
        v += p->s1E;
        if (v > 0.0f) {
            v = 0.0f;
        }
    }
    *pv = v;
    return (v == 0.0f) ? TRUE : FALSE;
}

/* 0x81325DB0: pull every cube back to the centre. */
void OptionIcon_UpdateEnter(void) {
    s32 i;
    s32 settled = 0;
    OptionParams* p = Option_GetParams();

    for (i = 0; i < OPT_RING; i++) {
        OptionIcon_ClampAlpha(&sOptRing[i].alpha, -p->s20);
        if (OptionIcon_Shrink(&sOptRingCube[i].dx)) {
            settled++;
        }
        if (OptionIcon_Shrink(&sOptRingCube[i].dy)) {
            settled++;
        }
    }
    for (i = 0; i < OPT_SHAPE; i++) {
        OptionIcon_ClampAlpha(&sOptShape[i].alpha, -p->s20);
        if (OptionIcon_Shrink(&sOptShapeCube[i].dx)) {
            settled++;
        }
        if (OptionIcon_Shrink(&sOptShapeCube[i].dy)) {
            settled++;
        }
    }
    if (settled == (OPT_RING + OPT_SHAPE) * 2) {
        sOptSettled = TRUE;
    } else {
        sOptSettled = FALSE;
    }
    sOptFrame = 0;
}

/* 0x8132610C */
BOOL OptionIcon_IsSettled(void) {
    return sOptSettled;
}
