// Non-matching: functionally equivalent decompilation of the rolling-cube
// intro (PAL 1.0 BS2, 0x8130EF48..0x81310F58). Addresses are given above
// each function. Data such as the path table lives in the user's ipl.bin.

#include "bs2/cube.h"
#include "bs2/scene.h"
#include "bs2/j3d.h"
#include "bs2/res.h"
#include "bs2/sound.h"
#include "bs2/pad.h"
#include <dolphin/gx.h>

extern CubePathStep cubePath[];          /* 0x81381160 */

static CubeTile cubeGrid[CUBE_FACES][CUBE_GRID][CUBE_GRID]; /* 0x814AFD58 */
static Mtx cubeMtx;                      /* 0x814B5DC0 (grid + 0x6068) */
static J3DModel cubeSmall;               /* 0x814B5DF0 */
static J3DModelInstance cubeSmallInst;   /* 0x814B5E28 */
static CubeState cubeDefault;            /* 0x814B5D58 */

static CubeState *cube;                  /* 0x814AD3D8 */
static s32 cubePhase;                    /* 0x814AD3DC */
static s32 cubeNextPhase;                /* 0x814AD3E0 */
static BOOL cubeRunning;                 /* 0x814AD3E4 */
static void *cubeTileTex;                /* 0x814AD3E8: resource 0x65 */
static u8 cubeLandPending;               /* 0x814ACFF8 */
static u8 cubeDropPending;               /* 0x814ACFF9 */
static u8 cubeJinglePending;             /* 0x814ACFFA */

static CubePathStep *Cube_PathStep(void) { return &cubePath[cube->step]; }

/* 0x8130EF48 */
CubeTile *Cube_GetGrid(void) { return &cubeGrid[0][0][0]; }

/* 0x8130EF54 */
CubeState *Cube_Get(void) { return cube; }

/* 0x8130EF5C */
static void Cube_ClearGrid(void) {
    s32 f, x, y;
    for (f = 0; f < CUBE_FACES; f++)
        for (x = 0; x < CUBE_GRID; x++)
            for (y = 0; y < CUBE_GRID; y++) {
                cubeGrid[f][x][y].life = 0;
                cubeGrid[f][x][y].group = 0;
                cubeGrid[f][x][y].state = 0;
            }
}

/* 0x8130F06C */
static void Cube_Reset(CubeState *c) {
    c->x = 16;
    c->y = 18;
    c->face = 0;
    c->step = 0;
    c->dir = cubePath[c->step].dir;
    c->tileInset = 0;
    c->done = 0;
    if (!(c->flags & 1))
        c->color = 0x9664FF00;
    cubePhase = CUBE_PHASE_FALL;
    cubeNextPhase = CUBE_PHASE_FALL;

    /* Constant acceleration so the fall covers fallHeight0 in fallFrames. */
    c->fallVel = 0.0f;
    c->fallAccel = (2.0f * c->fallHeight0) / (f32)(c->fallFrames * c->fallFrames);
    c->fallHeight = c->fallHeight0;

    /* The drop starts with dropVel0 and accelerates to land in dropFrames. */
    c->dropHeight0 = c->riseHeight;
    c->dropVel = c->dropVel0;
    c->dropAccel = (2.0f * c->dropHeight0) / (f32)(c->dropFrames * c->dropFrames)
                 - c->dropVel / (f32)c->dropFrames;
    if (c->dropAccel < 0.0f)
        c->dropAccel = 0.0f;
    c->dropHeight = c->dropHeight0;

    c->fallFrame = 0;
    c->dropFrame = 0;
    c->stepFrame = 0;
    c->riseFrame = 0;
    c->logoFrame = 0;
    c->textFrame = 0;
    c->jingleFrame = 0;
    cubeJinglePending = TRUE;
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
    c->alpha = 0xFF;
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
        cube = &cubeDefault;
        Cube_SetDefaults(&cubeDefault);
    }
    Cube_ClearGrid();
    Cube_Reset(cube);
    if (first)
        Cube_InitModels();
    cubeRunning = TRUE;
    cubeLandPending = TRUE;
    cubeDropPending = TRUE;
}

/* 0x8130F2F0: advance tile lifetimes by one path step. */
static void Cube_TickTiles(void) {
    s32 f, x, y;
    for (f = 0; f < CUBE_FACES; f++)
        for (x = 0; x < CUBE_GRID; x++)
            for (y = 0; y < CUBE_GRID; y++) {
                CubeTile *t = &cubeGrid[f][x][y];
                if (t->state == 2) {
                    t->state = 0;
                    t->group = 0;
                    t->life = 0;
                } else if (t->state == 1) {
                    t->state = 0;
                } else if (t->group != 0 && t->life != 0xFF) {
                    if (--t->life == 0)
                        t->state = 2;
                }
            }
}

