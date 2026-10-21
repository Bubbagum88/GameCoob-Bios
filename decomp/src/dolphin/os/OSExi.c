// Based on the Pikmin decomp (CC0) EXIBios.c
#include <dolphin/os.h>
#include <dolphin/exi.h>
#include <dolphin/hw_regs.h>
#include <string.h>

#include "__os.h"

// Later SDKs made these static and renamed them __EXIProbe and __EXIAttach
#define __EXIProbe EXIProbe
#define __EXIAttach EXIAttach

#define EXI_0LENGTH_EXILENGTH_MASK 0x03FFFFE0

static EXIControl Ecb[EXI_MAX_CHAN];

static void SetExiInterruptMask(s32 chan, EXIControl* exi) {
    EXIControl* exi2;

    exi2 = &Ecb[2];
    switch (chan) {
    case 0:
    {
        if ((exi->exiCallback == NULL && exi2->exiCallback == NULL) || (exi->state & EXI_STATE_LOCKED)) {
            __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_2_EXI);
        } else {
            __OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_2_EXI);
        }
        break;
    }
    case 1:
    {
        if (exi->exiCallback == NULL || (exi->state & EXI_STATE_LOCKED)) {
            __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXI);
        } else {
            __OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXI);
        }
        break;
    }
    case 2:
    {
        if (__OSGetInterruptHandler(__OS_INTERRUPT_PI_DEBUG) == NULL || (exi->state & EXI_STATE_LOCKED)) {
            __OSMaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
        } else {
            __OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
        }
        break;
    }
    }
}

static void CompleteTransfer(s32 chan) {
    EXIControl* exi;
    u8* buf;
    u32 data;
    int i;
    int len;

    exi = &Ecb[chan];

    if ((exi->state & EXI_STATE_BUSY)) {
        if ((exi->state & EXI_STATE_IMM) && (len = exi->immLen) != 0) {
            buf  = exi->immBuf;
            data = EXIREG(chan, 4);
            for (i = 0; i < len; i++) {
                *buf++ = data >> ((3 - i) * 8);
            }
        }
        exi->state &= ~EXI_STATE_BUSY;
    }
}

