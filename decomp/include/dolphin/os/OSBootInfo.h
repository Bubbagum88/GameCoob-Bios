#ifndef _DOLPHIN_OS_OSBOOTINFO_H_
#define _DOLPHIN_OS_OSBOOTINFO_H_

#include <dolphin/types.h>
#include <dolphin/dvd.h>

typedef struct OSBootInfo {
    DVDDiskID DVDDiskID;
    u32 magic;
    u32 version;
    u32 memorySize;
    u32 consoleType;
    void* arenaLo;
    void* arenaHi;
    void* FSTLocation;
    u32 FSTMaxLength;
} OSBootInfo;

typedef struct BI2Debug {
    int debugMonSize;
    int simMemSize;
    u32 argOffset;
    u32 debugFlag;
    int trackLocation;
    int trackSize;
    u32 countryCode;
    u8 _1C[0x4];
    u32 dvdLongFileNameFlag;
    u32 padSpec;
} BI2Debug;

#define OS_BOOTINFO_MAGIC 0x0D15EA5E
#define OS_BOOTINFO_MAGIC_JTAG 0xE5207C22
#define OS_DVD_MAGIC_NINTENDO 0xC2339F3D

#endif
