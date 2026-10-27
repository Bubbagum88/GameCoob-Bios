/*
 * BS2 camera, light and 2D view setup (PAL 1.0 IPL, .text 0x81302DF0..0x81303204).
 */
#include <dolphin/types.h>
#include <dolphin/mtx.h>
#include <dolphin/gx.h>
#include <math.h>
#include "bs2/scene.h"
#include "bs2/mtx.h"

/* Display settings (0x813010F8), only the fields used here. */
typedef struct CamDispSettings {
    u8 pad0[8];
    f32 fovy;        /* 0x08 vertical field of view in degrees */
    u8 padC[8];
    f32 camYFar;     /* 0x14 camera height offset (normal) */
    f32 camYNear;    /* 0x18 camera height offset (menu) */
} CamDispSettings;
CamDispSettings *Disp_GetSettings(void);


static Mtx lbl_814AF630;  /* camera view */
static Mtx lbl_814AF660;  /* view without the height offset */
static Mtx lbl_814AF690;  /* 2D model-view */

void fn_81302DF0(void) {
    CamDispSettings *disp = Disp_GetSettings();
    Scene *scene = Scene_Get();
    Vec up = { 0.0f, 1.0f, 0.0f };
    Vec pos = { 1700.0f, 1700.0f, 1700.0f };
    Vec target = { 0.0f, 0.0f, 0.0f };
    f32 y;

    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 224.0f / tanf(3.141592f * disp->fovy / 360.0f);
    C_MTXLookAt(lbl_814AF660, &pos, &up, &target);

    switch (Scene_GetState()) {
    case 4:
        if (scene->outFrame <= scene->outFadeFrames) {
            y = disp->camYFar - (f32)scene->outFrame * (disp->camYFar - disp->camYNear) / (f32)scene->outFadeFrames;
        } else {
            y = disp->camYNear;
        }
        break;
    case 5:
        y = disp->camYNear;
        break;
    default:
        y = disp->camYFar;
        break;
    }

    pos.y += y;
    target.y += y;
    C_MTXLookAt(lbl_814AF630, &pos, &up, &target);
}

MtxPtr Camera_GetView(void) {
    return lbl_814AF630;
}

MtxPtr fn_81302F94(void) {
    return lbl_814AF660;
}

static inline Vec *Camera_VecRef(Vec *v) {
    (void)v;
    return v;
}

static inline GXColor *Camera_ColorRef(GXColor *c) {
    (void)c;
    return c;
}

/* out-param + nested ref inline: rodata -> temp -> local with no dead
   result slot (a Vec-returning inline leaves 12 dead bytes each) */
static inline void Camera_LightPos(Vec *out) {
    Vec v = { 400.0f, 500.0f, 200.0f };
    *out = *Camera_VecRef(&v);
}

static inline void Camera_LightDir(Vec *out) {
    Vec v = { 0.0f, 0.0f, 0.0f };
    *out = *Camera_VecRef(&v);
}

static inline void Camera_SetWhite(GXLightObj *light) {
    GXColor c = { 255, 255, 255, 255 };
    GXInitLightColor(light, *Camera_ColorRef(&c));
}

void fn_81302FA0(void) {
    GXLightObj light;
    CamDispSettings *disp = Disp_GetSettings();
    Vec pos;
    Vec dir;

    Camera_LightPos(&pos);
    Camera_LightDir(&dir);
    PSMTXMultVec(lbl_814AF630, &pos, &pos);
    PSMTXMultVec(lbl_814AF630, &dir, &dir);
    GXInitLightPos(&light, pos.x, pos.y, pos.z);
    GXInitLightDir(&light, dir.x, dir.y, dir.z);
    Camera_SetWhite(&light);
    GXLoadLightObjImm(&light, GX_LIGHT0);
}

void GX_Setup2DView(void) {
    CamDispSettings *disp = Disp_GetSettings();
    SRT srt = { 1.0f, -1.0f, 1.0f, 0, 0, 0, 0, -292.0f, 224.0f, 0.0f };
    Vec up = { 0.0f, 1.0f, 0.0f };
    Vec pos = { 0.0f, 0.0f, 0.0f };
    Vec target = { 0.0f, 0.0f, 0.0f };
    Mtx view;
    Mtx nrm;

    pos.z = 224.0f / tanf(3.141592f * disp->fovy / 360.0f);
    C_MTXLookAt(view, &pos, &up, &target);
    MtxFromSRT(&srt, lbl_814AF690);
    PSMTXConcat(view, lbl_814AF690, lbl_814AF690);
    GXLoadPosMtxImm(lbl_814AF690, GX_PNMTX0);
    PSMTXInverse(lbl_814AF690, nrm);
    PSMTXTranspose(nrm, nrm);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}
