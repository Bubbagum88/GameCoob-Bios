// Mosaic cube text shape morph (PAL 1.0 BS2, 0x81333344..0x8133370C):
// the cubes morph into another shape when the selection mode changes.

#include "bs2/menu_fx_cube.h"
#include <string.h>

typedef struct FxShapeState {
    s16 state;             /* 0 none, 1 morph in, 2 morph out */
    u16 frame;
} FxShapeState;

static FxCube sFxShape[100];

static FxShapeState sFxShapeState;

/* 0x81333344 */
void fn_81333344(void) {
    sFxShapeState.state = 0;
    sFxShapeState.frame = 0;
    memset(sFxShape, 0, sizeof(sFxShape));
}

/* 0x81333384: start the morph when the selection mode changes. */
void fn_81333384(void) {
    sFxShapeState.frame++;
    switch (sFxShapeState.state) {
    case 1:
        if (sFxShapeState.frame >= Menu_Get()->fx124) {
            sFxShapeState.frame = Menu_Get()->fx124;
        }
        break;
    case 2:
        if (sFxShapeState.frame >= Menu_Get()->fx126) {
            sFxShapeState.frame = 0;
            sFxShapeState.state = 0;
        }
        break;
    }
    if (fn_8132CFE8()->mode != fn_8132CFF4()->mode) {
        switch (fn_8132CFE8()->mode) {
        case 2:
            sFxShapeState.state = 1;
            sFxShapeState.frame = 0;
            break;
        case 0:
            if (fn_8132CFF4()->mode == 2) {
                sFxShapeState.state = 2;
                sFxShapeState.frame = 0;
            } else {
                sFxShapeState.state = 0;
            }
            break;
        case 1:
        default:
            sFxShapeState.state = 0;
            break;
        }
    }
}

/* 0x813334AC */
FxCube *fn_813334AC(void) {
    u32 info[2];

    switch (sFxShapeState.state) {
    case 1:
        Fx_EvalShape(3, sFxShape, info, (f32)sFxShapeState.frame / Menu_Get()->fx124);
        break;
    case 2:
        Fx_EvalShape(4, sFxShape, info, 1.0f - (f32)sFxShapeState.frame / Menu_Get()->fx126);
        break;
    }
    return sFxShape;
}

/* 0x813335AC */
u16 fn_813335AC(void) {
    switch (sFxShapeState.state) {
    case 1:
        return fn_81329B24(3);
    case 2:
        return fn_81329B24(4);
    case 0:
        return 0;
    default:
        return 0;
    }
}

/* 0x81333614 */
s32 fn_81333614(void) {
    switch (sFxShapeState.state) {
    case 1:
        return 255.0f * sFxShapeState.frame / Menu_Get()->fx124;
    case 2:
        return 255.0f - 255.0f * sFxShapeState.frame / Menu_Get()->fx126;
    default:
        return 0;
    }
}
