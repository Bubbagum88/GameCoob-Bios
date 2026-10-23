// Partly matching.
//
// Mosaic text ("fx", PAL 1.0 BS2, 0x81329B34..0x81330F30): the titles and
// items of the Game Play, Options and Calendar submenus are drawn with
// small cubes. Every cube flies along a Hermite curve from a keyframe
// table (res 37, see menu_fx_shape.c) to its place in a glyph shape
// (res 179..184); this unit owns the cube pool and lays out the text of
// each submenu. The glyph strings live in menu_fx_anim (0x81330F30..), the
// per-screen cube effects in menu_fx_draw (0x81333030..).

#include "bs2/menu.h"
#include "string.h"
#include "dolphin/os.h"
#include "bs2/menu_fx.h"

/* Per-cube animated values: v[2k] eases one step per frame toward v[2k+1]. */
typedef struct FxPool {
    s16 v[10][FX_MAX];
    s16 frame;      /* 0xA794 frames since the screen opened */
    s16 ready;      /* 0xA796 frame count at which input is accepted */
} FxPool;

/* A value easing from 'from' to 'to' over 10 frames. */
typedef struct FxSlide {
    s16 t;
    f32 from;
    f32 to;
} FxSlide;

typedef struct FxCursor {
    s16 v[3];       /* 0x00 */
    f32 f[3];       /* 0x08 */
    u8 unk14[0x280];
} FxCursor;         /* 0x294 */

/* Which screen the mosaic shows and its sub-state (copied every frame). */
typedef struct FxState {
    s16 screen;     /* 0x00 1 Options, 2 Calendar, 3 Game Play */
    s16 v[13];
} FxState;

void fn_81330F30(void* keys); /* takes no argument; the caller passes one anyway */

void FxOption_SetState(s16 sub, s16 row, s16 offset, s16 sound);
void FxOption_ChangeLang(s32 from, s32 to);
void FxOption_ChangeUiLang(s32 from, s32 to);
void fn_8132A690(s16* out, s16 field, s16* e);
void fn_8132A75C(void);
void fn_8132ADCC(void);
void fn_8132B114(void);
void fn_8132B678(void);
void fn_8132BD00(void);
void fn_8132BF20(void);
void fn_8132BFF8(s16* v, s16 target);
void fn_8132C024(void);
void fn_8132C2FC(void);
void fn_8132C5E0(void);
void fn_8132C660(s16* digits, s32 value);
f32 fn_8132CB88(u32 i);
f32 fn_8132CC00(u32 i);
f32 fn_8132CC78(u32 i);
f32 fn_8132CCF4(void);
f32 fn_8132CD5C(u32 i);
void fn_8132CE3C(s32 i, s32 on);
void fn_8132CF14(s32 i, s32 on);
void fn_8132EEB0(s32 i, s32 on);
void fn_8132CEA4(s32 i, s32 on);
void fn_8132CDD4(s32 i, s32 on);
void fn_8132CF7C(s32 i, s32 on);
void fn_8132D000(void);
void fn_8132D084(s16 starting, s16 discMsg);
void fn_8132D194(s16 sub, s16 row, s16 dateField, s16 timeField, ClockEdit* e);
void fn_8132D234(void);
f32 fn_8132DE98(void);
void fn_8132DEFC(void);
void fn_8132EC54(void);
void fn_8132F010(void);
void fn_8132FFF0(void);
void fn_81330374(FxCube* c, u32* n, f32 t);
void fn_813306C8(s32 row);
void fn_813306FC(s32 from, s32 to);
void fn_813307B4(void);
void fn_81330B7C(void);
void fn_81330518(GXColor* a, GXColor* b, GXColor* c, GXColor* out, f32 t, f32 u);

void Fx_EmitSparks(FxCube* cubes);
u16* fn_81330F88(s32 lang);
u16 fn_81330FAC(s32 lang);
u16* fn_81330FC8(s32 lang);
u16 fn_81330FEC(s32 lang);
u16* fn_813310AC(void);
u16 fn_81331114(void);
u16* fn_81331150(void);
u16 fn_813311B8(void);
u16* fn_813329A4(void);
u16 fn_81332A0C(void);
u16* fn_81332A48(void);
u16 fn_81332AB0(void);
u16* fn_81332AEC(void);
u16 fn_81332B54(void);
u16* fn_81332B90(void);
u16 fn_81332BF8(void);
u16* fn_81332008(void);
u16 fn_81332070(void);
FxCube* fn_8133491C(void);
u16* fn_8133166C(void);
u16 fn_813316D4(void);
u16* fn_81331858(void);
u16 fn_813318C0(void);
u16* fn_81331A44(void);
u16 fn_81331AAC(void);
u16* fn_81331C30(void);
u16 fn_81331C98(void);
u16* fn_81331E1C(void);
u16 fn_81331E84(void);
u16* fn_81331710(void);
u16 fn_81331778(void);
u16* fn_813317B4(void);
u16 fn_8133181C(void);
u16* fn_813318FC(void);
u16 fn_81331964(void);
u16* fn_813319A0(void);
u16 fn_81331A08(void);
u16* fn_81331AE8(void);
u16 fn_81331B50(void);
u16* fn_81331B8C(void);
u16 fn_81331BF4(void);
u16* fn_81331CD4(void);
u16 fn_81331D3C(void);
u16* fn_81331D78(void);
u16 fn_81331DE0(void);
u16* fn_81331EC0(void);
u16 fn_81331F28(void);
u16* fn_81331F64(void);
u16 fn_81331FCC(void);
u16* fn_813320AC(void);
u16 fn_81332114(void);
u16* fn_81332150(void);
u16 fn_813321B8(void);
u16* fn_813321F4(void);
u16 fn_8133225C(void);
u16* fn_81332298(void);
u16 fn_81332300(void);
u16* fn_813311F4(void);
u16 fn_81331258(void);
u16* fn_81331294(void);
u16 fn_813312FC(void);
u16* fn_813315C8(void);
u16 fn_81331630(void);
u16* fn_81331008(void);
u16 fn_81331070(void);
u16* fn_813327B8(void);
u16 fn_81332820(void);
u16* fn_8133285C(void);
u16 fn_813328C4(void);
u16* fn_81332900(void);
u16 fn_81332968(void);
u16* fn_81332C34(void);
u16* fn_813325CC(void);
u16 fn_81332634(void);
u16* fn_81332670(void);
u16 fn_813326D8(void);
u16* fn_81332714(void);
u16 fn_8133277C(void);
u16* fn_81332CD8(void);
u16 fn_81332D40(void);
u16* fn_81332D7C(s16 digit);
u16 fn_81332C9C(void);
void fn_81333344(void);
void fn_81333384(void);
void* fn_813334AC(void);
s32 fn_813335AC(void);
u8 fn_81333614(void);
void fn_8133370C(void);
void fn_8133374C(FxCube* cubes, u32 count);
void fn_81333C7C(FxCube* cubes);
s32 fn_81334C08(void);
void* fn_81334C10(void);
void* fn_81334C1C(void);
void fn_81334C28(void);
void fn_81334C60(FxCube* cubes);
void fn_813355E0(FxCube* cubes);
s32 fn_81336968(void);
void* fn_81336970(void);
void* fn_8133697C(void);
void fn_81336988(void);
void fn_813369B0(FxCube* cubes);
void fn_81336F9C(FxCube* cubes);

static FxPool sFxPool;
static FxState sFxState;
static FxState sFxPrev;
static FxCube sFxCube[FX_MAX];
static u8 sFxAlpha[FX_MAX];
static GXColor sFxLightCol[FX_MAX];
static GXColor sFxDarkCol[FX_MAX];
static FxCursor sFxCursor;
static FxSlide sFxSlide;
static u8 sFxPad[0x1C];

