// Mosaic cube text ("fx") glyph tables, PAL 1.0 BS2 0x81330F30..0x81332FB8.
//
// Each language has one glyph resource (Res_FxGlyphsLang). It starts with a
// table of {count, offset} entries, one per text the mosaic can show; the
// offset is relative to the entry itself. The small accessors return the
// cell data and the cell count of one entry for the current language.

#include "bs2/menu.h"

typedef struct FxGlyphEntry {
    u16 count;
    u16 offset;
} FxGlyphEntry;

typedef struct FxGlyphs {
    FxGlyphEntry entry[0x38];
} FxGlyphs;

#define FX_GLYPH_DATA(g, k) ((u8*)&(g)->entry[k] + (g)->entry[k].offset)

s32 Res_FxGlyphsLang(s32 lang);

static FxGlyphs* sFxGlyphRes[6];

static inline FxGlyphs* FxGlyphs_Load(s32 lang) {
    s32 id = Res_FxGlyphsLang(lang);
    return ResGet(id);
}

/* 0x81330F30 */
void fn_81330F30(void) {
    s32 i;

    for (i = 0; i < 6; i++) {
        sFxGlyphRes[i] = FxGlyphs_Load(i);
    }
}

/* 0x81330F88 */
void* fn_81330F88(s32 lang) {
    return FX_GLYPH_DATA(sFxGlyphRes[lang], 0x2C);
}

/* 0x81330FAC */
u16 fn_81330FAC(s32 lang) {
    return sFxGlyphRes[lang]->entry[0x2C].count;
}

/* 0x81330FC8 */
void* fn_81330FC8(s32 lang) {
    return FX_GLYPH_DATA(sFxGlyphRes[lang], 0x37);
}

/* 0x81330FEC */
u16 fn_81330FEC(s32 lang) {
    return sFxGlyphRes[lang]->entry[0x37].count;
}

/* 0x81331008 */
void* fn_81331008(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x2B);
}

/* 0x81331070 */
u16 fn_81331070(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x2B].count;
}

/* 0x813310AC */
void* fn_813310AC(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x34);
}

/* 0x81331114 */
u16 fn_81331114(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x34].count;
}

/* 0x81331150 */
void* fn_81331150(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x35);
}

/* 0x813311B8 */
u16 fn_813311B8(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x35].count;
}

/* 0x813311F4 */
void* fn_813311F4(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x00);
}

/* 0x81331258 */
u16 fn_81331258(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x00].count;
}

/* 0x81331294 */
void* fn_81331294(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x01);
}

/* 0x813312FC */
u16 fn_813312FC(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x01].count;
}

/* 0x81331338 */
void* fn_81331338(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x02);
}

/* 0x813313A0 */
u16 fn_813313A0(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x02].count;
}

/* 0x813313DC */
void* fn_813313DC(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x03);
}

/* 0x81331444 */
u16 fn_81331444(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x03].count;
}

/* 0x81331480 */
void* fn_81331480(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x04);
}

/* 0x813314E8 */
u16 fn_813314E8(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x04].count;
}

/* 0x81331524 */
void* fn_81331524(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x05);
}

/* 0x8133158C */
u16 fn_8133158C(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x05].count;
}

/* 0x813315C8 */
void* fn_813315C8(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x06);
}

/* 0x81331630 */
u16 fn_81331630(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x06].count;
}

/* 0x8133166C */
void* fn_8133166C(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x07);
}

/* 0x813316D4 */
u16 fn_813316D4(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x07].count;
}

/* 0x81331710 */
void* fn_81331710(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x08);
}

/* 0x81331778 */
u16 fn_81331778(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x08].count;
}

/* 0x813317B4 */
void* fn_813317B4(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x09);
}

/* 0x8133181C */
u16 fn_8133181C(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x09].count;
}

/* 0x81331858 */
void* fn_81331858(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x0A);
}

/* 0x813318C0 */
u16 fn_813318C0(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x0A].count;
}

/* 0x813318FC */
void* fn_813318FC(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x0B);
}

/* 0x81331964 */
u16 fn_81331964(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x0B].count;
}

/* 0x813319A0 */
void* fn_813319A0(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x0C);
}

/* 0x81331A08 */
u16 fn_81331A08(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x0C].count;
}

/* 0x81331A44 */
void* fn_81331A44(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x0D);
}

/* 0x81331AAC */
u16 fn_81331AAC(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x0D].count;
}

/* 0x81331AE8 */
void* fn_81331AE8(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x0E);
}

/* 0x81331B50 */
u16 fn_81331B50(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x0E].count;
}

/* 0x81331B8C */
void* fn_81331B8C(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x0F);
}

/* 0x81331BF4 */
u16 fn_81331BF4(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x0F].count;
}

