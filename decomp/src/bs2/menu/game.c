// "Game Play" submenu (PAL 1.0 BS2, 0x813275CC..0x81327EE4). Shows the disc
// banner and a status line that follows the DVD state, and boots the game on
// START.

#include "bs2/menu.h"
#include "bs2/scene.h"
#include "bs2/res.h"
#include <math.h>

/* Status lines (index into the game STH0, res 22..27) */
enum {
    GAMEMSG_PRESS_START = 0,
    GAMEMSG_ERROR = 1,
    GAMEMSG_CANT_READ = 2,
    GAMEMSG_INSERT_DISC = 3,
    GAMEMSG_READING = 4
};

/* Offsets inside a BNR1/BNR2 opening.bnr image */
#define BNR_IMAGE 0x0020      /* 96 x 32 RGB5A3 */
#define BNR_LONG_NAME 0x1860  /* 0x40 bytes */
#define BNR_LONG_MAKER 0x18A0 /* 0x40 bytes */
#define BNR_DESC 0x18E0       /* 0x80 bytes */
#define BNR2_LANG_SIZE 0x140  /* BNR2: one text block per language */
static void* sGameGlh;       /* layout, res 21 */
static void* sGameSth;       /* strings, res 22..27 */
static s32 sStarting;        /* START pressed, booting */
static s32 sFatal;           /* fatal disc error seen */
static s32 sAmbientStopped;

/* [0] banner, [1..5] status lines */
static Fader sGameFader[6];

static const GXColor sBannerColor = { 255, 255, 255, 255 };
static const GXColor sTextColor = { 255, 255, 255, 255 };

/* 0x813275CC: reload the per-language strings. */
void Game_LoadLangRes(void) {
    sGameSth = ResGet(Res_GameStrings());
}

/* 0x813275F4 */
void Game_Init(BOOL first) {
    if (first) {
        sGameGlh = ResGet(Res_GameLayout());
        sGameSth = ResGet(Res_GameStrings());
    }
    Fader_Init(&sGameFader[0]);
    Fader_Init(&sGameFader[1]);
    sGameFader[1].id = GAMEMSG_PRESS_START;
    Fader_Init(&sGameFader[2]);
    sGameFader[2].id = GAMEMSG_INSERT_DISC;
    Fader_Init(&sGameFader[3]);
    sGameFader[3].id = GAMEMSG_ERROR;
    Fader_Init(&sGameFader[4]);
    sGameFader[4].id = GAMEMSG_READING;
    Fader_Init(&sGameFader[5]);
    sGameFader[5].id = GAMEMSG_CANT_READ;
    sStarting = FALSE;
    sFatal = FALSE;
    sAmbientStopped = FALSE;
}

/* 0x813276AC: B leaves (SE 6), START boots when a disc is ready (SE 22). */
s32 Game_HandleInput(void) {
    MenuWork* menu = Menu_Get();
    s32 next = MENU_GAME;

    if (!GameIcon_IsSettled()) {
        return MENU_GAME;
    }
    if (!Fx_IsReady(0)) {
        return MENU_GAME;
    }
    if (sFatal) {
        return MENU_GAME;
    }

    if ((gMenuPad.buttonDown & PAD_B) && (menu->zoom == 0x7FFF || menu->inputAlways) && !Disc_GetFadeAlpha()) {
        Sound_Play(SE_EXIT);
        next = MENU_GAME_FACE;
    }
    if ((gMenuPad.buttonDown & PAD_START) && Disc_GetState() == DISC_READY) {
        sAmbientStopped = TRUE;
        Sound_StopAmbient();
        Sound_Play(SE_GAME_START);
        sStarting = TRUE;
    }
    return next;
}

/* 0x813277A8: fade the status lines according to the disc state. */
void Game_UpdateMessages(void) {
    Scene* scene = Scene_Get();
    s32 st;

    if (!sFatal) {
        st = Disc_GetState();
    } else {
        st = DISC_FATAL;
    }

    if (st == DISC_READY) {
        Fader_Step(&sGameFader[1], FADE_IN);
    } else {
        Fader_Step(&sGameFader[1], FADE_OUT);
    }
    if (st == DISC_NONE || st == DISC_COVER_OPEN) {
        Fader_Step(&sGameFader[2], FADE_IN);
    } else {
        Fader_Step(&sGameFader[2], FADE_OUT);
    }
    if (st == DISC_RETRY || st == DISC_WRONG) {
        Fader_Step(&sGameFader[5], FADE_IN);
    } else {
        Fader_Step(&sGameFader[5], FADE_OUT);
    }
    if (st == DISC_FATAL) {
        Fader_Step(&sGameFader[3], FADE_IN);
    } else {
        Fader_Step(&sGameFader[3], FADE_OUT);
    }

    switch (st) {
    case DISC_READY:
        Fader_Step(&sGameFader[4], FADE_OUT);
        break;
    case DISC_FATAL:
        sStarting = FALSE;
        sFatal = TRUE;
        SCENE_FATAL_ERROR(scene) = TRUE;
        Fader_Step(&sGameFader[4], FADE_OUT);
        break;
    case DISC_NONE:
    case DISC_COVER_OPEN:
    case DISC_WRONG:
    case DISC_RETRY:
        sStarting = FALSE;
        Fader_Step(&sGameFader[4], FADE_OUT);
        break;
    default: /* reading, motor spinning up, ... */
        Fader_Step(&sGameFader[4], FADE_IN);
        break;
    }
}

