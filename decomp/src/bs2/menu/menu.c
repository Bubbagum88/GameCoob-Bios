// Main menu cube (PAL 1.0 BS2, 0x81310F58..0x81315724): resources, the cube
// state machine, input, cube rotation/zoom, fader bookkeeping and drawing of
// the faces, submenus and button guide. One unit: the drawing half shares
// the .sdata2 and .bss pools with the rest.

#include "bs2/menu.h"
#include "bs2/scene.h"
#include "bs2/res.h"
#include "bs2/glh.h"
#include "bs2/menu_fx_cube.h"
#include <dolphin/os.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <string.h>

static MenuWork gMenuWork;          /* 0x814B5EA0 */
static Mtx sCubeMtx;                /* 0x814B6058 menu cube model matrix */
static J3DModel sIconModel;         /* 0x814B6088 small cube for the submenu icons (res 178) */
static void *sGuideSth[6];          /* 0x814B60C0 button guide strings per language (res 116..121) */
static void *sDigitTex[10];         /* 0x814B60D8 digits 0..9 (res 91..100) */
static void *sWeekdayTex[7];        /* 0x814B6100 weekday names (current language) */
static Fader sGuideIcon[6];         /* 0x814B611C button glyph groups (see Menu_UpdateGuide) */
static Fader sGuideText[8];         /* 0x814B617C button guide labels */
static Fader sOptionFade[3];        /* 0x814B61FC submenu fade-ins, delays subFadeDelay[] */
static Fader sCardFade[3];          /* 0x814B622C */
static Fader sClockFade[3];         /* 0x814B625C */
static Fader sGameFade[3];          /* 0x814B628C */
static Fader sDiscBanner;           /* 0x814B62BC Game face: disc with banner */
static Fader sDiscNone;             /* 0x814B62CC Game face: "no disc" */
static Fader sDiscUnknown;          /* 0x814B62DC Game face: disc without banner */
static Fader sErrorMsg;             /* 0x814B62EC fatal disc error text (string 4) */
static Fader sLangGuide[6];         /* 0x814B62FC cross-fade of the guide text between languages */

/* .sbss: CW lays globals out in reverse definition order */
s16 gGuideLang;                     /* 0x814AD438 language the guide text is shown in */
u16 gGuideBlink;                    /* 0x814AD436 */
s16 gGuideAlpha;                    /* 0x814AD434 */
void *gSoundModeTex[2];             /* 0x814AD42C mono / stereo label */
s32 gSlotFill[2];                   /* 0x814AD424 card slot fill, 0..255 */
s16 gSlotGlow[2];                   /* 0x814AD420 card slot glow, 60..255 */
void *gDialogSth;                   /* 0x814AD41C res 109..114 */
void *gDialogGlh;                   /* 0x814AD418 res 108 */
void *gGuideGlh;                    /* 0x814AD414 button guide layout, res 115 */
void *gMenuTexBank;                 /* 0x814AD410 TXH0 res 167 */
void *gClockFaceGlh;                /* 0x814AD40C */
void *gOptionFaceGlh;               /* 0x814AD408 */
void *gCardFaceGlh;                 /* 0x814AD404 */
void *gGameFaceGlh;                 /* 0x814AD400 */
void *gTopFaceGlh;                  /* 0x814AD3FC front face layout */
s32 gMenuNextState;                 /* 0x814AD3F8 */
s32 gMenuState;                     /* 0x814AD3F4 MenuState */
MenuWork *gMenu;                    /* 0x814AD3F0 */

extern s32 gSramLostPending;        /* 0x814AD000 */
static const Vec sZeroVec = { 0.0f, 0.0f, 0.0f };   /* 0x8137EC28 */

/* 0x8137EC34: SRT of each face layout relative to the cube. The missing
   translation component of each entry is filled in by the code. */
static const SRT sFaceSRT[6] = {
    { 1.0f, -1.0f, 1.0f,       0,       0, 0, 0, -144.0f, 144.0f,  144.0f }, /* top */
    { 1.0f, -1.0f, 1.0f,       0,       0, 0, 0, -144.0f, 144.0f,    0.0f }, /* work copy */
    { 1.0f, -1.0f, 1.0f,  0x4000,       0, 0, 0, -144.0f,   0.0f,  144.0f }, /* card */
    { 1.0f, -1.0f, 1.0f, -0x4000,       0, 0, 0, -144.0f,   0.0f, -144.0f }, /* game */
    { 1.0f, -1.0f, 1.0f,       0,  0x4000, 0, 0,    0.0f, 144.0f,  144.0f }, /* clock */
    { 1.0f, -1.0f, 1.0f,       0, -0x4000, 0, 0,    0.0f, 144.0f, -144.0f }, /* option */
};

static void Menu_InitRes(BOOL first);
static void Menu_SetPalTextLayout(MenuWork *m);
static void Menu_SetDefaults(MenuWork *m);
static void Menu_ResetCube(MenuWork *m);

/* 0x81310F58: reload the per-language face layouts after a language change. */
#pragma auto_inline on
void Menu_LoadLangRes(void) {
    s32 i;

    gGameFaceGlh = ResGet(Res_GameFaceLayout());
    gTopFaceGlh = ResGet(Res_TopFaceLayout());
    gCardFaceGlh = ResGet(Res_CardFaceLayout());
    gClockFaceGlh = ResGet(Res_ClockFaceLayout());
    gOptionFaceGlh = ResGet(Res_OptionFaceLayout());
    gDialogSth = ResGet(Res_DialogStrings());
    for (i = 0; i < 7; i++) {
        sWeekdayTex[i] = ResGet(Res_WeekdayTexBase() + i);
    }
    gSoundModeTex[0] = ResGet(Res_MonoTex());
    gSoundModeTex[1] = ResGet(Res_StereoTex());
}
#pragma auto_inline reset

/* 0x81311014 */
static void Menu_InitRes(BOOL first) {
    s32 i;

    if (first) {
        gGuideGlh = ResGet(Res_GuideLayout());
        gMenuTexBank = ResGet(Res_MenuTexBank());
        gDialogGlh = ResGet(Res_DialogLayout());
        for (i = 0; i < 6; i++) {
            sGuideSth[i] = ResGet(Res_GuideStrings(i));
        }
        Menu_LoadLangRes();
        Txh_SetBank(gMenuTexBank);
        for (i = 0; i < 10; i++) {
            sDigitTex[i] = ResGet(Res_DigitTexBase() + i);
        }
    }

    gSlotGlow[0] = 0;
    gSlotGlow[1] = 0;
    gSlotFill[0] = 0;
    gSlotFill[1] = 0;
    gGuideAlpha = 0;
    gGuideBlink = 0;
    gGuideLang = Lang_Get();

    /* Button glyph groups: 0..2 used inside submenus, 3 on the front face,
     * 4..5 on a selected face. */
    Fader_Init(&sGuideIcon[0]);
    sGuideIcon[0].id = 0;
    Fader_Init(&sGuideIcon[1]);
    sGuideIcon[1].id = 0;
    Fader_Init(&sGuideIcon[2]);
    sGuideIcon[2].id = 0;
    Fader_Init(&sGuideIcon[3]);
    sGuideIcon[3].id = 0;
    Fader_Init(&sGuideIcon[4]);
    sGuideIcon[4].id = 0;
    Fader_Init(&sGuideIcon[5]);
    sGuideIcon[5].id = 0;

    /* Button guide labels, ids index the guide STH0 table */
    Fader_Init(&sGuideText[0]); sGuideText[0].id = GUIDE_MENU_SELECT;
    Fader_Init(&sGuideText[1]); sGuideText[1].id = GUIDE_CANCEL;
    Fader_Init(&sGuideText[2]); sGuideText[2].id = GUIDE_CONFIRM;
    Fader_Init(&sGuideText[7]); sGuideText[7].id = GUIDE_CONFIRM2;
    Fader_Init(&sGuideText[3]); sGuideText[3].id = GUIDE_SELECT;
    Fader_Init(&sGuideText[4]); sGuideText[4].id = GUIDE_CHANGE;
    Fader_Init(&sGuideText[6]); sGuideText[6].id = GUIDE_FINISH;
    Fader_Init(&sGuideText[5]); sGuideText[5].id = GUIDE_CANCEL2;

    /* Each submenu fades in with three staggered faders (delays 0, 20, 70). */
    for (i = 0; i < 3; i++) {
        Fader_Init(&sOptionFade[i]);
        Fader_Init(&sCardFade[i]);
        Fader_Init(&sClockFade[i]);
        Fader_Init(&sGameFade[i]);
        sOptionFade[i].delay = gMenu->subFadeDelay[i];
        sCardFade[i].delay = gMenu->subFadeDelay[i];
        sClockFade[i].delay = gMenu->subFadeDelay[i];
        sGameFade[i].delay = gMenu->subFadeDelay[i];
    }

    Fader_Init(&sDiscBanner);
    Fader_Init(&sDiscNone);
    Fader_Init(&sDiscUnknown);
    Fader_Init(&sErrorMsg);
    sErrorMsg.id = 4;          /* "An error has occurred." */
    for (i = 0; i < 6; i++) {
        Fader_Init(&sLangGuide[i]);
    }
}

