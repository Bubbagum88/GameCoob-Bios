#ifndef _DOLPHIN_MTX_H_
#define _DOLPHIN_MTX_H_

#include <dolphin/types.h>

typedef struct Vec {
    f32 x, y, z;
} Vec, *VecPtr, Point3d, *Point3dPtr;

typedef f32 Mtx[3][4];
typedef f32 (*MtxPtr)[4];
typedef f32 Mtx44[4][4];
typedef f32 (*Mtx44Ptr)[4];

#define MTXDegToRad(a) ((a) * 0.01745329252f)

void PSMTXIdentity(Mtx m);
void PSMTXCopy(const Mtx src, Mtx dst);
void PSMTXConcat(const Mtx a, const Mtx b, Mtx ab);
void PSMTXTranspose(const Mtx src, Mtx xPose);
u32 PSMTXInverse(const Mtx src, Mtx inv);
void PSMTXTrans(Mtx m, f32 xT, f32 yT, f32 zT);
void PSMTXScale(Mtx m, f32 xS, f32 yS, f32 zS);
void C_MTXLookAt(Mtx m, const Point3d* camPos, const Vec* camUp, const Point3d* target);
void PSMTXMultVec(const Mtx m, const Vec* src, Vec* dst);
void C_MTXPerspective(Mtx44 m, f32 fovY, f32 aspect, f32 n, f32 f);
void C_MTXOrtho(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f);
void PSVECSubtract(const Vec* a, const Vec* b, Vec* ab);
void PSVECNormalize(const Vec* src, Vec* unit);
void PSVECCrossProduct(const Vec* a, const Vec* b, Vec* axb);

#define MTXIdentity PSMTXIdentity
#define MTXCopy PSMTXCopy
#define MTXConcat PSMTXConcat
#define MTXTranspose PSMTXTranspose
#define MTXInverse PSMTXInverse
#define MTXTrans PSMTXTrans
#define MTXScale PSMTXScale
#define MTXLookAt C_MTXLookAt
#define MTXMultVec PSMTXMultVec
#define MTXPerspective C_MTXPerspective
#define MTXOrtho C_MTXOrtho
#define VECSubtract PSVECSubtract
#define VECNormalize PSVECNormalize
#define VECCrossProduct PSVECCrossProduct

#endif
