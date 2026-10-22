#ifndef _JAUDIO_SDK_H
#define _JAUDIO_SDK_H

// JAudio is C++ but the SDK and MSL headers have no extern "C" blocks, so
// JAudio pulls them all in through here, inside one.

#include "jaudio/jaudio_types.h"

BEGIN_SCOPE_EXTERN_C

#include <dolphin/os.h>
#include <dolphin/os/OSCache.h>
#include <dolphin/os/OSError.h>
#include <dolphin/os/OSInterrupt.h>
#include <dolphin/os/OSMessage.h>
#include <dolphin/os/OSThread.h>
#include <dolphin/base/PPCArch.h>
#include <dolphin/hw_regs.h>
#include <dolphin/ai.h>
#include <dolphin/ar.h>
#include <dolphin/dsp.h>
#include <dolphin/dvd.h>
#include <dolphin/exi.h>
#include <dolphin/card.h>
#include <math.h>
#include <string.h>

// MSL bits BS2's headers do not declare yet
char* strcat(char* dst, const char* src);
int rand(void);
void srand(unsigned int seed);
double atan2(double y, double x);

// OSFastCast.h is not in BS2's SDK headers yet: the two JAudio uses
#ifndef _DOLPHIN_OS_OSFASTCAST_H
#define OS_GQR_U8  (0x0004)
#define OS_GQR_U16 (0x0005)
#define OS_GQR_S8  (0x0006)
#define OS_GQR_S16 (0x0007)
#define OS_FASTCAST_S8 (4)

static inline void OSInitFastCast(void)
{
#ifdef __MWERKS__
	asm {
		li     r3,     OS_GQR_U8
		oris   r3, r3, OS_GQR_U8
		mtspr  914, r3 // GQR2
		li     r3,     OS_GQR_U16
		oris   r3, r3, OS_GQR_U16
		mtspr  915, r3 // GQR3
		li     r3,     OS_GQR_S8
		oris   r3, r3, OS_GQR_S8
		mtspr  916, r3 // GQR4
		li     r3,     OS_GQR_S16
		oris   r3, r3, OS_GQR_S16
		mtspr  917, r3 // GQR5
	}
#endif
}

static inline void OSf32tos8(register f32* in, register s8* out)
{
#ifdef __MWERKS__
	asm {
		lfs     fp1, 0 (in)
		psq_st  fp1, 0 (out), 1, OS_FASTCAST_S8
	}
#else
	*out = *in;
#endif
}
#endif

END_SCOPE_EXTERN_C

#ifndef ALIGN_NEXT
#define ALIGN_PREV(X, N) ((X) & ~((N) - 1))
#define ALIGN_NEXT(X, N) ALIGN_PREV(((X) + ((N) - 1)), N)
#endif

#ifndef SHRT_MAX
#define SHRT_MAX 32767
#endif

#endif