/* 0x8130F418: move the cube one cell (or onto the next face) and drop a tile. */
static void Cube_StepGrid(void) {
    CubeTile *t;
    s32 rel;

    switch ((s8)cube->dir) {
    case CUBE_DIR_XPOS: cube->x = cube->x < 32 ? cube->x + 1 : 32; break;
    case CUBE_DIR_YPOS: cube->y = cube->y < 32 ? cube->y + 1 : 32; break;
    case CUBE_DIR_XNEG: cube->x = cube->x > 0 ? cube->x - 1 : 0; break;
    case CUBE_DIR_YNEG: cube->y = cube->y > 0 ? cube->y - 1 : 0; break;
    default: {
        s8 face = (s8)cube->dir - CUBE_DIR_FACE0;
        rel = (face + 3 - (s8)cube->face) % 3;
        /* Crossing an edge swaps which axis sits on the shared edge (16). */
        if (rel == 1 && cube->y == 16) {
            cube->y = cube->x;
            cube->x = 16;
            cube->face = face;
        }
        rel = (face + 3 - (s8)cube->face) % 3;
        if (rel == 2 && cube->x == 16) {
            cube->x = cube->y;
            cube->y = 16;
            cube->face = face;
        }
        break;
    }
    }

    Cube_TickTiles();
    t = &cubeGrid[(s8)cube->face][cube->x][cube->y];
    t->life = Cube_PathStep()->life;
    t->group = Cube_PathStep()->group;
    t->state = 1;
    t->color = cube->color;
}

static s32 Cube_RollAngle(s32 *max) {
    if ((s8)cube->dir >= CUBE_DIR_FACE0) {
        *max = 0x8000;
        return (cube->stepFrame << 15) / (cube->cornerFrames - cube->stepDelay);
    }
    *max = 0x4000;
    return (cube->stepFrame << 14) / (cube->sideFrames - cube->stepDelay);
}

/* 0x8130F6B8: rotate about the leading bottom edge. */
static void Cube_CalcRollMtx(Mtx m) {
    Mtx t, r;
    s32 max;
    s32 angle = Cube_RollAngle(&max);
    s32 half = cube->size / 2;
    if (angle > max)
        angle = max;
    MtxRotTrans(0, 0, 0, t, -half, half, -half);
    PSMTXConcat(m, t, m);
    MtxRotAxis(r, 'x', (s16)angle);
    PSMTXConcat(r, m, m);
}

