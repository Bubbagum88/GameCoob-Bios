#ifndef _JAUDIO_VIRLOAD_H
#define _JAUDIO_VIRLOAD_H



#include "jaudio/jaudio_types.h"

typedef struct BarcHeader BarcHeader;
typedef struct BarcEntry BarcEntry;
typedef union Barc Barc;

// BARC sequence archive header (names invented by the Pikmin decomp)
struct BarcHeader {
	char fileMagic[8]; // _00
	u32 _04;           // _08
	u32 seqCount;      // _0C
	char arcName[16];  // _10
};

struct BarcEntry {
	char jamName[14]; // _00
	u16 isDummy;      // _0E, 0xffff for used tracks, 0 for dummy tracks
	u32 _10;          // _10
	u32 _14;          // _14
	u32 offset;       // _18
	u32 size;         // _1C
};

union Barc {
	BarcHeader meta;
	BarcEntry entry;
};

void JV_InitHeader(immut char*);
BOOL JV_InitHeader_M(immut char* fileName, u8* barcData, u8* archiveWork);
u32 JV_GetArchiveHandle(immut char* archiveName);
void JV_GetLogicalHandleS(immut char*, immut char*);
void JV_GetLogicalHandle(immut char*);
void JV_GetHandle(u32);
BarcEntry* JV_GetRealHandle(u32 handle); // Return type unsure
u32 JV_CheckSize(u32 handle);
u32 JV_LoadFile(u32 handle, u8* dst, u32 offset, u32 length);
u32 JV_LoadFile_Async2(u32 handle, u8* dst, u32 offset, u32 length, void (*callback)(u32), u32 owner);
void JV_GetMemoryFile(u32);

#endif
