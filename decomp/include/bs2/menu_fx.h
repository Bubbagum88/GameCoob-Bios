#ifndef BS2_MENU_FX_H
#define BS2_MENU_FX_H

#include "bs2/menu.h"

/* Shared by the mosaic text units (menu_fx_shape.c, menu_fx.c, menu_fx_draw.c). */

#define FX_MAX 0x861

/* A placed cube, as handed to the renderer. */
typedef struct FxCube {
    Vec scale;      /* 0x00 */
    s16 rot[3];     /* 0x0C */
    Vec pos;        /* 0x14 */
} FxCube;           /* 0x20 */

void Fx_SetShapes(void* file);
void Fx_EvalShape(s32 n, FxCube* out, u32* count, f32 t);
u16 fn_81329B24(s32 n);

#endif
