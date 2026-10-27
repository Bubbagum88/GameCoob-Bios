#include "bs2/menu.h"
#include "bs2/mtx.h"

void J3D_LoadTexture(s32 map, void *tex); /* 0x81305F58 */

/* 0x8130AB14: GX state for vertex-coloured 2D quads */
void fn_8130AB14(void) {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 4);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3X4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_KONST, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
}

/* 0x8130AD0C: GX state for the lit, textured backdrop */
void Backdrop_SetupGX(u32 lights) {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 4);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 4);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_S16, 4);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanCtrl(GX_ALPHA0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, lights, GX_DF_CLAMP, GX_AF_SPOT);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3X4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
}

/* One spot light pointing down -z (view space) from pos, which is moved to view space. */
static inline void Backdrop_LoadSpot(Mtx mv, Vec *pos, f32 bright, f32 cutoff, u8 alpha, GXLightID id) {
    MenuWork *menu = Menu_Get();
    Vec dir = { 0.0f, 0.0f, -1.0f };
    GXLightObj light;
    GXColor color = { 255, 255, 255, 255 };
    Mtx nrm;

    color.a = alpha;
    PSMTXMultVec(mv, pos, pos);
    MtxInverse33(mv, nrm);
    PSMTXMultVec(nrm, &dir, &dir);
    GXInitLightPos(&light, pos->x, pos->y, pos->z);
    GXInitLightDir(&light, dir.x, dir.y, dir.z);
    GXInitLightSpot(&light, cutoff, GX_SP_COS2);
    GXInitLightDistAttn(&light, menu->lightAttn, bright, GX_DA_GENTLE);
    GXInitLightColor(&light, color);
    GXLoadLightObjImm(&light, id);
}

/* copy *p into the caller's zeroed v; returning the pointer keeps v a
   caller local (slot below amb) instead of an inline temp */
static inline Vec *Backdrop_Vec(Vec *v, Vec *p) {
    v->x = p->x;
    v->y = p->y;
    v->z = p->z;
    return v;
}

/* 0x8130AF54: the menu background, a 584x448 grid of 32x32 tiles (res 89)
   lit by GX_LIGHT3 at menu->lightPos plus the extra lights in mask */
void Backdrop_DrawLit(Mtx mv, u8 alpha, u32 lights, u8 spotAlpha) {
    /* named consts, assigned after pos: the original stores mat/amb late */
    static const GXColor kMat = { 255, 255, 255, 255 };
    static const GXColor kAmb = { 0, 0, 0, 0 };
    MenuWork *menu = Menu_Get();
    Vec pos;
    GXColor mat;
    s32 pad;
    GXColor amb;
    Vec v = { 0.0f, 0.0f, 0.0f };
    s32 x;
    s32 y;
    void *tex;

    pos = *Backdrop_Vec(&v, &menu->lightPos);
    mat = kMat;
    amb = kAmb;
    tex = ResGet(89);
    Backdrop_LoadSpot(mv, &pos, menu->lightAttnK, menu->lightCutoff, spotAlpha, GX_LIGHT3);
    J3D_LoadTexture(0, tex);
    Backdrop_SetupGX(lights | GX_LIGHT3);
    mat.a = alpha;
    GXSetChanMatColor(GX_COLOR0A0, mat);
    GXSetChanAmbColor(GX_COLOR0A0, amb);
    for (x = 0; x < 584; x += 32) {
        for (y = 0; y < 448; y += 32) {
            GXBegin(GX_QUADS, GX_VTXFMT1, 4);
            GXPosition3s16(x * 16, y * 16, 0);
            GXNormal3s16(0, 0, 16);
            GXTexCoord2s16(0, 0);
            GXPosition3s16((x * 16 + 512), y * 16, 0);
            GXNormal3s16(0, 0, 16);
            GXTexCoord2s16(32, 0);
            GXPosition3s16((x * 16 + 512), (y * 16 + 512), 0);
            GXNormal3s16(0, 0, 16);
            GXTexCoord2s16(32, 32);
            GXPosition3s16(x * 16, (y * 16 + 512), 0);
            GXNormal3s16(0, 0, 16);
            GXTexCoord2s16(0, 32);
            GXEnd();
        }
    }
    GXSetChanAmbColor(GX_COLOR0A0, mat);
    GXSetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_SPOT);
}

/* 0x8130B244 */
void Backdrop_Draw(Mtx mv, u8 alpha) {
    Backdrop_DrawLit(mv, alpha, GX_LIGHT3, 255);
}

/* 0x8130B26C: backdrop with up to two extra spot lights (GX_LIGHT4/5) */
void Backdrop_DrawSpots(Mtx mv, u8 alpha, Vec *pos1, Vec *pos2, u8 alpha1, u8 alpha2, u8 spotAlpha) {
    u32 lights = 0;

    if (alpha1 != 0) {
        Backdrop_LoadSpot(mv, pos1, 0.525f, 28.0f, alpha1, GX_LIGHT4);
        lights |= GX_LIGHT4;
    }
    if (alpha2 != 0) {
        Backdrop_LoadSpot(mv, pos2, 0.525f, 28.0f, alpha2, GX_LIGHT5);
        lights |= GX_LIGHT5;
    }
    Backdrop_DrawLit(mv, alpha, lights, spotAlpha);
}
