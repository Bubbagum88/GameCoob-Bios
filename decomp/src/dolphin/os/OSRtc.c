// Based on the Pikmin decomp (CC0) OSRtc.c. GetRTC, __OSGetRTC,
// __OSCheckSram and __OSReadROM are stubs there and were written from the
// IPL's own code.
#include <dolphin/os.h>
#include <dolphin/exi.h>

#define RTC_CMD_READ 0x20000000
#define RTC_CMD_WRITE 0xA0000000

#define RTC_SRAM_ADDR 0x00000100
#define RTC_SRAM_SIZE 64

#define RTC_CHAN 0
#define RTC_DEV 1
#define RTC_FREQ 3

typedef void (*SramCallback)(void);

typedef struct SramControlBlock {
    u8 sram[0x40];
    u32 offset;
    BOOL enabled;
    BOOL locked;
    BOOL sync;
    SramCallback callback;
} SramControlBlock;

static BOOL WriteSram(void* buffer, u32 offset, u32 size);

static SramControlBlock Scb ATTRIBUTE_ALIGN(32);

static BOOL GetRTC(u32* rtc) {
    BOOL err;
    u32 cmd;

    if (!EXILock(RTC_CHAN, RTC_DEV, NULL)) {
        return FALSE;
    }
    if (!EXISelect(RTC_CHAN, RTC_DEV, RTC_FREQ)) {
        EXIUnlock(RTC_CHAN);
        return FALSE;
    }

    cmd = RTC_CMD_READ;
    err = FALSE;
    err |= !EXIImm(RTC_CHAN, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHAN);
    err |= !EXIImm(RTC_CHAN, &cmd, 4, EXI_READ, NULL);
    err |= !EXISync(RTC_CHAN);
    err |= !EXIDeselect(RTC_CHAN);
    EXIUnlock(RTC_CHAN);

    *rtc = cmd;
    return !err;
}

