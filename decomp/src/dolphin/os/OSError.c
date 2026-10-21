#include <dolphin/os.h>
#include <dolphin/base/PPCArch.h>
#include <stdio.h>

#define OS_ERROR_MAX 16
#define MSR_RI 0x00000002

#define __OS_EXCEPTION_DSI 2
#define __OS_EXCEPTION_ISI 3
#define __OS_EXCEPTION_ALIGNMENT 5
#define __OS_EXCEPTION_PROGRAM 6

OSErrorHandler __OSErrorTable[OS_ERROR_MAX];

void OSReport(const char* msg, ...) {
    va_list marker;

    va_start(marker, msg);
    vprintf(msg, marker);
    va_end(marker);
}

void OSPanic(const char* file, int line, const char* msg, ...) {
    va_list marker;
    u32 i;
    u32* p;

    OSDisableInterrupts();
    va_start(marker, msg);
    vprintf(msg, marker);
    va_end(marker);
    OSReport(" in \"%s\" on line %d.\n", file, line);

    OSReport("\nAddress:      Back Chain    LR Save\n");
    for (i = 0, p = (u32*)OSGetStackPointer(); p && (u32)p != 0xFFFFFFFF && i++ < 16; p = (u32*)*p) {
        OSReport("0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1]);
    }

    PPCHalt();
}

OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler) {
    OSErrorHandler oldHandler;

    oldHandler = __OSErrorTable[error];
    __OSErrorTable[error] = handler;
    return oldHandler;
}

void __OSUnhandledException(__OSException exception, OSContext* context, u32 dsisr, u32 dar) {
    if (!(context->srr1 & MSR_RI)) {
        OSReport("Non-recoverable Exception %d", exception);
    } else {
        if (__OSErrorTable[exception]) {
            __OSErrorTable[exception](exception, context, dsisr, dar);
            OSLoadContext(context);
        }

        if (exception == __OS_EXCEPTION_DECREMENTER) {
            OSLoadContext(context);
        }

        OSReport("Unhandled Exception %d", exception);
    }

    OSReport("\n");
    OSDumpContext(context);
    OSReport("\nDSISR= 0x%08x                   DAR  = 0x%08x\n", dsisr, dar);

    switch (exception) {
    case __OS_EXCEPTION_DSI:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access invalid address 0x%x (read from DAR)\n",
                 context->srr0, dar);
        break;
    case __OS_EXCEPTION_ISI:
        OSReport("\nAttempted to fetch instruction from invalid address 0x%x (read from SRR0)\n", context->srr0);
        break;
    case __OS_EXCEPTION_ALIGNMENT:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access unaligned address 0x%x (read from DAR)\n",
                 context->srr0, dar);
        break;
    case __OS_EXCEPTION_PROGRAM:
        OSReport("\nProgram exception : Possible illegal instruction/operation at or around 0x%x (read from SRR0)\n",
                 context->srr0, dar);
        break;
    }

    PPCHalt();
}
