#ifndef _DOLPHIN_OSALLOC_H_
#define _DOLPHIN_OSALLOC_H_

#include <dolphin/types.h>

typedef int OSHeapHandle;

extern volatile OSHeapHandle __OSCurrHeap;

void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps);
OSHeapHandle OSCreateHeap(void* start, void* end);
void OSDestroyHeap(OSHeapHandle heap);
void OSAddToHeap(OSHeapHandle heap, void* start, void* end);
OSHeapHandle OSSetCurrentHeap(OSHeapHandle heap);
void* OSAllocFromHeap(OSHeapHandle heap, u32 size);
void* OSAllocFixed(void** rstart, void** rend);
void OSFreeToHeap(OSHeapHandle heap, void* ptr);
long OSCheckHeap(OSHeapHandle heap);
void OSDumpHeap(OSHeapHandle heap);

#define OSAlloc(size) OSAllocFromHeap(__OSCurrHeap, (size))
#define OSFree(ptr) OSFreeToHeap(__OSCurrHeap, (ptr))

#endif
