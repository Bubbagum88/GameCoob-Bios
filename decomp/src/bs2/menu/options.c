// Partly matching: Option_Init, Option_Update and Option_DrawText differ only in
// register numbering; Option_GetLangRow still to check.
//
// "Options" submenu (PAL 1.0 BS2, 0x81327EE4..0x81328CA4): sound mode,
// screen position and language.
//
// Sub-states (sOptSub):
//   0 OPT_MENU    up/down picks a row (SE 11), A enters it (SE 8),
//                 B zooms back out to the Options face (SE 6)
//   1 OPT_SOUND   left = mono, right = stereo (SE 9, SE 13 at the end);
//                 A stores it with OSSetSoundMode (SE 12), B restores (SE 7)
//   2 OPT_OFFSET  left/right -32..+32, applied live with
//                 __VISetAdjustingValues; A writes SRAM 0x10 (SE 12)
//   3 OPT_LANG    left/right 0..5, the guide text follows live;
//                 A calls Lang_Set (reloads every language resource)
// The rows are drawn by the mosaic particle system (FxOption_Update);
// this file draws only the language list on the right of the screen.

#include "bs2/menu.h"
#include "bs2/scene.h"
#include "bs2/res.h"
#include <math.h>

enum { OPT_MENU = 0, OPT_SOUND = 1, OPT_OFFSET = 2, OPT_LANG = 3 };
enum { OPTROW_SOUND = 0, OPTROW_OFFSET = 1, OPTROW_LANG = 2 };

static GlhHeader* sOptGlh; /* layout, res 122 */
static u32 sOptPrevSub;    /* sub-state seen by the draw code */
static s32 sOptSub;
static s16 sOptRow;    /* 0..2 */
static s16 sOptOffset; /* screen offset being edited */
static s16 sOptSound;  /* 0 mono, 1 stereo */
static s16 sOptLang;   /* language being browsed */
static s16 sOptPulse;  /* 50..120 */
static s8 sOptPulseStep; /* +-2 */

static void* sOptSth[6];        /* option strings in every language (res 123..128) */
static Fader sOptLangName[6];   /* 'lan1'..'lan6' panes: full when chosen, half otherwise */
static Fader sOptLangText[6];   /* the item texts in each language (cross-fade) */
static s16 sOptLangY[6];        /* y of the 'lan1'..'lan6' panes (pixels) */

static const GXColor sOptWhite = { 255, 255, 255, 255 };

/* 0x81327EE4: called after a language change. */
void Option_LoadLangRes(void) {
    sOptLang = Lang_Get();
}

/* 0x81327F08 */
void Option_Init(BOOL first) {
    s32 i;
    s32 j;
    GlhAnchor* base;
    GlhAnchor* anchor;
    OSSram* sram;

    if (first) {
        sOptGlh = ResGet(Res_OptionLayout());
        for (i = 0; i < 6; i++) {
            sOptSth[i] = ResGet(Res_OptionStrings(i));
        }
        base = (GlhAnchor*)((u8*)sOptGlh + sOptGlh->anchorOfs);
        for (i = 0; i < 6; i++) {
            for (anchor = base, j = 0; j < sOptGlh->anchorCount; anchor++, j++) {
                if (anchor->tag == 'lan1' + i) {
                    sOptLangY[i] = (s16)(anchor->y >> 4);
                }
            }
        }
    }
    sOptPrevSub = OPT_MENU;
    sOptSub = OPT_MENU;
    sOptRow = OPTROW_SOUND;
    sOptSound = OSGetSoundMode();
    for (i = 0; i < 6; i++) {
        Fader_Init(&sOptLangName[i]);
        Fader_Init(&sOptLangText[i]);
    }
    sram = __OSLockSram();
    sOptOffset = sram->displayOffsetH;
    __OSUnlockSram(FALSE);
    sOptLang = Lang_Get();
    sOptPulse = 100;
    sOptPulseStep = 2;
}

/* 0x81328104 */
s32 Option_UpdateMenu(void) {
    MenuWork* menu;
    OSSram* sram;
    s32 next = MENU_OPTION;

    if (!Fx_IsReady(0)) {
        return MENU_OPTION;
    }

    if ((gMenuPad.dirsNew & DIR_UP) && sOptRow > 0) {
        Sound_Play(SE_CURSOR);
        sOptRow--;
    }
    if ((gMenuPad.dirsNew & DIR_DOWN) && sOptRow < 2) {
        Sound_Play(SE_CURSOR);
        sOptRow++;
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_DECIDE);
        switch (sOptRow) {
        case OPTROW_SOUND:
            sOptSound = OSGetSoundMode();
            sOptSub = OPT_SOUND;
            break;
        case OPTROW_OFFSET:
            sOptSub = OPT_OFFSET;
            sram = __OSLockSram();
            sOptOffset = sram->displayOffsetH;
            __OSUnlockSram(FALSE);
            break;
        case OPTROW_LANG:
            sOptSub = OPT_LANG;
            sOptLang = Lang_Get();
            break;
        }
    }
    if (gMenuPad.buttonDown & PAD_B) {
        menu = Menu_Get();
        if (menu->zoom == 0x7FFF || menu->inputAlways) {
            Sound_Play(SE_EXIT);
            next = MENU_OPTION_FACE;
        }
    }
    return next;
}

