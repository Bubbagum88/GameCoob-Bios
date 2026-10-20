// Non-matching: functionally equivalent decompilation
//
// Main menu core (PAL 1.0 BS2, 0x81310F58..0x81312F88): resources, the
// cube state machine, input, cube rotation/zoom and fader bookkeeping.
// Drawing is in menu_draw.c. Addresses are given above each function.

#include "bs2/menu.h"
#include "bs2/scene.h"
#include "bs2/res.h"
#include "bs2/sound.h"
#include <dolphin/os.h>

MenuWork gMenuWork;                 /* 0x814B5EA0 */

MenuWork *gMenu;                    /* 0x814AD3F0 */
s32 gMenuState;                     /* 0x814AD3F4 MenuState */
s32 gMenuNextState;                 /* 0x814AD3F8 */
void *gTopFaceGlh;                  /* 0x814AD3FC front face layout */
void *gGameFaceGlh;                 /* 0x814AD400 */
void *gCardFaceGlh;                 /* 0x814AD404 */
void *gOptionFaceGlh;               /* 0x814AD408 */
void *gClockFaceGlh;                /* 0x814AD40C */
void *gMenuTexBank;                 /* 0x814AD410 TXH0 res 167 */
void *gGuideGlh;                    /* 0x814AD414 button guide layout, res 115 */
void *gDialogGlh;                   /* 0x814AD418 res 108 */
void *gDialogSth;                   /* 0x814AD41C res 109..114 */
s16 gSlotGlow[2];                   /* 0x814AD420 card slot glow, 60..255 */
s32 gSlotFill[2];                   /* 0x814AD424 card slot fill, 0..255 */
void *gSoundModeTex[2];             /* 0x814AD42C mono / stereo label */
s16 gGuideAlpha;                    /* 0x814AD434 */
s16 gGuideBlink;                    /* 0x814AD436 */
s16 gGuideLang;                     /* 0x814AD438 language the guide text is shown in */

extern u32 gSramLostPending;        /* 0x814AD000 */
extern const Vec sZeroVec;          /* 0x8137EC28 */

static void Menu_InitRes(BOOL first);
static void Menu_SetPalTextLayout(MenuWork *m);
static void Menu_SetDefaults(MenuWork *m);
static void Menu_ResetCube(MenuWork *m);

/* 0x81310F58: reload the per-language face layouts after a language change. */
void Menu_LoadLangRes(void) {
    s32 i;

    gGameFaceGlh = ResGet(Res_GameFaceLayout());
    gTopFaceGlh = ResGet(Res_TopFaceLayout());
    gCardFaceGlh = ResGet(Res_CardFaceLayout());
    gClockFaceGlh = ResGet(Res_ClockFaceLayout());
    gOptionFaceGlh = ResGet(Res_OptionFaceLayout());
    gDialogSth = ResGet(Res_DialogStrings());
    for (i = 0; i < 7; i++) {
        gMenuWork.weekdayTex[i] = ResGet(Res_WeekdayTexBase() + i);
    }
    gSoundModeTex[0] = ResGet(Res_MonoTex());
    gSoundModeTex[1] = ResGet(Res_StereoTex());
}

