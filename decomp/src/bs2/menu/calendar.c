// "Calendar" submenu (PAL 1.0 BS2, 0x81328CA4..0x81329890): view and set the
// date and time.
//
// Sub-states (sClockSub):
//   0 CLOCK_MENU  up/down picks "date" or "time" (SE 11), A enters (SE 8),
//                 B zooms back out to the Calendar face (SE 6)
//   1 CLOCK_DATE  edit year / month / day
//   2 CLOCK_TIME  edit hour / minute / second (second starts at 0)
// In 1 and 2, left/right move between fields (SE 10) and down/up change
// the value by -1/+1 (SE 9); A stores the clock (SE 12), B cancels (SE 7).
// Date field order follows Lang_GetCode: for the PAL codes (4..9) "left"
// increases the field index, so the fields read day / month / year from
// left to right; for codes 0 and 2 the order is year / month / day.
// Time fields are always hour / minute / second.

#include "bs2/menu.h"
#include "bs2/scene.h"
#include "bs2/res.h"
#include <math.h>

enum { CLOCK_MENU = 0, CLOCK_DATE = 1, CLOCK_TIME = 2 };
enum { CLOCKROW_DATE = 0, CLOCKROW_TIME = 1 };

static void* sClockGlh;     /* layout, res 14 */
static void* sClockSth;     /* strings, res 15..20 */
static u32 sClockShownSub;  /* sub-state used this frame */
static s32 sClockSub;
static s32 sClockRow;       /* 0 date, 1 time */
static s32 sDateField;      /* 0 year, 1 month, 2 day */
static s32 sTimeField;      /* 0 hour, 1 minute, 2 second */
static s16 sClockPulse;     /* 50..120 */
static s8 sClockPulseStep;  /* +-2 */

ClockEdit gClockEdit;

/* Spelled as a byte offset: with &gClockEdit.now the compiler shares one
 * register between it and the &now.sec store in the time case. */
#define CLOCK_NOW ((OSCalendarTime*)((u8*)&gClockEdit + 0xC))

/* days before each month, normal and leap year */
static s32 sDaysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static s32 sDaysInMonthLeap[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/* Never used by the code, but kept in .rodata ahead of the draw templates. */
static const u32 sUnusedPane[9] = {
    0x61616161, 0x12400B40, 0x19000280, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00200020, 0xFFFF0000, 0x00000000,
};

static const GXColor sClockWhite = { 255, 255, 255, 255 };
static const GXColor sClockYellow = { 255, 255, 100, 150 }; /* rgb replaced by ClockParams.textColor */

/* 0x81328CA4: write the edited date or time to the RTC. The RTC itself is
   not written; SRAM counterBias is set so that RTC + bias = new time. */
void Clock_Commit(void) {
    OSTime t;
    u32 rtc;
    OSSram* sram;

    switch (sClockShownSub) {
    case CLOCK_DATE:
        OSTicksToCalendarTime(OSGetTime(), CLOCK_NOW);
        gClockEdit.now.year = gClockEdit.value[0];
        gClockEdit.now.mon = gClockEdit.value[1];
        gClockEdit.now.mday = gClockEdit.value[2];
        break;
    case CLOCK_TIME:
        OSTicksToCalendarTime(OSGetTime(), CLOCK_NOW);
        gClockEdit.now.hour = gClockEdit.value[0];
        gClockEdit.now.min = gClockEdit.value[1];
        gClockEdit.now.sec = gClockEdit.value[2];
        if (gClockEdit.now.year > 2099) {
            gClockEdit.now.year = 2000;
        }
        break;
    default:
        return;
    }

    t = OSCalendarTimeToTicks(&gClockEdit.now);
    __OSSetTime(t);
    __OSGetRTC(&rtc);
    sram = __OSLockSram();
    sram->counterBias = (u32)OSTicksToSeconds(t) - rtc;
    sram->flags |= 0x20; /* clock has been set */
    __OSUnlockSram(TRUE);
    while (!__OSSyncSram()) {
    }
}

/* 0x81328DD4 */
void Clock_LoadLangRes(void) {
    sClockSth = ResGet(Res_ClockStrings());
}

/* 0x81328DFC */
void Clock_Init(BOOL first) {
    if (first) {
        sClockGlh = ResGet(Res_ClockLayout());
        sClockSth = ResGet(Res_ClockStrings());
    }
    sClockShownSub = CLOCK_MENU;
    sClockSub = CLOCK_MENU;
    sClockRow = CLOCKROW_DATE;
    sDateField = 0;
    sTimeField = 0;
    sClockPulse = 100;
    sClockPulseStep = 2;
}

/* 0x81328E60 */
s32 Clock_UpdateMenu(void) {
    MenuWork* menu;
    s32 next = MENU_CLOCK;

    if (!Fx_IsReady(0)) {
        return MENU_CLOCK;
    }

    if (gMenuPad.dirsNew & DIR_UP) {
        if (sClockRow != CLOCKROW_DATE) {
            Sound_Play(SE_CURSOR);
        }
        sClockRow = CLOCKROW_DATE;
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        if (sClockRow != CLOCKROW_TIME) {
            Sound_Play(SE_CURSOR);
        }
        sClockRow = CLOCKROW_TIME;
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_DECIDE);
        switch (sClockRow) {
        case CLOCKROW_DATE:
            OSTicksToCalendarTime(OSGetTime(), CLOCK_NOW);
            gClockEdit.value[0] = gClockEdit.now.year;
            gClockEdit.value[1] = gClockEdit.now.mon;
            gClockEdit.value[2] = gClockEdit.now.mday;
            sClockSub = CLOCK_DATE;
            break;
        case CLOCKROW_TIME:
            OSTicksToCalendarTime(OSGetTime(), CLOCK_NOW);
            gClockEdit.value[0] = gClockEdit.now.hour;
            gClockEdit.value[1] = gClockEdit.now.min;
            gClockEdit.value[2] = 0;
            sClockSub = CLOCK_TIME;
            break;
        }
    }
    if (gMenuPad.buttonDown & PAD_B) {
        menu = Menu_Get();
        if (menu->zoom == 0x7FFF || menu->inputAlways) {
            Sound_Play(SE_EXIT);
            next = MENU_CLOCK_FACE;
        }
    }
    return next;
}

