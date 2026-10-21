// Intro scene (PAL 1.0 BS2, 0x8130C250..0x8130E70C): spin input, wobble,
// impact shake, the world matrix, the per-state update and 2D drawing for
// every boot path.

#include "bs2/scene.h"
#include "bs2/cube.h"
#include "bs2/j3d.h"
#include "bs2/menu.h"
#include "bs2/res.h"

/* STH0 string table entry; the string is at (u8 *)entry + ofs. */
typedef struct SthEntry {
    u16 box;               /* GLH0 text box index */
    u16 pad;
    s32 ofs;
} SthEntry;

/* GLH0 text box (0x24 bytes, table at header + 0x8) */
typedef struct GlhTextBox {
    u32 tag;
    u16 x;                 /* 0x04 12.4 */
    u16 y;                 /* 0x06 12.4 */
    u8 pad8[0x18 - 0x8];
    s16 spacing;           /* 0x18 line spacing in pixels */
    u16 pad1A;
    u16 pane;              /* 0x1C GLH0 pane index of the frame */
    u8 pad1E[0x24 - 0x1E];
} GlhTextBox;

/* J3D animation slots (BS2 model loader) */
typedef struct J3DAnmJoint {
    u8 pad0[0x14];
    void *data;            /* 0x14 */
} J3DAnmJoint;

typedef struct J3DAnmColor {
    u8 pad0[0x1C];
    void *data;            /* 0x1C */
} J3DAnmColor;

/* Not yet decompiled */
void fn_813020F4(void);                                     /* 0x813020F4 */
void fn_8130213C(void);                                     /* 0x8130213C boot the disc */
void fn_81303B00(J3DAnmJoint *anm, s32 flags);              /* 0x81303B00 */
void fn_81303B9C(J3DModelInstance *inst, J3DAnmJoint *anm); /* 0x81303B9C */
void fn_81303BA4(J3DModelInstance *inst, J3DAnmColor *anm, s32 flags); /* 0x81303BA4 */
void fn_81304CE8(void);                                     /* 0x81304CE8 */
void J3D_DrawPacket(J3DModelInstance *inst, void *packet);  /* 0x81305248 */
void MtxScaleCols(Mtx m, Vec *s);                           /* 0x81307754 */
f32 Hermite(f32 t, f32 t0, f32 p0, f32 v0, f32 t1, f32 p1, f32 v1); /* 0x81307D08 */
void fn_81308DB8(GlhTextBox *box, const char *s);           /* 0x81308DB8 */
void fn_813090A4(GlhTextBox *box, const char *s);           /* 0x813090A4 */
void Glh_MeasureString(GlhTextBox *box, const char *s, u16 *w, u16 *h); /* 0x813092D8 */
void fn_81309554(GXColor *in, GXColor *out);                /* 0x81309554 */
void fn_813095A0(GXColor *in, GXColor *out);                /* 0x813095A0 */
s32 Res_DialogLayout(void);                                 /* 0x8130B8AC */
s32 Res_DialogStrings(void);                                /* 0x8130B920 */
void Sound_UpdateSpin(s32 id, f32 level);                   /* 0x8135A8E0 */
void GXSetFog_(s32 type, GXColor color);                    /* 0x8137759C */

static void *sDialogGlh;             /* 0x814AD3C8 */
static void *sDialogSth;             /* 0x814AD3CC */

static J3DModel sBgInnerModel;       /* 0x814AF980 res 0x23 */
static J3DModelInstance sBgInner;    /* 0x814AF9B8 */
static J3DModel sBigCubeModel;       /* 0x814AFA30 res 0x67 */
static J3DModelInstance sBigCube;    /* 0x814AFA68 */
static J3DModel sLogoModel;          /* 0x814AFAE0 res 0x66 */
static J3DModelInstance sLogo;       /* 0x814AFB18 */
static J3DModel sBgOuterModel;       /* 0x814AFB90 res 0x24 */
static J3DModelInstance sBgOuter;    /* 0x814AFBC8 */
static J3DModelInstance sText;       /* 0x814AFC40 "GAMECUBE" wordmark */
static J3DModel sTextModel;          /* 0x814AFCB8 res 0x6B */
static J3DAnmJoint sTextBck;         /* 0x814AFCF0 res 0x69 */
static J3DAnmColor sTextBrk;         /* 0x814AFD08 res 0x6A */

/* 0x8130C250: hold A while the cube runs to spin the scene; released, the
   spin eases back to rest over 50 frames. */
