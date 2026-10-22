#include <dolphin/os.h>
#include <dolphin/db.h>

#define OS_CURRENT_CONTEXT (*(OSContext**)0x800000D4)

extern volatile OSContext* __OSCurrentContext : 0x800000D4;
#define OS_CURRENT_CONTEXT_PHYS (*(OSContext**)0x800000C0)
#define __OSFPUContext (*(OSContext**)0x800000D8)

#define OS_CONTEXT_STATE_FPSAVED 0x01u
#define OS_CONTEXT_STATE_EXC 0x02

extern void __RAS_OSDisableInterrupts_begin(void);
extern void __RAS_OSDisableInterrupts_end(void);

static asm void __OSLoadFPUContext(register u32 dummy, register OSContext* fpucontext) {
    nofralloc
    lhz r5, 0x1A2(fpucontext)
    clrlwi. r5, r5, 31
    beq _return

    lfd fp0, 0x190(fpucontext)
    mtfsf 0xFF, fp0
    mfspr r5, 920
    rlwinm. r5, r5, 3, 31, 31
    beq _regular

    psq_l fp0, 456(fpucontext), 0, 0
    psq_l fp1, 464(fpucontext), 0, 0
    psq_l fp2, 472(fpucontext), 0, 0
    psq_l fp3, 480(fpucontext), 0, 0
    psq_l fp4, 488(fpucontext), 0, 0
    psq_l fp5, 496(fpucontext), 0, 0
    psq_l fp6, 504(fpucontext), 0, 0
    psq_l fp7, 512(fpucontext), 0, 0
    psq_l fp8, 520(fpucontext), 0, 0
    psq_l fp9, 528(fpucontext), 0, 0
    psq_l fp10, 536(fpucontext), 0, 0
    psq_l fp11, 544(fpucontext), 0, 0
    psq_l fp12, 552(fpucontext), 0, 0
    psq_l fp13, 560(fpucontext), 0, 0
    psq_l fp14, 568(fpucontext), 0, 0
    psq_l fp15, 576(fpucontext), 0, 0
    psq_l fp16, 584(fpucontext), 0, 0
    psq_l fp17, 592(fpucontext), 0, 0
    psq_l fp18, 600(fpucontext), 0, 0
    psq_l fp19, 608(fpucontext), 0, 0
    psq_l fp20, 616(fpucontext), 0, 0
    psq_l fp21, 624(fpucontext), 0, 0
    psq_l fp22, 632(fpucontext), 0, 0
    psq_l fp23, 640(fpucontext), 0, 0
    psq_l fp24, 648(fpucontext), 0, 0
    psq_l fp25, 656(fpucontext), 0, 0
    psq_l fp26, 664(fpucontext), 0, 0
    psq_l fp27, 672(fpucontext), 0, 0
    psq_l fp28, 680(fpucontext), 0, 0
    psq_l fp29, 688(fpucontext), 0, 0
    psq_l fp30, 696(fpucontext), 0, 0
    psq_l fp31, 704(fpucontext), 0, 0

_regular:
    lfd fp0, 144(fpucontext)
    lfd fp1, 152(fpucontext)
    lfd fp2, 160(fpucontext)
    lfd fp3, 168(fpucontext)
    lfd fp4, 176(fpucontext)
    lfd fp5, 184(fpucontext)
    lfd fp6, 192(fpucontext)
    lfd fp7, 200(fpucontext)
    lfd fp8, 208(fpucontext)
    lfd fp9, 216(fpucontext)
    lfd fp10, 224(fpucontext)
    lfd fp11, 232(fpucontext)
    lfd fp12, 240(fpucontext)
    lfd fp13, 248(fpucontext)
    lfd fp14, 256(fpucontext)
    lfd fp15, 264(fpucontext)
    lfd fp16, 272(fpucontext)
    lfd fp17, 280(fpucontext)
    lfd fp18, 288(fpucontext)
    lfd fp19, 296(fpucontext)
    lfd fp20, 304(fpucontext)
    lfd fp21, 312(fpucontext)
    lfd fp22, 320(fpucontext)
    lfd fp23, 328(fpucontext)
    lfd fp24, 336(fpucontext)
    lfd fp25, 344(fpucontext)
    lfd fp26, 352(fpucontext)
    lfd fp27, 360(fpucontext)
    lfd fp28, 368(fpucontext)
    lfd fp29, 376(fpucontext)
    lfd fp30, 384(fpucontext)
    lfd fp31, 392(fpucontext)

_return:
    blr
}

