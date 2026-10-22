#include "bs2/gbase.h"
#include <dolphin/os.h>
#include <dolphin/os/OSUtil.h>

static OSHeapHandle sHeap;
static u32 sRandA;
static u32 sRandB;
static u32 sRandC;
static u32 sRandD;

/* 0x81307D08: cubic Hermite between (t0, p0, v0) and (t1, p1, v1) */
f32 Hermite(f32 t, f32 t0, f32 p0, f32 v0, f32 t1, f32 p1, f32 v1) {
    f32 d = t1 - t0;
    f32 u = t - t0;
    f32 inv = 1.0f / d;
    f32 s2 = u * u * inv;
    f32 s2n = s2 * inv;
    f32 s3 = u * s2n;
    f32 s3n = s3 * inv;

    return p0 * (2.0f * s3n - 3.0f * s2n + 1.0f) + p1 * (-2.0f * s3n + 3.0f * s2n) + v0 * (s3 - 2.0f * s2 + u) + v1 * (s3 - s2);
}

/* 0x81307D90: one heap over the whole arena (allocator state at 0x80700000) */
void HeapInit(void) {
    void *lo = (void *)OSRoundUp32B(OSGetArenaLo());
    void *hi = (void *)OSRoundDown32B(OSGetArenaHi());
    void *start = OSInitAlloc((void *)0x80700000, hi, 2);

    OSSetArenaLo(hi);
    OSSetCurrentHeap(sHeap = OSCreateHeap(lo, hi));
    OSAddToHeap(sHeap, start, (void *)0x81100000);
    HeapDump();
}

/* 0x81307E1C */
void HeapDump(void) {
    OSDumpHeap(sHeap);
}

/* 0x81307E40 */
void *HeapAlloc(u32 size) {
    void *p = OSAllocFromHeap(sHeap, size);

    if (p == NULL) {
        OSErrorLine(65, "");
    }
    return p;
}

/* 0x81307E90 */
void HeapFree(void *p) {
    OSFreeToHeap(sHeap, p);
}

/* 0x81307EB8 */
void RandInit(void) {
    sRandA = OSGetTick();
    sRandB = OSGetTick();
    sRandC = OSGetTick();
    sRandD = OSGetTick();
    Rand();
}

/* 0x81307EF8: two LCG + LFSR pairs, picked by a timer bit */
u32 Rand(void) {
    if (OSGetTick() & 0x100) {
        sRandA = sRandA * 0x41C64E6D + 12345;
        if (sRandB & 1) {
            sRandB ^= 0x11020;
        }
        sRandB >>= 1;
        return sRandA ^ sRandB;
    } else {
        sRandC = sRandC * 0x41C64E6D + 67890;
        if (sRandD & 1) {
            sRandD ^= 0x11020;
        }
        sRandD >>= 1;
        return sRandC + sRandD;
    }
}