/* 0x81328FE4: wrap year (2000..2099) and month, keep the day valid. */
void Clock_FixDate(void) {
    s32 days;
    s32 leap;

    if (gClockEdit.value[0] < 2000) {
        gClockEdit.value[0] = 2099;
    }
    if (gClockEdit.value[0] > 2099) {
        gClockEdit.value[0] = 2000;
    }
    if (gClockEdit.value[1] < 0) {
        gClockEdit.value[1] = 11;
    }
    if (gClockEdit.value[1] > 11) {
        gClockEdit.value[1] = 0;
    }
    leap = ((gClockEdit.value[0] % 4 == 0) && (gClockEdit.value[0] % 100 != 0)) || (gClockEdit.value[0] % 400 == 0);
    if (leap) {
        days = sDaysInMonthLeap[gClockEdit.value[1]];
    } else {
        days = sDaysInMonth[gClockEdit.value[1]];
    }
    if (gClockEdit.value[2] > days) {
        if (sDateField == 2) {
            gClockEdit.value[2] = 1;
        } else {
            gClockEdit.value[2] = days;
        }
    }
    if (gClockEdit.value[2] < 1) {
        gClockEdit.value[2] = days;
    }
}
/* Direction in which "left" moves the field cursor. */
static s32 Clock_FieldStep(void) {
    switch (Lang_GetCode()) {
    case 0:
    case 2:
        return 1;
    case 3:
        return -1;
    default: /* every PAL language (codes 4..9) */
        return -1;
    }
}

/* 0x8132912C */
s32 Clock_UpdateDate(void) {
    u8 unused[16]; /* never used, but still takes stack space */

    if ((gMenuPad.dirsNew & DIR_LEFT) && !(gMenuPad.dirsNew & (DIR_UP | DIR_DOWN))) {
        Sound_Play(SE_CURSOR_H);
        sDateField -= Clock_FieldStep();
        if (sDateField < 0) {
            sDateField = 2;
        }
        if (sDateField > 2) {
            sDateField = 0;
        }
    }
    if ((gMenuPad.dirsNew & DIR_RIGHT) && !(gMenuPad.dirsNew & (DIR_UP | DIR_DOWN))) {
        Sound_Play(SE_CURSOR_H);
        sDateField += Clock_FieldStep();
        if (sDateField > 2) {
            sDateField = 0;
        }
        if (sDateField < 0) {
            sDateField = 2;
        }
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        gClockEdit.value[sDateField]--;
        Sound_Play(SE_CHANGE);
        Clock_FixDate();
    }
    if (gMenuPad.dirsNew & DIR_UP) {
        gClockEdit.value[sDateField]++;
        Sound_Play(SE_CHANGE);
        Clock_FixDate();
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_CONFIRM);
        Clock_Commit();
        sClockSub = CLOCK_MENU;
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        sClockSub = CLOCK_MENU;
    }
    return MENU_CLOCK;
}