/* 0x8132827C */
s32 Option_UpdateSound(void) {
    u8 unused[4]; /* never used, but still takes a stack slot */

    if (gMenuPad.dirsNew & DIR_LEFT) {
        if (sOptSound != OS_SOUND_MODE_MONO) {
            Sound_Play(SE_CHANGE);
        } else if (!Pad_IsRepeat()) {
            Sound_Play(SE_BUZZER);
        }
        sOptSound = OS_SOUND_MODE_MONO;
    }
    if (gMenuPad.dirsNew & DIR_RIGHT) {
        if (sOptSound != OS_SOUND_MODE_STEREO) {
            Sound_Play(SE_CHANGE);
        } else if (!Pad_IsRepeat()) {
            Sound_Play(SE_BUZZER);
        }
        sOptSound = OS_SOUND_MODE_STEREO;
    }
    if (gMenuPad.buttonDown & PAD_A) {
        if (sOptSound != OSGetSoundMode()) {
            OSSetSoundMode(sOptSound);
            Sound_SetOutputMode(sOptSound);
        }
        Sound_Play(SE_CONFIRM);
        sOptSub = OPT_MENU;
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        sOptSound = OSGetSoundMode();
        sOptSub = OPT_MENU;
    }
    return MENU_OPTION;
}

/* 0x813283A0 */
s32 Option_UpdateOffset(void) {
    u8 unused[4]; /* never used, but still takes a stack slot */
    OSSram* sram;

    if (gMenuPad.dirsNew & DIR_LEFT) {
        sOptOffset--;
        if (sOptOffset < -32) {
            sOptOffset = -32;
            if (!Pad_IsRepeat()) {
                Sound_Play(SE_BUZZER);
            }
        } else {
            Sound_Play(SE_CHANGE);
        }
        __VISetAdjustingValues(sOptOffset, 0);
    }
    if (gMenuPad.dirsNew & DIR_RIGHT) {
        sOptOffset++;
        if (sOptOffset > 32) {
            sOptOffset = 32;
            if (!Pad_IsRepeat()) {
                Sound_Play(SE_BUZZER);
            }
        } else {
            Sound_Play(SE_CHANGE);
        }
        __VISetAdjustingValues(sOptOffset, 0);
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_CONFIRM);
        sram = __OSLockSram();
        sram->displayOffsetH = sOptOffset;
        __OSUnlockSram(TRUE);
        while (!__OSSyncSram()) {
        }
        sOptSub = OPT_MENU;
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        sram = __OSLockSram();
        sOptOffset = sram->displayOffsetH;
        __OSUnlockSram(FALSE);
        __VISetAdjustingValues(sOptOffset, 0);
        sOptSub = OPT_MENU;
    }
    return MENU_OPTION;
}

/* 0x81328510 */
s32 Option_UpdateLang(void) {
    u8 unused[4]; /* never used, but still takes a stack slot */

    if (gMenuPad.dirsNew & DIR_LEFT) {
        sOptLang--;
        if (sOptLang < 0) {
            sOptLang = 0;
            if (!Pad_IsRepeat()) {
                Sound_Play(SE_BUZZER);
            }
        } else {
            Sound_Play(SE_CHANGE);
        }
        Menu_SetGuideLang(sOptLang);
    }
    if (gMenuPad.dirsNew & DIR_RIGHT) {
        sOptLang++;
        if (sOptLang > 5) {
            sOptLang = 5;
            if (!Pad_IsRepeat()) {
                Sound_Play(SE_BUZZER);
            }
        } else {
            Sound_Play(SE_CHANGE);
        }
        Menu_SetGuideLang(sOptLang);
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_CONFIRM);
        Lang_Set(sOptLang);
        sOptSub = OPT_MENU;
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        sOptLang = Lang_Get();
        Menu_SetGuideLang(sOptLang);
        sOptSub = OPT_MENU;
    }
    return MENU_OPTION;
}

