#ifndef _DOLPHIN_OSCACHE_H_
#define _DOLPHIN_OSCACHE_H_

#include <dolphin/types.h>

void DCEnable(void);
void DCInvalidateRange(void* addr, u32 nBytes);
void DCFlushRange(void* addr, u32 nBytes);
void DCStoreRange(void* addr, u32 nBytes);
void DCFlushRangeNoSync(void* addr, u32 nBytes);
void ICFlashInvalidate(void);
void ICInvalidateRange(void* addr, u32 nBytes);
void DCZeroRange(void* addr, u32 nBytes);
void DCTouchRange(void* addr, u32 nBytes);
void LCEnable(void);

#endif