/* 0x81331C30 */
void* fn_81331C30(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x10);
}

/* 0x81331C98 */
u16 fn_81331C98(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x10].count;
}

/* 0x81331CD4 */
void* fn_81331CD4(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x11);
}

/* 0x81331D3C */
u16 fn_81331D3C(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x11].count;
}

/* 0x81331D78 */
void* fn_81331D78(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x12);
}

/* 0x81331DE0 */
u16 fn_81331DE0(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x12].count;
}

/* 0x81331E1C */
void* fn_81331E1C(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x13);
}

/* 0x81331E84 */
u16 fn_81331E84(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x13].count;
}

/* 0x81331EC0 */
void* fn_81331EC0(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x14);
}

/* 0x81331F28 */
u16 fn_81331F28(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x14].count;
}

/* 0x81331F64 */
void* fn_81331F64(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x15);
}

/* 0x81331FCC */
u16 fn_81331FCC(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x15].count;
}

/* 0x81332008 */
void* fn_81332008(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x16);
}

/* 0x81332070 */
u16 fn_81332070(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x16].count;
}

/* 0x813320AC */
void* fn_813320AC(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x17);
}

/* 0x81332114 */
u16 fn_81332114(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x17].count;
}

/* 0x81332150 */
void* fn_81332150(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x18);
}

/* 0x813321B8 */
u16 fn_813321B8(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x18].count;
}

/* 0x813321F4 */
void* fn_813321F4(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x19);
}

/* 0x8133225C */
u16 fn_8133225C(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x19].count;
}

/* 0x81332298 */
void* fn_81332298(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x1A);
}

/* 0x81332300 */
u16 fn_81332300(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x1A].count;
}

/* 0x8133233C */
void* fn_8133233C(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x1B);
}

/* 0x813323A4 */
u16 fn_813323A4(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x1B].count;
}

/* 0x813323E0 */
void* fn_813323E0(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x1C);
}

/* 0x81332448 */
u16 fn_81332448(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x1C].count;
}

/* 0x81332484 */
void* fn_81332484(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x1D);
}

/* 0x813324EC */
u16 fn_813324EC(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x1D].count;
}

/* 0x81332528 */
void* fn_81332528(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x28);
}

/* 0x81332590 */
u16 fn_81332590(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x28].count;
}

/* 0x813325CC */
void* fn_813325CC(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x29);
}

/* 0x81332634 */
u16 fn_81332634(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x29].count;
}

/* 0x81332670 */
void* fn_81332670(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x2A);
}

/* 0x813326D8 */
u16 fn_813326D8(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x2A].count;
}

/* 0x81332714 */
void* fn_81332714(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x2C);
}

/* 0x8133277C */
u16 fn_8133277C(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x2C].count;
}

/* 0x813327B8 */
void* fn_813327B8(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x2D);
}

/* 0x81332820 */
u16 fn_81332820(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x2D].count;
}

/* 0x8133285C */
void* fn_8133285C(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x2E);
}

/* 0x813328C4 */
u16 fn_813328C4(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x2E].count;
}

/* 0x81332900 */
void* fn_81332900(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x2F);
}

/* 0x81332968 */
u16 fn_81332968(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x2F].count;
}

/* 0x813329A4 */
void* fn_813329A4(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x30);
}

/* 0x81332A0C */
u16 fn_81332A0C(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x30].count;
}

/* 0x81332A48 */
void* fn_81332A48(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x31);
}

/* 0x81332AB0 */
u16 fn_81332AB0(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x31].count;
}

/* 0x81332AEC */
void* fn_81332AEC(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x32);
}

/* 0x81332B54 */
u16 fn_81332B54(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x32].count;
}

/* 0x81332B90 */
void* fn_81332B90(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x33);
}

/* 0x81332BF8 */
u16 fn_81332BF8(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x33].count;
}

/* 0x81332C34 */
void* fn_81332C34(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x36);
}

/* 0x81332C9C */
u16 fn_81332C9C(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x36].count;
}

/* 0x81332CD8 */
void* fn_81332CD8(void) {
    return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x37);
}

/* 0x81332D40 */
u16 fn_81332D40(void) {
    return sFxGlyphRes[Lang_Get()]->entry[0x37].count;
}

/* 0x81332D7C */
void* fn_81332D7C(s16 text) {
    switch (text) {
    case 0:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x1E);
    case 1:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x1F);
    case 2:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x20);
    case 3:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x21);
    case 4:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x22);
    case 5:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x23);
    case 6:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x24);
    case 7:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x25);
    case 8:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x26);
    case 9:
        return FX_GLYPH_DATA(sFxGlyphRes[Lang_Get()], 0x27);
    }
    return NULL;
}