static asm void __OSSaveFPUContext(register s32 dummy1, register s32 dummy2, register OSContext* fpucontext) {
    nofralloc
    lhz r3, 0x1A2(fpucontext)
    ori r3, r3, 1
    sth r3, 0x1A2(fpucontext)

    stfd fp0, 144(fpucontext)
    stfd fp1, 152(fpucontext)
    stfd fp2, 160(fpucontext)
    stfd fp3, 168(fpucontext)
    stfd fp4, 176(fpucontext)
    stfd fp5, 184(fpucontext)
    stfd fp6, 192(fpucontext)
    stfd fp7, 200(fpucontext)
    stfd fp8, 208(fpucontext)
    stfd fp9, 216(fpucontext)
    stfd fp10, 224(fpucontext)
    stfd fp11, 232(fpucontext)
    stfd fp12, 240(fpucontext)
    stfd fp13, 248(fpucontext)
    stfd fp14, 256(fpucontext)
    stfd fp15, 264(fpucontext)
    stfd fp16, 272(fpucontext)
    stfd fp17, 280(fpucontext)
    stfd fp18, 288(fpucontext)
    stfd fp19, 296(fpucontext)
    stfd fp20, 304(fpucontext)
    stfd fp21, 312(fpucontext)
    stfd fp22, 320(fpucontext)
    stfd fp23, 328(fpucontext)
    stfd fp24, 336(fpucontext)
    stfd fp25, 344(fpucontext)
    stfd fp26, 352(fpucontext)
    stfd fp27, 360(fpucontext)
    stfd fp28, 368(fpucontext)
    stfd fp29, 376(fpucontext)
    stfd fp30, 384(fpucontext)
    stfd fp31, 392(fpucontext)

    mffs fp0
    stfd fp0, 0x190(fpucontext)
    lfd fp0, 0x90(fpucontext)
    mfspr r3, 920
    rlwinm. r3, r3, 3, 31, 31
    bc 12, 2, _return

    psq_st fp0, 456(fpucontext), 0, 0
    psq_st fp1, 464(fpucontext), 0, 0
    psq_st fp2, 472(fpucontext), 0, 0
    psq_st fp3, 480(fpucontext), 0, 0
    psq_st fp4, 488(fpucontext), 0, 0
    psq_st fp5, 496(fpucontext), 0, 0
    psq_st fp6, 504(fpucontext), 0, 0
    psq_st fp7, 512(fpucontext), 0, 0
    psq_st fp8, 520(fpucontext), 0, 0
    psq_st fp9, 528(fpucontext), 0, 0
    psq_st fp10, 536(fpucontext), 0, 0
    psq_st fp11, 544(fpucontext), 0, 0
    psq_st fp12, 552(fpucontext), 0, 0
    psq_st fp13, 560(fpucontext), 0, 0
    psq_st fp14, 568(fpucontext), 0, 0
    psq_st fp15, 576(fpucontext), 0, 0
    psq_st fp16, 584(fpucontext), 0, 0
    psq_st fp17, 592(fpucontext), 0, 0
    psq_st fp18, 600(fpucontext), 0, 0
    psq_st fp19, 608(fpucontext), 0, 0
    psq_st fp20, 616(fpucontext), 0, 0
    psq_st fp21, 624(fpucontext), 0, 0
    psq_st fp22, 632(fpucontext), 0, 0
    psq_st fp23, 640(fpucontext), 0, 0
    psq_st fp24, 648(fpucontext), 0, 0
    psq_st fp25, 656(fpucontext), 0, 0
    psq_st fp26, 664(fpucontext), 0, 0
    psq_st fp27, 672(fpucontext), 0, 0
    psq_st fp28, 680(fpucontext), 0, 0
    psq_st fp29, 688(fpucontext), 0, 0
    psq_st fp30, 696(fpucontext), 0, 0
    psq_st fp31, 704(fpucontext), 0, 0

_return:
    blr
}

asm void OSSaveFPUContext(register OSContext* fpucontext) {
    nofralloc
    addi r5, fpucontext, 0
    b __OSSaveFPUContext
}

asm void OSSetCurrentContext(register OSContext* context) {
    nofralloc
    addis r4, r0, 0x8000
    stw context, 0xD4(r4)
    clrlwi r5, context, 2
    stw r5, 0xC0(r4)
    lwz r5, 0xD8(r4)
    cmpw r5, context
    bne _disableFPU

    lwz r6, 0x19C(context)
    ori r6, r6, 0x2000
    stw r6, 0x19C(context)
    mfmsr r6
    ori r6, r6, 2
    mtmsr r6
    blr

_disableFPU:
    lwz r6, 0x19C(context)
    rlwinm r6, r6, 0, 19, 17
    stw r6, 0x19C(context)
    mfmsr r6
    rlwinm r6, r6, 0, 19, 17
    ori r6, r6, 2
    mtmsr r6
    isync
    blr
}

