/*
 * BS2 pad input (PAL 1.0 IPL, .text 0x81302688..0x81302DF0).
 *
 * An IPL-modified copy of the SDK's DEMOPad. Structure follows the CC0
 * reconstruction in zeldaret/tp (libs/dolphin/src/demo/DEMOPad.c, credit
 * to the zeldaret contributors); the IPL version adds stick-to-D-pad
 * mapping, key repeat driven by the display settings, an analog
 * magnitude/angle, and a Z-button "easter egg" check.
 */
#include <dolphin/types.h>
#include <dolphin/pad.h>
#include <dolphin/os/OSRtc.h>
#include <math.h>

typedef struct DEMOPadStatus {
    PADStatus pst;       /* 0x00 */
    u16 buttonDown;      /* 0x0C */
    u16 buttonUp;        /* 0x0E */
    u16 dirs;            /* 0x10 */
    u16 dirsNew;         /* 0x12 */
    u16 dirsReleased;    /* 0x14 */
    s16 stickDeltaX;     /* 0x16 */
    s16 stickDeltaY;     /* 0x18 */
    s16 substickDeltaX;  /* 0x1A */
    s16 substickDeltaY;  /* 0x1C */
    s16 angle;           /* 0x1E */
    f32 magnitude;       /* 0x20 */
} DEMOPadStatus;

/* Display settings (0x813010F8); key repeat delay/rate at 0x2E/0x30. */
typedef struct PadDispSettings {
    u8 pad0[0x2E];
    s16 repeatDelay;  /* 0x2E frames before key repeat starts */
    s16 repeatRate;   /* 0x30 frames between repeats */
} PadDispSettings;
PadDispSettings *Disp_GetSettings(void);

void *memset(void *dst, int c, unsigned long n);
double atan2(double y, double x);

#define DIR_UP    0x1000
#define DIR_DOWN  0x2000
#define DIR_LEFT  0x4000
#define DIR_RIGHT 0x8000


/* CW emits these .sbss globals in reverse declaration order. */
s32 lbl_814AD314;    /* connected pad count */
u32 lbl_814AD310;    /* PADInit done */
s32 lbl_814AD30C;    /* repeat counter */
u16 lbl_814AD308;    /* previous dirs */
s32 lbl_814AD304;    /* easter egg: 0 none, 1 pad 0 Z, 2 all four Z */
BOOL lbl_814AD300;   /* key repeat fired this frame */

u32 lbl_81381050[4] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000 };

static PADStatus lbl_814AF5A0[4];  /* raw PADRead */
static PADStatus lbl_814AF5D0[4];  /* last good status per port */
static PADStatus lbl_814AF600;     /* all ports OR-ed */
#define Pad lbl_814AF5A0
#define LastPad lbl_814AF5D0
#define MergedPad lbl_814AF600
DEMOPadStatus gMenuPad;

BOOL fn_81302688(void) {
    BOOL ret = FALSE;
    BOOL commit = FALSE;
    OSSram *sram = __OSLockSram();

    if ((sram->flags & 0x40u) == 0x40u) {
        sram->flags ^= 0x40;
        ret = TRUE;
        commit = TRUE;
    }
    __OSUnlockSram(commit);
    if (commit) {
        while (!__OSSyncSram()) {
        }
    }
    return ret;
}

static inline f32 Pad_CalcMagnitude(PADStatus *pad) {
    f32 m = sqrtf(pad->stickX * pad->stickX + pad->stickY * pad->stickY);
    return m;
}

void fn_81302700(PADStatus *pad, DEMOPadStatus *dp) {
    PadDispSettings *disp = Disp_GetSettings();
    u16 dirs = 0;
    s32 y;

    if ((pad->button & PAD_BUTTON_LEFT) == PAD_BUTTON_LEFT) {
        dirs |= DIR_LEFT;
    }
    if ((pad->button & PAD_BUTTON_RIGHT) == PAD_BUTTON_RIGHT) {
        dirs |= DIR_RIGHT;
    }
    if ((pad->button & PAD_BUTTON_DOWN) == PAD_BUTTON_DOWN) {
        dirs |= DIR_DOWN;
    }
    if ((pad->button & PAD_BUTTON_UP) == PAD_BUTTON_UP) {
        dirs |= DIR_UP;
    }

    lbl_814AD300 = FALSE;
    if (lbl_814AD308 == dirs && dirs != 0) {
        lbl_814AD30C++;
        if (lbl_814AD30C >= disp->repeatDelay) {
            if (lbl_814AD30C == disp->repeatDelay) {
                dp->dirs = 0;
                lbl_814AD300 = TRUE;
            } else if (lbl_814AD30C > disp->repeatDelay + disp->repeatRate) {
                dp->dirs = 0;
                lbl_814AD300 = TRUE;
                lbl_814AD30C -= disp->repeatRate;
            }
        }
    } else {
        lbl_814AD30C = 0;
    }
    lbl_814AD308 = dirs;

    dp->dirsNew = PADButtonDown(dp->dirs, dirs);
    dp->dirsReleased = PADButtonUp(dp->dirs, dirs);
    dp->dirs = dirs;

    dp->buttonDown = PADButtonDown(dp->pst.button, pad->button);
    dp->buttonUp = PADButtonUp(dp->pst.button, pad->button);

    dp->stickDeltaX = pad->stickX - dp->pst.stickX;
    dp->stickDeltaY = pad->stickY - dp->pst.stickY;
    dp->substickDeltaX = pad->substickX - dp->pst.substickX;
    dp->substickDeltaY = pad->substickY - dp->pst.substickY;

    dp->magnitude = Pad_CalcMagnitude(pad);
    if (dp->magnitude > 1.0f) {
        dp->magnitude = 1.0f;
    }
    if (dp->magnitude > 0.0f) {
        y = -pad->stickY;
        dp->angle = 32768.0 * atan2(pad->stickX, y) / 3.1415926;
    }

    if (dp->buttonDown & PAD_BUTTON_B) {
        if (dp->buttonDown & PAD_BUTTON_A) {
            dp->buttonDown ^= PAD_BUTTON_A;
        }
        if (dp->buttonDown & PAD_BUTTON_START) {
            dp->buttonDown ^= PAD_BUTTON_START;
        }
    }

    dp->pst = *pad;
}