BOOL EXIImm(s32 chan, void* buf, s32 len, u32 type, EXICallback callback) {
    EXIControl* exi;
    BOOL enabled;

    exi = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if ((exi->state & EXI_STATE_BUSY) || !(exi->state & EXI_STATE_SELECTED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->tcCallback = callback;
    if (exi->tcCallback) {
        EXIClearInterrupts(chan, FALSE, TRUE, FALSE);
        __OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_TC >> (chan * 3));
    }

    exi->state |= EXI_STATE_IMM;

    if (type != EXI_READ) {
        u32 data;
        int i;

        data = 0;
        for (i = 0; i < len; i++) {
            data |= ((u8*)buf)[i] << ((3 - i) * 8);
        }
        EXIREG(chan, 4) = data;
    }

    exi->immBuf = buf;
    exi->immLen = (type != EXI_WRITE) ? len : 0;

    EXIREG(chan, 3) = EXI_0CR(1, 0, type, len - 1);

    OSRestoreInterrupts(enabled);

    return TRUE;
}

BOOL EXIImmEx(s32 chan, void* buf, s32 len, u32 mode) {
    s32 xLen;

    while (len) {
        xLen = (len < 4) ? len : 4;
        if (!EXIImm(chan, buf, xLen, mode, NULL)) {
            return FALSE;
        }

        if (!EXISync(chan)) {
            return FALSE;
        }

        (u8*)buf += xLen;
        len -= xLen;
    }
    return TRUE;
}

BOOL EXIDma(s32 chan, void* buf, s32 len, u32 type, EXICallback callback) {
    EXIControl* exi;
    BOOL enabled;

    exi = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if ((exi->state & EXI_STATE_BUSY) || !(exi->state & EXI_STATE_SELECTED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->tcCallback = callback;
    if (exi->tcCallback) {
        EXIClearInterrupts(chan, FALSE, TRUE, FALSE);
        __OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_TC >> (chan * 3));
    }

    exi->state |= EXI_STATE_DMA;

    EXIREG(chan, 1) = (u32)buf & 0x3ffffe0;
    EXIREG(chan, 2) = (u32)len;
    EXIREG(chan, 3) = EXI_0CR(1, 1, type, 0);

    OSRestoreInterrupts(enabled);

    return TRUE;
}

BOOL EXISync(s32 chan) {
    EXIControl* exi;
    BOOL rc;
    BOOL enabled;

    exi = &Ecb[chan];
    rc  = FALSE;

    while (exi->state & EXI_STATE_SELECTED) {
        if (!((EXIREG(chan, 3) & 1) >> 0)) {
            enabled = OSDisableInterrupts();
            if (exi->state & EXI_STATE_SELECTED) {
                CompleteTransfer(chan);
                if (__OSGetDIConfig() != 0xff || exi->immLen != 4 || (EXIREG(chan, 0) & 0x00000070) != (EXI_FREQ_1M << 4)
                    || (EXIREG(chan, 4) != EXI_USB_ADAPTER)) {
                    rc = TRUE;
                }
            }
            OSRestoreInterrupts(enabled);
            break;
        }
    }
    return rc;
}

u32 EXIClearInterrupts(s32 chan, BOOL exi, BOOL tc, BOOL ext) {
    u32 cpr;
    u32 prev;

    prev = cpr = EXIREG(chan, 0);
    cpr &= 0x7f5;
    if (exi)
        cpr |= 2;
    if (tc)
        cpr |= 8;
    if (ext)
        cpr |= 0x800;
    EXIREG(chan, 0) = cpr;
    return prev;
}

EXICallback EXISetExiCallback(s32 chan, EXICallback exiCallback) {
    EXIControl* exi;
    EXICallback prev;
    BOOL enabled;

    exi = &Ecb[chan];
    enabled          = OSDisableInterrupts();
    prev             = exi->exiCallback;
    exi->exiCallback = exiCallback;

    if (chan != 2) {
        SetExiInterruptMask(chan, exi);
    } else {
        SetExiInterruptMask(0, &Ecb[0]);
    }

    OSRestoreInterrupts(enabled);
    return prev;
}

void EXIProbeReset() {
    __EXIProbeStartTime[0] = __EXIProbeStartTime[1] = 0;
    __EXIProbe(0);
    __EXIProbe(1);
}

BOOL __EXIProbe(s32 chan) {
    EXIControl* exi;
    BOOL enabled;
    BOOL rc;
    u32 cpr;
    s32 t;

    exi = &Ecb[chan];
    if (chan == 2) {
        return TRUE;
    }

    rc      = TRUE;
    enabled = OSDisableInterrupts();
    cpr     = EXIREG(chan, 0);

    if (!(exi->state & EXI_STATE_ATTACHED)) {
        if (cpr & 0x00000800) {
            EXIClearInterrupts(chan, FALSE, FALSE, TRUE);
            __EXIProbeStartTime[chan] = 0;
        }

        if (cpr & 0x00001000) {
            t = (s32)(OSTicksToMilliseconds(OSGetTime()) / 100) + 1;
            if ((u32)__EXIProbeStartTime[chan] == 0)
            {
                __EXIProbeStartTime[chan] = t;
            }
            if (t - __EXIProbeStartTime[chan] < 300 / 100) {
                rc = FALSE;
            }
        } else {
            __EXIProbeStartTime[chan] = 0;
            rc                        = FALSE;
        }
    } else if (!(cpr & 0x00001000) || (cpr & 0x00000800)) {
        __EXIProbeStartTime[chan] = 0;
        rc                        = FALSE;
    }

    OSRestoreInterrupts(enabled);
    return rc;
}

s32 EXIProbeEx(s32 chan) {
    if (EXIProbe(chan))
        return 1;
    if ((u32)__EXIProbeStartTime[chan] != 0)
        return 0;
    return -1;
}

BOOL __EXIAttach(s32 chan, EXICallback extCallback) {
    EXIControl* exi;
    BOOL enabled;
    BOOL rc;

    exi     = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if ((exi->state & EXI_STATE_ATTACHED))
    {
        OSRestoreInterrupts(enabled);
        rc = FALSE;
    }
    else if (__EXIProbe(chan) == FALSE) {
        OSRestoreInterrupts(enabled);
        rc = FALSE;
    }
    else {
        EXIClearInterrupts(chan, TRUE, FALSE, FALSE);
        exi->extCallback = extCallback;
        __OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXT >> (chan * 3));
        exi->state |= EXI_STATE_ATTACHED;
        OSRestoreInterrupts(enabled);
        rc = TRUE;
    }

    return rc;
}

BOOL EXIDetach(s32 chan) {
    EXIControl* exi;
    BOOL enabled;

    exi = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if (!(exi->state & EXI_STATE_ATTACHED)) {
        OSRestoreInterrupts(enabled);
        return TRUE;
    }
    if ((exi->state & EXI_STATE_LOCKED) && exi->dev == 0) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }
    exi->state &= ~EXI_STATE_ATTACHED;
    __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_0 >> (chan * 3));

    OSRestoreInterrupts(enabled);
    return TRUE;
}

