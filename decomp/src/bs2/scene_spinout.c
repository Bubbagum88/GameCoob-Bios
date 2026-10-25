// Intro spin-out (PAL 1.0 BS2, 0x8130E70C..0x8130EF48): once the cube is
// spun fast enough the scene coasts, turns and settles into the main menu.

#include "bs2/scene.h"
#include "bs2/cube.h"
#include "bs2/j3d.h"
#include "bs2/menu.h"
#include "bs2/res.h"

/* Not yet decompiled */
f32 Hermite(f32 t, f32 t0, f32 p0, f32 v0, f32 t1, f32 p1, f32 v1); /* 0x81307D08 */
void Sound_UpdateSpin(u8 mode, f32 speed);                   /* 0x8135A8E0 */

static s16 sPrevSpin;                /* 0x814AD3D0 */

static Mtx sSpinOutMtx;              /* 0x814AFD28 */

/* 0x8130E70C: coast, then 3 full turns plus outSettle (Hermite), then
   settle back to 0. Returns FALSE while faster than the threshold. */
static void Scene_UpdateSpinOut(void) {
    Scene *s = Scene_Get();
    u8 slow;
    f32 level;

    sPrevSpin = s->spin;
    if (s->outFrame >= 0 && s->outFrame < s->outCoastFrames) {
        s->spin = s->spin + s->spinSpeed;
        s->outStart = s->spin;
    } else if (s->outFrame >= s->outCoastFrames &&
               s->outFrame < s->outTurnFrames + s->outCoastFrames) {
        s->spin = (s32)Hermite(s->outFrame, s->outCoastFrames, -(s->outStart + (s->outTurns << 16)),
                          s->outSpeed, s->outCoastFrames + s->outTurnFrames, s->outSettle, 0.0f);
        if (s->outFrame > s->outCoastFrames) {
            s->spinSpeed = (s16)((u16)s->spin - (u16)sPrevSpin);
        }
    } else if (s->outFrame >= s->outTurnFrames + s->outCoastFrames &&
               s->outFrame < s->outCoastFrames + (s->outTurnFrames + s->outSettleFrames)) {
        s->spin = (s32)Hermite(s->outFrame, s->outTurnFrames + s->outCoastFrames, s->outSettle, 0.0f,
                          s->outCoastFrames + (s->outTurnFrames + s->outSettleFrames), 0.0f, 0.0f);
        s->spinSpeed = (s16)((u16)s->spin - (u16)sPrevSpin);
    } else if (s->outFrame == s->outCoastFrames + (s->outTurnFrames + s->outSettleFrames)) {
        s->spin = 0;
        s->spinSpeed = 0.0f;
    }

    if (s->spinSpeed > 5000.0f) {
        slow = FALSE;
    } else {
        slow = TRUE;
    }
    level = s->spinSpeed / 5000.0f;
    if (level > 1.0f) {
        level = 1.0f;
    }
    if (level < 0.01f) {
        level = 0.0f;
    }
    if (sPrevSpin < 0 && s->spin >= 0 && level > s->outWhooshMin) {
        Sound_UpdateSpin(slow, level);
    } else if (sPrevSpin > 0 && s->spin < 0 && level > s->outWhooshMin) {
        Sound_UpdateSpin(slow, level);
    }
}

/* 0x8130EA34: the spin-out cube matrix: the intro wobble blended into the
   menu cube's idle wobble as the spin slows down. */
static void Scene_CalcSpinOutMtx(Mtx m, u8 advance) {
    Scene *s = Scene_Get();
    MenuWork *menu = Menu_Get();
    s16 frame = menu->wobbleFrame;
    s16 phase = s->wobblePhase;
    f32 tx, ty;
    f32 a, b;
    f32 amp = s->wobbleAmp;
    f32 k = s->spinSpeed / s->spinThreshold;
    f32 tilt = 1.0f - s->spinTilt;
    Mtx t;

    if (s->outFrame == 1) {
        frame = 0;
    }
    PSMTXIdentity(t);
    a = tilt * k;
    b = 1.0f - a;
    ty = menu->wobbleAmpTY * SinS(menu->wobbleSpeedTY * frame);
    tx = menu->wobbleAmpTX * SinS(menu->wobblePhaseTX + menu->wobbleSpeedTX * frame);
    MtxRotTrans(a * (8191.0f + amp * SinS(90.0f * phase)) +
                    b * (menu->wobbleAmp[0] * CosS(menu->wobbleSpeed[0] * frame)),
                a * (-6371.0f + amp * SinS(60.0f * phase)) +
                    b * (menu->wobbleAmp[1] * CosS(menu->wobblePhaseRY + menu->wobbleSpeed[1] * frame)),
                a * (-5461.0f + amp * SinS(70.0f * phase)) +
                    b * (menu->wobbleAmp[2] * CosS(menu->wobbleSpeed[2] * frame)),
                m, tx, ty, 0.0f);
    PSMTXConcat(m, t, m);
    if (advance) {
        frame += 7;
    }
    menu->wobbleFrame = frame;
}

/* Pointer pass-through helpers: routing the by-value Vec through them gives
   the original's stack layout (copy right above the local, the helpers'
   parameter slots at the bottom of the frame). */
static inline Vec *Scene_VecRef0(Vec *v) {
    return v;
}

static inline Vec *Scene_VecRef(Vec *v) {
    (void)v;
    return Scene_VecRef0(v);
}

static inline void Scene_SetBigCubeMtx(Scene *s) {
    f32 k = s->spinSpeed / s->spinThreshold;
    f32 rest = 1.1f * (1.0f - k);
    f32 scale;
    f32 xz;
    Vec r = { 0.0f, 0.0f, 0.0f };

    scale = s->modelScale;
    xz = scale * (rest + (1.0f - (1.0f - s->squashXZ)) * k);
    r.x = xz;
    r.y = scale * (rest + (1.0f + s->stretchY) * k);
    r.z = xz;
    J3D_SetModelMtx(Scene_GetBigCube(), sSpinOutMtx, *Scene_VecRef(&r));
}

/* 0x8130ED30: state 4. Returns SCENE_MENU once the spin has settled. */
s32 Scene_SpinOut(void) {
    Scene *s = Scene_Get();
    DispSettings *disp = Disp_GetSettings();
    Scene *t = Scene_Get();
    Mtx rot;
    Mtx wobble;
    MtxPtr view;
    f32 k;    /* unused, but they take stack slots in the original */
    f32 rest;

    if (t->outFirst) {
        t->outStart = t->spin;
        t->outFirst = 0;
    }
    Scene_UpdateSpinOut();
    t->outFrame++;
    if (t->spinSpeed <= 0.0f && t->spin == 0) {
        t->outFirst = 1;
    }
    disp->menuFlag = FALSE;
    MtxRotTrans(0, Scene_Get()->spin, 0, rot, 0.0f, 0.0f, 0.0f);
    Scene_CalcSpinOutMtx(wobble, TRUE);
    PSMTXConcat(rot, wobble, sSpinOutMtx);

    Scene_SetBigCubeMtx(s);
    view = Camera_GetView();
    J3D_SetViewMtx(Scene_GetBigCube(), view);
    J3D_CalcJoints(Scene_GetBigCube());
    if (s->outFrame <= s->outFadeFrames) {
        Scene_DrawWorld();
    }
    if (s->spinSpeed <= 0.0f && s->spin == 0) {
        return SCENE_MENU;
    }
    return SCENE_SPINOUT;
}

/* 0x8130EF3C */
MtxPtr Scene_GetSpinOutMtx(void) {
    return sSpinOutMtx;
}
