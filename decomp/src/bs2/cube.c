// Rolling-cube intro (PAL 1.0 BS2, 0x8130EF48..0x813108B8): the small cube
// falls, rolls a path over three faces leaving tiles behind, rises, drops
// into the big cube and the logo fades in.

#include "bs2/cube.h"
#include "bs2/scene.h"
#include "bs2/j3d.h"
#include "bs2/menu.h"
#include <dolphin/gx.h>
#include <dolphin/pad.h>


u32 Pad_GetEasterEgg(void);                                /* 0x81302DE8 */
void J3D_AnimateJoints(void *inst, f32 frame);             /* 0x813043BC */
void MtxRotAxis(Mtx m, s32 axis, s16 angle);               /* 0x81307208 */

static CubePathStep sCubePath[] = {                        /* 0x81381160 */
    { 0, 1, 5 },    { 0, 1, 0xFF }, { 3, 1, 0xFF }, { 3, 1, 0xFF }, { 5, 1, 0xFF },
    { 0, 2, 0xFF }, { 0, 2, 0xFF }, { 3, 2, 0xFF }, { 3, 2, 0xFF }, { 6, 2, 0xFF },
    { 0, 3, 0xFF }, { 0, 3, 0xFF }, { 3, 3, 0xFF }, { 3, 3, 0xFF }, { 2, 3, 0xFF },
    { 2, 3, 0xFF }, { 7, 3, 0xFF }, { 3, 1, 0xFF }, { 3, 1, 5 },    { 3, 1, 5 },
    { 3, 1, 5 },    { 3, 1, 5 },    { 3, 1, 5 },    { 3, 1, 5 },    { 3, 1, 5 },
    { 3, 1, 5 },    { 3, 1, 5 },    { 3, 1, 5 },    { 3, 1, 5 },    { 3, 1, 5 },
    { 2, 1, 5 },    { 2, 1, 5 },    { 2, 1, 5 },    { 8, 0, 0 },
};

static u8 sLandPending = TRUE;       /* 0x814ACFF8 */
static u8 sDropPending = TRUE;       /* 0x814ACFF9 */
static u8 sJinglePending = TRUE;     /* 0x814ACFFA */

static CubeState *sCube;             /* 0x814AD3D8 */
static s32 sPhase;                   /* 0x814AD3DC */
static s32 sNextPhase;               /* 0x814AD3E0 */
static BOOL sRunning;                /* 0x814AD3E4 */

static CubeTile sGrid[CUBE_FACES][CUBE_GRID][CUBE_GRID]; /* 0x814AFD58 */
static CubeState sCubeDefault;       /* 0x814B5D58 */
static Mtx sCubeMtx;                 /* 0x814B5DC0 */

/* 0x8130EF48 */
CubeTile *Cube_GetGrid(void) {
    return &sGrid[0][0][0];
}

/* 0x8130EF54 */
CubeState *Cube_Get(void) {
    return sCube;
}

/* 0x8130EF5C */
static void Cube_ClearGrid(void) {
    s32 f, x, y;

    for (f = 0; f < CUBE_FACES; f++) {
        for (x = 0; x < CUBE_GRID; x++) {
            for (y = 0; y < CUBE_GRID; y++) {
                sGrid[f][x][y].life = 0;
                sGrid[f][x][y].group = 0;
                sGrid[f][x][y].state = 0;
            }
        }
    }
}

/* 0x8130F06C */
static void Cube_Reset(CubeState *c) {
    c->x = 16;
    c->y = 18;
    c->face = 0;
    c->step = 0;
    c->dir = sCubePath[c->step].dir;
    c->tileInset = 0;
    c->done = 0;
    if (!(c->flags & 1)) {
        c->color = 0x9664FF00;
    }
    sPhase = CUBE_PHASE_FALL;
    sNextPhase = CUBE_PHASE_FALL;

    /* Constant acceleration so the fall covers fallHeight0 in fallFrames. */
    c->fallVel = 0.0f;
    c->fallAccel = 2.0f * c->fallHeight0 / (c->fallFrames * c->fallFrames);
    c->fallHeight = c->fallHeight0;

    /* The drop starts at dropVel0 and accelerates to land in dropFrames. */
    c->dropHeight0 = c->riseHeight;
    c->dropVel = c->dropVel0;
    c->dropAccel = 2.0f * c->dropHeight0 / (c->dropFrames * c->dropFrames) - c->dropVel / c->dropFrames;
    if (c->dropAccel < 0.0f) {
        c->dropAccel = 0.0f;
    }
    c->dropHeight = c->dropHeight0;

    c->fallFrame = 0;
    c->dropFrame = 0;
    c->stepFrame = 0;
    c->riseFrame = 0;
    c->logoFrame = 0;
    c->textFrame = 0;
    c->jingleFrame = 0;
    sJinglePending = TRUE;
}