/* Per-cell fly-out delay (0xFF = first to go). */
static u8 sFxFlyDelay[0x2C8] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF1,
    0xED, 0xE9, 0xE6, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCB, 0xC5, 0xC1, 0xBC,
    0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8A, 0x84, 0x7E, 0x79, 0x73,
    0x68, 0x63, 0x53, 0x48, 0x44, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF1, 0xED, 0xE9, 0xE5, 0xE1, 0xDD, 0xD9,
    0xD4, 0xCF, 0xCA, 0xC5, 0xC0, 0xBB, 0xB5, 0xB0, 0xAB, 0xA6, 0xA0, 0x9A,
    0x95, 0x8F, 0x89, 0x84, 0x7E, 0x79, 0x62, 0x5D, 0x4E, 0x43, 0x3F, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF0, 0xED,
    0xEA, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5, 0xC1, 0xBB, 0xB6,
    0xB0, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8F, 0x89, 0x84, 0x7F, 0x79, 0x73,
    0x5D, 0x58, 0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7,
    0xF4, 0xF0, 0xED, 0xEA, 0xE5, 0xE2, 0xDE, 0xD8, 0xD4, 0xCF, 0xCB, 0xC6,
    0xC1, 0xBB, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9B, 0x95, 0x8F, 0x8A, 0x84,
    0x7E, 0x78, 0x73, 0x6D, 0x68, 0x62, 0x58, 0x52, 0x44, 0x3A, 0x36, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF1, 0xED, 0xE9,
    0xE6, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5, 0xC1, 0xBB, 0xB6, 0xB0,
    0xAB, 0xA5, 0xA0, 0x9B, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6E,
    0x68, 0x53, 0x4E, 0x44, 0x3F, 0x36, 0x32, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC,
    0xFA, 0xF7, 0xF4, 0xF0, 0xED, 0xEA, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF,
    0xCA, 0xC5, 0xC1, 0xBC, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8F,
    0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6D, 0x68, 0x62, 0x4E, 0x49, 0x44, 0x3B,
    0x32, 0x2E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF1, 0xED,
    0xEA, 0xE6, 0xE1, 0xDD, 0xD8, 0xD4, 0xCF, 0xCB, 0xC6, 0xC1, 0xBB, 0xB5,
    0xB0, 0xAB, 0xA5, 0xA0, 0x9A, 0x95, 0x8F, 0x89, 0x84, 0x7E, 0x78, 0x73,
    0x6E, 0x68, 0x62, 0x5E, 0x58, 0x53, 0x49, 0x44, 0x36, 0x2E, 0x2A, 0xD9,
    0xD4, 0xCF, 0xCB, 0xC5, 0xB0, 0xAB, 0xA5, 0xA0, 0x9A, 0x95, 0x8F, 0x8A,
    0x84, 0x7F, 0xCA, 0xC6, 0xC0, 0xBB, 0xA6, 0xA0, 0x9A, 0x95, 0x90, 0x8A,
    0x84, 0x7E, 0x78, 0x73, 0x6E, 0x58, 0x52, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFC, 0xFA, 0xF7, 0xF4, 0xED, 0xE9, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF,
    0xCA, 0xC6, 0xC0, 0xBB, 0xB6, 0xB0, 0xAB, 0xA5, 0xA0, 0x9B, 0x95, 0x8F,
    0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6E, 0x68, 0x62, 0x5D, 0x58, 0x52, 0x4E,
    0x48, 0x44, 0x3F, 0x3A, 0x36, 0x32, 0x2E, 0x29, 0x26, 0x22, 0x1F, 0x1C,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF1, 0xEA, 0xE5, 0xE1,
    0xDD, 0xD8, 0xD4, 0xCF, 0xCA, 0xC5, 0xC1, 0xBB, 0xB6, 0xB0, 0xAB, 0xA6,
    0xA0, 0x9A, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x78, 0x73, 0x6D, 0x68, 0x62,
    0x5D, 0x58, 0x53, 0x4E, 0x48, 0x44, 0x3F, 0x3B, 0x36, 0x32, 0x2E, 0x2A,
    0x26, 0x23, 0x1F, 0x1C, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF0,
    0xED, 0xE5, 0xE1, 0xDD, 0xD8, 0xD4, 0xCF, 0xCA, 0xC6, 0xC0, 0xBB, 0xB6,
    0xB0, 0xAB, 0xA5, 0xA0, 0x9A, 0x95, 0x8F, 0x8A, 0x84, 0x7F, 0x79, 0x73,
    0x6D, 0x68, 0x62, 0x5D, 0x58, 0x53, 0x4E, 0x49, 0x43, 0x3F, 0x3A, 0x36,
    0x32, 0x2E, 0x29, 0x26, 0x22, 0x1F, 0x1C, 0x19, 0xFF, 0xFF, 0xFC, 0xFA,
    0xF7, 0xF4, 0xF0, 0xED, 0xE9, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA,
    0xC5, 0xC1, 0xBB, 0xB6, 0xB1, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8F, 0x8A,
    0x84, 0x7E, 0x79, 0x73, 0x6E, 0x67, 0x63, 0x5D, 0x58, 0x52, 0x4D, 0x49,
    0x43, 0x3F, 0x3B, 0x36, 0x32, 0x2E, 0x2A, 0x26, 0x22, 0x1F, 0x1C, 0x19,
    0x16, 0x14, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF0, 0xED, 0xE6,
    0xE1, 0xDD, 0xD9, 0xD4, 0xD0, 0xCA, 0xC5, 0xC0, 0xBB, 0xB5, 0xB0, 0xAB,
    0xA5, 0xA0, 0x9B, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6D, 0x68,
    0x63, 0x5D, 0x58, 0x52, 0x4D, 0x49, 0x44, 0x3F, 0x3B, 0x36, 0x32, 0x2E,
    0x2A, 0x26, 0x23, 0x1F, 0x1C, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA,
    0xF7, 0xF4, 0xF1, 0xEA, 0xE6, 0xE2, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5,
    0xC0, 0xBB, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9B, 0x94, 0x8F, 0x8A, 0x84,
    0x7E, 0x78, 0x73, 0x6D, 0x68, 0x63, 0x5D, 0x58, 0x53, 0x4D, 0x49, 0x44,
    0x3F, 0x3B, 0x36, 0x32, 0x2D, 0x29, 0x26, 0x22, 0x1F, 0x1C, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF8, 0xF4, 0xF1, 0xED, 0xE9, 0xE5, 0xE1,
    0xDD, 0xD9, 0xD4, 0xD0, 0xCA, 0xC6, 0xC0, 0xBB, 0xB6, 0xB0, 0xAB, 0xA5,
    0xA0, 0x9A, 0x95, 0x8F, 0x89, 0x84, 0x7F, 0x79, 0x73, 0x6D, 0x68, 0x62,
    0x5D, 0x58, 0x52, 0x4D, 0x49, 0x44, 0x3F, 0x3A, 0x36, 0x32, 0x2D, 0x2A,
    0x26, 0x22, 0x1F, 0x1C, 0xED, 0xE9, 0xE5, 0xE1, 0xA0, 0x9B, 0xED, 0xE2,
    0x00, 0x00, 0x00, 0x00,
};

static u16 sFxBlink;
static s32 sFxRand;
static GXColor sFxLight0;
static GXColor sFxDark0;
static GXColor sFxLight1;
static GXColor sFxDark1;
static GXColor sFxLight2;
static GXColor sFxDark2;
static u32 sFxCount;
static s32 sFxLang;
static s32 sFxUiLang;

/* 0x81329B34 */
FxCube* Fx_GetCubes(void) {
    return sFxCube;
}

/* 0x81329B40 */
u8* Fx_GetDrawBuf(void) {
    return sFxAlpha;
}

/* 0x81329B4C */
u32 Fx_GetCount(void) {
    return sFxCount;
}

/* 0x81329B54 */
void* fn_81329B54(void) {
    return sFxLightCol;
}

/* 0x81329B60 */
void* fn_81329B60(void) {
    return sFxDarkCol;
}

/* 0x81329B6C */
u32 Fx_IsReady(s32 margin) {
    if (sFxPool.frame < (u32)(Menu_Get()->fxReadyCount - margin)) {
        return FALSE;
    }
    return TRUE;
}

/* 0x81329BC4 */
void Fx_Init(void* keys, void* curves) {
    memset(&sFxState, 0, sizeof(FxState));
    memset(&sFxPool, 0, sizeof(FxPool));
    memset(&sFxSlide, 0, sizeof(FxSlide));
    sFxState.screen = 0;
    sFxPrev = sFxState;
    fn_81330F30(keys);
    Fx_SetShapes(curves);
}

/* 0x81329C98 */
void Fx_Clear(void) {
    fn_8132D000();
    memset(&sFxPool, 0, sizeof(FxPool));
    memset(&sFxSlide, 0, sizeof(FxSlide));
}

#define FX_SET_COLORS(c)                                                \
    sFxLight0.r = Menu_Get()->c[0];                                     \
    sFxLight0.g = Menu_Get()->c[2];                                     \
    sFxLight0.b = Menu_Get()->c[4];                                     \
    sFxLight0.a = 0xFF;                                                 \
    sFxDark0.r = Menu_Get()->c[1];                                      \
    sFxDark0.g = Menu_Get()->c[3];                                      \
    sFxDark0.b = Menu_Get()->c[5];                                      \
    sFxDark0.a = 0xFF;                                                  \
    sFxLight1.r = Menu_Get()->c[6];                                     \
    sFxLight1.g = Menu_Get()->c[8];                                     \
    sFxLight1.b = Menu_Get()->c[10];                                    \
    sFxLight1.a = 0xFF;                                                 \
    sFxDark1.r = Menu_Get()->c[7];                                      \
    sFxDark1.g = Menu_Get()->c[9];                                      \
    sFxDark1.b = Menu_Get()->c[11];                                     \
    sFxDark1.a = 0xFF;                                                  \
    sFxLight2.r = Menu_Get()->c[12];                                    \
    sFxLight2.g = Menu_Get()->c[14];                                    \
    sFxLight2.b = Menu_Get()->c[16];                                    \
    sFxLight2.a = 0xFF;                                                 \
    sFxDark2.r = Menu_Get()->c[13];                                     \
    sFxDark2.g = Menu_Get()->c[15];                                     \
    sFxDark2.b = Menu_Get()->c[17];                                     \
    sFxDark2.a = 0xFF;

/* Append the cubes of one effect (count, cube and alpha arrays) to the pool. */
#define FX_APPEND(count, cubes, alpha)                                  \
    memcpy(&sFxCube[sFxCount], cubes(), count() * sizeof(FxCube));      \
    memcpy(&sFxAlpha[sFxCount], alpha(), count());                      \
    sFxCount += count();

