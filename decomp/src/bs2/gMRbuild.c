// BS2 model renderer: J3DBinary (BMD) loading, offset fixups and scene
// graph construction (PAL 1.0 BS2, 0x81303204..0x81303CAC).

#include <dolphin/os.h>
#include "bs2/j3d.h"

void* HeapAlloc(u32 size);                                   /* 0x81307E40 */
void fn_813053F8(J3DNode* node);                             /* gMRshape.c */
void fn_81305410(J3DNode* parent, J3DNode* node);            /* gMRshape.c */
J3DNode* fn_813054D4(J3DNode* node);                         /* gMRshape.c */
void fn_8130555C(J3DJointBlock* blk, J3DJoint* out);         /* gMRshape.c */
void fn_813055F4(J3DShapeBlock* blk, J3DShape* out);         /* gMRshape.c */
void fn_81305824(void* mat, J3DMaterial* out);               /* 0x81305824 */

#define RELOC(field, base)                                   \
    if ((field) != 0) {                                      \
        (field) = (u32)(field) + (u32)(base);                \
    }

typedef struct VtxBlock {
    u32 tag;
    u32 size;
    u32 fmt;          /* 0x08 */
    u32 pos;          /* 0x0C */
    u32 nrm;          /* 0x10 */
    u32 nbt;          /* 0x14 */
    u32 color[2];     /* 0x18 */
    u32 texCoord[8];  /* 0x20 */
} VtxBlock;

void fn_81303204(VtxBlock* blk) {
    s32 i;
    RELOC(blk->fmt, blk);
    RELOC(blk->pos, blk);
    RELOC(blk->nrm, blk);
    RELOC(blk->nbt, blk);
    RELOC(blk->color[0], blk);
    RELOC(blk->color[1], blk);
    for (i = 0; i < 8; i++) {
        RELOC(blk->texCoord[i], blk);
    }
}

typedef struct MatBlock {
    u32 tag;
    u32 size;
    u16 count;        /* 0x08 */
    u8 pad0A[2];
    u32 offs[22];     /* 0x0C..0x60, [2] (0x14) is the name table */
} MatBlock;

void fn_8130333C(MatBlock* blk) {
    RELOC(blk->offs[0], blk);
    RELOC(blk->offs[1], blk);
    RELOC(blk->offs[3], blk);
    RELOC(blk->offs[4], blk);
    RELOC(blk->offs[5], blk);
    RELOC(blk->offs[6], blk);
    RELOC(blk->offs[7], blk);
    RELOC(blk->offs[8], blk);
    RELOC(blk->offs[9], blk);
    RELOC(blk->offs[10], blk);
    RELOC(blk->offs[11], blk);
    RELOC(blk->offs[12], blk);
    RELOC(blk->offs[13], blk);
    RELOC(blk->offs[14], blk);
    RELOC(blk->offs[15], blk);
    RELOC(blk->offs[16], blk);
    RELOC(blk->offs[17], blk);
    RELOC(blk->offs[18], blk);
    RELOC(blk->offs[19], blk);
    RELOC(blk->offs[20], blk);
    RELOC(blk->offs[21], blk);
}

void fn_81303974(J3DModel* model);

#define ALIGN32(x) (((x) + 0x1F) & ~0x1F)

void J3D_LoadModelData(J3DModel* model) {
    J3DFileHeader* hdr = model->data;
    u8* blk;
    s32 size = hdr->size - 0x20;

    if (hdr->magic != 'J3D1' || hdr->type != 'bmd1') {
        model->data = NULL;
        OSPanic("gMRbuild.c", 154, "ERROR: This renderer ONLY supports J3DBinary1.5\n");
        return;
    }

    blk = (u8*)hdr + 0x20;
    while (size > 0) {
        switch (((J3DBlock*)blk)->tag) {
        case 'INF1':
            model->inf1 = blk;
            RELOC(((u32*)blk)[0x14 / 4], blk);
            break;
        case 'VTX1':
            model->vtx1 = blk;
            fn_81303204((VtxBlock*)blk);
            break;
        case 'EVP1':
            model->evp1 = blk;
            RELOC(((u32*)blk)[0xC / 4], blk);
            RELOC(((u32*)blk)[0x10 / 4], blk);
            RELOC(((u32*)blk)[0x14 / 4], blk);
            RELOC(((u32*)blk)[0x18 / 4], blk);
            break;
        case 'DRW1':
            model->drw1 = blk;
            RELOC(((u32*)blk)[0xC / 4], blk);
            RELOC(((u32*)blk)[0x10 / 4], blk);
            break;
        case 'JNT1':
            model->jnt1 = (J3DJointBlock*)blk;
            RELOC(((u32*)blk)[0xC / 4], blk);
            RELOC(((u32*)blk)[0x10 / 4], blk);
            break;
        case 'MAT1':
            model->mat = blk;
            fn_8130333C((MatBlock*)blk);
            break;
        case 'SHP1':
            model->shp1 = (J3DShapeBlock*)blk;
            RELOC(((u32*)blk)[0xC / 4], blk);
            RELOC(((u32*)blk)[0x10 / 4], blk);
            RELOC(((u32*)blk)[0x18 / 4], blk);
            RELOC(((u32*)blk)[0x1C / 4], blk);
            RELOC(((u32*)blk)[0x20 / 4], blk);
            RELOC(((u32*)blk)[0x24 / 4], blk);
            RELOC(((u32*)blk)[0x28 / 4], blk);
            break;
        case 'TEX1':
            model->tex1 = blk;
            RELOC(((u32*)blk)[0xC / 4], blk);
            break;
        default:
            if (size > 0) {
                size = 0;
            }
            break;
        }
        if (size > 0) {
            size -= ALIGN32(((J3DBlock*)blk)->size);
        }
        blk += ALIGN32(((J3DBlock*)blk)->size);
    }
    fn_81303974(model);
}