OSContext* OSGetCurrentContext(void) {
    return (OSContext*)__OSCurrentContext;
}

asm u32 OSSaveContext(register OSContext* context) {
    nofralloc
    stmw r13, 0x34(context)
    mfspr r0, 913
    stw r0, 0x1A8(context)
    mfspr r0, 914
    stw r0, 0x1AC(context)
    mfspr r0, 915
    stw r0, 0x1B0(context)
    mfspr r0, 916
    stw r0, 0x1B4(context)
    mfspr r0, 917
    stw r0, 0x1B8(context)
    mfspr r0, 918
    stw r0, 0x1BC(context)
    mfspr r0, 919
    stw r0, 0x1C0(context)
    mfcr r0
    stw r0, 0x80(context)
    mflr r0
    stw r0, 0x84(context)
    stw r0, 0x198(context)
    mfmsr r0
    stw r0, 0x19C(context)
    mfctr r0
    stw r0, 0x88(context)
    mfxer r0
    stw r0, 0x8C(context)
    stw r1, 0x4(context)
    stw r2, 0x8(context)
    li r0, 1
    stw r0, 0xC(context)
    li r3, 0
    blr
}

asm void OSLoadContext(register OSContext* context) {
    nofralloc
    lis r4, __RAS_OSDisableInterrupts_begin@ha
    lwz r6, 0x198(context)
    addi r5, r4, __RAS_OSDisableInterrupts_begin@l
    cmplw r6, r5
    blt _notInRAS
    lis r4, __RAS_OSDisableInterrupts_end@ha
    addi r0, r4, __RAS_OSDisableInterrupts_end@l
    cmplw r6, r0
    bgt _notInRAS
    stw r5, 0x198(context)

_notInRAS:
    lwz r0, 0x0(context)
    lwz r1, 0x4(context)
    lwz r2, 0x8(context)

    lhz r4, 0x1A2(context)
    rlwinm. r5, r4, 0, 30, 30
    beq notexc
    rlwinm r4, r4, 0, 31, 29
    sth r4, 0x1A2(context)
    lmw r5, 0x14(context)
    b misc
notexc:
    lmw r13, 0x34(context)
misc:
    lwz r4, 0x1A8(context)
    mtspr 913, r4
    lwz r4, 0x1AC(context)
    mtspr 914, r4
    lwz r4, 0x1B0(context)
    mtspr 915, r4
    lwz r4, 0x1B4(context)
    mtspr 916, r4
    lwz r4, 0x1B8(context)
    mtspr 917, r4
    lwz r4, 0x1BC(context)
    mtspr 918, r4
    lwz r4, 0x1C0(context)
    mtspr 919, r4

    lwz r4, 0x80(context)
    mtcrf 0xFF, r4
    lwz r4, 0x84(context)
    mtlr r4
    lwz r4, 0x88(context)
    mtctr r4
    lwz r4, 0x8C(context)
    mtxer r4

    mfmsr r4
    rlwinm r4, r4, 0, 17, 15
    rlwinm r4, r4, 0, 31, 29
    mtmsr r4

    lwz r4, 0x198(context)
    mtsrr0 r4
    lwz r4, 0x19C(context)
    mtsrr1 r4

    lwz r4, 0x10(context)
    lwz r3, 0xC(context)
    rfi
}

asm u32 OSGetStackPointer(void) {
    nofralloc
    mr r3, r1
    blr
}

void OSClearContext(register OSContext* context) {
    context->mode = 0;
    context->state = 0;
    if (context == __OSFPUContext) {
        __OSFPUContext = NULL;
    }
}

