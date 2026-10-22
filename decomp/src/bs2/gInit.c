// gInit.c (PAL 1.0 BS2, 0x81301190..0x81302688): the IPL's modified SDK DEMO
// layer (render mode, XFB double buffering, frame timing), 2D GX setup,
// screen dimming, SRAM / RTC init and the disc boot state helpers.
//
// The DEMO functions follow the structure of the SDK DEMOInit.c as
// reconstructed in zeldaret/tp (libs/dolphin/src/demo/DEMOInit.c, CC0).

#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <dolphin/pad.h>
#include <dolphin/mtx.h>
#include "bs2/scene.h"

void *HeapAlloc(u32 size);                  /* 0x81307E40 */
void *memset(void *dst, int c, u32 n);
s32 DVDStep(void);
BOOL __OSCheckSram(void);
void __OSSetTime(OSTime time);
void __VISetAdjustingValues(s16 x, s16 y);
BOOL OSGetResetSwitchState(void);
u8 CardThread_IsEnabled(void);
void CardThread_Enable(void);
void CardThread_UnmountAll(void);
void fn_8135B180(void);
void fn_8135B200(void);
BOOL fn_8135B2C0(void);
void fn_81300830(void);
void fn_81300668(void);
void fn_813006D4(void *buf, u32 size);
BOOL fn_813006E0(void);

/* .sdata */
u8 lbl_814ACFC8 = TRUE;                     /* first frame: unblank VI */

/* .sbss */
/* declared in reverse: CW lays .sbss out last-declared first */
BOOL lbl_814AD2FC;                          /* previous reset switch state */
BOOL lbl_814AD2F8;
BOOL lbl_814AD2F4;
void *lbl_814AD2F0;                         /* banner buffer */
s32 lbl_814AD2EC;                           /* disc state */
s16 lbl_814AD2E8;                           /* fade alpha */
BOOL lbl_814AD2E4;                          /* reset fade active */
BOOL lbl_814AD2E0;                          /* SRAM was lost */
BOOL lbl_814AD2DC;                          /* SRAM was reinitialized */
f32 lbl_814AD2D8;                           /* frame time in ms */
OSTime lbl_814AD2D0;                        /* frame end time */
OSTime lbl_814AD2C8;                        /* frame start time */
GXFifoObj *lbl_814AD2C4;                    /* DefaultFifoObj */
void *lbl_814AD2C0;                         /* DefaultFifo */
GXRenderModeObj *lbl_814AD2BC;              /* rmode */
void *lbl_814AD2B8;                         /* DemoCurrentBuffer */
void *lbl_814AD2B4;                         /* DemoFrameBuffer2 */
void *lbl_814AD2B0;                         /* DemoFrameBuffer1 */

/* .bss */
u8 lbl_814AEBA0[0xA00];

/* 0x81301190 */
/* Not inlined into DEMOInit in the IPL. */
#pragma push
#pragma auto_inline off
void __DEMOInitGX(GXRenderModeObj *rmode)
{
    GXSetViewport(0.0f, 0.0f, rmode->fbWidth, rmode->xfbHeight, 0.0f, 1.0f);
    GXSetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GXSetDispCopyDst((u16)((rmode->fbWidth + 15) & ~15), rmode->xfbHeight);
    GXSetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    if (rmode->aa && rmode->xfbHeight >= 440) {
        GXSetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
        GXSetDispCopyYScale(1.0f);
        GXSetDither(GX_TRUE);
        GXSetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
    } else {
        GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
        GXSetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
        GXSetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    }
}
#pragma pop

/* Inline helper locals take stack slots in the caller's frame; the helpers
 * below reproduce the original frame layouts of DEMOInit and fn_81302368. */
static inline void SetClear(GXColor *c)
{
    GXColor *p = c;

    GXSetCopyClear(*p, 0xFFFFFF);
}

static inline void __DEMOInitVI(void)
{
    GXColor black = { 0, 0, 0, 0xFF };

    SetClear(&black);
    GXCopyDisp(lbl_814AD2B0, GX_TRUE);
    GXCopyDisp(lbl_814AD2B0, GX_TRUE);
    GXCopyDisp(lbl_814AD2B4, GX_TRUE);
    GXCopyDisp(lbl_814AD2B4, GX_TRUE);
    GXSetDispCopyGamma(GX_GM_1_0);
    VISetBlack(TRUE);
    VISetNextFrameBuffer(lbl_814AD2B8);
    VIFlush();
    VIWaitForRetrace();
}

