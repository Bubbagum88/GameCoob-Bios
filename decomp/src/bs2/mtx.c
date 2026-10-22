#include "bs2/mtx.h"
#include "bs2/gbase.h"
#include <math.h>

static u16 sSinCount;  /* entries per full turn */
static s32 sSinShift;  /* 16 - log2(sSinCount) */
static f32 *sSinTable; /* sSinCount * 5/4 entries */
static f32 *sCosTable; /* sSinTable + quarter turn */

/* 0x81307208: rotation about one axis ('x'/'X', 'y'/'Y', 'z'/'Z') */
void MtxRotAxis(Mtx m, char axis, s16 angle) {
    switch (axis) {
    case 'x':
    case 'X':
        m[0][0] = 1.0f;
        m[0][1] = 0.0f;
        m[0][2] = 0.0f;
        m[0][3] = 0.0f;
        m[1][0] = 0.0f;
        m[1][1] = CosS(angle);
        m[1][2] = -SinS(angle);
        m[1][3] = 0.0f;
        m[2][0] = 0.0f;
        m[2][1] = SinS(angle);
        m[2][2] = CosS(angle);
        m[2][3] = 0.0f;
        break;
    case 'y':
    case 'Y':
        m[0][0] = CosS(angle);
        m[0][1] = 0.0f;
        m[0][2] = SinS(angle);
        m[0][3] = 0.0f;
        m[1][0] = 0.0f;
        m[1][1] = 1.0f;
        m[1][2] = 0.0f;
        m[1][3] = 0.0f;
        m[2][0] = -SinS(angle);
        m[2][1] = 0.0f;
        m[2][2] = CosS(angle);
        m[2][3] = 0.0f;
        break;
    case 'z':
    case 'Z':
        m[0][0] = CosS(angle);
        m[0][1] = -SinS(angle);
        m[0][2] = 0.0f;
        m[0][3] = 0.0f;
        m[1][0] = SinS(angle);
        m[1][1] = CosS(angle);
        m[1][2] = 0.0f;
        m[1][3] = 0.0f;
        m[2][0] = 0.0f;
        m[2][1] = 0.0f;
        m[2][2] = 1.0f;
        m[2][3] = 0.0f;
        break;
    }
}

/* 0x813073B4: m = Rz * Ry * Rx with translation */
void MtxRotTrans(s16 rx, s16 ry, s16 rz, Mtx m, f32 tx, f32 ty, f32 tz) {
    f32 sx = SinS(rx);
    f32 cx = CosS(rx);
    f32 sy = SinS(ry);
    f32 cy = CosS(ry);
    f32 sz = SinS(rz);
    f32 cz = CosS(rz);

    f32 czsy;
    f32 szsy;

    czsy = cz * sy;
    m[0][0] = cz * cy;
    m[0][1] = -sz * cx + sx * czsy;
    m[0][2] = sz * sx + cx * czsy;
    m[0][3] = tx;
    szsy = sz * sy;
    m[1][0] = sz * cy;
    m[1][1] = cz * cx + sx * szsy;
    m[1][2] = -cz * sx + cx * szsy;
    m[1][3] = ty;
    m[2][0] = -sy;
    m[2][1] = cy * sx;
    m[2][2] = cy * cx;
    m[2][3] = tz;
}

/* 0x81307510 */
void MtxRot(s16 rx, s16 ry, s16 rz, Mtx m) {
    f32 sx = SinS(rx);
    f32 cx = CosS(rx);
    f32 sy = SinS(ry);
    f32 cy = CosS(ry);
    f32 sz = SinS(rz);
    f32 cz = CosS(rz);

    m[0][0] = cz * cy;
    m[0][1] = -sz * cx + sx * (cz * sy);
    m[0][2] = sz * sx + cx * (cz * sy);
    m[0][3] = 0.0f;
    m[1][0] = sz * cy;
    m[1][1] = cz * cx + sx * (sz * sy);
    m[1][2] = -cz * sx + cx * (sz * sy);
    m[1][3] = 0.0f;
    m[2][0] = -sy;
    m[2][1] = cy * sx;
    m[2][2] = cy * cx;
    m[2][3] = 0.0f;
}

