#ifndef BS2_GMRTEX_H
#define BS2_GMRTEX_H

#include <dolphin/types.h>
#include <dolphin/mtx.h>
#include <dolphin/gx.h>

/* Types for gMRtexture.c / gMRanim.c (BS2's small J3D renderer). Only the
   fields touched there are named. */

/* J3D key table header (count, first key index, tangent mode) */
typedef struct GMRKeyTable {
    u16 count;   /* 0x0 */
    u16 index;   /* 0x2 */
    u16 tangent; /* 0x4 0: shared tangent (3 values/key), else in+out (4) */
} GMRKeyTable;

typedef struct GMRJointKeys {
    GMRKeyTable scale;     /* 0x0 */
    GMRKeyTable rotate;    /* 0x6 */
    GMRKeyTable translate; /* 0xC */
} GMRJointKeys;            /* 0x12, three per joint (x, y, z) */

typedef struct GMRAnmTransform {
    u8 unk0;
    u8 rotShift;           /* 0x01 */
    u8 pad2[2];
    GMRJointKeys* tables;  /* 0x04 */
    f32* scale;            /* 0x08 */
    s16* rotation;         /* 0x0C */
    f32* translation;      /* 0x10 */
} GMRAnmTransform;

typedef struct GMRColorKeys {
    GMRKeyTable r, g, b, a; /* 0x00 */
} GMRColorKeys;             /* 0x18 */

typedef struct GMRAnmColor {
    u8 pad0[4];
    s16* remap;            /* 0x04 */
    GMRColorKeys* tables;  /* 0x08 */
    f32* r;                /* 0x0C */
    f32* g;                /* 0x10 */
    f32* b;                /* 0x14 */
    f32* a;                /* 0x18 */
} GMRAnmColor;

typedef struct GMRAnimOwner {
    u8 pad0[0x68];
    void* unk68;               /* 0x68 */
    GMRAnmTransform* jointAnm; /* 0x6C */
    GMRAnmColor* colorAnm;     /* 0x70 */
} GMRAnimOwner;

typedef struct GMRJointXform {
    Vec scale;   /* 0x00 */
    s16 rot[3];  /* 0x0C */
    u8 pad12[2];
    Vec trans;   /* 0x14 */
} GMRJointXform;

/* BTI texture header (ResTIMG) */
typedef struct GMRTexImg {
    u8 format;         /* 0x00 */
    u8 alpha;          /* 0x01 */
    u16 width;         /* 0x02 */
    u16 height;        /* 0x04 */
    u8 wrapS;          /* 0x06 */
    u8 wrapT;          /* 0x07 */
    u8 indexed;        /* 0x08 */
    u8 tlutFormat;     /* 0x09 */
    u16 numColors;     /* 0x0A */
    s32 tlutOffset;    /* 0x0C */
    u8 mipmap;         /* 0x10 */
    u8 edgeLOD;        /* 0x11 */
    u8 biasClamp;      /* 0x12 */
    u8 maxAniso;       /* 0x13 */
    u8 minFilter;      /* 0x14 */
    u8 magFilter;      /* 0x15 */
    s8 minLOD;         /* 0x16 */
    s8 maxLOD;         /* 0x17 */
    u8 numMips;        /* 0x18 */
    u8 pad19;
    s16 lodBias;       /* 0x1A */
    s32 imageOffset;   /* 0x1C */
} GMRTexImg;           /* 0x20 */

typedef struct GMRChanCtrl {
    u8 enable, ambSrc, matSrc, lightMask, diffFn, attnFn;
    u8 pad[2];
} GMRChanCtrl;         /* 0x8 */

typedef struct GMRTexGen {
    u8 type, src, mtx, pad;
} GMRTexGen;           /* 0x4 */

typedef struct GMRTexMtx {
    Mtx mtx;           /* 0x00 */
    u8 type;           /* 0x30 */
    u8 pad[3];
} GMRTexMtx;           /* 0x34 */

typedef struct GMRTevOrder {
    u8 coord, map, color, pad;
} GMRTevOrder;         /* 0x4 */

typedef struct GMRTevStage {
    u8 op;             /* 0x00 <5: GXSetTevOp mode */
    u8 cIn[4];         /* 0x01 */
    u8 cOp[5];         /* 0x05 */
    u8 aIn[4];         /* 0x0A */
    u8 aOp[5];         /* 0x0E */
    u8 pad[1];
} GMRTevStage;         /* 0x14 */

typedef struct GMRUnk2C { u8 data[0x2C]; } GMRUnk2C;
typedef struct GMRUnk8 { u8 data[0x8]; } GMRUnk8;
typedef struct GMRUnk4 { u8 data[0x4]; } GMRUnk4;

typedef struct GMRVtxNrm {
    u8 isNBT;          /* 0x00 */
    u8 pad[3];
    u8 unk4[0xC];      /* 0x04 */
} GMRVtxNrm;           /* 0x10 */

