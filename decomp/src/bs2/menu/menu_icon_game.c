// Partly matching: GameIcon_CalcMtx and GameIcon_UpdateEnter still differ.
//
// Game Play icon (PAL 1.0 BS2, 0x813230F0..0x81323C30): a ring of 16 small
// cubes behind the Game Play submenu. The cubes fly out one after the other,
// the ring spins and tilts while the submenu is open, and they shrink back to
// the centre when it closes.

#include "bs2/menu.h"

#define GAMEICON_CUBES 16

static J3DModel sGameIconUnused; /* never referenced, but takes its place in .bss */
static J3DModelInstance sGameIconInst[GAMEICON_CUBES];
static GameIconCube sGameCube[GAMEICON_CUBES];

static s16 sGameIconRotXStep = 100;
static s16 sGameIconRotZStep = 50;

static s16 sGameIconFrame;  /* frames since the ring started (capped at 300) */
static s16 sGameIconRotX;   /* tilt of the ring */
static s16 sGameIconRotZ;
static s16 sGameIconSpin;   /* spin around the ring axis */
static GXColorS10 sGameIconTint;
static BOOL sGameIconSettled;

/* 0x813230F0 */
void GameIcon_Init(J3DModel* model, BOOL first) {
    s16 i;

    if (first) {
        sGameIconInst[0].model = model;
        J3D_InitModel(&sGameIconInst[0], 0);
        for (i = 1; i < GAMEICON_CUBES; i++) {
            J3D_CopyInstance(&sGameIconInst[i], &sGameIconInst[0]);
        }
    }
    for (i = 0; i < GAMEICON_CUBES; i++) {
        sGameIconInst[i].alpha = 0;
    }
    sGameIconRotZStep = 50;
    sGameIconRotXStep = 100;
    sGameIconFrame = 0;
    GameIcon_InitRing();
}

/* 0x813231F8: cube i starts flying out at frame 10 * i and takes 20 frames. */
void GameIcon_InitRing(void) {
    s16 i;

    for (i = 0; i < GAMEICON_CUBES; i++) {
        sGameCube[i].start = i * 10;
        sGameCube[i].end = i * 10 + 20;
        sGameCube[i].angle = (i << 16) / GAMEICON_CUBES;
    }
    sGameIconRotX = 0;
    sGameIconRotZ = 0;
    sGameIconSpin = 0;
}

/* 0x813233C8 */
void GameIcon_UpdateFace(void) {
    GameParams* p;
    s16 i;
    s16 start;
    s16 end;

    p = Game_GetParams();
    for (i = 0; i < GAMEICON_CUBES; i++) {
        start = sGameCube[i].start;
        end = sGameCube[i].end;
        if (sGameIconFrame > start && sGameIconFrame < end) {
            sGameCube[i].radius = p->ringRadius * (sGameIconFrame - start) / (end - start);
            sGameIconInst[i].alpha = (sGameIconFrame - start) * 255 / (end - start);
        } else if (sGameIconFrame <= start) {
            sGameCube[i].radius = 0;
            sGameIconInst[i].alpha = 0;
        } else if (sGameIconFrame >= end) {
            sGameCube[i].radius = p->ringRadius;
            sGameIconInst[i].alpha = 255;
        }
        sGameCube[i].x = sGameCube[i].radius * SinS(sGameCube[i].angle - sGameIconSpin);
        sGameCube[i].y = sGameCube[i].radius * CosS(sGameCube[i].angle - sGameIconSpin);
    }
    sGameIconFrame++;
    sGameIconSpin += 0x199;
    if (sGameIconFrame > 300) {
        sGameIconFrame = 300;
        sGameIconRotX += sGameIconRotXStep;
        sGameIconRotZ += sGameIconRotZStep;
        if (sGameIconRotX > 32000) {
            sGameIconRotXStep = -100;
        }
        if (sGameIconRotX < 1000) {
            sGameIconRotXStep = 100;
        }
        if (sGameIconRotZ > 0x2000) {
            sGameIconRotZStep = -50;
        }
        if (sGameIconRotZ < -0x2000) {
            sGameIconRotZStep = 50;
        }
        if (sGameIconSpin > 0x1000) {
            sGameIconSpin %= 0x1000;
        }
    }
}

static void GameIcon_SetScale(Vec* out, f32 s) {
    Vec tmp = { 0.0f, 0.0f, 0.0f };

    tmp.x = s;
    tmp.y = s;
    tmp.z = s;
    *out = tmp;
}

