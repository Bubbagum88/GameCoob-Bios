// Partly matching.
//
// Mosaic text ("fx", PAL 1.0 BS2, 0x81329890..0x81330F30): the titles and
// items of the Game Play, Options and Calendar submenus are drawn with
// small cubes. Every cube flies along a Hermite curve from a keyframe
// table (res 37) to its place in a glyph shape (res 179..184); this unit
// owns the cube pool, evaluates the curves and lays out the text of each
// submenu. The glyph strings live in menu_fx_anim (0x81330F30..), the
// per-screen cube effects in menu_fx_draw (0x81333030..).

#include "bs2/menu.h"
#include "string.h"
#include "dolphin/os.h"

#define FX_MAX 0x861

/* One cube target written by the layout code. */
typedef struct FxItem {
    u8 data[20];
} FxItem;

/* A placed cube, as handed to the renderer. */
typedef struct FxCube {
    Vec scale;      /* 0x00 */
    s16 rot[3];     /* 0x0C */
    Vec pos;        /* 0x14 */
} FxCube;           /* 0x20 */

/* Shape table (res 37): one entry per shape, then per-cube key lists. */
typedef struct FxShape {
    u16 count;      /* 0x0 cubes in the shape */
    u16 nKeys;      /* 0x2 keyframes per cube */
    s32 cubes;      /* 0x4 offset of the FxShapeCube list from this entry */
} FxShape;

typedef struct FxShapeCube {
    u8 start;       /* 0x0 when the cube starts moving (0..255 -> 0..0.4) */
    u8 end;         /* 0x1 when it arrives (0..255 -> 0.6..1.0) */
    s32 keys;       /* 0x4 offset of the u16 key index list from this entry */
} FxShapeCube;

/* Curve key: a point and its incoming/outgoing tangents. */
typedef struct FxKey {
    Vec pos;        /* 0x00 */
    Vec in;         /* 0x0C */
    Vec out;        /* 0x18 */
} FxKey;            /* 0x24 */

typedef struct FxShapeFile {
    u8 pad[8];
    s32 keys;       /* 0x8 offset of the FxKey array */
    s32 shapes;     /* 0xC offset of the FxShape array */
} FxShapeFile;

typedef struct FxPool {
    FxItem item[FX_MAX];
    s16 frame;      /* 0xA794 frames since the screen opened */
    s16 ready;      /* 0xA796 frame count at which input is accepted */
} FxPool;

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
void fn_8132C024(void);
void fn_8132C2FC(void);
void fn_8132C5E0(void);
void fn_8132C660(s16* digits, s32 value);
f32 fn_8132CCF4(void);
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
void fn_8132DEFC(void);
void fn_8132EC54(void);
void fn_8132F010(void);
void fn_8132FFF0(void);
void fn_81330374(FxCube* out, u32* count);
void fn_813306C8(s32 lang);
void fn_813307B4(void);

void Fx_EmitSparks(FxCube* cubes);
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
static u8 lbl_814FA28C[0x2184];
static u8 lbl_814FC410[0x2418];
static s16 lbl_814FE828[6];
static u8 sFxPad[0x1C];

static FxShapeFile* sFxFile;
static FxShape* sFxShapes;
static FxKey* sFxKeys;
static u32 sFxUnused;
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
static u32 sFxUnused2;

/* 0x81329890 */
void Fx_SetShapes(FxShapeFile* file) {
    sFxFile = file;
    sFxShapes = (FxShape*)((u8*)file + file->shapes);
    sFxKeys = (FxKey*)((u8*)file + file->keys);
}

/* 0x813298B0: place the cubes of shape n at time t (0..1). */
void Fx_EvalShape(s32 n, FxCube* out, u32* count, f32 t) {
    FxShapeCube* c;
    FxShape* s;
    f32 step;
    u32 i;
    u32 k;
    Vec v;
    f32 seg;
    f32 u;
    f32 hi;
    f32 lo;

    *count = sFxShapes[n].count;
    s = &sFxShapes[n];
    c = (FxShapeCube*)((u8*)sFxShapes + (n * 8 + s->cubes));
    step = 1.0f / (s32)(s->nKeys - 1);
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    for (i = 0; i < *count; i++) {
        u16* keys = (u16*)((u8*)c + c->keys);
        FxKey* k0;
        FxKey* k1;

        hi = 0.6f + 0.4f * (1.0f - c->end / 255.0f);
        lo = 0.4f * (1.0f - c->start / 255.0f);
        u = (t - lo) / (hi - lo);
        seg = u / step;
        k = seg;
        if ((u32)seg >= sFxShapes[n].nKeys) {
            k = sFxShapes[n].nKeys - 1;
        }
        if (u < 0.0f) {
            u = 0.0f;
        }
        if (u > 1.0f) {
            u = 1.0f;
        }
        k1 = &sFxKeys[keys[k + 1]];
        k0 = &sFxKeys[keys[k]];
        VecHermite(&k0->pos, &k0->out, &k1->pos, &k1->in, &v, u, k * step, (k + 1) * step);
        out->pos.x = v.x;
        out->pos.y = v.y;
        out->pos.z = v.z;
        out->rot[0] = out->rot[1] = out->rot[2] = 0;
        out->scale.x = out->scale.y = out->scale.z = Menu_Get()->fxCubeScale;
        c++;
        out++;
    }
}

/* 0x81329B24 */
u16 fn_81329B24(s32 n) {
    return sFxShapes[n].count;
}

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
    return lbl_814FA28C;
}

/* 0x81329B60 */
void* fn_81329B60(void) {
    return lbl_814FC410;
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
    memset(lbl_814FE828, 0, sizeof(lbl_814FE828));
    sFxState.screen = 0;
    sFxPrev = sFxState;
    fn_81330F30(keys);
    Fx_SetShapes(curves);
}

/* 0x81329C98 */
void Fx_Clear(void) {
    fn_8132D000();
    memset(&sFxPool, 0, sizeof(FxPool));
    memset(lbl_814FE828, 0, sizeof(lbl_814FE828));
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
        memset(lbl_814FE828, 0, sizeof(lbl_814FE828));
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
        memset(lbl_814FE828, 0, sizeof(lbl_814FE828));
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
        memset(lbl_814FE828, 0, sizeof(lbl_814FE828));
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
        fn_8132CCF4();
        fn_81330374(sFxCube, &sFxCount);
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