BOOL EXISelect(s32 chan, u32 dev, u32 freq) {
    EXIControl* exi;
    u32 cpr;
    BOOL enabled;

    exi = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if ((exi->state & EXI_STATE_SELECTED)
        || chan != 2
               && (dev == 0 && !(exi->state & EXI_STATE_ATTACHED) && !__EXIProbe(chan) || !(exi->state & EXI_STATE_LOCKED)
                   || (exi->dev != dev))) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state |= EXI_STATE_SELECTED;
    cpr = EXIREG(chan, 0);
    cpr &= 0x405;
    cpr |= EXI_CPR_CS(dev) | EXI_CPR_CLK(freq);
    EXIREG(chan, 0) = cpr;

    if (exi->state & EXI_STATE_ATTACHED) {
        switch (chan) {
        case 0:
        {
            __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXT);
            break;
        }
        case 1:
        {
            __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXT);
            break;
        }
        }
    }

    OSRestoreInterrupts(enabled);
    return TRUE;
}

BOOL EXIDeselect(s32 chan) {
    EXIControl* exi;
    u32 cpr;
    BOOL enabled;

    exi = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if (!(exi->state & EXI_STATE_SELECTED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state &= ~EXI_STATE_SELECTED;
    cpr             = EXIREG(chan, 0);
    EXIREG(chan, 0) = cpr & 0x405;

    if (exi->state & EXI_STATE_ATTACHED) {
        switch (chan) {
        case 0:
        {
            __OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXT);
            break;
        }
        case 1:
        {
            __OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXT);
            break;
        }
        }
    }

    OSRestoreInterrupts(enabled);
    if (chan != 2 && (cpr & EXI_CPR_CS(0))) {
        return __EXIProbe(chan) ? TRUE : FALSE;
    }
    return TRUE;
}

static void EXIIntrruptHandler(__OSInterrupt interrupt, OSContext* context) {
    s32 chan;
    EXIControl* exi;
    EXICallback callback;

    chan = (interrupt - __OS_INTERRUPT_EXI_0_EXI) / 3;
    exi = &Ecb[chan];

    EXIClearInterrupts(chan, TRUE, FALSE, FALSE);
    callback = exi->exiCallback;
    if (callback) {
        callback(chan, context);
    }
}