/* 0x8130F1C0 */
static void Cube_SetDefaults(CubeState *c) {
    c->stepDoneFrame = 0;
    c->flags = 0;
    c->holdFall = 0;
    c->jingleDelay[0] = 0;
    c->jingleDelay[1] = 6;
    c->jingleDelay[2] = 5;
    c->fallHeight0 = 100.0f;
    c->fallFrames = 4;
    c->fallDelay = 13;
    c->sideFrames = 8;
    c->cornerFrames = 15;
    c->stepDelay = 0;
    c->alpha = 255;
    c->unk1C = 5;
    c->riseTurns = 6;
    c->riseFrames = 15;
    c->riseDelay = 6;
    c->riseHeight = 130.0f;
    c->dropFrames = 8;
    c->dropDelay = 17;
    c->dropVel0 = 30.0f;
    c->logoFrames = 33;
    c->textFrames = 50;
    c->textDelay = 4;
    c->size = 54;
    c->tileR = 100;
    c->tileG = 80;
    c->tileB = 190;
}

/* 0x8130F28C */
void Cube_Init(BOOL first) {
    if (first) {
        sCube = &sCubeDefault;
        Cube_SetDefaults(&sCubeDefault);
    }
    Cube_ClearGrid();
    Cube_Reset(sCube);
    if (first) {
        Cube_InitModels();
    }
    sRunning = TRUE;
    sLandPending = TRUE;
    sDropPending = TRUE;
}

/* 0x8130F2F0: advance tile lifetimes by one path step. */
static void Cube_TickTiles(void) {
    s32 f, x, y;
    CubeTile *t;

    for (f = 0; f < CUBE_FACES; f++) {
        for (x = 0; x < CUBE_GRID; x++) {
            for (y = 0; y < CUBE_GRID; y++) {
                t = &sGrid[f][x][y];
                if (t->state == 2) {
                    t->state = 0;
                    t->group = 0;
                    t->life = 0;
                } else if (t->state == 1) {
                    t->state = 0;
                } else if (t->group != 0 && t->life != 0xFF) {
                    t->life--;
                    if (t->life == 0) {
                        t->state = 2;
                    }
                }
            }
        }
    }
}

/* 0x8130F418: move one cell (or over an edge onto another face) and leave
   a tile there. */
static void Cube_StepGrid(void) {
    CubeTile *t;
    s8 face;

    switch (sCube->dir) {
    case CUBE_DIR_XPOS:
        sCube->x = sCube->x < CUBE_GRID ? sCube->x + 1 : CUBE_GRID;
        break;
    case CUBE_DIR_YPOS:
        sCube->y = sCube->y < CUBE_GRID ? sCube->y + 1 : CUBE_GRID;
        break;
    case CUBE_DIR_XNEG:
        sCube->x = sCube->x > 0 ? sCube->x - 1 : 0;
        break;
    case CUBE_DIR_YNEG:
        sCube->y = sCube->y > 0 ? sCube->y - 1 : 0;
        break;
    default:
        face = sCube->dir - CUBE_DIR_FACE0;
        if ((face + 3 - sCube->face) % 3 == 1 && sCube->y == 16) {
            sCube->y = sCube->x;
            sCube->x = 16;
            sCube->face = face;
        }
        if ((face + 3 - sCube->face) % 3 == 2 && sCube->x == 16) {
            sCube->x = sCube->y;
            sCube->y = 16;
            sCube->face = face;
        }
        break;
    }
    Cube_TickTiles();
    sGrid[sCube->face][sCube->x][sCube->y].life = sCubePath[sCube->step].life;
    sGrid[sCube->face][sCube->x][sCube->y].group = sCubePath[sCube->step].group;
    sGrid[sCube->face][sCube->x][sCube->y].state = 1;
    sGrid[sCube->face][sCube->x][sCube->y].color = sCube->color;
}

