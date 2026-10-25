// Calendar icon (PAL 1.0 BS2, 0x81326114..0x813275CC): a clock made of
// small cubes behind the Calendar submenu. Five cubes mark the centre and the
// quarter hours, four cubes form the minute hand, three the hour hand, and a
// single cube runs round the dial with the seconds. The clock follows the
// real time; the cubes are pulled back to the centre when the submenu opens.

#include "bs2/menu.h"

#define CLOCK_DIAL 5
#define CLOCK_MIN 4
#define CLOCK_HOUR 3

typedef struct ClockPos {
    f32 x, y;
} ClockPos;

static J3DModelInstance sClockDial[CLOCK_DIAL];
static J3DModelInstance sClockMin[CLOCK_MIN];
static J3DModelInstance sClockHour[CLOCK_HOUR];
static J3DModelInstance sClockSec;
static ClockPos sClockDialPos[CLOCK_DIAL];
static ClockPos sClockMinPos[CLOCK_MIN];
static ClockPos sClockHourPos[CLOCK_HOUR];

static ClockPos sClockSecPos;
static s16 sClockFrame; /* frames since the clock appeared (capped at 50) */
BOOL sClockIconSettled;

void ClockIcon_InitHands(void);

/* frames since the hands started to grow in (they start at frame 40) */
#define HAND_T() (sClockFrame > 40 ? sClockFrame - 40 : 0)

/* 0x81326114 */
void ClockIcon_Init(J3DModel* model, BOOL first) {
    s16 i;

    if (first) {
        sClockSec.model = model;
        J3D_InitModel(&sClockSec, 0);
        for (i = 0; i < CLOCK_DIAL; i++) {
            J3D_CopyInstance(&sClockDial[i], &sClockSec);
        }
        for (i = 0; i < CLOCK_MIN; i++) {
            J3D_CopyInstance(&sClockMin[i], &sClockSec);
        }
        for (i = 0; i < CLOCK_HOUR; i++) {
            J3D_CopyInstance(&sClockHour[i], &sClockSec);
        }
    }
    sClockFrame = 0;
    sClockIconSettled = FALSE;
    sClockSec.alpha = 0;
    for (i = 0; i < CLOCK_DIAL; i++) {
        sClockDial[i].alpha = 0;
    }
    for (i = 0; i < CLOCK_MIN; i++) {
        sClockMin[i].alpha = 0;
    }
    for (i = 0; i < CLOCK_HOUR; i++) {
        sClockHour[i].alpha = 0;
    }
    ClockIcon_InitHands();
}

/* 0x81326248 */
void ClockIcon_InitHands(void) {
    s16 i;

    sClockFrame = 0;
    for (i = 0; i < CLOCK_DIAL; i++) {
        sClockDialPos[i].x = 0.0f;
        sClockDialPos[i].y = 0.0f;
    }
    for (i = 0; i < CLOCK_MIN; i++) {
        sClockMinPos[i].x = 0.0f;
        sClockMinPos[i].y = 0.0f;
    }
    for (i = 0; i < CLOCK_HOUR; i++) {
        sClockHourPos[i].x = 0.0f;
        sClockHourPos[i].y = 0.0f;
    }
    sClockSecPos.x = 0.0f;
    sClockSecPos.y = 0.0f;
}

/* 0x813262E4 */
void ClockIcon_UpdateFace(void) {
    OSCalendarTime t;
    s16 i;
    s32 n;
    s32 r;

    OSTicksToCalendarTime(OSGetTime(), &t);
    sClockFrame++;
    if (sClockFrame > 50) {
        sClockFrame = 50;
    }

    for (i = 0; i < CLOCK_DIAL; i++) {
        r = 100;
        if (i == 0) {
            r = 0;
        }
        n = sClockFrame;
        if (n > 30) {
            n = 30;
        }
        sClockDial[i].alpha = (u8)(n * 255 / 30);
        sClockDialPos[i].x = r * SinS((i - 1) << 14);
        sClockDialPos[i].y = r * CosS((i - 1) << 14);
    }

    for (i = 0; i < CLOCK_MIN; i++) {
        sClockMin[i].alpha = (u8)(HAND_T() * 255 / 10);
        sClockMinPos[i].x = ((i + 1) * 20 * HAND_T()) * SinS(-t.min * 0x10000 / 60 - (t.sec << 16) / 3600) / 10.0f;
        sClockMinPos[i].y = ((i + 1) * 20 * HAND_T()) * CosS(-t.min * 0x10000 / 60 - (t.sec << 16) / 3600) / 10.0f;
    }

    for (i = 0; i < CLOCK_HOUR; i++) {
        sClockHour[i].alpha = (u8)(HAND_T() * 255 / 10);
        sClockHourPos[i].x = ((i + 1) * 20 * HAND_T()) * SinS(-(t.hour % 12) * 0x10000 / 12 - (t.min << 16) / 720) / 10.0f;
        sClockHourPos[i].y = ((i + 1) * 20 * HAND_T()) * CosS(-(t.hour % 12) * 0x10000 / 12 - (t.min << 16) / 720) / 10.0f;
    }

    sClockSec.alpha = (u8)(HAND_T() * 255 / 10);
    /* the loop counter (3) is subtracted from the angle, as in the original */
    sClockSecPos.x = 100.0f * SinS(-t.sec * 0x10000 / 60 - i);
    sClockSecPos.y = 100.0f * CosS(-t.sec * 0x10000 / 60 - i);
}

