// Non-matching: functionally equivalent decompilation of the intro scene
// director (PAL 1.0 BS2, 0x8130BDB8..0x8130D738): spin input, wobble,
// impact shake, the world matrix and the per-state update loop.

#include "bs2/scene.h"
#include "bs2/cube.h"
#include "bs2/j3d.h"
#include "bs2/sound.h"
#include "bs2/pad.h"
#include "bs2/disc.h"

static Scene sceneDefault;     /* 0x814AF878 */
static Scene *scene;           /* 0x814AD3C0 */
static s32 sceneState;         /* 0x814AD3B8 */
static s32 sceneInitialState;  /* 0x814AD3BC */
static Mtx sceneWorld;         /* 0x814AF950 */

/* Per-model scale vectors (rodata 0x8137EBC0) */
static const Vec bgInnerScale0 = { 0.0f, 0.0f, 0.0f }; /* replaced by modelScale */
static const Vec logoScale = { 1.0f, 1.0f, 1.0f };
static const Vec bgOuterScale = { 1.01f, 1.01f, 1.01f };

/* 0x8130BEE8 */
void Scene_Init(BOOL first) {
    ScreenParams *screen = Screen_Get();
    Disc_Poll();
    if (Disc_IsBootable())
        sceneState = SCENE_DISC;
    else if (Disc_IsMode0())
        sceneState = SCENE_MODE0;
    else if (Disc_GetState() == 0x13 || Disc_HasError())
        sceneState = SCENE_MODE3;
    else
        sceneState = SCENE_NODISC;
    sceneInitialState = sceneState;
    screen->ortho = TRUE;   /* the whole cube intro is orthographic */
    if (first) {
        scene = &sceneDefault;
        Scene_SetDefaults(scene);
    }
    Scene_Reset(scene);
    if (first)
        Scene_InitModels();
    Cube_Init(first);
    Menu_InitAll(first);
}

/* 0x8130C248 */
Scene *Scene_Get(void) { return scene; }

/* 0x8130C250: hold A while the cube runs to spin the scene. */
static void Scene_UpdateSpin(BOOL force) {
    Scene *s = Scene_Get();
    s16 easeFrom = s->easeFrom;
    f32 easeSpeed = s->easeSpeed;
    s16 prev = s->spin;
    f32 level;

    if (force || ((Pad_Held() & PAD_BUTTON_A) && Cube_IsRunning())) {
        s->spinAccel += s->spinAccelStep;
        s->spinSpeed += s->spinAccel;
        s->spin = (s16)((f32)s->spin + s->spinSpeed);
        if (!force) {
            easeFrom = s->spin;
            easeSpeed = s->spinSpeed;
            s->easeFrame = 0;
        }
    } else {
        s->spinAccel *= 0.76f;
        s->spinSpeed *= 0.95f;
        if (s->spinAccel < 0.1f) s->spinAccel = 0.0f;
        if (s->spinSpeed < 0.01f) s->spinSpeed = 0.0f;
        /* Ease back to rest over 50 frames. */
        s->spin = (s16)Hermite((f32)s->easeFrame, 0.0f, (f32)easeFrom, easeSpeed, 50.0f, 0.0f, 0.0f);
        if (++s->easeFrame > 50)
            s->easeFrame = 50;
    }

    level = s->spinSpeed / 5000.0f;
    if (level > 1.0f) level = 1.0f;
    if (level < 0.01f) level = 0.0f;
    if (Cube_IsRunning() || force) {
        /* whoosh each time the spin wraps through zero */
        if (((prev < 0 && s->spin >= 0) || (prev > 0 && s->spin < 0)) && level > s->whooshMin)
            Sound_PlayWhoosh(0, level);
    }
    s->lastSpin = prev;
    s->easeFrom = easeFrom;
    s->easeSpeed = easeSpeed;
}

/* 0x8130C518 */
static void Scene_DecayShake(void) {
    Scene *s = Scene_Get();
    s16 phase = s->shakePhase;
    f32 land = s->landImpulse * s->landDecay;
    f32 drop = s->dropImpulse * s->dropDecay;
    if (land < 10.0f) land = 0.0f;
    if (drop < 80.0f) drop = 0.0f;
    if (land > 0.0f || drop > 0.0f)
        phase += s->shakeStep;
    s->landImpulse = (s16)land;
    s->dropImpulse = (s16)drop;
    s->shakePhase = phase;
}

/* 0x8130C5E4 */
static void Scene_UpdateWobble(void) {
    Scene *s = Scene_Get();
    s16 phase = s->wobblePhase;
    f32 amp = s->wobbleAmp;
    amp = s->spinAccel > 0.0f ? amp + 36.0f : amp * 0.945f;
    if (amp < 1.0f) amp = 0.0f;
    if (amp > 0.0f) phase += 7;
    if (amp > 5000.0f) amp = 5000.0f;
    s->wobbleAmp = (s16)amp;
    s->wobblePhase = phase;
    Scene_DecayShake();
}

