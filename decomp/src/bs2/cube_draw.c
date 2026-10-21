// Rolling-cube intro drawing (PAL 1.0 BS2, 0x813108B8..0x81310F58): the
// trail tiles and the small cube model.

#include "bs2/cube.h"
#include "bs2/scene.h"
#include "bs2/j3d.h"
#include "bs2/menu.h"
#include <dolphin/gx.h>

void J3D_LoadTexture(s32 map, void *tex);                  /* 0x81305F58 */

static void *sTileTex;               /* 0x814AD3E8 res 0x65 */

static J3DModel sCubeModel;          /* 0x814B5DF0 res 0xA8 */
static J3DModelInstance sCubeInst;   /* 0x814B5E28 */

static void Cube_DrawTile(s32 x, s32 y, s32 face, u8 alpha);

#define CUBE_CENTER (Cube_Get()->size * 3 / 2)

/* 0x813108B8 */
static void Cube_DrawTiles(u8 alpha) {
    CubeState *c = Cube_Get();
    CubeTile *grid = Cube_GetGrid();
    Mtx m;
    Mtx nrm;
    s32 f, x, y;
    s32 a;
    CubeTile *t;

    MtxRotTrans(0, 0, 0, m, CUBE_CENTER, CUBE_CENTER, CUBE_CENTER);
    PSMTXConcat(Scene_GetWorldMtx(), m, m);
    PSMTXConcat(Camera_GetView(), m, m);
    MtxInverse33(m, nrm);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    for (f = 0; f < CUBE_FACES; f++) {
        for (x = 0; x < CUBE_GRID; x++) {
            for (y = 0; y < CUBE_GRID; y++) {
                t = &grid[(f * CUBE_GRID + x) * CUBE_GRID + y];
                if (t->group == 0) {
                    continue;
                }
                a = 255;
                if (t->state == 1) {
                    a = c->stepFrame * 255 / c->sideFrames;
                }
                if (t->state == 2) {
                    a = (c->sideFrames - c->stepFrame) * 255 / c->sideFrames;
                }
                if (a > 255) {
                    a = 255;
                }
                if (a < 0) {
                    a = 0;
                }
                Cube_DrawTile(x, y, f, a * alpha / 255);
            }
        }
    }
}

/* face 0: (a, 0, b)  face 1: (b, a, 0)  face 2: (0, b, a) */
static inline void Cube_TileVtx(s32 face, s16 a, s16 b, s16 s, s16 t) {
    switch (face) {
    case 0:
        GXPosition3s16(a, 0, b);
        break;
    case 1:
        GXPosition3s16(b, a, 0);
        break;
    case 2:
        GXPosition3s16(0, b, a);
        break;
    }
    GXTexCoord2s16(s, t);
}

/* 0x81310AD0: one textured quad on face plane `face`, in 1/16 units. */
static void Cube_DrawTile(s32 x, s32 y, s32 face, u8 alpha) {
    CubeState *c = Cube_Get();
    u8 inset = c->tileInset;
    CubeTile *grid = Cube_GetGrid();
    s32 full = Cube_Get()->size * 16;
    s32 px = (x - 16) * Cube_Get()->size * 16;
    s32 py = (y - 16) * Cube_Get()->size * 16;
    GXColor color;

    *(u32 *)&color = grid[(face * CUBE_GRID + x) * CUBE_GRID + y].color;
    color.r = c->tileR;
    color.g = c->tileG;
    color.b = c->tileB;
    color.a = alpha;
    GXSetChanMatColor(4, color);
    J3D_LoadTexture(0, sTileTex);
    GXBegin(0x80, 1, 4);
    Cube_TileVtx(face, -(px + inset), -(py + inset), 0, 0);
    Cube_TileVtx(face, -(px + full), -(py + inset), 0, 0x4000);
    Cube_TileVtx(face, -(px + full), -(py + full), 0x4000, 0x4000);
    Cube_TileVtx(face, -(px + inset), -(py + full), 0x4000, 0);
}

/* 0x81310E24: the trail tiles, then the small cube, faded by `alpha`. */
void Cube_DrawBig(u8 alpha) {
    CubeState *c = Cube_Get();
    MtxPtr cubeMtx = Cube_GetMtx();
    Mtx m;

    PSMTXConcat(Camera_GetView(), cubeMtx, m);
    GX_Setup2D(1, 0, 0);
    GXSetVtxAttrFmt(1, 13, 1, 3, 13);
    GXSetCullMode(0);
    Cube_DrawTiles(alpha);
    sCubeInst.alpha = c->alpha * alpha / 255;
    J3D_Draw(&sCubeInst);
}

/* 0x81310EF0 */
void Cube_InitModels(void) {
    sTileTex = ResGet(0x65);
    sCubeModel.data = ResGet(0xA8);
    sCubeInst.model = &sCubeModel;
    sCubeInst.alpha = 255;
    J3D_InitModel(&sCubeInst, 1);
}

/* 0x81310F4C */
J3DModelInstance *Cube_GetModel(void) {
    return &sCubeInst;
}