typedef struct InfEntry {
    u16 type;
    u16 index;
} InfEntry;

void fn_81303838(InfEntry* inf, J3DModel* model) {
    J3DNode* nodes = (J3DNode*)model->packets;
    InfEntry* e = inf;
    s32 n = 0;
    J3DNode* cur = NULL;
    J3DNode* node = nodes;

    for (; e->type != 0; e++) {
        switch (e->type) {
        case 1:
            cur = node;
            break;
        case 2:
            cur = fn_813054D4(cur);
            break;
        case 0x10:
            node = &nodes[n++];
            fn_813053F8(node);
            node->type = 0x10;
            node->index = e->index;
            fn_81305410(cur, node);
            break;
        case 0x11:
            node = &nodes[n++];
            fn_813053F8(node);
            node->type = 0x11;
            node->index = e->index;
            fn_81305410(cur, node);
            break;
        case 0x12:
            node = &nodes[n++];
            fn_813053F8(node);
            node->type = 0x12;
            node->index = e->index;
            fn_81305410(cur, node);
            break;
        }
    }
}

void fn_81303974(J3DModel* model) {
    model->packets = HeapAlloc((model->jnt1->count + model->shp1->count + ((J3DJointBlock*)model->mat)->count) * sizeof(J3DNode));
    model->joints = HeapAlloc(model->jnt1->count * sizeof(J3DJoint));
    model->material = HeapAlloc(model->shp1->count * 0xD8);
    model->shapes = HeapAlloc(((J3DJointBlock*)model->mat)->count * sizeof(J3DShape));
    fn_8130555C(model->jnt1, model->joints);
    fn_81305824(model->mat, model->material);
    fn_813055F4(model->shp1, model->shapes);
    fn_81303838(*(InfEntry**)((u8*)model->inf1 + 0x14), model);
}

void J3D_InitModel(J3DModelInstance* inst, s32 flags) {
    if (flags) {
        J3D_LoadModelData(inst->model);
    }
    inst->jointMtx = HeapAlloc(inst->model->jnt1->count * 0x30);
    inst->anim = NULL;
    inst->unk6C = NULL;
    inst->animWork = NULL;
    inst->unk76 = 0;
}

void J3D_CopyInstance(J3DModelInstance* dst, J3DModelInstance* src) {
    *dst = *src;
    dst->jointMtx = HeapAlloc(dst->model->jnt1->count * 0x30);
}

typedef struct AnkBlock {
    u32 tag;
    u32 size;
    u8 loopMode;      /* 0x08 */
    u8 angleMul;      /* 0x09 */
    s16 duration;     /* 0x0A */
    u16 numJoints;    /* 0x0C */
    u16 scaleCount;
    u16 rotCount;
    u16 transCount;
    u32 joints;       /* 0x14 */
    u32 scales;       /* 0x18 */
    u32 rots;         /* 0x1C */
    u32 trans;        /* 0x20 */
} AnkBlock;

typedef struct J3DAnkAnim {
    u8 loopMode;      /* 0x00 */
    u8 angleMul;      /* 0x01 */
    s16 duration;     /* 0x02 */
    u32 joints;       /* 0x04 */
    u32 scales;       /* 0x08 */
    u32 rots;         /* 0x0C */
    u32 trans;        /* 0x10 */
    u8* data;         /* 0x14 */
} J3DAnkAnim;

void fn_81303B00(J3DAnkAnim* anim, s32 reloc) {
    AnkBlock* blk = (AnkBlock*)(anim->data + 0x20);
    if (reloc) {
        RELOC(blk->joints, blk);
        RELOC(blk->scales, blk);
        RELOC(blk->rots, blk);
        RELOC(blk->trans, blk);
    }
    anim->loopMode = blk->loopMode;
    anim->angleMul = blk->angleMul;
    anim->duration = blk->duration;
    anim->joints = blk->joints;
    anim->scales = blk->scales;
    anim->rots = blk->rots;
    anim->trans = blk->trans;
}

void fn_81303B9C(J3DModelInstance* inst, void* p) {
    inst->unk6C = p;
}

typedef struct AnimBlock {
    u32 tag;
    u32 size;
    u16 loopMode;     /* 0x08 */
    s16 duration;     /* 0x0A */
    u32 c;            /* 0x0C */
    u32 d10;          /* 0x10 */
    u32 d14;          /* 0x14 */
    u32 d18;          /* 0x18 */
    u32 d1C;          /* 0x1C */
    u32 d20;          /* 0x20 */
} AnimBlock;

void fn_81303BA4(J3DModelInstance* inst, J3DAnim* anim, s32 reloc) {
    AnimBlock* blk = anim->data;
    u16 count = (blk->d10 - blk->c) >> 1;

    if (reloc) {
        RELOC(blk->c, blk);
        RELOC(blk->d14, blk);
        RELOC(blk->d18, blk);
        RELOC(blk->d1C, blk);
        RELOC(blk->d20, blk);
        RELOC(blk->d10, blk);
    }
    inst->anim = anim;
    anim->loopMode = blk->loopMode;
    anim->duration = blk->duration;
    anim->p4 = (void*)blk->c;
    anim->pC = (void*)blk->d14;
    anim->p10 = (void*)blk->d18;
    anim->p14 = (void*)blk->d1C;
    anim->p18 = (void*)blk->d20;
    anim->p8 = (void*)blk->d10;
    inst->animWork = HeapAlloc(count * 4);
}
