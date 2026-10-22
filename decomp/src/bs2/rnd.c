#include "bs2/gbase.h"
#include <dolphin/os.h>

static u32 sRandA;
static u32 sRandB;
static u32 sRandC;
static u32 sRandD;

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