/* 0x81329CE4 */
void FxOption_Update(s32 sub, s16 row, s16 offset, s16 sound) {
    u8* a;
    u32 i;

    if (sFxLang != Option_GetLang()) {
        FxOption_ChangeLang(sFxLang, Option_GetLang());
    }
    sFxLang = Option_GetLang();
    if (sFxUiLang != Lang_Get()) {
        FxOption_ChangeUiLang(sFxUiLang, Lang_Get());
    }
    sFxUiLang = Lang_Get();
    FxOption_SetState(sub, row, offset, sound);
    if (sFxPrev.screen != 1) {
        fn_81333344();
        fn_8133370C();
        memset(&sFxPool, 0, sizeof(FxPool));
        memset(&sFxSlide, 0, sizeof(FxSlide));
        sFxPool.ready = Menu_Get()->fxReadyCount;
        sFxBlink = 0;
        sFxLang = Option_GetLang();
        FxOption_ChangeLang(sFxLang, sFxLang);
        fn_813306C8(Option_GetLang());
    }
    FX_SET_COLORS(optionColor)
    fn_8132FFF0();
    fn_8132ADCC();
    fn_8132A75C();
    fn_8132C024();
    fn_8132C2FC();
    Fx_EvalShape(2, sFxCube, &sFxCount, fn_8132CCF4());
    fn_8132F010();
    fn_8132D234();
    if (fn_8132CCF4() > Menu_Get()->fxReadyRatio) {
        Fx_EmitSparks(sFxCube);
    }
    fn_8132BD00();
    fn_81333384();
    memcpy(&sFxCube[sFxCount], fn_813334AC(), fn_813335AC() * sizeof(FxCube));
    for (i = sFxCount, a = &sFxAlpha[sFxCount]; i < sFxCount + fn_813335AC(); i++) {
        *a++ = fn_81333614();
    }
    sFxCount += fn_813335AC();
    fn_81334C60(sFxCube);
    FX_APPEND(fn_81334C08, fn_81334C1C, fn_81334C10)
    if (sFxState.v[2] == 2 && sFxPrev.v[2] != 2) {
        fn_81334C28();
    }
    fn_813369B0(sFxCube);
    FX_APPEND(fn_81336968, fn_8133697C, fn_81336970)
    if (sFxState.v[2] == 1 && sFxPrev.v[2] != 1) {
        fn_81336988();
    }
    if (sFxState.v[2] == 3) {
        fn_813307B4();
    }
    fn_8132BF20();
    sFxBlink++;
    if (sFxBlink >= Menu_Get()->fxBlinkPeriod) {
        sFxBlink = 0;
    }
}

/* 0x8132A14C */
void FxClock_Update(s32 sub, s32 row, s32 dateField, s32 timeField, ClockEdit* e) {
    fn_8132D194(sub, row, dateField, timeField, e);
    if (sFxPrev.screen != 2) {
        memset(&sFxPool, 0, sizeof(FxPool));
        memset(&sFxSlide, 0, sizeof(FxSlide));
        sFxPool.ready = Menu_Get()->fxReadyCount;
        fn_8133370C();
    }
    FX_SET_COLORS(clockColor)
    fn_8132FFF0();
    fn_8132B114();
    fn_8132B678();
    Fx_EvalShape(0, sFxCube, &sFxCount, fn_8132CCF4());
    fn_8132DEFC();
    if (fn_8132CCF4() > Menu_Get()->fxReadyRatio) {
        fn_81333C7C(sFxCube);
    }
    fn_8132BD00();
    fn_813355E0(sFxCube);
    FX_APPEND(fn_81334C08, fn_81334C1C, fn_81334C10)
    if (sFxState.v[6] != 0 && sFxPrev.v[6] == 0) {
        fn_81334C28();
    }
    fn_8132BF20();
}

/* 0x8132A3D4 */
void FxGame_Update(s32 starting, s32 discMsg) {
    u8* a;
    s32 i;

    fn_8132D084(starting, discMsg);
    if (sFxPrev.screen != 3) {
        memset(&sFxPool, 0, sizeof(FxPool));
        memset(&sFxSlide, 0, sizeof(FxSlide));
        i = 0;
        a = sFxAlpha;
        do {
            *a = 0xFF;
            fn_8132CEA4(i, 1);
            fn_8132CE3C(i, 1);
            fn_8132CF7C(i, 1);
            i++;
            a++;
        } while (i < FX_MAX);
        fn_8133370C();
        sFxPool.ready = Menu_Get()->fxReadyCount;
    }
    sFxLight0.r = sFxLight1.r = sFxLight2.r = Menu_Get()->gameColor[0];
    sFxLight0.g = sFxLight1.g = sFxLight2.g = Menu_Get()->gameColor[2];
    sFxLight0.b = sFxLight1.b = sFxLight2.b = Menu_Get()->gameColor[4];
    sFxLight0.a = 0xFF;
    sFxDark0.r = sFxDark1.r = sFxDark2.r = Menu_Get()->gameColor[1];
    sFxDark0.g = sFxDark1.g = sFxDark2.g = Menu_Get()->gameColor[3];
    sFxDark0.b = sFxDark1.b = sFxDark2.b = Menu_Get()->gameColor[5];
    sFxDark0.a = 0xFF;
    if (Menu_Get()->fxCountOverride >= 0) {
        sFxPool.frame = Menu_Get()->fxCountOverride;
    }
    if (sFxState.v[0] == 0) {
        Fx_EvalShape(1, sFxCube, &sFxCount, fn_8132CCF4());
        fn_8132EC54();
    } else {
        if (sFxPrev.v[0] == 0) {
            sFxPool.frame = 0;
            sFxPool.ready = Menu_Get()->fxReadyCount;
            sFxRand = Rand() % 5;
        }
        fn_81330374(sFxCube, &sFxCount, fn_8132CCF4());
        fn_8132EC54();
    }
    fn_8132FFF0();
    fn_8132C5E0();
    if (fn_8132CCF4() > Menu_Get()->fxReadyRatio) {
        fn_8133374C(sFxCube, sFxCount);
    }
    fn_8132BD00();
    fn_81336F9C(sFxCube);
    FX_APPEND(fn_81336968, fn_8133697C, fn_81336970)
    fn_8132BF20();
}

/* 0x8132A690: current date and time, with the field being edited taken from e. */
void fn_8132A690(s16* out, s16 field, s16* e) {
    OSCalendarTime t;
    s32 year, mon, mday, hour, min, sec;

    OSTicksToCalendarTime(OSGetTime(), &t);
    year = t.year;
    mon = t.mon;
    mday = t.mday;
    hour = t.hour;
    min = t.min;
    sec = t.sec;
    if (field == 0) {
    } else if (field == 1) {
        year = e[0];
        mon = e[1];
        mday = e[2];
    } else if (field == 2) {
        hour = e[0];
        min = e[1];
        sec = e[2];
    }
    out[0] = year;
    out[1] = mon + 1;
    out[2] = mday;
    out[3] = hour;
    out[4] = min;
    out[5] = sec;
}

/* Apply f(cell, on) to every cell of a glyph string. */
#define FX_CELLS(f, list, n, on)                                        \
    for (i = 0; i < n; i++) {                                           \
        f(list[i], on);                                                 \
    }

/* 0x8132A75C: show or hide the Options title, item and cursor glyphs. */
void fn_8132A75C(void) {
    s32 i;
    u16* item = fn_813327B8();
    u16 nItem = fn_81332820();
    u16* title = fn_81332C34();
    u16 nTitle = fn_81332C9C();
    u16* sub0 = fn_8133285C();
    u16* sub1 = fn_81332900();
    u16 nSub0 = fn_813328C4();
    u16 nSub1 = fn_81332968();
    u16* lang = fn_81331008();
    u16 nLang = fn_81331070();

    switch (sFxState.v[3]) {
    case 0:
        FX_CELLS(fn_8132CF14, title, nTitle, 1)
        break;
    case 1:
        FX_CELLS(fn_8132CF14, item, nItem, 1)
        break;
    case 2:
        FX_CELLS(fn_8132CF14, lang, nLang, 1)
        break;
    }
    switch (sFxState.v[2]) {
    case 0:
        FX_CELLS(fn_8132CE3C, title, nTitle, 1)
        FX_CELLS(fn_8132CE3C, item, nItem, 1)
        FX_CELLS(fn_8132CE3C, lang, nLang, 1)
        fn_8132EEB0(0, 0);
        fn_8132EEB0(1, 0);
        fn_8132EEB0(2, 0);
        fn_8132EEB0(3, 0);
        fn_8132EEB0(4, 0);
        fn_8132EEB0(5, 0);
        break;
    case 1:
        FX_CELLS(fn_8132CE3C, title, nTitle, 1)
        FX_CELLS(fn_8132CE3C, item, nItem, 0)
        FX_CELLS(fn_8132CE3C, lang, nLang, 0)
        switch (sFxState.v[5]) {
        case 0:
            fn_8132EEB0(0, 0);
            fn_8132EEB0(1, 1);
            fn_8132EEB0(2, 0);
            fn_8132EEB0(3, 0);
            fn_8132EEB0(4, 0);
            fn_8132EEB0(5, 0);
            break;
        case 1:
            fn_8132EEB0(0, 1);
            fn_8132EEB0(1, 0);
            fn_8132EEB0(2, 0);
            fn_8132EEB0(3, 0);
            fn_8132EEB0(4, 0);
            fn_8132EEB0(5, 0);
            break;
        }
        if (sFxPrev.v[2] == 1) {
            FX_CELLS(fn_8132CF7C, title, nTitle, 0)
        }
        break;
    case 2:
        FX_CELLS(fn_8132CE3C, title, nTitle, 0)
        FX_CELLS(fn_8132CE3C, item, nItem, 1)
        FX_CELLS(fn_8132CE3C, lang, nLang, 0)
        if (sFxPrev.v[2] == 2) {
            FX_CELLS(fn_8132CF7C, sub0, nSub0, 0)
            FX_CELLS(fn_8132CF7C, sub1, nSub1, 0)
        }
        fn_8132EEB0(0, 0);
        fn_8132EEB0(1, 0);
        if (sFxState.v[4] <= -0x20) {
            fn_8132EEB0(2, 0);
        } else {
            fn_8132EEB0(2, 1);
        }
        if (sFxState.v[4] >= 0x20) {
            fn_8132EEB0(3, 0);
        } else {
            fn_8132EEB0(3, 1);
        }
        fn_8132EEB0(4, 0);
        fn_8132EEB0(5, 0);
        break;
    case 3:
        FX_CELLS(fn_8132CE3C, title, nTitle, 0)
        FX_CELLS(fn_8132CE3C, item, nItem, 0)
        FX_CELLS(fn_8132CE3C, lang, nLang, 1)
        fn_8132EEB0(0, 0);
        fn_8132EEB0(1, 0);
        fn_8132EEB0(2, 0);
        fn_8132EEB0(3, 0);
        if (Option_GetLang() == 0) {
            fn_8132EEB0(4, 0);
        } else {
            fn_8132EEB0(4, 1);
        }
        if (Option_GetLang() == 5) {
            fn_8132EEB0(5, 0);
        } else {
            fn_8132EEB0(5, 1);
        }
        break;
    }
}

