#ifndef _DOLPHIN_OSRTC_H_
#define _DOLPHIN_OSRTC_H_

#include <dolphin/types.h>

typedef struct OSSram {
    u16 checkSum;
    u16 checkSumInv;
    u32 ead0;
    u32 ead1;
    u32 counterBias;
    s8 displayOffsetH;
    u8 ntd;
    u8 language;
    u8 flags;
} OSSram;

typedef struct OSSramEx {
    u8 flashID[2][12];
    u32 wirelessKeyboardID;
    u16 wirelessPadID[4];
    u8 dvdErrorCode;
    u8 _padding0;
    u8 flashIDCheckSum[2];
    u8 _padding1[4];
} OSSramEx;

#define OS_SOUND_MODE_MONO 0
#define OS_SOUND_MODE_STEREO 1

BOOL __OSGetRTC(u32* rtc);
OSSram* __OSLockSram(void);
OSSramEx* __OSLockSramEx(void);
BOOL __OSUnlockSram(BOOL commit);
BOOL __OSUnlockSramEx(BOOL commit);
BOOL __OSSyncSram(void);
u32 OSGetSoundMode(void);
u16 OSGetWirelessID(s32 channel);
void OSSetWirelessID(s32 channel, u16 id);
void OSSetSoundMode(u32 mode);

#endif
