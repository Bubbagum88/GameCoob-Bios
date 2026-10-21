#ifndef _DOLPHIN_DB_H_
#define _DOLPHIN_DB_H_

#include <dolphin/types.h>
#include <dolphin/os/OSException.h>

#define OS_DBINTERFACE_ADDR 0x00000040

typedef struct DBInterface {
    u32 bPresent;
    u32 exceptionMask;
    void (*ExceptionDestination)(void);
    void* exceptionReturn;
} DBInterface;

extern DBInterface* __DBInterface;

void DBInit(void);
BOOL DBIsDebuggerPresent(void);
void __DBExceptionDestination(void);
void __DBExceptionDestinationAux(void);
BOOL __DBIsExceptionMarked(__OSException exception);
void __DBMarkException(u8 exception, int value);
void __DBSetPresent(u32 value);
void DBPrintf(char* str, ...);

#endif