/* 0x81329338: hours wrap 0..23, minutes and seconds 0..59. */
void Clock_FixTime(void) {
    s32* v = gClockEdit.value;

    switch (sTimeField) {
    case 0:
        v = &v[sTimeField];
        if (*v < 0) {
            *v = 23;
        }
        if (*v > 23) {
            *v = 0;
        }
        break;
    case 1:
        v = &v[sTimeField];
        if (*v < 0) {
            *v = 59;
        }
        if (*v > 59) {
            *v = 0;
        }
        break;
    case 2:
        v = &v[sTimeField];
        if (*v < 0) {
            *v = 59;
        }
        if (*v > 59) {
            *v = 0;
        }
        break;
    }
}
/* 0x81329404 */
s32 Clock_UpdateTime(void) {
    u8 unused[4]; /* never used, but still takes a stack slot */

    if ((gMenuPad.dirsNew & DIR_LEFT) && !(gMenuPad.dirsNew & (DIR_UP | DIR_DOWN))) {
        Sound_Play(SE_CURSOR_H);
        sTimeField--;
        if (sTimeField < 0) {
            sTimeField = 2;
        }
        if (sTimeField > 2) {
            sTimeField = 0;
        }
    }
    if ((gMenuPad.dirsNew & DIR_RIGHT) && !(gMenuPad.dirsNew & (DIR_UP | DIR_DOWN))) {
        Sound_Play(SE_CURSOR_H);
        sTimeField++;
        if (sTimeField > 2) {
            sTimeField = 0;
        }
        if (sTimeField < 0) {
            sTimeField = 2;
        }
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        Sound_Play(SE_CHANGE);
        gClockEdit.value[sTimeField]--;
        Clock_FixTime();
    }
    if (gMenuPad.dirsNew & DIR_UP) {
        Sound_Play(SE_CHANGE);
        gClockEdit.value[sTimeField]++;
        Clock_FixTime();
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_CONFIRM);
        Clock_Commit();
        sClockSub = CLOCK_MENU;
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        sClockSub = CLOCK_MENU;
    }
    return MENU_CLOCK;
}

/* 0x81329590: state MENU_CLOCK. Returns the next menu state. */
s32 Clock_Update(void) {
    s32 next = MENU_CLOCK;
    Scene* scene = Scene_Get();

    sClockShownSub = sClockSub;
    if (!SCENE_FATAL_ERROR(scene)) {
        switch (sClockShownSub) {
        case CLOCK_MENU:
            next = Clock_UpdateMenu();
            break;
        case CLOCK_DATE:
            next = Clock_UpdateDate();
            break;
        case CLOCK_TIME:
            next = Clock_UpdateTime();
            break;
        }
    }
    if (ClockIcon_IsSettled()) {
        FxClock_Update(sClockShownSub, sClockRow, sDateField, sTimeField, &gClockEdit);
        Fx_Update();
    }
    sClockPulse += sClockPulseStep;
    if (sClockPulse > 120) {
        sClockPulseStep = -2;
    }
    if (sClockPulse < 50) {
        sClockPulseStep = 2;
    }
    return next;
}

/* 0x81329680 */
BOOL Clock_IsTop(void) {
    if (sClockShownSub == CLOCK_MENU) {
        return TRUE;
    }
    return FALSE;
}

/* 0x8132969C: drawn by Menu_DrawSubmenus while zoomed into the Calendar. */
void Clock_Draw(u8 fxAlpha, u8 bgAlpha, u8 textAlpha) {
    DispSettings* disp = Disp_GetSettings();
    SRT srt = { 1.0f, -1.0f, 1.0f, 0, 0, 0, 0, -292.0f, 224.0f, 0.0f };
    Vec up = { 0.0f, 1.0f, 0.0f };
    Vec eye = { 0.0f, 0.0f, 0.0f };
    Vec at = { 0.0f, 0.0f, 0.0f };
    Mtx mv;
    Mtx view;
    Mtx nrm;
    GXColor yellow;
    GXColor white;
    ClockParams* p;

    eye.z = 224.0f / tanf(3.141592f * disp->fovy / 360.0f);
    C_MTXLookAt(view, &eye, &up, &at);
    MtxFromSRT(&srt, mv);
    PSMTXConcat(view, mv, mv);
    MtxInverse33(mv, nrm);
    GXLoadPosMtxImm(mv, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    Backdrop_Draw(mv, bgAlpha);

    p = Clock_GetParams();
    white = sClockWhite;
    yellow = sClockYellow;
    white.a = textAlpha;
    yellow.a = textAlpha;
    GX_Setup2DText();
    Glh_DrawText(0, sClockSth, sClockGlh, &white); /* title */
    yellow.r = p->textColor[0];
    yellow.g = p->textColor[1];
    yellow.b = p->textColor[2];
    Glh_DrawText(1, sClockSth, sClockGlh, &yellow); /* row labels */
    Glh_DrawText(2, sClockSth, sClockGlh, &yellow);
    if (ClockIcon_IsSettled()) {
        Fx_Draw(fxAlpha);
    }
}