static void TCIntrruptHandler(__OSInterrupt interrupt, OSContext* context) {
    s32 chan;
    EXIControl* exi;
    EXICallback callback;

    chan = (interrupt - __OS_INTERRUPT_EXI_0_TC) / 3;
    exi = &Ecb[chan];

    __OSMaskInterrupts(OS_INTERRUPTMASK(interrupt));
    EXIClearInterrupts(chan, FALSE, TRUE, FALSE);
    callback = exi->tcCallback;
    if (callback) {
        exi->tcCallback = NULL;
        CompleteTransfer(chan);
        callback(chan, context);
    }
}

static void EXTIntrruptHandler(__OSInterrupt interrupt, OSContext* context) {
    s32 chan;
    EXIControl* exi;
    EXICallback callback;

    chan = (interrupt - __OS_INTERRUPT_EXI_0_EXT) / 3;

    __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_0 >> (chan * 3));
    EXIREG(chan, 0) = 0;
    exi             = &Ecb[chan];
    callback        = exi->extCallback;
    exi->state &= ~EXI_STATE_ATTACHED;
    if (callback) {
        exi->extCallback = NULL;
        callback(chan, context);
    }
}

void EXIInit(void) {
    __OSMaskInterrupts(OS_INTERRUPTMASK_EXI);

    EXIREG(0, 0) = 0;
    EXIREG(1, 0) = 0;
    EXIREG(2, 0) = 0;

    EXIREG(0, 0) = 0x00002000;

    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_0_EXI, EXIIntrruptHandler);
    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_0_TC, TCIntrruptHandler);
    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_0_EXT, EXTIntrruptHandler);
    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_1_EXI, EXIIntrruptHandler);
    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_1_TC, TCIntrruptHandler);
    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_1_EXT, EXTIntrruptHandler);
    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_2_EXI, EXIIntrruptHandler);
    __OSSetInterruptHandler(__OS_INTERRUPT_EXI_2_TC, TCIntrruptHandler);

    if (OSGetConsoleType() & OS_CONSOLE_DEVELOPMENT) {
        EXIProbeReset();
    }
}

BOOL EXILock(s32 chan, u32 dev, EXICallback unlockedCallback) {
    EXIControl* exi;
    BOOL enabled;
    int i;

    exi = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if (exi->state & EXI_STATE_LOCKED) {
        if (unlockedCallback) {
            for (i = 0; i < exi->items; i++) {
                if (exi->queue[i].dev == dev) {
                    OSRestoreInterrupts(enabled);
                    return FALSE;
                }
            }
            exi->queue[exi->items].callback = unlockedCallback;
            exi->queue[exi->items].dev      = dev;
            exi->items++;
        }
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state |= EXI_STATE_LOCKED;
    exi->dev = dev;
    SetExiInterruptMask(chan, exi);

    OSRestoreInterrupts(enabled);
    return TRUE;
}

BOOL EXIUnlock(s32 chan) {
    EXIControl* exi;
    BOOL enabled;
    EXICallback unlockedCallback;

    exi = &Ecb[chan];
    enabled = OSDisableInterrupts();

    if (!(exi->state & EXI_STATE_LOCKED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }
    exi->state &= ~EXI_STATE_LOCKED;
    SetExiInterruptMask(chan, exi);

    if (exi->items > 0) {
        unlockedCallback = exi->queue[0].callback;
        if (--exi->items > 0) {
            memmove(&exi->queue[0], &exi->queue[1], exi->items * sizeof(exi->queue[0]));
        }
        unlockedCallback(chan, 0);
    }

    OSRestoreInterrupts(enabled);
    return TRUE;
}

u32 EXIGetState(s32 chan) {
    EXIControl* exi;

    exi = &Ecb[chan];
    return exi->state;
}

static void UnlockedHandler(s32 chan, OSContext* context) {
    u32 id;
    EXIGetID(chan, 0, &id);
}

s32 EXIGetID(s32 chan, u32 dev, u32* id) {
}