BOOL __OSGetRTC(u32* rtc) {
    BOOL err;
    u32 first;
    u32 second;
    int tries;

    // Read twice and only accept a value both reads agree on
    for (tries = 0; tries < 16; tries++) {
        err = !GetRTC(&first);
        err |= !GetRTC(&second);
        if (err) {
            break;
        }
        if (first == second) {
            *rtc = first;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL ReadSram(void* buffer) {
    BOOL err;
    u32 cmd;

    DCInvalidateRange(buffer, RTC_SRAM_SIZE);

    if (!EXILock(RTC_CHAN, RTC_DEV, 0)) {
        return FALSE;
    }
    if (!EXISelect(RTC_CHAN, RTC_DEV, RTC_FREQ)) {
        EXIUnlock(RTC_CHAN);
        return FALSE;
    }

    cmd = RTC_CMD_READ | RTC_SRAM_ADDR;
    err = FALSE;
    err |= !EXIImm(RTC_CHAN, &cmd, 4, 1, NULL);
    err |= !EXISync(RTC_CHAN);
    err |= !EXIDma(RTC_CHAN, buffer, RTC_SRAM_SIZE, 0, NULL);
    err |= !EXISync(RTC_CHAN);
    err |= !EXIDeselect(RTC_CHAN);
    EXIUnlock(RTC_CHAN);

    return !err;
}

static void WriteSramCallback(s32 channel, OSContext* context) {
    Scb.sync = WriteSram(Scb.sram + Scb.offset, Scb.offset, RTC_SRAM_SIZE - Scb.offset);
    if (Scb.sync) {
        Scb.offset = RTC_SRAM_SIZE;
    }
}

static BOOL WriteSram(void* buffer, u32 offset, u32 size) {
    BOOL err;
    u32 cmd;

    if (!EXILock(RTC_CHAN, RTC_DEV, WriteSramCallback)) {
        return FALSE;
    }
    if (!EXISelect(RTC_CHAN, RTC_DEV, RTC_FREQ)) {
        EXIUnlock(RTC_CHAN);
        return FALSE;
    }

    offset <<= 6;
    cmd = RTC_CMD_WRITE | RTC_SRAM_ADDR + offset;
    err = FALSE;
    err |= !EXIImm(RTC_CHAN, &cmd, 4, 1, NULL);
    err |= !EXISync(RTC_CHAN);
    err |= !EXIImmEx(RTC_CHAN, buffer, (s32)size, 1);
    err |= !EXIDeselect(RTC_CHAN);
    EXIUnlock(RTC_CHAN);

    return !err;
}

void __OSInitSram(void) {
    Scb.locked = Scb.enabled = FALSE;
    Scb.sync                 = ReadSram(&Scb);
    Scb.offset               = RTC_SRAM_SIZE;
}

static void* LockSram(u32 offset) {
    BOOL enabled;
    enabled = OSDisableInterrupts();

    if (Scb.locked != FALSE) {
        OSRestoreInterrupts(enabled);
        return NULL;
    }

    Scb.enabled = enabled;
    Scb.locked  = TRUE;

    return Scb.sram + offset;
}

OSSram* __OSLockSram(void) {
    return LockSram(0);
}

OSSramEx* __OSLockSramEx(void) {
    return LockSram(sizeof(OSSram));
}

static BOOL UnlockSram(BOOL commit, u32 offset) {
    u16* p;

    if (commit) {
        if (offset == 0) {
            OSSram* sram = (OSSram*)Scb.sram;

            if (2u < (sram->flags & 3)) {
                sram->flags &= ~3;
            }

            sram->checkSum = sram->checkSumInv = 0;
            for (p = (u16*)&sram->counterBias; p < (u16*)(Scb.sram + sizeof(OSSram)); p++) {
                sram->checkSum += *p;
                sram->checkSumInv += ~*p;
            }
        }

        if (offset < Scb.offset) {
            Scb.offset = offset;
        }

        if (Scb.offset <= 20) {
            OSSramEx* sramEx = (OSSramEx*)(&Scb.sram[20]);
        }

        Scb.sync = WriteSram(Scb.sram + Scb.offset, Scb.offset, RTC_SRAM_SIZE - Scb.offset);
        if (Scb.sync) {
            Scb.offset = RTC_SRAM_SIZE;
        }
    }
    Scb.locked = FALSE;
    OSRestoreInterrupts(Scb.enabled);
    return Scb.sync;
}

BOOL __OSUnlockSram(BOOL commit) {
    return UnlockSram(commit, 0);
}

BOOL __OSUnlockSramEx(BOOL commit) {
    return UnlockSram(commit, sizeof(OSSram));
}

BOOL __OSSyncSram(void) {
    return Scb.sync;
}

BOOL __OSCheckSram(void) {
    OSSram* sram;
    u16* p;
    u16 sum;
    u16 sumInv;

    sum = sumInv = 0;
    sram = (OSSram*)Scb.sram;
    for (p = (u16*)&sram->counterBias; p < (u16*)(Scb.sram + sizeof(OSSram)); p++) {
        sum += *p;
        sumInv += ~*p;
    }
    return sram->checkSum == sum && sram->checkSumInv == sumInv;
}

BOOL __OSReadROM(void* buffer, s32 length, s32 offset) {
    BOOL err;
    u32 cmd;

    DCInvalidateRange(buffer, length);

    if (!EXILock(RTC_CHAN, RTC_DEV, NULL)) {
        return FALSE;
    }
    if (!EXISelect(RTC_CHAN, RTC_DEV, RTC_FREQ)) {
        EXIUnlock(RTC_CHAN);
        return FALSE;
    }

    cmd = (u32)offset << 6;
    err = FALSE;
    err |= !EXIImm(RTC_CHAN, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHAN);
    err |= !EXIDma(RTC_CHAN, buffer, length, EXI_READ, NULL);
    err |= !EXISync(RTC_CHAN);
    err |= !EXIDeselect(RTC_CHAN);
    EXIUnlock(RTC_CHAN);

    return !err;
}

u32 OSGetSoundMode(void) {
    OSSram* sram;
    u32 mode;
    int pad[1];

    sram = LockSram(0);
    mode = (sram->flags & 0x4) ? OS_SOUND_MODE_STEREO : OS_SOUND_MODE_MONO;
    __OSUnlockSram(FALSE);
    return mode;
}

void OSSetSoundMode(u32 mode) {
    OSSram* sram;
    int pad[1];

    mode <<= 2;
    mode &= 4;

    sram = LockSram(0);
    if (mode == (sram->flags & 4)) {
        __OSUnlockSram(FALSE);
        return;
    }

    sram->flags &= ~4;
    sram->flags |= mode;
    __OSUnlockSram(TRUE);
}

// This was moved from Pad.c
u16 OSGetWirelessID(s32 channel) {
    OSSramEx* sram;
    u16 id;

    sram = __OSLockSramEx();
    id   = sram->wirelessPadID[channel];
    __OSUnlockSramEx(FALSE);
    return id;
}

// This was moved from Pad.c
void OSSetWirelessID(s32 channel, u16 id) {
    OSSramEx* sram;

    sram = __OSLockSramEx();
    if (sram->wirelessPadID[channel] != id) {
        sram->wirelessPadID[channel] = id;
        __OSUnlockSramEx(TRUE);
        return;
    }

    __OSUnlockSramEx(FALSE);
}