/* Passing the scale through VecRef gives the IPL's stack layout for the
 * by-value Vec argument (rodata copy, then the argument copy above it). */
static inline Vec* VecRef(Vec* v) {
    (void)v;
    return v;
}

static inline void ClockIcon_SetDialMtx(J3DModelInstance* inst, Mtx m, s16 n) {
    Vec v = { 0.0f, 0.0f, 0.0f };

    v.x = 0.7f * (n > 30 ? 30 : n) / 30.0f;
    v.y = 0.7f * (n > 30 ? 30 : n) / 30.0f;
    v.z = 0.7f * (n > 30 ? 30 : n) / 30.0f;
    J3D_SetModelMtx(inst, m, *VecRef(&v));
}

/* min and hour each get their own copy of the 0.55 literal in .rodata */
static inline void ClockIcon_SetMinMtx(J3DModelInstance* inst, Mtx m) {
    Vec v = { 0.55f, 0.55f, 0.55f };

    J3D_SetModelMtx(inst, m, *VecRef(&v));
}

static inline void ClockIcon_SetHourMtx(J3DModelInstance* inst, Mtx m) {
    Vec v = { 0.55f, 0.55f, 0.55f };

    J3D_SetModelMtx(inst, m, *VecRef(&v));
}

static inline void ClockIcon_SetSecMtx(J3DModelInstance* inst, Mtx m) {
    Vec v = { 0.0f, 0.0f, 0.0f };

    v.x = 0.55f * HAND_T() / 10.0f;
    v.y = 0.55f * HAND_T() / 10.0f;
    v.z = 0.55f * HAND_T() / 10.0f;
    J3D_SetModelMtx(inst, m, *VecRef(&v));
}

