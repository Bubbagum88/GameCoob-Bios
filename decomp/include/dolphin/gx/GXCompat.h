#ifndef _DOLPHIN_GXCOMPAT_H
#define _DOLPHIN_GXCOMPAT_H

// Glue for the GX headers and sources, which come from the Pikmin decomp
// (projectPiki/pikmin, CC0) and expect its <dolphin/gx/GXCompat.h> helpers.

#include <dolphin/types.h>

#include <dolphin/gx/GXRegs.h>

// OS/PPC helpers the GX sources use that are not in our os.h / PPCArch.h yet
#ifndef OS_BASE_UNCACHED
#define OS_BASE_CACHED   (0x80000000)
#define OS_BASE_UNCACHED (0xC0000000)
#define OSPhysicalToCached(paddr)    ((void*)((u32)(paddr) + OS_BASE_CACHED))
#define OSPhysicalToUncached(paddr)  ((void*)((u32)(paddr) + OS_BASE_UNCACHED))
#define OSCachedToPhysical(caddr)    ((u32)((u8*)(caddr) - OS_BASE_CACHED))
#define OSUncachedToPhysical(ucaddr) ((u32)((u8*)(ucaddr) - OS_BASE_UNCACHED))
#endif
#ifndef SPR_WPAR
#define SPR_WPAR 921
#endif
#ifndef HID2_WPE
#define HID2_WPE 0x40000000
#endif
u32 PPCMfwpar(void);

#ifdef __cplusplus
#define BEGIN_SCOPE_EXTERN_C extern "C" {
#define END_SCOPE_EXTERN_C }
#else
#define BEGIN_SCOPE_EXTERN_C
#define END_SCOPE_EXTERN_C
#endif

#ifdef __MWERKS__
#define ASM asm
#else
#define ASM
#endif

#ifndef ATTRIBUTE_ALIGN
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#endif

#ifdef __MWERKS__
#define AT_ADDRESS(addr) : (addr)
#define __mwerks_sync() __sync()
#define __mwerks_cntlzw(x) __cntlzw(x)
#else
#define AT_ADDRESS(addr)
#define __mwerks_cntlzw(x) __builtin_clz(x)
#define __mwerks_rlwimi(a, b, sh, mb, me) (a)
#define __mwerks_sync() ((void)0)
#endif

#ifndef OSErrorLine
#define OSErrorLine(line, ...) OSPanic(__FILE__, line, __VA_ARGS__)
#endif
#ifndef OSAssertMsgLine
#ifdef DEBUG
#define OSAssertMsgLine(line, cond, ...) ((void)((cond) || (OSErrorLine(line, __VA_ARGS__), 0)))
#else
#define OSAssertMsgLine(line, cond, ...) ((void)(0))
#endif
#define OSAssertLine(line, cond) OSAssertMsgLine(line, cond, "Failed assertion " #cond)
#endif

// BS2 PAL 1.0 links the Nov 2001 SDK: newer than 2001-10-02 (no tevTcEnab, no EURGB60)
#ifndef OS_BUILD_VERSION
#define OS_BUILD_VERSION 20011112L
#endif

#endif
