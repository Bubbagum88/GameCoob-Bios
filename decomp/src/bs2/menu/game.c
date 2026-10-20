// Non-matching: functionally equivalent decompilation
//
// "Game Play" submenu (PAL 1.0 BS2, 0x81315724..0x813157B8 and
// 0x813275CC..0x81327EE4). Shows the disc banner and a status line that
// follows the DVD state, and boots the game on START.

#include "bs2/menu.h"
#include "bs2/scene.h"
#include "bs2/res.h"
#include "bs2/sound.h"
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

/* Status lines (index into the game STH0, res 22..27) */
enum {
    GAMEMSG_PRESS_START = 0,
    GAMEMSG_ERROR = 1,
    GAMEMSG_CANT_READ = 2,
    GAMEMSG_INSERT_DISC = 3,
    GAMEMSG_READING = 4
};

/* Offsets inside a BNR1/BNR2 opening.bnr image */
#define BNR_IMAGE      0x0020  /* 96 x 32 RGB5A3 */
#define BNR_LONG_NAME  0x1860  /* 0x40 bytes */
#define BNR_LONG_MAKER 0x18A0  /* 0x40 bytes */
#define BNR_DESC       0x18E0  /* 0x80 bytes */
#define BNR2_LANG_SIZE 0x140   /* BNR2: one text block per language */

GameParams gGameParamsBuf;          /* 0x814B6360 */
GameParams *gGameParams;            /* 0x814AD440 */

static void *sGameGlh;              /* 0x814AD570 layout, res 21 */
static void *sGameSth;              /* 0x814AD574 strings, res 22..27 */
static s32 sStarting;               /* 0x814AD578 START pressed, booting */
static s32 sFatal;                  /* 0x814AD57C fatal disc error seen */
static s32 sAmbientStopped;         /* 0x814AD580 */

/* 0x814DE4B8: [0] banner, [1..5] status lines */
static Fader sGameFader[6];

extern GXColor sWhite;              /* 0x814AE038 (255,255,255,255) */
extern GXColor sWhite2;             /* 0x814AE03C */
extern const SRT s2DLayoutSRT;      /* 0x8137EE88 S(1,-1,1) T(-292,224,0) */
extern const Vec s2DCamUp;          /* 0x8137EEA8 (0,1,0) */
extern const Vec s2DCamEye;         /* 0x8137EEB4 (0,0,z) z set from fovy */
extern const Vec s2DCamAt;          /* 0x8137EEC0 (0,0,0) */

static void GameParams_SetDefaults(GameParams *p);

/* 0x81315724 */
void Game_InitParams(BOOL first) {
    if (first) {
        gGameParams = &gGameParamsBuf;
        GameParams_SetDefaults(gGameParams);
    }
    Game_Init(first);
}

/* 0x8131576C */
static void GameParams_SetDefaults(GameParams *p) {
    p->tint[0] = 0;
    p->tint[1] = -90;
    p->tint[2] = 20;
    p->tint[3] = 255;
    p->ringRadius = 75;
    p->cubeScale = 0.65f;
    p->fadeSpeed = 10;
    p->shrinkSpeed = 5;
}

