// BS2's own small J3D-binary model renderer ("gMR"): joint matrix calc,
// vertex array setup and shape/packet drawing
// (PAL 1.0 BS2, 0x81303CAC..0x813053F8).

#include <dolphin/types.h>
#include <dolphin/mtx.h>
#include <dolphin/gx.h>

/* Scene graph node (INF1 hierarchy entry). */
typedef struct GMRNode {
    u32 type;                /* 0x00 0x10 joint, 0x11 material, 0x12 shape */
    u32 pad4;
    struct GMRNode* child;   /* 0x08 */
    u32 padC;
    struct GMRNode* next;    /* 0x10 */
    u16 index;               /* 0x14 */
} GMRNode;

typedef struct GMRTransform {
    Vec scale;               /* 0x00 */
    s16 rx, ry, rz;          /* 0x0C */
    s16 pad12;
    Vec trans;               /* 0x14 */
} GMRTransform;

typedef struct GMRJoint {
    u32 pad0;
    GMRTransform xf;         /* 0x04 */
} GMRJoint; /* size 0x24 */

typedef struct GMRTexGen {
    u8 pad0;
    u8 type;                 /* 0x01 */
    u8 mtx;                  /* 0x02 GX_TEXMTXn */
} GMRTexGen;

typedef struct GMRTexMtx {
    Mtx mtx;                 /* 0x00 */
    u8 type;                 /* 0x30 */
} GMRTexMtx;

typedef struct GMRMaterial {
    u8 pad0[3];
    u8 numTexGens;           /* 0x03 */
    u8 pad4[0x18 - 0x4];
    void* unk18;             /* 0x18 */
    u8 pad1C[0x40 - 0x1C];
    GMRTexGen* texGens[8];   /* 0x40 */
    u8 pad60[0xA0 - 0x60];
    GMRTexMtx* texMtx[10];   /* 0xA0 */
    u8 padC8[0xD8 - 0xC8];
} GMRMaterial;

typedef struct GMRDisplayList {
    u32 size;                /* 0x00 */
    void* data;              /* 0x04 */
} GMRDisplayList;

typedef struct GMRShape {
    u8 mtxType;              /* 0x00 */
    u8 pad1[3];
    GXVtxDescList* vtxDesc;  /* 0x04 */
    u16* mtxIndex;           /* 0x08 */
    GMRDisplayList* dl;      /* 0x0C */
} GMRShape;

typedef struct GMRVtxData {
    u8 pad0[8];
    GXVtxAttrFmtList* fmt;   /* 0x08 */
    void* pos;               /* 0x0C */
    void* nrm;               /* 0x10 */
    u8 pad14[4];
    void* color[2];          /* 0x18 */
    void* tex[8];            /* 0x20 */
} GMRVtxData;

typedef struct GMRDrawData {
    u8 pad0[0x10];
    u16* jointIndex;         /* 0x10 */
} GMRDrawData;

typedef struct GMRMatTable {
    u8 pad0[8];
    u16 count;               /* 0x08 */
} GMRMatTable;

typedef struct GMRModel {
    void* data;              /* 0x00 */
    u8 pad4[4];
    GMRNode* root;           /* 0x08 */
    GMRJoint* joints;        /* 0x0C */
    GMRMaterial* materials;  /* 0x10 */
    GMRShape* shapes;        /* 0x14 */
    u8 pad18[4];
    GMRVtxData* vtx;         /* 0x1C */
    u8 pad20[4];
    GMRDrawData* draw;       /* 0x24 */
    u8 pad28[4];
    GMRMatTable* matTable;   /* 0x2C */
} GMRModel;

typedef struct GMRInstance {
    Mtx* jointMtx;           /* 0x00 */
    Mtx baseMtx;             /* 0x04 */
    Mtx curMtx;              /* 0x34 */
    u32* matAnm;             /* 0x64 */
    GMRModel* model;         /* 0x68 */
    u8 pad6C[4];
    u32 animMat;             /* 0x70 */
} GMRInstance;

typedef struct GMRPacket {
    u8 pad0[0x14];
    u16 matIndex;            /* 0x14 */
    u16 pad16;
    GMRNode node;            /* 0x18 */
} GMRPacket;

