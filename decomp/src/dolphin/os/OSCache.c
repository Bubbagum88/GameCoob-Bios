#include <dolphin/os.h>
#include <dolphin/base/PPCArch.h>
#include <dolphin/db.h>

#define HID0_ICE 0x00008000
#define HID0_DCE 0x00004000
#define HID2_DCHERR 0x00800000
#define HID2_DNCERR 0x00400000
#define HID2_DCMERR 0x00200000
#define HID2_DQOERR 0x00100000
#define L2CR_L2E 0x80000000
#define L2CR_L2I 0x00200000
#define L2CR_L2IP 0x00000001
#define SRR1_DMA_BIT 0x00200000

asm void DCEnable(void) {
    nofralloc
    sync
    mfspr r3, HID0
    ori r3, r3, 0x4000
    mtspr HID0, r3
    blr
}

asm void DCInvalidateRange(register void* addr, register u32 nBytes) {
    nofralloc
    cmplwi nBytes, 0
    blelr
    clrlwi. r5, addr, 27
    beq @1
    addi nBytes, nBytes, 32
@1
    addi nBytes, nBytes, 31
    srwi nBytes, nBytes, 5
    mtctr nBytes
@2
    dcbi r0, addr
    addi addr, addr, 32
    bdnz @2
    blr
}

asm void DCFlushRange(register void* addr, register u32 nBytes) {
    nofralloc
    cmplwi nBytes, 0
    blelr
    clrlwi. r5, addr, 27
    beq @1
    addi nBytes, nBytes, 32
@1
    addi nBytes, nBytes, 31
    srwi nBytes, nBytes, 5
    mtctr nBytes
@2
    dcbf r0, addr
    addi addr, addr, 32
    bdnz @2
    sc
    blr
}

asm void DCStoreRange(register void* addr, register u32 nBytes) {
    nofralloc
    cmplwi nBytes, 0
    blelr
    clrlwi. r5, addr, 27
    beq @1
    addi nBytes, nBytes, 32
@1
    addi nBytes, nBytes, 31
    srwi nBytes, nBytes, 5
    mtctr nBytes
@2
    dcbst r0, addr
    addi addr, addr, 32
    bdnz @2
    sc
    blr
}

asm void DCFlushRangeNoSync(register void* addr, register u32 nBytes) {
    nofralloc
    cmplwi nBytes, 0
    blelr
    clrlwi. r5, addr, 27
    beq @1
    addi nBytes, nBytes, 32
@1
    addi nBytes, nBytes, 31
    srwi nBytes, nBytes, 5
    mtctr nBytes
@2
    dcbf r0, addr
    addi addr, addr, 32
    bdnz @2
    blr
}

asm void DCZeroRange(register void* addr, register u32 nBytes) {
    nofralloc
    cmplwi nBytes, 0
    blelr
    clrlwi. r5, addr, 27
    beq @1
    addi nBytes, nBytes, 32
@1
    addi nBytes, nBytes, 31
    srwi nBytes, nBytes, 5
    mtctr nBytes
@2
    dcbz r0, addr
    addi addr, addr, 32
    bdnz @2
    blr
}

asm void DCTouchRange(register void* addr, register u32 nBytes) {
    nofralloc
    cmplwi nBytes, 0
    blelr
    clrlwi. r5, addr, 27
    beq @1
    addi nBytes, nBytes, 32
@1
    addi nBytes, nBytes, 31
    srwi nBytes, nBytes, 5
    mtctr nBytes
@2
    dcbt r0, addr
    addi addr, addr, 32
    bdnz @2
    blr
}

asm void ICInvalidateRange(register void* addr, register u32 nBytes) {
    nofralloc
    cmplwi nBytes, 0
    blelr
    clrlwi. r5, addr, 27
    beq @1
    addi nBytes, nBytes, 32
@1
    addi nBytes, nBytes, 31
    srwi nBytes, nBytes, 5
    mtctr nBytes
@2
    icbi r0, addr
    addi addr, addr, 32
    bdnz @2
    sync
    isync
    blr
}


asm void ICFlashInvalidate(void) {
    nofralloc
    mfspr r3, HID0
    ori r3, r3, 0x0800
    mtspr HID0, r3
    blr
}

asm void ICEnable(void) {
    nofralloc
    isync
    mfspr r3, HID0
    ori r3, r3, 0x8000
    mtspr HID0, r3
    blr
}

static inline void L2Disable(void) {
    __sync();
    PPCMtl2cr(PPCMfl2cr() & ~L2CR_L2E);
    __sync();
}

void L2GlobalInvalidate(void) {
    L2Disable();
    PPCMtl2cr(PPCMfl2cr() | L2CR_L2I);
    while (PPCMfl2cr() & L2CR_L2IP)
        ;
    PPCMtl2cr(PPCMfl2cr() & ~L2CR_L2I);
    while (PPCMfl2cr() & L2CR_L2IP) {
        DBPrintf(">>> L2 INVALIDATE : SHOULD NEVER HAPPEN\n");
    }
}

void DMAErrorHandler(OSError error, OSContext* context, ...) {
    u32 hid2;

    hid2 = PPCMfhid2();

    OSReport("Machine check received\n");
    OSReport("HID2 = 0x%x   SRR1 = 0x%x\n", hid2, context->srr1);
    if (!(hid2 & (HID2_DCHERR | HID2_DNCERR | HID2_DCMERR | HID2_DQOERR)) || !(context->srr1 & SRR1_DMA_BIT)) {
        OSReport("Machine check was not DMA/locked cache related\n");
        OSDumpContext(context);
        PPCHalt();
    }

    OSReport("DMAErrorHandler(): An error occurred while processing DMA.\n");
    OSReport("The following errors have been detected and cleared :\n");

    if (hid2 & HID2_DCHERR) {
        OSReport("\t- Requested a locked cache tag that was already in the cache\n");
    }
    if (hid2 & HID2_DNCERR) {
        OSReport("\t- DMA attempted to access normal cache\n");
    }
    if (hid2 & HID2_DCMERR) {
        OSReport("\t- DMA missed in data cache\n");
    }
    if (hid2 & HID2_DQOERR) {
        OSReport("\t- DMA queue overflowed\n");
    }

    PPCMthid2(hid2);
}

void __OSCacheInit(void) {
    if (!(PPCMfhid0() & HID0_ICE)) {
        ICEnable();
        DBPrintf("L1 i-caches initialized\n");
    }
    if (!(PPCMfhid0() & HID0_DCE)) {
        DCEnable();
        DBPrintf("L1 d-caches initialized\n");
    }

    if (!(PPCMfl2cr() & L2CR_L2E)) {
        u32 oldMSR;

        oldMSR = PPCMfmsr();
        __sync();
        PPCMtmsr(0x30);
        __sync();
        L2Disable();
        L2GlobalInvalidate();
        PPCMtmsr(oldMSR);
        PPCMtl2cr((PPCMfl2cr() | L2CR_L2E) & ~L2CR_L2I);
        DBPrintf("L2 cache initialized\n");
    }

    OSSetErrorHandler(OS_ERROR_MACHINE_CHECK, DMAErrorHandler);
    DBPrintf("Locked cache machine check handler installed\n");
}
