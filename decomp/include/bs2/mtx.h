#ifndef BS2_MTX_H
#define BS2_MTX_H

#include <dolphin/mtx.h>

/* Scale / rotate / translate record used by MtxFromSRT (0x8130764C). */
typedef struct SRT {
    f32 sx, sy, sz;        /* 0x00 */
    s16 rx, ry, rz;        /* 0x0C */
    s16 pad;
    f32 tx, ty, tz;        /* 0x14 */
} SRT;

/* BS2 math (mtx.c, 0x81307208..0x81307D08). Angles: 0x10000 = full turn. */
void MtxRotAxis(Mtx m, char axis, s16 angle);                           /* 0x81307208 */
void MtxRotTrans(s16 rx, s16 ry, s16 rz, Mtx m, f32 tx, f32 ty, f32 tz); /* 0x813073B4 */
void MtxRot(s16 rx, s16 ry, s16 rz, Mtx m);                             /* 0x81307510 */
void MtxFromSRT(const SRT *srt, Mtx out);                               /* 0x8130764C */
void MtxScaleCols(Mtx m, Vec *s);                                       /* 0x81307754 */
BOOL MtxInverse33(Mtx in, Mtx out);                                     /* 0x81307820 */
void VecHermite(const Vec *p0, const Vec *v0, const Vec *p1, const Vec *v1, Vec *out,
                f32 t, f32 t0, f32 t1);                                 /* 0x813079FC */
BOOL SinTableInit(u8 bits);                                             /* 0x81307B0C */
f32 SinS(s16 angle);                                                    /* 0x81307CD0 */
f32 CosS(s16 angle);                                                    /* 0x81307CEC */

#endif