/* 0x81311314: called from Scene_Init. */
void Menu_Init(BOOL first) {
    if (first) {
        gMenu = &gMenuWork;
        Menu_SetDefaults(gMenu);
    }
    Menu_ResetCube(gMenu);
    gMenuState = MENU_TOP;
    gMenuNextState = MENU_TOP;
    Menu_InitRes(first);
    if (first) {
        sIconModel.data = ResGet(178);      /* small cube model */
        J3D_LoadModelData(&sIconModel);
        Fx_InitCubeModel();
        /* mosaic shapes (res 179..184) and CAFD keyframes (res 37) */
        Fx_Init(ResGet(Res_FxGlyphs()), ResGet(37));
    }
    CardIcon_Init(&sIconModel, first);
    GameIcon_Init(&sIconModel, first);
    OptionIcon_Init(&sIconModel, first);
    ClockIcon_Init(&sIconModel, first);
}

/* Text layout for language code 3, which PAL never reports (codes are
   4..9): leftover from another region. Inlined into Menu_SetDefaults. */
#pragma auto_inline on
static void Menu_SetLang3TextLayout(MenuWork *m) {
    m->fxTitle[0].x = 10.0f;
    m->fxTitle[0].y = -22.0f;
    m->fxTitle[0].scale = 0.6f;
    m->fxTitle[1].scale = 0.7f;
    m->fxSel[0] = -18.0f;
    m->fxSel[1] = 9.0f;
    m->fxSel[2] = -47.0f;
    m->fxSel[3] = 9.0f;
    m->fxCursorScale = 0.66f;
    m->fxItem[0].scale = 0.74f;
    m->fxItem[1].scale = 0.74f;
}
#pragma auto_inline reset

/* 0x813113EC: PAL text layout overrides for the mosaic screens (longer words). */
static void Menu_SetPalTextLayout(MenuWork *m) {
    m->fxTitle[0].y = -84.0f;
    m->fxTitle[0].scale = 0.55f;
    m->fxTitle[1].y = -35.0f;
    m->fxTitle[1].scale = 0.64f;
    m->fxTitle[2].y = 10.0f;
    m->fxSel[0] = -10.0f;
    m->fxSel[1] = 31.0f;
    m->fxSel[2] = -30.0f;
    m->fxSel[3] = 31.0f;
    m->fxSel[4] = -15.0f;
    m->fxSel[5] = 13.0f;
    m->fxSel[6] = 15.0f;
    m->fxSel[7] = 13.0f;
    m->fxCursorScale = 0.66f;
    m->fxItem[0].scale = 0.74f;
    m->fxItem[1].scale = 0.74f;
}

/* 0x81311464 */
static void Menu_SetDefaults(MenuWork *m) {
    m->rotStep = 750;
    m->zoomDepth = -70.0f;
    m->zoomInStep = 0x730;
    m->zoomOutStep = 0x578;
    m->bgMinAlpha = 0;
    m->zoomSpin = 0.0f;
    m->unk00 = 0.0f;
    m->fxDepth = 5.0f;

    m->wobbleSpeed[0] = 50;
    m->wobbleSpeed[1] = 25;
    m->wobbleSpeed[2] = 25;
    m->wobbleAmp[0] = 350;
    m->wobbleAmp[1] = 1000;
    m->wobbleAmp[2] = 1000;
    m->wobbleSpeedTX = 25;
    m->wobbleSpeedTY = 50;
    m->wobbleAmpTX = 15;
    m->wobbleAmpTY = 6;
    m->wobblePhaseRY = -15000;
    m->wobblePhaseTX = -0x4000;
    m->inputAlways = 0;

    m->fxTitle[0].x = 10.0f;
    m->fxTitle[0].y = -11.0f;
    m->fxTitle[0].scale = 0.82f;
    m->fxTitle[0].spacing = 0.2f;
    m->fxTitle[1].x = 0.0f;
    m->fxTitle[1].y = 13.0f;
    m->fxTitle[1].scale = 0.82f;
    m->fxTitle[1].spacing = 0.2f;
    m->fxTitle[2].x = 0.0f;
    m->fxTitle[2].y = 19.0f;
    m->fxTitle[2].scale = 0.66f;
    m->fxTitle[2].spacing = 0.2f;
    m->fxLineOfs[0] = -2;
    m->fxLineOfs[1] = -2;
    m->fxLineOfs[2] = -2;
    m->fxLineOfs[3] = -2;
    m->fxLineOfs[4] = -1;
    m->fxLineOfs[5] = -1;
    m->fxSel[0] = -20.0f;
    m->fxSel[1] = 1.0f;
    m->fxSel[2] = 0.0f;
    m->fxSel[3] = 1.0f;
    m->fxSel[4] = -15.0f;
    m->fxSel[5] = -3.0f;
    m->fxSel[6] = 15.0f;
    m->fxSel[7] = -3.0f;
    m->fxSel2[0] = -50.0f;
    m->fxSel2[1] = 13.0f;
    m->fxSel2[2] = 50.0f;
    m->fxSel2[3] = 13.0f;
    m->fx124 = 30;
    m->fx126 = 10;
    m->fxBlinkPeriod = 25;
    m->fxF0 = 10.0f;
    m->fxItem[0].x = 0.0f;
    m->fxItem[0].y = -46.0f;
    m->fxItem[0].scale = 0.82f;
    m->fxItem[0].spacing = 0.2f;
    m->fxItem[1].x = 0.0f;
    m->fxItem[1].y = -20.0f;
    m->fxItem[1].scale = 0.82f;
    m->fxItem[1].spacing = 0.2f;
    m->fxCursorX = -8.0f;
    m->fxCursorY = -8.0f;
    m->fxCursorScale = 0.8f;
    m->fxFadeIn = 10;
    m->fxFadeOut = 10;
    m->fxCubeScale = 1.0f;
    m->fxB0 = 100;
    m->fxB4 = 2.0f;
    m->fxB8 = 5.0f;
    m->fxBC = 30.0f;
    m->fxC0 = 3.0f;
    m->fxC4 = 0.45f;
    m->fxC8 = 120;
    m->fxCC = 0.4f;
    m->fxD0 = 2.0f;
    m->fxD4 = 10;
    m->fxReadyCount = 58;
    m->fxReadyRatio = 0.9f;
    m->fxDC = 200;
    m->fxDE = 200;
    m->fxE0 = 200.0f;
    m->fxE4 = 200.0f;
    m->fxE8 = 200.0f;
    m->fxCountOverride = -1;

    /* Mosaic cube colours (R,G,B of the light and dark shade, interleaved). */
    m->gameColor[0] = 130; m->gameColor[2] = 50;  m->gameColor[4] = 160;
    m->gameColor[1] = 50;  m->gameColor[3] = 20;  m->gameColor[5] = 70;
    m->optionColor[0] = 180; m->optionColor[2] = 255; m->optionColor[4] = 0;
    m->optionColor[1] = 20;  m->optionColor[3] = 100; m->optionColor[5] = 50;
    m->optionColor[6] = 120; m->optionColor[8] = 220; m->optionColor[10] = 35;
    m->optionColor[7] = 0;   m->optionColor[9] = 80;  m->optionColor[11] = 25;
    m->optionColor[12] = 40; m->optionColor[14] = 120; m->optionColor[16] = 0;
    m->optionColor[13] = 0;  m->optionColor[15] = 40;  m->optionColor[17] = 10;
    m->clockColor[0] = 240; m->clockColor[2] = 255; m->clockColor[4] = 0;
    m->clockColor[1] = 100; m->clockColor[3] = 50;  m->clockColor[5] = 20;
    m->clockColor[6] = 235; m->clockColor[8] = 185; m->clockColor[10] = 0;
    m->clockColor[7] = 70;  m->clockColor[9] = 40;  m->clockColor[11] = 0;
    m->clockColor[12] = 160; m->clockColor[14] = 90; m->clockColor[16] = 0;
    m->clockColor[13] = 50;  m->clockColor[15] = 40; m->clockColor[17] = 0;
    m->fx17E = 1;

    m->lightAttn = 500.0f;
    m->lightAttnK = 0.375f;
    m->lightPos.x = 296.0f;
    m->lightPos.y = 224.0f;
    m->lightPos.z = 474.0f;
    m->lightCutoff = 36.0f;
    m->guideColorA[0] = 0;
    m->guideColorA[1] = 200;
    m->guideColorA[2] = 140;
    m->guideColorB[0] = 210;
    m->guideColorB[1] = 0;
    m->guideColorB[2] = 70;
    m->subFadeDelay[0] = 0;
    m->subFadeDelay[1] = 20;
    m->subFadeDelay[2] = 70;

    /* Language code 3 never occurs on PAL (codes are 4..9): leftover from
     * another region's layout. */
    if (Lang_GetCode() == 3) {
        Menu_SetLang3TextLayout(m);
    }
    Menu_SetPalTextLayout(m);
}

