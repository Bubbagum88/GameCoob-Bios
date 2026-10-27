// gMR model renderer: materials and textures (PAL 1.0 BS2, 0x81305824..0x81306A3C).
// Builds runtime materials from the MAT3 block and loads TEV/channel/texture state.

#include "bs2/gMRtex.h"

void* lbl_814AD32C;
void* lbl_814AD328;
GMRModelData* lbl_814AD324;
GMRInstance* lbl_814AD320;

static inline u8 GetU32(u32* tbl, int idx) {
    if (idx == 0xFF) {
        return 0xFF;
    }
    return tbl[idx];
}

static inline u8 GetU8(u8* tbl, int idx) {
    if (idx == 0xFF) {
        return 0xFF;
    }
    return tbl[idx];
}

static inline u8 GetUnk(int idx) {
    u8 v[1] = {0xFF};
    if (idx == 0xFF) {
        v[0] = 0xFF;
    }
    return v[0];
}

#define DEF_GET(name, type, field)                          \
    static inline type* name(GMRMatBlock* blk, int idx) {   \
        type* p;                                            \
        if (idx == 0xFFFF) {                                \
            p = NULL;                                       \
        } else {                                            \
            p = &blk->field[idx];                           \
        }                                                   \
        return p;                                           \
    }

#define DEF_GET_N(name, type, field)                        \
    static inline type* name(GMRMatBlock* blk, int idx) {   \
        if (idx == 0xFFFF) {                                \
            return NULL;                                    \
        }                                                   \
        return &blk->field[idx];                            \
    }

DEF_GET(GetMatColor, GXColor, matColor)
DEF_GET(GetColorChan, GMRChanCtrl, colorChan)
DEF_GET_N(GetTevColor, GXColorS10, tevColor)
DEF_GET_N(GetTexGen, GMRTexGen, texGen)
DEF_GET(GetTevStage, GMRTevStage, tevStage)
DEF_GET_N(GetTevOrder, GMRTevOrder, tevOrder)
DEF_GET(GetTexMtx, GMRTexMtx, texMtx)
DEF_GET(GetUnk48, GMRUnk2C, unk48)
DEF_GET_N(GetUnk4C, GMRUnk8, unk4C)
DEF_GET_N(GetUnk50, GMRUnk4, unk50)
DEF_GET(GetVtxNrm, GMRVtxNrm, vtxNrm)

static inline u16 GetTexNo(u16* tbl, int idx) {
    if (idx == 0xFFFF) {
        return 0xFFFF;
    }
    return tbl[idx];
}

/* 0x81305824: build the runtime material table */
void fn_81305824(GMRMatBlock* blk, GMRMaterial* mat) {
    int i;
    int j;

    for (i = 0; i < blk->count; i++, mat++) {
        mat->flag = blk->init[blk->remap[i]].flag;
        mat->cull = GetU32(blk->cull, blk->init[blk->remap[i]].cull);
        mat->numChans = GetU8(blk->numChans, blk->init[blk->remap[i]].numChans);
        mat->numTexGens = GetU8(blk->numTexGens, blk->init[blk->remap[i]].numTexGens);
        mat->numTevStages = GetU8(blk->numTevStages, blk->init[blk->remap[i]].numTevStages);
        mat->unk5 = GetU32(blk->unk5C, blk->init[blk->remap[i]].unk7);
        mat->unk6 = GetU32(blk->unk58, blk->init[blk->remap[i]].unk5);
        mat->unk7 = GetUnk(blk->init[blk->remap[i]].unk6);
        mat->matColor[0] = GetMatColor(blk, blk->init[blk->remap[i]].matColor[0]);
        mat->matColor[1] = GetMatColor(blk, blk->init[blk->remap[i]].matColor[1]);
        for (j = 0; j < 4; j++) {
            mat->colorChan[j] = GetColorChan(blk, blk->init[blk->remap[i]].colorChan[j]);
            mat->tevColor[j] = GetTevColor(blk, blk->init[blk->remap[i]].tevColor[j]);
        }
        for (j = 0; j < 8; j++) {
            mat->texNo[j] = GetTexNo(blk->texNo, blk->init[blk->remap[i]].texNo[j]);
            mat->texGen[j] = GetTexGen(blk, blk->init[blk->remap[i]].texGen[j]);
            mat->tevStage[j] = GetTevStage(blk, blk->init[blk->remap[i]].tevStage[j]);
            mat->tevOrder[j] = GetTevOrder(blk, blk->init[blk->remap[i]].tevOrder[j]);
        }
        for (j = 0; j < 10; j++) {
            mat->texMtx[j] = GetTexMtx(blk, blk->init[blk->remap[i]].texMtx[j]);
        }
        mat->unkC8 = GetUnk48(blk, blk->init[blk->remap[i]].unk90);
        mat->unkCC = GetUnk4C(blk, blk->init[blk->remap[i]].unk92);
        mat->unkD0 = GetUnk50(blk, blk->init[blk->remap[i]].unk94);
        mat->vtxNrm = GetVtxNrm(blk, blk->init[blk->remap[i]].vtxNrm);
    }
}

