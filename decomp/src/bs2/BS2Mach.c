// Disc boot state machine (PAL 1.0 BS2, 0x81300598..0x81300F50).
// The original file name is "BS2Mach.c" (it is passed to OSPanic).
// Reads the disc ID and apploader, runs the apploader, loads the banner and
// finally jumps to the game entry point.

#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/dvd.h>
#include <dolphin/vi.h>
#include <dolphin/pad.h>
#include <dolphin/hw_regs.h>
#include <string.h>

typedef void (*AppInitFunc)(void (*report)(const char* fmt, ...));
typedef BOOL (*AppMainFunc)(void** addr, s32* length, s32* offset);
typedef void* (*AppCloseFunc)(void);
typedef void (*AppEntryFunc)(AppInitFunc* init, AppMainFunc* main, AppCloseFunc* close);

typedef struct AppLoaderHeader {
    char date[16];      /* 0x00 */
    AppEntryFunc entry; /* 0x10 */
    s32 size;           /* 0x14 */
    s32 trailer;        /* 0x18 */
    u8 pad[4];          /* 0x1C */
} AppLoaderHeader;

typedef struct BootWork {
    u8 unk00[0x20];
    AppLoaderHeader header; /* 0x20 */
} BootWork;

extern u32 __DVDDiskMagic : 0x8000001C;
extern u32* __OSBI2 : 0x800000F4;

BOOL AD16Probe(void);
void __OSStopAudioSystem(void);
u32 OSGetConsoleType(void);
void DVDReset(void);
BOOL DVDResetRequired(void);
void __DVDFSInit(void);
BOOL DVDReadAbsAsyncForBS(DVDCommandBlock* block, void* addr, s32 length, s32 offset, DVDCBCallback callback);
void __DVDAudioBufferConfig(DVDCommandBlock* block, u32 enable, u32 size, DVDCBCallback callback);
BOOL DVDChangeDiskAsyncForBS(DVDCommandBlock* block, DVDCBCallback callback);

u32 DVDStep(void);

/* .sbss is laid out in reverse declaration order */
OSTime lbl_814AD2A0;        /* reset start time */
BOOL lbl_814AD298;          /* reset in progress */
u32 lbl_814AD294;
BOOL lbl_814AD290;          /* banner loaded */
volatile u32 lbl_814AD28C;           /* banner buffer size */
void* lbl_814AD288;         /* banner buffer */
volatile s32 lbl_814AD284;
volatile s32 lbl_814AD280;
volatile s32 lbl_814AD27C;
u32* lbl_814AD278;
AppCloseFunc lbl_814AD274;
AppMainFunc lbl_814AD270;
AppInitFunc lbl_814AD26C;
u32 lbl_814AD268;           /* state */

static BootWork lbl_814AE6E0;
DVDCommandBlock lbl_814AE720;

/* 0x81300598 */
void fn_81300598(const char* fmt, ...) {
}

/* 0x813005E8 */
void fn_813005E8(u32 intType) {
    __DIRegs[DI_COVER_STATUS] = 0;
    lbl_814AD268 = 0x11;
}

/* 0x81300600 */
void fn_81300600(u32 intType) {
    lbl_814AD268 = 5;
}

/* 0x8130060C */
void fn_8130060C(s32 result, DVDCommandBlock* block) {
    if (lbl_814AD284) {
        lbl_814AD284 = 0;
        lbl_814AD27C += lbl_814AD280;
        if (result < 0) {
            *lbl_814AD278 = 0;
        }
    }
    DVDLowSetResetCoverCallback(NULL);
    DVDStep();
}

/* 0x81300668 */
void fn_81300668(void) {
    BOOL enabled;

    if (lbl_814AD268 > 5) {
        enabled = OSDisableInterrupts();
        *lbl_814AD278 = 0;
        lbl_814AD27C = 0;
        lbl_814AD268 = 0x19;
        DVDCancelAsync(&lbl_814AE720, NULL);
        OSRestoreInterrupts(enabled);
    }
}

/* 0x813006D4 */
void fn_813006D4(void* buf, u32 size) {
    lbl_814AD288 = buf;
    lbl_814AD28C = size;
}

/* 0x813006E0 */
BOOL fn_813006E0(void) {
    return lbl_814AD290;
}

/* 0x813006E8 */
BOOL fn_813006E8(void) {
    u32 country = __OSBI2[6];

    switch (VIGetTvFormat()) {
    case VI_NTSC:
        if (__VIRegs[0x37] & 2) {
            if (country == 0) {
                return TRUE;
            }
        } else if (country == 1) {
            return TRUE;
        }
        break;
    case VI_PAL:
        if (country == 2) {
            return TRUE;
        }
        break;
    case VI_MPAL:
        if (country == 1) {
            return TRUE;
        }
        break;
    default:
        return FALSE;
    }
    if (country == 3 && AD16Probe()) {
        return TRUE;
    }
    return FALSE;
}

