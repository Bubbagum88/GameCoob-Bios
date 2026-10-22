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

/* scene graph node built from INF1 (0x18) */
typedef struct J3DNode {
    s32 type;                  /* 0x00 0x10 joint, 0x11 material, 0x12 shape */
    struct J3DNode* parent;    /* 0x04 */
    struct J3DNode* child;     /* 0x08 */
    struct J3DNode* prev;      /* 0x0C */
    struct J3DNode* next;      /* 0x10 */
    u16 index;                 /* 0x14 */
} J3DNode;

/* BMD block header (every chunk starts with tag + size) */
typedef struct J3DBlock {
    u32 tag;                   /* 0x00 */
    u32 size;                  /* 0x04 */
} J3DBlock;

/* BMD file header */
typedef struct J3DFileHeader {
    u32 magic;                 /* 0x00 'J3D1' */
    u32 type;                  /* 0x04 'bmd1' */
    s32 size;                  /* 0x08 */
    u8 pad0C[0x20 - 0xC];
} J3DFileHeader;

typedef struct J3DTransformInfo {
    f32 sx, sy, sz;
    s16 rx, ry, rz;
    s16 pad;
    f32 tx, ty, tz;
} J3DTransformInfo;

/* JNT1 joint entry (0x40) */
typedef struct J3DJointData {
    u16 flags;                 /* 0x00 */
    u8 pad02[2];
    J3DTransformInfo xf;       /* 0x04 */
    u8 pad24[0x40 - 0x24];
} J3DJointData;

/* runtime joint (0x24) */
typedef struct J3DJoint {
    u16 flags;                 /* 0x00 */
    J3DTransformInfo xf;       /* 0x04 */
} J3DJoint;

/* JNT1 / MAT / SHP1 common head: tag, size, count */
typedef struct J3DJointBlock {
    u32 tag;
    u32 size;
    u16 count;                 /* 0x08 */
    u8 pad0A[2];
    J3DJointData* joints;      /* 0x0C */
    u16* remap;                /* 0x10 */
} J3DJointBlock;

/* SHP1 shape entry (0x28) */
typedef struct J3DShapeData {
    u8 mtxType;                /* 0x00 */
    u8 pad01;
    u16 numPackets;            /* 0x02 */
    u16 vtxDescOffset;         /* 0x04 */
    u16 firstMtxData;          /* 0x06 */
    u16 firstPacket;           /* 0x08 */
    u8 pad0A[0x28 - 0xA];
} J3DShapeData;

typedef struct J3DShapeMtxData {
    u16 unk0;
    u16 count;
    u32 first;
} J3DShapeMtxData;

typedef struct J3DShapePacketLoc {
    u32 size;
    u32 offset;
} J3DShapePacketLoc;

typedef struct J3DShapeBlock {
    u32 tag;
    u32 size;
    u16 count;                 /* 0x08 */
    u8 pad0A[2];
    J3DShapeData* shapes;      /* 0x0C */
    u16* remap;                /* 0x10 */
    void* names;               /* 0x14 */
    u8* vtxDesc;               /* 0x18 */
    u16* mtxTable;             /* 0x1C */
    u8* dlData;                /* 0x20 */
    J3DShapeMtxData* mtxData;  /* 0x24 */
    J3DShapePacketLoc* packetLoc; /* 0x28 */
} J3DShapeBlock;

typedef struct J3DShapeMtxGroup {
    u16 unk0;
    u16 count;
    u16* table;
} J3DShapeMtxGroup;

typedef struct J3DShapePacket {
    u32 size;
    u8* dl;
} J3DShapePacket;

/* runtime shape (0x10) */
typedef struct J3DShape {
    u8 mtxType;                /* 0x00 */
    u16 numPackets;            /* 0x02 */
    u8* vtxDesc;               /* 0x04 */
    J3DShapeMtxGroup* mtxGroups; /* 0x08 */
    J3DShapePacket* packets;   /* 0x0C */
} J3DShape;

typedef struct J3DModel {
    void* data;            /* 0x00 BMD resource */
    u8 pad4[0x8 - 0x4];
    u8* packets;           /* 0x08 scene graph nodes (J3DNode[]) */
    J3DJoint* joints;      /* 0x0C */
    J3DMaterial* material; /* 0x10 */
    J3DShape* shapes;      /* 0x14 */
    void* inf1;            /* 0x18 */
    void* vtx1;            /* 0x1C */
    void* evp1;            /* 0x20 */
    void* drw1;            /* 0x24 */
    J3DJointBlock* jnt1;   /* 0x28 */
    void* mat;             /* 0x2C */
    J3DShapeBlock* shp1;   /* 0x30 */
    void* tex1;            /* 0x34 */
} J3DModel;

typedef struct J3DAnim {
    u16 loopMode;          /* 0x00 */
    s16 duration;          /* 0x02 */
    void* p4;              /* 0x04 */
    void* p8;              /* 0x08 */
    void* pC;              /* 0x0C */
    void* p10;             /* 0x10 */
    void* p14;             /* 0x14 */
    void* p18;             /* 0x18 */
    void* data;            /* 0x1C */
} J3DAnim;

typedef struct J3DModelInstance {
    void* jointMtx;        /* 0x00 */
    u8 pad4[0x64 - 0x4];
    void* animWork;        /* 0x64 */
    J3DModel* model;       /* 0x68 */
    void* unk6C;           /* 0x6C */
    J3DAnim* anim;         /* 0x70 */
    s16 alpha;             /* 0x74 */
    s16 unk76;             /* 0x76 */
} J3DModelInstance;

void J3D_InitModel(J3DModelInstance* inst, s32 flags);                 /* 0x81303A38 */
void J3D_CopyInstance(J3DModelInstance* dst, J3DModelInstance* src);   /* 0x81303A9C */
void J3D_CalcJoints(J3DModelInstance* inst);                           /* 0x8130418C */
void J3D_Draw(J3DModelInstance* inst);                                 /* 0x813051AC */
void J3D_SetModelMtx(J3DModelInstance* inst, Mtx m, Vec scale);       /* 0x8130537C */
void J3D_SetViewMtx(J3DModelInstance* inst, MtxPtr view);              /* 0x813053C8 */

/* BS2 math helpers */
void MtxRotTrans(s16 rx, s16 ry, s16 rz, Mtx m, f32 tx, f32 ty, f32 tz); /* 0x813073B4 */
f32 SinS(s16 angle);                                                    /* 0x81307CD0 */
f32 CosS(s16 angle);                                                    /* 0x81307CEC */
void MtxRot(s16 rx, s16 ry, s16 rz, Mtx m);                             /* 0x81307510 */
f32 Hermite(f32 t, f32 t0, f32 p0, f32 v0, f32 t1, f32 p1, f32 v1);     /* 0x81307D08 */
u32 Rand(void);                                                         /* 0x81307EF8 */

#endif