/* 0x8132ADCC: glyphs of the sound / offset rows of the Options screen. */
void fn_8132ADCC(void) {
    s32 i;
    u16* list;
    u16* plus;
    u16* minus;
    u16 nPlus;
    u16 nMinus;
    u16* mono;
    u16 nMono;
    u16* stereo;
    u16 nStereo;
    u16* digit;
    s16 digits[4];
    u8 unused[8];

    list = fn_813327B8();
    FX_CELLS(fn_8132CF7C, list, fn_81332820(), 0)
    list = fn_81332C34();
    FX_CELLS(fn_8132CF7C, list, fn_81332C9C(), 0)
    plus = fn_81332670();
    minus = fn_813325CC();
    nPlus = fn_813326D8();
    nMinus = fn_81332634();
    mono = fn_81332714();
    nMono = fn_8133277C();
    stereo = fn_81332CD8();
    nStereo = fn_81332D40();
    FX_CELLS(fn_8132CF7C, fn_81331008(), fn_81331070(), 1)
    switch (sFxState.v[5]) {
    case 0:
        FX_CELLS(fn_8132CF7C, mono, nMono, 1)
        fn_8132EEB0(0, 0);
        fn_8132EEB0(1, 1);
        fn_8132EEB0(2, 0);
        fn_8132EEB0(3, 0);
        break;
    case 1:
        FX_CELLS(fn_8132CF7C, stereo, nStereo, 1)
        fn_8132EEB0(0, 1);
        fn_8132EEB0(1, 0);
        fn_8132EEB0(2, 0);
        fn_8132EEB0(3, 0);
        break;
    }
    if (sFxState.v[4] < 0) {
        FX_CELLS(fn_8132CF7C, minus, nMinus, 1)
    } else if (sFxState.v[4] > 0) {
        FX_CELLS(fn_8132CF7C, plus, nPlus, 1)
    }
    fn_8132C660(digits, (s16)__fabs((f64)sFxState.v[4]));
    list = fn_8133285C();
    digit = fn_81332D7C(digits[1]);
    for (i = 0; i < fn_813328C4(); i++) {
        fn_8132CF7C(list[digit[i]], 1);
    }
    list = fn_81332900();
    digit = fn_81332D7C(digits[0]);
    for (i = 0; i < fn_81332968(); i++) {
        fn_8132CF7C(list[digit[i]], 1);
    }
}

/* Draw the cells of one digit glyph: list holds the cells of every digit,
 * fn_81332D7C(d) the indices that make up digit d. */
#define FX_DIGIT(listFn, countFn, d)                                    \
    list = listFn();                                                    \
    digit = fn_81332D7C(d);                                             \
    for (i = 0; i < countFn(); i++) {                                   \
        fn_8132CF7C(list[digit[i]], 1);                                 \
    }

/* 0x8132B114: the Calendar date and time read-out. */
void fn_8132B114(void) {
    s32 i;
    u16* list;
    u16* digit;
    s16 dt[6];
    s16 digits[4];

    fn_8132A690(dt, sFxState.v[6], &sFxState.v[10]);
    list = fn_813315C8();
    FX_CELLS(fn_8132CF7C, list, fn_81331630(), 0)
    list = fn_813311F4();
    FX_CELLS(fn_8132CF7C, list, fn_81331258(), 0)
    fn_8132C660(digits, dt[0]);
    FX_DIGIT(fn_813320AC, fn_81332114, digits[3])
    FX_DIGIT(fn_81332150, fn_813321B8, digits[2])
    FX_DIGIT(fn_813321F4, fn_8133225C, digits[1])
    FX_DIGIT(fn_81332298, fn_81332300, digits[0])
    fn_8132C660(digits, dt[1]);
    FX_DIGIT(fn_81331CD4, fn_81331D3C, digits[1])
    FX_DIGIT(fn_81331D78, fn_81331DE0, digits[0])
    fn_8132C660(digits, dt[2]);
    FX_DIGIT(fn_81331710, fn_81331778, digits[1])
    FX_DIGIT(fn_813317B4, fn_8133181C, digits[0])
    fn_8132C660(digits, dt[3]);
    FX_DIGIT(fn_813318FC, fn_81331964, digits[1])
    FX_DIGIT(fn_813319A0, fn_81331A08, digits[0])
    fn_8132C660(digits, dt[4]);
    FX_DIGIT(fn_81331AE8, fn_81331B50, digits[1])
    FX_DIGIT(fn_81331B8C, fn_81331BF4, digits[0])
    fn_8132C660(digits, dt[5]);
    FX_DIGIT(fn_81331EC0, fn_81331F28, digits[1])
    FX_DIGIT(fn_81331F64, fn_81331FCC, digits[0])
    list = fn_81331294();
    FX_CELLS(fn_8132CF7C, list, fn_813312FC(), 1)
}

/* Cells of the field being edited: lit, shown, not blinking. */
#define FX_EDIT_CELLS(listFn, countFn, prevField)                       \
    list = listFn();                                                    \
    for (i = 0; i < countFn(); i++) {                                   \
        if (sFxPrev.v[6] == prevField) {                                \
            fn_8132CF7C(list[i], 0);                                    \
        }                                                               \
        fn_8132CE3C(list[i], 1);                                        \
        fn_8132CDD4(list[i], 1);                                        \
        fn_8132CEA4(list[i], 0);                                        \
    }

/* 0x8132B678: highlight and blink the Calendar fields. */
void fn_8132B678(void) {
    s32 i;
    u16* list;

    if (sFxState.v[7] != 0) {
        list = fn_813315C8();
        for (i = 0; i < fn_81331630(); i++) {
            fn_8132CDD4(list[i], 0);
            fn_8132CEA4(list[i], 0);
        }
        list = fn_813311F4();
        for (i = 0; i < fn_81331258(); i++) {
            fn_8132CDD4(list[i], 0);
            fn_8132CEA4(list[i], 1);
            fn_8132CF14(list[i], 1);
        }
    } else {
        list = fn_813315C8();
        for (i = 0; i < fn_81331630(); i++) {
            fn_8132CDD4(list[i], 0);
            fn_8132CEA4(list[i], 1);
            fn_8132CF14(list[i], 1);
        }
        list = fn_813311F4();
        for (i = 0; i < fn_81331258(); i++) {
            fn_8132CDD4(list[i], 0);
            fn_8132CEA4(list[i], 0);
        }
    }
    if (sFxState.v[6] == 1) {
        if (Menu_Get()->fx17E == 0) {
            list = fn_813315C8();
            FX_CELLS(fn_8132CEA4, list, fn_81331630(), 0)
        }
        switch (sFxState.v[8]) {
        case 0:
            list = fn_813320AC();
            for (i = 0; i < fn_81332114(); i++) {
                fn_8132CE3C(list[i], 1);
                fn_8132CDD4(list[i], 1);
                fn_8132CEA4(list[i], 0);
            }
            list = fn_81332150();
            for (i = 0; i < fn_813321B8(); i++) {
                fn_8132CE3C(list[i], 1);
                fn_8132CDD4(list[i], 1);
                fn_8132CEA4(list[i], 0);
            }
            FX_EDIT_CELLS(fn_813321F4, fn_8133225C, 1)
            FX_EDIT_CELLS(fn_81332298, fn_81332300, 1)
            break;
        case 1:
            FX_EDIT_CELLS(fn_81331C30, fn_81331C98, 1)
            break;
        case 2:
            FX_EDIT_CELLS(fn_8133166C, fn_813316D4, 1)
            break;
        }
    } else if (sFxState.v[6] == 2) {
        if (Menu_Get()->fx17E == 0) {
            list = fn_813311F4();
            FX_CELLS(fn_8132CEA4, list, fn_81331258(), 0)
        }
        switch (sFxState.v[9]) {
        case 0:
            FX_EDIT_CELLS(fn_81331858, fn_813318C0, 2)
            break;
        case 1:
            FX_EDIT_CELLS(fn_81331A44, fn_81331AAC, 2)
            break;
        case 2:
            FX_EDIT_CELLS(fn_81331E1C, fn_81331E84, 2)
            break;
        }
    } else {
        list = fn_813315C8();
        FX_CELLS(fn_8132CE3C, list, fn_81331630(), 1)
        list = fn_813311F4();
        FX_CELLS(fn_8132CE3C, list, fn_81331258(), 1)
    }
}