/* 0x8130F6B8: roll about the leading bottom edge. */
static void Cube_CalcRollMtx(Mtx m) {
    Mtx t;
    Mtx rot;
    s32 angle;
    s32 max = 0x4000;

    if (sCube->dir >= CUBE_DIR_FACE0) {
        max = 0x8000;
        angle = (sCube->stepFrame << 15) / (sCube->cornerFrames - sCube->stepDelay);
    } else {
        angle = (sCube->stepFrame << 14) / (sCube->sideFrames - sCube->stepDelay);
    }
    if (angle > max) {
        angle = max;
    }
    MtxRotTrans(0, 0, 0, t, -sCube->size / 2, sCube->size / 2, -sCube->size / 2);
    PSMTXConcat(m, t, m);
    MtxRotAxis(rot, 'x', angle);
    PSMTXConcat(rot, m, m);
}

/* 0x8130F7F0: turn to the step direction, move to the grid cell, and onto
   the current face. */
static void Cube_CalcGridMtx(Mtx m) {
    Mtx t;
    Mtx face;
    s16 yaw = 0;

    switch (sCube->dir) {
    case CUBE_DIR_XPOS:
        yaw = -0x4000;
        break;
    case CUBE_DIR_YPOS:
        yaw = 0x7FFF;
        break;
    case CUBE_DIR_XNEG:
        yaw = 0x4000;
        break;
    case CUBE_DIR_YNEG:
        yaw = 0;
        break;
    }
    PSMTXTrans(t, sCube->size / 2, 0.0f, sCube->size / 2);
    PSMTXConcat(t, m, m);
    MtxRotTrans(0, yaw, 0, t, -sCube->size / 2, 0.0f, -sCube->size / 2);
    PSMTXConcat(t, m, m);
    PSMTXTrans(t, -(sCube->x - 16) * sCube->size, 0.0f, -(sCube->y - 16) * sCube->size);
    PSMTXConcat(t, m, m);
    if (sCube->face == 1) {
        MtxRotTrans(0x4000, 0, 0x4000, face, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(face, m, m);
    }
    if (sCube->face == 2) {
        MtxRotTrans(0, -0x4000, -0x4000, face, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(face, m, m);
    }
}

/* 0x8130FA3C */
MtxPtr Cube_GetMtx(void) {
    return sCubeMtx;
}

/* 0x8130FA48 */
static BOOL Cube_IsStepDone(void) {
    s32 angle;
    s32 max = 0x4000;

    if (sCube->dir >= CUBE_DIR_FACE0) {
        max = 0x8000;
        angle = (sCube->stepFrame << 15) / (sCube->cornerFrames - sCube->stepDelay);
    } else {
        angle = (sCube->stepFrame << 14) / (sCube->sideFrames - sCube->stepDelay);
    }
    if (angle > max && sCube->stepDoneFrame == 0) {
        sCube->stepDoneFrame = sCube->stepFrame;
    }
    if (sCube->stepDoneFrame > 0 && sCube->stepFrame >= sCube->stepDoneFrame + sCube->stepDelay) {
        return TRUE;
    }
    return FALSE;
}

/* 0x8130FAF0 */
static s32 Cube_UpdateFall(void) {
    Scene *s = Scene_Get();

    if (sCube->holdFall) {
        return CUBE_PHASE_FALL;
    }
    sCube->fallHeight -= sCube->fallVel;
    if (sCube->fallHeight < 0.0f) {
        sCube->fallHeight = 0.0f;
        if (sLandPending) {
            s->landImpulse = s->landImpulse0;
            s->shakePhase = 0x7FFF;
            sLandPending = FALSE;
        }
    }
    sCube->fallVel += sCube->fallAccel;
    if (sCube->fallFrame > sCube->fallFrames + sCube->fallDelay) {
        sLandPending = TRUE;
        return CUBE_PHASE_PATH;
    }
    sCube->fallFrame++;
    return CUBE_PHASE_FALL;
}

/* 0x8130FBC4: grow to 1.5x while making riseTurns quarter turns. */
static void Cube_CalcRiseMtx(Mtx m) {
    Mtx t;
    f32 scale;
    s32 max;
    s32 angle;
    f32 y;
    f32 lo;

    scale = 1.0f + 0.5f * (s32)sCube->riseFrame / (s32)sCube->riseFrames;
    if (scale > 1.5f) {
        scale = 1.5f;
    }
    max = sCube->riseTurns << 14;
    angle = max * sCube->riseFrame / sCube->riseFrames;
    if (angle > max) {
        angle = max;
    }
    PSMTXScale(m, scale, scale, scale);
    y = sCube->size / 2 - sCube->riseFrame * ((1.0f + scale) * sCube->size) / sCube->riseFrames * 0.5f;
    lo = -(scale * sCube->size * 0.5f);
    if (y < lo) {
        y = lo;
    }
    MtxRotTrans(angle, 0, 0, t, scale * -sCube->size * 0.5f, y, scale * -sCube->size * 0.5f);
    PSMTXConcat(t, m, m);
    MtxRotTrans(0, -0x4000, -0x4000, t, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(t, m, m);
}

static inline Vec Cube_VecSplat(f32 v) {
    Vec r = { 0.0f, 0.0f, 0.0f };

    r.x = v;
    r.y = v;
    r.z = v;
    return r;
}

static inline void Cube_DrawPath(void) {
    Scene *s = Scene_Get();
    Mtx t;
    MtxPtr view;

    PSMTXIdentity(sCubeMtx);
    Cube_CalcRollMtx(sCubeMtx);
    Cube_CalcGridMtx(sCubeMtx);
    MtxRotTrans(0, 0, 0, t, sCube->size * 3 / 2, sCube->size * 3 / 2, sCube->size * 3 / 2);
    PSMTXConcat(t, sCubeMtx, sCubeMtx);
    PSMTXConcat(Scene_GetWorldMtx(), sCubeMtx, sCubeMtx);
    if (sPhase == CUBE_PHASE_FALL) {
        PSMTXTrans(t, 0.0f, sCube->fallHeight, 0.0f);
        PSMTXConcat(t, sCubeMtx, sCubeMtx);
    }
    J3D_SetModelMtx(Cube_GetModel(), sCubeMtx, Cube_VecSplat(s->modelScale));
    view = Camera_GetView();
    J3D_SetViewMtx(Cube_GetModel(), view);
    J3D_CalcJoints(Cube_GetModel());
}

static inline Vec Cube_VecOne(void) {
    Vec v = { 1.0f, 1.0f, 1.0f };

    return v;
}

/* 0x8130FDE0: phases 2..4 (rise, drop, logo). */
static void Cube_DrawRise(void) {
    Scene *s = Scene_Get();
    s32 n = sCube->riseFrames;
    s32 r = n - sCube->riseFrame;
    Mtx t;
    MtxPtr view;
    f32 h;
    f32 y;

    PSMTXIdentity(sCubeMtx);
    Cube_CalcRiseMtx(sCubeMtx);
    MtxRotTrans(0, 0, 0, t, sCube->size * 3 / 2, sCube->size * 3 / 2, sCube->size * 3 / 2);
    PSMTXConcat(t, sCubeMtx, sCubeMtx);
    PSMTXConcat(Scene_GetWorldMtx(), sCubeMtx, sCubeMtx);
    switch (sPhase) {
    case CUBE_PHASE_RISE:
        /* Ease out: h * (1 - r^4 / n^4) */
        if (r < 0) {
            r = 0;
        }
        h = sCube->riseHeight;
        y = h - r * (r * (r * (h * r))) / (n * (n * (n * n)));
        if (y > h) {
            y = h;
        }
        PSMTXTrans(t, 0.0f, y, 0.0f);
        break;
    case CUBE_PHASE_DROP:
    case CUBE_PHASE_LOGO:
        PSMTXTrans(t, 0.0f, sCube->dropHeight, 0.0f);
        break;
    }
    PSMTXConcat(t, sCubeMtx, sCubeMtx);
    {
        Vec scale = { 0.0f, 0.0f, 0.0f };

        scale.x = s->modelScale;
        scale.y = s->modelScale;
        scale.z = s->modelScale;
        J3D_SetModelMtx(Cube_GetModel(), sCubeMtx, scale);
    }
    view = Camera_GetView();
    J3D_SetViewMtx(Cube_GetModel(), view);
    J3D_CalcJoints(Cube_GetModel());
}

/* Phases 0 and 1: fall, then roll along the path. */

/* 0x81310060 */
void Cube_Draw(void) {
    Mtx m;
    MtxPtr view;
    f32 frame;

    switch (sPhase) {
    case CUBE_PHASE_FALL:
        Cube_DrawPath();
        break;
    case CUBE_PHASE_PATH:
        Cube_DrawPath();
        break;
    case CUBE_PHASE_RISE:
        Cube_DrawRise();
        break;
    case CUBE_PHASE_DROP:
        Cube_DrawRise();
        break;
    case CUBE_PHASE_LOGO:
        Cube_DrawRise();
        break;
    }

    /* The "GAMECUBE" wordmark joint animation (60 frames) runs off the
       text clock that starts with the drop. */
    if (sCube->textFrame > sCube->textDelay) {
        frame = 60.0f * ((f32)(sCube->textFrame - sCube->textDelay) / sCube->textFrames);
    } else {
        frame = 0.0f;
    }
    PSMTXIdentity(m);
    J3D_SetModelMtx(Scene_GetLogoText(), m, Cube_VecOne());
    view = Camera_GetView();
    J3D_SetViewMtx(Scene_GetLogoText(), view);
    J3D_AnimateJoints(Scene_GetLogoText(), frame);
}

/* 0x81310494: the boot jingle, with the Z easter eggs. */
static void Cube_UpdateJingle(void) {
    PADStatus pads[4];

    PADRead(pads);
    if (Pad_GetEasterEgg() == 2) {
        /* Z held on all four controllers */
        if (sJinglePending && sCube->jingleFrame > sCube->jingleDelay[1]) {
            Sound_Play(1);
            sJinglePending = FALSE;
        }
    } else if (Pad_GetEasterEgg() == 1) {
        /* Z held on controller 1 */
        if (sJinglePending && sCube->jingleFrame > sCube->jingleDelay[2]) {
            Sound_Play(2);
            sJinglePending = FALSE;
        }
    } else if (sJinglePending && sCube->jingleFrame > sCube->jingleDelay[0]) {
        Sound_Play(0);
        sJinglePending = FALSE;
    }
    if (sJinglePending) {
        sCube->jingleFrame++;
    }
}

/* 0x81310584: one logic frame. */
s32 Cube_Update(BOOL playJingle) {
    Scene *s;
    BOOL textDone;

    sPhase = sNextPhase;
    if (playJingle) {
        Cube_UpdateJingle();
    }
    switch (sPhase) {
    case CUBE_PHASE_FALL:
        sNextPhase = Cube_UpdateFall();
        break;
    case CUBE_PHASE_PATH:
        if (Cube_IsStepDone()) {
            sCube->stepFrame = 0;
            sCube->stepDoneFrame = 0;
            sCube->step++;
            if (sCubePath[sCube->step].dir == CUBE_DIR_END) {
                sCube->done = 1;
                Cube_TickTiles();
            } else {
                Cube_StepGrid();
                sCube->dir = sCubePath[sCube->step].dir;
            }
        }
        if (sCube->dir == CUBE_DIR_RISE) {
            sNextPhase = CUBE_PHASE_RISE;
        } else {
            sCube->stepFrame++;
            sNextPhase = CUBE_PHASE_PATH;
        }
        break;
    case CUBE_PHASE_RISE:
        sRunning = FALSE;
        sCube->riseFrame++;
        sNextPhase = sCube->riseFrame > sCube->riseFrames + sCube->riseDelay ? CUBE_PHASE_DROP : CUBE_PHASE_RISE;
        break;
    case CUBE_PHASE_DROP:
        s = Scene_Get();
        sCube->dropHeight -= sCube->dropVel;
        if (sCube->dropHeight < 0.0f) {
            sCube->dropHeight = 0.0f;
            if (sDropPending) {
                s->dropImpulse = s->dropImpulse0;
                s->shakePhase = 0x7FFF;
                sDropPending = FALSE;
            }
        }
        sCube->dropVel += sCube->dropAccel;
        if (sCube->dropFrame > sCube->dropFrames + sCube->dropDelay) {
            sDropPending = TRUE;
            sNextPhase = CUBE_PHASE_LOGO;
        } else {
            sCube->dropFrame++;
            sCube->textFrame++;
            if (sCube->textFrame >= sCube->textFrames + sCube->textDelay) {
                sCube->textFrame = sCube->textFrames + sCube->textDelay;
            }
            sNextPhase = CUBE_PHASE_DROP;
        }
        break;
    case CUBE_PHASE_LOGO:
        s = Scene_Get();
        sCube->logoFrame++;
        if (sCube->logoFrame > sCube->logoFrames) {
            sCube->logoFrame = sCube->logoFrames;
            sCube->done = 1;
        }
        textDone = FALSE;
        s->logoAlpha = sCube->logoFrame * 255 / sCube->logoFrames;
        sCube->textFrame++;
        if (sCube->textFrame >= sCube->textFrames + sCube->textDelay) {
            sCube->textFrame = sCube->textFrames + sCube->textDelay;
            textDone = TRUE;
        }
        if (!textDone) {
            sCube->done = 0;
        }
        if (s->dropImpulse != 0.0f) {
            sCube->done = 0;
        }
        break;
    }
    return TRUE;
}

/* 0x813108B0: TRUE until the cube lifts off (A spins the scene while TRUE). */
BOOL Cube_IsRunning(void) {
    return sRunning;
}
