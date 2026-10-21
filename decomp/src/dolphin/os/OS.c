#include <dolphin/os.h>
#include <dolphin/db.h>
#include <dolphin/exi.h>
#include <dolphin/hw_regs.h>
#include <dolphin/base/PPCArch.h>
#include <string.h>

#include "__os.h"

// Based on the Pikmin decomp (CC0) OS.c

#define OS_DBINTERFACE_ADDR 0x40

#define OS_EXCEPTIONTABLE_ADDR 0x3000
#define OS_DBJUMPPOINT_ADDR 0x60
#define __OS_EXCEPTION_MAX 15

#define NOP 0x60000000

#define OS_BI2_DEBUG_ADDRESS 0x800000F4
#define DEBUGFLAG_ADDR 0x800030E8
#define OS_DEBUG_ADDRESS_2 0x800030E9
#define HID2_WPE 0x40000000

// Memory region a restarting application asked to keep, set up before the reset
extern void* OS_BOOT_REGION_START : 0x812FDFF0;
extern void* OS_BOOT_REGION_END : 0x812FDFEC;

// Disk header information, a.k.a. BI2
typedef struct BI2Debug {
    /* 0x00 */ int debugMonSize;
    /* 0x04 */ int simMemSize;
    /* 0x08 */ u32 argOffset;
    /* 0x0C */ u32 debugFlag;
    /* 0x10 */ int trackLocation;
    /* 0x14 */ int trackSize;
    /* 0x18 */ u32 countryCode;
    /* 0x1C */ u8 unk1C[4];
    /* 0x20 */ u32 dvdLongFileNameFlag;
    /* 0x24 */ u32 padSpec;
} BI2Debug;

extern u32 __DVDLongFileNameFlag;
extern u32 __PADSpec;

extern u8 __ArenaLo[];
extern char _stack_addr[];
extern u8 __ArenaHi[];

void EnableMetroTRKInterrupts(void);

// dummy entry points to the OS exception vector
void __OSEVStart(void);
void __OSEVEnd(void);
void __OSEVSetNumber(void);
void __DBVECTOR(void);
void __OSDBINTSTART(void);
void __OSDBINTEND(void);
void __OSDBJUMPSTART(void);
void __OSDBJUMPEND(void);

static OSBootInfo* BootInfo;
static vu32* BI2DebugFlag;
static u32* BI2DebugFlagHolder;
static f64 ZeroF = 0.0;
static BOOL AreWeInitialized = FALSE;
static __OSExceptionHandler* OSExceptionTable;
OSTime __OSStartTime;

static void OSExceptionInit(void);
static void OSDefaultExceptionHandler(__OSException exception, OSContext* context);

u32 __OSIsDebuggerPresent(void) {
    return *(u32*)OSPhysicalToCached(OS_DBINTERFACE_ADDR);
}

static asm void __OSInitFPRs(void) {
    nofralloc
    lfd fp0, ZeroF
    fmr fp1, fp0
    fmr fp2, fp0
    fmr fp3, fp0
    fmr fp4, fp0
    fmr fp5, fp0
    fmr fp6, fp0
    fmr fp7, fp0
    fmr fp8, fp0
    fmr fp9, fp0
    fmr fp10, fp0
    fmr fp11, fp0
    fmr fp12, fp0
    fmr fp13, fp0
    fmr fp14, fp0
    fmr fp15, fp0
    fmr fp16, fp0
    fmr fp17, fp0
    fmr fp18, fp0
    fmr fp19, fp0
    fmr fp20, fp0
    fmr fp21, fp0
    fmr fp22, fp0
    fmr fp23, fp0
    fmr fp24, fp0
    fmr fp25, fp0
    fmr fp26, fp0
    fmr fp27, fp0
    fmr fp28, fp0
    fmr fp29, fp0
    fmr fp30, fp0
    fmr fp31, fp0
    blr
}

u32 OSGetConsoleType(void) {
    if (!BootInfo || BootInfo->consoleType == 0) {
        return OS_CONSOLE_ARTHUR;
    }
    return BootInfo->consoleType;
}