/* 0x8132BD00: fade, offset and tint every placed cube. */
void fn_8132BD00(void) {
    FxCube* ofs = fn_8133491C();
    f32 t = fn_8132CCF4();
    u32 i;

    for (i = 0; i < sFxCount; i++) {
        sFxAlpha[i] = 255.0f * (t * fn_8132CC00(i));
        sFxCube[i].pos.x += ofs[i].pos.x * fn_8132CC78(i);
        sFxCube[i].pos.y += ofs[i].pos.y * fn_8132CC78(i);
        sFxCube[i].pos.z += ofs[i].pos.z * fn_8132CC78(i);
        sFxCube[i].rot[0] = ofs[i].rot[0] * fn_8132CC78(i);
        sFxCube[i].rot[1] = ofs[i].rot[1] * fn_8132CC78(i);
        sFxCube[i].rot[2] = ofs[i].rot[2] * fn_8132CC78(i);
        fn_81330518(&sFxLight1, &sFxLight2, &sFxLight0, &sFxLightCol[i], fn_8132CD5C(i), fn_8132CB88(i));
        fn_81330518(&sFxDark1, &sFxDark2, &sFxDark0, &sFxDarkCol[i], fn_8132CD5C(i), fn_8132CB88(i));
    }
}

/* 0x8132BF20: step every animated value one frame toward its target. */
void fn_8132BF20(void) {
    u32 i;

    for (i = 0; i < sFxCount; i++) {
        fn_8132BFF8(&sFxPool.v[0][i], sFxPool.v[1][i]);
        fn_8132BFF8(&sFxPool.v[2][i], sFxPool.v[3][i]);
        fn_8132BFF8(&sFxPool.v[4][i], sFxPool.v[5][i]);
        fn_8132BFF8(&sFxPool.v[8][i], sFxPool.v[9][i]);
        fn_8132BFF8(&sFxPool.v[6][i], sFxPool.v[7][i]);
    }
    fn_8132BFF8(&sFxPool.frame, sFxPool.ready);
    fn_8132BFF8(&sFxSlide.t, 10);
    fn_8132BFF8(&sFxCursor.v[2], 10);
}

/* 0x8132BFF8 */
void fn_8132BFF8(s16* v, s16 target) {
    if (*v > target) {
        *v -= 1;
    } else if (*v < target) {
        *v += 1;
    }
}

/* 0x8132C024: light the Options row that has the cursor. */
void fn_8132C024(void) {
    s32 i;

    switch (sFxState.v[2]) {
    case 1:
        FX_CELLS(fn_8132CDD4, fn_81332C34(), fn_81332C9C(), 1)
        FX_CELLS(fn_8132CDD4, fn_813327B8(), fn_81332820(), 0)
        FX_CELLS(fn_8132CDD4, fn_81331008(), fn_81331070(), 0)
        break;
    case 2:
        FX_CELLS(fn_8132CDD4, fn_81332C34(), fn_81332C9C(), 0)
        FX_CELLS(fn_8132CDD4, fn_813327B8(), fn_81332820(), 1)
        FX_CELLS(fn_8132CDD4, fn_81331008(), fn_81331070(), 0)
        break;
    case 3:
        FX_CELLS(fn_8132CDD4, fn_81332C34(), fn_81332C9C(), 0)
        FX_CELLS(fn_8132CDD4, fn_813327B8(), fn_81332820(), 0)
        FX_CELLS(fn_8132CDD4, fn_81331008(), fn_81331070(), 1)
        break;
    default:
        FX_CELLS(fn_8132CDD4, fn_81332C34(), fn_81332C9C(), 0)
        FX_CELLS(fn_8132CDD4, fn_813327B8(), fn_81332820(), 0)
        FX_CELLS(fn_8132CDD4, fn_81331008(), fn_81331070(), 0)
        break;
    }
}

/* 0x8132C2FC: blink the Options row that has the cursor. */
void fn_8132C2FC(void) {
    s32 i;

    if (sFxState.v[2] == 0) {
        switch (sFxState.v[3]) {
        case 0:
            FX_CELLS(fn_8132CEA4, fn_81332C34(), fn_81332C9C(), 1)
            FX_CELLS(fn_8132CEA4, fn_813327B8(), fn_81332820(), 0)
            FX_CELLS(fn_8132CEA4, fn_81331008(), fn_81331070(), 0)
            break;
        case 1:
            FX_CELLS(fn_8132CEA4, fn_81332C34(), fn_81332C9C(), 0)
            FX_CELLS(fn_8132CEA4, fn_813327B8(), fn_81332820(), 1)
            FX_CELLS(fn_8132CEA4, fn_81331008(), fn_81331070(), 0)
            break;
        case 2:
            FX_CELLS(fn_8132CEA4, fn_81332C34(), fn_81332C9C(), 0)
            FX_CELLS(fn_8132CEA4, fn_813327B8(), fn_81332820(), 0)
            FX_CELLS(fn_8132CEA4, fn_81331008(), fn_81331070(), 1)
            break;
        }
    } else {
        FX_CELLS(fn_8132CEA4, fn_81332C34(), fn_81332C9C(), 0)
        FX_CELLS(fn_8132CEA4, fn_813327B8(), fn_81332820(), 0)
        FX_CELLS(fn_8132CEA4, fn_81331008(), fn_81331070(), 0)
    }
}

/* 0x8132C5E0: Game Play shows every cube, unlit and still. */
void fn_8132C5E0(void) {
    s32 i;

    i = 0;
    do {
        fn_8132CEA4(i, 1);
        fn_8132CE3C(i, 1);
        fn_8132CDD4(i, 0);
        fn_8132CF7C(i, 0);
        i++;
    } while (i < FX_MAX);
    i = 0;
    do {
        fn_8132CF7C(i, 0);
        i++;
    } while (i < FX_MAX);
}

/* 0x8132C660: split 0..9999 into decimal digits, ones first. */
void fn_8132C660(s16* digits, s32 value) {
    if (value < 0 || value > 9999) {
        digits[0] = 0;
        digits[1] = 0;
        digits[2] = 0;
        digits[3] = 0;
    }
    if (value > 999) {
        digits[3] = value / 1000;
        value -= digits[3] * 1000;
    } else {
        digits[3] = 0;
    }
    if (value > 99) {
        digits[2] = value / 100;
        value -= digits[2] * 100;
    } else {
        digits[2] = 0;
    }
    if (value > 9) {
        digits[1] = value / 10;
        value -= digits[1] * 10;
    } else {
        digits[1] = 0;
    }
    digits[0] = value;
}

/* Rotate a cube about (cx, cy, cz); out gets the offset. */
static inline void Fx_RotAbout(s16 rx, s16 ry, s16 rz, FxCube* in, FxCube* out, f32 cx, f32 cy, f32 cz) {
    Mtx m;
    Vec v;
    Vec r;

    MtxRot(rx, ry, rz, m);
    v.x = in->pos.x - cx;
    v.y = in->pos.y - cy;
    v.z = in->pos.z - cz;
    PSMTXMultVec(m, &v, &r);
    out->pos.x = (r.x + cx) - in->pos.x;
    out->pos.y = (r.y + cy) - in->pos.y;
    out->pos.z = (r.z + cz) - in->pos.z;
    out->rot[0] = rx;
    out->rot[1] = ry;
    out->rot[2] = rz;
}

/* 0x8132C750: out-of-line copy of Fx_RotAbout. */
void fn_8132C750(s16 rx, s16 ry, s16 rz, FxCube* in, FxCube* out, f32 cx, f32 cy, f32 cz) {
    Mtx m;
    Vec v;
    Vec r;

    MtxRot(rx, ry, rz, m);
    v.x = in->pos.x - cx;
    v.y = in->pos.y - cy;
    v.z = in->pos.z - cz;
    PSMTXMultVec(m, &v, &r);
    out->pos.x = (r.x + cx) - in->pos.x;
    out->pos.y = (r.y + cy) - in->pos.y;
    out->pos.z = (r.z + cz) - in->pos.z;
    out->rot[0] = rx;
    out->rot[1] = ry;
    out->rot[2] = rz;
}

/* 0x8132C830: centre of n cubes (all of them, or the ones idx names). */
void fn_8132C830(Vec* out, FxCube* cubes, u16* idx, u32 n) {
    f32 x, y, z;
    s32 i;

    x = y = z = 0.0f;
    if (idx != NULL) {
        for (i = 0; i < n; i++) {
            FxCube* c = &cubes[*idx++];

            x += c->pos.x;
            y += c->pos.y;
            z += c->pos.z;
        }
    } else {
        for (i = 0; i < n; i++) {
            x += cubes->pos.x;
            y += cubes->pos.y;
            z += cubes->pos.z;
            cubes++;
        }
    }
    {
        Vec c = { 0.0f, 0.0f, 0.0f };
        Vec *o = out;

        c.x = x / n;
        c.y = y / n;
        c.z = z / n;
        *o = c;
    }

}

/* 0x8132CB60: degrees to a 16-bit angle. */
s32 fn_8132CB60(f32 deg) {
    return 65535.0f * deg / 360.0f;
}

/* 0x8132CB88 */
f32 fn_8132CB88(u32 i) {
    return (f32)sFxPool.v[1][i] / (f32)Menu_Get()->fxFadeOut;
}

/* 0x8132CC00 */
f32 fn_8132CC00(u32 i) {
    return (f32)sFxPool.v[2][i] / (f32)Menu_Get()->fxFadeIn;
}

