// gMR model renderer: keyframe animation sampling (PAL 1.0 BS2, 0x81306A3C..0x81307208).
// Hermite-interpolated J3D key tables for joint (ANK1) and color (TRK1-like) animation.

#include "bs2/gMRtex.h"

f32 Hermite(f32 t, f32 t0, f32 p0, f32 v0, f32 t1, f32 p1, f32 v1); /* 0x81307D08 */

GMRAnimOwner* lbl_814AD330;
void* lbl_814AD334;

/* 0x81306A3C: sample a float key table (keys are {time, value, [inTan,] outTan}) */
f32 fn_81306A3C(GMRKeyTable* tbl, f32* keys, f32 frame) {
    s32 stride = 4;
    s32 n;
    s32 half;

    if (tbl->tangent == 0) {
        stride = 3;
    }
    if (frame < keys[0]) {
        return keys[1];
    }
    if (frame >= keys[stride * (tbl->count - 1)]) {
        return keys[stride * (tbl->count - 1) + 1];
    }
    n = tbl->count;
    while (n > 1) {
        half = n / 2;
        if (frame >= keys[half * stride]) {
            keys += half * stride;
            n -= half;
        } else {
            n = half;
        }
    }
    return Hermite(frame, keys[0], keys[1], keys[stride - 1], keys[stride], keys[stride + 1], keys[stride + 2]);
}

/* 0x81306B10: same for s16 keys */
f32 fn_81306B10(GMRKeyTable* tbl, s16* keys, f32 frame) {
    s32 stride = 4;
    s32 n;
    s32 half;

    if (tbl->tangent == 0) {
        stride = 3;
    }
    if (frame < keys[0]) {
        return keys[1];
    }
    if (frame >= keys[stride * (tbl->count - 1)]) {
        return keys[stride * (tbl->count - 1) + 1];
    }
    n = tbl->count;
    while (n > 1) {
        half = n / 2;
        if (frame >= keys[half * stride]) {
            keys += half * stride;
            n -= half;
        } else {
            n = half;
        }
    }
    return Hermite(frame, keys[0], keys[1], keys[stride - 1], keys[stride], keys[stride + 1], keys[stride + 2]);
}

/* 0x81306CD0 */
void fn_81306CD0(GMRAnimOwner* owner) {
    lbl_814AD330 = owner;
    lbl_814AD334 = owner->unk68;
}

/* 0x81306CE0: sample joint transform */
void fn_81306CE0(u16 joint, GMRJointXform* out, f32 frame) {
    f32 v[3];
    s16 r[3];
    GMRAnmTransform* anm = lbl_814AD330->jointAnm;
    GMRJointKeys* tbl = anm->tables;
    u8 shift = anm->rotShift;
    f32* scale = anm->scale;
    s16* rot = anm->rotation;
    f32* trans = anm->translation;
    s32 i;

    for (i = 0; i < 3; i++) {
        switch (tbl[joint * 3 + i].scale.count) {
        case 0:
            v[i] = 1.0f;
            break;
        case 1:
            v[i] = scale[tbl[joint * 3 + i].scale.index];
            break;
        default:
            v[i] = fn_81306A3C(&tbl[joint * 3 + i].scale, &scale[tbl[joint * 3 + i].scale.index], frame);
            break;
        }
    }
    out->scale.x = v[0];
    out->scale.y = v[1];
    out->scale.z = v[2];

    for (i = 0; i < 3; i++) {
        switch (tbl[joint * 3 + i].rotate.count) {
        case 0:
            r[i] = 0;
            break;
        case 1:
            r[i] = rot[tbl[joint * 3 + i].rotate.index] << shift;
            break;
        default:
            r[i] = (s32)fn_81306B10(&tbl[joint * 3 + i].rotate, &rot[tbl[joint * 3 + i].rotate.index], frame) << shift;
            break;
        }
    }
    out->rot[0] = r[0];
    out->rot[1] = r[1];
    out->rot[2] = r[2];

    for (i = 0; i < 3; i++) {
        switch (tbl[joint * 3 + i].translate.count) {
        case 0:
            v[i] = 0.0f;
            break;
        case 1:
            v[i] = trans[tbl[joint * 3 + i].translate.index];
            break;
        default:
            v[i] = fn_81306A3C(&tbl[joint * 3 + i].translate, &trans[tbl[joint * 3 + i].translate.index], frame);
            break;
        }
    }
    out->trans.x = v[0];
    out->trans.y = v[1];
    out->trans.z = v[2];
}

#define CLAMP_U8(v) ((v) < 0.0f ? 0 : (v) > 255.0f ? 255 : (u8)(v))

/* 0x81306F14: sample material color */
void fn_81306F14(u16 mat, GXColor* out, f32 frame) {
    f32 r, g, b, a;
    f32 t = frame / 30.0f;
    GMRAnmColor* anm = lbl_814AD330->colorAnm;
    s16* remap = anm->remap;
    GMRColorKeys* tbl = anm->tables;
    f32* rv = anm->r;
    f32* gv = anm->g;
    f32* bv = anm->b;
    f32* av = anm->a;

    switch (tbl[remap[mat]].r.count) {
    case 0:
        r = 0.0f;
        break;
    case 1:
        r = rv[tbl[remap[mat]].r.index];
        break;
    default:
        r = fn_81306A3C(&tbl[remap[mat]].r, &rv[tbl[remap[mat]].r.index], t);
        break;
    }
    switch (tbl[remap[mat]].g.count) {
    case 0:
        g = 0.0f;
        break;
    case 1:
        g = gv[tbl[remap[mat]].g.index];
        break;
    default:
        g = fn_81306A3C(&tbl[remap[mat]].g, &gv[tbl[remap[mat]].g.index], t);
        break;
    }
    switch (tbl[remap[mat]].b.count) {
    case 0:
        b = 0.0f;
        break;
    case 1:
        b = bv[tbl[remap[mat]].b.index];
        break;
    default:
        b = fn_81306A3C(&tbl[remap[mat]].b, &bv[tbl[remap[mat]].b.index], t);
        break;
    }
    switch (tbl[remap[mat]].a.count) {
    case 0:
        a = 0.0f;
        break;
    case 1:
        a = av[tbl[remap[mat]].a.index];
        break;
    default:
        a = fn_81306A3C(&tbl[remap[mat]].a, &av[tbl[remap[mat]].a.index], t);
        break;
    }
    out->r = CLAMP_U8(r);
    out->g = CLAMP_U8(g);
    out->b = CLAMP_U8(b);
    out->a = CLAMP_U8(a);
}