void ClearArena(void) {
    void *start, *end;

    if ((u32)(OSGetResetCode() + 0x80000000) != 0U) {
        memset(OSGetArenaLo(), 0U, (char*)OSGetArenaHi() - (char*)OSGetArenaLo());
        return;
    }
    start = OS_BOOT_REGION_START;
    end = OS_BOOT_REGION_END;
    if (OS_BOOT_REGION_START == NULL) {
        memset(OSGetArenaLo(), 0U, (char*)OSGetArenaHi() - (char*)OSGetArenaLo());
        return;
    }

    if (OSGetArenaLo() < start) {
        if (OSGetArenaHi() <= start) {
            memset(OSGetArenaLo(), 0U, (char*)OSGetArenaHi() - (char*)OSGetArenaLo());
            return;
        }
        memset(OSGetArenaLo(), 0U, (char*)start - (char*)OSGetArenaLo());
        if (OSGetArenaHi() > end) {
            memset(end, 0, (char*)OSGetArenaHi() - (char*)end);
        }
    }
}

void OSInit(void) {
    BI2Debug* DebugInfo;
    void* debugArenaLo;
    u32 inputConsoleType;
    u32 tdev;

    if (!AreWeInitialized) {
        AreWeInitialized = TRUE;

        __OSStartTime = __OSGetSystemTime();

        OSDisableInterrupts();

        BI2DebugFlag = 0;
        BootInfo = (OSBootInfo*)OSPhysicalToCached(0);

        __DVDLongFileNameFlag = FALSE;

        DebugInfo = (BI2Debug*)*((u32*)OS_BI2_DEBUG_ADDRESS);

        if (DebugInfo != NULL) {
            BI2DebugFlag = &DebugInfo->debugFlag;
            __PADSpec = (u32)DebugInfo->padSpec;
            *((u8*)DEBUGFLAG_ADDR) = (u8)*BI2DebugFlag;
            *((u8*)OS_DEBUG_ADDRESS_2) = (u8)__PADSpec;
        } else if (BootInfo->arenaHi) {
            BI2DebugFlagHolder = (u32*)*((u8*)DEBUGFLAG_ADDR);
            BI2DebugFlag = (u32*)&BI2DebugFlagHolder;
            __PADSpec = (u32) * ((u8*)OS_DEBUG_ADDRESS_2);
        }

        __DVDLongFileNameFlag = TRUE;

        OSSetArenaLo((BootInfo->arenaLo == NULL) ? (void*)(__ArenaLo) : BootInfo->arenaLo);

        if ((BootInfo->arenaLo == NULL) && (BI2DebugFlag != 0) && (*BI2DebugFlag < 2)) {
            debugArenaLo = (void*)_stack_addr;
            OSSetArenaLo((void*)(((u32)debugArenaLo + 31) & ~31));
        }

        OSSetArenaHi((BootInfo->arenaHi == NULL) ? __ArenaHi : BootInfo->arenaHi);

        OSExceptionInit();
        __OSInitSystemCall();
        OSInitAlarm();
        __OSModuleInit();
        __OSInterruptInit();
        __OSSetInterruptHandler(__OS_INTERRUPT_PI_RSW, __OSResetSWInterruptHandler);
        __OSContextInit();
        __OSCacheInit();
        EXIInit();
        SIInit();
        __OSInitSram();
        __OSThreadInit();
        __OSInitAudioSystem();
        PPCMthid2(PPCMfhid2() & ~HID2_WPE);
        if ((BootInfo->consoleType & OS_CONSOLE_DEVELOPMENT) != 0) {
            BootInfo->consoleType = OS_CONSOLE_DEVHW1;
        } else {
            BootInfo->consoleType = OS_CONSOLE_RETAIL1;
        }
        BootInfo->consoleType += (__PIRegs[11] & 0xF0000000) >> 28;

        OSReport("\nDolphin OS $Revision: 47 $.\n");
        OSReport("Kernel built : %s %s\n", "Sep 27 2001", "15:15:22");
        OSReport("Console Type : ");
        if (BootInfo == NULL || (inputConsoleType = BootInfo->consoleType) == 0) {
            inputConsoleType = OS_CONSOLE_ARTHUR;
        } else {
            inputConsoleType = BootInfo->consoleType;
        }

        inputConsoleType = OSGetConsoleType();
        if ((inputConsoleType & 0x10000000) == OS_CONSOLE_RETAIL) {
            OSReport("Retail %d\n", inputConsoleType);
        } else {
            switch (inputConsoleType) {
            case OS_CONSOLE_EMULATOR:
            {
                OSReport("Mac Emulator\n");
                break;
            }
            case OS_CONSOLE_PC_EMULATOR:
            {
                OSReport("PC Emulator\n");
                break;
            }
            case OS_CONSOLE_ARTHUR:
            {
                OSReport("EPPC Arthur\n");
                break;
            }
            case OS_CONSOLE_MINNOW:
            {
                OSReport("EPPC Minnow\n");
                break;
            }
            default:
            {
                tdev = (u32)inputConsoleType - 0x10000000;
                OSReport("Development HW%d\n", tdev - 3);
                break;
            }
            }
        }

        OSReport("Memory %d MB\n", (u32)BootInfo->memorySize >> 0x14U);
        OSReport("Arena : 0x%x - 0x%x\n", OSGetArenaLo(), OSGetArenaHi());

        if (BI2DebugFlag && ((*BI2DebugFlag) >= 2)) {
            EnableMetroTRKInterrupts();
        }

        ClearArena();
        OSEnableInterrupts();
    }
}