/* 0x813235D4 */
void GameIcon_CalcMtx(void) {
    GameParams* p = Game_GetParams();
    GameIconCube* cube;
    J3DModelInstance* inst;
    Mtx m;
    Mtx rot;
    Vec scale;
    s16 i;

    for (i = 0, cube = &sGameCube[i], inst = &sGameIconInst[i]; i < GAMEICON_CUBES; i++) {
        PSMTXTrans(m, cube->x, cube->y, 0.0f);
        MtxRotTrans(sGameIconRotX, 0, sGameIconRotZ, rot, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(rot, m, m);
        MtxRotTrans(-0x4000, 0, 0, rot, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(rot, m, m);
        PSMTXConcat(Menu_GetCubeMtx(), m, m);
        GameIcon_SetScale(&scale, p->cubeScale);
        J3D_SetModelMtx(inst, m, scale);
        J3D_SetViewMtx(inst, Camera_GetView());
        J3D_CalcJoints(inst);
        cube++;
        inst++;
    }
}
/* 0x8132373C: the ring is drawn back to front once it stands still. */
void GameIcon_Draw(void) {
    GameParams* p = Game_GetParams();
    void* tev;
    s16 i;

    sGameIconTint.r = p->tint[0];
    sGameIconTint.g = p->tint[1];
    sGameIconTint.b = p->tint[2];
    sGameIconTint.a = p->tint[3];
    tev = sGameIconInst[0].model->material->tevColor;
    sGameIconInst[0].model->material->tevColor = &sGameIconTint;

    if (sGameIconFrame < 300) {
        for (i = 0; i < GAMEICON_CUBES; i++) {
            if (sGameIconInst[i].alpha > 0) {
                J3D_Draw(&sGameIconInst[i]);
            }
        }
    } else {
        for (i = 0; i < GAMEICON_CUBES / 2; i++) {
            if (sGameIconInst[7 - i].alpha > 0) {
                J3D_Draw(&sGameIconInst[7 - i]);
            }
            if (sGameIconInst[i + 8].alpha > 0) {
                J3D_Draw(&sGameIconInst[i + 8]);
            }
        }
    }

    sGameIconInst[0].model->material->tevColor = tev;
}

/* 0x81323868: fade every cube out while the submenu closes. */
void GameIcon_UpdateHide(void) {
    s32 i;
    s32 hidden = 0;

    for (i = 0; i < GAMEICON_CUBES; i++) {
        sGameIconInst[i].alpha -= 10;
        if (sGameIconInst[i].alpha < 0) {
            sGameIconInst[i].alpha = 0;
            hidden++;
        }
    }
    sGameIconFrame = 0;
    if (hidden == GAMEICON_CUBES) {
        sGameIconRotX = 0;
        sGameIconRotZ = 0;
        sGameIconSpin = 0;
    }
}

/* 0x813239E4: pull every cube back to the centre of the ring. */
void GameIcon_UpdateEnter(void) {
    u8 unused[4]; /* never used, but still takes a stack slot */
    s16 settled = 0;
    s16 i;
    s16 alpha;
    f32 v;
    GameParams* p;

    for (i = 0; i < GAMEICON_CUBES; i++) {
        alpha = sGameIconInst[i].alpha;
        alpha -= Game_GetParams()->fadeSpeed;
        if (alpha < 0) {
            alpha = 0;
        }
        sGameIconInst[i].alpha = alpha;

        v = sGameCube[i].x;
        p = Game_GetParams();
        if (v > 0.0f) {
            v -= p->shrinkSpeed;
            if (v < 0.0f) {
                v = 0.0f;
            }
        } else if (v < 0.0f) {
            v += p->shrinkSpeed;
            if (v > 0.0f) {
                v = 0.0f;
            }
        }
        sGameCube[i].x = v;
        settled += (v == 0.0f) ? 1 : 0;
        v = sGameCube[i].y;
        p = Game_GetParams();
        if (v > 0.0f) {
            v -= p->shrinkSpeed;
            if (v < 0.0f) {
                v = 0.0f;
            }
        } else if (v < 0.0f) {
            v += p->shrinkSpeed;
            if (v > 0.0f) {
                v = 0.0f;
            }
        }
        sGameCube[i].y = v;
        settled += (v == 0.0f) ? 1 : 0;
    }
    if (settled == GAMEICON_CUBES * 2) {
        sGameIconSettled = TRUE;
        sGameIconRotX = 0;
        sGameIconRotZ = 0;
        sGameIconSpin = 0;
    } else {
        sGameIconSettled = FALSE;
    }
    sGameIconFrame = 0;
}

/* 0x81323BC0 */
BOOL GameIcon_IsSettled(void) {
    s16 n = 0;
    s16 i;

    for (i = 0; i < GAMEICON_CUBES; i++) {
        if (sGameCube[i].x == 0.0f && sGameCube[i].y == 0.0f) {
            n++;
        }
    }
    if (n == GAMEICON_CUBES) {
        sGameIconSettled = TRUE;
    } else {
        sGameIconSettled = FALSE;
    }
    return sGameIconSettled;
}
