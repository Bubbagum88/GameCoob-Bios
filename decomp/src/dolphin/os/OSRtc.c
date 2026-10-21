#include <dolphin/os.h>
#include <dolphin/exi.h>

#define SRAM_SIZE 0x40

typedef struct SramControl {
    u8 sram[SRAM_SIZE];
    u32 offset;
    BOOL enabled;
    BOOL locked;
    BOOL sync;
    void (*callback)(void);
} SramControl;

static SramControl Scb ATTRIBUTE_ALIGN(32);

static BOOL GetRTC(u32* rtc);
static BOOL ReadSram(void* buffer);
static void WriteSramCallback(s32 chan, OSContext* context);
static BOOL WriteSram(void* buffer, u32 offset, u32 size);
static void* LockSram(u32 offset);
static BOOL UnlockSram(BOOL commit, u32 offset);

static BOOL GetRTC(u32* rtc) {
    BOOL err;
    u32 cmd;

    if (!EXILock(0, 1, NULL)) {
        return FALSE;
    }
    if (!EXISelect(0, 1, EXI_FREQ_8M)) {
        EXIUnlock(0);
        return FALSE;
    }
    cmd = 0x20000000;
    err = FALSE;
    err |= !EXIImm(0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(0);
    err |= !EXIImm(0, &cmd, 4, EXI_READ, NULL);
    err |= !EXISync(0);
    err |= !EXIDeselect(0);
    EXIUnlock(0);
    *rtc = cmd;
    return !err;
}

BOOL __OSGetRTC(u32* rtc) {
    BOOL err;
    u32 t0;
    u32 t1;
    int i;

    for (i = 0; i < 16; i++) {
        err = FALSE;
        err |= !GetRTC(&t0);
        err |= !GetRTC(&t1);
        if (err) {
            break;
        }
        if (t0 == t1) {
            *rtc = t0;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL __OSSetRTC(u32 rtc) {
    BOOL err;
    u32 cmd;

    if (!EXILock(0, 1, NULL)) {
        return FALSE;
    }
    if (!EXISelect(0, 1, EXI_FREQ_8M)) {
        EXIUnlock(0);
        return FALSE;
    }
    cmd = 0xA0000000;
    err = FALSE;
    err |= !EXIImm(0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(0);
    err |= !EXIImm(0, &rtc, 4, EXI_WRITE, NULL);
    err |= !EXISync(0);
    err |= !EXIDeselect(0);
    EXIUnlock(0);
    return !err;
}

static BOOL ReadSram(void* buffer) {
    BOOL err;
    u32 cmd;

    DCInvalidateRange(buffer, SRAM_SIZE);
    if (!EXILock(0, 1, NULL)) {
        return FALSE;
    }
    if (!EXISelect(0, 1, EXI_FREQ_8M)) {
        EXIUnlock(0);
        return FALSE;
    }
    cmd = 0x20000100;
    err = FALSE;
    err |= !EXIImm(0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(0);
    err |= !EXIDma(0, buffer, SRAM_SIZE, EXI_READ, NULL);
    err |= !EXISync(0);
    err |= !EXIDeselect(0);
    EXIUnlock(0);
    return !err;
}

static void WriteSramCallback(s32 chan, OSContext* context) {
    Scb.sync = WriteSram(&Scb.sram[Scb.offset], Scb.offset, SRAM_SIZE - Scb.offset);
    if (Scb.sync) {
        Scb.offset = SRAM_SIZE;
    }
}

static BOOL WriteSram(void* buffer, u32 offset, u32 size) {
    BOOL err;
    u32 cmd;

    if (!EXILock(0, 1, WriteSramCallback)) {
        return FALSE;
    }
    if (!EXISelect(0, 1, EXI_FREQ_8M)) {
        EXIUnlock(0);
        return FALSE;
    }
    offset <<= 6;
    cmd = (offset + 0x100) | 0xA0000000;
    err = FALSE;
    err |= !EXIImm(0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(0);
    err |= !EXIImmEx(0, buffer, size, EXI_WRITE);
    err |= !EXIDeselect(0);
    EXIUnlock(0);
    return !err;
}

void __OSInitSram(void) {
    Scb.locked = Scb.enabled = FALSE;
    Scb.sync = ReadSram(&Scb);
    Scb.offset = SRAM_SIZE;
}

static void* LockSram(u32 offset) {
    BOOL enabled;

    enabled = OSDisableInterrupts();
    if (Scb.locked) {
        OSRestoreInterrupts(enabled);
        return NULL;
    }
    Scb.enabled = enabled;
    Scb.locked = TRUE;
    return &Scb.sram[offset];
}

OSSram* __OSLockSram(void) {
    return (OSSram*)LockSram(0);
}

OSSramEx* __OSLockSramEx(void) {
    return (OSSramEx*)LockSram(sizeof(OSSram));
}

static BOOL UnlockSram(BOOL commit, u32 offset) {
    u16* p;

    if (commit) {
        if (offset == 0) {
            OSSram* sram = (OSSram*)Scb.sram;

            if ((sram->flags & 3) > 2) {
                sram->flags &= ~3;
            }

            sram->checkSum = sram->checkSumInv = 0;
            for (p = (u16*)&sram->counterBias; p < (u16*)&Scb.sram[sizeof(OSSram)]; p++) {
                sram->checkSum += *p;
                sram->checkSumInv += ~*p;
            }
        }
        if (offset < Scb.offset) {
            Scb.offset = offset;
        }
        Scb.sync = WriteSram(&Scb.sram[Scb.offset], Scb.offset, SRAM_SIZE - Scb.offset);
        if (Scb.sync) {
            Scb.offset = SRAM_SIZE;
        }
    }
    Scb.locked = FALSE;
    OSRestoreInterrupts(Scb.enabled);
    return Scb.sync;
}

BOOL __OSUnlockSram(BOOL commit) {
    UnlockSram(commit, 0);
}

BOOL __OSUnlockSramEx(BOOL commit) {
    UnlockSram(commit, sizeof(OSSram));
}

BOOL __OSSyncSram(void) {
    return Scb.sync;
}

BOOL __OSCheckSram(void) {
    u16* p;
    u16 checkSum;
    u16 checkSumInv;
    OSSram* sram;

    checkSum = checkSumInv = 0;
    sram = (OSSram*)Scb.sram;
    for (p = (u16*)&sram->counterBias; p < (u16*)&Scb.sram[sizeof(OSSram)]; p++) {
        checkSum += *p;
        checkSumInv += ~*p;
    }
    return sram->checkSum == checkSum && sram->checkSumInv == checkSumInv;
}

BOOL __OSReadROM(void* buffer, s32 length, s32 offset) {
    BOOL err;
    u32 cmd;

    DCInvalidateRange(buffer, length);
    if (!EXILock(0, 1, NULL)) {
        return FALSE;
    }
    if (!EXISelect(0, 1, EXI_FREQ_8M)) {
        EXIUnlock(0);
        return FALSE;
    }
    cmd = offset << 6;
    err = FALSE;
    err |= !EXIImm(0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(0);
    err |= !EXIDma(0, buffer, length, EXI_READ, NULL);
    err |= !EXISync(0);
    err |= !EXIDeselect(0);
    EXIUnlock(0);
    return !err;
}

u32 OSGetSoundMode(void) {
    OSSram* sram = __OSLockSram();
    u32 mode = (sram->flags & 4) ? OS_SOUND_MODE_STEREO : OS_SOUND_MODE_MONO;

    __OSUnlockSram(FALSE);
    return mode;
}

void OSSetSoundMode(u32 mode) {
    OSSram* sram;

    mode <<= 2;
    mode &= 4;
    sram = __OSLockSram();
    if (mode == (sram->flags & 4)) {
        __OSUnlockSram(FALSE);
        return;
    }
    sram->flags &= ~4;
    sram->flags |= mode;
    __OSUnlockSram(TRUE);
}

u16 OSGetWirelessID(s32 chan) {
    OSSramEx* sram;
    u16 id;

    sram = __OSLockSramEx();
    id = sram->wirelessPadID[chan];
    __OSUnlockSramEx(FALSE);
    return id;
}

void OSSetWirelessID(s32 chan, u16 id) {
    OSSramEx* sram;

    sram = __OSLockSramEx();
    if (sram->wirelessPadID[chan] != id) {
        sram->wirelessPadID[chan] = id;
        __OSUnlockSramEx(TRUE);
        return;
    }
    __OSUnlockSramEx(FALSE);
}