/* 0x813012D0 */
void DEMOInit(u32 fifoSize, GXRenderModeObj *mode)
{
    u32 size;

    size = (u16)(mode->fbWidth + 15 & ~15) * mode->xfbHeight * 2;
    lbl_814AD2B0 = HeapAlloc(size);
    lbl_814AD2B4 = HeapAlloc(size);
    lbl_814AD2B8 = lbl_814AD2B4;

    if (mode != NULL) {
        lbl_814AD2BC = mode;
    } else {
        switch (VIGetTvFormat()) {
        case VI_NTSC:
            lbl_814AD2BC = &GXNtsc480IntDf;
            break;
        case VI_PAL:
            lbl_814AD2BC = &GXPal528IntDf;
            break;
        case VI_MPAL:
            lbl_814AD2BC = &GXMpal480IntDf;
            break;
        default:
            OSPanic(__FILE__, 102, "DEMOInit: invalid TV format\n");
            break;
        }
    }

    VIConfigure(mode);
    size = (fifoSize + 31) & ~31;
    lbl_814AD2C0 = HeapAlloc(size);
    lbl_814AD2C4 = GXInit(lbl_814AD2C0, size);
    __DEMOInitGX(mode);
    __DEMOInitVI();
}

/* 0x81301468 */
void DEMOBeforeRender(void)
{
    GXRenderModeObj *rmode;

    lbl_814AD2C8 = OSGetTime();
    rmode = lbl_814AD2BC;
    if (rmode->field_rendering) {
        OSPanic(__FILE__, 198, "not support field rendering\n");
        rmode = lbl_814AD2BC;
        GXSetViewportJitter(0.0f, 0.0f, rmode->fbWidth, rmode->xfbHeight, 0.0f, 1.0f, VIGetNextField());
    } else {
        GXSetViewport(0.0f, 0.0f, 0.5f + rmode->fbWidth, 0.5f + rmode->xfbHeight, 0.0f, 1.0f);
    }
    GXInvalidateVtxCache();
    GXInvalidateTexAll();
    rmode = lbl_814AD2BC;
    if (rmode->aa && rmode->xfbHeight >= 440) {
        GXSetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
        GXSetScissorBoxOffset(0, 0);
    }
}

/* 0x813015A4 */
void fn_813015A4(void)
{
    GXRenderModeObj *rmode = lbl_814AD2BC;

    if (rmode->aa && rmode->xfbHeight >= 440) {
        GXSetScissor(0, rmode->efbHeight - 4, rmode->fbWidth, rmode->efbHeight);
        GXSetScissorBoxOffset(0, lbl_814AD2BC->efbHeight - 4);
    }
}

/* 0x81301604 */
void DEMODoneRender(void)
{
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXCopyDisp(lbl_814AD2B8, GX_TRUE);
}

/* 0x81301644 */
void fn_81301644(void)
{
    u16 height = lbl_814AD2BC->efbHeight - 2;
    u32 offset = (u16)(lbl_814AD2BC->fbWidth + 15 & ~15) * height * 2;

    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXSetCopyClamp(GX_CLAMP_BOTTOM);
    GXSetDispCopySrc(0, 2, lbl_814AD2BC->fbWidth, height);
    GXCopyDisp((u8 *)lbl_814AD2B8 + offset, GX_TRUE);
    GXSetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
    GXSetDispCopySrc(0, 0, lbl_814AD2BC->fbWidth, 2);
    GXCopyDisp(lbl_814AEBA0, GX_TRUE);
    GXSetDispCopySrc(0, 0, lbl_814AD2BC->fbWidth, lbl_814AD2BC->efbHeight);
}

/* 0x81301724 */
void fn_81301724(void)
{
    GXDrawDone();
    lbl_814AD2D0 = OSGetTime();
    lbl_814AD2D8 = OSTicksToMicroseconds(lbl_814AD2D0 - lbl_814AD2C8) / 1000.0f;
    VISetNextFrameBuffer(lbl_814AD2B8);
    if (lbl_814ACFC8) {
        VISetBlack(FALSE);
        lbl_814ACFC8 = FALSE;
    }
    VIFlush();
    VIWaitForRetrace();
    if (lbl_814AD2B8 == lbl_814AD2B0) {
        lbl_814AD2B8 = lbl_814AD2B4;
    } else {
        lbl_814AD2B8 = lbl_814AD2B0;
    }
}

/* 0x8130180C */
void GX_Setup2D(BOOL tex, BOOL color, BOOL modulate)
{
    u16 src = GX_SRC_REG;

    if (!tex && !color) {
        return;
    }

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 4);
    if (color) {
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        src = GX_SRC_VTX;
    }
    if (tex) {
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_S16, 14);
    }
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, (GXColorSrc)src, (GXColorSrc)src, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);

    if (tex) {
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3X4 /* = 1, SDK GX_TG_MTX2x4 */, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        if (modulate) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
        } else {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
        }
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    } else {
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
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
    }
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXSetCullMode(GX_CULL_BACK);
}