void MtxRotTrans(s16 rx, s16 ry, s16 rz, Mtx m, f32 tx, f32 ty, f32 tz); /* 0x813073B4 */
void MtxScaleCols(Mtx m, Vec* s);                                       /* 0x81307754 */
void MtxInverse33(Mtx in, Mtx out);                                     /* 0x81307820 */
void fn_81305F48(GMRInstance* inst);                                    /* 0x81305F48 */
void fn_813061EC(u16 mat);                                              /* 0x813061EC */
void fn_81306850(u16 mat);                                              /* 0x81306850 */
Vec* fn_81306A34(void);                                                 /* 0x81306A34 */
void fn_81306CD0(GMRInstance* inst);                                     /* 0x81306CD0 */
void fn_81306CE0(u16 joint, GMRTransform* xf, f32 frame);               /* 0x81306CE0 */
void fn_81306F14(u16 mat, u32* out, f32 frame);                         /* 0x81306F14 */

/* real SDK value (GXEnum.h here has MTX2x4/MTX3x4 swapped) */
#define TG_MTX2x4 ((GXTexGenType)1)

u16 lbl_814ACFD8 = 0xFFFF;

/* CW lays out .sbss in reverse definition order */
GMRInstance* lbl_814AD31C; /* current instance */
GMRModel* lbl_814AD318;    /* current model */

void fn_81303CAC(u16 idx, GMRTransform* xf) {
    Mtx m;

    MtxRotTrans(xf->rx, xf->ry, xf->rz, m, xf->trans.x, xf->trans.y, xf->trans.z);
    if (xf->scale.x != 1.0f || xf->scale.y != 1.0f || xf->scale.z != 1.0f) {
        m[0][0] *= xf->scale.x;
        m[0][1] *= xf->scale.y;
        m[0][2] *= xf->scale.z;
        m[1][0] *= xf->scale.x;
        m[1][1] *= xf->scale.y;
        m[1][2] *= xf->scale.z;
        m[2][0] *= xf->scale.x;
        m[2][1] *= xf->scale.y;
        m[2][2] *= xf->scale.z;
    }
    PSMTXConcat(lbl_814AD31C->curMtx, m, lbl_814AD31C->curMtx);
    PSMTXCopy(lbl_814AD31C->curMtx, lbl_814AD31C->jointMtx[idx]);
}

void fn_81303DE4(void) {
    GXVtxAttrFmtList* fmt;
    u8 stride;

    for (fmt = lbl_814AD318->vtx->fmt; fmt->attr != GX_VA_NULL; fmt++) {
        switch (fmt->attr) {
        case GX_VA_POS:
            switch (fmt->count) {
            case GX_POS_XY:
                stride = 2;
                break;
            case GX_POS_XYZ:
            default:
                stride = 3;
                break;
            }
            switch (fmt->type) {
            case GX_U16:
                stride *= 2;
                break;
            case GX_S16:
                stride *= 2;
                break;
            case GX_F32:
                stride *= 4;
                break;
            }
            GXSetArray(GX_VA_POS, lbl_814AD318->vtx->pos, stride);
            break;
        case GX_VA_NRM:
            stride = 3;
            switch (fmt->type) {
            case GX_S8:
                break;
            case GX_S16:
                stride *= 2;
                break;
            case GX_F32:
                stride *= 4;
                break;
            }
            GXSetArray(GX_VA_NRM, lbl_814AD318->vtx->nrm, stride);
            break;
        case GX_VA_CLR0:
        case GX_VA_CLR1:
            stride = 1;
            switch (fmt->type) {
            case GX_RGB565:
            case GX_RGBA4:
                stride *= 2;
                break;
            case GX_RGB8:
            case GX_RGBA6:
                stride *= 3;
                break;
            case GX_RGBX8:
            case GX_RGBA8:
            default:
                stride *= 4;
                break;
            }
            GXSetArray(fmt->attr, lbl_814AD318->vtx->color[(u8)(fmt->attr - GX_VA_CLR0)], stride);
            break;
        case GX_VA_TEX0:
        case GX_VA_TEX1:
        case GX_VA_TEX2:
        case GX_VA_TEX3:
        case GX_VA_TEX4:
        case GX_VA_TEX5:
        case GX_VA_TEX6:
        case GX_VA_TEX7:
            switch (fmt->count) {
            case GX_TEX_S:
                stride = 1;
                break;
            case GX_TEX_ST:
            default:
                stride = 2;
                break;
            }
            switch (fmt->type) {
            case GX_U16:
                stride *= 2;
                break;
            case GX_S16:
                stride *= 2;
                break;
            case GX_F32:
                stride *= 4;
                break;
            }
            GXSetArray(fmt->attr, lbl_814AD318->vtx->tex[(u8)(fmt->attr - GX_VA_TEX0)], stride);
            break;
        }
    }
    GXSetVtxAttrFmtv(GX_VTXFMT0, lbl_814AD318->vtx->fmt);
}