/* 0x813157B0 */
GameParams *Game_GetParams(void) {
    return gGameParams;
}

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
static s32 Game_HandleInput(void) {
    MenuWork *menu = Menu_Get();
    s32 next = MENU_GAME;

    if (!GameIcon_IsSettled()) return MENU_GAME;
    if (!Fx_IsReady(0)) return MENU_GAME;
    if (sFatal) return MENU_GAME;

    if ((gMenuPad.buttonDown & PAD_B) &&
        (menu->zoom == 0x7FFF || menu->inputAlways) &&
        !Disc_GetFadeAlpha()) {
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
static void Game_UpdateMessages(void) {
    Scene *scene = Scene_Get();
    s32 st;

    st = sFatal ? DISC_FATAL : Disc_GetState();

    Fader_Step(&sGameFader[1], st == DISC_READY ? FADE_IN : FADE_OUT);
    Fader_Step(&sGameFader[2], (st == DISC_NONE || st == DISC_COVER_OPEN) ? FADE_IN : FADE_OUT);
    Fader_Step(&sGameFader[5], (st == DISC_RETRY || st == DISC_WRONG) ? FADE_IN : FADE_OUT);
    Fader_Step(&sGameFader[3], st == DISC_FATAL ? FADE_IN : FADE_OUT);

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
    default:            /* reading, motor spinning up, ... */
        Fader_Step(&sGameFader[4], FADE_IN);
        break;
    }
}

static s32 Game_FxMessage(void) {
    switch (Disc_GetState()) {
    case DISC_READY: return 0;
    case DISC_NONE:  return 1;
    default:         return 2;
    }
}

/* 0x81327920: state MENU_GAME. Returns the next menu state. */
s32 Game_Update(void) {
    s32 next = MENU_GAME;

    if (sStarting) {
        Disc_StartGame();           /* fades out and boots once the fade ends */
    } else {
        if (sAmbientStopped) {
            Sound_StartAmbient(0);
            sAmbientStopped = FALSE;
        }
        Disc_FadeOutStart();        /* keeps the boot fade parked */
        next = Game_HandleInput();
    }
    Game_UpdateMessages();
    if (GameIcon_IsSettled()) {
        FxGame_Update(sStarting, Game_FxMessage());
        Fx_Update();
    }
    Fader_Step(&sGameFader[0], Disc_HasBanner() ? FADE_IN : FADE_OUT);
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
static void Game_DrawBanner(u8 alpha) {
    GXColor color = sWhite;
    TexHeader tex;
    s16 a;
    s32 slide;
    u8 *bnr;
    u32 ofs;

    Fader_GetAlpha(&sGameFader[0], &a, &slide);
    if (a == 0) return;

    bnr = Disc_GetBanner();
    tex.format = GX_TF_RGB5A3;
    tex.alpha = 0;
    tex.width = 96;
    tex.height = 32;
    tex.wrapS = GX_CLAMP;
    tex.wrapT = GX_CLAMP;
    tex.indexed = 0;
    tex.tlutFormat = 0;
    tex.tlutCount = 0;
    tex.tlutOffset = 0;
    tex.mipmap = 0;
    tex.edgeLod = 0;
    tex.biasClamp = 0;
    tex.maxAniso = 0;
    tex.minFilter = GX_LINEAR;
    tex.magFilter = GX_LINEAR;
    tex.minLod = 0;
    tex.maxLod = 0;
    tex.lodBias = 0;
    tex.imageOffset = (bnr + BNR_IMAGE) - (u8 *)&tex;
    color.a = (u8)(alpha * a / 255);

    GX_Setup2D(1, 0, 1, 0, color.a);
    Glh_DrawPaneTex('bana', sGameGlh, &color, &tex);
    GX_Setup2DText();

    if (Disc_GetBannerType() == 0) {            /* BNR1: one language */
        ofs = 0;
    } else if (Disc_GetBannerType() == 1) {     /* BNR2: six languages */
        ofs = Lang_Get() * BNR2_LANG_SIZE;
    } else {
        return;
    }
    Glh_DrawBannerLine('titl', sGameGlh, &color, bnr + ofs + BNR_LONG_NAME, 0x40);
    Glh_DrawBannerLine('makr', sGameGlh, &color, bnr + ofs + BNR_LONG_MAKER, 0x40);
    Glh_DrawBannerBox('info', sGameGlh, &color, bnr + ofs + BNR_DESC, 0x80);
}

/* 0x81327CCC: drawn by Menu_DrawSubmenus while zoomed into the Game face.
   fxAlpha: mosaic, bgAlpha: backdrop, textAlpha: banner and status. */
void Game_Draw(u8 fxAlpha, u8 bgAlpha, u8 textAlpha) {
    DispSettings *disp = Disp_GetSettings();
    SRT srt = s2DLayoutSRT;
    Vec up = s2DCamUp;
    Vec eye = s2DCamEye;
    Vec at = s2DCamAt;
    Mtx view, mv, nrm;
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
    color = sWhite2;
    color.a = (u8)(textAlpha * 255 / 255);
    GX_Setup2DText();
    Fader_DrawString(&sGameFader[1], sGameSth, sGameGlh, &color);
    Fader_DrawString(&sGameFader[2], sGameSth, sGameGlh, &color);
    Fader_DrawString(&sGameFader[4], sGameSth, sGameGlh, &color);
    Fader_DrawString(&sGameFader[5], sGameSth, sGameGlh, &color);
}
