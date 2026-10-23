// Mosaic cube text sparks (PAL 1.0 BS2, 0x8133370C..0x81334C08): the
// glyph cubes spin around the centre of their glyph and drift on an eased
// path, looping every fxC8 frames.

#include "bs2/menu_fx_cube.h"
#include <string.h>

static FxCube sFxSpark[FX_MAX_CUBES];

static u32 sFxSparkFrame;
static u32 sFxSparkCount;

/* 0x8133370C */
void fn_8133370C(void) {
    sFxSparkFrame = 0;
    sFxSparkCount = 0;
    memset(sFxSpark, 0, sizeof(sFxSpark));
}

/* 0x8133374C: blow the whole cube set apart (Game Play). */
void fn_8133374C(FxCube *cubes, u32 n) {
    f32 dx;
    f32 dy;
    u16 rx;
    u16 ry;
    Vec c;
    FxCube tmp;
    u32 i;

    fn_8133498C(&dx, &dy, &rx, &ry, (f32)sFxSparkFrame / Menu_Get()->fxC8);
    fn_8132C830(&c, cubes, NULL, n);
    for (i = 0; i < n; i++) {
        cubes[i].pos.x += dx;
        cubes[i].pos.y += dy;
        fn_8132C750(0, c.x, c.y, c.z, rx, ry, &cubes[i], &tmp);
        sFxSpark[i].pos.x = tmp.pos.x;
        sFxSpark[i].pos.y = tmp.pos.y;
        sFxSpark[i].pos.z = tmp.pos.z;
        sFxSpark[i].rot[0] = tmp.rot[0];
        sFxSpark[i].rot[1] = tmp.rot[1];
        sFxSpark[i].rot[2] = tmp.rot[2];
    }
    fn_81334928();
}

#define SPIN_LIST(get, count)                                     \
    list = get();                                                 \
    fn_8132C830(&c, cubes, list, count());                        \
    for (i = 0; i < count(); i++) {                               \
        fn_8132C750(0, c.x, c.y, c.z, rx, ry, &cubes[list[i]], &spark[list[i]]); \
        spark[list[i]].pos.x += dx;                               \
        spark[list[i]].pos.y += dy;                               \
    }

/* 0x81333898: Options text sparks. */
void Fx_EmitSparks(FxCube *cubes) {
    FxCube *spark = sFxSpark;
    f32 dx;
    f32 dy;
    u16 rx;
    u16 ry;
    Vec c;
    u16 *list;
    s32 i;

    fn_8133498C(&dx, &dy, &rx, &ry, (f32)sFxSparkFrame / Menu_Get()->fxC8);
    SPIN_LIST(fn_81332528, fn_81332590)
    SPIN_LIST(fn_8133285C, fn_813328C4)
    SPIN_LIST(fn_81332900, fn_81332968)
    SPIN_LIST(fn_81332C34, fn_81332C9C)
    SPIN_LIST(fn_81331008, fn_81331070)
    fn_81334928();
}

/* 0x81333C7C: Calendar text sparks. */
void fn_81333C7C(FxCube *cubes) {
    FxCube *spark = sFxSpark;
    f32 dx;
    f32 dy;
    u16 rx;
    u16 ry;
    Vec c;
    u16 *list;
    s32 i;

    fn_8133498C(&dx, &dy, &rx, &ry, (f32)sFxSparkFrame / Menu_Get()->fxC8);
    SPIN_LIST(fn_81331338, fn_813313A0)
    SPIN_LIST(fn_813313DC, fn_81331444)
    SPIN_LIST(fn_81331480, fn_813314E8)
    SPIN_LIST(fn_81331524, fn_8133158C)
    SPIN_LIST(fn_813320AC, fn_81332114)
    SPIN_LIST(fn_81332150, fn_813321B8)
    SPIN_LIST(fn_813321F4, fn_8133225C)
    SPIN_LIST(fn_81332298, fn_81332300)
    SPIN_LIST(fn_81331CD4, fn_81331D3C)
    SPIN_LIST(fn_81331D78, fn_81331DE0)
    SPIN_LIST(fn_81331710, fn_81331778)
    SPIN_LIST(fn_813317B4, fn_8133181C)
    SPIN_LIST(fn_813318FC, fn_81331964)
    SPIN_LIST(fn_813319A0, fn_81331A08)
    SPIN_LIST(fn_81331AE8, fn_81331B50)
    SPIN_LIST(fn_81331B8C, fn_81331BF4)
    SPIN_LIST(fn_81331EC0, fn_81331F28)
    SPIN_LIST(fn_81331F64, fn_81331FCC)
    fn_81334928();
}

/* 0x8133491C */
FxCube *fn_8133491C(void) {
    return sFxSpark;
}

/* 0x81334928 */
void fn_81334928(void) {
    sFxSparkFrame++;
    sFxSparkCount++;
    if (sFxSparkFrame >= Menu_Get()->fxC8) {
        sFxSparkFrame = 0;
    }
    if (sFxSparkCount >= 100) {
        sFxSparkCount = 0;
    }
}

static inline f32 FxMod(f32 x, f32 y) {
    f32 m = y;
    u64 n;
    s64 k;

    if (__fabs(m) > __fabs(x)) {
        return x;
    }
    n = x / m;
    k = n;
    return x - m * k;
}

/* 0x8133498C: spark flight offsets and spin angles at time t (0..1). */
void fn_8133498C(f32 *dx, f32 *dy, u16 *rx, u16 *ry, f32 t) {
    f32 px;
    f32 py;
    f32 pr;

    px = fn_81334BA8(t + 0.5f * Menu_Get()->fxC4, 1.0f);
    py = fn_81334BA8(0.25f + 2.0f * FxMod(t, 1.0f), 1.0f);
    pr = FxMod(0.25f + t, 1.0f);
    *dx = fn_81334B40(Menu_Get()->fxC4, px, -Menu_Get()->fxB4, Menu_Get()->fxB4);
    *dy = fn_81334B40(0.5f, py, -Menu_Get()->fxB8, Menu_Get()->fxB8);
    *rx = fn_8132CB60(fn_81334B40(0.5f, pr, -Menu_Get()->fxBC, Menu_Get()->fxBC));
    *ry = fn_8132CB60(fn_81334B40(0.5f, pr, -Menu_Get()->fxC0, Menu_Get()->fxC0));
}

/* 0x81334B40: ease lo -> hi over 0..mid, back to lo over mid..1. */
f32 fn_81334B40(f32 mid, f32 t, f32 lo, f32 hi) {
    if (t < mid) {
        return Hermite(t, 0.0f, lo, 0.0f, mid, hi, 0.0f);
    }
    return Hermite(t, mid, hi, 0.0f, 1.0f, lo, 0.0f);
}

/* 0x81334BA8 */
f32 fn_81334BA8(f32 x, f32 y) {
    u64 n;

    if (__fabs(y) > __fabs(x)) {
        return x;
    }
    n = x / y;
    return x - y * (s64)n;
}