void fn_8130402C(GMRNode* node) {
    Mtx save;

    PSMTXCopy(lbl_814AD31C->curMtx, save);
    if (node->type == 0x10) {
        fn_81303CAC(node->index, &lbl_814AD318->joints[node->index].xf);
    }
    if (node->child != NULL) {
        fn_8130402C(node->child);
    }
    PSMTXCopy(save, lbl_814AD31C->curMtx);
    if (node->next != NULL) {
        fn_8130402C(node->next);
    }
}

void J3D_CalcJoints(GMRInstance* inst) {
    lbl_814AD31C = inst;
    lbl_814AD318 = inst->model;
    PSMTXCopy(lbl_814AD31C->baseMtx, lbl_814AD31C->curMtx);
    fn_8130402C(inst->model->root);
}

void fn_81304240(GMRNode* node, f32 frame) {
    GMRTransform xf;
    Mtx save;

    PSMTXCopy(lbl_814AD31C->curMtx, save);
    if (node->type == 0x10) {
        fn_81306CE0(node->index, &xf, frame);
        fn_81303CAC(node->index, &xf);
    }
    if (node->child != NULL) {
        fn_81304240(node->child, frame);
    }
    PSMTXCopy(save, lbl_814AD31C->curMtx);
    if (node->next != NULL) {
        fn_81304240(node->next, frame);
    }
}

static inline void gmr_AnimMaterials(f32 frame) {
    u16 i;

    for (i = 0; i < lbl_814AD31C->model->matTable->count; i++) {
        fn_81306F14(i, &lbl_814AD31C->matAnm[i], frame);
        lbl_814AD318->materials[i].unk18 = &lbl_814AD31C->matAnm[i];
    }
}

void J3D_AnimateJoints(GMRInstance* inst, f32 frame) {
    lbl_814AD31C = inst;
    lbl_814AD318 = inst->model;
    fn_81306CD0(inst);
    PSMTXCopy(lbl_814AD31C->baseMtx, lbl_814AD31C->curMtx);
    fn_81304240(inst->model->root, frame);
    if (lbl_814AD31C->animMat != 0) {
        gmr_AnimMaterials(frame);
    }
}

static inline void gmr_LoadTexMtx(GMRTexMtx* tm, Mtx view, u32 id) {
    Mtx m;
    u8 type = tm->type;
    PSMTXCopy(view, m);
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    PSMTXConcat(tm->mtx, m, m);
    GXLoadTexMtxImm(m, id, (GXTexMtxType)type);
}

void fn_81304510(u16 matIdx, Mtx view) {
    u8 used[10];
    GMRMaterial* materials;
    GMRTexMtx* tm;
    GMRTexGen* tg;
    int i;
    int j;

    materials = lbl_814AD318->materials;
    if (matIdx != 0xFFFF) {
        for (i = 0; i < 10; i++)
            used[i] = 0;
        for (i = 0; i < materials[matIdx].numTexGens; i++) {
            tg = materials[matIdx].texGens[i];
            if (tg != NULL && tg->type == 1) {
                used[(tg->mtx - GX_TEXMTX0) / 3] = 1;
            }
        }
        for (j = 0; j < 10; j++) {
            if ((tm = materials[matIdx].texMtx[j]) != NULL && used[j] == 1) {
                gmr_LoadTexMtx(tm, view, GX_TEXMTX0 + j * 3);
            }
        }
    }
}

static const double _half = 0.5;
static const double _three = 3.0;

static inline f32 gmr_sqrtf(f32 x) {
    volatile f32 y;
    if (x > 0.0f) {
        double guess = __frsqrte((double)x);
        guess = _half * guess * (_three - guess * guess * x);
        guess = _half * guess * (_three - guess * guess * x);
        guess = _half * guess * (_three - guess * guess * x);
        y = (f32)(x * guess);
        return y;
    }
    return x;
}

void fn_81304694(Mtx m) {
    f32 sx = gmr_sqrtf(m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0]);
    f32 sy = gmr_sqrtf(m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1]);
    f32 sz = gmr_sqrtf(m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2]);

    m[0][0] = sx;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = sy;
    m[1][2] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = sz;
}