/* 0x81301BC8 */
/* Unreferenced camera vectors that sit in front of Screen_Dim's ones in .rodata. */
static const Vec sUnused[3] = { { 0.0f, 0.0f, 100.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
void Screen_Dim(u8 alpha)
{
    Mtx m;
    Vec up = { 0.0f, 1.0f, 0.0f };
    Vec pos = { 0.0f, 0.0f, 1700.0f };
    Vec at = { 0.0f, 0.0f, 0.0f };
    u32 color = alpha;

    C_MTXLookAt(m, &pos, &up, &at);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXSetZCompLoc(GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetCullMode(GX_CULL_NONE);

    GXBegin(GX_QUADS, GX_VTXFMT1, 4);
    GXPosition3s16(-400, 400, 10);
    GXColor1u32(color);
    GXPosition3s16(400, 400, 10);
    GXColor1u32(color);
    GXPosition3s16(400, -400, 10);
    GXColor1u32(color);
    GXPosition3s16(-400, -400, 10);
    GXColor1u32(color);
    GXEnd();
}

/* 0x81301E3C */
void fn_81301E3C(OSSram *sram)
{
    u32 bias = sram->counterBias;
    u32 rtc;
    OSCalendarTime td;

    if (__OSGetRTC(&rtc)) {
        OSTicksToCalendarTime(OSSecondsToTicks((OSTime)(rtc + bias)), &td);
        if (td.year < 2000 || td.year > 2007) {
            bias = 0;
        }
    } else {
        bias = 0;
    }
    memset(sram, 0, sizeof(OSSram));
    sram->counterBias = bias;
    sram->flags |= 0x10;
    sram->flags |= 0x04;
    lbl_814AD2DC = TRUE;
}

/* 0x81301F0C */
void fn_81301F0C(void)
{
    OSSram *sram;
    BOOL dirty = FALSE;

    lbl_814AD2DC = FALSE;
    lbl_814AD2E0 = FALSE;
    sram = __OSLockSram();
    if (!__OSCheckSram()) {
        fn_81301E3C(sram);
        dirty = TRUE;
    }
    if (!(sram->flags & 0x08)) {
        lbl_814AD2E0 = TRUE;
    }
    __OSUnlockSram(dirty);
    if (dirty) {
        while (!__OSSyncSram()) {
        }
    }
}

/* 0x81302034 */
void BS2SetTime(void)
{
    OSSram *sram = __OSLockSram();
    u32 rtc;

    if (__OSGetRTC(&rtc)) {
        rtc += sram->counterBias;
        __OSSetTime(OSSecondsToTicks((OSTime)rtc));
    }
    __OSUnlockSram(FALSE);
}

/* 0x813020B8 */
BOOL fn_813020B8(void)
{
    return lbl_814AD2DC;
}

/* 0x813020C0 */
BOOL Boot_SramWasLost(void)
{
    return lbl_814AD2E0;
}

/* 0x813020C8 */
void InitDVDLoader(void)
{
    lbl_814AD2EC = DVDStep();
}

/* 0x813020EC */
s32 Disc_GetState(void)
{
    return lbl_814AD2EC;
}

/* 0x813020F4 */
void fn_813020F4(void)
{
    lbl_814AD2E8 = 0xFF;
}

/* 0x81302100 */
void Disc_FadeOutStart(void)
{
    if (lbl_814AD2E4 == 0) {
        lbl_814AD2E8 -= 6;
        if (lbl_814AD2E8 < 0) {
            lbl_814AD2E8 = 0;
        }
    }
}

/* 0x81302130 */
u8 Disc_GetFadeAlpha(void)
{
    return lbl_814AD2E8;
}

static inline void BootGame(void)
{
    do {
        if (!lbl_814AD2F4) {
            fn_8135B180();
            fn_8135B200();
            lbl_814AD2F4 = TRUE;
        }
        if (!lbl_814AD2F8 && fn_8135B2C0()) {
            LCEnable();
            lbl_814AD2F8 = TRUE;
        }
    } while (!lbl_814AD2F8);
    GXDrawDone();
    VISetBlack(TRUE);
    VIFlush();
    VIWaitForRetrace();
    while (!__OSSyncSram()) {
    }
    GXFlush();
    GXAbortFrame();
    if (lbl_814AD2EC == 0x10) {
        fn_81300830();
    }
}

/* 0x8130213C */
void fn_8130213C(void)
{
    lbl_814AD2E8 += 12;
    if (lbl_814AD2E8 > 0xFF) {
        lbl_814AD2E8 = 0xFF;
        BootGame();
    }
}

/* 0x81302200 */
void Disc_StartGame(void)
{
    lbl_814AD2E8 += 3;
    if (lbl_814AD2E8 > 0xFF) {
        lbl_814AD2E8 = 0xFF;
        BootGame();
    }
}

/* 0x813022C4 */
void fn_813022C4(void)
{
    s16 alpha = lbl_814AD2E8;

    if (alpha > 0) {
        Screen_Dim(alpha);
    }
}

/* 0x813022F4 */
void fn_813022F4(void)
{
    s32 state;

    do {
        lbl_814AD2EC = DVDStep();
        VIWaitForRetrace();
        state = lbl_814AD2EC;
    } while (state != 0x13 && !(state >= 5 && state <= 0x10) && state != 0x12 && state != 0x14 &&
             state != 0x15 && state != 0x16 && state != 0x17 && state != 0x18);
}

static inline s32 GetDisplayOffsetH(void)
{
    OSSram *sram = __OSLockSram();
    s8 h = sram->displayOffsetH;
    s32 offsetH = h;

    __OSUnlockSram(FALSE);
    return offsetH;
}

static inline BOOL PadConnected(PADStatus *pad)
{
    s8 err = pad->err;
    BOOL connected = err != PAD_ERR_NO_CONTROLLER;

    return connected;
}

static inline void RecalibratePads(PADStatus *pad)
{
    u32 mask;

    PADRead(pad);
    mask = 0;
    if (PadConnected(&pad[0])) {
        mask |= PAD_CHAN0_BIT;
    }
    if (PadConnected(&pad[1])) {
        mask |= PAD_CHAN1_BIT;
    }
    if (PadConnected(&pad[2])) {
        mask |= PAD_CHAN2_BIT;
    }
    if (PadConnected(&pad[3])) {
        mask |= PAD_CHAN3_BIT;
    }
    PADRecalibrate(mask);
}

/* 0x81302368 */
BOOL fn_81302368(void)
{
    PADStatus pad[PAD_MAX_CONTROLLERS];
    Scene *scene = Scene_Get();
    BOOL reset = OSGetResetSwitchState();

    if (lbl_814AD2E4) {
        if (CardThread_IsEnabled() != TRUE) {
            lbl_814AD2E8 += 8;
            if (lbl_814AD2E8 > 0xFF) {
                s32 i;

                lbl_814AD2E4 = FALSE;
                lbl_814AD2E8 = 0;
                fn_8135B180();
                while (!fn_8135B2C0()) {
                }
                for (i = 0; i < 10; i++) {
                    VIWaitForRetrace();
                }
                __VISetAdjustingValues(GetDisplayOffsetH(), 0);
                fn_81301F0C();
                CardThread_Enable();
                RecalibratePads(pad);
                fn_81300668();
                fn_813022F4();
                Scene_Init(FALSE);
                goto restarted;
            }
        }
    } else if (lbl_814AD2FC && !reset && *(s16 *)((u8 *)scene + 0xB0) == 0) {
        lbl_814AD2E4 = TRUE;
        if (CardThread_IsEnabled() == TRUE) {
            CardThread_UnmountAll();
        }
    }
    lbl_814AD2FC = reset;
    return TRUE;

restarted:
    lbl_814AD2FC = FALSE;
    return TRUE;
}

/* 0x813025A4 */
void fn_813025A4(void)
{
    lbl_814AD2F0 = HeapAlloc(0x2000);
    memset(lbl_814AD2F0, 0, 0x2000);
    fn_813006D4(lbl_814AD2F0, 0x2000);
}

/* 0x813025E8 */
BOOL Disc_HasBanner(void)
{
    if (lbl_814AD2EC == 0x10 && fn_813006E0()) {
        u32 magic = *(u32 *)lbl_814AD2F0;
        if (magic == 'BNR1') {
            return TRUE;
        }
        if (magic == 'BNR2') {
            return TRUE;
        }
    }
    return FALSE;
}

/* 0x8130264C */
void *Disc_GetBanner(void)
{
    return lbl_814AD2F0;
}

/* 0x81302654 */
s32 Disc_GetBannerType(void)
{
    u32 magic = *(u32 *)lbl_814AD2F0;

    if (magic == 'BNR1') {
        return 0;
    }
    if (magic == 'BNR2') {
        return 1;
    }
    return 2;
}