/* 0x8130764C */
void MtxFromSRT(const SRT *srt, Mtx m) {
    MtxRotTrans(srt->rx, srt->ry, srt->rz, m, srt->tx, srt->ty, srt->tz);
    if (srt->sx != 1.0f) {
        m[0][0] *= srt->sx;
        m[1][0] *= srt->sx;
        m[2][0] *= srt->sx;
    }
    if (srt->sy != 1.0f) {
        m[0][1] *= srt->sy;
        m[1][1] *= srt->sy;
        m[2][1] *= srt->sy;
    }
    if (srt->sz != 1.0f) {
        m[0][2] *= srt->sz;
        m[1][2] *= srt->sz;
        m[2][2] *= srt->sz;
    }
}

/* 0x81307754 */
void MtxScaleCols(Mtx m, Vec *s) {
    f32 *p = m[0];
    s32 i;

    for (i = 0; i < 3; i++) {
        *p++ *= s->x;
        *p++ *= s->y;
        *p++ *= s->z;
        p++;
    }
}

/* 0x81307820: inverse of the 3x3 part, translation cleared */
BOOL MtxInverse33(Mtx a, Mtx b) {
    f32 det;
    f32 inv;

    det = a[0][0] * a[1][1] * a[2][2] + a[0][1] * a[1][2] * a[2][0] + a[0][2] * a[1][0] * a[2][1]
        - a[2][0] * a[1][1] * a[0][2] - a[1][0] * a[0][1] * a[2][2] - a[0][0] * a[2][1] * a[1][2];
    if (det == 0.0f) {
        return FALSE;
    }
    inv = 1.0f / det;
    b[0][0] = inv * (a[1][1] * a[2][2] - a[2][1] * a[1][2]);
    b[0][1] = inv * -(a[1][0] * a[2][2] - a[2][0] * a[1][2]);
    b[0][2] = inv * (a[1][0] * a[2][1] - a[2][0] * a[1][1]);
    b[0][3] = 0.0f;
    b[1][0] = inv * -(a[0][1] * a[2][2] - a[2][1] * a[0][2]);
    b[1][1] = inv * (a[0][0] * a[2][2] - a[2][0] * a[0][2]);
    b[1][2] = inv * -(a[0][0] * a[2][1] - a[2][0] * a[0][1]);
    b[1][3] = 0.0f;
    b[2][0] = inv * (a[0][1] * a[1][2] - a[1][1] * a[0][2]);
    b[2][1] = inv * -(a[0][0] * a[1][2] - a[1][0] * a[0][2]);
    b[2][2] = inv * (a[0][0] * a[1][1] - a[1][0] * a[0][1]);
    b[2][3] = 0.0f;
    return TRUE;
}

/* 0x813079FC: Hermite curve per component between key times t0 and t1 */
void VecHermite(const Vec *p0, const Vec *v0, const Vec *p1, const Vec *v1, Vec *out, f32 t, f32 t0, f32 t1) {
    Vec r;
    f32 *pr = &r.x;
    s32 i;

    if (t < t0) {
        *out = *p0;
    } else if (t > t1) {
        *out = *p1;
    } else {
        for (i = 0; i < 3; i++) {
            pr[i] = Hermite(t, t0, (&p0->x)[i], (&v0->x)[i], t1, (&p1->x)[i], (&v1->x)[i]);
        }
        *out = r;
    }
}

/* 0x81307B0C: table of 2^bits sines (+ a quarter turn for cosines) */
BOOL SinTableInit(u8 bits) {
    u8 i;
    u16 j;

    sSinShift = 16 - bits;
    sSinCount = 1;
    for (i = 0; i < bits; i++) {
        sSinCount *= 2;
    }
    sSinTable = HeapAlloc((sSinCount + sSinCount / 4) * sizeof(f32));
    if (sSinTable == NULL) {
        return FALSE;
    }
    for (j = 0; j < sSinCount + sSinCount / 4; j++) {
        sSinTable[j] = sinf(6.2831855f / sSinCount * j);
    }
    sCosTable = sSinTable + sSinCount / 4;
    return TRUE;
}

/* 0x81307CD0: angle 0x10000 = full turn */
f32 SinS(s16 angle) {
    return sSinTable[(u16)angle >> sSinShift];
}

/* 0x81307CEC */
f32 CosS(s16 angle) {
    return sCosTable[(u16)angle >> sSinShift];
}