static u32 __OSExceptionLocations[] = {
    0x00000100, 0x00000200, 0x00000300, 0x00000400, 0x00000500, 0x00000600, 0x00000700, 0x00000800,
    0x00000900, 0x00000C00, 0x00000D00, 0x00000F00, 0x00001300, 0x00001400, 0x00001700,
};

static void OSExceptionInit(void) {
    __OSException exception;
    void* destAddr;
    u32* opCodeAddr;
    u32 oldOpCode;
    u8* handlerStart;
    u32 handlerSize;

    opCodeAddr = (u32*)__OSEVSetNumber;
    oldOpCode = *opCodeAddr;
    handlerStart = (u8*)__OSEVStart;
    handlerSize = (u32)((u8*)__OSEVEnd - (u8*)__OSEVStart);

    // Install the DB integrator only if this is the first OSInit to run
    destAddr = OSPhysicalToCached(OS_DBJUMPPOINT_ADDR);
    if (*(u32*)destAddr == 0) {
        DBPrintf("Installing OSDBIntegrator\n");
        memcpy(destAddr, (void*)__OSDBINTSTART, (u32)__OSDBINTEND - (u32)__OSDBINTSTART);
        DCFlushRangeNoSync(destAddr, (u32)__OSDBINTEND - (u32)__OSDBINTSTART);
        __sync();
        ICInvalidateRange(destAddr, (u32)__OSDBINTEND - (u32)__OSDBINTSTART);
    }

    for (exception = 0; exception < __OS_EXCEPTION_MAX; exception++) {
        if (BI2DebugFlag && *BI2DebugFlag >= 2 && __DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d commandeered by TRK\n", exception);
            continue;
        }

        *opCodeAddr = oldOpCode | exception;

        if (__DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d vectored to debugger\n", exception);
            memcpy((void*)__DBVECTOR, (void*)__OSDBJUMPSTART, (u32)__OSDBJUMPEND - (u32)__OSDBJUMPSTART);
        } else {
            u32* ops = (u32*)__DBVECTOR;
            int cb;

            for (cb = 0; cb < (u32)__OSDBJUMPEND - (u32)__OSDBJUMPSTART; cb += sizeof(u32)) {
                *ops++ = NOP;
            }
        }

        destAddr = OSPhysicalToCached(__OSExceptionLocations[(u32)exception]);
        memcpy(destAddr, handlerStart, handlerSize);
        DCFlushRangeNoSync(destAddr, handlerSize);
        __sync();
        ICInvalidateRange(destAddr, handlerSize);
    }

    OSExceptionTable = OSPhysicalToCached(OS_EXCEPTIONTABLE_ADDR);
    for (exception = 0; exception < __OS_EXCEPTION_MAX; exception++) {
        __OSSetExceptionHandler(exception, OSDefaultExceptionHandler);
    }

    *opCodeAddr = oldOpCode;

    DBPrintf("Exceptions initialized...\n");
}