static s32 Game_FxMessage(void) {
    switch (Disc_GetState()) {
    case DISC_READY:
        return 0;
    case DISC_NONE:
        return 1;
    default:
        return 2;
    }
}

/* 0x81327920: state MENU_GAME. Returns the next menu state. */
s32 Game_Update(void) {
    s32 next = MENU_GAME;

    if (sStarting) {
        Disc_StartGame(); /* fades out and boots once the fade ends */
    } else {
        if (sAmbientStopped) {
            Sound_StartAmbient(0);
            sAmbientStopped = FALSE;
        }
        Disc_FadeOutStart(); /* keeps the boot fade parked */
        next = Game_HandleInput();
    }
    Game_UpdateMessages();
    if (GameIcon_IsSettled()) {
        FxGame_Update(sStarting, Game_FxMessage());
        Fx_Update();
    }
    if (Disc_HasBanner()) {
        Fader_Step(&sGameFader[0], FADE_IN);
    } else {
        Fader_Step(&sGameFader[0], FADE_OUT);
    }
    return next;
}

/* 0x81327A10: entering from the Game face. */
void Game_Enter(void) {
    FxGame_Update(sStarting, Game_FxMessage());
    Fx_Update();
    Fader_Reset(&sGameFader[1]);
    Fader_Reset(&sGameFader[2]);
    Fader_Reset(&sGameFader[3]);
    Fader_Reset(&sGameFader[4]);
    Fader_Reset(&sGameFader[5]);
    Fader_Reset(&sGameFader[0]);
}

/* 0x81327AB4 */
BOOL Game_IsFatal(void) {
    return sFatal;
}

/* 0x81327ABC: banner image ("bana" pane) and its texts. */
void Game_DrawBanner(u8 alpha) {
    TexHeader tex;
    s16 a;
    GXColor color = sBannerColor;
    u8* bnr;

    Fader_GetAlpha(&sGameFader[0], &a, NULL);
    if (a == 0) {
        return;
    }

    bnr = Disc_GetBanner();
    color.a = (u8)(alpha * a / 255);
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
    tex.imageOffset = (bnr + BNR_IMAGE) - (u8*)&tex;

    GX_Setup2D(1, 0, 1);
    Glh_DrawPaneTex('bana', sGameGlh, &color, &tex);
    GX_Setup2DText();

    if (Disc_GetBannerType() == 0) { /* BNR1: one language */
        Glh_DrawBannerLine('titl', sGameGlh, &color, bnr + BNR_LONG_NAME, 0x40);
        Glh_DrawBannerLine('makr', sGameGlh, &color, bnr + BNR_LONG_MAKER, 0x40);
        Glh_DrawBannerBox('info', sGameGlh, &color, bnr + BNR_DESC, 0x80);
    } else if (Disc_GetBannerType() == 1) { /* BNR2: six languages */
        Glh_DrawBannerLine('titl', sGameGlh, &color, bnr + Lang_Get() * BNR2_LANG_SIZE + BNR_LONG_NAME, 0x40);
        Glh_DrawBannerLine('makr', sGameGlh, &color, bnr + Lang_Get() * BNR2_LANG_SIZE + BNR_LONG_MAKER, 0x40);
        Glh_DrawBannerBox('info', sGameGlh, &color, bnr + Lang_Get() * BNR2_LANG_SIZE + BNR_DESC, 0x80);
    }
}

/* 0x81327CCC: drawn by Menu_DrawSubmenus while zoomed into the Game face.
   fxAlpha: mosaic, bgAlpha: backdrop, textAlpha: banner and status. */
void Game_Draw(u8 fxAlpha, u8 bgAlpha, u8 textAlpha) {
    DispSettings* disp = Disp_GetSettings();
    SRT srt = { 1.0f, -1.0f, 1.0f, 0, 0, 0, 0, -292.0f, 224.0f, 0.0f };
    Vec up = { 0.0f, 1.0f, 0.0f };
    Vec eye = { 0.0f, 0.0f, 0.0f };
    Vec at = { 0.0f, 0.0f, 0.0f };
    Mtx mv;
    Mtx view;
    Mtx nrm;
    GXColor unused; /* never used, but still takes a stack slot */
    GXColor color;

    /* Camera at a distance where one unit is one pixel on a 448 line screen */
    eye.z = 224.0f / tanf(3.141592f * disp->fovy / 360.0f);
    C_MTXLookAt(view, &eye, &up, &at);
    MtxFromSRT(&srt, mv);
    PSMTXConcat(view, mv, mv);
    MtxInverse33(mv, nrm);
    GXLoadPosMtxImm(mv, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    Backdrop_Draw(mv, bgAlpha);
    Game_DrawBanner(textAlpha);
    if (GameIcon_IsSettled()) {
        Fx_Draw(fxAlpha);
    }

    GXLoadPosMtxImm(mv, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    color = sTextColor;
    color.a = (u8)(textAlpha * 255 / 255);
    GX_Setup2DText();
    Fader_DrawString(&sGameFader[1], sGameSth, sGameGlh, &color);
    Fader_DrawString(&sGameFader[2], sGameSth, sGameGlh, &color);
    Fader_DrawString(&sGameFader[4], sGameSth, sGameGlh, &color);
    Fader_DrawString(&sGameFader[5], sGameSth, sGameGlh, &color);
}
