// Mosaic cube text ("fx") drawing (PAL 1.0 BS2, 0x81332FB8..0x81333344):
// the cube model and its 2145 instances, their per-frame matrices and the
// instanced draw.

#include "bs2/menu_fx.h"

static J3DModel sFxCubeModel;
static J3DModelInstance sFxCubeInst[FX_MAX_CUBES];
static f32 sFxDepth[FX_MAX_CUBES * 2];

static u32 sFxPass;

/* 0x81332FB8: load the text cube model (res 104) and point every cube
   instance at it. */
void Fx_InitCubeModel(void) {
    u32 i;

    sFxCubeModel.data = ResGet(0x68);
    J3D_LoadModelData(&sFxCubeModel);
    for (i = 0; i < FX_MAX_CUBES; i++) {
        sFxCubeInst[i].model = &sFxCubeModel;
        J3D_InitModel(&sFxCubeInst[i], 0);
    }
}

/* 0x81333030: draw every text cube; even frames draw the cubes behind the
   menu plane, odd frames the ones in front. */
void Fx_Draw(u8 alpha) {
    u32 i;
    GXColorS10 c0;
    void *save0;
    u8 *buf;
    f32 nearZ;
    f32 farZ;
    GXColor *light;
    u32 count;
    f32 z;
    void *save1;
    s16 drawn;
    f32 d;
    GXColorS10 c1;
    GXColor *dark;

    buf = Fx_GetDrawBuf();
    count = Fx_GetCount();
    light = fn_81329B54();
    dark = fn_81329B60();
    z = Menu_Get()->unk00;
    d = Menu_Get()->fxDepth;
    nearZ = z - d;
    farZ = z + d;
    drawn = 0;

    for (i = 0; i < count; i++) {
        c0.r = light[i].r;
        c0.g = light[i].g;
        c0.b = light[i].b;
        c0.a = 0xFF;
        c1.r = dark[i].r;
        c1.g = dark[i].g;
        c1.b = dark[i].b;
        c1.a = 0xFF;
        save1 = sFxCubeInst[i].model->material->tevColor2;
        save0 = sFxCubeInst[i].model->material->tevColor;
        sFxCubeInst[i].model->material->tevColor = &c0;
        sFxCubeInst[i].model->material->tevColor2 = &c1;
        sFxCubeInst[i].alpha = (u8)((light[i].a * (alpha * buf[i])) / 65025);
        if (sFxCubeInst[i].alpha > 10) {
            if (sFxPass == 0 && sFxDepth[i] > nearZ) {
                if (drawn == 0) {
                    J3D_DrawPacket(&sFxCubeInst[i], sFxCubeInst[i].model->packets + 0x18);
                    drawn++;
                } else {
                    fn_813052E8(&sFxCubeInst[i], sFxCubeInst[i].model->packets + 0x18);
                }
            }
            if (sFxPass == 1 && sFxDepth[i] < farZ) {
                if (drawn == 0) {
                    J3D_DrawPacket(&sFxCubeInst[i], sFxCubeInst[i].model->packets + 0x18);
                    drawn++;
                } else {
                    fn_813052E8(&sFxCubeInst[i], sFxCubeInst[i].model->packets + 0x18);
                }
            }
        }
        sFxCubeInst[i].model->material->tevColor = save0;
        sFxCubeInst[i].model->material->tevColor2 = save1;
    }
    sFxPass ^= 1;
}

/* 0x81333250: model and view matrices of every text cube. */
void Fx_Update(void) {
    u32 count = Fx_GetCount();
    FxCube *cube = Fx_GetCubes();
    u32 i;
    Mtx m;
    Vec scale;

    for (i = 0; i < count; i++) {
        MtxRotTrans(cube[i].rot[0], cube[i].rot[1], cube[i].rot[2], m, cube[i].pos.x, cube[i].pos.y, cube[i].pos.z);
        sFxDepth[i] = cube[i].pos.y;
        scale.x = cube[i].scale.x;
        scale.y = cube[i].scale.y;
        scale.z = cube[i].scale.z;
        J3D_SetModelMtx(&sFxCubeInst[i], m, scale);
        J3D_SetViewMtx(&sFxCubeInst[i], fn_81302F94());
        J3D_CalcJoints(&sFxCubeInst[i]);
    }
}
