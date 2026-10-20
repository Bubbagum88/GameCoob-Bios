// Non-matching: functionally equivalent decompilation
//
// "Options" submenu (PAL 1.0 BS2, 0x81321730..0x813217F0 and
// 0x81327EE4..0x81328CA4): sound mode, screen position and language.
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
#include "bs2/sound.h"
#include <dolphin/os.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

enum { OPT_MENU = 0, OPT_SOUND = 1, OPT_OFFSET = 2, OPT_LANG = 3 };
enum { OPTROW_SOUND = 0, OPTROW_OFFSET = 1, OPTROW_LANG = 2 };

OptionParams gOptionParamsBuf;      /* 0x814D9F00 */
OptionParams *gOptionParams;        /* 0x814AD510 */

static GlhHeader *sOptGlh;          /* 0x814AD588 layout, res 122 */
static s32 sOptPrevSub;             /* 0x814AD58C sub-state seen by the draw code */
static s32 sOptSub;                 /* 0x814AD590 */
static s16 sOptRow;                 /* 0x814AD594 0..2 */
static s16 sOptOffset;              /* 0x814AD596 screen offset being edited */
static s16 sOptSound;               /* 0x814AD598 0 mono, 1 stereo */
static s16 sOptLang;                /* 0x814AD59A language being browsed */
static s16 sOptPulse;               /* 0x814AD59C 50..120 */
static s16 sOptPulseStep;           /* 0x814AD59E +-2 */

/* 0x814DE518 */
static struct {
    void *sth[6];          /* 0x00 option strings in every language (res 123..128) */
    Fader langName[6];     /* 0x18 'lan1'..'lan6' panes: full when chosen, half otherwise */
    Fader langText[6];     /* 0x78 the item texts in each language (cross-fade) */
    s16 langY[6];          /* 0xD8 y of the 'lan1'..'lan6' panes (pixels) */
} sOpt;

extern GXColor sOptWhite;           /* 0x814AE050 */
extern const SRT sOpt2DSRT;         /* 0x8137EED0 S(1,-1,1) T(-292,224,0) */
extern const Vec sOptCamUp, sOptCamEye, sOptCamAt; /* 0x8137EEF0.. */

static void OptionParams_SetDefaults(OptionParams *p);

/* 0x81321730 */
void Option_InitParams(BOOL first) {
    if (first) {
        gOptionParams = &gOptionParamsBuf;
        OptionParams_SetDefaults(gOptionParams);
    }
    Option_Init(first);
}

/* 0x81321778 */
static void OptionParams_SetDefaults(OptionParams *p) {
    p->tint[0] = -30;
    p->tint[1] = 20;
    p->tint[2] = -50;
    p->tint[3] = 255;
    p->textColor[0] = 0;
    p->textColor[1] = 255;
    p->textColor[2] = 120;
    p->f18 = 2.0f;
    p->s20 = 12;
    p->s1E = 5;
    p->itemOfs[0] = -15.0f;
    p->itemOfs[1] = -4.0f;
    p->itemOfs[2] = 6.0f;
    p->langOfs = 8.0f;
}

/* 0x813217E8 */
OptionParams *Option_GetParams(void) {
    return gOptionParams;
}

/* 0x81327EE4: called after a language change. */
void Option_LoadLangRes(void) {
    sOptLang = Lang_Get();
}

static s16 Glh_FindAnchorY(GlhHeader *glh, u32 tag) {
    GlhAnchor *pane = (GlhAnchor *)((u8 *)glh + glh->anchorOfs);
    s16 y = 0;
    s32 i;

    for (i = 0; i < glh->anchorCount; i++, pane++) {
        if (pane->tag == tag) {
            y = (s16)(pane->y >> 4);
        }
    }
    return y;
}

/* 0x81327F08 */
void Option_Init(BOOL first) {
    s32 i;
    OSSram *sram;

    if (first) {
        sOptGlh = ResGet(Res_OptionLayout());
        for (i = 0; i < 6; i++) {
            sOpt.sth[i] = ResGet(Res_OptionStrings(i));
        }
        for (i = 0; i < 6; i++) {
            sOpt.langY[i] = Glh_FindAnchorY(sOptGlh, 'lan1' + i);
        }
    }
    sOptPrevSub = OPT_MENU;
    sOptSub = OPT_MENU;
    sOptRow = OPTROW_SOUND;
    sOptSound = OSGetSoundMode();
    for (i = 0; i < 6; i++) {
        Fader_Init(&sOpt.langName[i]);
        Fader_Init(&sOpt.langText[i]);
    }
    sram = __OSLockSram();
    sOptOffset = (s8)sram->displayOffsetH;
    __OSUnlockSram(FALSE);
    sOptLang = Lang_Get();
    sOptPulse = 100;
    sOptPulseStep = 2;
}

