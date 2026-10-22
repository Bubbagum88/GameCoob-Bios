#ifndef BS2_GBASE_H
#define BS2_GBASE_H

#include "types.h"

/* gBase.c (0x81307D08..0x81307FB8): curve helper, heap, random numbers */
f32 Hermite(f32 t, f32 t0, f32 p0, f32 v0, f32 t1, f32 p1, f32 v1);   /* 0x81307D08 */
void HeapInit(void);                                                  /* 0x81307D90 */
void HeapDump(void);                                                  /* 0x81307E1C */
void *HeapAlloc(u32 size);                                            /* 0x81307E40 */
void HeapFree(void *p);                                               /* 0x81307E90 */
void RandInit(void);                                                  /* 0x81307EB8 */
u32 Rand(void);                                                       /* 0x81307EF8 */

#endif