/* MAT3 material init entry */
typedef struct GMRMatInit {
    u8 flag;           /* 0x00 */
    u8 cull;           /* 0x01 */
    u8 numChans;       /* 0x02 */
    u8 numTexGens;     /* 0x03 */
    u8 numTevStages;   /* 0x04 */
    u8 unk5;           /* 0x05 */
    u8 unk6;           /* 0x06 */
    u8 unk7;           /* 0x07 */
    u16 matColor[2];   /* 0x08 */
    u16 colorChan[4];  /* 0x0C */
    u16 texGen[8];     /* 0x14 */
    u16 texMtx[10];    /* 0x24 */
    u16 texNo[8];      /* 0x38 */
    u16 tevOrder[8];   /* 0x48 */
    u8 pad58[0x10];    /* 0x58 */
    u16 tevColor[4];   /* 0x68 */
    u16 tevStage[8];   /* 0x70 */
    u8 pad80[0x10];    /* 0x80 */
    u16 unk90;         /* 0x90 */
    u16 unk92;         /* 0x92 */
    u16 unk94;         /* 0x94 */
    u16 vtxNrm;        /* 0x96 */
} GMRMatInit;          /* 0x98 */

/* MAT3 block */
typedef struct GMRMatBlock {
    u8 pad0[8];
    u16 count;             /* 0x08 */
    u8 padA[2];
    GMRMatInit* init;      /* 0x0C */
    u16* remap;            /* 0x10 */
    u8 pad14[4];
    u32* cull;             /* 0x18 */
    GXColor* matColor;     /* 0x1C */
    u8* numChans;          /* 0x20 */
    GMRChanCtrl* colorChan; /* 0x24 */
    u8* numTexGens;        /* 0x28 */
    GMRTexGen* texGen;     /* 0x2C */
    GMRTexMtx* texMtx;     /* 0x30 */
    u16* texNo;            /* 0x34 */
    GMRTevOrder* tevOrder; /* 0x38 */
    GXColorS10* tevColor;  /* 0x3C */
    u8* numTevStages;      /* 0x40 */
    GMRTevStage* tevStage; /* 0x44 */
    GMRUnk2C* unk48;       /* 0x48 */
    GMRUnk8* unk4C;        /* 0x4C */
    GMRUnk4* unk50;        /* 0x50 */
    u8 pad54[4];
    u32* unk58;            /* 0x58 */
    u32* unk5C;            /* 0x5C */
    GMRVtxNrm* vtxNrm;     /* 0x60 */
} GMRMatBlock;

/* runtime material, built by fn_81305824 */
typedef struct GMRMaterial {
    u8 flag;                  /* 0x00 bit0: z-update */
    u8 cull;                  /* 0x01 */
    u8 numChans;              /* 0x02 */
    u8 numTexGens;            /* 0x03 */
    u8 numTevStages;          /* 0x04 */
    u8 unk5;                  /* 0x05 */
    u8 unk6;                  /* 0x06 */
    u8 unk7;                  /* 0x07 */
    u16 texNo[8];             /* 0x08 */
    GXColor* matColor[2];     /* 0x18 */
    GMRChanCtrl* colorChan[4]; /* 0x20 */
    GXColorS10* tevColor[4];  /* 0x30 */
    GMRTexGen* texGen[8];     /* 0x40 */
    GMRTevOrder* tevOrder[8]; /* 0x60 */
    GMRTevStage* tevStage[8]; /* 0x80 */
    GMRTexMtx* texMtx[10];    /* 0xA0 */
    GMRUnk2C* unkC8;          /* 0xC8 */
    GMRUnk8* unkCC;           /* 0xCC */
    GMRUnk4* unkD0;           /* 0xD0 */
    GMRVtxNrm* vtxNrm;        /* 0xD4 */
} GMRMaterial;                /* 0xD8 */

typedef struct GMRVtxInfo {
    u8 pad0[0x18];
    s32 nrmType;              /* 0x18 */
    u8 nrmFrac;               /* 0x1C */
} GMRVtxInfo;

typedef struct GMRVtxBlock {
    u8 pad0[8];
    GMRVtxInfo* info;         /* 0x08 */
    u8 padC[4];
    void* nrmArray;           /* 0x10 */
    void* nbtArray;           /* 0x14 */
} GMRVtxBlock;

typedef struct GMRTexBlock {
    u8 pad0[0xC];
    GMRTexImg* images;        /* 0x0C */
} GMRTexBlock;

typedef struct GMRModelData {
    u8 pad0[0x10];
    GMRMaterial* materials;   /* 0x10 */
    u8 pad14[8];
    GMRVtxBlock* vtx;         /* 0x1C */
    u8 pad20[0x14];
    GMRTexBlock* tex;         /* 0x34 */
} GMRModelData;

typedef struct GMRInstance {
    u8 pad0[0x68];
    GMRModelData* model;      /* 0x68 */
    u8 pad6C[8];
    s16 alpha;                /* 0x74 */
} GMRInstance;

#endif
