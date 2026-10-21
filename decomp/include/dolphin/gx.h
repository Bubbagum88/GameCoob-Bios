#ifndef _DOLPHIN_GX_H_
#define _DOLPHIN_GX_H_

#include <dolphin/types.h>
#include <dolphin/mtx.h>

typedef struct GXColor {
    u8 r, g, b, a;
} GXColor;

typedef struct GXColorS10 {
    s16 r, g, b, a;
} GXColorS10;

#define GX_PNMTX0 0

#define GX_TF_RGB5A3 5
#define GX_CLAMP 0
#define GX_LINEAR 1

void GXLoadPosMtxImm(Mtx mtx, u32 id);
void GXLoadNrmMtxImm(Mtx mtx, u32 id);
void GXSetCurrentMtx(u32 id);

#endif