/* 0x81326808 */
void ClockIcon_CalcMtx(void) {
    Mtx m;
    Mtx rot;
    Mtx t;
    Mtx base;
    s16 i;

    MtxRotTrans(0, -0x4000, 0, rot, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(Menu_GetCubeMtx(), rot, base);

    for (i = 0; i < CLOCK_DIAL; i++) {
        PSMTXTrans(t, sClockDialPos[i].x, sClockDialPos[i].y, 0.0f);
        PSMTXConcat(base, t, m);
        ClockIcon_SetDialMtx(&sClockDial[i], m, sClockFrame);
        J3D_SetViewMtx(&sClockDial[i], Camera_GetView());
        J3D_CalcJoints(&sClockDial[i]);
    }

    for (i = 0; i < CLOCK_MIN; i++) {
        PSMTXTrans(t, sClockMinPos[i].x, sClockMinPos[i].y, 0.0f);
        PSMTXConcat(base, t, m);
        ClockIcon_SetMinMtx(&sClockMin[i], m);
        J3D_SetViewMtx(&sClockMin[i], Camera_GetView());
        J3D_CalcJoints(&sClockMin[i]);
    }

    for (i = 0; i < CLOCK_HOUR; i++) {
        PSMTXTrans(t, sClockHourPos[i].x, sClockHourPos[i].y, 0.0f);
        PSMTXConcat(base, t, m);
        ClockIcon_SetHourMtx(&sClockHour[i], m);
        J3D_SetViewMtx(&sClockHour[i], Camera_GetView());
        J3D_CalcJoints(&sClockHour[i]);
    }

    PSMTXTrans(t, sClockSecPos.x, sClockSecPos.y, 0.0f);
    PSMTXConcat(base, t, m);
    ClockIcon_SetSecMtx(&sClockSec, m);
    J3D_SetViewMtx(&sClockSec, Camera_GetView());
    J3D_CalcJoints(&sClockSec);
}

/* 0x81326CA4: the dial uses the second tint, the hands the first. */
void ClockIcon_Draw(void) {
    ClockParams* p = Clock_GetParams();
    GXColorS10 tint2;
    GXColorS10 tint;
    void* tev;
    s16 i;

    tint2.r = p->tint2[0];
    tint2.g = p->tint2[1];
    tint2.b = p->tint2[2];
    tint2.a = p->tint2[3];
    tint.r = p->tint[0];
    tint.g = p->tint[1];
    tint.b = p->tint[2];
    tint.a = p->tint[3];
    tev = sClockDial[0].model->material->tevColor;
    sClockDial[0].model->material->tevColor = &tint2;
    for (i = 0; i < CLOCK_DIAL; i++) {
        J3D_Draw(&sClockDial[i]);
    }
    sClockDial[0].model->material->tevColor = &tint;
    for (i = 0; i < CLOCK_MIN; i++) {
        J3D_Draw(&sClockMin[i]);
    }
    for (i = 0; i < CLOCK_HOUR; i++) {
        J3D_Draw(&sClockHour[i]);
    }
    J3D_Draw(&sClockSec);
    sClockDial[0].model->material->tevColor = tev;
}

/* 0x81326DE0: fade every cube out while the submenu closes. */
void ClockIcon_UpdateHide(void) {
    s32 i;
    s32 hidden = 0;

    for (i = 0; i < CLOCK_DIAL; i++) {
        sClockDial[i].alpha -= 10;
        if (sClockDial[i].alpha < 0) {
            sClockDial[i].alpha = 0;
            hidden++;
        }
    }
    for (i = 0; i < CLOCK_MIN; i++) {
        sClockMin[i].alpha -= 10;
        if (sClockMin[i].alpha < 0) {
            sClockMin[i].alpha = 0;
            hidden++;
        }
    }
    for (i = 0; i < CLOCK_HOUR; i++) {
        sClockHour[i].alpha -= 10;
        if (sClockHour[i].alpha < 0) {
            sClockHour[i].alpha = 0;
            hidden++;
        }
    }
    sClockSec.alpha -= 10;
    if (sClockSec.alpha < 0) {
        sClockSec.alpha = 0;
        hidden++;
    }
    if (hidden == CLOCK_DIAL + CLOCK_MIN + CLOCK_HOUR + 1) {
        sClockFrame = 0;
    }
}

/* Move v one step of p->s22 towards 0; counts 1 once it is there. */
#define SHRINK(v, settled)                \
    p = Clock_GetParams();                \
    if (v > 0.0f) {                       \
        v -= p->s22;                      \
        if (v < 0.0f) {                   \
            v = 0.0f;                     \
        }                                 \
    } else if (v < 0.0f) {                \
        v += p->s22;                      \
        if (v > 0.0f) {                   \
            v = 0.0f;                     \
        }                                 \
    }

#define FADE(inst)                        \
    alpha = (inst).alpha;                 \
    alpha -= Clock_GetParams()->s24;      \
    if (alpha < 0) {                      \
        alpha = 0;                        \
    }                                     \
    (inst).alpha = alpha;

#define SETTLE(pos)                       \
    v = (pos).x;                          \
    SHRINK(v, settled)                    \
    (pos).x = v;                          \
    ok = (v == 0.0f) ? 1 : 0;             \
    settled += ok;                        \
    v = (pos).y;                          \
    SHRINK(v, settled)                    \
    (pos).y = v;                          \
    ok = (v == 0.0f) ? 1 : 0;             \
    settled += ok;

/* 0x81326FDC: pull every cube back to the centre of the dial. */
void ClockIcon_UpdateEnter(void) {
    s32 settled = 0;
    s32 i;
    s16 alpha;
    s16 ok;
    f32 v;
    ClockParams* p;

    for (i = 0; i < CLOCK_DIAL; i++) {
        FADE(sClockDial[i])
        SETTLE(sClockDialPos[i])
    }
    for (i = 0; i < CLOCK_MIN; i++) {
        FADE(sClockMin[i])
        SETTLE(sClockMinPos[i])
    }
    for (i = 0; i < CLOCK_HOUR; i++) {
        FADE(sClockHour[i])
        SETTLE(sClockHourPos[i])
    }
    FADE(sClockSec)
    SETTLE(sClockSecPos)
    if (settled == (CLOCK_DIAL + CLOCK_MIN + CLOCK_HOUR + 1) * 2) {
        sClockFrame = 0;
        sClockIconSettled = TRUE;
    } else {
        sClockIconSettled = FALSE;
    }
}

/* 0x813275C4 */
BOOL ClockIcon_IsSettled(void) {
    return sClockIconSettled;
}