/* 0x81311014 */
static void Menu_InitRes(BOOL first) {
    s32 i;

    if (first) {
        gGuideGlh = ResGet(Res_GuideLayout());
        gMenuTexBank = ResGet(Res_MenuTexBank());
        gDialogGlh = ResGet(Res_DialogLayout());
        for (i = 0; i < 6; i++) {
            gMenuWork.guideSth[i] = ResGet(Res_GuideStrings(i));
        }
        gGameFaceGlh = ResGet(Res_GameFaceLayout());
        gTopFaceGlh = ResGet(Res_TopFaceLayout());
        gCardFaceGlh = ResGet(Res_CardFaceLayout());
        gClockFaceGlh = ResGet(Res_ClockFaceLayout());
        gOptionFaceGlh = ResGet(Res_OptionFaceLayout());
        gDialogSth = ResGet(Res_DialogStrings());
        for (i = 0; i < 7; i++) {
            gMenuWork.weekdayTex[i] = ResGet(Res_WeekdayTexBase() + i);
        }
        gSoundModeTex[0] = ResGet(Res_MonoTex());
        gSoundModeTex[1] = ResGet(Res_StereoTex());
        Txh_SetBank(gMenuTexBank);
        for (i = 0; i < 10; i++) {
            gMenuWork.digitTex[i] = ResGet(Res_DigitTexBase() + i);
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
    for (i = 0; i < 6; i++) {
        Fader_Init(&gMenuWork.guideIcon[i]);
        gMenuWork.guideIcon[i].id = 0;
    }

    /* Button guide labels, ids index the guide STH0 table */
    Fader_Init(&gMenuWork.guideText[0]); gMenuWork.guideText[0].id = GUIDE_MENU_SELECT;
    Fader_Init(&gMenuWork.guideText[1]); gMenuWork.guideText[1].id = GUIDE_CANCEL;
    Fader_Init(&gMenuWork.guideText[2]); gMenuWork.guideText[2].id = GUIDE_CONFIRM;
    Fader_Init(&gMenuWork.guideText[7]); gMenuWork.guideText[7].id = GUIDE_CONFIRM2;
    Fader_Init(&gMenuWork.guideText[3]); gMenuWork.guideText[3].id = GUIDE_SELECT;
    Fader_Init(&gMenuWork.guideText[4]); gMenuWork.guideText[4].id = GUIDE_CHANGE;
    Fader_Init(&gMenuWork.guideText[6]); gMenuWork.guideText[6].id = GUIDE_FINISH;
    Fader_Init(&gMenuWork.guideText[5]); gMenuWork.guideText[5].id = GUIDE_CANCEL2;

    /* Each submenu fades in with three staggered faders (delays 0, 20, 70). */
    for (i = 0; i < 3; i++) {
        Fader_Init(&gMenuWork.subFade[SUBFADE_OPTION][i]);
        Fader_Init(&gMenuWork.subFade[SUBFADE_CARD][i]);
        Fader_Init(&gMenuWork.subFade[SUBFADE_CLOCK][i]);
        Fader_Init(&gMenuWork.subFade[SUBFADE_GAME][i]);
        gMenuWork.subFade[SUBFADE_OPTION][i].delay = gMenu->subFadeDelay[i];
        gMenuWork.subFade[SUBFADE_CARD][i].delay = gMenu->subFadeDelay[i];
        gMenuWork.subFade[SUBFADE_CLOCK][i].delay = gMenu->subFadeDelay[i];
        gMenuWork.subFade[SUBFADE_GAME][i].delay = gMenu->subFadeDelay[i];
    }

    Fader_Init(&gMenuWork.discBanner);
    Fader_Init(&gMenuWork.discNone);
    Fader_Init(&gMenuWork.discUnknown);
    Fader_Init(&gMenuWork.errorMsg);
    gMenuWork.errorMsg.id = 4;          /* "An error has occurred." */
    for (i = 0; i < 6; i++) {
        Fader_Init(&gMenuWork.langGuide[i]);
    }
}

/* 0x81311314: called from Scene_Init. */
void Menu_Init(BOOL first) {
    void *keys;

    if (first) {
        gMenu = &gMenuWork;
        Menu_SetDefaults(gMenu);
    }
    Menu_ResetCube(gMenu);
    gMenuState = MENU_TOP;
    gMenuNextState = MENU_TOP;
    Menu_InitRes(first);
    if (first) {
        gMenuWork.iconModel.data = ResGet(178);      /* small cube model */
        J3D_LoadModelData(&gMenuWork.iconModel);
        Fx_InitCubeModel();
        keys = ResGet(37);                            /* CAFD keyframes */
        Fx_Init(keys, ResGet(Res_FxGlyphs()));        /* mosaic shapes, res 179..184 */
    }
    CardIcon_Init(&gMenuWork.iconModel, first);
    GameIcon_Init(&gMenuWork.iconModel, first);
    OptionIcon_Init(&gMenuWork.iconModel, first);
    ClockIcon_Init(&gMenuWork.iconModel, first);
}

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
    Mtx m, rot, wob;
    Vec scale, s;
    Scene *scene;
    s16 t;
    f32 tx, ty;
    s32 rz, ry, rx;
    MtxPtr view;

    scene = Scene_Get();
    PSMTXIdentity(m);
    MtxRotTrans(gMenu->rotX, gMenu->rotY, 0, rot, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(rot, m, m);

    t = gMenu->wobbleFrame;
    PSMTXIdentity(wob);
    PSMTXConcat(wob, m, m);
    ty = (f32)gMenu->wobbleAmpTY * SinS(gMenu->wobbleSpeedTY * t);
    tx = (f32)gMenu->wobbleAmpTX * SinS(gMenu->wobblePhaseTX + gMenu->wobbleSpeedTX * t);
    rz = (s32)((f32)gMenu->wobbleAmp[2] * CosS(gMenu->wobbleSpeed[2] * t));
    ry = (s16)((f32)gMenu->wobbleAmp[1] * CosS(gMenu->wobblePhaseRY + gMenu->wobbleSpeed[1] * t));
    rx = (s32)((f32)gMenu->wobbleAmp[0] * CosS(gMenu->wobbleSpeed[0] * t));
    MtxRotTrans(rx, ry, rz, wob, tx, ty, 0.0f);
    PSMTXConcat(wob, m, m);

    PSMTXIdentity(gMenuWork.cubeMtx);
    PSMTXConcat(m, gMenuWork.cubeMtx, gMenuWork.cubeMtx);

    /* Zooming into a submenu pushes the cube back by zoomDepth*(1-cos(zoom)). */
    if (gMenu->zoom != 0) {
        f32 z = (f32)gMenu->zoom;
        MtxRotTrans(0, 0, (s32)(z * gMenu->zoomSpin), m, 0.0f, 0.0f,
                    gMenu->zoomDepth * (1.0f - CosS(gMenu->zoom)));
        PSMTXConcat(m, gMenuWork.cubeMtx, gMenuWork.cubeMtx);
    }

    s = sZeroVec;
    s.x = s.y = s.z = 1.1f * scene->modelScale;
    scale = s;
    J3D_SetModelMtx(Scene_GetBigCube(), gMenuWork.cubeMtx, &scale);
    view = Camera_GetView();
    J3D_SetViewMtx(Scene_GetBigCube(), view);
}

/* 0x81312044: Game Play face shows the banner, "no disc" or an unknown disc. */
static void Menu_UpdateDiscFaders(void) {
    if (Disc_HasBanner()) {
        Fader_Step(&gMenuWork.discBanner, FADE_IN);
        Fader_Step(&gMenuWork.discNone, FADE_OUT);
        Fader_Step(&gMenuWork.discUnknown, FADE_OUT);
        return;
    }
    Fader_Step(&gMenuWork.discBanner, FADE_OUT);
    if (Disc_GetState() == DISC_NONE) {
        Fader_Step(&gMenuWork.discNone, FADE_IN);
        Fader_Step(&gMenuWork.discUnknown, FADE_OUT);
        return;
    }
    Fader_Step(&gMenuWork.discNone, FADE_OUT);
    Fader_Step(&gMenuWork.discUnknown, FADE_IN);
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
    case MENU_TOP:
        ROT_TOWARD_ZERO(gMenu->rotX, step);
        ROT_TOWARD_ZERO(gMenu->rotY, step);
        break;
    case MENU_GAME_FACE:
    case MENU_GAME:
        gMenu->rotX += step;
        ROT_TOWARD_ZERO(gMenu->rotY, step);
        if (gMenu->rotX > 0x4000) gMenu->rotX = 0x4000;
        break;
    case MENU_CARD_FACE:
    case MENU_CARD:
        gMenu->rotX -= step;
        ROT_TOWARD_ZERO(gMenu->rotY, step);
        if (gMenu->rotX < -0x4000) gMenu->rotX = -0x4000;
        break;
    case MENU_OPTION_FACE:
    case MENU_OPTION:
        ROT_TOWARD_ZERO(gMenu->rotX, step);
        gMenu->rotY += step;
        if (gMenu->rotY > 0x4000) gMenu->rotY = 0x4000;
        break;
    case MENU_CLOCK_FACE:
    case MENU_CLOCK:
        ROT_TOWARD_ZERO(gMenu->rotX, step);
        gMenu->rotY -= step;
        if (gMenu->rotY < -0x4000) gMenu->rotY = -0x4000;
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
    s8 d;

    if (slot[0].state == CARD_SLOT_READY) {
        d = 4;
        gSlotFill[0] = 0xFF - (slot[0].freeBlocks * 0xFF) / slot[0].totalBlocks;
    } else {
        d = -4;
    }
    gSlotGlow[0] += d;
    if (gSlotGlow[0] > 0xFF) gSlotGlow[0] = 0xFF;
    if (gSlotGlow[0] < 60) gSlotGlow[0] = 60;

    if (slot[1].state == CARD_SLOT_READY) {
        d = 4;
        gSlotFill[1] = 0xFF - (slot[1].freeBlocks * 0xFF) / slot[1].totalBlocks;
    } else {
        d = -4;
    }
    gSlotGlow[1] += d;
    if (gSlotGlow[1] > 0xFF) gSlotGlow[1] = 0xFF;
    if (gSlotGlow[1] < 60) gSlotGlow[1] = 60;
}

/* 0x81312580: which button guide labels/glyphs are visible in each state. */
static void Menu_UpdateGuide(void) {
    s32 i;
    Fader *t = gMenuWork.guideText;
    Fader *ic = gMenuWork.guideIcon;

    switch (gMenuState) {
    case MENU_TOP:
        Fader_Step(&t[0], FADE_IN);     /* Menu Selection */
        Fader_Step(&t[1], FADE_OUT);
        Fader_Step(&t[2], FADE_OUT);
        Fader_Step(&t[3], FADE_OUT);
        Fader_Step(&t[4], FADE_OUT);
        Fader_Step(&t[5], FADE_OUT);
        Fader_Step(&t[7], FADE_OUT);
        Fader_Step(&t[6], FADE_OUT);
        break;
    case MENU_GAME_FACE:
    case MENU_CARD_FACE:
    case MENU_OPTION_FACE:
    case MENU_CLOCK_FACE:
        Fader_Step(&t[0], FADE_OUT);
        Fader_Step(&t[1], FADE_IN);     /* Cancel */
        Fader_Step(&t[2], FADE_IN);     /* Confirm */
        Fader_Step(&t[3], FADE_OUT);
        Fader_Step(&t[4], FADE_OUT);
        Fader_Step(&t[5], FADE_OUT);
        Fader_Step(&t[6], FADE_OUT);
        Fader_Step(&t[7], FADE_OUT);
        break;
    case MENU_GAME:
        Fader_Step(&t[0], FADE_OUT);
        Fader_Step(&t[1], FADE_OUT);
        Fader_Step(&t[2], FADE_OUT);
        Fader_Step(&t[3], FADE_OUT);
        Fader_Step(&t[4], FADE_OUT);
        if (!Game_IsFatal()) {
            Fader_Step(&t[5], FADE_IN); /* Cancel */
        } else {
            Fader_Step(&t[5], FADE_OUT);
        }
        Fader_Step(&t[6], FADE_OUT);
        Fader_Step(&t[7], FADE_OUT);
        break;
    case MENU_OPTION:
        if (Option_IsTop()) {
            Fader_Step(&t[3], FADE_IN); /* Select */
            Fader_Step(&t[4], FADE_OUT);
            Fader_Step(&t[6], FADE_IN); /* Finish */
            Fader_Step(&t[5], FADE_OUT);
        } else {
            Fader_Step(&t[3], FADE_OUT);
            Fader_Step(&t[4], FADE_IN); /* Change */
            Fader_Step(&t[6], FADE_OUT);
            Fader_Step(&t[5], FADE_IN); /* Cancel */
        }
        Fader_Step(&t[0], FADE_OUT);
        Fader_Step(&t[1], FADE_OUT);
        Fader_Step(&t[2], FADE_OUT);
        Fader_Step(&t[7], FADE_IN);     /* Confirm */
        break;
    case MENU_CLOCK:
        if (Clock_IsTop()) {
            Fader_Step(&t[3], FADE_IN);
            Fader_Step(&t[4], FADE_OUT);
            Fader_Step(&t[6], FADE_IN);
            Fader_Step(&t[5], FADE_OUT);
        } else {
            Fader_Step(&t[3], FADE_OUT);
            Fader_Step(&t[4], FADE_IN);
            Fader_Step(&t[6], FADE_OUT);
            Fader_Step(&t[5], FADE_IN);
        }
        Fader_Step(&t[7], FADE_IN);
        Fader_Step(&t[0], FADE_OUT);
        Fader_Step(&t[1], FADE_OUT);
        Fader_Step(&t[2], FADE_OUT);
        break;
    case MENU_CARD:
        if (Card_IsBrowsing()) {
            Fader_Step(&t[5], FADE_OUT);
            Fader_Step(&t[6], FADE_IN); /* Finish */
        } else {
            Fader_Step(&t[5], FADE_IN); /* Cancel */
            Fader_Step(&t[6], FADE_OUT);
        }
        Fader_Step(&t[7], FADE_IN);
        Fader_Step(&t[0], FADE_OUT);
        Fader_Step(&t[1], FADE_OUT);
        Fader_Step(&t[2], FADE_OUT);
        Fader_Step(&t[3], FADE_IN);
        break;
    }

    switch (gMenuState) {
    case MENU_TOP:
        Fader_Step(&ic[0], FADE_OUT);
        Fader_Step(&ic[1], FADE_OUT);
        Fader_Step(&ic[2], FADE_OUT);
        Fader_Step(&ic[3], FADE_IN);    /* stick */
        Fader_Step(&ic[4], FADE_OUT);
        Fader_Step(&ic[5], FADE_OUT);
        break;
    case MENU_GAME:
        Fader_Step(&ic[0], FADE_OUT);
        if (!Game_IsFatal()) {
            Fader_Step(&ic[1], FADE_IN);
        } else {
            Fader_Step(&ic[1], FADE_OUT);
        }
        Fader_Step(&ic[2], FADE_OUT);
        Fader_Step(&ic[3], FADE_OUT);
        Fader_Step(&ic[4], FADE_OUT);
        Fader_Step(&ic[5], FADE_OUT);
        break;
    case MENU_GAME_FACE:
    case MENU_CARD_FACE:
    case MENU_OPTION_FACE:
    case MENU_CLOCK_FACE:
        Fader_Step(&ic[0], FADE_OUT);
        Fader_Step(&ic[1], FADE_OUT);
        Fader_Step(&ic[2], FADE_OUT);
        Fader_Step(&ic[3], FADE_OUT);
        Fader_Step(&ic[4], FADE_IN);    /* B */
        Fader_Step(&ic[5], FADE_IN);    /* A */
        break;
    default:                            /* MENU_CARD, MENU_OPTION, MENU_CLOCK */
        Fader_Step(&ic[0], FADE_IN);
        Fader_Step(&ic[1], FADE_IN);
        Fader_Step(&ic[2], FADE_IN);
        Fader_Step(&ic[3], FADE_OUT);
        Fader_Step(&ic[4], FADE_OUT);
        Fader_Step(&ic[5], FADE_OUT);
        break;
    }

    for (i = 0; i < 6; i++) {
        Fader_Step(&gMenuWork.langGuide[i], (i == gGuideLang) ? FADE_IN : FADE_OUT);
    }
}

/* 0x81312A94 */
static void Menu_UpdateSubFaders(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        Fader_Step(&gMenuWork.subFade[SUBFADE_CARD][i], gMenuState == MENU_CARD ? FADE_IN : FADE_OUT);
        Fader_Step(&gMenuWork.subFade[SUBFADE_GAME][i], gMenuState == MENU_GAME ? FADE_IN : FADE_OUT);
        Fader_Step(&gMenuWork.subFade[SUBFADE_OPTION][i], gMenuState == MENU_OPTION ? FADE_IN : FADE_OUT);
        Fader_Step(&gMenuWork.subFade[SUBFADE_CLOCK][i], gMenuState == MENU_CLOCK ? FADE_IN : FADE_OUT);
    }
}

/* 0x81312B9C: Scene state 5. Returns the next scene state (always 5). */
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
        Fader_Reset(&gMenuWork.discBanner);
        Fader_Reset(&gMenuWork.discNone);
        Fader_Reset(&gMenuWork.discUnknown);
    }

    gMenuState = gMenuNextState;
    gMenu->state = gMenuState;

    switch (gMenuState) {
    case MENU_TOP:
    case MENU_GAME_FACE:
    case MENU_CARD_FACE:
    case MENU_OPTION_FACE:
    case MENU_CLOCK_FACE:
        Scene_Get();
        step = gMenu->zoomOutStep;
        if (gMenu->zoom > step) gMenu->zoom -= step;
        if (gMenu->zoom <= step) gMenu->zoom = 0;
        if (!SCENE_FATAL_ERROR(Scene_Get())) {
            Menu_HandleInput();
        }
        Scene_Get();
        gMenu->wobbleFrame += 7;
        Menu_CalcCubeMtx();
        J3D_CalcJoints(Scene_GetBigCube());
        Menu_UpdateDiscFaders();
        Fx_Clear();
        break;

    /* Inside a submenu the cube zooms away and the submenu decides the next state. */
    case MENU_GAME:
        step = gMenu->zoomInStep;
        if (gMenu->zoom < 0x7FFF - step) gMenu->zoom += step;
        if (gMenu->zoom >= 0x7FFF - step) gMenu->zoom = 0x7FFF;
        Scene_Get();
        gMenu->wobbleFrame += 7;
        Menu_CalcCubeMtx();
        J3D_CalcJoints(Scene_GetBigCube());
        gMenuNextState = Game_Update();
        break;
    case MENU_CARD:
        step = gMenu->zoomInStep;
        if (gMenu->zoom < 0x7FFF - step) gMenu->zoom += step;
        if (gMenu->zoom >= 0x7FFF - step) gMenu->zoom = 0x7FFF;
        Scene_Get();
        gMenu->wobbleFrame += 7;
        Menu_CalcCubeMtx();
        J3D_CalcJoints(Scene_GetBigCube());
        gMenuNextState = Card_Update();
        break;
    case MENU_CLOCK:
        step = gMenu->zoomInStep;
        if (gMenu->zoom < 0x7FFF - step) gMenu->zoom += step;
        if (gMenu->zoom >= 0x7FFF - step) gMenu->zoom = 0x7FFF;
        Scene_Get();
        gMenu->wobbleFrame += 7;
        Menu_CalcCubeMtx();
        J3D_CalcJoints(Scene_GetBigCube());
        gMenuNextState = Clock_Update();
        break;
    case MENU_OPTION:
        step = gMenu->zoomInStep;
        if (gMenu->zoom < 0x7FFF - step) gMenu->zoom += step;
        if (gMenu->zoom >= 0x7FFF - step) gMenu->zoom = 0x7FFF;
        Scene_Get();
        gMenu->wobbleFrame += 7;
        Menu_CalcCubeMtx();
        J3D_CalcJoints(Scene_GetBigCube());
        gMenuNextState = Option_Update();
        break;
    }

    if (SCENE_FATAL_ERROR(scene)) {
        Fader_Step(&gMenuWork.errorMsg, FADE_IN);
    }
    return 5;
}

/* 0x81312F68 */
MtxPtr Menu_GetCubeMtx(void) {
    return gMenuWork.cubeMtx;
}

/* 0x81312F74 */
MenuWork *Menu_Get(void) {
    return gMenu;
}

/* 0x81312F7C: called by Options while scrolling through the languages. */
void Menu_SetGuideLang(s16 lang) {
    gGuideLang = lang;
}