static asm void __OSDBIntegrator(void) {
    nofralloc
entry __OSDBINTSTART
    li r5, OS_DBINTERFACE_ADDR
    mflr r3
    stw r3, 0xC(r5)
    lwz r3, 0x8(r5)
    oris r3, r3, 0x8000
    mtlr r3
    li r3, 0x30
    mtmsr r3
    blr
entry __OSDBINTEND
}

static asm void __OSDBJump(void) {
    nofralloc
entry __OSDBJUMPSTART
    bla OS_DBJUMPPOINT_ADDR
entry __OSDBJUMPEND
}

__OSExceptionHandler __OSSetExceptionHandler(__OSException exception, __OSExceptionHandler handler) {
    __OSExceptionHandler oldHandler;

    oldHandler = OSExceptionTable[exception];
    OSExceptionTable[exception] = handler;
    return oldHandler;
}

__OSExceptionHandler __OSGetExceptionHandler(__OSException exception) {
    return OSExceptionTable[exception];
}

static asm void OSExceptionVector(void) {
    nofralloc

entry __OSEVStart
    // Save r4 into SPRG0
    mtsprg 0, r4

    // Load the current context's physical address into r4
    lwz r4, 0x00C0

    // Save r3 - r5 into the current context
    stw r3, 0xC(r4)
    mfsprg r3, 0
    stw r3, 0x10(r4)
    stw r5, 0x14(r4)

    lhz r3, 0x1A2(r4)
    ori r3, r3, 0x2
    sth r3, 0x1A2(r4)

    mfcr r3
    stw r3, 0x80(r4)
    mflr r3
    stw r3, 0x84(r4)
    mfctr r3
    stw r3, 0x88(r4)
    mfxer r3
    stw r3, 0x8C(r4)
    mfsrr0 r3
    stw r3, 0x198(r4)
    mfsrr1 r3
    stw r3, 0x19C(r4)
    mr r5, r3

entry __DBVECTOR
    nop

    // Set SRR1[IR|DR] to turn address translation on at the next rfi
    mfmsr r3
    ori r3, r3, 0x30
    mtsrr1 r3

    // OSExceptionInit patches the exception number in here
entry __OSEVSetNumber
    addi r3, 0, 0x0000

    // Load the current context's virtual address into r4
    lwz r4, 0xD4

    // Non-recoverable exceptions go straight to the default handler
    rlwinm. r5, r5, 0, 30, 30
    bne recoverable
    addis r5, 0, OSDefaultExceptionHandler@ha
    addi r5, r5, OSDefaultExceptionHandler@l
    mtsrr0 r5
    rfi

recoverable:
    rlwinm r5, r3, 2, 22, 29
    lwz r5, OS_EXCEPTIONTABLE_ADDR(r5)
    mtsrr0 r5
    rfi

entry __OSEVEnd
    nop
}

static asm void OSDefaultExceptionHandler(register __OSException exception, register OSContext* context) {
    nofralloc
    OS_EXCEPTION_SAVE_GPRS(context)
    mfdsisr r5
    mfdar r6
    b __OSUnhandledException
}

void __OSPSInit(void) {
    PPCMthid2(PPCMfhid2() | 0x80000000 | 0x20000000);
    ICFlashInvalidate();
    __sync();
    asm {
        li r3, 0
        mtspr 912, r3
    }
}

u32 __OSGetDIConfig(void) {
    return __DIRegs[9] & 0xFF;
}
