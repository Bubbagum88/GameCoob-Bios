// Matching.
//
// Mosaic text keyframes (PAL 1.0 BS2, 0x81329890..0x81329B34): every cube of
// a mosaic shape flies along a Hermite curve. The shape file (res 37) holds
// one entry per shape; each entry lists its cubes, and each cube a start and
// end time and the curve keys it passes through.

#include "bs2/menu_fx.h"
#include "bs2/mtx.h"

/* Shape table (res 37): one entry per shape, then per-cube key lists. */
typedef struct FxShape {
    u16 count;      /* 0x0 cubes in the shape */
    u16 nKeys;      /* 0x2 keyframes per cube */
    s32 cubes;      /* 0x4 offset of the FxShapeCube list from this entry */
} FxShape;

typedef struct FxShapeCube {
    u8 start;       /* 0x0 when the cube starts moving (0..255 -> 0..0.4) */
    u8 end;         /* 0x1 when it arrives (0..255 -> 0.6..1.0) */
    s32 keys;       /* 0x4 offset of the u16 key index list from this entry */
} FxShapeCube;

/* Curve key: a point and its incoming/outgoing tangents. */
typedef struct FxKey {
    Vec pos;        /* 0x00 */
    Vec in;         /* 0x0C */
    Vec out;        /* 0x18 */
} FxKey;            /* 0x24 */

typedef struct FxShapeFile {
    u8 pad[8];
    s32 keys;       /* 0x8 offset of the FxKey array */
    s32 shapes;     /* 0xC offset of the FxShape array */
} FxShapeFile;

static FxShapeFile* sFxFile;
static FxShape* sFxShapes;
static FxKey* sFxKeys;

/* 0x81329890 */
void Fx_SetShapes(void* f) {
    FxShapeFile* file = f;

    sFxFile = file;
    sFxShapes = (FxShape*)((u8*)file + file->shapes);
    sFxKeys = (FxKey*)((u8*)file + file->keys);
}

/* 0x813298B0: place the cubes of shape n at time t (0..1). */
void Fx_EvalShape(s32 n, FxCube* out, u32* count, f32 t) {
    FxShapeCube* c;
    f32 step;
    u32 i;
    u32 k;
    f32 seg;
    f32 u;
    f32 hi;
    Vec v;
    f32 lo;

    *count = sFxShapes[n].count;
    step = 1.0f / (s32)(sFxShapes[n].nKeys - 1);
    c = (FxShapeCube*)((u8*)&sFxShapes[n] + sFxShapes[n].cubes);
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    for (i = 0; i < *count; i++) {
        u16* keys = (u16*)((u8*)&c[i] + c[i].keys);
        FxKey* k0;
        FxKey* k1;

        hi = 0.6f + 0.4f * (1.0f - c[i].end / 255.0f);
        lo = 0.4f * (1.0f - c[i].start / 255.0f);
        u = (t - lo) / (hi - lo);
        seg = u / step;
        k = seg;
        if ((u32)seg >= sFxShapes[n].nKeys) {
            k = sFxShapes[n].nKeys - 1;
        }
        if (u < 0.0f) {
            u = 0.0f;
        }
        if (u > 1.0f) {
            u = 1.0f;
        }
        k1 = &sFxKeys[keys[k + 1]];
        k0 = &sFxKeys[keys[k]];
        VecHermite(&k0->pos, &k0->out, &k1->pos, &k1->in, &v, u, k * step, (k + 1) * step);
        out[i].pos.x = v.x;
        out[i].pos.y = v.y;
        out[i].pos.z = v.z;
        out[i].rot[0] = out[i].rot[1] = out[i].rot[2] = 0;
        out[i].scale.x = out[i].scale.y = out[i].scale.z = Menu_Get()->fxCubeScale;
    }
}

/* 0x81329B24 */
u16 fn_81329B24(s32 n) {
    return sFxShapes[n].count;
}

