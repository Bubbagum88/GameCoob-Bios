// Intro scene state machine (PAL 1.0 BS2, 0x8130BDB8..0x8130C250): the
// per-frame update and 2D draw dispatch, init and the scene defaults.

#include "bs2/scene.h"
#include "bs2/cube.h"
#include "bs2/j3d.h"
#include "bs2/menu.h"
#include "bs2/res.h"

/* Not yet decompiled */
void InitDVDLoader(void);                                   /* 0x813020C8 */
BOOL fn_813020B8(void);                                     /* 0x813020B8 disc is bootable */
BOOL fn_81302688(void);                                     /* 0x81302688 drive error */

static s32 sAmbientPending = 1;      /* 0x814ACFF0 */

static s32 sState;                   /* 0x814AD3B8 */
static s32 sNextState;               /* 0x814AD3BC */
static Scene *sScene;                /* 0x814AD3C0 */

static Scene sSceneDefault;          /* 0x814AF878 */
Mtx sWorldMtx;                       /* 0x814AF950 */

static void Scene_SetDefaults(Scene *s);
static void Scene_Reset(Scene *s);

/* 0x8130BDB8 */
void Scene_Update(void) {
    sState = sNextState;
    switch (sState) {
    case SCENE_DISC:
        sNextState = Scene_UpdateDisc();
        Scene_DrawWorld();
        break;
    case SCENE_MODE0:
        sNextState = Scene_UpdateMode0();
        Scene_DrawWorld();
        break;
    case SCENE_NODISC:
        sNextState = Scene_UpdateNoDisc();
        Scene_DrawWorld();
        break;
    case SCENE_MODE3:
        sNextState = Scene_UpdateMode3();
        Scene_DrawWorld();
        break;
    case SCENE_SPINOUT:
        sNextState = Scene_SpinOut();
        sAmbientPending = TRUE;
        break;
    case SCENE_MENU:
        if (sAmbientPending) {
            Sound_StartAmbient(0);
            sAmbientPending = FALSE;
        }
        sNextState = Menu_Update();
        break;
    case 6:
        break;
    }
}

/* 0x8130BE78 */
void Scene_Draw2D(void) {
    switch (sState) {
    case SCENE_NODISC:
    case SCENE_MODE3:
        Scene_Draw2DNoDisc();
        break;
    case SCENE_MODE0:
        Scene_Draw2DMode0();
        break;
    case SCENE_DISC:
        Scene_Draw2DDisc();
        break;
    case SCENE_SPINOUT:
        Scene_Draw2DSpinOut();
        break;
    case SCENE_MENU:
        Scene_Draw2DMenu();
        break;
    case 6:
        break;
    }
}

/* 0x8130BEDC */
void fn_8130BEDC(void) {
}

/* 0x8130BEE0 */
s32 Scene_GetState(void) {
    return sState;
}

/* 0x8130BEE8 */
void Scene_Init(BOOL first) {
    DispSettings *disp = Disp_GetSettings();

    InitDVDLoader();
    if (fn_813020B8()) {
        sState = SCENE_DISC;
    } else if (Boot_SramWasLost()) {
        sState = SCENE_MODE0;
    } else if (Disc_GetState() == DISC_COVER_OPEN || fn_81302688()) {
        sState = SCENE_MODE3;
    } else {
        sState = SCENE_NODISC;
    }
    sNextState = sState;
    disp->menuFlag = TRUE;
    if (first) {
        sScene = &sSceneDefault;
        Scene_SetDefaults(sScene);
    }
    Scene_Reset(sScene);
    if (first) {
        Scene_InitModels();
    }
    Cube_Init(first);
    Menu_Init(first);
    Game_InitParams(first);
    Card_InitParams(first);
    Option_InitParams(first);
    Clock_InitParams(first);
}

/* 0x8130BFF0 */
static void Scene_SetDefaults(Scene *s) {
    s->unk0 = 0;
    s->unk2 = 100;
    s->spinThreshold = 5000;
    s->spinTilt = 0.7f;
    s->outFadeFrames = 30;
    s->squashXZ = 0.5f;
    s->stretchY = 1.0f;
    s->spinAccelStep = 1.5f;
    s->unk1C = 100.0f;
    s->shakeAlpha = 255;
    s->unk58 = 255;
    s->landDecay = 0.93f;
    s->landLift = 0.015f;
    s->landImpulse0 = 4000;
    s->dropDecay = 0.925f;
    s->dropLift = 0.005f;
    s->dropImpulse0 = 12000;
    s->shakeStep = 1000;
    s->m0LogoFrames = 30;
    s->m0TextFrames = 30;
    s->unk70 = 30;
    s->discFadeFrames = 30;
    s->askFadeFrames = 30;
    s->askGap = 8;
    s->discLogoDim = 150;
    s->errFadeFrames = 30;
    s->fatalFadeFrames = 30;
    s->outCoastFrames = 15;
    s->outTurnFrames = 80;
    s->outSettleFrames = 50;
    s->outSettle = 5000;
    s->outTurns = 3;
    s->outSpeed = s->spinThreshold;
    s->outFaceFrames = 100;
    s->isoX = 0x2000;
    s->isoY = -0x1914;
    s->isoZ = -0x1555;
    s->fatal = 0;
    s->whooshMin = 0.4f;
    s->outWhooshMin = 0.5f;
}

/* 0x8130C130 */
static void Scene_Reset(Scene *s) {
    s->coverOpened = 0;
    s->modelScale = 1.0f;
    s->spin = 0;
    s->spinSpeed = 0.0f;
    s->spinAccel = 0.0f;
    s->easeSpeed = 0.0f;
    s->easeFrom = 0;
    s->lastSpin = 0;
    sScene->outFrame = 1;
    sScene->outFirst = 1;
    s->spinRest = 0;
    s->easeFrame = 60;
    s->wobblePhase = 2000;
    s->wobbleAmp = 0;
    s->spinTilt = 0.0f;
    s->unk50 = 0.1f;
    s->unk54 = s->squashXZ;
    s->bgAlpha = 255;
    s->logoAlpha = 0;
    s->landImpulse = 0;
    s->dropImpulse = 0;
    s->shakePhase = 0;
    s->m0LogoFade = s->m0LogoFrames;
    s->m0TextFade = 0;
    s->unk6C = 0;
    s->m0State = 0;
    s->discFade = 0;
    s->discState = 0;
    s->askFade = 0;
    s->askChoice = 1;
    s->errFade = 0;
    s->fatalFade = 0;
    s->m3Wait = 120;
    s->unkC0 = 0;
    s->unkC2 = 1;
    s->unkC4 = 0;
    s->unkC6 = 0;
    s->unkC8 = 0;
    s->unkCA = 1;
    s->unkCC = 0;
    s->unkCE = 0;
    s->langSelect = 1;
    s->lang = Lang_Get();
    s->langFade = 0;
}

/* 0x8130C248 */
Scene *Scene_Get(void) {
    return sScene;
}