void fn_81302A40(PADStatus *pad) {
    if (pad->stickX < -35) {
        pad->button |= PAD_BUTTON_LEFT;
    }
    if (pad->stickX > 35) {
        pad->button |= PAD_BUTTON_RIGHT;
    }
    if (pad->stickY < -35) {
        pad->button |= PAD_BUTTON_DOWN;
    }
    if (pad->stickY > 35) {
        pad->button |= PAD_BUTTON_UP;
    }
    if (pad->substickX < -35) {
        pad->button |= PAD_BUTTON_LEFT;
    }
    if (pad->substickX > 35) {
        pad->button |= PAD_BUTTON_RIGHT;
    }
    if (pad->substickY < -35) {
        pad->button |= PAD_BUTTON_DOWN;
    }
    if (pad->substickY > 35) {
        pad->button |= PAD_BUTTON_UP;
    }
}

void DEMOPadRead(void) {
    s32 i;
    u32 resetMask;

    PADRead(Pad);
    PADClamp(Pad);
    lbl_814AD314 = 0;
    resetMask = 0;
    memset(&MergedPad, 0, sizeof(PADStatus));

    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        if (Pad[i].err == PAD_ERR_NONE) {
            lbl_814AD314++;
            fn_81302A40(&Pad[i]);
            MergedPad.button |= Pad[i].button;
            MergedPad.stickX = 0;
            MergedPad.stickY = 0;
            MergedPad.substickX = 0;
            MergedPad.substickY = 0;
            MergedPad.triggerLeft = 0;
            MergedPad.triggerRight = 0;
            LastPad[i] = Pad[i];
            LastPad[i].err = 1;
        } else if (Pad[i].err == PAD_ERR_NO_CONTROLLER) {
            resetMask |= lbl_81381050[i];
            LastPad[i].err = 0;
        } else if (LastPad[i].err) {
            lbl_814AD314++;
            MergedPad.button |= LastPad[i].button;
            MergedPad.stickX = 0;
            MergedPad.stickY = 0;
            MergedPad.substickX = 0;
            MergedPad.substickY = 0;
            MergedPad.triggerLeft = 0;
            MergedPad.triggerRight = 0;
        }
    }

    if (resetMask) {
        PADReset(resetMask);
    }

    fn_81302700(&MergedPad, &gMenuPad);

    if (Pad[0].err == PAD_ERR_NONE && (Pad[0].button & PAD_TRIGGER_Z) &&
        Pad[1].err == PAD_ERR_NONE && (Pad[1].button & PAD_TRIGGER_Z) &&
        Pad[2].err == PAD_ERR_NONE && (Pad[2].button & PAD_TRIGGER_Z) &&
        Pad[3].err == PAD_ERR_NONE && (Pad[3].button & PAD_TRIGGER_Z)) {
        lbl_814AD304 = 2;
    } else if (Pad[0].err == PAD_ERR_NONE && (Pad[0].button & PAD_TRIGGER_Z)) {
        lbl_814AD304 = 1;
    } else {
        lbl_814AD304 = 0;
    }
}

void DEMOPadInit(void) {
    s32 i;

    if (lbl_814AD310 == 0) {
        PADInit();
        lbl_814AD310++;
    }

    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        LastPad[i].err = 0;
    }

    gMenuPad.pst.button = 0;
    gMenuPad.pst.stickX = 0;
    gMenuPad.pst.stickY = 0;
    gMenuPad.pst.substickX = 0;
    gMenuPad.pst.substickY = 0;
    gMenuPad.pst.triggerLeft = 0;
    gMenuPad.pst.triggerRight = 0;
    gMenuPad.pst.analogA = 0;
    gMenuPad.pst.analogB = 0;
    gMenuPad.pst.err = 0;
    gMenuPad.buttonDown = 0;
    gMenuPad.buttonUp = 0;
    gMenuPad.dirs = 0;
    gMenuPad.dirsNew = 0;
    gMenuPad.dirsReleased = 0;
    gMenuPad.stickDeltaX = 0;
    gMenuPad.stickDeltaY = 0;
    gMenuPad.substickDeltaX = 0;
    gMenuPad.substickDeltaY = 0;
    gMenuPad.angle = 0;
    gMenuPad.magnitude = 0.0f;
}

BOOL Pad_IsRepeat(void) {
    return lbl_814AD300;
}

s32 Pad_GetEasterEgg(void) {
    return lbl_814AD304;
}