/* 0x813007B8: flush the IPL image out of the caches and jump to the game
 * entry point. Hand-written asm, branches kept exactly as in the binary. */
asm void fn_813007B8(register void* entry, register void* addr, register u32 nblocks) {
    fralloc
    bl ICFlashInvalidate
    sync
    isync
    mtctr nblocks
    li r0, 0
    b check
loop:
    dcbz addr, r0
    dcbf addr, r0
    addi addr, addr, 0x20
    bdnz loop
    b done
done:
    mtlr entry
    blr
check:
    b loop
    frfree
    blr
}

/* The original was built with the peephole optimizer off for these two. */
#pragma peephole off

/* 0x81300830 */
void fn_81300830(void) {
    while (!PADSync()) {
    }
    OSDisableInterrupts();
    __OSStopAudioSystem();
    if (!(OSGetConsoleType() & 0x10000000)) {
        memset((void*)0x80700000, 0, 0xA00000);
        memset((void*)0x81130000, 0, 0xD0000);
    }
    fn_813007B8(lbl_814AD274(), (void*)0x81300000, 0x20000);
}

#pragma peephole on

/* 0x813008B0 */
BOOL fn_813008B0(DVDCommandBlock* block) {
    switch (block->state) {
    case 0:
        return TRUE;
    case 3:
        lbl_814AD268 = 0x11;
        break;
    case 4:
        lbl_814AD268 = 0x12;
        break;
    case 5:
        lbl_814AD268 = 0x13;
        break;
    case -1:
        lbl_814AD268 = 0x17;
        break;
    case 11:
        lbl_814AD268 = 0x18;
        break;
    case 10:
        if (DVDResetRequired()) {
            lbl_814AD268 = 2;
        } else {
            lbl_814AD268 = 3;
        }
        break;
    case 7:
        lbl_814AD268 = 0x16;
        break;
    }
    return FALSE;
}

#pragma peephole off

