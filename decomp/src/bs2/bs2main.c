// BS2 main loop (PAL 1.0 BS2, 0x81300F50..0x81301190): display settings
// defaults, the per-frame main loop and the scene projection setup.

#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
u32 ARInit(u32* stack, u32 num);
void ARQInit(void);
#include <bs2/scene.h>

typedef struct DispSettings {
    f32 near;       /* 0x00 */
    f32 far;        /* 0x04 */
    f32 fovY;       /* 0x08 */
    f32 unk0C;      /* 0x0C */
    f32 unk10;      /* 0x10 */
    f32 unk14;      /* 0x14 */
    f32 unk18;      /* 0x18 */
    u8 dirty;       /* 0x1C */
    u8 unk1D;       /* 0x1D */
    u8 ortho;       /* 0x1E */
    f32 unk20;      /* 0x20 */
    f32 unk24;      /* 0x24 */
    u8 unk28;       /* 0x28 */
    u8 unk29;       /* 0x29 */
    u8 unk2A;       /* 0x2A */
    u8 unk2B;       /* 0x2B */
    s16 unk2C;      /* 0x2C */
    s16 unk2E;      /* 0x2E */
    s16 unk30;      /* 0x30 */
    s16 unk32;      /* 0x32 */
    s16 unk34;      /* 0x34 */
    s16 unk36;      /* 0x36 */
    s16 unk38;      /* 0x38 */
    s16 unk3A;      /* 0x3A */
    u8 pad3C[0x50 - 0x3C];
} DispSettings;

void HeapInit(void);                              /* 0x81307D90 */
void* HeapAlloc(u32 size);                        /* 0x81307E40 */
void DEMOInit(u32 arg, GXRenderModeObj* mode);    /* 0x813012D0 */
void fn_8135B0C0(void* buf, u32 size, u32 arg);   /* 0x8135B0C0 */
void Lang_Init(void);
void ResInit(void);
void fn_813025A4(void);
void __CARDSetDiskID(void* id);
void CardThread_Init(void);
void DEMOPadInit(void);
void fn_81308208(s32);
void RandInit(void);                  /* 0x81307EB8 */
void fn_813022F4(void);
void Sound_SetOutputMode(u32);
void DEMOPadRead(void);
void InitDVDLoader(void);
void DEMOBeforeRender(void);
void fn_8130903C(void);
void fn_81302DF0(void);
void fn_81302FA0(void);
void fn_813022C4(void);
void DEMODoneRender(void);
void fn_813015A4(void);
void fn_81301644(void);
void fn_81301724(void);
void fn_8130BEDC(void);
void CardThread_Poll(void);
void fn_81302368(void);
void fn_81301100(void);

GXRenderModeObj BS2RenderMode = {
    VI_TVMODE_PAL_INT,
    592, 262, 520, 40, 27, 640, 520,
    VI_XFBMODE_DF,
    GX_FALSE,
    GX_TRUE,
    {
        {3, 2}, {9, 6}, {3, 10},
        {3, 2}, {9, 6}, {3, 10},
        {9, 2}, {3, 6}, {9, 10},
        {9, 2}, {3, 6}, {9, 10},
    },
    {8, 8, 10, 12, 10, 8, 8},
};

DispSettings* lbl_814AD2A8;
u32 lbl_814AE750[0x100];
DispSettings lbl_814AEB50;

/* 0x81300F50 */
void fn_81300F50(DispSettings* s) {
    s->near = 50.0f;
    s->far = 10000.0f;
    s->fovY = 20.0f;
    s->unk0C = 2000.0f;
    s->unk10 = -55.0f;
    s->unk14 = -55.0f;
    s->unk18 = -10.0f;
    s->dirty = 1;
    s->unk1D = 0;
    s->ortho = 1;
    s->unk20 = 1.0f;
    s->unk24 = 0.0f;
    s->unk28 = 0;
    s->unk29 = 0;
    s->unk2A = 0;
    s->unk2B = 8;
    s->unk2C = 6;
    s->unk2E = 35;
    s->unk30 = 10;
    s->unk32 = 255;
    s->unk34 = 70;
    s->unk36 = 0;
    s->unk38 = 35;
    s->unk3A = 1;
}

/* 0x81300FF4 */
void BS2Main(void) {
    GXRenderModeObj* mode;

    ARInit(lbl_814AE750, 0x100);
    ARQInit();
    mode = &BS2RenderMode;
    HeapInit();
    DEMOInit(0x80000, mode);
    fn_8135B0C0(HeapAlloc(0x100000), 0x100000, 0x800000);
    Lang_Init();
    ResInit();
    fn_813025A4();
    lbl_814AD2A8 = &lbl_814AEB50;
    fn_81300F50(lbl_814AD2A8);
    __CARDSetDiskID(NULL);
    CardThread_Init();
    DEMOPadInit();
    fn_81308208(0);
    SinTableInit(12);
    RandInit();
    fn_813022F4();
    Scene_Init(1);
    Sound_SetOutputMode(OSGetSoundMode());

    while (TRUE) {
        DEMOPadRead();
        InitDVDLoader();
        DEMOBeforeRender();
        fn_8130903C();
        fn_81302DF0();
        fn_81302FA0();
        Scene_Update();
        fn_81301100();
        Scene_Draw2D();
        fn_813022C4();
        DEMODoneRender();
        fn_813015A4();
        fn_81301100();
        Scene_Draw2D();
        fn_813022C4();
        fn_81301644();
        GXFlush();
        fn_81301724();
        fn_8130BEDC();
        CardThread_Poll();
        fn_81302368();
    }
}

/* 0x813010F8 */
DispSettings* Disp_GetSettings(void) {
    return lbl_814AD2A8;
}

/* 0x81301100 */
void fn_81301100(void) {
    Mtx44 m;
    Scene* scene;

    scene = Scene_Get();
    if (!lbl_814AD2A8->ortho) {
        C_MTXPerspective(m, lbl_814AD2A8->fovY, 592.0f / 448.0f, lbl_814AD2A8->near, lbl_814AD2A8->far);
        GXSetProjection(m, GX_PERSPECTIVE);
    } else {
        C_MTXOrtho(m, 224.0f, -224.0f, -296.0f, 292.0f, lbl_814AD2A8->near, lbl_814AD2A8->far);
        GXSetProjection(m, GX_ORTHOGRAPHIC);
    }
    lbl_814AD2A8->dirty = 0;
}