/* 0x8132CC78 */
f32 fn_8132CC78(u32 i) {
    return (f32)sFxPool.v[8][i] / (f32)Menu_Get()->fxFadeOut;
}

/* 0x8132CCF4: how far the screen has come in (0..1). */
f32 fn_8132CCF4(void) {
    return (f32)sFxPool.frame / (f32)Menu_Get()->fxReadyCount;
}

/* 0x8132CD5C */
f32 fn_8132CD5C(u32 i) {
    return (f32)sFxPool.v[6][i] / (f32)Menu_Get()->fxFadeIn;
}

/* 0x8132CDD4 */
void fn_8132CDD4(s32 i, s32 on) {
    if (on) {
        sFxPool.v[1][i] = Menu_Get()->fxFadeOut;
    } else {
        sFxPool.v[1][i] = 0;
    }
}

/* 0x8132CE3C */
void fn_8132CE3C(s32 i, s32 on) {
    if (on) {
        sFxPool.v[5][i] = Menu_Get()->fxFadeIn;
    } else {
        sFxPool.v[5][i] = 0;
    }
}

/* 0x8132CEA4 */
void fn_8132CEA4(s32 i, s32 on) {
    if (on) {
        sFxPool.v[9][i] = Menu_Get()->fxFadeOut;
    } else {
        sFxPool.v[9][i] = 0;
    }
}

/* 0x8132CF14 */
void fn_8132CF14(s32 i, s32 on) {
    if (on) {
        sFxPool.v[7][i] = Menu_Get()->fxFadeIn;
    } else {
        sFxPool.v[7][i] = 0;
    }
}

/* 0x8132CF7C */
void fn_8132CF7C(s32 i, s32 on) {
    if (on) {
        sFxPool.v[2][i] = sFxPool.v[3][i] = Menu_Get()->fxFadeIn;
    } else {
        sFxPool.v[2][i] = sFxPool.v[3][i] = 0;
    }
}

/* 0x8132CFE8 */
FxState* fn_8132CFE8(void) {
    return &sFxState;
}

/* 0x8132CFF4 */
FxState* fn_8132CFF4(void) {
    return &sFxPrev;
}

/* 0x8132D000: close the mosaic. */
void fn_8132D000(void) {
    sFxPrev = sFxState;
    sFxState.screen = 0;
    sFxState.v[1] = 0x7F;
    sFxState.v[2] = 0x7F;
    sFxState.v[3] = 0x7F;
    sFxState.v[4] = 0x7F;
    sFxState.v[5] = 0x7F;
    sFxState.v[6] = 0x7F;
    sFxState.v[7] = 0x7F;
    sFxState.v[8] = 0x7F;
    sFxState.v[9] = 0x7F;
    sFxState.v[10] = 0x7F;
    sFxState.v[11] = 0x7F;
    sFxState.v[12] = 0x7F;
}

/* 0x8132D084 */
void fn_8132D084(s16 starting, s16 discMsg) {
    sFxPrev = sFxState;
    sFxState.screen = 3;
    sFxState.v[0] = starting;
    sFxState.v[1] = discMsg;
    sFxState.v[2] = 0x7F;
    sFxState.v[3] = 0x7F;
    sFxState.v[4] = 0x7F;
    sFxState.v[5] = 0x7F;
    sFxState.v[6] = 0x7F;
    sFxState.v[7] = 0x7F;
    sFxState.v[8] = 0x7F;
    sFxState.v[9] = 0x7F;
    sFxState.v[10] = 0x7F;
    sFxState.v[11] = 0x7F;
    sFxState.v[12] = 0x7F;
}

/* 0x8132D10C */
void FxOption_SetState(s16 sub, s16 row, s16 offset, s16 sound) {
    sFxPrev = sFxState;
    sFxState.screen = 1;
    sFxState.v[0] = 0x7F;
    sFxState.v[1] = 0x7F;
    sFxState.v[2] = sub;
    sFxState.v[3] = row;
    sFxState.v[4] = offset;
    sFxState.v[5] = sound;
    sFxState.v[6] = 0x7F;
    sFxState.v[7] = 0x7F;
    sFxState.v[8] = 0x7F;
    sFxState.v[9] = 0x7F;
    sFxState.v[10] = 0x7F;
    sFxState.v[11] = 0x7F;
    sFxState.v[12] = 0x7F;
}

/* 0x8132D194 */
void fn_8132D194(s16 sub, s16 row, s16 dateField, s16 timeField, ClockEdit* e) {
    sFxPrev = sFxState;
    sFxState.screen = 2;
    sFxState.v[0] = 0x7F;
    sFxState.v[1] = 0x7F;
    sFxState.v[2] = 0x7F;
    sFxState.v[3] = 0x7F;
    sFxState.v[4] = 0x7F;
    sFxState.v[5] = 0x7F;
    sFxState.v[6] = sub;
    sFxState.v[7] = row;
    sFxState.v[8] = dateField;
    sFxState.v[9] = timeField;
    sFxState.v[10] = e->value[0];
    sFxState.v[11] = e->value[1];
    sFxState.v[12] = e->value[2];
}

/* fn_8132CCF4, inlined. */
static inline f32 Fx_Progress(void) {
    return (f32)sFxPool.frame / (f32)Menu_Get()->fxReadyCount;
}

static inline f32 Fx_Lit(u32 i) {
    return (f32)sFxPool.v[0][i] / (f32)Menu_Get()->fxFadeOut;
}

/* Scale a cube's offset from o and its size. */
static inline void Fx_ScaleAbout(Vec* o, f32 sx, f32 sy, f32 sz, FxCube* c) {
    f32 ox = o->x;
    f32 oz = o->z;
    f32 oy = o->y;

    c->pos.x = ox + sx * (c->pos.x - ox);
    c->pos.y = oy + sy * (c->pos.y - oy);
    c->pos.z = oz + sz * (c->pos.z - oz);
    c->scale.x *= sx;
    c->scale.y *= sy;
    c->scale.z *= sz;
}

/* Place a text line: slide it in from its offset and scale it about its centre. */
#define FX_PLACE_LINE(listFn, countFn, line)                                          \
    list = listFn();                                                                \
    fn_8132C830(&o, sFxCube, list, countFn());                                      \
    for (i = 0; i < countFn(); i++) {                                               \
        sFxCube[list[i]].pos.x += Menu_Get()->fxTitle[line].x * Fx_Progress();      \
        sFxCube[list[i]].pos.y += Menu_Get()->fxTitle[line].y * Fx_Progress();      \
        Fx_ScaleAbout(&o, Menu_Get()->fxTitle[line].scale, Menu_Get()->fxTitle[line].scale, \
                      Menu_Get()->fxTitle[line].scale, &sFxCube[list[i]]);          \
    }

/* Grow the lit cells of a text line. */
#define FX_GROW_LINE(listFn, countFn, line)                                           \
    list = listFn();                                                                \
    fn_8132C830(&o, sFxCube, list, countFn());                                      \
    for (i = 0; i < countFn(); i++) {                                               \
        Fx_ScaleAbout(&o,                                                           \
                      1.0f + Fx_Lit(list[i]) * Menu_Get()->fxTitle[line].spacing,   \
                      1.0f + Fx_Lit(list[i]) * Menu_Get()->fxTitle[line].spacing,   \
                      1.0f + Fx_Lit(list[i]) * Menu_Get()->fxTitle[line].spacing,   \
                      &sFxCube[list[i]]);                                           \
        LAST                                                                        \
    }

/* Out-of-line slide-in weight; nothing calls it, so the linker strips it,
   but its constants stay in .sdata2 ahead of fn_8132D234's. */
f32 Fx_SlideT(void) {
    f32 t = (Fx_Progress() - 0.9f) / (1.0f - 0.9f);

    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }
    return t;
}

static inline f32 FxSlideOff(void) {
    f32 t = (Fx_Progress() - 0.9f) / (1.0f - 0.9f);

    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }
    return t * (-271.0 + (271.0 / 296.0) * fn_8132DE98());
}

/* 0x8132D234: lay out the three Options text lines. */
void fn_8132D234(void) {
    s32 i;
    u16* list;
    Vec o;
    f32 t;

    o.x = o.y = o.z = 0.0f;
    FX_PLACE_LINE(fn_81332C34, fn_81332C9C, 0)
    FX_PLACE_LINE(fn_813327B8, fn_81332820, 1)
    list = fn_81331008();
    fn_8132C830(&o, sFxCube, list, fn_81331070());
    o.x = 5.0f;
    o.y = -135.0f;
    for (i = 0; i < fn_81331070(); i++) {
        sFxCube[list[i]].pos.x += Menu_Get()->fxTitle[2].x * Fx_Progress();
        sFxCube[list[i]].pos.y += Menu_Get()->fxTitle[2].y * Fx_Progress();
        Fx_ScaleAbout(&o, Menu_Get()->fxTitle[2].scale, Menu_Get()->fxTitle[2].scale,
                      Menu_Get()->fxTitle[2].scale, &sFxCube[list[i]]);
    }
#define LAST
    FX_GROW_LINE(fn_81332C34, fn_81332C9C, 0)
    FX_GROW_LINE(fn_813327B8, fn_81332820, 1)
#undef LAST
#define LAST                                                                        \
    t = FxSlideOff();                                                               \
    sFxCube[list[i]].pos.x += t;
    FX_GROW_LINE(fn_81331008, fn_81331070, 2)
#undef LAST
}