/* 0x8130C694: out = RotY(spin) * Shake * Wobble * Iso * T(15,15,15) */
static void Scene_CalcWorldMtx(Mtx out) {
    Scene *s = Scene_Get();
    Mtx t, shake;
    f32 f = (s->spinTilt * s->spinSpeed) / s->spinThreshold;
    f32 amp = s->wobbleAmp;
    s16 ph = s->wobblePhase;
    f32 base = 1000.0f * f;
    f32 land, drop, sn;

    PSMTXIdentity(t);
    t[0][3] = t[1][3] = t[2][3] = 15.0f;
    MtxRotTrans(s->isoX, s->isoY, s->isoZ, out, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(out, t, t);
    MtxRotTrans((s16)(base + (1.0f - f) * amp * SinS(ph * 0x5A)),
                (s16)(base + (1.0f - f) * amp * SinS(ph * 0x3C)),
                (s16)(base + (1.0f - f) * amp * SinS(ph * 0x46)),
                out, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(out, t, out);

    land = s->landImpulse;
    drop = s->dropImpulse;
    sn = SinS(s->shakePhase);
    MtxRotTrans((s16)(-drop * sn), 0, (s16)(land * sn), shake,
                0.0f, s->landLift * land * sn + s->dropLift * drop * sn, 0.0f);
    PSMTXConcat(shake, out, out);
    MtxRotTrans(0, s->spin, 0, t, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(t, out, out);
}

/* 0x8130C90C: matrices only, drawing happens in Scene_Draw2D*. */
static void Scene_DrawWorld(void) {
    Scene *s = Scene_Get();
    Mtx w, id;
    Vec sq, sc;
    f32 k = s->spinSpeed / s->spinThreshold;

    Scene_CalcWorldMtx(w);
    PSMTXIdentity(id);
    PSMTXConcat(w, id, sceneWorld);
    /* squash and stretch while spinning */
    sq.x = 1.0f - (1.0f - s->squashXZ) * k;
    sq.y = 1.0f + s->stretchY * k;
    sq.z = 1.0f - (1.0f - s->squashXZ) * k;
    MtxScaleCols(sceneWorld, &sq);

    sc.x = sc.y = sc.z = s->modelScale;
    J3D_SetModelMtx(Scene_GetBgInner(), sceneWorld, &sc);
    J3D_SetViewMtx(Scene_GetBgInner(), Camera_GetView());
    J3D_CalcJoints(Scene_GetBgInner());

    J3D_SetModelMtx(Scene_GetLogo(), sceneWorld, &logoScale);
    J3D_SetViewMtx(Scene_GetLogo(), Camera_GetView());
    J3D_CalcJoints(Scene_GetLogo());

    J3D_SetModelMtx(Scene_GetBgOuter(), sceneWorld, &bgOuterScale);
    J3D_SetViewMtx(Scene_GetBgOuter(), Camera_GetView());
    J3D_CalcJoints(Scene_GetBgOuter());

    Cube_Draw();
}

/* 0x8130CB74: no disc. The cube plays, A spins, fast spin opens the menu. */
static s32 Scene_UpdateNoDisc(void) {
    Scene *s = Scene_Get();
    CubeState *c = Cube_Get();

    if (!c->done)
        Scene_UpdateSpin(FALSE);
    Scene_UpdateWobble();
    Cube_Update(TRUE);
    if (s->spinSpeed > s->spinThreshold) {
        s->spin = s->spinRest;
        return SCENE_SPINOUT;
    }
    /* ... disc-state banners (fields 0x84/0x88/0xB0/0xB2) omitted here,
       see docs/intro.md */
    return SCENE_NODISC;
}

/* 0x8130BDB8 */
s32 Scene_Update(void) {
    switch (sceneState) {
    case SCENE_DISC:   sceneState = Scene_UpdateDisc(); break;
    case SCENE_MODE0:  sceneState = Scene_UpdateMode0(); break;
    case SCENE_NODISC: sceneState = Scene_UpdateNoDisc(); break;
    case SCENE_MODE3:  sceneState = Scene_UpdateMode3(); break;
    case SCENE_SPINOUT: sceneState = Scene_SpinOut(); return sceneState;
    case SCENE_MENU:   sceneState = Menu_Update(); return sceneState;
    }
    Scene_DrawWorld();
    return sceneState;
}

/* 0x8130D5AC */
MtxPtr Scene_GetWorldMtx(void) { return sceneWorld; }