static void Scene_UpdateSpin(BOOL force) {
    Scene *s = Scene_Get();
    s16 easeFrom = s->easeFrom;
    f32 easeSpeed = s->easeSpeed;
    s16 prev = s->spin;
    f32 level;

    if (force) {
        s->spinAccel += s->spinAccelStep;
        s->spinSpeed += s->spinAccel;
        s->spin = s->spin + s->spinSpeed;
    } else if ((gMenuPad.button & PAD_A) && Cube_IsRunning()) {
        s->spinAccel += s->spinAccelStep;
        s->spinSpeed += s->spinAccel;
        s->spin = s->spin + s->spinSpeed;
        easeFrom = s->spin;
        easeSpeed = s->spinSpeed;
        s->easeFrame = 0;
    } else {
        s->spinAccel *= 0.76f;
        s->spinSpeed *= 0.95f;
        if (s->spinAccel < 0.1) {
            s->spinAccel = 0.0f;
        }
        if (s->spinSpeed < 0.01) {
            s->spinSpeed = 0.0f;
        }
        s->spin = Hermite(s->easeFrame, 0.0f, easeFrom, easeSpeed, 50.0f, 0.0f, 0.0f);
        s->easeFrame++;
        if (s->easeFrame > 50) {
            s->easeFrame = 50;
        }
    }

    level = s->spinSpeed / 5000.0f;
    if (level > 1.0f) {
        level = 1.0f;
    }
    if (level < 0.01f) {
        level = 0.0f;
    }
    if (Cube_IsRunning() || force) {
        if (prev < 0 && s->spin >= 0 && level > s->whooshMin) {
            Sound_UpdateSpin(0, level);
        } else if (prev > 0 && s->spin < 0 && level > s->whooshMin) {
            Sound_UpdateSpin(0, level);
        }
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

    if (land < 10.0f) {
        land = 0.0f;
    }
    if (drop < 80.0f) {
        drop = 0.0f;
    }
    if (land > 0.0f || drop > 0.0f) {
        phase += s->shakeStep;
    }
    s->landImpulse = land;
    s->dropImpulse = drop;
    s->shakePhase = phase;
}

/* 0x8130C5E4 */
static void Scene_UpdateWobble(void) {
    Scene *s = Scene_Get();
    s16 phase = s->wobblePhase;
    f32 amp = s->wobbleAmp;

    if (s->spinAccel > 0.0f) {
        amp = 36.0f + amp;
    } else {
        amp = amp * 0.945f;
    }
    if (amp < 1.0f) {
        amp = 0.0f;
    }
    if (amp > 0.0f) {
        phase += 7;
    }
    if (amp > 5000.0f) {
        amp = 5000.0f;
    }
    s->wobbleAmp = amp;
    s->wobblePhase = phase;
    Scene_DecayShake();
}

/* Impact shake: rolls about X (drop) and Z (landing) and lifts the scene. */
static inline void Scene_CalcShakeMtx(Mtx m) {
    Scene *s = Scene_Get();
    s16 phase = s->shakePhase;
    f32 land = s->landImpulse;
    f32 drop = s->dropImpulse;
    f32 ty;

    ty = s->dropLift * drop * SinS(phase);
    ty = s->landLift * land * SinS(phase) + ty;
    MtxRotTrans(-1.0f * drop * SinS(phase), 0, land * SinS(phase), m, 0.0f, ty, 0.0f);
}

/* 0x8130C694: RotY(spin) * Shake * Wobble * Iso * T(15, 15, 15) */
static void Scene_CalcWorldMtx(Mtx m) {
    Scene *s = Scene_Get();
    s16 phase = s->wobblePhase;
    f32 k = s->spinTilt * s->spinSpeed / s->spinThreshold;
    f32 amp = s->wobbleAmp;
    Mtx shake;
    Mtx t;
    f32 rest;

    PSMTXIdentity(t);
    t[0][3] = t[1][3] = t[2][3] = 15.0f;
    MtxRotTrans(s->isoX, s->isoY, s->isoZ, m, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(m, t, t);
    rest = 1.0f - k;
    k = 1000.0f * k;
    MtxRotTrans(k + rest * (amp * SinS(phase * 0x5A)),
                k + rest * (amp * SinS(phase * 0x3C)),
                k + rest * (amp * SinS(phase * 0x46)), m, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(m, t, m);
    Scene_CalcShakeMtx(shake);
    PSMTXConcat(shake, m, m);
    MtxRotTrans(0, s->spin, 0, t, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(t, m, m);
}

/* Squash the world along XZ and stretch it along Y as the spin speeds up */
static inline void Scene_SquashWorld(Scene *s) {
    Vec squash = { 0.0f, 0.0f, 0.0f };
    f32 xz = 1.0f - s->squashXZ;

    squash.x = 1.0f - xz * s->spinSpeed / s->spinThreshold;
    squash.y = 1.0f + s->stretchY * s->spinSpeed / s->spinThreshold;
    squash.z = 1.0f - xz * s->spinSpeed / s->spinThreshold;
    MtxScaleCols(sWorldMtx, &squash);
}

static inline Vec Scene_VecSplat(f32 v) {
    Vec r = { 0.0f, 0.0f, 0.0f };

    r.x = v;
    r.y = v;
    r.z = v;
    return r;
}

static inline Vec Scene_VecOne(void) {
    Vec r = { 1.0f, 1.0f, 1.0f };

    return r;
}

static inline Vec Scene_VecOuter(void) {
    Vec r = { 1.01f, 1.01f, 1.01f };

    return r;
}

/* 0x8130C90C */
void Scene_DrawWorld(void) {
    Scene *s = Scene_Get();
    Mtx m;
    Mtx id;
    MtxPtr view;

    Scene_CalcWorldMtx(m);
    PSMTXIdentity(id);
    PSMTXConcat(m, id, sWorldMtx);
    Scene_SquashWorld(s);
    J3D_SetModelMtx(Scene_GetBgInner(), sWorldMtx, Scene_VecSplat(s->modelScale));
    view = Camera_GetView();
    J3D_SetViewMtx(Scene_GetBgInner(), view);
    J3D_CalcJoints(Scene_GetBgInner());
    J3D_SetModelMtx(Scene_GetLogo(), sWorldMtx, Scene_VecOne());
    view = Camera_GetView();
    J3D_SetViewMtx(Scene_GetLogo(), view);
    J3D_CalcJoints(Scene_GetLogo());
    J3D_SetModelMtx(Scene_GetBgOuter(), sWorldMtx, Scene_VecOuter());
    view = Camera_GetView();
    J3D_SetViewMtx(Scene_GetBgOuter(), view);
    J3D_CalcJoints(Scene_GetBgOuter());

    Cube_Draw();
}

/* 0x8130CB74 */
s32 Scene_UpdateNoDisc(void) {
    Scene *s = Scene_Get();
    CubeState *cube = Cube_Get();

    if (!cube->done) {
        Scene_UpdateSpin(FALSE);
    }
    Scene_UpdateWobble();
    Cube_Update(TRUE);
    if (s->spinSpeed > s->spinThreshold) {
        s->spin = s->spinRest;
        return SCENE_SPINOUT;
    }

    if (s->fatal) {
        s->fatalFade++;
        if (s->fatalFade > s->fatalFadeFrames) {
            s->fatalFade = s->fatalFadeFrames;
        }
        return SCENE_NODISC;
    }

    if (Disc_GetState() == DISC_WRONG || Disc_GetState() == DISC_RETRY) {
        s->errFade++;
        if (s->errFade > s->errFadeFrames) {
            s->errFade = s->errFadeFrames;
        }
    } else {
        s->errFade--;
        if (s->errFade < 0) {
            s->errFade = 0;
        }
    }

    if (s->coverOpened) {
        if (cube->done) {
            Disc_FadeOutStart();
            if (Disc_GetFadeAlpha() == 0) {
                Scene_UpdateSpin(TRUE);
                Scene_UpdateWobble();
                if (s->spinSpeed > s->spinThreshold) {
                    s->spin = s->spinRest;
                    return SCENE_SPINOUT;
                }
            }
        }
        return SCENE_NODISC;
    }

    switch (Disc_GetState()) {
    case DISC_READY:
        if (cube->done) {
            fn_8130213C();
        }
        break;
    case DISC_FATAL:
        s->fatal = 1;
        break;
    case DISC_NONE:
    case DISC_COVER_OPEN:
        s->coverOpened = 1;
        break;
    case DISC_READING:
    case DISC_WRONG:
    case DISC_RETRY:
        break;
    }
    return SCENE_NODISC;
}

/* 0x8130CD68: pick a language with up/down, A to confirm. */
static void Scene_LanguageInput(Scene *s) {
    if (gMenuPad.dirsNew & DIR_UP) {
        s->lang--;
        if (s->lang < 0) {
            s->lang = 5;
        }
        Sound_Play(SE_CURSOR);
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        s->lang++;
        if (s->lang >= 6) {
            s->lang = 0;
        }
        Sound_Play(SE_CURSOR);
    }
    if (gMenuPad.buttonDown & PAD_A) {
        Sound_Play(SE_DECIDE);
        Lang_Set(s->lang);
        s->langSelect = 0;
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_BUZZER);
    }
}

/* 0x8130CE50: SRAM was lost. Language select, then a notice, then any
   button spins out to the menu. */
s32 Scene_UpdateMode0(void) {
    Scene *s = Scene_Get();
    CubeState *cube = Cube_Get();

    if (Disc_GetState() == DISC_FATAL) {
        s->fatal = 1;
    }
    if (s->fatal) {
        s->fatalFade++;
        if (s->fatalFade > s->fatalFadeFrames) {
            s->fatalFade = s->fatalFadeFrames;
        }
        return SCENE_MODE0;
    }
    if (s->langSelect) {
        s->langFade += 10;
        if (s->langFade > 255) {
            s->langFade = 255;
            Scene_LanguageInput(s);
        }
        return SCENE_MODE0;
    }
    if (s->langFade > 0) {
        s->langFade -= 10;
        if (s->langFade <= 0) {
            s->langFade = 0;
        }
        return SCENE_MODE0;
    }
    if (!cube->done) {
        Scene_UpdateWobble();
        Cube_Update(TRUE);
        return SCENE_MODE0;
    }

    switch (s->m0State) {
    case 0:
        s->m0LogoFade--;
        if (s->m0LogoFade < 0) {
            s->m0LogoFade = 0;
            s->m0State = 1;
        }
        break;
    case 1:
        s->m0TextFade++;
        if (s->m0TextFade > s->m0TextFrames) {
            s->m0TextFade = s->m0TextFrames;
            s->m0State = 3;
        }
        break;
    case 3:
        if (gMenuPad.buttonDown & 0x1F70) {
            Sound_Play(SE_DECIDE);
            s->m0State = 4;
        }
        break;
    case 4:
        s->m0TextFade--;
        if (s->m0TextFade < 0) {
            s->m0TextFade = 0;
            s->m0State = 6;
        }
        break;
    case 6:
        Scene_UpdateSpin(TRUE);
        Scene_UpdateWobble();
        if (s->spinSpeed > s->spinThreshold) {
            s->spin = s->spinRest;
            return SCENE_SPINOUT;
        }
        break;
    }
    return SCENE_MODE0;
}

/* 0x8130D078 */
static void Scene_YesNoInput(Scene *s) {
    if (gMenuPad.dirsNew & DIR_UP) {
        if (s->askChoice != 0) {
            Sound_Play(SE_CURSOR);
        }
        s->askChoice = 0;
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        if (s->askChoice != 1) {
            Sound_Play(SE_CURSOR);
        }
        s->askChoice = 1;
    }
    if (gMenuPad.buttonDown & PAD_A) {
        if (s->askChoice == 0) {
            Sound_Play(SE_DECIDE);
            s->discState = 3;
        } else {
            Sound_Play(SE_CANCEL);
            s->discState = 2;
        }
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        s->discState = 2;
    }
}

#define SCENE_DISC_GONE()                                                   \
    (Disc_GetState() == DISC_COVER_OPEN || Disc_GetState() == DISC_NONE ||  \
     Disc_GetState() == DISC_RETRY || Disc_GetState() == DISC_WRONG)

/* 0x8130D168: a bootable disc is in. The cube intro runs to its end in
   one frame, then the logo fades and the game boots. */
s32 Scene_UpdateDisc(void) {
    CubeState *cube = Cube_Get();
    Scene *s = Scene_Get();

    while (!cube->done) {
        Cube_Update(FALSE);
        Scene_UpdateWobble();
    }
    if (Disc_GetState() == DISC_FATAL) {
        s->fatal = 1;
    }
    if (s->langSelect) {
        s->langFade += 10;
        if (s->langFade > 255) {
            s->langFade = 255;
            Scene_LanguageInput(s);
        }
        return SCENE_DISC;
    }
    if (s->langFade > 0) {
        s->langFade -= 10;
        if (s->langFade <= 0) {
            s->langFade = 0;
            fn_813020F4();
        }
        return SCENE_DISC;
    }

    if (Disc_GetFadeAlpha() == 0) {
        if (s->fatal) {
            s->askFade--;
            if (s->askFade < 0) {
                s->askFade = 0;
                s->fatalFade++;
                if (s->fatalFade > s->fatalFadeFrames) {
                    s->fatalFade = s->fatalFadeFrames;
                }
            }
            return SCENE_DISC;
        }
        s->discWait++;
        if (s->discWait > 20) {
            s->discWait = 20;
            switch (s->discState) {
            case 0:
                s->discFade++;
                if (s->discFade > s->discFadeFrames) {
                    s->discFade = s->discFadeFrames;
                    s->discState = 1;
                }
                break;
            case 1:
                s->askFade++;
                if (s->askFade > s->askFadeFrames) {
                    s->askFade = s->askFadeFrames;
                }
                Scene_YesNoInput(s);
                break;
            case 2:
                s->askFade--;
                if (s->askFade < 0) {
                    s->askFade = 0;
                    s->discFade--;
                    if (s->discFade < 0) {
                        s->discFade = 0;
                        s->discState = 5;
                    }
                }
                break;
            case 3:
                s->askFade--;
                if (s->askFade < 0) {
                    s->askFade = 0;
                    s->discState = 4;
                }
                break;
            case 4:
                s->discFade--;
                if (s->discFade < 0) {
                    s->discFade = 0;
                    Scene_UpdateSpin(TRUE);
                    Scene_UpdateWobble();
                    if (s->spinSpeed > s->spinThreshold) {
                        s->spin = s->spinRest;
                        return SCENE_SPINOUT;
                    }
                }
                break;
            case 5:
                if (SCENE_DISC_GONE()) {
                    s->discState = 3;
                } else if (Disc_GetState() == DISC_READY) {
                    fn_8130213C();
                }
                break;
            }
        }
    }
    if (s->discState == 5) {
        if (SCENE_DISC_GONE()) {
            s->discState = 3;
        } else if (Disc_GetState() == DISC_READY) {
            fn_8130213C();
        }
    } else {
        Disc_FadeOutStart();
    }
    return SCENE_DISC;
}

/* 0x8130D4E0: cover open or drive error. */
s32 Scene_UpdateMode3(void) {
    CubeState *cube = Cube_Get();
    Scene *s = Scene_Get();

    while (!cube->done) {
        Cube_Update(FALSE);
        Scene_UpdateWobble();
        fn_813020F4();
    }
    Disc_FadeOutStart();
    s->m3Wait--;
    if (s->m3Wait < 0) {
        s->m3Wait = 0;
        Scene_UpdateSpin(TRUE);
        Scene_UpdateWobble();
        if (s->spinSpeed > s->spinThreshold) {
            s->spin = s->spinRest;
            return SCENE_SPINOUT;
        }
    }
    return SCENE_MODE3;
}

/* 0x8130D5AC */
MtxPtr Scene_GetWorldMtx(void) {
    return sWorldMtx;
}

/* 0x8130D5B8 */
void Scene_LoadLangRes(void) {
    sDialogSth = ResGet(Res_DialogStrings());
}

/* 0x8130D5E0 */
void Scene_InitModels(void) {
    sBgInnerModel.data = ResGet(0x23);
    sBgInner.model = &sBgInnerModel;
    J3D_InitModel(&sBgInner, 1);
    sBigCubeModel.data = ResGet(0x67);
    sBigCube.model = &sBigCubeModel;
    J3D_InitModel(&sBigCube, 1);
    sLogoModel.data = ResGet(0x66);
    sLogo.model = &sLogoModel;
    J3D_InitModel(&sLogo, 1);
    sBgOuterModel.data = ResGet(0x24);
    sBgOuter.model = &sBgOuterModel;
    J3D_InitModel(&sBgOuter, 1);
    sTextModel.data = ResGet(0x6B);
    sText.model = &sTextModel;
    J3D_InitModel(&sText, 1);
    sTextBck.data = ResGet(0x69);
    fn_81303B00(&sTextBck, 1);
    sTextBrk.data = ResGet(0x6A);
    fn_81303BA4(&sText, &sTextBrk, 1);
    fn_81303B9C(&sText, &sTextBck);
    sDialogGlh = ResGet(Res_DialogLayout());
    sDialogSth = ResGet(Res_DialogStrings());
}

/* 0x8130D700 */
J3DModelInstance *Scene_GetBgInner(void) {
    return &sBgInner;
}

/* 0x8130D70C */
J3DModelInstance *Scene_GetBigCube(void) {
    return &sBigCube;
}

/* 0x8130D718 */
J3DModelInstance *Scene_GetLogo(void) {
    return &sLogo;
}

/* 0x8130D724 */
void *Scene_GetLogoText(void) {
    return &sText;
}

/* 0x8130D730 */
J3DModelInstance *Scene_GetBgOuter(void) {
    return &sBgOuter;
}

#define SCENE_SHAKE_MAX 3000

/* Inner background alpha follows the impact shake. */
static inline u8 Scene_ShakeAlpha(Scene *s, s32 shake) {
    return s->shakeAlpha * shake / SCENE_SHAKE_MAX;
}

static inline void Scene_DrawDialogText(s32 idx, GXColor *c) {
    Glh_DrawText(idx, sDialogSth, sDialogGlh, c);
}

/* 0x8130D73C: no disc / cover open: the cube intro and the disc errors. */
void Scene_Draw2DNoDisc(void) {
    Scene *s = Scene_Get();
    GXColor c = { 255, 255, 255, 255 };
    u8 fade0 = 255 - s->unkC4 * 150 / 30;
    u8 fade1 = 255 - s->unkCE * 150 / 30;
    u8 fade = fade0 > fade1 ? fade1 : fade0;
    s32 shake;
    s16 wobble;

    shake = s->landImpulse + s->dropImpulse;
    if (shake > SCENE_SHAKE_MAX) {
        shake = SCENE_SHAKE_MAX;
    }
    sText.alpha = (u8)(fade * 255 / 255);
    J3D_Draw(&sText);
    fn_81304CE8();
    sBgInner.alpha = (u8)(fade * Scene_ShakeAlpha(s, shake) / 255);
    if (sBgInner.alpha > 0) {
        J3D_Draw(&sBgInner);
    }
    wobble = s->wobbleAmp;
    if (wobble > SCENE_SHAKE_MAX) {
        wobble = SCENE_SHAKE_MAX;
    }
    sBgOuter.alpha = (u8)(fade * Scene_ShakeAlpha(s, wobble) / 255);
    if (sBgOuter.alpha > 0) {
        J3D_DrawPacket(&sBgOuter, (u8 *)sBgOuter.model->packets + 0x30);
    }
    Cube_DrawBig((255 - s->logoAlpha) * fade / 255);
    sLogo.alpha = (u8)(s->logoAlpha * fade / 255);
    if (sLogo.alpha > 0) {
        J3D_Draw(&sLogo);
    }
    if (sBgOuter.alpha > 0) {
        J3D_DrawPacket(&sBgOuter, (u8 *)sBgOuter.model->packets + 0x78);
    }

    c.a = s->fatalFade * 255 / s->fatalFadeFrames;
    if (c.a != 0) {
        GX_Setup2DView();
        GX_Setup2DText();
        Scene_DrawDialogText(4, &c);
    }
    c.a = s->errFade * 255 / s->errFadeFrames;
    if (c.a != 0) {
        GX_Setup2DView();
        GX_Setup2DText();
        Scene_DrawDialogText(5, &c);
    }
}

/* 0x8130D9FC: a vertical list of `count` strings starting at entry `first`,
   with `sel` highlighted. */
static void Scene_DrawList(GlhTextBox *boxes, SthEntry *entries, u16 first, u16 count, u16 y,
                           u16 h, s16 sel, u8 alpha) {
    DispSettings *disp = Disp_GetSettings();
    GXColor hi = { 255, 255, 255, 255 };
    GXColor lo = { 0xF0, 0xE6, 0x00, 0xFF };
    GlhTextBox box;
    s32 i;
    u32 ofs = first * 8;

    hi.a = alpha;
    lo.a = alpha;
    box = boxes[*(u16 *)((u8 *)entries + ofs)];
    box.y = y + disp->textPad * 16 + (s32)h / 2;
    GX_Setup2DText();
    for (i = 0; i < count; i++) {
        fn_813095A0(&lo, &lo);
        fn_81309554(&hi, &hi);
        if (sel == i) {
            SthEntry *e = (SthEntry *)((u8 *)entries + ofs);
            fn_813090A4(&box, (char *)e + e->ofs);
        } else {
            SthEntry *e = (SthEntry *)((u8 *)entries + ofs);
            fn_81308DB8(&box, (char *)e + e->ofs);
        }
        ofs += 8;
        box.y += box.spacing * 16;
    }
}

/* 0x8130DB58: the language list (strings 12..17) in a frame. */
static void Scene_DrawLanguageList(Scene *s) {
    DispSettings *disp = Disp_GetSettings();
    u16 maxW = 0;
    u16 h = 0;
    u16 i;
    SthEntry *entries = (SthEntry *)((u8 *)sDialogSth + 8);
    GlhTextBox *boxes = (GlhTextBox *)((u8 *)sDialogGlh + ((u32 *)sDialogGlh)[2]);
    GlhPane *panes = (GlhPane *)((u8 *)sDialogGlh + ((u32 *)sDialogGlh)[3]);
    GXColor c = { 255, 255, 255, 255 };
    u16 w;
    GlhTextBox *box;
    GlhPane *pane;
    u16 y;

    c.a = s->langFade;
    for (i = 0; i < 6; i++) {
        Glh_MeasureString(&boxes[entries[12 + i].box], (char *)&entries[12 + i] + entries[12 + i].ofs,
                          &w, &h);
        if (maxW < w) {
            maxW = w;
        }
    }
    box = &boxes[entries[12].box];
    pane = &panes[box->pane];
    w = maxW + (disp->textPad << 5);
    h = h + box->spacing * 80 + (disp->textPad << 5);
    y = pane->y - h / 2;
    Glh_DrawPaneRect(boxes[entries[12].box].pane, sDialogGlh, &c, (u16)(pane->x - w / 2), y, w, h);
    Scene_DrawList(boxes, entries, 12, 6, y, h, s->lang, s->langFade);
}

static inline void Scene_DrawLanguageScreen(Scene *s, GXColor *c) {
    c->a = s->langFade;
    GX_Setup2DView();
    GX_Setup2DText();
    Scene_DrawDialogText(6, c);
    Scene_DrawDialogText(7, c);
    Scene_DrawDialogText(8, c);
    Scene_DrawDialogText(9, c);
    Scene_DrawDialogText(10, c);
    Scene_DrawDialogText(11, c);
    Scene_DrawLanguageList(s);
}

/* 0x8130DCDC */
void Scene_Draw2DMode0(void) {
    Scene *s = Scene_Get();
    GXColor c = { 255, 255, 255, 255 };
    s32 shake;
    s16 wobble;

    if (s->langSelect || s->langFade > 0) {
        Scene_DrawLanguageScreen(s, &c);
        return;
    }

    sText.alpha = (u8)(s->m0LogoFade * 255 / s->m0LogoFrames);
    J3D_Draw(&sText);
    shake = s->landImpulse + s->dropImpulse;
    if (shake > SCENE_SHAKE_MAX) {
        shake = SCENE_SHAKE_MAX;
    }
    sBgInner.alpha = Scene_ShakeAlpha(s, shake);
    fn_81304CE8();
    if (sBgInner.alpha > 0) {
        J3D_Draw(&sBgInner);
    }
    wobble = s->wobbleAmp;
    if (wobble > SCENE_SHAKE_MAX) {
        wobble = SCENE_SHAKE_MAX;
    }
    sBgOuter.alpha = Scene_ShakeAlpha(s, wobble);
    if (sBgOuter.alpha > 0) {
        J3D_DrawPacket(&sBgOuter, (u8 *)sBgOuter.model->packets + 0x30);
    }
    Cube_DrawBig(255 - s->logoAlpha);
    sLogo.alpha = s->logoAlpha;
    if (sLogo.alpha > 0) {
        J3D_Draw(&sLogo);
    }
    if (sBgOuter.alpha > 0) {
        J3D_DrawPacket(&sBgOuter, (u8 *)sBgOuter.model->packets + 0x78);
    }

    GX_Setup2DView();
    GX_Setup2DText();
    c.a = s->m0TextFade * 255 / s->m0TextFrames;
    if (c.a != 0) {
        Scene_DrawDialogText(3, &c);
    }
    c.a = s->fatalFade * 255 / s->fatalFadeFrames;
    if (c.a != 0) {
        Scene_DrawDialogText(4, &c);
    }
}

/* 0x8130DF64: string `idx` with a yes/no choice under it, in a frame. */
static void Scene_DrawAsk(Scene *s, u16 idx, s16 sel, u8 alpha) {
    DispSettings *disp = Disp_GetSettings();
    u16 maxW = 0;
    u16 textW = 0;
    u16 textH = 0;
    u16 listH = 0;
    SthEntry *entries = (SthEntry *)((u8 *)sDialogSth + 8);
    GlhTextBox *boxes = (GlhTextBox *)((u8 *)sDialogGlh + ((u32 *)sDialogGlh)[2]);
    GXColor c = { 255, 255, 255, 255 };
    SthEntry *e;
    SthEntry *opt;
    GlhTextBox *box;
    u16 w;
    u16 i;
    u16 frameH;
    u16 totalH;
    u16 y;

    c.a = alpha;
    GX_Setup2DView();
    GX_Setup2DText();
    e = &entries[idx];
    Glh_MeasureString(&boxes[e->box], (char *)e + e->ofs, &textW, &textH);
    w = 0;
    for (i = 0, opt = entries; i < 2; i++, opt++) {
        Glh_MeasureString(&boxes[opt[1].box], (char *)&entries[i + 1] + opt[1].ofs, &w, &listH);
        if (maxW < w) {
            maxW = w;
        }
    }
    textH += disp->textPad << 5;
    frameH = listH + boxes[entries[1].box].spacing * 16 + (disp->textPad << 5);
    totalH = textH + (frameH + s->askGap * 16);
    Glh_DrawStringOfs(idx, sDialogSth, sDialogGlh, &c, 0, (u16)(-totalH / 2 + textH / 2));
    box = &boxes[e->box];
    y = totalH / 2 + box->y - frameH;
    Glh_DrawPaneRect(box->pane, sDialogGlh, &c, (u16)(box->x - (maxW / 2 + disp->textPad * 16)), y,
                     maxW + (disp->textPad << 5), frameH);
    Scene_DrawList(boxes, entries, 1, 2, y, listH, sel, alpha);
}

/* 0x8130E17C */
void Scene_Draw2DDisc(void) {
    Scene *s = Scene_Get();
    GXColor c = { 255, 255, 255, 255 };
    s16 wobble;

    if (s->langSelect || s->langFade > 0) {
        Scene_DrawLanguageScreen(s, &c);
        return;
    }

    wobble = s->wobbleAmp;
    if (wobble > SCENE_SHAKE_MAX) {
        wobble = SCENE_SHAKE_MAX;
    }
    sBgOuter.alpha = Scene_ShakeAlpha(s, wobble);
    if (sBgOuter.alpha > 0) {
        J3D_DrawPacket(&sBgOuter, (u8 *)sBgOuter.model->packets + 0x30);
    }
    sLogo.alpha = s->logoAlpha - s->discLogoDim * s->discFade / s->discFadeFrames;
    sText.alpha = sLogo.alpha;
    if (sLogo.alpha > 0) {
        J3D_Draw(&sLogo);
        J3D_Draw(&sText);
    }
    wobble = s->wobbleAmp;
    if (wobble > SCENE_SHAKE_MAX) {
        wobble = SCENE_SHAKE_MAX;
    }
    sBgOuter.alpha = Scene_ShakeAlpha(s, wobble);
    if (sBgOuter.alpha > 0) {
        J3D_DrawPacket(&sBgOuter, (u8 *)sBgOuter.model->packets + 0x78);
    }
    Scene_DrawAsk(s, 0, s->askChoice, s->askFade * 255 / s->askFadeFrames);
    c.a = s->fatalFade * 255 / s->fatalFadeFrames;
    if (c.a != 0) {
        GX_Setup2DView();
        GX_Setup2DText();
        Scene_DrawDialogText(4, &c);
    }
}

/* 0x8130E3C8 */
void Scene_Draw2DSpinOut(void) {
    Scene *s = Scene_Get();
    s32 shake;
    s32 end;
    s32 start;

    if (s->outFrame <= s->outFadeFrames) {
        shake = s->landImpulse + s->dropImpulse + s->wobbleAmp;
        if (shake > SCENE_SHAKE_MAX) {
            shake = SCENE_SHAKE_MAX;
        }
        sBgInner.alpha = Scene_ShakeAlpha(s, shake);
        sBigCube.alpha = (u8)(s->bgAlpha * s->outFrame / s->outFadeFrames);
        sText.alpha = (u8)(s->bgAlpha * s->outFrame / s->outFadeFrames);
        sBgInner.alpha = (u8)((s->outFadeFrames - s->outFrame) * (s->shakeAlpha * shake) /
                                         (s->outFadeFrames * SCENE_SHAKE_MAX));
        sText.alpha = sBgInner.alpha * s->m0LogoFade / s->m0LogoFrames;
        if (sText.alpha < 0) {
            sText.alpha = 0;
        }
        J3D_Draw(&sBgInner);
        J3D_Draw(&sText);
    } else {
        sBigCube.alpha = s->bgAlpha;
    }
    if (sBigCube.alpha > 0) {
        GXColor fog = { 0x28, 0x28, 0x28, 0xFF };
        GXColor nofog = { 0xFF, 0xFF, 0xFF, 0xFF };
        GXSetFog_(4, fog);
        J3D_Draw(&sBigCube);
        GXSetFog_(4, nofog);
    }
    end = s->outTurnFrames + s->outSettleFrames + s->outCoastFrames;
    start = end - s->outFaceFrames;
    if (s->outFrame >= start && s->outFrame <= end) {
        Menu_DrawSpinOutFace(Scene_GetSpinOutMtx(), (s16)((s->outFrame - start) * 255 / (end - start)));
    } else if (s->outFrame > end) {
        Menu_DrawSpinOutFace(Scene_GetSpinOutMtx(), 255);
    }
}

/* 0x8130E5C0 */
void Scene_Draw2DMenu(void) {
    MenuWork *menu;
    u8 alpha;

    Scene_Get();
    menu = Menu_Get();
    alpha = menu->bgMinAlpha + (255 - menu->bgMinAlpha) * (0x7FFF - menu->zoom) / 0x7FFF;
    sBigCube.alpha = alpha;
    if (sBigCube.alpha > 0) {
        GXColor fog = { 0x28, 0x28, 0x28, 0xFF };
        GXColor nofog = { 0xFF, 0xFF, 0xFF, 0xFF };
        GXSetFog_(4, fog);
        J3D_DrawPacket(&sBigCube, (u8 *)sBigCube.model->packets + 0x78);
        GXSetFog_(4, nofog);
    }
    CardIcon_Draw();
    GameIcon_Draw();
    OptionIcon_Draw();
    ClockIcon_Draw();
    if (sBigCube.alpha > 0) {
        GXColor fog = { 0x28, 0x28, 0x28, 0xFF };
        GXColor nofog = { 0xFF, 0xFF, 0xFF, 0xFF };
        GXSetFog_(4, fog);
        J3D_DrawPacket(&sBigCube, (u8 *)sBigCube.model->packets + 0x168);
        GXSetFog_(4, nofog);
    }
    Menu_Draw2D(alpha);
}
