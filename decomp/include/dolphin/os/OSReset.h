#ifndef _DOLPHIN_OS_OSRESET_H_
#define _DOLPHIN_OS_OSRESET_H_

#include <dolphin/types.h>

typedef BOOL (*OSResetFunction)(BOOL final);
typedef struct OSResetFunctionInfo OSResetFunctionInfo;

struct OSResetFunctionInfo {
    OSResetFunction func;
    u32 priority;
    OSResetFunctionInfo* next;
    OSResetFunctionInfo* prev;
};

void OSRegisterResetFunction(OSResetFunctionInfo* info);
u32 OSGetResetCode(void);

#endif
