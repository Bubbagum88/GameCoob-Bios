#ifndef _DOLPHIN_EXI_H_
#define _DOLPHIN_EXI_H_

#include <dolphin/os.h>
#include <dolphin/hw_regs.h>

#define EXI_MEMORY_CARD_59 0x00000004
#define EXI_MEMORY_CARD_123 0x00000008
#define EXI_MEMORY_CARD_251 0x00000010
#define EXI_MEMORY_CARD_507 0x00000020
#define EXI_USB_ADAPTER 0x01010000
#define EXI_NPDP_GDEV 0x01020000
#define EXI_MODEM 0x02020000
#define EXI_ETHER 0x04020200
#define EXI_MIC 0x04060000
#define EXI_AD16 0x04120000
#define EXI_RS232C 0x04040404
#define EXI_MARLIN 0x03010000

#define EXI_READ 0
#define EXI_WRITE 1

#define EXI_FREQ_1M 0
#define EXI_FREQ_2M 1
#define EXI_FREQ_4M 2
#define EXI_FREQ_8M 3
#define EXI_FREQ_16M 4
#define EXI_FREQ_32M 5

#define EXI_MAX_DEV 3
#define EXI_MAX_CHAN 3
#define EXI_REG_MAX 5

#define EXI_STATE_IDLE 0x00
#define EXI_STATE_DMA 0x01
#define EXI_STATE_IMM 0x02
#define EXI_STATE_BUSY (EXI_STATE_DMA | EXI_STATE_IMM)
#define EXI_STATE_SELECTED 0x04
#define EXI_STATE_ATTACHED 0x08
#define EXI_STATE_LOCKED 0x10

#define EXI_0CR(tstart, dma, rw, tlen)                                                             \
    ((((u32)(tstart)) << 0) | (((u32)(dma)) << 1) | (((u32)(rw)) << 2) | (((u32)(tlen)) << 4))

#define EXIREG(chan, idx) (__EXIRegs[((chan) * EXI_REG_MAX) + (idx)])

#define EXI_CPR_CS(x) ((1u << (x)) << 7)
#define EXI_CPR_CLK(x) ((x) << 4)

typedef void (*EXICallback)(s32 chan, OSContext* context);

typedef struct EXIControl {
    /* 0x00 */ EXICallback exiCallback;
    /* 0x04 */ EXICallback tcCallback;
    /* 0x08 */ EXICallback extCallback;
    /* 0x0C */ vu32 state;
    /* 0x10 */ int immLen;
    /* 0x14 */ u8* immBuf;
    /* 0x18 */ u32 dev;
    /* 0x1C */ int items;
    struct {
        u32 dev;
        EXICallback callback;
    } queue[EXI_MAX_DEV];
} EXIControl;

// Time (in 1/10 s units) each port first saw a device, kept in low memory
extern s32 __EXIProbeStartTime[2] : 0x800030C0;

EXICallback EXISetExiCallback(s32 channel, EXICallback callback);

void EXIInit(void);
BOOL EXILock(s32 channel, u32 device, EXICallback callback);
BOOL EXIUnlock(s32 channel);
BOOL EXISelect(s32 channel, u32 device, u32 frequency);
BOOL EXIDeselect(s32 channel);
BOOL EXIImm(s32 channel, void* buffer, s32 length, u32 type, EXICallback callback);
BOOL EXIImmEx(s32 channel, void* buffer, s32 length, u32 type);
BOOL EXIDma(s32 channel, void* buffer, s32 length, u32 type, EXICallback callback);
BOOL EXISync(s32 channel);
u32 EXIClearInterrupts(s32 channel, BOOL clearExiBit, BOOL clearTcBit, BOOL clearExtBit);
BOOL EXIProbe(s32 channel);
s32 EXIProbeEx(s32 channel);
BOOL EXIAttach(s32 channel, EXICallback callback);
BOOL EXIDetach(s32 channel);
u32 EXIGetState(s32 channel);
s32 EXIGetID(s32 channel, u32 device, u32* id);
void EXIProbeReset(void);

#endif