void fn_81304854(u16 shapeIdx, u16 matIdx) {
    MtxPtr mtx;
    GMRShape* shape;
    Mtx nrm;
    GXAttrType nbt;

    shape = &lbl_814AD318->shapes[shapeIdx];
    mtx = lbl_814AD31C->jointMtx[lbl_814AD318->draw->jointIndex[*shape->mtxIndex]];

    GXClearVtxDesc();
    GXSetVtxDescv(shape->vtxDesc);
    if (shape->mtxType == 1) {
        fn_81304694(mtx);
    }
    MtxInverse33(mtx, nrm);
    GXGetVtxDesc(GX_VA_NBT, &nbt);
    if (nbt != GX_NONE) {
        MtxScaleCols(nrm, fn_81306A34());
    }
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    fn_81304510(matIdx, mtx);
    GXCallDisplayList(shape->dl->data, shape->dl->size);
}

void fn_81304AF8(GMRNode* node) {
    if (node->type == 0x12) {
        fn_81304854(node->index, lbl_814ACFD8);
    } else if (node->type == 0x11) {
        fn_813061EC(node->index);
        lbl_814ACFD8 = node->index;
    }
    if (node->child != NULL) {
        fn_81304AF8(node->child);
    }
    if (node->next != NULL) {
        fn_81304AF8(node->next);
    }
}

void fn_81304CE8(void) {
    Mtx m;
    u8 i;

    GXSetCullMode(GX_CULL_BACK);
    GXSetCoPlanar(GX_FALSE);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetDither(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, 7);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_S16, 7);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_S16, 7);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX3, GX_TEX_ST, GX_S16, 7);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX4, GX_TEX_ST, GX_S16, 7);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX5, GX_TEX_ST, GX_S16, 7);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX6, GX_TEX_ST, GX_S16, 7);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX7, GX_TEX_ST, GX_S16, 7);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE8, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE9, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE10, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE11, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE12, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE13, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE14, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE15, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    PSMTXIdentity(m);
    for (i = 0; i < 10; i++) {
        GXLoadPosMtxImm(m, GX_PNMTX0 + i * 3);
        GXLoadNrmMtxImm(m, GX_PNMTX0 + i * 3);
        GXLoadTexMtxImm(m, GX_TEXMTX0 + i * 3, GX_MTX2x4);
    }
    GXSetChanMatColor(GX_COLOR0A0, (GXColor){0xFF, 0xFF, 0xFF, 0xFF});
    GXSetChanMatColor(GX_COLOR1A1, (GXColor){0xFF, 0xFF, 0xFF, 0xFF});
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetTexCoordGen2(GX_TEXCOORD0, TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD1, TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD2, TG_MTX2x4, GX_TG_TEX2, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD3, TG_MTX2x4, GX_TG_TEX3, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD4, TG_MTX2x4, GX_TG_TEX4, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD5, TG_MTX2x4, GX_TG_TEX5, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD6, TG_MTX2x4, GX_TG_TEX6, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD7, TG_MTX2x4, GX_TG_TEX7, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXClearVtxDesc();
    GXInvalidateVtxCache();
    GXInvalidateTexAll();
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
}

void J3D_Draw(GMRInstance* inst) {
    lbl_814AD31C = inst;
    lbl_814AD318 = inst->model;
    fn_81303DE4();
    fn_81305F48(inst);
    fn_81304AF8(inst->model->root);
}

void J3D_DrawPacket(GMRInstance* inst, GMRNode* node) {
    lbl_814AD31C = inst;
    lbl_814AD318 = inst->model;
    fn_81303DE4();
    fn_81305F48(inst);
    fn_81304AF8(node);
}

void fn_813052E8(GMRInstance* inst, GMRPacket* pkt) {
    lbl_814AD31C = inst;
    lbl_814AD318 = inst->model;
    fn_81305F48(inst);
    fn_81306850(pkt->matIndex);
    fn_81304AF8(&pkt->node);
}

void J3D_SetModelMtx(GMRInstance* inst, Mtx m, Vec* scale) {
    PSMTXCopy(m, inst->baseMtx);
    MtxScaleCols(inst->baseMtx, scale);
}

void J3D_SetViewMtx(GMRInstance* inst, Mtx view) {
    PSMTXConcat(view, inst->baseMtx, inst->baseMtx);
}