asm void OSInitContext(register OSContext* context, register u32 pc, register u32 newsp) {
    nofralloc
    stw pc, 0x198(context)
    stw newsp, 0x4(context)
    li r11, 0
    ori r11, r11, 0x9032
    stw r11, 0x19C(context)
    li r0, 0
    stw r0, 0x80(context)
    stw r0, 0x8C(context)
    stw r2, 0x8(context)
    stw r13, 0x34(context)
    stw r0, 12(context)
    stw r0, 16(context)
    stw r0, 20(context)
    stw r0, 24(context)
    stw r0, 28(context)
    stw r0, 32(context)
    stw r0, 36(context)
    stw r0, 40(context)
    stw r0, 44(context)
    stw r0, 48(context)
    stw r0, 56(context)
    stw r0, 60(context)
    stw r0, 64(context)
    stw r0, 68(context)
    stw r0, 72(context)
    stw r0, 76(context)
    stw r0, 80(context)
    stw r0, 84(context)
    stw r0, 88(context)
    stw r0, 92(context)
    stw r0, 96(context)
    stw r0, 100(context)
    stw r0, 104(context)
    stw r0, 108(context)
    stw r0, 112(context)
    stw r0, 116(context)
    stw r0, 120(context)
    stw r0, 124(context)
    stw r0, 420(context)
    stw r0, 424(context)
    stw r0, 428(context)
    stw r0, 432(context)
    stw r0, 436(context)
    stw r0, 440(context)
    stw r0, 444(context)
    stw r0, 448(context)
    b OSClearContext
}

void OSDumpContext(OSContext* context) {
    u32 i;
    u32* p;

    OSReport("------------------------- Context 0x%08x -------------------------\n", context);

    for (i = 0; i < 16; ++i) {
        OSReport("r%-2d  = 0x%08x (%14d)  r%-2d  = 0x%08x (%14d)\n", i, context->gpr[i], context->gpr[i], i + 16,
                 context->gpr[i + 16], context->gpr[i + 16]);
    }

    OSReport("LR   = 0x%08x                   CR   = 0x%08x\n", context->lr, context->cr);
    OSReport("SRR0 = 0x%08x                   SRR1 = 0x%08x\n", context->srr0, context->srr1);

    OSReport("\nGQRs----------\n");
    for (i = 0; i < 4; ++i) {
        OSReport("gqr%d = 0x%08x \t gqr%d = 0x%08x\n", i, context->gqr[i], i + 4, context->gqr[i + 4]);
    }

    if (context->state & OS_CONTEXT_STATE_FPSAVED) {
        OSContext* currentContext;
        OSContext fpuContext;
        BOOL enabled;

        enabled = OSDisableInterrupts();
        currentContext = OSGetCurrentContext();
        OSClearContext(&fpuContext);
        OSSetCurrentContext(&fpuContext);

        OSReport("\n\nFPRs----------\n");
        for (i = 0; i < 32; i += 2) {
            OSReport("fr%d \t= %d \t fr%d \t= %d\n", i, (u32)context->fpr[i], i + 1, (u32)context->fpr[i + 1]);
        }
        OSReport("\n\nPSFs----------\n");
        for (i = 0; i < 32; i += 2) {
            OSReport("ps%d \t= 0x%x \t ps%d \t= 0x%x\n", i, (u32)context->psf[i], i + 1, (u32)context->psf[i + 1]);
        }

        OSClearContext(&fpuContext);
        OSSetCurrentContext(currentContext);
        OSRestoreInterrupts(enabled);
    }

    OSReport("\nAddress:      Back Chain    LR Save\n");
    for (i = 0, p = (u32*)context->gpr[1]; p && (u32)p != 0xFFFFFFFF && i++ < 16; p = (u32*)*p) {
        OSReport("0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1]);
    }
}

static asm void OSSwitchFPUContext(register __OSException exception, register OSContext* context) {
    nofralloc
    mfmsr r5
    ori r5, r5, 0x2000
    mtmsr r5
    isync
    lwz r5, 0x19C(context)
    ori r5, r5, 0x2000
    mtsrr1 r5
    addis r3, r0, 0x8000
    lwz r5, 0xD8(r3)
    stw context, 0xD8(r3)
    cmpw r5, r4
    beq _restoreAndExit
    cmpwi r5, 0
    beq _loadNewFPUContext
    bl __OSSaveFPUContext
_loadNewFPUContext:
    bl __OSLoadFPUContext
_restoreAndExit:
    lwz r3, 0x80(context)
    mtcrf 0xFF, r3
    lwz r3, 0x84(context)
    mtlr r3
    lwz r3, 0x198(context)
    mtsrr0 r3
    lwz r3, 0x88(context)
    mtctr r3
    lwz r3, 0x8C(context)
    mtxer r3
    lhz r3, 0x1A2(context)
    rlwinm r3, r3, 0, 31, 29
    sth r3, 0x1A2(context)
    lwz r5, 0x14(context)
    lwz r3, 0xC(context)
    lwz r4, 0x10(context)
    rfi
}

void __OSContextInit(void) {
    __OSSetExceptionHandler(7, OSSwitchFPUContext);
    __OSFPUContext = NULL;
    DBPrintf("FPU-unavailable handler installed\n");
}
