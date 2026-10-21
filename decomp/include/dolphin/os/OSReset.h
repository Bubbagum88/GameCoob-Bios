#ifndef _DOLPHIN_OS_OSRESET_H_
#define _DOLPHIN_OS_OSRESET_H_

#include <dolphin/types.h>

typedef struct OSResetFunctionInfo OSResetFunctionInfo;
typedef BOOL (*OSResetFunction)(BOOL final);

struct OSResetFunctionInfo {
    OSResetFunction func;
    u32 priority;
    OSResetFunctionInfo* next;
    OSResetFunctionInfo* prev;
};

void OSRegisterResetFunction(OSResetFunctionInfo* info);
u32 OSGetResetCode(void);

#define OS_RESETCODE_RESTART 0x80000000

#endif
