#ifndef _DOLPHIN_OS_OSUTIL_H_
#define _DOLPHIN_OS_OSUTIL_H_

#include <dolphin/types.h>

#define OS_BASE_CACHED 0x80000000
#define OS_BASE_UNCACHED 0xC0000000

#define OSRoundUp32B(x) (((u32)(x) + 0x1F) & ~(0x1F))
#define OSRoundDown32B(x) (((u32)(x)) & ~(0x1F))

#define OSPhysicalToCached(paddr) ((void*)((u32)(paddr) + OS_BASE_CACHED))
#define OSPhysicalToUncached(paddr) ((void*)((u32)(paddr) + OS_BASE_UNCACHED))
#define OSCachedToPhysical(caddr) ((u32)((u8*)(caddr) - OS_BASE_CACHED))
#define OSUncachedToPhysical(ucaddr) ((u32)((u8*)(ucaddr) - OS_BASE_UNCACHED))

#ifndef OSNanosecondsToTicks
#define OSNanosecondsToTicks(nsec) (((nsec) * (OS_TIMER_CLOCK / 125000)) / 8000)
#endif

// Assertions compile away in the IPL's release SDK; OSError keeps its
// original line number so the __FILE__/line pair passed to OSPanic matches.
#define OSErrorLine(line, ...) OSPanic(__FILE__, line, __VA_ARGS__)
#define OSAssertMsgLine(line, cond, ...) ((void)0)
#define OSAssertLine(line, cond) ((void)0)

u16 OSGetFontEncode(void);

#endif
