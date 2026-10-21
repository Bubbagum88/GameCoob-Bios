#ifndef BS2_J3D_H
#define BS2_J3D_H

#include <dolphin/types.h>
#include <dolphin/mtx.h>

/* BS2's own small model loader (J3D_* at 0x813034E4..). Only the fields the
   decompiled code touches are named. */
typedef struct J3DMaterial {
    u8 pad0[0x30];
    void* tevColor;        /* 0x30 GXColorS10 used by the TEV stage */
} J3DMaterial;

typedef struct J3DModel {
    u8 pad0[0x10];
    J3DMaterial* material; /* 0x10 */
    u8 pad14[0x38 - 0x14];
} J3DModel;

typedef struct J3DModelInstance {
    u8 pad0[0x68];
    J3DModel* model;       /* 0x68 */
    u8 pad6C[0x74 - 0x6C];
    s16 alpha;             /* 0x74 */
    u8 pad76[0x78 - 0x76];
} J3DModelInstance;

void J3D_InitModel(J3DModelInstance* inst, s32 flags);                 /* 0x81303A38 */
void J3D_CopyInstance(J3DModelInstance* dst, J3DModelInstance* src);   /* 0x81303A9C */
void J3D_CalcJoints(J3DModelInstance* inst);                           /* 0x8130418C */
void J3D_Draw(J3DModelInstance* inst);                                 /* 0x813051AC */
void J3D_SetModelMtx(J3DModelInstance* inst, Mtx m, Vec* scale);       /* 0x8130537C */
void J3D_SetViewMtx(J3DModelInstance* inst, MtxPtr view);              /* 0x813053C8 */

/* BS2 math helpers */
void MtxRotTrans(s16 rx, s16 ry, s16 rz, Mtx m, f32 tx, f32 ty, f32 tz); /* 0x813073B4 */
f32 SinS(s32 angle);                                                    /* 0x81307CD0 */
f32 CosS(s32 angle);                                                    /* 0x81307CEC */

#endif
