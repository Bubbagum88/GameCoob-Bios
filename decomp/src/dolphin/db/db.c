// Based on the Pikmin decomp (CC0) db.c
#include <dolphin/db.h>
#include <dolphin/base/PPCArch.h>
#include <dolphin/os.h>
#include <dolphin/os/OSUtil.h>

#define OS_CURRENTCONTEXT_PADDR 0x000000C0
#define MSR_IR 0x00000020
#define MSR_DR 0x00000010

DBInterface* __DBInterface = NULL;
BOOL DBVerbose = FALSE;

void DBInit(void) {
    __DBInterface = (void*)IsDebuggerPresent;
    *(u32*)ExceptionHookDestination = OSCachedToPhysical(__DBExceptionDestination);
    DBVerbose = TRUE;
}

void __DBExceptionDestinationAux(void) {
    OSContext* ctx = OSPhysicalToCached(*(u32*)OS_CURRENTCONTEXT_PADDR);
    int pad[2];
    OSReport("DBExceptionDestination\n");
    OSDumpContext(ctx);
    PPCHalt();
}

ASM void __DBExceptionDestination(void) {
#ifdef __MWERKS__ // clang-format off
    nofralloc
    mfmsr  r3
    ori    r3, r3, MSR_IR | MSR_DR
    mtmsr  r3
    b      __DBExceptionDestinationAux
#endif // clang-format on
}

int __DBIsExceptionMarked(u8 a) {
    return __DBInterface->unk4 & (1 << a);
}

void DBPrintf(const char* format, ...) {
}
