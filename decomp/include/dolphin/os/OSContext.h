#ifndef _DOLPHIN_OSCONTEXT_H_
#define _DOLPHIN_OSCONTEXT_H_

#include <dolphin/types.h>

#define OS_CONTEXT_R0 0
#define OS_CONTEXT_R1 4
#define OS_CONTEXT_R2 8
#define OS_CONTEXT_R6 24
#define OS_CONTEXT_GQR1 424
#define OS_CONTEXT_GQR2 428
#define OS_CONTEXT_GQR3 432
#define OS_CONTEXT_GQR4 436
#define OS_CONTEXT_GQR5 440
#define OS_CONTEXT_GQR6 444
#define OS_CONTEXT_GQR7 448

typedef struct OSContext {
    u32 gpr[32];
    u32 cr;
    u32 lr;
    u32 ctr;
    u32 xer;
    f64 fpr[32];
    u32 fpscr_pad;
    u32 fpscr;
    u32 srr0;
    u32 srr1;
    u16 mode;
    u16 state;
    u32 gqr[8];
    u32 psf_pad;
    f64 psf[32];
} OSContext;

u32 OSSaveContext(OSContext* context);
void OSLoadContext(OSContext* context);
void OSClearContext(OSContext* context);
OSContext* OSGetCurrentContext(void);
void OSSetCurrentContext(OSContext* context);
void OSInitContext(OSContext* context, u32 pc, u32 sp);
void OSSaveFPUContext(OSContext* fpucontext);
u32 OSGetStackPointer(void);

#define OS_EXCEPTION_SAVE_GPRS(context)                                                            \
    stw r0, OS_CONTEXT_R0(context);                                                                \
    stw r1, OS_CONTEXT_R1(context);                                                                \
    stw r2, OS_CONTEXT_R2(context);                                                                \
    stmw r6, OS_CONTEXT_R6(context);                                                               \
    mfspr r0, 913;                                                                                 \
    stw r0, OS_CONTEXT_GQR1(context);                                                              \
    mfspr r0, 914;                                                                                 \
    stw r0, OS_CONTEXT_GQR2(context);                                                              \
    mfspr r0, 915;                                                                                 \
    stw r0, OS_CONTEXT_GQR3(context);                                                              \
    mfspr r0, 916;                                                                                 \
    stw r0, OS_CONTEXT_GQR4(context);                                                              \
    mfspr r0, 917;                                                                                 \
    stw r0, OS_CONTEXT_GQR5(context);                                                              \
    mfspr r0, 918;                                                                                 \
    stw r0, OS_CONTEXT_GQR6(context);                                                              \
    mfspr r0, 919;                                                                                 \
    stw r0, OS_CONTEXT_GQR7(context);

#endif
