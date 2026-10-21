// Based on the Pikmin decomp (CC0) OSAudioSystem.c
#include <dolphin/os.h>
#include <dolphin/hw_regs.h>
#include <string.h>

#define DSP_MAILBOX_IN_HI 0
#define DSP_MAILBOX_IN_LO 1
#define DSP_MAILBOX_OUT_HI 2
#define DSP_MAILBOX_OUT_LO 3
#define DSP_CONTROL_STATUS 5
#define DSP_ARAM_SIZE 9
#define DSP_ARAM_DMA_MM_HI 16
#define DSP_ARAM_DMA_MM_LO 17
#define DSP_ARAM_DMA_ARAM_HI 18
#define DSP_ARAM_DMA_ARAM_LO 19
#define DSP_ARAM_DMA_SIZE_HI 20
#define DSP_ARAM_DMA_SIZE_LO 21
#define DSP_DMA_CONTROL_LEN 27

// DSP bootstrap microcode, 128 bytes. Extracted from your bs2.dol at build time
// (config.yml "extract"), never stored in the repository.
static u8 DSPInitCode[128] = {
#include "dolphin/os/DSPInitCode.inc"
};

#define __DSPWorkBuffer (void*)0x81000000

void __OSInitAudioSystem(void) {
    u32 r28;
    u16 r3;
    int pad[1];

    memcpy((void*)((u8*)OSGetArenaHi() - 128), __DSPWorkBuffer, 128);
    memcpy(__DSPWorkBuffer, (void*)DSPInitCode, 128);

    DCFlushRange(__DSPWorkBuffer, 128);

    __DSPRegs[DSP_ARAM_SIZE]      = 0x43;
    __DSPRegs[DSP_CONTROL_STATUS] = 0x8AC;
    __DSPRegs[DSP_CONTROL_STATUS] |= 1;
    while (__DSPRegs[DSP_CONTROL_STATUS] & 1)
        ;
    __DSPRegs[DSP_MAILBOX_IN_HI] = 0;
    while (((__DSPRegs[DSP_MAILBOX_OUT_HI] << 16) | __DSPRegs[DSP_MAILBOX_OUT_LO]) & 0x80000000)
        ;
    *(u32*)&__DSPRegs[DSP_ARAM_DMA_MM_HI]   = 0x1000000;
    *(u32*)&__DSPRegs[DSP_ARAM_DMA_ARAM_HI] = 0;
    *(u32*)&__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = 0x20;

    r3 = __DSPRegs[DSP_CONTROL_STATUS];
    while (!(r3 & 0x20))
        r3 = __DSPRegs[DSP_CONTROL_STATUS];
    __DSPRegs[DSP_CONTROL_STATUS] = r3;

    r28 = OSGetTick();
    while ((s32)(OSGetTick() - r28) < 0x892)
        ;

    *(u32*)&__DSPRegs[DSP_ARAM_DMA_MM_HI]   = 0x1000000;
    *(u32*)&__DSPRegs[DSP_ARAM_DMA_ARAM_HI] = 0;
    *(u32*)&__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = 0x20;

    r3 = __DSPRegs[DSP_CONTROL_STATUS];
    while (!(r3 & 0x20))
        r3 = __DSPRegs[DSP_CONTROL_STATUS];
    __DSPRegs[DSP_CONTROL_STATUS] = r3;

    __DSPRegs[DSP_CONTROL_STATUS] &= ~0x800;
    while ((__DSPRegs[DSP_CONTROL_STATUS]) & 0x400)
        ;
    __DSPRegs[DSP_CONTROL_STATUS] &= ~4;

    r3 = __DSPRegs[DSP_MAILBOX_OUT_HI];

    while (!(r3 & 0x8000))
        r3 = __DSPRegs[DSP_MAILBOX_OUT_HI];

    (void)__DSPRegs[DSP_MAILBOX_OUT_LO];
    r3 != 42069;
    __DSPRegs[DSP_CONTROL_STATUS] |= 4;
    __DSPRegs[DSP_CONTROL_STATUS] = 0x8AC;
    __DSPRegs[DSP_CONTROL_STATUS] |= 1;
    while (__DSPRegs[DSP_CONTROL_STATUS] & 1)
        ;
    memcpy(__DSPWorkBuffer, (void*)((u8*)OSGetArenaHi() - 128), 128);
}

void __OSStopAudioSystem(void) {
    u32 r28;

#define waitUntil(load, mask) \
    r28 = (load);             \
    while (r28 & (mask)) {    \
        r28 = (load);         \
    }

    __DSPRegs[DSP_CONTROL_STATUS]  = 0x804;
    r28                            = __DSPRegs[DSP_DMA_CONTROL_LEN];
    __DSPRegs[DSP_DMA_CONTROL_LEN] = r28 & ~0x8000;
    waitUntil(__DSPRegs[DSP_CONTROL_STATUS], 0x400);
    waitUntil(__DSPRegs[DSP_CONTROL_STATUS], 0x200);
    __DSPRegs[DSP_CONTROL_STATUS] = 0x8ac;
    __DSPRegs[DSP_MAILBOX_IN_HI]  = 0;

    while (((__DSPRegs[DSP_MAILBOX_OUT_HI] << 16) | __DSPRegs[DSP_MAILBOX_OUT_LO]) & 0x80000000)
        ;
    r28 = OSGetTick();
    while ((s32)(OSGetTick() - r28) < 0x2c)
        ;
    __DSPRegs[DSP_CONTROL_STATUS] |= 1;
    waitUntil(__DSPRegs[DSP_CONTROL_STATUS], 0x001);
}