/* 0x81311850 */
static void Menu_ResetCube(MenuWork *m) {
    m->wobbleFrame = 0;
    m->rotX = 0;
    m->rotY = 0;
    m->zoom = 0;
}

/* True when the cube sits exactly on the front face (or input is forced). */
#define AT_FRONT()      ((gMenu->rotX == 0 && gMenu->rotY == 0) || gMenu->inputAlways)
#define AT_FACE_X(a)    ((gMenu->rotX == (a) && gMenu->zoom == 0) || gMenu->inputAlways)
#define AT_FACE_Y(a)    ((gMenu->rotY == (a) && gMenu->zoom == 0) || gMenu->inputAlways)

/* 0x81311868: input on the front face and on the four selected faces. */
static s32 Menu_HandleInput(void) {
    if (gMenuPad.dirsNew & DIR_UP) {
        switch (gMenuState) {
        case MENU_TOP:
            if (AT_FRONT()) {
                Sound_Play(SE_FACE_SELECT);
                gMenuNextState = MENU_GAME_FACE;
            }
            break;
        case MENU_CARD_FACE:
            if (AT_FACE_X(-0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        }
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        switch (gMenuState) {
        case MENU_TOP:
            if (AT_FRONT()) {
                Sound_Play(SE_FACE_SELECT);
                gMenuNextState = MENU_CARD_FACE;
            }
            break;
        case MENU_GAME_FACE:
            if (AT_FACE_X(0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        }
    }
    if (gMenuPad.dirsNew & DIR_LEFT) {
        switch (gMenuState) {
        case MENU_TOP:
            if (AT_FRONT()) {
                Sound_Play(SE_FACE_SELECT);
                gMenuNextState = MENU_OPTION_FACE;
            }
            break;
        case MENU_CLOCK_FACE:
            if (AT_FACE_Y(-0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        }
    }
    if (gMenuPad.dirsNew & DIR_RIGHT) {
        switch (gMenuState) {
        case MENU_TOP:
            if (AT_FRONT()) {
                Sound_Play(SE_FACE_SELECT);
                gMenuNextState = MENU_CLOCK_FACE;
            }
            break;
        case MENU_OPTION_FACE:
            if (AT_FACE_Y(0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        }
    }
    if (gMenuPad.buttonDown & PAD_A) {
        switch (gMenuState) {
        case MENU_CARD_FACE:
            if (AT_FACE_X(-0x4000)) {
                Sound_Play(SE_ENTER);
                gMenuNextState = MENU_CARD;
            }
            break;
        case MENU_GAME_FACE:
            if (AT_FACE_X(0x4000)) {
                Sound_Play(SE_ENTER);
                gMenuNextState = MENU_GAME;
            }
            break;
        case MENU_CLOCK_FACE:
            if (AT_FACE_Y(-0x4000)) {
                Sound_Play(SE_ENTER);
                gMenuNextState = MENU_CLOCK;
            }
            break;
        case MENU_OPTION_FACE:
            if (AT_FACE_Y(0x4000)) {
                Sound_Play(SE_ENTER);
                gMenuNextState = MENU_OPTION;
            }
            break;
        }
    }
    if (gMenuPad.buttonDown & PAD_B) {
        switch (gMenuState) {
        case MENU_CARD_FACE:
            if (AT_FACE_X(-0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        case MENU_GAME_FACE:
            if (AT_FACE_X(0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        case MENU_CLOCK_FACE:
            if (AT_FACE_Y(-0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        case MENU_OPTION_FACE:
            if (AT_FACE_Y(0x4000)) {
                Sound_Play(SE_FACE_BACK);
                gMenuNextState = MENU_TOP;
            }
            break;
        }
    }
    return 0;
}

/* 0x81311D50: cube matrix = rotation * idle wobble * zoom, then the model. */
static void Menu_CalcCubeMtx(void) {
    Mtx m;
    Vec s;
    Mtx rot;
    Mtx wob;
    Scene *scene;
    s16 t;
    MenuWork *w;
    f32 tx, ty;
    s16 rz, ry;
    MtxPtr view;

    scene = Scene_Get();
    PSMTXIdentity(m);
    MtxRotTrans(gMenu->rotX, gMenu->rotY, 0, rot, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(rot, m, m);

    t = gMenu->wobbleFrame;
    PSMTXIdentity(wob);
    PSMTXConcat(wob, m, m);
    w = gMenu;
    ty = w->wobbleAmpTY * SinS(w->wobbleSpeedTY * t);
    tx = w->wobbleAmpTX * SinS(w->wobblePhaseTX + w->wobbleSpeedTX * t);
    rz = w->wobbleAmp[2] * CosS(w->wobbleSpeed[2] * t);
    ry = w->wobbleAmp[1] * CosS(w->wobblePhaseRY + w->wobbleSpeed[1] * t);
    MtxRotTrans(gMenu->wobbleAmp[0] * CosS(w->wobbleSpeed[0] * t), ry, rz, wob, tx, ty, 0.0f);
    PSMTXConcat(wob, m, m);

    PSMTXIdentity(sCubeMtx);
    PSMTXConcat(m, sCubeMtx, sCubeMtx);

    /* Zooming into a submenu pushes the cube back by zoomDepth*(1-cos(zoom)). */
    if (gMenu->zoom != 0) {
        MtxRotTrans(0, 0, gMenu->zoom * gMenu->zoomSpin, m, 0.0f, 0.0f,
                    gMenu->zoomDepth * (1.0f - CosS(gMenu->zoom)));
        PSMTXConcat(m, sCubeMtx, sCubeMtx);
    }

    s = sZeroVec;
    s.z = s.y = s.x = 1.1f * scene->modelScale;
    J3D_SetModelMtx(Scene_GetBigCube(), sCubeMtx, s);
    view = Camera_GetView();
    J3D_SetViewMtx(Scene_GetBigCube(), view);
}

/* 0x81312044: Game Play face shows the banner, "no disc" or an unknown disc. */
static void Menu_UpdateDiscFaders(void) {
    if (Disc_HasBanner()) {
        Fader_Step(&sDiscBanner, FADE_IN);
        Fader_Step(&sDiscNone, FADE_OUT);
        Fader_Step(&sDiscUnknown, FADE_OUT);
        return;
    }
    Fader_Step(&sDiscBanner, FADE_OUT);
    if (Disc_GetState() == DISC_NONE) {
        Fader_Step(&sDiscNone, FADE_IN);
        Fader_Step(&sDiscUnknown, FADE_OUT);
        return;
    }
    Fader_Step(&sDiscNone, FADE_OUT);
    Fader_Step(&sDiscUnknown, FADE_IN);
}

/* Move an angle one step towards 0, snapping it onto the step grid first. */
#define ROT_TOWARD_ZERO(a, step)                         \
    do {                                                 \
        if ((a) > 0) (a) = (step) * ((a) / (step)) - (step); \
        if ((a) < 0) (a) = (step) + (step) * ((a) / (step)); \
    } while (0)

/* 0x813120F0: turn the cube towards the face of gMenuNextState. */
static void Menu_UpdateRotation(void) {
    s16 step = gMenu->rotStep;

    switch (gMenuNextState) {
    case MENU_CARD_FACE:
    case MENU_CARD:
        gMenu->rotX -= step;
        ROT_TOWARD_ZERO(gMenu->rotY, step);
        if (gMenu->rotX < -0x4000) gMenu->rotX = -0x4000;
        break;
    case MENU_GAME_FACE:
    case MENU_GAME:
        gMenu->rotX += step;
        ROT_TOWARD_ZERO(gMenu->rotY, step);
        if (gMenu->rotX > 0x4000) gMenu->rotX = 0x4000;
        break;
    case MENU_CLOCK_FACE:
    case MENU_CLOCK:
        ROT_TOWARD_ZERO(gMenu->rotX, step);
        gMenu->rotY -= step;
        if (gMenu->rotY < -0x4000) gMenu->rotY = -0x4000;
        break;
    case MENU_OPTION_FACE:
    case MENU_OPTION:
        ROT_TOWARD_ZERO(gMenu->rotX, step);
        gMenu->rotY += step;
        if (gMenu->rotY > 0x4000) gMenu->rotY = 0x4000;
        break;
    case MENU_TOP:
        ROT_TOWARD_ZERO(gMenu->rotX, step);
        ROT_TOWARD_ZERO(gMenu->rotY, step);
        break;
    }
}

/* 0x813123B0: the icon cubes behind each submenu (menu_icon_*.c). */
static void Menu_UpdateIcons(void) {
    if (gMenuState == MENU_CARD_FACE) {
        CardIcon_UpdateFace();
    } else if (gMenuState == MENU_CARD) {
        CardIcon_UpdateEnter();
    } else {
        CardIcon_UpdateHide();
    }
    if (gMenuState == MENU_GAME_FACE) {
        GameIcon_UpdateFace();
    } else if (gMenuState == MENU_GAME) {
        GameIcon_UpdateEnter();
    } else {
        GameIcon_UpdateHide();
    }
    if (gMenuState == MENU_OPTION_FACE) {
        OptionIcon_UpdateFace();
    } else if (gMenuState == MENU_OPTION) {
        OptionIcon_UpdateEnter();
    } else {
        OptionIcon_UpdateHide();
    }
    if (gMenuState == MENU_CLOCK_FACE) {
        ClockIcon_UpdateFace();
    } else if (gMenuState == MENU_CLOCK) {
        ClockIcon_UpdateEnter();
    } else {
        ClockIcon_UpdateHide();
    }
    CardIcon_CalcMtx();
    GameIcon_CalcMtx();
    OptionIcon_CalcMtx();
    ClockIcon_CalcMtx();
}

/* 0x8131247C: memory card slot pictures on the card face glow while a card is ready. */
static void Menu_UpdateSlotGlow(void) {
    CardSlot *slot = CardThread_GetSlots();
    s32 i;
    s8 d;

    for (i = 0; i < 2; i++) {
        if (slot[i].state == CARD_SLOT_READY) {
            d = 4;
            gSlotFill[i] = 0xFF - (slot[i].freeBlocks * 0xFF) / slot[i].userBlocks;
        } else {
            d = -4;
        }
        gSlotGlow[i] += d;
        if (gSlotGlow[i] > 0xFF) gSlotGlow[i] = 0xFF;
        if (gSlotGlow[i] < 60) gSlotGlow[i] = 60;
    }
}

/* 0x81312580: which button guide labels/glyphs are visible in each state. */
static void Menu_UpdateGuide(void) {
    s32 i;

    switch (gMenuState) {
    case MENU_TOP:
        Fader_Step(&sGuideText[0], FADE_IN);     /* Menu Selection */
        Fader_Step(&sGuideText[1], FADE_OUT);
        Fader_Step(&sGuideText[2], FADE_OUT);
        Fader_Step(&sGuideText[3], FADE_OUT);
        Fader_Step(&sGuideText[4], FADE_OUT);
        Fader_Step(&sGuideText[5], FADE_OUT);
        Fader_Step(&sGuideText[7], FADE_OUT);
        Fader_Step(&sGuideText[6], FADE_OUT);
        break;
    case MENU_GAME_FACE:
    case MENU_CARD_FACE:
    case MENU_OPTION_FACE:
    case MENU_CLOCK_FACE:
        Fader_Step(&sGuideText[0], FADE_OUT);
        Fader_Step(&sGuideText[1], FADE_IN);     /* Cancel */
        Fader_Step(&sGuideText[2], FADE_IN);     /* Confirm */
        Fader_Step(&sGuideText[3], FADE_OUT);
        Fader_Step(&sGuideText[4], FADE_OUT);
        Fader_Step(&sGuideText[5], FADE_OUT);
        Fader_Step(&sGuideText[6], FADE_OUT);
        Fader_Step(&sGuideText[7], FADE_OUT);
        break;
    case MENU_GAME:
        Fader_Step(&sGuideText[0], FADE_OUT);
        Fader_Step(&sGuideText[1], FADE_OUT);
        Fader_Step(&sGuideText[2], FADE_OUT);
        Fader_Step(&sGuideText[3], FADE_OUT);
        Fader_Step(&sGuideText[4], FADE_OUT);
        if (!Game_IsFatal()) {
            Fader_Step(&sGuideText[5], FADE_IN); /* Cancel */
        } else {
            Fader_Step(&sGuideText[5], FADE_OUT);
        }
        Fader_Step(&sGuideText[6], FADE_OUT);
        Fader_Step(&sGuideText[7], FADE_OUT);
        break;
    case MENU_OPTION:
        if (Option_IsTop()) {
            Fader_Step(&sGuideText[3], FADE_IN); /* Select */
            Fader_Step(&sGuideText[4], FADE_OUT);
            Fader_Step(&sGuideText[6], FADE_IN); /* Finish */
            Fader_Step(&sGuideText[5], FADE_OUT);
        } else {
            Fader_Step(&sGuideText[3], FADE_OUT);
            Fader_Step(&sGuideText[4], FADE_IN); /* Change */
            Fader_Step(&sGuideText[6], FADE_OUT);
            Fader_Step(&sGuideText[5], FADE_IN); /* Cancel */
        }
        Fader_Step(&sGuideText[0], FADE_OUT);
        Fader_Step(&sGuideText[1], FADE_OUT);
        Fader_Step(&sGuideText[2], FADE_OUT);
        Fader_Step(&sGuideText[7], FADE_IN);     /* Confirm */
        break;
    case MENU_CLOCK:
        if (Clock_IsTop()) {
            Fader_Step(&sGuideText[3], FADE_IN);
            Fader_Step(&sGuideText[4], FADE_OUT);
            Fader_Step(&sGuideText[6], FADE_IN);
            Fader_Step(&sGuideText[5], FADE_OUT);
        } else {
            Fader_Step(&sGuideText[3], FADE_OUT);
            Fader_Step(&sGuideText[4], FADE_IN);
            Fader_Step(&sGuideText[6], FADE_OUT);
            Fader_Step(&sGuideText[5], FADE_IN);
        }
        Fader_Step(&sGuideText[7], FADE_IN);
        Fader_Step(&sGuideText[0], FADE_OUT);
        Fader_Step(&sGuideText[1], FADE_OUT);
        Fader_Step(&sGuideText[2], FADE_OUT);
        break;
    case MENU_CARD:
        if (Card_IsBrowsing()) {
            Fader_Step(&sGuideText[5], FADE_OUT);
            Fader_Step(&sGuideText[6], FADE_IN); /* Finish */
        } else {
            Fader_Step(&sGuideText[5], FADE_IN); /* Cancel */
            Fader_Step(&sGuideText[6], FADE_OUT);
        }
        Fader_Step(&sGuideText[7], FADE_IN);
        Fader_Step(&sGuideText[0], FADE_OUT);
        Fader_Step(&sGuideText[1], FADE_OUT);
        Fader_Step(&sGuideText[2], FADE_OUT);
        Fader_Step(&sGuideText[3], FADE_IN);
        break;
    }

    switch (gMenuState) {
    case MENU_TOP:
        Fader_Step(&sGuideIcon[0], FADE_OUT);
        Fader_Step(&sGuideIcon[1], FADE_OUT);
        Fader_Step(&sGuideIcon[2], FADE_OUT);
        Fader_Step(&sGuideIcon[3], FADE_IN);    /* stick */
        Fader_Step(&sGuideIcon[4], FADE_OUT);
        Fader_Step(&sGuideIcon[5], FADE_OUT);
        break;
    case MENU_GAME:
        Fader_Step(&sGuideIcon[0], FADE_OUT);
        if (!Game_IsFatal()) {
            Fader_Step(&sGuideIcon[1], FADE_IN);
        } else {
            Fader_Step(&sGuideIcon[1], FADE_OUT);
        }
        Fader_Step(&sGuideIcon[2], FADE_OUT);
        Fader_Step(&sGuideIcon[3], FADE_OUT);
        Fader_Step(&sGuideIcon[4], FADE_OUT);
        Fader_Step(&sGuideIcon[5], FADE_OUT);
        break;
    case MENU_GAME_FACE:
    case MENU_CARD_FACE:
    case MENU_OPTION_FACE:
    case MENU_CLOCK_FACE:
        Fader_Step(&sGuideIcon[0], FADE_OUT);
        Fader_Step(&sGuideIcon[1], FADE_OUT);
        Fader_Step(&sGuideIcon[2], FADE_OUT);
        Fader_Step(&sGuideIcon[3], FADE_OUT);
        Fader_Step(&sGuideIcon[4], FADE_IN);    /* B */
        Fader_Step(&sGuideIcon[5], FADE_IN);    /* A */
        break;
    default:                            /* MENU_CARD, MENU_OPTION, MENU_CLOCK */
        Fader_Step(&sGuideIcon[0], FADE_IN);
        Fader_Step(&sGuideIcon[1], FADE_IN);
        Fader_Step(&sGuideIcon[2], FADE_IN);
        Fader_Step(&sGuideIcon[3], FADE_OUT);
        Fader_Step(&sGuideIcon[4], FADE_OUT);
        Fader_Step(&sGuideIcon[5], FADE_OUT);
        break;
    }

    for (i = 0; i < 6; i++) {
        u8 mode = (i == gGuideLang) ? FADE_IN : FADE_OUT;
        Fader_Step(&sLangGuide[i], mode);
    }
}

/* 0x81312A94 */
static void Menu_UpdateSubFaders(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        if (gMenuState == MENU_CARD) {
            Fader_Step(&sCardFade[i], FADE_IN);
        } else {
            Fader_Step(&sCardFade[i], FADE_OUT);
        }
        if (gMenuState == MENU_GAME) {
            Fader_Step(&sGameFade[i], FADE_IN);
        } else {
            Fader_Step(&sGameFade[i], FADE_OUT);
        }
        if (gMenuState == MENU_OPTION) {
            Fader_Step(&sOptionFade[i], FADE_IN);
        } else {
            Fader_Step(&sOptionFade[i], FADE_OUT);
        }
        if (gMenuState == MENU_CLOCK) {
            Fader_Step(&sClockFade[i], FADE_IN);
        } else {
            Fader_Step(&sClockFade[i], FADE_OUT);
        }
    }
}

/* 0x81312B9C: Scene state 5. Returns the next scene state (always 5). */
static inline void Menu_UpdateCube(void) {
    Scene *scene = Scene_Get();
    s16 step = gMenu->zoomOutStep;

    if (gMenu->zoom > step) gMenu->zoom -= step;
    if (gMenu->zoom <= step) gMenu->zoom = 0;
    if (!SCENE_FATAL_ERROR(scene)) {
        Menu_HandleInput();
    }
}

static inline void Menu_ZoomIn(void) {
    s16 step = gMenu->zoomInStep;
    if (gMenu->zoom < 0x7FFF - step) gMenu->zoom += step;
    if (gMenu->zoom >= 0x7FFF - step) gMenu->zoom = 0x7FFF;
}

static inline void Menu_AnimCube(void) {
    Scene *scene = Scene_Get();
    gMenu->wobbleFrame = gMenu->wobbleFrame + 7;
    Menu_CalcCubeMtx();
    J3D_CalcJoints(Scene_GetBigCube());
}

s32 Menu_Update(void) {
    Scene *scene = Scene_Get();
    DispSettings *disp = Disp_GetSettings();
    OSSram *sram;
    s16 step;

    /* The SRAM was found corrupt at boot: flag it once the menu is up. */
    if (gSramLostPending) {
        if (Boot_SramWasLost()) {
            sram = __OSLockSram();
            sram->flags |= 0x08;
            __OSUnlockSram(TRUE);
            while (!__OSSyncSram()) {
            }
        }
        gSramLostPending = FALSE;
    }
    if (Disc_GetState() == DISC_FATAL) {
        SCENE_FATAL_ERROR(scene) = TRUE;
    }
    disp->menuFlag = 0;

    Menu_UpdateRotation();
    Menu_UpdateIcons();
    Menu_UpdateSlotGlow();
    Menu_UpdateGuide();
    Menu_UpdateSubFaders();

    /* One-shot actions on state changes */
    if (gMenuState == MENU_CARD_FACE && gMenuNextState == MENU_CARD) {
        Card_Enter();
        CardUi_Reset();
        CardIcon_ResetFiles();
    }
    if (gMenuState == MENU_GAME_FACE && gMenuNextState == MENU_GAME) {
        Game_Enter();
    }
    if (gMenuState == MENU_GAME && gMenuNextState == MENU_GAME_FACE) {
        Fader_Reset(&sDiscBanner);
        Fader_Reset(&sDiscNone);
        Fader_Reset(&sDiscUnknown);
    }

    gMenuState = gMenuNextState;
    gMenu->state = gMenuState;

    switch (gMenuState) {
    case MENU_TOP:
    case MENU_GAME_FACE:
    case MENU_CARD_FACE:
    case MENU_OPTION_FACE:
    case MENU_CLOCK_FACE:
        Menu_UpdateCube();
        Menu_AnimCube();
        Menu_UpdateDiscFaders();
        Fx_Clear();
        break;

    /* Inside a submenu the cube zooms away and the submenu decides the next state. */
    case MENU_GAME:
        Menu_ZoomIn();
        Menu_AnimCube();
        gMenuNextState = Game_Update();
        break;
    case MENU_CARD:
        Menu_ZoomIn();
        Menu_AnimCube();
        gMenuNextState = Card_Update();
        break;
    case MENU_CLOCK:
        Menu_ZoomIn();
        Menu_AnimCube();
        gMenuNextState = Clock_Update();
        break;
    case MENU_OPTION:
        Menu_ZoomIn();
        Menu_AnimCube();
        gMenuNextState = Option_Update();
        break;
    }

    if (SCENE_FATAL_ERROR(scene)) {
        Fader_Step(&sErrorMsg, FADE_IN);
    }
    return 5;
}

/* 0x81312F68 */
MtxPtr Menu_GetCubeMtx(void) {
    return sCubeMtx;
}

/* 0x81312F74 */
MenuWork *Menu_Get(void) {
    return gMenu;
}

/* 0x81312F7C: called by Options while scrolling through the languages. */
void Menu_SetGuideLang(s32 lang) {
    gGuideLang = lang;
}

/* ------------------------------------------------------------------ */
/* Drawing                                                              */
/* ------------------------------------------------------------------ */
// Faces sit on a cube of half-size 144 (the layouts are 288 x 288 with
// y pointing down, hence the scale (1,-1,1) in every SRT). A side face is
// drawn once the cube is turned at least 0x2E3A (65 degrees) toward it and
// fades in over the last 5000 units (27.5 degrees) of the turn:
//     alpha = 255 - (0x4000 - |rot|) * 255 / 5000
// The front face fades out the same way as soon as the cube turns.
//
// "z" panes (zitl, zodi, ...) are drop shadows drawn in 0x282828 before
// the matching white pane.

#define FACE_SHOW_ROT   0x2E3A
#define FACE_FADE_RANGE 5000

#define FACE_HALF       144.0f

enum { FACE_TOP = 0, FACE_WORK = 1, FACE_CARD = 2, FACE_GAME = 3, FACE_CLOCK = 4, FACE_OPTION = 5 };

/* view * base * SRT -> GX_PNMTX0 (written out at every call site) */
#define FACE_LOAD_MTX(base, srt, tmp, m)                \
    do {                                                \
        PSMTXConcat(Camera_GetView(), (base), (tmp));   \
        MtxFromSRT((srt), (m));                         \
        PSMTXConcat((tmp), (m), (m));                   \
        GXLoadPosMtxImm((m), GX_PNMTX0);                \
        GXSetCurrentMtx(GX_PNMTX0);                     \
    } while (0)

/* 0x81312F88: the front face on the spinning cube during the spin-out
   (Scene state 4). The alpha is applied twice (alpha^2 / 255). */
static inline void Menu_DrawFrontFace(MtxPtr cube, SRT srt, u8 alpha) {
    GXColor c = { 255, 255, 255, 255 };
    Mtx m;
    Mtx tmp;

    PSMTXConcat(Camera_GetView(), cube, tmp);
    c.a = alpha;
    MtxFromSRT(&srt, m);
    PSMTXConcat(tmp, m, m);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    c.a = (u8)(c.a * alpha / 255);
    GX_Setup2DTex();
    Glh_DrawAll(gTopFaceGlh, &c);
}

void Menu_DrawSpinOutFace(MtxPtr cube, u8 alpha) {
    SRT srt = sFaceSRT[FACE_TOP];
    SRT *d = &srt;

    Menu_DrawFrontFace(cube, *d, alpha);
}

/* 0x813130CC: disc banner on the Game face: image, short name, maker. */
static void Menu_DrawBannerInfo(void *glh, u8 alpha) {
    TexHeader tex;
    s16 a;
    GXColor white = { 255, 255, 255, 255 };
    GXColor shadow = { 40, 40, 40, 255 };
    u8 *bnr;

    Fader_GetAlpha(&sDiscBanner, &a, NULL);
    if (a == 0) {
        return;
    }

    /* 96 x 32 RGB5A3 image at the start of the banner */
    bnr = Disc_GetBanner();
    shadow.a = white.a = (u8)(alpha * a / 255);
    tex.mipmap = 0;
    tex.edgeLod = 0;
    tex.biasClamp = 0;
    tex.maxAniso = 0;
    tex.width = 96;
    tex.height = 32;
    tex.wrapS = GX_CLAMP;
    tex.wrapT = GX_CLAMP;
    tex.format = GX_TF_RGB5A3;
    tex.minFilter = GX_LINEAR;
    tex.magFilter = GX_LINEAR;
    tex.minLod = 0;
    tex.maxLod = 0;
    tex.lodBias = 0;
    tex.indexed = 0;
    tex.imageOffset = (bnr + 0x20) - (u8 *)&tex;

    GX_Setup2D(1, 0, 1);
    Glh_DrawPaneTex('bana', glh, &white, &tex);
    GX_Setup2DText();
    if (Disc_GetBannerType() == 0) {
        Glh_DrawBannerLine('zame', glh, &shadow, bnr + 0x1820, 0x20);  /* short name */
        Glh_DrawBannerLine('game', glh, &white, bnr + 0x1820, 0x20);
        Glh_DrawBannerLine('zakr', glh, &shadow, bnr + 0x1840, 0x20);  /* short maker */
        Glh_DrawBannerLine('makr', glh, &white, bnr + 0x1840, 0x20);
    } else if (Disc_GetBannerType() == 1) {
        /* BNR2: one 0x140 byte text block per language */
        Glh_DrawBannerLine('zame', glh, &shadow, bnr + Lang_Get() * 0x140 + 0x1820, 0x20);
        Glh_DrawBannerLine('game', glh, &white, bnr + Lang_Get() * 0x140 + 0x1820, 0x20);
        Glh_DrawBannerLine('zakr', glh, &shadow, bnr + Lang_Get() * 0x140 + 0x1840, 0x20);
        Glh_DrawBannerLine('makr', glh, &white, bnr + Lang_Get() * 0x140 + 0x1840, 0x20);
    }
}

/* 0x81313334: Game Play face ("nodi" no disc / "qust" unknown disc). */
static void Menu_DrawGameFace(void *glh, const SRT *srt, u8 alpha) {
    Mtx tmp;
    Mtx m;
    GXColor white = { 255, 255, 255, 255 };
    GXColor w2;
    GXColor s2;
    GXColor shadow = { 40, 40, 40, 255 };
    s16 a;

    PSMTXConcat(Camera_GetView(), sCubeMtx, tmp);
    white.a = alpha;
    MtxFromSRT(srt, m);
    PSMTXConcat(tmp, m, m);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    white.a = (u8)(white.a * alpha / 255);
    shadow.a = (u8)(shadow.a * alpha / 255);
    Glh_DrawFrame('fram', glh, &white);
    GX_Setup2DTex();
    Glh_DrawPane('titl', glh, &white);

    Fader_GetAlpha(&sDiscNone, &a, NULL);
    if (a > 0) {
        w2 = white; s2 = shadow;
        w2.a = (u8)(w2.a * a / 255);
        s2.a = (u8)(s2.a * a / 255);
        Glh_DrawPane('zodi', glh, &s2);
        Glh_DrawPane('nodi', glh, &w2);
    }
    Fader_GetAlpha(&sDiscUnknown, &a, NULL);
    if (a > 0) {
        w2 = white; s2 = shadow;
        w2.a = (u8)(w2.a * a / 255);
        s2.a = (u8)(s2.a * a / 255);
        Glh_DrawPane('zust', glh, &s2);
        Glh_DrawPane('qust', glh, &w2);
    }
    Menu_DrawBannerInfo(glh, alpha);
}

/* Draw the frames tagged `tag` as bars filled `fill`/255 from the bottom. */
static inline void Menu_DrawFillBar(u32 tag, void *glh, GXColor *c, u8 fill) {
    GlhFrame *f = GLH_FRAMES(glh);
    u16 i;

    if (fill == 0) {
        return;
    }
    for (i = 0; i < ((Glh *)glh)->nFrames; f++, i++) {
        if (tag == f->tag) {
            Glh_DrawPaneRect(i, glh, c, f->x - f->innerW / 2,
                             f->y - f->innerH / 2 + f->innerH - fill * f->innerH / 255,
                             f->innerW, fill * f->innerH / 255);
        }
    }
}

/* 0x8131358C: one memory card slot on the Card face: label, glowing
   frame and a fill bar that rises with the card's used space. */
static void Menu_DrawSlot(u8 slot, void *glh, u8 alpha) {
    GXColor c = { 255, 255, 255, 255 };
    u32 tag;

    c.a = alpha;
    GX_Setup2DTex();
    Glh_DrawPane('texa' + slot, glh, &c);

    c.a = (u8)(alpha * gSlotGlow[slot] / 255);
    tag = slot << 8;                                /* 'mca?' / 'mcb?' */
    Glh_DrawFrame('mca1' + tag, gCardFaceGlh, &c);
    Menu_DrawFillBar('mca2' + tag, gCardFaceGlh, &c,
                     (u8)(gSlotFill[slot] * (gSlotGlow[slot] - 60) / 195));
}

/* bottom face: Memory Card frame, title and the two slot bars */
static inline void Menu_DrawCardFace(void *glh, SRT *srt, u8 alpha) {
    Mtx tmp;
    Mtx m;
    GXColor c = { 255, 255, 255, 255 };

    PSMTXConcat(Camera_GetView(), sCubeMtx, tmp);
    c.a = alpha;
    MtxFromSRT(srt, m);
    PSMTXConcat(tmp, m, m);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    c.a = (u8)(c.a * alpha / 255);
    Glh_DrawFrame('fram', glh, &c);
    GX_Setup2DTex();
    Glh_DrawPane('titl', glh, &c);
    Menu_DrawSlot(0, glh, alpha);
    Menu_DrawSlot(1, glh, alpha);
}

static inline void *Digit(s32 n) {
    return sDigitTex[n];
}

/* 0x81313708: Calendar face, today's date and the current time. */
static void Menu_DrawClockFace(void *glh, const SRT *srt, u8 alpha) {
    Mtx tmp;
    Mtx m;
    GXColor white = { 255, 255, 255, 255 };
    GXColor shadow = { 40, 40, 40, 255 };
    OSCalendarTime t;

    PSMTXConcat(Camera_GetView(), sCubeMtx, tmp);
    memset(&t, 0, sizeof(t));
    OSTicksToCalendarTime(OSGetTime(), &t);
    white.a = alpha;
    MtxFromSRT(srt, m);
    PSMTXConcat(tmp, m, m);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    white.a = (u8)(white.a * alpha / 255);
    shadow.a = (u8)(shadow.a * alpha / 255);
    Glh_DrawFrame('fram', glh, &white);
    GX_Setup2DTex();

    /* drop shadows ('z' + 3 letters) first, then the white panes; the
       first year digit is always "2" */
    Glh_DrawPane('zolo', glh, &shadow);
    Glh_DrawPane('zlas', glh, &shadow);
    Glh_DrawPane('zare', glh, &shadow);
    Glh_DrawPane('zitl', glh, &shadow);
    Glh_DrawPaneTex('zal1', glh, &shadow, sDigitTex[2]);
    Glh_DrawPaneTex('zal2', glh, &shadow, sDigitTex[t.year / 100 % 10]);
    Glh_DrawPaneTex('zal3', glh, &shadow, sDigitTex[t.year / 10 % 10]);
    Glh_DrawPaneTex('zal4', glh, &shadow, sDigitTex[t.year % 10]);
    Glh_DrawPaneTex('zal5', glh, &shadow, sDigitTex[(t.mon + 1) / 10 % 10]);
    Glh_DrawPaneTex('zal6', glh, &shadow, sDigitTex[(t.mon + 1) % 10]);
    Glh_DrawPaneTex('zal7', glh, &shadow, sDigitTex[t.mday / 10]);
    Glh_DrawPaneTex('zal8', glh, &shadow, sDigitTex[t.mday % 10]);
    Glh_DrawPaneTex('zeek', glh, &shadow, sWeekdayTex[t.wday]);
    Glh_DrawPaneTex('zim1', glh, &shadow, sDigitTex[t.hour / 10]);
    Glh_DrawPaneTex('zim2', glh, &shadow, sDigitTex[t.hour % 10]);
    Glh_DrawPaneTex('zim3', glh, &shadow, sDigitTex[t.min / 10]);
    Glh_DrawPaneTex('zim4', glh, &shadow, sDigitTex[t.min % 10]);
    Glh_DrawPaneTex('zim5', glh, &shadow, sDigitTex[t.sec / 10]);
    Glh_DrawPaneTex('zim6', glh, &shadow, sDigitTex[t.sec % 10]);
    Glh_DrawPane('colo', glh, &white);
    Glh_DrawPane('slas', glh, &white);
    Glh_DrawPane('pare', glh, &white);
    Glh_DrawPane('titl', glh, &white);
    Glh_DrawPaneTex('cal1', glh, &white, sDigitTex[2]);
    Glh_DrawPaneTex('cal2', glh, &white, sDigitTex[t.year / 100 % 10]);
    Glh_DrawPaneTex('cal3', glh, &white, sDigitTex[t.year / 10 % 10]);
    Glh_DrawPaneTex('cal4', glh, &white, sDigitTex[t.year % 10]);
    Glh_DrawPaneTex('cal5', glh, &white, sDigitTex[(t.mon + 1) / 10 % 10]);
    Glh_DrawPaneTex('cal6', glh, &white, sDigitTex[(t.mon + 1) % 10]);
    Glh_DrawPaneTex('cal7', glh, &white, sDigitTex[t.mday / 10]);
    Glh_DrawPaneTex('cal8', glh, &white, sDigitTex[t.mday % 10]);
    Glh_DrawPaneTex('week', glh, &white, sWeekdayTex[t.wday]);
    Glh_DrawPaneTex('tim1', glh, &white, sDigitTex[t.hour / 10]);
    Glh_DrawPaneTex('tim2', glh, &white, sDigitTex[t.hour % 10]);
    Glh_DrawPaneTex('tim3', glh, &white, sDigitTex[t.min / 10]);
    Glh_DrawPaneTex('tim4', glh, &white, sDigitTex[t.min % 10]);
    Glh_DrawPaneTex('tim5', glh, &white, sDigitTex[t.sec / 10]);
    Glh_DrawPaneTex('tim6', glh, &white, sDigitTex[t.sec % 10]);
    GX_Setup2DText();
}

/* 0x81313F20: Options face: sound mode, screen offset with an arrow on
   the side it is shifted to, and the language. */
static void Menu_DrawOptionFace(void *glh, const SRT *srt, u8 alpha) {
    Mtx tmp;
    Mtx m;
    GXColor white = { 255, 255, 255, 255 };
    GXColor shadow = { 40, 40, 40, 255 };
    u32 stereo;
    s16 ofs;
    OSSram *sram;

    stereo = OSGetSoundMode();
    sram = __OSLockSram();
    ofs = (s8)sram->displayOffsetH;
    __OSUnlockSram(FALSE);

    FACE_LOAD_MTX(sCubeMtx, srt, tmp, m);
    white.a = (u8)(white.a * alpha / 255);
    shadow.a = (u8)(shadow.a * alpha / 255);
    Glh_DrawFrame('fram', glh, &white);
    GX_Setup2DTex();
    Glh_DrawPane('zxt1', glh, &shadow);
    Glh_DrawPane('zxt2', glh, &shadow);
    Glh_DrawPane('zitl', glh, &shadow);
    Glh_DrawPane('txt1', glh, &white);
    Glh_DrawPane('txt2', glh, &white);
    Glh_DrawPane('titl', glh, &white);
    if (ofs > 0) {
        Glh_DrawPane('zrwr', glh, &shadow);
        Glh_DrawPane('arwr', glh, &white);
    } else if (ofs < 0) {
        Glh_DrawPane('zrwl', glh, &shadow);
        Glh_DrawPane('arwl', glh, &white);
    }
    if (ofs < -32) {
        ofs = -32;
    }
    if (ofs > 32) {
        ofs = 32;
    }
    Glh_DrawPaneTex('zum1', glh, &shadow, sDigitTex[(ofs >= 0 ? ofs : -ofs) / 10]);
    Glh_DrawPaneTex('zum2', glh, &shadow, sDigitTex[(ofs >= 0 ? ofs : -ofs) % 10]);
    Glh_DrawPaneTex('num1', glh, &white, sDigitTex[(ofs >= 0 ? ofs : -ofs) / 10]);
    Glh_DrawPaneTex('num2', glh, &white, sDigitTex[(ofs >= 0 ? ofs : -ofs) % 10]);

    if (stereo != 0) {
        stereo = 1;
    }
    Glh_DrawPaneTex('zond', glh, &shadow, gSoundModeTex[stereo]);
    Glh_DrawPaneTex('sond', glh, &white, gSoundModeTex[stereo]);
    Glh_DrawPane('zxt3', glh, &shadow);
    Glh_DrawPane('zang', glh, &shadow);
    Glh_DrawPane('txt3', glh, &white);
    Glh_DrawPane('lang', glh, &white);       /* language name (per-language layout) */
    GX_Setup2DText();
}

static inline s16 AbsClamp(s16 rot) {
    if ((rot >= 0 && rot < FACE_FADE_RANGE) || (rot <= 0 && rot > -FACE_FADE_RANGE)) {
        return (s16)(rot >= 0 ? rot : -rot);
    }
    return FACE_FADE_RANGE;
}


static inline s32 Face_Fade(s16 rot) {
    if (rot < 0) rot = -rot;
    return 255 - (0x4000 - rot) * 255 / FACE_FADE_RANGE;
}

/* 0x81314304: all faces of the cube, alpha = overall menu alpha. */
void Menu_DrawFaces(u8 alpha) {
    SRT srt;
    f32 half = FACE_HALF;
    s16 ax, ay;
    s16 *rot;

    {
        SRT t = sFaceSRT[FACE_WORK];
        t.tz = half;
        srt = t;
    }
    ax = AbsClamp(gMenu->rotX);
    ay = AbsClamp(gMenu->rotY);

    /* front face: fades with the larger of the two turns */
    if (ax > ay) {
        MtxPtr cube = sCubeMtx;
        SRT s = sFaceSRT[FACE_TOP];
        SRT *d = &s;
        Menu_DrawFrontFace(cube, *d, 255 - ax * 255 / FACE_FADE_RANGE);
    } else if (ax < ay) {
        MtxPtr cube = sCubeMtx;
        SRT s = sFaceSRT[FACE_TOP];
        SRT *d = &s;
        Menu_DrawFrontFace(cube, *d, 255 - ay * 255 / FACE_FADE_RANGE);
    } else if (ax == 0) {
        MtxPtr cube = sCubeMtx;
        SRT s = sFaceSRT[FACE_TOP];
        SRT *d = &s;
        Menu_DrawFrontFace(cube, *d, 255 - ax * 255 / FACE_FADE_RANGE);
    }

    /* bottom: Memory Card */
    rot = &gMenu->rotX;
    if (*rot <= -FACE_SHOW_ROT) {
        SRT t = sFaceSRT[FACE_CARD];
        t.ty = -half;
        srt = t;
        Menu_DrawCardFace(gCardFaceGlh, &srt, alpha * Face_Fade(*rot) / 255);
    }
    /* top: Game Play */
    rot = &gMenu->rotX;
    if (*rot >= FACE_SHOW_ROT) {
        SRT t = sFaceSRT[FACE_GAME];
        t.ty = half;
        srt = t;
        Menu_DrawGameFace(gGameFaceGlh, &srt, alpha * Face_Fade(*rot) / 255);
    }
    /* right: Calendar */
    rot = &gMenu->rotY;
    if (*rot <= -FACE_SHOW_ROT) {
        SRT t = sFaceSRT[FACE_CLOCK];
        t.tx = half;
        srt = t;
        Menu_DrawClockFace(gClockFaceGlh, &srt, alpha * Face_Fade(*rot) / 255);
    }
    /* left: Options */
    rot = &gMenu->rotY;
    if (*rot >= FACE_SHOW_ROT) {
        SRT t = sFaceSRT[FACE_OPTION];
        t.tx = -half;
        srt = t;
        Menu_DrawOptionFace(gOptionFaceGlh, &srt, alpha * Face_Fade(*rot) / 255);
    }
}

/* 0x81314CC8: the card submenu has (almost) faded in. */
BOOL Menu_CardFadeDone(void) {
    s16 a[3];
    s32 i;

    for (i = 0; i < 3; i++) {
        Fader_GetAlpha(&sCardFade[i], &a[i], NULL);
    }
    if (a[0] > 172 && a[1] > 172 && a[2] > 172) return TRUE;
    return FALSE;
}

/* 0x81314D70: each submenu is drawn while its first fader is non zero;
   the three fader alphas are passed as (fx/icons, backdrop, text). */
void Menu_DrawSubmenus(void) {
    s16 a[3];
    s32 i;

    for (i = 0; i < 3; i++) Fader_GetAlpha(&sCardFade[i], &a[i], NULL);
    if (a[0] > 0) Card_Draw((u8)a[0], (u8)a[1], (u8)a[2]);

    for (i = 0; i < 3; i++) Fader_GetAlpha(&sGameFade[i], &a[i], NULL);
    if (a[0] > 0) Game_Draw((u8)a[0], (u8)a[1], (u8)a[2]);

    for (i = 0; i < 3; i++) Fader_GetAlpha(&sOptionFade[i], &a[i], NULL);
    if (a[0] > 0) Option_Draw((u8)a[0], (u8)a[1], (u8)a[2]);

    for (i = 0; i < 3; i++) Fader_GetAlpha(&sClockFade[i], &a[i], NULL);
    if (a[0] > 0) Clock_Draw((u8)a[0], (u8)a[1], (u8)a[2]);
}

/* 0x81314F44: guide labels, drawn once per language so that a language
   change cross-fades (langGuide[] faders). */
static void Menu_DrawGuideText(u8 alpha) {
    s32 lang;
    s16 a;

    for (lang = 0; lang < 6; lang++) {
        GXColor c = { 255, 255, 255, 255 };

        Fader_GetAlpha(&sLangGuide[lang], &a, NULL);
        c.a = (u8)(alpha * a / 255);
        Fader_DrawString(&sGuideText[0], sGuideSth[lang], gGuideGlh, &c);
        Fader_DrawString(&sGuideText[1], sGuideSth[lang], gGuideGlh, &c);
        Fader_DrawString(&sGuideText[2], sGuideSth[lang], gGuideGlh, &c);
        Fader_DrawString(&sGuideText[3], sGuideSth[lang], gGuideGlh, &c);
        Fader_DrawString(&sGuideText[4], sGuideSth[lang], gGuideGlh, &c);
        Fader_DrawString(&sGuideText[5], sGuideSth[lang], gGuideGlh, &c);
        Fader_DrawString(&sGuideText[6], sGuideSth[lang], gGuideGlh, &c);
        Fader_DrawString(&sGuideText[7], sGuideSth[lang], gGuideGlh, &c);
    }
}

/* 0x8131508C: button guide at the bottom of the screen (res 115).
   Two groups of three glyphs (stick, B, A), one per guide line.
   guideIcon[0..2] = stick/B/A of line 1, [3..5] of line 2. */
void Menu_DrawGuide(u8 alpha) {
    GXColor white = { 255, 255, 255, 255 };
    GXColor stick = { 255, 255, 255, 255 };
    GXColor dots = { 255, 255, 255, 255 };
    GXColor butB = { 255, 255, 255, 255 };
    GXColor butA = { 255, 255, 255, 255 };
    GXColor back = { 150, 150, 150, 255 };
    GXColor colB;
    GXColor colA;
    s16 ga[2];
    s16 gb[2];
    s16 gc[2];
    s16 pulse, base, f;
    s16 k;
    u8 fa;
    u32 i;
    u32 tag;

    colA.r = (u8)gMenu->guideColorA[0];
    colA.g = (u8)gMenu->guideColorA[1];
    colA.b = (u8)gMenu->guideColorA[2];
    colA.a = 255;
    colB.r = (u8)gMenu->guideColorB[0];
    colB.g = (u8)gMenu->guideColorB[1];
    colB.b = (u8)gMenu->guideColorB[2];
    colB.a = 255;

    GX_Setup2DView();
    GX_Setup2DTex();
    Fader_GetAlpha(&sGuideIcon[0], &ga[0], NULL);
    Fader_GetAlpha(&sGuideIcon[1], &gb[0], NULL);
    Fader_GetAlpha(&sGuideIcon[2], &gc[0], NULL);
    Fader_GetAlpha(&sGuideIcon[3], &ga[1], NULL);
    Fader_GetAlpha(&sGuideIcon[4], &gb[1], NULL);
    Fader_GetAlpha(&sGuideIcon[5], &gc[1], NULL);

    /* glyph backgrounds pulse 255 -> 100 -> 255 every 256 frames */
    if (gGuideBlink % 256 > 127) {
        pulse = (gGuideBlink % 128) * 155 / 127 + 100;
    } else {
        pulse = 255 - (gGuideBlink % 128) * 155 / 127;
    }
    base = (s16)(pulse * alpha / 255);
    for (i = 0; i < 2; i++) {
        back.a = (u8)(base * ga[i] / 255);
        colB.a = (u8)(base * gb[i] / 255);
        colA.a = (u8)(base * gc[i] / 255);
        tag = 'bac1' + i * 3;
        Glh_DrawPane(tag, gGuideGlh, &back);
        Glh_DrawPane(tag + 1, gGuideGlh, &colB);
        Glh_DrawPane(tag + 2, gGuideGlh, &colA);
    }

    /* stick: every 128 frames cross-fade between the neutral sprite
       (stk0 / stk5) and one of the four tilted ones, cycling directions */
    for (i = 0; i < 2; i++) {
        k = (gGuideBlink / 128) % 8;
        if (k % 2 == 0) {
            f = (u8)((gGuideBlink % 128) * 255 / 127);
        } else {
            f = (u8)(255 - (gGuideBlink % 128) * 255 / 127);
        }
        fa = (u8)(f * alpha / 255);
        stick.a = 255 - fa;
        stick.a = (u8)(stick.a * ga[i] / 255);
        tag = 'stk0' + i * 5;
        Glh_DrawPane(tag + ((k + 1) / 2 + 3) % 4 + 1, gGuideGlh, &stick);
        stick.a = fa;
        stick.a = (u8)(stick.a * ga[i] / 255);
        Glh_DrawPane(tag, gGuideGlh, &stick);
    }

    /* button glyphs and the dots after them, faded in by gGuideAlpha */
    white.a = (u8)(white.a * alpha / 255);
    white.a = (u8)(white.a * gGuideAlpha / 255);
    fa = white.a;
    butB.a = (u8)(fa * gb[0] / 255);
    Glh_DrawPane('butb', gGuideGlh, &butB);
    Glh_DrawPane('dot2', gGuideGlh, &butB);
    butB.a = (u8)(fa * gb[1] / 255);
    Glh_DrawPane('butc', gGuideGlh, &butB);
    Glh_DrawPane('dot5', gGuideGlh, &butB);
    butA.a = (u8)(fa * gc[0] / 255);
    Glh_DrawPane('buta', gGuideGlh, &butA);
    Glh_DrawPane('dot3', gGuideGlh, &butA);
    butA.a = (u8)(fa * gc[1] / 255);
    Glh_DrawPane('butd', gGuideGlh, &butA);
    Glh_DrawPane('dot6', gGuideGlh, &butA);
    dots.a = (u8)(fa * ga[0] / 255);
    Glh_DrawPane('dot1', gGuideGlh, &dots);
    dots.a = (u8)(fa * ga[1] / 255);
    Glh_DrawPane('dot4', gGuideGlh, &dots);

    GX_Setup2DText();
    Menu_DrawGuideText(alpha);

    gGuideAlpha += 4;
    gGuideBlink++;
    if (gGuideAlpha > 255) gGuideAlpha = 255;
}

/* 0x8131567C: Scene draw hook for state 5. */
void Menu_Draw2D(u8 alpha) {
    GXColor c = { 255, 255, 255, 255 };
    Scene *scene = Scene_Get();
    s16 a;

    Menu_DrawFaces(alpha);
    Menu_DrawSubmenus();
    Menu_DrawGuide(255);
    if (SCENE_FATAL_ERROR(scene)) {
        /* "An error has occurred..." over a darkened screen */
        Fader_GetAlpha(&sErrorMsg, &a, NULL);
        Screen_Dim((u8)(a / 2));
        GX_Setup2DView();
        GX_Setup2DText();
        Fader_DrawString(&sErrorMsg, gDialogSth, gDialogGlh, &c);
    }
}