/* 0x81305F48 */
void fn_81305F48(GMRInstance* inst) {
    lbl_814AD320 = inst;
    lbl_814AD324 = inst->model;
}

/* 0x81305F58 */
void J3D_LoadTexture(u8 map, GMRTexImg* img) {
    GXTexObj obj;
    GXTlutObj tlut;

    if (img->indexed == 0) {
        GXInitTexObj(&obj, (u8*)img + img->imageOffset, img->width, img->height, (GXTexFmt)img->format,
                     (GXTexWrapMode)img->wrapS, (GXTexWrapMode)img->wrapT, img->mipmap);
    } else {
        GXInitTexObjCI(&obj, (u8*)img + img->imageOffset, img->width, img->height, (GXCITexFmt)img->format,
                       (GXTexWrapMode)img->wrapS, (GXTexWrapMode)img->wrapT, img->mipmap, (u8)map);
    }
    GXInitTexObjLOD(&obj, (GXTexFilter)img->minFilter, (GXTexFilter)img->magFilter, 0.125f * img->minLOD,
                    0.125f * img->maxLOD, 0.01f * img->lodBias, img->biasClamp, img->edgeLOD,
                    (GXAnisotropy)img->maxAniso);
    if (img->indexed == 1) {
        GXInitTlutObj(&tlut, (u8*)img + img->tlutOffset, (GXTlutFmt)img->tlutFormat, img->numColors);
        GXLoadTlut(&tlut, (u8)map);
    }
    GXLoadTexObj(&obj, (GXTexMapID)(u8)map);
}

/* 0x813060B8: normal / NBT vertex format and array */
void fn_813060B8(GMRVtxNrm* nrm) {
    GMRVtxInfo* info = lbl_814AD324->vtx->info;

    if (nrm->isNBT == 1) {
        u8 stride = 9;
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NBT, GX_NRM_NBT, (GXCompType)info->nrmType, info->nrmFrac);
        switch (info->nrmType) {
        case GX_S8:
            break;
        case GX_S16:
            stride *= 2;
            break;
        case GX_F32:
            stride *= 4;
            break;
        }
        GXSetArray(GX_VA_NBT, lbl_814AD324->vtx->nbtArray, stride);
        lbl_814AD328 = nrm->unk4;
    } else {
        u8 stride = 3;
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, (GXCompType)info->nrmType, info->nrmFrac);
        switch (info->nrmType) {
        case GX_S8:
            break;
        case GX_S16:
            stride *= 2;
            break;
        case GX_F32:
            stride *= 4;
            break;
        }
        GXSetArray(GX_VA_NRM, lbl_814AD324->vtx->nrmArray, stride);
        lbl_814AD328 = NULL;
    }
}





static inline void GXSetTexCoordGen(GXTexCoordID dst, GXTexGenType func, GXTexGenSrc src, u32 mtx) {
    GXSetTexCoordGen2(dst, func, src, mtx, GX_FALSE, GX_PTIDENTITY);
}

static inline GXColor gMR_AlphaColor(u8 a) {
    GXColor c = {0, 0, 0, 0};
    c.a = a;
    return c;
}

