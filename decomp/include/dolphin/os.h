#ifndef _DOLPHIN_OS_H_
#define _DOLPHIN_OS_H_

#include <dolphin/types.h>
#include <dolphin/os/OSThread.h>
#include <dolphin/os/OSMessage.h>
#include <dolphin/os/OSAlloc.h>
#include <dolphin/os/OSContext.h>
#include <dolphin/os/OSException.h>
#include <dolphin/os/OSTime.h>
#include <dolphin/os/OSAlarm.h>
#include <dolphin/os/OSCache.h>
#include <dolphin/os/OSError.h>
#include <dolphin/os/OSInterrupt.h>
#include <dolphin/os/OSRtc.h>

void* OSGetArenaHi(void);
void* OSGetArenaLo(void);
void OSSetArenaHi(void* newHi);
void OSSetArenaLo(void* newLo);

BOOL OSDisableInterrupts(void);
BOOL OSEnableInterrupts(void);
BOOL OSRestoreInterrupts(BOOL level);

void OSReport(const char* msg, ...);
void OSPanic(const char* file, int line, const char* msg, ...);

void OSDumpContext(OSContext* context);


#endif
