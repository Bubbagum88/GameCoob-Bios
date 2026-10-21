#ifndef _DOLPHIN_OS_INTERNAL_H_
#define _DOLPHIN_OS_INTERNAL_H_

#include <dolphin/os.h>

#define OSPhysicalToCached(paddr) ((void*)((u32)(paddr) + 0x80000000))
#define OSCachedToPhysical(caddr) ((u32)(caddr) - 0x80000000)

#define OS_CONSOLE_RETAIL 0x00000000
#define OS_CONSOLE_RETAIL1 0x00000001
#define OS_CONSOLE_DEVELOPMENT 0x10000000
#define OS_CONSOLE_EMULATOR 0x10000000
#define OS_CONSOLE_PC_EMULATOR 0x10000001
#define OS_CONSOLE_ARTHUR 0x10000002
#define OS_CONSOLE_MINNOW 0x10000003
#define OS_CONSOLE_DEVHW1 0x10000004

#define OS_RESETCODE_RESTART 0x80000000

typedef struct OSBootInfo {
    u8 gameName[4];
    u8 company[2];
    u8 diskNumber;
    u8 gameVersion;
    u8 streaming;
    u8 streamingBufSize;
    u8 padding[22];
    u32 magic;
    u32 version;
    u32 memorySize;
    u32 consoleType;
    void* arenaLo;
    void* arenaHi;
    void* FSTLocation;
    u32 FSTMaxLength;
} OSBootInfo;

extern OSTime __OSStartTime;

u32 OSGetConsoleType(void);
u32 OSGetResetCode(void);
void __OSInitSystemCall(void);
void __OSModuleInit(void);
void __OSInterruptInit(void);
void __OSResetSWInterruptHandler(__OSInterrupt interrupt, OSContext* context);
void __OSContextInit(void);
void __OSCacheInit(void);
void __OSThreadInit(void);
void __OSInitSram(void);
void __OSInitAudioSystem(void);
void __OSStopAudioSystem(void);
void __OSUnhandledException(__OSException exception, OSContext* context, u32 dsisr, u32 dar);
u32 __OSGetDIConfig(void);
void OSInitAlarm(void);
void EXIInit(void);
void SIInit(void);

#endif
