#ifndef BS2_SCENE_H
#define BS2_SCENE_H

#include "types.h"
#include "bs2/mtx.h"
#include "bs2/j3d.h"

typedef enum SceneState {
    SCENE_MODE0 = 0,    /* SRAM was lost: language select, then the logo */
    SCENE_NODISC = 1,   /* default boot: no disc inserted */
    SCENE_DISC = 2,     /* disc present, boot after the logo */
    SCENE_MODE3 = 3,    /* cover open or drive error */
    SCENE_SPINOUT = 4,  /* cube spun fast enough: transition to the menu */
    SCENE_MENU = 5
} SceneState;

/* 0x814AF878, 0xD8 bytes. Defaults from Scene_SetDefaults in (). */
typedef struct Scene {
    s16 unk0;            /* 0x00 (0) */
    s16 unk2;            /* 0x02 (100) */
    f32 modelScale;      /* 0x04 */
    s16 spin;            /* 0x08 Y rotation of the whole scene */
    s16 spinRest;        /* 0x0A */
    s32 spinThreshold;   /* 0x0C (5000): spin speed that starts the menu */
    f32 spinSpeed;       /* 0x10 */
    f32 spinAccel;       /* 0x14 */
    f32 spinAccelStep;   /* 0x18 (1.5) */
    f32 unk1C;           /* 0x1C (100) */
    s16 wobblePhase;     /* 0x20 */
    s16 wobbleAmp;       /* 0x22 */
    f32 spinTilt;        /* 0x24 (0.7) */
    s16 outFadeFrames;   /* 0x28 (30) spin-out: frames the intro models fade */
    s16 outTurns;        /* 0x2A (3) spin-out: extra full turns */
    s16 outSettle;       /* 0x2C (5000) spin-out: angle reached after the turns */
    s16 outStart;        /* 0x2E spin when the spin-out began */
    s32 outCoastFrames;  /* 0x30 (15) */
    s32 outTurnFrames;   /* 0x34 (80) */
    s32 outSettleFrames; /* 0x38 (50) */
    s32 outFrame;        /* 0x3C */
    s32 outFaceFrames;   /* 0x40 (100) menu face fades in over the last N frames */
    f32 outSpeed;        /* 0x44 (= spinThreshold) */
    f32 squashXZ;        /* 0x48 (0.5) */
    f32 stretchY;        /* 0x4C (1.0) */
    f32 unk50;           /* 0x50 (0.1) */
    f32 unk54;           /* 0x54 (= squashXZ) */
    u8 unk58;            /* 0x58 (255) */
    u8 shakeAlpha;       /* 0x59 (255) inner background alpha at full shake */
    u8 bgAlpha;          /* 0x5A */
    u8 logoAlpha;        /* 0x5B */
    s16 landImpulse;     /* 0x5C */
    s16 dropImpulse;     /* 0x5E */
    s16 landImpulse0;    /* 0x60 (4000) */
    s16 dropImpulse0;    /* 0x62 (12000) */
    s16 shakePhase;      /* 0x64 */
    s16 m0LogoFade;      /* 0x66 mode 0: logo fade, from m0LogoFrames down */
    s16 m0LogoFrames;    /* 0x68 (30) */
    s16 m0TextFade;      /* 0x6A mode 0: "settings lost" text fade */
    s16 unk6C;           /* 0x6C */
    s16 m0TextFrames;    /* 0x6E (30) */
    s16 unk70;           /* 0x70 (30) */
    s16 m0State;         /* 0x72 */
    s16 discFade;        /* 0x74 disc boot: logo fade-out */
    s16 discFadeFrames;  /* 0x76 (30) */
    s16 discLogoDim;     /* 0x78 (150) logo alpha removed at full discFade */
    s16 askFade;         /* 0x7A disc boot: yes/no prompt fade */
    s16 askFadeFrames;   /* 0x7C (30) */
    s16 discState;       /* 0x7E */
    s16 askChoice;       /* 0x80 0 yes, 1 no */
    s16 askGap;          /* 0x82 (8) */
    s16 errFade;         /* 0x84 "disc could not be read" fade */
    s16 errFadeFrames;   /* 0x86 (30) */
    s16 fatalFade;       /* 0x88 "an error has occurred" fade */
    s16 fatalFadeFrames; /* 0x8A (30) */
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
    s16 outFirst;        /* 0xAE set until the spin-out has started */
    s16 fatal;           /* 0xB0 disc reported DISC_FATAL */
    s16 coverOpened;     /* 0xB2 */
    s16 discWait;        /* 0xB4 */
    s16 m3Wait;          /* 0xB6 (120) */
    f32 whooshMin;       /* 0xB8 (0.4) */
    f32 outWhooshMin;    /* 0xBC (0.5) */
    s16 unkC0, unkC2, unkC4, unkC6, unkC8, unkCA, unkCC, unkCE;
    s16 langSelect;      /* 0xD0 mode 0: the language list is up */
    s16 lang;            /* 0xD2 language under the cursor */
    s16 langFade;        /* 0xD4 */
} Scene;

/* scene.c */
extern Mtx sWorldMtx;
void Scene_Update(void);
void Scene_Draw2D(void);
s32 Scene_GetState(void);
void Scene_Init(BOOL first);
Scene *Scene_Get(void);

/* scene_intro.c */
void Scene_DrawWorld(void);
s32 Scene_UpdateNoDisc(void);
s32 Scene_UpdateMode0(void);
s32 Scene_UpdateDisc(void);
s32 Scene_UpdateMode3(void);
void Scene_InitModels(void);
void Scene_Draw2DNoDisc(void);
void Scene_Draw2DMode0(void);
void Scene_Draw2DDisc(void);
void Scene_Draw2DSpinOut(void);
void Scene_Draw2DMenu(void);
MtxPtr Scene_GetWorldMtx(void);
void Scene_LoadLangRes(void);
J3DModelInstance *Scene_GetBgInner(void);
J3DModelInstance *Scene_GetBigCube(void);
J3DModelInstance *Scene_GetLogo(void);
void *Scene_GetLogoText(void);
J3DModelInstance *Scene_GetBgOuter(void);

/* scene_spinout.c */
s32 Scene_SpinOut(void);
MtxPtr Scene_GetSpinOutMtx(void);

#endif