/* 0x813061EC: load all GX state of a material */
void fn_813061EC(u16 index) {
    GMRMaterial* mat = &lbl_814AD324->materials[index];
    GXColor kc;
    u8 skipMtx[10];
    int i;

    for (i = 0; i < 10; i++) {
        skipMtx[i] = 0;
    }

    if (mat->cull != 0xFF) {
        GXSetCullMode((GXCullMode)mat->cull);
    }
    if (mat->numChans != 0xFF) {
        GXSetNumChans(mat->numChans);
    }
    if (mat->matColor[0] != NULL) {
        GXSetChanMatColor(GX_COLOR0A0, *mat->matColor[0]);
    }
    if (mat->matColor[1] != NULL) {
        GXSetChanMatColor(GX_COLOR1A1, *mat->matColor[1]);
    }
    if (mat->colorChan[0] != NULL) {
        GXSetChanCtrl(GX_COLOR0, mat->colorChan[0]->enable, GX_SRC_REG, (GXColorSrc)mat->colorChan[0]->ambSrc,
                      mat->colorChan[0]->matSrc, (GXDiffuseFn)mat->colorChan[0]->lightMask,
                      (GXAttnFn)mat->colorChan[0]->diffFn);
    }
    if (mat->colorChan[1] != NULL) {
        GXSetChanCtrl(GX_ALPHA0, mat->colorChan[1]->enable, GX_SRC_REG, (GXColorSrc)mat->colorChan[1]->ambSrc,
                      mat->colorChan[1]->matSrc, (GXDiffuseFn)mat->colorChan[1]->lightMask,
                      (GXAttnFn)mat->colorChan[1]->diffFn);
    }
    if (mat->colorChan[2] != NULL) {
        GXSetChanCtrl(GX_COLOR1, mat->colorChan[2]->enable, GX_SRC_REG, (GXColorSrc)mat->colorChan[2]->ambSrc,
                      mat->colorChan[2]->matSrc, (GXDiffuseFn)mat->colorChan[2]->lightMask,
                      (GXAttnFn)mat->colorChan[2]->diffFn);
    }
    if (mat->colorChan[3] != NULL) {
        GXSetChanCtrl(GX_ALPHA1, mat->colorChan[3]->enable, GX_SRC_REG, (GXColorSrc)mat->colorChan[3]->ambSrc,
                      mat->colorChan[3]->matSrc, (GXDiffuseFn)mat->colorChan[3]->lightMask,
                      (GXAttnFn)mat->colorChan[3]->diffFn);
    }
    if (mat->numTexGens != 0xFF) {
        GXSetNumTexGens(mat->numTexGens);
    }
    for (i = 0; i < mat->numTexGens; i++) {
        if (mat->texGen[i] != NULL) {
            GXSetTexCoordGen((GXTexCoordID)i, (GXTexGenType)mat->texGen[i]->type, (GXTexGenSrc)mat->texGen[i]->src,
                             mat->texGen[i]->mtx);
        }
    }
    for (i = 0; i < 10; i++) {
        if (mat->texMtx[i] != NULL && skipMtx[i] == 0) {
            GXLoadTexMtxImm(mat->texMtx[i]->mtx, i * 3 + GX_TEXMTX0, (GXTexMtxType)mat->texMtx[i]->type);
        }
    }
    if (mat->numTevStages != 0xFF) {
        GXSetNumTevStages(mat->numTevStages);
    }
    for (i = 0; i < 4; i++) {
        if (mat->tevColor[i] != NULL) {
            GXSetTevColorS10((GXTevRegID)(i + 1), *mat->tevColor[i]);
        }
    }
    for (i = 0; i < 8; i++) {
        GMRTevStage* st = mat->tevStage[i];
        if (st != NULL) {
            GXSetTevKColorSel((GXTevStageID)i, GX_TEV_KCSEL_1_4);
            GXSetTevKAlphaSel((GXTevStageID)i, GX_TEV_KASEL_1);
            switch (st->op) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                GXSetTevOp((GXTevStageID)i, (GXTevMode)st->op);
                break;
            default:
                GXSetTevColorIn((GXTevStageID)i, (GXTevColorArg)st->cIn[0], (GXTevColorArg)st->cIn[1],
                                (GXTevColorArg)st->cIn[2], (GXTevColorArg)st->cIn[3]);
                GXSetTevColorOp((GXTevStageID)i, (GXTevOp)st->cOp[0], (GXTevBias)st->cOp[1],
                                (GXTevScale)st->cOp[2], st->cOp[3], (GXTevRegID)st->cOp[4]);
                GXSetTevAlphaIn((GXTevStageID)i, (GXTevAlphaArg)st->aIn[0], (GXTevAlphaArg)st->aIn[1],
                                (GXTevAlphaArg)st->aIn[2], (GXTevAlphaArg)st->aIn[3]);
                GXSetTevAlphaOp((GXTevStageID)i, (GXTevOp)st->aOp[0], (GXTevBias)st->aOp[1],
                                (GXTevScale)st->aOp[2], st->aOp[3], (GXTevRegID)st->aOp[4]);
                break;
            }
        }
        if (mat->tevOrder[i] != NULL) {
            GXSetTevOrder((GXTevStageID)i, (GXTexCoordID)mat->tevOrder[i]->coord, mat->tevOrder[i]->map,
                          (GXChannelID)mat->tevOrder[i]->color);
        } else {
            GXSetTevOrder((GXTevStageID)i, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        }
        if (mat->texNo[i] != 0xFFFF) {
            J3D_LoadTexture(i, &lbl_814AD324->tex->images[mat->texNo[i]]);
        }
    }
    if (mat->vtxNrm != NULL) {
        fn_813060B8(mat->vtxNrm);
    }
    if (lbl_814AD320->alpha != 0xFF) {
        kc = gMR_AlphaColor(lbl_814AD320->alpha);
        if (mat->numTevStages != 0xFF) {
            GXSetNumTevStages(mat->numTevStages + 1);
            GXSetTevKColor(GX_KCOLOR0, *(GXColor*)&kc);
            GXSetTevKAlphaSel((GXTevStageID)mat->numTevStages, GX_TEV_KASEL_K0_A);
            GXSetTevColorIn((GXTevStageID)mat->numTevStages, GX_CC_ZERO, GX_CC_ONE, GX_CC_CPREV, GX_CC_ZERO);
            GXSetTevColorOp((GXTevStageID)mat->numTevStages, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn((GXTevStageID)mat->numTevStages, GX_CA_ZERO, GX_CA_KONST, GX_CA_APREV, GX_CA_ZERO);
            GXSetTevAlphaOp((GXTevStageID)mat->numTevStages, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevOrder((GXTevStageID)mat->numTevStages, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        }
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        GXSetZCompLoc(GX_FALSE);
    } else {
        GXColor w = {0, 0, 0, 0xFF};
        GXSetTevKColor(GX_KCOLOR0, *(GXColor*)&w);
        if ((mat->flag & 1) == 1) {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
            GXSetZCompLoc(GX_TRUE);
        } else {
            GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
            GXSetZCompLoc(GX_FALSE);
        }
    }
}

/* 0x81306850: reload only the TEV colors and the alpha stage of a material */
void fn_81306850(u16 index) {
    GMRMaterial* mat = &lbl_814AD324->materials[index];
    s32 i;
    GXColor kc;

    for (i = 0; i < 4; i++) {
        if (mat->tevColor[i] != NULL) {
            GXSetTevColorS10((GXTevRegID)(i + 1), *mat->tevColor[i]);
        }
    }
    if (lbl_814AD320->alpha != 0xFF) {
        kc = gMR_AlphaColor(lbl_814AD320->alpha);
        if (mat->numTevStages != 0xFF) {
            GXSetNumTevStages(mat->numTevStages + 1);
            GXSetTevKColor(GX_KCOLOR0, *(GXColor*)&kc);
            GXSetTevKAlphaSel((GXTevStageID)mat->numTevStages, GX_TEV_KASEL_K0_A);
            GXSetTevColorIn((GXTevStageID)mat->numTevStages, GX_CC_ZERO, GX_CC_ONE, GX_CC_CPREV, GX_CC_ZERO);
            GXSetTevColorOp((GXTevStageID)mat->numTevStages, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn((GXTevStageID)mat->numTevStages, GX_CA_ZERO, GX_CA_KONST, GX_CA_APREV, GX_CA_ZERO);
            GXSetTevAlphaOp((GXTevStageID)mat->numTevStages, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevOrder((GXTevStageID)mat->numTevStages, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        }
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        GXSetZCompLoc(GX_FALSE);
    } else {
        GXColor w = {0, 0, 0, 0xFF};
        GXSetTevKColor(GX_KCOLOR0, *(GXColor*)&w);
        if ((mat->flag & 1) == 1) {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
            GXSetZCompLoc(GX_TRUE);
        } else {
            GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
            GXSetZCompLoc(GX_FALSE);
        }
    }
}

/* 0x81306A34 */
void* fn_81306A34(void) {
    return lbl_814AD328;
}
