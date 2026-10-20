#ifndef BS2_SCENE_H
#define BS2_SCENE_H

#include "types.h"
#include "bs2/mtx.h"

typedef enum SceneState {
    SCENE_MODE0 = 0,    /* fn_813020C0 != 0 */
    SCENE_NODISC = 1,   /* default boot: no disc inserted */
    SCENE_DISC = 2,     /* disc present, boot after the logo */
    SCENE_MODE3 = 3,    /* disc state 0x13 or error */
    SCENE_SPINOUT = 4,  /* cube spun fast enough: transition to the menu */
    SCENE_MENU = 5
} SceneState;

/* 0x814AF878 */
typedef struct Scene {
    s16 unk0, unk2;
    f32 modelScale;      /* 0x04 */
    s16 spin;            /* 0x08 Y rotation of the whole scene */
    s16 spinRest;        /* 0x0A */
    s16 spinThreshold;   /* 0x0C (5000): spin speed that starts the menu */
    f32 spinSpeed;       /* 0x10 */
    f32 spinAccel;       /* 0x14 */
    f32 spinAccelStep;   /* 0x18 (1.5) */
    f32 unk1C;
    s16 wobblePhase;     /* 0x20 */
    s16 wobbleAmp;       /* 0x22 */
    f32 spinTilt;        /* 0x24 (0.7) */
    u8 pad28[0x48 - 0x28];
    f32 squashXZ;        /* 0x48 (0.5) */
    f32 stretchY;        /* 0x4C (1.0) */
    u8 pad50[0x5A - 0x50];
    u8 bgAlpha;          /* 0x5A */
    u8 logoAlpha;        /* 0x5B */
    s16 landImpulse;     /* 0x5C */
    s16 dropImpulse;     /* 0x5E */
    s16 landImpulse0;    /* 0x60 (4000) */
    s16 dropImpulse0;    /* 0x62 (12000) */
    s16 shakePhase;      /* 0x64 */
    u8 pad66[0x8C - 0x66];
    f32 landDecay;       /* 0x8C (0.93) */
    f32 dropDecay;       /* 0x90 (0.925) */
    f32 landLift;        /* 0x94 (0.015) */
    f32 dropLift;        /* 0x98 (0.005) */
    s16 shakeStep;       /* 0x9C (1000) */
    s16 isoX, isoY, isoZ;/* 0x9E..0xA2 (0x2000, -0x1914, -0x1555) */
    f32 easeSpeed;       /* 0xA4 */
    s16 easeFrame;       /* 0xA8 */
    s16 lastSpin;        /* 0xAA */
    s16 easeFrom;        /* 0xAC */
    u8 padAE[0xB8 - 0xAE];
    f32 whooshMin;       /* 0xB8 (0.4) */
    f32 unkBC;           /* 0xBC (0.5) */
} Scene;

Scene *Scene_Get(void);
void Scene_Init(BOOL first);
s32 Scene_Update(void);
void Scene_Draw2D(void);
MtxPtr Scene_GetWorldMtx(void);
void *Scene_GetLogoText(void);

#endif