/* 0x81328104 */
static s32 Option_UpdateMenu(void) {
    MenuWork *menu;
    OSSram *sram;
    s32 next = MENU_OPTION;

    if (!Fx_IsReady(0)) return MENU_OPTION;

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
            sOptOffset = (s8)sram->displayOffsetH;
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
static s32 Option_UpdateSound(void) {
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
        if ((u32)sOptSound != OSGetSoundMode()) {
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
static s32 Option_UpdateOffset(void) {
    OSSram *sram;

    if (gMenuPad.dirsNew & DIR_LEFT) {
        if (--sOptOffset < -32) {
            sOptOffset = -32;
            if (!Pad_IsRepeat()) Sound_Play(SE_BUZZER);
        } else {
            Sound_Play(SE_CHANGE);
        }
        __VISetAdjustingValues(sOptOffset, 0);
    }
    if (gMenuPad.dirsNew & DIR_RIGHT) {
        if (++sOptOffset > 32) {
            sOptOffset = 32;
            if (!Pad_IsRepeat()) Sound_Play(SE_BUZZER);
        } else {
            Sound_Play(SE_CHANGE);
        }
        __VISetAdjustingValues(sOptOffset, 0);
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_CONFIRM);
        sram = __OSLockSram();
        sram->displayOffsetH = (s8)sOptOffset;
        __OSUnlockSram(TRUE);
        while (!__OSSyncSram()) {
        }
        sOptSub = OPT_MENU;
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        sram = __OSLockSram();
        sOptOffset = (s8)sram->displayOffsetH;
        __OSUnlockSram(FALSE);
        __VISetAdjustingValues(sOptOffset, 0);
        sOptSub = OPT_MENU;
    }
    return MENU_OPTION;
}

/* 0x81328510 */
static s32 Option_UpdateLang(void) {
    if (gMenuPad.dirsNew & DIR_LEFT) {
        if (--sOptLang < 0) {
            sOptLang = 0;
            if (!Pad_IsRepeat()) Sound_Play(SE_BUZZER);
        } else {
            Sound_Play(SE_CHANGE);
        }
        Menu_SetGuideLang(sOptLang);
    }
    if (gMenuPad.dirsNew & DIR_RIGHT) {
        if (++sOptLang > 5) {
            sOptLang = 5;
            if (!Pad_IsRepeat()) Sound_Play(SE_BUZZER);
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
    Scene *scene = Scene_Get();
    s32 next = MENU_OPTION;
    s32 i;

    sOptPrevSub = sOptSub;
    if (!SCENE_FATAL_ERROR(scene)) {
        switch (sOptPrevSub) {
        case OPT_MENU:   next = Option_UpdateMenu();   break;
        case OPT_SOUND:  next = Option_UpdateSound();  break;
        case OPT_OFFSET: next = Option_UpdateOffset(); break;
        case OPT_LANG:   next = Option_UpdateLang();   break;
        }
    }
    if (OptionIcon_IsSettled()) {
        FxOption_Update(sOptPrevSub, sOptRow, sOptOffset, sOptSound);
        Fx_Update();
    }
    for (i = 0; i < 6; i++) {
        Fader_Step(&sOpt.langName[i], i == sOptLang ? FADE_IN : FADE_HALF);
        Fader_Step(&sOpt.langText[i], i == sOptLang ? FADE_IN : FADE_OUT);
    }
    /* pulse of the selection highlight, 50..120 */
    sOptPulse += sOptPulseStep;
    if (sOptPulse > 120) sOptPulseStep = -2;
    if (sOptPulse < 50) sOptPulseStep = 2;
    return next;
}

/* 0x813287D4 */
BOOL Option_IsTop(void) {
    return sOptPrevSub == OPT_MENU;
}

/* 0x813287F0: y of the language currently browsed (for the fx cursor). */
s16 Option_GetLangRow(void) {
    return sOpt.langY[sOptLang];
}

/* 0x8132880C */
s16 Option_GetLang(void) {
    return sOptLang;
}

/* 0x81328814: item titles (cross-faded between languages) and the
   language list. */
static void Option_DrawText(u8 alpha) {
    OptionParams *p = Option_GetParams();
    GXColor white = sOptWhite;
    GXColor green;
    s16 a;
    s32 i;

    green.r = (u8)p->textColor[0];
    green.g = (u8)p->textColor[1];
    green.b = (u8)p->textColor[2];
    GX_Setup2DText();
    for (i = 0; i < 6; i++) {
        Fader_GetAlpha(&sOpt.langText[i], &a, NULL);
        green.a = white.a = (u8)(alpha * a / 255);
        Glh_DrawText(0, sOpt.sth[i], sOptGlh, &white);      /* "Choose settings" */
        Glh_DrawStringOfs(1, sOpt.sth[i], sOptGlh, &green, 0, (s16)(16.0f * p->itemOfs[0]));
        Glh_DrawStringOfs(2, sOpt.sth[i], sOptGlh, &green, 0, (s16)(16.0f * p->itemOfs[1]));
        Glh_DrawStringOfs(3, sOpt.sth[i], sOptGlh, &green, 0, (s16)(16.0f * p->itemOfs[2]));
    }

    GX_Setup2DTex();
    for (i = 0; i < 6; i++) {
        Fader_GetAlpha(&sOpt.langName[i], &a, NULL);
        white.a = (u8)(alpha * a / 255);
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
    DispSettings *disp = Disp_GetSettings();
    SRT srt = sOpt2DSRT;
    Vec up = sOptCamUp;
    Vec eye = sOptCamEye;
    Vec at = sOptCamAt;
    Mtx view, mv, nrm;

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