/* 0x8130F7F0: orient the roll along dir and place the cube on its cell/face. */
static void Cube_CalcGridMtx(Mtx m) {
    Mtx t, r;
    s16 yaw = 0;
    s32 half = cube->size / 2;

    switch ((s8)cube->dir) {
    case CUBE_DIR_XPOS: yaw = -0x4000; break;
    case CUBE_DIR_YPOS: yaw = 0x7FFF; break;
    case CUBE_DIR_XNEG: yaw = 0x4000; break;
    case CUBE_DIR_YNEG: yaw = 0; break;
    }
    PSMTXTrans(t, half, 0.0f, half);
    PSMTXConcat(t, m, m);
    MtxRotTrans(0, yaw, 0, t, -half, 0.0f, -half);
    PSMTXConcat(t, m, m);
    PSMTXTrans(t, -(cube->x - 16) * cube->size, 0.0f, -(cube->y - 16) * cube->size);
    PSMTXConcat(t, m, m);
    if (cube->face == 1) {
        MtxRotTrans(0x4000, 0, 0x4000, r, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(r, m, m);
    }
    if (cube->face == 2) {
        MtxRotTrans(0, -0x4000, -0x4000, r, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(r, m, m);
    }
}

/* 0x8130FA3C */
MtxPtr Cube_GetMtx(void) { return cubeMtx; }

/* 0x8130FA48 */
static BOOL Cube_IsStepDone(void) {
    s32 max;
    s32 angle = Cube_RollAngle(&max);
    if (angle > max && cube->stepDoneFrame == 0)
        cube->stepDoneFrame = cube->stepFrame;
    if (cube->stepDoneFrame > 0 && cube->stepFrame >= cube->stepDoneFrame + cube->stepDelay)
        return TRUE;
    return FALSE;
}

/* 0x8130FAF0: initial fall; kicks the scene's landing shake on impact. */
static BOOL Cube_UpdateFall(void) {
    Scene *scene = Scene_Get();
    if (cube->holdFall)
        return FALSE;
    cube->fallHeight -= cube->fallVel;
    if (cube->fallHeight < 0.0f) {
        cube->fallHeight = 0.0f;
        if (cubeLandPending) {
            scene->landImpulse = scene->landImpulse0;
            scene->shakePhase = 0x7FFF;
            cubeLandPending = FALSE;
        }
    }
    cube->fallVel += cube->fallAccel;
    if (cube->fallFrame > cube->fallFrames + cube->fallDelay) {
        cubeLandPending = TRUE;
        return TRUE;
    }
    cube->fallFrame++;
    return FALSE;
}

/* 0x8130FBC4: grow to 1.5x while flipping riseTurns quarter turns about X. */
static void Cube_CalcRiseMtx(Mtx m) {
    Mtx t;
    f32 scale = 1.0f + (0.5f * cube->riseFrame) / cube->riseFrames;
    s32 maxAngle = cube->riseTurns << 14;
    s32 angle = (maxAngle * cube->riseFrame) / cube->riseFrames;
    f32 lift, minLift;
    if (scale > 1.5f)
        scale = 1.5f;
    if (angle > maxAngle)
        angle = maxAngle;

    PSMTXScale(m, scale, scale, scale);
    lift = cube->size / 2
         - ((cube->riseFrame * ((1.0f + scale) * cube->size)) / cube->riseFrames) * 0.5f;
    minLift = -(scale * cube->size * 0.5f);
    if (lift < minLift)
        lift = minLift;
    MtxRotTrans((s16)angle, 0, 0, t, scale * -cube->size * 0.5f, lift, scale * -cube->size * 0.5f);
    PSMTXConcat(t, m, m);
    MtxRotTrans(0, -0x4000, -0x4000, t, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(t, m, m);
}

/* Shared tail of the small-cube draw: world, height, then submit model. */
static void Cube_SubmitSmall(const Vec *scaleSrc) {
    Scene *scene = Scene_Get();
    Vec scale;
    scale.x = scale.y = scale.z = scene->modelScale;
    J3D_SetModelMtx(Cube_GetModel(), cubeMtx, &scale);
    J3D_SetViewMtx(Cube_GetModel(), Camera_GetView());
    J3D_CalcJoints(Cube_GetModel());
}

/* 0x8130FDE0: phases 2..4 (rise, drop, logo). */
static void Cube_DrawRise(void) {
    Mtx t;
    s32 n = cube->riseFrames;
    s32 r = n - cube->riseFrame;
    s32 c3 = cube->size * 3 / 2;

    PSMTXIdentity(cubeMtx);
    Cube_CalcRiseMtx(cubeMtx);
    MtxRotTrans(0, 0, 0, t, c3, c3, c3);
    PSMTXConcat(t, cubeMtx, cubeMtx);
    PSMTXConcat(Scene_GetWorldMtx(), cubeMtx, cubeMtx);

    if (cubePhase == CUBE_PHASE_RISE) {
        /* Ease-out: h * (1 - r^4 / n^4) */
        f32 h = cube->riseHeight;
        f32 y;
        if (r < 0)
            r = 0;
        y = h - ((f32)r * r * r * (h * r)) / (f32)(n * n * n * n);
        if (y > h)
            y = h;
        PSMTXTrans(t, 0.0f, y, 0.0f);
    } else {
        PSMTXTrans(t, 0.0f, cube->dropHeight, 0.0f);
    }
    PSMTXConcat(t, cubeMtx, cubeMtx);
    Cube_SubmitSmall(NULL);
}

/* 0x81310060 */
void Cube_Draw(void) {
    Mtx t, textMtx;
    Vec one;
    f32 textFrame;

    switch (cubePhase) {
    case CUBE_PHASE_FALL:
    case CUBE_PHASE_PATH: {
        s32 c3 = cube->size * 3 / 2;
        PSMTXIdentity(cubeMtx);
        Cube_CalcRollMtx(cubeMtx);
        Cube_CalcGridMtx(cubeMtx);
        MtxRotTrans(0, 0, 0, t, c3, c3, c3);
        PSMTXConcat(t, cubeMtx, cubeMtx);
        PSMTXConcat(Scene_GetWorldMtx(), cubeMtx, cubeMtx);
        if (cubePhase == CUBE_PHASE_FALL) {
            PSMTXTrans(t, 0.0f, cube->fallHeight, 0.0f);
            PSMTXConcat(t, cubeMtx, cubeMtx);
        }
        Cube_SubmitSmall(NULL);
        break;
    }
    case CUBE_PHASE_RISE:
    case CUBE_PHASE_DROP:
    case CUBE_PHASE_LOGO:
        Cube_DrawRise();
        break;
    }

    /* The "GAMECUBE" wordmark animation (BCK, 60 frames) is driven here. */
    if (cube->textFrame > cube->textDelay)
        textFrame = 60.0f * ((f32)(cube->textFrame - cube->textDelay) / cube->textFrames);
    else
        textFrame = 0.0f;
    PSMTXIdentity(textMtx);
    one.x = one.y = one.z = 1.0f;
    J3D_SetModelMtx(Scene_GetLogoText(), textMtx, &one);
    J3D_SetViewMtx(Scene_GetLogoText(), Camera_GetView());
    J3D_AnimateJoints(Scene_GetLogoText(), textFrame);
}

/* 0x81310494: choose and start the boot jingle (Z easter eggs). */
static void Cube_UpdateJingle(void) {
    PADStatus pads[4];
    PADRead(pads);
    if (Pad_GetEasterEgg() == 2) {          /* Z held on all four pads */
        if (cubeJinglePending && cube->jingleFrame > cube->jingleDelay[1]) {
            Sound_Play(1);
            cubeJinglePending = FALSE;
        }
    } else if (Pad_GetEasterEgg() == 1) {   /* Z held on the first pad */
        if (cubeJinglePending && cube->jingleFrame > cube->jingleDelay[2]) {
            Sound_Play(2);
            cubeJinglePending = FALSE;
        }
    } else if (cubeJinglePending && cube->jingleFrame > cube->jingleDelay[0]) {
        Sound_Play(0);
        cubeJinglePending = FALSE;
    }
    if (cubeJinglePending)
        cube->jingleFrame++;
}

/* 0x81310584: one logic frame. */
s32 Cube_Update(BOOL playJingle) {
    Scene *scene;
    BOOL textDone;

    cubePhase = cubeNextPhase;
    if (playJingle)
        Cube_UpdateJingle();

    switch (cubePhase) {
    case CUBE_PHASE_FALL:
        cubeNextPhase = Cube_UpdateFall();
        break;

    case CUBE_PHASE_PATH:
        if (Cube_IsStepDone()) {
            cube->stepFrame = 0;
            cube->stepDoneFrame = 0;
            cube->step++;
            if (Cube_PathStep()->dir == CUBE_DIR_END) {
                cube->done = TRUE;
                Cube_TickTiles();
            } else {
                Cube_StepGrid();
                cube->dir = Cube_PathStep()->dir;
            }
        }
        if (cube->dir == CUBE_DIR_RISE) {
            cubeNextPhase = CUBE_PHASE_RISE;
        } else {
            cubeNextPhase = CUBE_PHASE_PATH;
            cube->stepFrame++;
        }
        break;

    case CUBE_PHASE_RISE:
        cubeRunning = FALSE;
        cube->riseFrame++;
        cubeNextPhase = cube->riseFrame > cube->riseFrames + cube->riseDelay
                      ? CUBE_PHASE_DROP : CUBE_PHASE_RISE;
        break;

    case CUBE_PHASE_DROP:
        scene = Scene_Get();
        cube->dropHeight -= cube->dropVel;
        if (cube->dropHeight < 0.0f) {
            cube->dropHeight = 0.0f;
            if (cubeDropPending) {
                scene->dropImpulse = scene->dropImpulse0;
                scene->shakePhase = 0x7FFF;
                cubeDropPending = FALSE;
            }
        }
        cube->dropVel += cube->dropAccel;
        if (cube->dropFrame > cube->dropFrames + cube->dropDelay) {
            cubeDropPending = TRUE;
            cubeNextPhase = CUBE_PHASE_LOGO;
        } else {
            cube->dropFrame++;
            cube->textFrame++;
            if (cube->textFrame >= cube->textFrames + cube->textDelay)
                cube->textFrame = cube->textFrames + cube->textDelay;
            cubeNextPhase = CUBE_PHASE_DROP;
        }
        break;

    case CUBE_PHASE_LOGO:
        scene = Scene_Get();
        cube->logoFrame++;
        if (cube->logoFrame > cube->logoFrames) {
            cube->logoFrame = cube->logoFrames;
            cube->done = TRUE;
        }
        scene->logoAlpha = (cube->logoFrame * 0xFF) / cube->logoFrames;
        textDone = FALSE;
        cube->textFrame++;
        if (cube->textFrame >= cube->textFrames + cube->textDelay) {
            cube->textFrame = cube->textFrames + cube->textDelay;
            textDone = TRUE;
        }
        if (!textDone)
            cube->done = FALSE;
        if ((f32)scene->dropImpulse != 0.0f)
            cube->done = FALSE;
        break;
    }
    return 1;
}

/* 0x813108B0: TRUE until the cube lifts off (A-spin is allowed while TRUE). */
BOOL Cube_IsRunning(void) { return cubeRunning; }

/* 0x81310AD0: one textured quad on face plane `face`, in s16 1/16 units. */
static void Cube_DrawTile(s32 x, s32 y, s32 face, u8 alpha) {
    CubeState *c = Cube_Get();
    s32 inset = c->tileInset;
    s32 full = c->size * 16;
    s32 px = (x - 16) * c->size * 16;
    s32 py = (y - 16) * c->size * 16;
    GXColor col;
    s16 v[4][2];
    s16 uv[4][2] = { {0, 0}, {0, 0x4000}, {0x4000, 0x4000}, {0x4000, 0} };
    s32 i;

    col.r = c->tileR;
    col.g = c->tileG;
    col.b = c->tileB;
    col.a = alpha;
    GXSetChanMatColor(GX_COLOR0A0, col);
    J3D_LoadTexture(0, cubeTileTex);

    v[0][0] = -(px + inset); v[0][1] = -(py + inset);
    v[1][0] = -(px + full);  v[1][1] = -(py + inset);
    v[2][0] = -(px + full);  v[2][1] = -(py + full);
    v[3][0] = -(px + inset); v[3][1] = -(py + full);

    GXBegin(GX_QUADS, GX_VTXFMT1, 4);
    for (i = 0; i < 4; i++) {
        /* face 0: (a, 0, b)  face 1: (b, a, 0)  face 2: (0, b, a) */
        switch (face) {
        case 0: GXPosition3s16(v[i][0], 0, v[i][1]); break;
        case 1: GXPosition3s16(v[i][1], v[i][0], 0); break;
        case 2: GXPosition3s16(0, v[i][1], v[i][0]); break;
        }
        GXTexCoord2s16(uv[i][0], uv[i][1]);
    }
}

/* 0x813108B8 */
static void Cube_DrawTiles(u8 alpha) {
    CubeState *c = Cube_Get();
    Mtx m, nrm;
    s32 f, x, y;
    s32 c3 = c->size * 3 / 2;

    MtxRotTrans(0, 0, 0, m, c3, c3, c3);
    PSMTXConcat(Scene_GetWorldMtx(), m, m);
    PSMTXConcat(Camera_GetView(), m, m);
    MtxInverseTranspose(m, nrm);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    for (f = 0; f < CUBE_FACES; f++)
        for (x = 0; x < CUBE_GRID; x++)
            for (y = 0; y < CUBE_GRID; y++) {
                CubeTile *t = &cubeGrid[f][x][y];
                s32 a = 0xFF;
                if (t->group == 0)
                    continue;
                if (t->state == 1)
                    a = (c->stepFrame * 0xFF) / c->sideFrames;
                if (t->state == 2)
                    a = ((c->sideFrames - c->stepFrame) * 0xFF) / c->sideFrames;
                if (a > 0xFF) a = 0xFF;
                if (a < 0) a = 0;
                Cube_DrawTile(x, y, f, (u8)((a * alpha) / 255));
            }
}

/* 0x81310E24: tiles first, then the small cube, both scaled by `alpha`. */
void Cube_DrawBig(u8 alpha) {
    CubeState *c = Cube_Get();
    Mtx m;
    PSMTXConcat(Camera_GetView(), Cube_GetMtx(), m);
    GX_SetZMode(TRUE, GX_LEQUAL, FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetNumTevStages(1);
    Cube_DrawTiles(alpha);
    cubeSmallInst.alpha = (c->alpha * alpha) / 255;
    J3D_Draw(&cubeSmallInst, c->alpha);
}

/* 0x81310EF0 */
void Cube_InitModels(void) {
    cubeTileTex = ResGet(0x65);
    cubeSmall.data = ResGet(0xA8);
    cubeSmall.self = &cubeSmall;
    cubeSmall.alpha = 0xFF;
    J3D_InitModel(&cubeSmall.inst, 1);
}

/* 0x81310F4C */
void *Cube_GetModel(void) { return &cubeSmallInst; }