/* 0x81300968 */
u32 DVDStep(void) {
    u8 unused[8];       /* unreferenced; pads the frame to 0x68 */
    BOOL enabled;
    BOOL enabled2;
    s32 entry;
    void* addr;
    s32 length;
    s32 offset;
    DVDFileInfo fi;

    enabled = OSDisableInterrupts();
    switch (lbl_814AD268) {
    case 0:
        lbl_814AD278 = (u32*)0x800030D4;
        lbl_814AD268 = 1;
    case 1:
        if (__OSGetSystemTime() < OSMillisecondsToTicks(80)) {
            break;
        }
        lbl_814AD268 = 2;
    case 2:
        if (!lbl_814AD298) {
            *lbl_814AD278 = 0;
            lbl_814AD27C = 0;
            lbl_814AD294 = 1;
            DVDLowSetResetCoverCallback(NULL);
            DVDReset();
            lbl_814AD298 = TRUE;
            lbl_814AD2A0 = __OSGetSystemTime();
            break;
        }
        if (__OSGetSystemTime() - lbl_814AD2A0 < OSMillisecondsToTicks((OSTime)1150)) {
            break;
        }
        lbl_814AD298 = FALSE;
        DVDLowSetResetCoverCallback(fn_81300600);
        DVDReset();
        lbl_814AD268 = 3;
    case 3:
        DVDReadDiskID(&lbl_814AE720, (DVDDiskID*)0x80000000, fn_8130060C);
        lbl_814AD268 = 4;
        break;
    case 4:
    case 5:
        if (!fn_813008B0(&lbl_814AE720)) {
            break;
        }
        if (__DVDDiskMagic != 0xC2339F3D) {
            lbl_814AD268 = 0x14;
            break;
        }
        lbl_814AD268 = 6;
    case 6:
        if (!lbl_814AD294) {
            lbl_814AD268 = 8;
            break;
        }
        lbl_814AD294 = 0;
        if (((DVDDiskID*)0x80000000)->streaming) {
            __DVDAudioBufferConfig(&lbl_814AE720, 1, ((DVDDiskID*)0x80000000)->streamBufSize ? ((DVDDiskID*)0x80000000)->streamBufSize : 10, fn_8130060C);
        } else {
            __DVDAudioBufferConfig(&lbl_814AE720, 0, 0, fn_8130060C);
        }
        lbl_814AD268 = 7;
        break;
    case 7:
        if (!fn_813008B0(&lbl_814AE720)) {
            break;
        }
        lbl_814AD268 = 8;
    case 8:
        DVDReadAbsAsyncForBS(&lbl_814AE720, &lbl_814AE6E0.header, sizeof(AppLoaderHeader), 0x2440, fn_8130060C);
        lbl_814AD268 = 9;
        break;
    case 9:
        if (!fn_813008B0(&lbl_814AE720)) {
            break;
        }
        fn_81300598("  appLoaderLength ...... 0x%x\n", lbl_814AE6E0.header.size);
        fn_81300598("  appLoaderFunc1  ...... 0x%x\n", lbl_814AE6E0.header.entry);
        lbl_814AD268 = 0xA;
    case 10:
        DVDReadAbsAsyncForBS(&lbl_814AE720, (void*)0x81200000, OSRoundUp32B(lbl_814AE6E0.header.size), 0x2460, fn_8130060C);
        lbl_814AD268 = 0xB;
        break;
    case 11:
        if (!fn_813008B0(&lbl_814AE720)) {
            break;
        }
        lbl_814AE6E0.header.entry(&lbl_814AD26C, &lbl_814AD270, &lbl_814AD274);
        lbl_814AD26C(OSReport);
        fn_81300598("\nApploader Initialized\n");
        lbl_814AD268 = 0xC;
    case 12:
        if (lbl_814AD270(&addr, &length, &offset)) {
            fn_81300598("Addr [0x%x] length [0x%x] offset [0x%x]\n", addr, length, offset);
            enabled2 = OSDisableInterrupts();
            DVDReadAbsAsyncForBS(&lbl_814AE720, addr, length, offset, fn_8130060C);
            if (*lbl_814AD278) {
                lbl_814AD284 = 1;
                lbl_814AD280 = length;
            }
            OSRestoreInterrupts(enabled2);
            lbl_814AD268 = 0xD;
        } else if (!fn_813006E8()) {
            lbl_814AD268 = 0x14;
        } else {
            lbl_814AD268 = 0xE;
        }
        break;
    case 13:
        if (fn_813008B0(&lbl_814AE720)) {
            lbl_814AD268 = 0xC;
        }
        break;
    case 14:
        __DVDFSInit();
        entry = DVDConvertPathToEntrynum("/opening.bnr");
        if (entry >= 0) {
            DVDFastOpen(entry, &fi);
            if (lbl_814AD288 == NULL) {
                OSPanic("BS2Mach.c", 0x40E, "BS2 ERROR >>> Banner buffer address is not set");
            }
            if (lbl_814AD28C < fi.length) {
                OSPanic("BS2Mach.c", 0x411, "BS2 ERROR >>> Banner buffer length (0x%08x) is not enough", lbl_814AD28C);
            }
            if (fi.length != 0) {
                DVDReadAbsAsyncForBS(&lbl_814AE720, lbl_814AD288, OSRoundUp32B(fi.length), fi.startAddr, fn_8130060C);
                lbl_814AD268 = 0xF;
                break;
            }
        }
        lbl_814AD290 = FALSE;
        lbl_814AD268 = 0x10;
        DVDLowWaitCoverClose(fn_813005E8);
        break;
    case 15:
        if (!fn_813008B0(&lbl_814AE720)) {
            break;
        }
        lbl_814AD290 = TRUE;
        lbl_814AD268 = 0x10;
        DVDLowWaitCoverClose(fn_813005E8);
        break;
    case 16:
        break;
    case 17:
        lbl_814AD290 = FALSE;
        lbl_814AD268 = 2;
        break;
    case 18:
    case 19:
    case 24:
        DVDLowSetResetCoverCallback(NULL);
        lbl_814AD290 = FALSE;
        fn_813008B0(&lbl_814AE720);
        break;
    case 20:
        lbl_814AD290 = FALSE;
        DVDChangeDiskAsyncForBS(&lbl_814AE720, fn_8130060C);
        lbl_814AD268 = 0x15;
    case 21:
    case 22:
        fn_813008B0(&lbl_814AE720);
        break;
    case 23:
        lbl_814AD290 = FALSE;
        break;
    case 25:
        if (fn_813008B0(&lbl_814AE720)) {
            lbl_814AD268 = 3;
        }
        break;
    default:
        OSPanic("BS2Mach.c", 0x46A, "BS2 ERROR >>> UNKNOWN STATE");
        break;
    }
    OSRestoreInterrupts(enabled);
    if (DVDLowGetCoverStatus() == 1) {
        return 0x13;
    }
    return lbl_814AD268;
}
