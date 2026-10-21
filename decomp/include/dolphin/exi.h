#ifndef _DOLPHIN_EXI_H_
#define _DOLPHIN_EXI_H_

// Based on the Pikmin decomp (CC0) headers OSExpansion.h and exi.h

#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/hw_regs.h>

////////// EXI FUNCTIONS /////////
// Callback function type.
typedef void (*EXICallback)(s32 channel, OSContext* context);

// EXI functions.
BOOL EXIProbe(s32 channel);
s32 EXIProbeEx(s32 channel);

s32 EXIGetType(s32 channel, u32 dev, u32* type);
char* EXIGetTypeString(u32 type);

u32 __OSGetDIConfig(void);
void __OSEnableBarnacle(s32 chan, u32 dev);

//////////////////////////////////

/////////// EXI DEFINES //////////
// Memory card defines.
#define EXI_MEMORY_CARD_59  0x00000004
#define EXI_MEMORY_CARD_123 0x00000008
#define EXI_MEMORY_CARD_251 0x00000010
#define EXI_MEMORY_CARD_507 0x00000020

#define EXI_MEMORY_CARD_1019 0x00000040
#define EXI_MEMORY_CARD_2043 0x00000080

#define EXI_MEMORY_CARD_1019A 0x00000140
#define EXI_MEMORY_CARD_1019B 0x00000240
#define EXI_MEMORY_CARD_1019C 0x00000340
#define EXI_MEMORY_CARD_1019D 0x00000440
#define EXI_MEMORY_CARD_1019E 0x00000540
#define EXI_MEMORY_CARD_1019F 0x00000640
#define EXI_MEMORY_CARD_1019G 0x00000740

#define EXI_MEMORY_CARD_2043A 0x00000180
#define EXI_MEMORY_CARD_2043B 0x00000280
#define EXI_MEMORY_CARD_2043C 0x00000380
#define EXI_MEMORY_CARD_2043D 0x00000480
#define EXI_MEMORY_CARD_2043E 0x00000580
#define EXI_MEMORY_CARD_2043F 0x00000680
#define EXI_MEMORY_CARD_2043G 0x00000780

// Other external defines.
#define EXI_USB_ADAPTER 0x01010000
#define EXI_NPDP_GDEV   0x01020000

#define EXI_MODEM         0x02020000
#define EXI_ETHER         0x04020200
#define EXI_ETHER_VIEWER  0x04220001
#define EXI_STREAM_HANGER 0x04130000

#define EXI_MARLIN 0x03010000

#define EXI_IS_VIEWER 0x05070000

#define EXI_AD16 0x04120000

// Freq defines.
#define EXI_FREQ_1M  0
#define EXI_FREQ_2M  1
#define EXI_FREQ_4M  2
#define EXI_FREQ_8M  3
#define EXI_FREQ_16M 4
#define EXI_FREQ_32M 5

// Read/write.
#define EXI_READ  0
#define EXI_WRITE 1

// EXI status.
#define EXI_STATE_IDLE     0x00
#define EXI_STATE_DMA      0x01
#define EXI_STATE_IMM      0x02
#define EXI_STATE_BUSY     (EXI_STATE_DMA | EXI_STATE_IMM)
#define EXI_STATE_SELECTED 0x04
#define EXI_STATE_ATTACHED 0x08
#define EXI_STATE_LOCKED   0x10

//////////////////////////////////

/////////// EXI DEFINES //////////
// Useful numbers.
#define EXI_TX       0x800400u
#define EXI_MAGIC    0xa5ff005a
#define EXI_MAX_DEV  3
#define EXI_MAX_CHAN 3
#define EXI_REG_MAX  5

// Useful macros.
#define EXI_0CR(tstart, dma, rw, tlen) ((((u32)(tstart)) << 0) | (((u32)(dma)) << 1) | (((u32)(rw)) << 2) | (((u32)(tlen)) << 4))

#define EXIREG(chan, idx) (__EXIRegs[((chan) * EXI_REG_MAX) + (idx)])

#define EXI_CPR_CS(x)  ((1u << (x)) << 7)
#define EXI_CPR_CLK(x) ((x) << 4)

extern s32 __EXIProbeStartTime[2] : 0x800030C0;

//////////////////////////////////

/////////// EXI STRUCTS //////////
// Struct for handling expansion information (size 0x40).
typedef struct EXIControl {
    EXICallback exiCallback; // _00
    EXICallback tcCallback;  // _04
    EXICallback extCallback; // _08
    vu32 state;              // _0C
    int immLen;              // _10
    u8* immBuf;              // _14
    u32 dev;                 // _18
    int items; // _24
    struct {
        u32 dev;
        EXICallback callback;
    } queue[EXI_MAX_DEV]; // _28
} EXIControl;

//////////////////////////////////

////////// EXI FUNCTIONS /////////
void EXIInit(void);

BOOL EXIImm(s32 channel, void* buffer, s32 length, u32 type, EXICallback callback);
BOOL EXIImmEx(s32 channel, void* buffer, s32 length, u32 type);
BOOL EXIDma(s32 channel, void* buffer, s32 length, u32 type, EXICallback callback);
BOOL EXISync(s32 channel);
u32 EXIClearInterrupts(s32 channel, BOOL clearExiBit, BOOL clearTcBit, BOOL clearExtBit);
EXICallback EXISetExiCallback(s32 channel, EXICallback callback);
BOOL EXIAttach(s32 channel, EXICallback callback);
BOOL EXIDetach(s32 channel);
BOOL EXISelect(s32 channel, u32 device, u32 frequency);
BOOL EXIDeselect(s32 channel);

BOOL EXILock(s32 channel, u32 device, EXICallback callback);
BOOL EXIUnlock(s32 channel);
u32 EXIGetState(s32 channel);
s32 EXIGetID(s32 channel, u32 device, u32* id);

//////////////////////////////////

#endif