/* 0x8132DD84: slide the language list to the row of the new language. */
void FxOption_ChangeLang(s32 from, s32 to) {
    s32 ofs;

    switch (to) {
    case 0:
        ofs = Menu_Get()->fxLineOfs[0];
        break;
    case 1:
        ofs = Menu_Get()->fxLineOfs[1];
        break;
    case 2:
        ofs = Menu_Get()->fxLineOfs[2];
        break;
    case 3:
        ofs = Menu_Get()->fxLineOfs[3];
        break;
    case 4:
        ofs = Menu_Get()->fxLineOfs[4];
        break;
    case 5:
        ofs = Menu_Get()->fxLineOfs[5];
        break;
    }
    sFxSlide.from = fn_8132DE98();
    sFxSlide.to = ofs + Option_GetLangRow();
    sFxSlide.t = 0;
    fn_813306FC(from, to);
}

/* 0x8132DE94 */
void FxOption_ChangeUiLang(s32 from, s32 to) {
}

/* 0x8132DE98: current position of the language list. */
f32 fn_8132DE98(void) {
    return Hermite(sFxSlide.t, 0.0f, sFxSlide.from, 0.0f, 10.0f, sFxSlide.to, 0.0f);
}

#define FX_PLACE_ITEM(listFn, countFn, line)                                          \
    list = listFn();                                                                \
    fn_8132C830(&o, sFxCube, list, countFn());                                      \
    for (i = 0; i < countFn(); i++) {                                               \
        sFxCube[list[i]].pos.x += Menu_Get()->fxItem[line].x;                       \
        sFxCube[list[i]].pos.y += Menu_Get()->fxItem[line].y;                       \
        Fx_ScaleAbout(&o, Menu_Get()->fxItem[line].scale, Menu_Get()->fxItem[line].scale, \
                      Menu_Get()->fxItem[line].scale, &sFxCube[list[i]]);           \
    }

#define FX_GROW_ITEM(listFn, countFn, line)                                           \
    list = listFn();                                                                \
    fn_8132C830(&o, sFxCube, list, countFn());                                      \
    for (i = 0; i < countFn(); i++) {                                               \
        Fx_ScaleAbout(&o,                                                           \
                      1.0f + Fx_Lit(list[i]) * Menu_Get()->fxItem[line].spacing,    \
                      1.0f + Fx_Lit(list[i]) * Menu_Get()->fxItem[line].spacing,    \
                      1.0f + Fx_Lit(list[i]) * Menu_Get()->fxItem[line].spacing,    \
                      &sFxCube[list[i]]);                                           \
    }

/* 0x8132DEFC: lay out the Calendar date and time lines. */
void fn_8132DEFC(void) {
    s32 i;
    u16* list;
    Vec o;

    FX_PLACE_ITEM(fn_813315C8, fn_81331630, 0)
    FX_PLACE_ITEM(fn_813311F4, fn_81331258, 1)
    if (sFxState.v[6] == 1) {
        switch (sFxState.v[8]) {
        case 0:
            FX_GROW_ITEM(fn_81332008, fn_81332070, 0)
            break;
        case 1:
            FX_GROW_ITEM(fn_81331C30, fn_81331C98, 0)
            break;
        case 2:
            FX_GROW_ITEM(fn_8133166C, fn_813316D4, 0)
            break;
        }
    } else if (sFxState.v[6] == 2) {
        switch (sFxState.v[9]) {
        case 0:
            FX_GROW_ITEM(fn_81331858, fn_813318C0, 1)
            break;
        case 1:
            FX_GROW_ITEM(fn_81331A44, fn_81331AAC, 1)
            break;
        case 2:
            FX_GROW_ITEM(fn_81331E1C, fn_81331E84, 1)
            break;
        }
    }
}

/* 0x8132EC54: place the cursor glyph, per-language offset and scale. */
void fn_8132EC54(void) {
    f32 x;
    f32 y;
    f32 s;
    u32 i;
    FxCube* c;
    f32 oz;
    f32 oy;
    f32 ox;
    Vec o;
    u8 unused[12];

    y = Menu_Get()->fxCursorY;
    switch (Option_GetLang()) {
    case 0:
    case 3:
    case 4:
    case 5:
        x = Menu_Get()->fxCursorX;
        s = Menu_Get()->fxCursorScale;
        break;
    case 1:
        x = -36.0f;
        s = 0.65f;
        break;
    case 2:
        x = -36.0f;
        s = 0.6f;
        break;
    }
    c = sFxCube;
    fn_8132C830(&o, sFxCube, NULL, sFxCount);
    oz = o.z;
    oy = o.y;
    ox = o.x;
    for (i = 0; i < sFxCount; i++) {
        c->pos.x += x;
        c->pos.y += y;
        c->pos.x = ox + s * (c->pos.x - ox);
        c->pos.y = oy + s * (c->pos.y - oy);
        c->pos.z = oz + s * (c->pos.z - oz);
        c->scale.x *= s;
        c->scale.y *= s;
        c->scale.z *= s;
        c++;
    }
}

/* 0x8132EEB0: start fading a screen's cells in (on) or out. */
void fn_8132EEB0(s32 screen, s32 on) {
    u16* lists[6];
    u16 counts[6];
    u16* list;
    u16 n;
    u16 i;
    u16 idx;

    lists[0] = fn_813329A4();
    counts[0] = fn_81332A0C();
    lists[1] = fn_81332A48();
    counts[1] = fn_81332AB0();
    lists[2] = fn_81332AEC();
    counts[2] = fn_81332B54();
    lists[3] = fn_81332B90();
    counts[3] = fn_81332BF8();
    lists[4] = fn_813310AC();
    counts[4] = fn_81331114();
    lists[5] = fn_81331150();
    counts[5] = fn_813311B8();
    list = lists[screen];
    n = counts[screen];
    for (i = 0; i < n; i++) {
        idx = *list;
        if (on) {
            sFxPool.v[5][idx] = Menu_Get()->fxFadeIn;
        } else {
            sFxPool.v[5][idx] = 0;
        }
        idx = *list;
        if (on) {
            sFxPool.v[3][idx] = Menu_Get()->fxFadeIn;
        } else {
            sFxPool.v[3][idx] = 0;
        }
        idx = *list;
        sFxPool.v[7][idx] = Menu_Get()->fxFadeIn;
        idx = *list;
        sFxPool.v[1][idx] = Menu_Get()->fxFadeOut;
        list++;
    }
}

f32 fn_8132FF6C(void);

/* Selection bar: slide a line's cells by the title offset and scale them about o. */
#define FX_SEL_PLACE(listFn, countFn, o, line)                                        \
    list = listFn();                                                                \
    for (i = 0; i < countFn(); i++) {                                               \
        sFxCube[list[i]].pos.x += Menu_Get()->fxTitle[line].x;                      \
        sFxCube[list[i]].pos.y += Menu_Get()->fxTitle[line].y;                      \
        Fx_ScaleAbout(&o, Menu_Get()->fxTitle[line].scale, Menu_Get()->fxTitle[line].scale, \
                      Menu_Get()->fxTitle[line].scale, &sFxCube[list[i]]);          \
    }

/* Selection bar: grow the cells, swing them by the blink and move them to an end. */
#define FX_SEL_GROW(countFn, o, line, OP, X, Y)                                       \
    for (i = 0; i < countFn(); i++) {                                               \
        Fx_ScaleAbout(&o, 1.0f + Menu_Get()->fxTitle[line].spacing,                 \
                      1.0f + Menu_Get()->fxTitle[line].spacing,                     \
                      1.0f + Menu_Get()->fxTitle[line].spacing, &sFxCube[list[i]]); \
        sFxCube[list[i]].pos.x OP Menu_Get()->fxF0 * Hermite(fn_8132FF6C(), 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f); \
        X                                                                           \
        sFxCube[list[i]].pos.y += Y;                                                \
    }

/* 0x8132F010: lay out the selection bars of the three title lines. */
void fn_8132F010(void) {
    s32 i;
    u16* list;
    Vec o0;
    Vec o1;
    Vec o2;

    list = fn_81332C34();
    fn_8132C830(&o0, sFxCube, list, fn_81332C9C());
    o0.x += Menu_Get()->fxTitle[0].x;
    o0.y += Menu_Get()->fxTitle[0].y;
    list = fn_813327B8();
    fn_8132C830(&o1, sFxCube, list, fn_81332820());
    o1.x += Menu_Get()->fxTitle[1].x;
    o1.y += Menu_Get()->fxTitle[1].y;
    list = fn_81331008();
    fn_8132C830(&o2, sFxCube, list, fn_81331070());
    o2.x += Menu_Get()->fxTitle[2].x;
    o2.y += Menu_Get()->fxTitle[2].y;

    FX_SEL_PLACE(fn_813329A4, fn_81332A0C, o0, 0)
    FX_SEL_GROW(fn_81332A0C, o0, 0, -=,
                sFxCube[list[i]].pos.x += Menu_Get()->fxSel[0];, Menu_Get()->fxSel[1])
    FX_SEL_PLACE(fn_81332A48, fn_81332AB0, o0, 0)
    FX_SEL_GROW(fn_81332AB0, o0, 0, +=,
                if (Option_GetLang() == 4) {
                    sFxCube[list[i]].pos.x += -40.0f;
                } else {
                    sFxCube[list[i]].pos.x += Menu_Get()->fxSel[2];
                }, Menu_Get()->fxSel[3])
    FX_SEL_PLACE(fn_81332AEC, fn_81332B54, o1, 1)
    FX_SEL_GROW(fn_81332B54, o1, 1, -=,
                sFxCube[list[i]].pos.x += Menu_Get()->fxSel[4];, Menu_Get()->fxSel[5])
    FX_SEL_PLACE(fn_81332B90, fn_81332BF8, o1, 1)
    FX_SEL_GROW(fn_81332BF8, o1, 1, +=,
                sFxCube[list[i]].pos.x += Menu_Get()->fxSel[6];, Menu_Get()->fxSel[7])
    FX_SEL_PLACE(fn_813310AC, fn_81331114, o2, 2)
    FX_SEL_GROW(fn_81331114, o2, 2, -=,
                sFxCube[list[i]].pos.x += Menu_Get()->fxSel2[0];, Menu_Get()->fxSel2[1])
    FX_SEL_PLACE(fn_81331150, fn_813311B8, o2, 2)
    FX_SEL_GROW(fn_813311B8, o2, 2, +=,
                sFxCube[list[i]].pos.x += Menu_Get()->fxSel2[2];, Menu_Get()->fxSel2[3])
}

