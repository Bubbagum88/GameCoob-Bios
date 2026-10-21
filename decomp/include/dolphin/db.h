#ifndef _DOLPHIN_DB_H_
#define _DOLPHIN_DB_H_

// Based on the Pikmin decomp (CC0) db.h

#include <dolphin/types.h>

#define ExceptionHookDestination 0x80000048
#define IsDebuggerPresent 0x80000040

typedef struct DBInterface {
    u8 filler0[4];
    u32 unk4;
} DBInterface;

extern DBInterface* __DBInterface;
extern BOOL DBVerbose;

void DBInit(void);
void __DBExceptionDestination(void);
void __DBExceptionDestinationAux(void);
int __DBIsExceptionMarked(u8);
void DBPrintf(const char* format, ...);

#endif