/* 0x81328648: state MENU_OPTION. Returns the next menu state. */
s32 Option_Update(void) {
    s32 next = MENU_OPTION;
    Scene* scene = Scene_Get();
    s32 i;

    sOptPrevSub = sOptSub;
    if (!SCENE_FATAL_ERROR(scene)) {
        switch (sOptPrevSub) {
        case OPT_MENU:
            next = Option_UpdateMenu();
            break;
        case OPT_SOUND:
            next = Option_UpdateSound();
            break;
        case OPT_OFFSET:
            next = Option_UpdateOffset();
            break;
        case OPT_LANG:
            next = Option_UpdateLang();
            break;
        }
    }
    if (OptionIcon_IsSettled()) {
        FxOption_Update(sOptPrevSub, sOptRow, sOptOffset, sOptSound);
        Fx_Update();
    }
    for (i = 0; i < 6; i++) {
        Fader_Step(&sOptLangName[i], i == sOptLang ? FADE_IN : FADE_HALF);
        Fader_Step(&sOptLangText[i], i == sOptLang ? FADE_IN : FADE_OUT);
    }

    /* pulse of the selection highlight, 50..120 */
    sOptPulse += sOptPulseStep;
    if (sOptPulse > 120) {
        sOptPulseStep = -2;
    }
    if (sOptPulse < 50) {
        sOptPulseStep = 2;
    }
    return next;
}

/* 0x813287D4 */
BOOL Option_IsTop(void) {
    if (sOptPrevSub == OPT_MENU) {
        return TRUE;
    }
    return FALSE;
}

/* 0x813287F0: y of the language currently browsed (for the fx cursor). */
s16 Option_GetLangRow(void) {
    return sOptLangY[sOptLang];
}

/* 0x8132880C */
s16 Option_GetLang(void) {
    return sOptLang;
}

/* 0x81328814: item titles (cross-faded between languages) and the
   language list. */
void Option_DrawText(u8 alpha) {
    OptionParams* p = Option_GetParams();
    GXColor white = sOptWhite;
    GXColor green;
    s16 a;
    s16 b;
    s32 i;

    green.r = p->textColor[0];
    green.g = p->textColor[1];
    green.b = p->textColor[2];
    GX_Setup2DText();
    for (i = 0; i < 6; i++) {
        Fader_GetAlpha(&sOptLangText[i], &a, NULL);
        green.a = white.a = (u8)(alpha * a / 255);
        Glh_DrawText(0, sOptSth[i], sOptGlh, &white); /* "Choose settings" */
        Glh_DrawStringOfs(1, sOptSth[i], sOptGlh, &green, 0, (s16)(16.0f * p->itemOfs[0]));
        Glh_DrawStringOfs(2, sOptSth[i], sOptGlh, &green, 0, (s16)(16.0f * p->itemOfs[1]));
        Glh_DrawStringOfs(3, sOptSth[i], sOptGlh, &green, 0, (s16)(16.0f * p->itemOfs[2]));
    }

    GX_Setup2DTex();
    for (i = 0; i < 6; i++) {
        Fader_GetAlpha(&sOptLangName[i], &b, NULL);
        white.a = (u8)(alpha * b / 255);
        Glh_DrawPaneOfs('lan1' + i, sOptGlh, &white, 0, (s32)(16.0f * p->langOfs));
    }

    /* separators between the six names, half transparent */
    white.a = (u8)((alpha << 7) / 255);
    Glh_DrawPaneOfs('sla1', sOptGlh, &white, 0, (s32)(16.0f * p->langOfs));
    Glh_DrawPaneOfs('sla2', sOptGlh, &white, 0, (s32)(16.0f * p->langOfs));
    Glh_DrawPaneOfs('sla3', sOptGlh, &white, 0, (s32)(16.0f * p->langOfs));
    Glh_DrawPaneOfs('sla4', sOptGlh, &white, 0, (s32)(16.0f * p->langOfs));
    Glh_DrawPaneOfs('sla5', sOptGlh, &white, 0, (s32)(16.0f * p->langOfs));
}

/* 0x81328B20: drawn by Menu_DrawSubmenus while zoomed into Options. */
void Option_Draw(u8 fxAlpha, u8 bgAlpha, u8 textAlpha) {
    DispSettings* disp = Disp_GetSettings();
    SRT srt = { 1.0f, -1.0f, 1.0f, 0, 0, 0, 0, -292.0f, 224.0f, 0.0f };
    Vec up = { 0.0f, 1.0f, 0.0f };
    Vec eye = { 0.0f, 0.0f, 0.0f };
    Vec at = { 0.0f, 0.0f, 0.0f };
    Mtx mv;
    Mtx view;
    Mtx nrm;

    eye.z = 224.0f / tanf(3.141592f * disp->fovy / 360.0f);
    C_MTXLookAt(view, &eye, &up, &at);
    MtxFromSRT(&srt, mv);
    PSMTXConcat(view, mv, mv);
    MtxInverse33(mv, nrm);
    GXLoadPosMtxImm(mv, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    Backdrop_Draw(mv, bgAlpha);
    Option_DrawText(textAlpha);
    if (OptionIcon_IsSettled()) {
        Fx_Draw(fxAlpha);
    }
}