/* 0x8132FF6C: blink triangle wave, 0..1..0 over fxBlinkPeriod frames. */
f32 fn_8132FF6C(void) {
    f32 t = (f32)sFxBlink / (f32)Menu_Get()->fxBlinkPeriod;

    if (t < 0.5) {
        return 2.0f * t;
    }
    return 2.0f * (1.0f - t);
}

/* 0x8132FFF0: give every cell the base colours and stop its fades. */
void fn_8132FFF0(void) {
    s32 i;

    for (i = 0; i < FX_MAX; i++) {
        sFxLightCol[i].r = sFxLight0.r;
        sFxLightCol[i].g = sFxLight0.g;
        sFxLightCol[i].b = sFxLight0.b;
        sFxLightCol[i].a = sFxLight0.a;
        sFxDarkCol[i].r = sFxDark0.r;
        sFxDarkCol[i].g = sFxDark0.g;
        sFxDarkCol[i].b = sFxDark0.b;
        sFxDarkCol[i].a = sFxDark0.a;
        sFxPool.v[7][i] = 0;
        sFxPool.v[5][i] = 0;
    }
}

/* Squash factor of the fade-out's first tenth. */
static inline f32 Fx_Squash(f32 t) {
    if (t < 0.1f) {
        return 1.0f - 0.05f * Hermite(t, 0.0f, 0.0f, 0.0f, 0.1f, 1.0f, 0.0f);
    } else {
        return 1.0f - 0.05f * Hermite(t, 0.1f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    }
}

/* Out-of-line copy of Fx_Squash; nothing calls it, so the linker strips it,
   but its constants stay in .sdata2. */
f32 Fx_SquashOut(f32 t) {
    if (t < 0.1f) {
        return 1.0f - 0.05f * Hermite(t, 0.0f, 0.0f, 0.0f, 0.1f, 1.0f, 0.0f);
    } else {
        return 1.0f - 0.05f * Hermite(t, 0.1f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    }
}

/* 0x81330128: fly the cells away for the fade out, staggered by sFxFlyDelay. */
void fn_81330128(FxCube* c, u32* n, f32 t) {
    s16 a;
    u32 i;
    f32 k;
    FxCube d;
    u8 unused[8];

    a = 255.0f * (1.0f - 1.4f * t);
    if (a < 0) {
        a = 0;
    }
    sFxLight0.a = a;
    for (i = 0; i < *n; i++) {
        k = t - 0.4f * (1.0f - sFxFlyDelay[i] / 255.0f);
        if (k < 0.0f) {
            k = 0.0f;
        }
        c[i].pos.x += -585.0f * k;
        c[i].pos.y += 200.0f * k;
        c[i].pos.z -= 500.0f * k;
        Fx_RotAbout(65535.0f * k, 0, 65535.0f * k, &c[i], &d, 0.0f, 0.0f, 0.0f);
        c[i].pos.x += d.pos.x;
        c[i].pos.y += d.pos.y;
        c[i].pos.z += d.pos.z;
        c[i].rot[0] += d.rot[0];
        c[i].rot[2] += d.rot[2];
    }
}

/* 0x81330374: fade-out shape: a short squash, then the fly-away. */
void fn_81330374(FxCube* c, u32* n, f32 t) {
    Vec o;
    f32 s;
    u32 i;
    u8 unused[12];

    Fx_EvalShape(1, c, n, 1.0f);
    if (t < 0.1f) {
        fn_8132C830(&o, c, NULL, *n);
        s = Fx_Squash(10.0f * t);
        for (i = 0; i < *n; i++) {
            Fx_ScaleAbout(&o, s, s, s, &c[i]);
            c[i].scale.x *= s;
            c[i].scale.y *= s;
            c[i].scale.z *= s;
        }
    } else {
        fn_81330128(c, n, t - 0.1f);
    }
}

/* 0x81330518: bilinear colour mix: (a*u + b*(1-u))*(1-v) + c*v. */
void fn_81330518(GXColor* a, GXColor* b, GXColor* c, GXColor* out, f32 u, f32 v) {
    f32 iu = 1.0f - u;
    f32 iv = 1.0f - v;

    out->r = v * c->r + iv * (u * a->r + iu * b->r);
    out->g = v * c->g + iv * (u * a->g + iu * b->g);
    out->b = v * c->b + iv * (u * a->b + iu * b->b);
    out->a = v * c->a + iv * (u * a->a + iu * b->a);
}

/* 0x813306C8: park the cursor on row. */
void fn_813306C8(s32 row) {
    sFxCursor.v[2] = 10;
    sFxCursor.v[1] = row;
    sFxCursor.v[0] = row;
    sFxCursor.f[2] = 0.0f;
    sFxCursor.f[1] = 0.0f;
    sFxCursor.f[0] = 0.0f;
}

/* 0x813306FC: start moving the cursor from row to row, with random cell delays. */
void fn_813306FC(s32 from, s32 to) {
    s32 i;

    if (from < to) {
        sFxCursor.f[0] = -1.0f;
    } else {
        sFxCursor.f[0] = 1.0f;
    }
    sFxCursor.v[2] = 0;
    sFxCursor.v[0] = from;
    sFxCursor.v[1] = to;
    for (i = 0; i < 0x280; i++) {
        sFxCursor.unk14[i] = Rand() % 60 + 20;
    }
}

static inline f32 FxCursorT(void) {
    return sFxCursor.v[2] / 10.0f;
}

/* 0x813307B4: cross-fade the cursor glyph between its old and new row. */
void fn_813307B4(void) {
    u16* fromList;
    u16 fromCount;
    u16 toCount;
    u16* toList;

    s32 i;

    switch (sFxState.v[5]) {
    case 0:
        fromList = fn_81330F88(sFxCursor.v[0]);
        fromCount = fn_81330FAC(sFxCursor.v[0]);
        toList = fn_81330F88(sFxCursor.v[1]);
        toCount = fn_81330FAC(sFxCursor.v[1]);
        break;
    case 1:
        fromList = fn_81330FC8(sFxCursor.v[0]);
        fromCount = fn_81330FEC(sFxCursor.v[0]);
        toList = fn_81330FC8(sFxCursor.v[1]);
        toCount = fn_81330FEC(sFxCursor.v[1]);
        break;
    }
    for (i = 0; i < fn_81332C9C(); i++) {
        sFxAlpha[fn_81332C34()[i]] = 0;
    }
    for (i = 0; i < fromCount; i++) {
        sFxCube[sFxCount + i] = sFxCube[fromList[i]];
        sFxCube[sFxCount + i].pos.x -= 0.2 * (sFxCursor.f[0] * (sFxCursor.unk14[i] * FxCursorT()));
        sFxAlpha[sFxCount + i] = 255.0f * (1.0f - FxCursorT());
        sFxLightCol[sFxCount + i] = sFxLight2;
        sFxDarkCol[sFxCount + i] = sFxDark2;
    }
    sFxCount += fromCount;
    for (i = 0; i < toCount; i++) {
        sFxCube[sFxCount + i] = sFxCube[toList[i]];
        sFxCube[sFxCount + i].pos.x += 0.2 * (sFxCursor.f[0] * (sFxCursor.unk14[fromCount + i] * (1.0f - FxCursorT())));
        sFxAlpha[sFxCount + i] = 255.0f * FxCursorT();

        sFxLightCol[sFxCount + i] = sFxLight2;
        sFxDarkCol[sFxCount + i] = sFxDark2;
    }
    sFxCount += toCount;
    fn_81330B7C();
}

/* 0x81330B7C: fade the selected line's cells across to the cursor's copy. */
void fn_81330B7C(void) {
    u16* list;
    u16 n;
    s32 i;

    list = fn_813327B8();
    n = fn_81332820();
    for (i = 0; i < n; i++) {
        sFxCube[sFxCount + i] = sFxCube[list[i]];
        sFxCube[sFxCount + i].pos.x -= 0.2 * (sFxCursor.f[0] * (sFxCursor.unk14[i] * FxCursorT()));
        sFxAlpha[sFxCount + i] = sFxAlpha[list[i]] * (1.0f - FxCursorT());

        sFxLightCol[sFxCount + i] = sFxLightCol[list[i]];
        sFxDarkCol[sFxCount + i] = sFxDarkCol[list[i]];
    }
    sFxCount += n;
    for (i = 0; i < n; i++) {
        sFxCube[list[i]].pos.x += 0.2 * (sFxCursor.f[0] * (sFxCursor.unk14[i] * (1.0f - FxCursorT())));
        sFxAlpha[list[i]] = sFxAlpha[list[i]] * FxCursorT();

    }
}
