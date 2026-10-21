// AD16 (EXI channel 2 debug A/D board), written from the IPL's own code.
#include <dolphin/os.h>
#include <dolphin/exi.h>

#define AD16_CHAN 2
#define AD16_DEV 0
#define AD16_ID 0x04120000
#define AD16_CMD_WRITE_REG 0xA0000000

static BOOL Initialized;

BOOL AD16Init(void) {
    BOOL fail;
    u32 cmd;
    u32 id;

    if (Initialized) {
        return TRUE;
    }
    if (!EXILock(AD16_CHAN, AD16_DEV, NULL)) {
        return FALSE;
    }
    if (!EXISelect(AD16_CHAN, AD16_DEV, EXI_FREQ_1M)) {
        EXIUnlock(AD16_CHAN);
        return FALSE;
    }
    cmd = 0;
    fail = !EXIImm(AD16_CHAN, &cmd, 2, EXI_WRITE, NULL);
    fail |= !EXISync(AD16_CHAN);
    fail |= !EXIImm(AD16_CHAN, &id, 4, EXI_READ, NULL);
    fail |= !EXISync(AD16_CHAN);
    fail |= !EXIDeselect(AD16_CHAN);
    EXIUnlock(AD16_CHAN);

    if (fail || id != AD16_ID) {
        return FALSE;
    }
    Initialized = TRUE;
    return TRUE;
}

BOOL AD16WriteReg(u32 word) {
    BOOL fail;
    u32 cmd;

    if (!Initialized || !EXILock(AD16_CHAN, AD16_DEV, NULL)) {
        return FALSE;
    }
    if (!EXISelect(AD16_CHAN, AD16_DEV, EXI_FREQ_8M)) {
        EXIUnlock(AD16_CHAN);
        return FALSE;
    }
    cmd = AD16_CMD_WRITE_REG;
    fail = !EXIImm(AD16_CHAN, &cmd, 1, EXI_WRITE, NULL);
    fail |= !EXISync(AD16_CHAN);
    fail |= !EXIImm(AD16_CHAN, &word, 4, EXI_WRITE, NULL);
    fail |= !EXISync(AD16_CHAN);
    fail |= !EXIDeselect(AD16_CHAN);
    EXIUnlock(AD16_CHAN);

    if (fail) {
        return FALSE;
    }
    return TRUE;
}

BOOL AD16Probe(void) {
    return Initialized;
}
