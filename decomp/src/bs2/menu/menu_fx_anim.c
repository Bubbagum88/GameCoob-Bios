// Non-matching: functionally equivalent decompilation
//
// Mosaic cube text ("fx") used inside Game Play, Options and Calendar,
// 0x81330F30..0x81333030: keyframe (CAFD res 37) curves and small accessors.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x81330F30 */
void fn_81330F30(void) {
    s32 var_r30;
    u16 **var_r31;

    var_r31 = &lbl_814FE850;
    var_r30 = 0;
    do {
        Res_FxGlyphsLang(var_r30);
        var_r30 += 1;
        *var_r31 = ResGet();
        var_r31 += 4;
    } while (var_r30 < 6);
}

/* 0x81330F88 */
void *fn_81330F88(s32 arg0) {
    u16 *temp_r3;

    temp_r3 = (&lbl_814FE850)[arg0];
    return temp_r3 + temp_r3->unkB2 + 0xB0;
}

/* 0x81330FAC */
u16 fn_81330FAC(s32 arg0) {
    return (&lbl_814FE850)[arg0]->unkB0;
}

/* 0x81330FC8 */
void *fn_81330FC8(s32 arg0) {
    u16 *temp_r3;

    temp_r3 = (&lbl_814FE850)[arg0];
    return temp_r3 + temp_r3->unkDE + 0xDC;
}

/* 0x81330FEC */
u16 fn_81330FEC(s32 arg0) {
    return (&lbl_814FE850)[arg0]->unkDC;
}

/* 0x81331008 */
void *fn_81331008(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkAE;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xAC;
}

/* 0x81331070 */
u16 fn_81331070(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkAC;
}

/* 0x813310AC */
void *fn_813310AC(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkD2;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xD0;
}

/* 0x81331114 */
u16 fn_81331114(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkD0;
}

/* 0x81331150 */
void *fn_81331150(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkD6;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xD4;
}

/* 0x813311B8 */
u16 fn_813311B8(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkD4;
}

/* 0x813311F4 */
void *fn_813311F4(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk2;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31;
}

/* 0x81331258 */
u16 fn_81331258(void) {
    return *(&lbl_814FE850)[Lang_Get()];
}

/* 0x81331294 */
void *fn_81331294(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk6;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 4;
}

/* 0x813312FC */
u16 fn_813312FC(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk4;
}

/* 0x81331338 */
void *fn_81331338(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkA;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 8;
}

/* 0x813313A0 */
u16 fn_813313A0(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk8;
}

/* 0x813313DC */
void *fn_813313DC(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkE;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xC;
}

/* 0x81331444 */
u16 fn_81331444(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkC;
}

/* 0x81331480 */
void *fn_81331480(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk12;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x10;
}

/* 0x813314E8 */
u16 fn_813314E8(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk10;
}

/* 0x81331524 */
void *fn_81331524(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk16;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x14;
}

/* 0x8133158C */
u16 fn_8133158C(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk14;
}

/* 0x813315C8 */
void *fn_813315C8(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk1A;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x18;
}

/* 0x81331630 */
u16 fn_81331630(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk18;
}

/* 0x8133166C */
void *fn_8133166C(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk1E;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x1C;
}

/* 0x813316D4 */
u16 fn_813316D4(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk1C;
}

/* 0x81331710 */
void *fn_81331710(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk22;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x20;
}

/* 0x81331778 */
u16 fn_81331778(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk20;
}

/* 0x813317B4 */
void *fn_813317B4(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk26;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x24;
}

/* 0x8133181C */
u16 fn_8133181C(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk24;
}

/* 0x81331858 */
void *fn_81331858(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk2A;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x28;
}

/* 0x813318C0 */
u16 fn_813318C0(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk28;
}

/* 0x813318FC */
void *fn_813318FC(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk2E;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x2C;
}

/* 0x81331964 */
u16 fn_81331964(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk2C;
}

/* 0x813319A0 */
void *fn_813319A0(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk32;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x30;
}

/* 0x81331A08 */
u16 fn_81331A08(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk30;
}

/* 0x81331A44 */
void *fn_81331A44(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk36;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x34;
}

/* 0x81331AAC */
u16 fn_81331AAC(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk34;
}

/* 0x81331AE8 */
void *fn_81331AE8(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk3A;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x38;
}

/* 0x81331B50 */
u16 fn_81331B50(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk38;
}

/* 0x81331B8C */
void *fn_81331B8C(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk3E;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x3C;
}

/* 0x81331BF4 */
u16 fn_81331BF4(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk3C;
}

/* 0x81331C30 */
void *fn_81331C30(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk42;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x40;
}

/* 0x81331C98 */
u16 fn_81331C98(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk40;
}

/* 0x81331CD4 */
void *fn_81331CD4(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk46;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x44;
}

/* 0x81331D3C */
u16 fn_81331D3C(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk44;
}

/* 0x81331D78 */
void *fn_81331D78(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk4A;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x48;
}

/* 0x81331DE0 */
u16 fn_81331DE0(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk48;
}

/* 0x81331E1C */
void *fn_81331E1C(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk4E;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x4C;
}

/* 0x81331E84 */
u16 fn_81331E84(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk4C;
}

/* 0x81331EC0 */
void *fn_81331EC0(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk52;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x50;
}

/* 0x81331F28 */
u16 fn_81331F28(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk50;
}

/* 0x81331F64 */
void *fn_81331F64(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk56;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x54;
}

/* 0x81331FCC */
u16 fn_81331FCC(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk54;
}

/* 0x81332008 */
void *fn_81332008(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk5A;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x58;
}

/* 0x81332070 */
u16 fn_81332070(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk58;
}

/* 0x813320AC */
void *fn_813320AC(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk5E;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x5C;
}

/* 0x81332114 */
u16 fn_81332114(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk5C;
}

/* 0x81332150 */
void *fn_81332150(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk62;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x60;
}

/* 0x813321B8 */
u16 fn_813321B8(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk60;
}

/* 0x813321F4 */
void *fn_813321F4(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk66;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x64;
}

/* 0x8133225C */
u16 fn_8133225C(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk64;
}

/* 0x81332298 */
void *fn_81332298(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk6A;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x68;
}

/* 0x81332300 */
u16 fn_81332300(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk68;
}

/* 0x8133233C */
void *fn_8133233C(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk6E;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x6C;
}

/* 0x813323A4 */
u16 fn_813323A4(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk6C;
}

/* 0x813323E0 */
void *fn_813323E0(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk72;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x70;
}

/* 0x81332448 */
u16 fn_81332448(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk70;
}

/* 0x81332484 */
void *fn_81332484(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unk76;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0x74;
}

/* 0x813324EC */
u16 fn_813324EC(void) {
    return (&lbl_814FE850)[Lang_Get()]->unk74;
}

/* 0x81332528 */
void *fn_81332528(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkA2;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xA0;
}

/* 0x81332590 */
u16 fn_81332590(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkA0;
}

/* 0x813325CC */
void *fn_813325CC(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkA6;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xA4;
}

/* 0x81332634 */
u16 fn_81332634(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkA4;
}

/* 0x81332670 */
void *fn_81332670(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkAA;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xA8;
}

/* 0x813326D8 */
u16 fn_813326D8(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkA8;
}

/* 0x81332714 */
void *fn_81332714(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkB2;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xB0;
}

/* 0x8133277C */
u16 fn_8133277C(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkB0;
}

/* 0x813327B8 */
void *fn_813327B8(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkB6;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xB4;
}

/* 0x81332820 */
u16 fn_81332820(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkB4;
}

/* 0x8133285C */
void *fn_8133285C(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkBA;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xB8;
}

/* 0x813328C4 */
u16 fn_813328C4(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkB8;
}

/* 0x81332900 */
void *fn_81332900(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkBE;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xBC;
}

/* 0x81332968 */
u16 fn_81332968(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkBC;
}

/* 0x813329A4 */
void *fn_813329A4(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkC2;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xC0;
}

/* 0x81332A0C */
u16 fn_81332A0C(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkC0;
}

/* 0x81332A48 */
void *fn_81332A48(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkC6;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xC4;
}

/* 0x81332AB0 */
u16 fn_81332AB0(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkC4;
}

/* 0x81332AEC */
void *fn_81332AEC(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkCA;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xC8;
}

/* 0x81332B54 */
u16 fn_81332B54(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkC8;
}

/* 0x81332B90 */
void *fn_81332B90(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkCE;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xCC;
}

/* 0x81332BF8 */
u16 fn_81332BF8(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkCC;
}

/* 0x81332C34 */
void *fn_81332C34(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkDA;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xD8;
}

/* 0x81332C9C */
u16 fn_81332C9C(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkD8;
}

/* 0x81332CD8 */
void *fn_81332CD8(void) {
    u16 *temp_r3;
    u16 temp_r31;

    temp_r3 = (&lbl_814FE850)[Lang_Get()];
    temp_r31 = temp_r3->unkDE;
    return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r31 + 0xDC;
}

/* 0x81332D40 */
u16 fn_81332D40(void) {
    return (&lbl_814FE850)[Lang_Get()]->unkDC;
}

/* 0x81332D7C */
void *fn_81332D7C(s16 arg0) {
    u16 *temp_r3;
    u16 *temp_r3_10;
    u16 *temp_r3_2;
    u16 *temp_r3_3;
    u16 *temp_r3_4;
    u16 *temp_r3_5;
    u16 *temp_r3_6;
    u16 *temp_r3_7;
    u16 *temp_r3_8;
    u16 *temp_r3_9;
    u16 temp_r30;
    u16 temp_r30_10;
    u16 temp_r30_2;
    u16 temp_r30_3;
    u16 temp_r30_4;
    u16 temp_r30_5;
    u16 temp_r30_6;
    u16 temp_r30_7;
    u16 temp_r30_8;
    u16 temp_r30_9;

    if ((u32) arg0 <= 9U) {
        switch (arg0) {
        case 0:
            temp_r3 = (&lbl_814FE850)[Lang_Get()];
            temp_r30 = temp_r3->unk7A;
            return (&lbl_814FE850)[Lang_Get(temp_r3)] + temp_r30 + 0x78;
        case 1:
            temp_r3_2 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_2 = temp_r3_2->unk7E;
            return (&lbl_814FE850)[Lang_Get(temp_r3_2)] + temp_r30_2 + 0x7C;
        case 2:
            temp_r3_3 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_3 = temp_r3_3->unk82;
            return (&lbl_814FE850)[Lang_Get(temp_r3_3)] + temp_r30_3 + 0x80;
        case 3:
            temp_r3_4 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_4 = temp_r3_4->unk86;
            return (&lbl_814FE850)[Lang_Get(temp_r3_4)] + temp_r30_4 + 0x84;
        case 4:
            temp_r3_5 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_5 = temp_r3_5->unk8A;
            return (&lbl_814FE850)[Lang_Get(temp_r3_5)] + temp_r30_5 + 0x88;
        case 5:
            temp_r3_6 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_6 = temp_r3_6->unk8E;
            return (&lbl_814FE850)[Lang_Get(temp_r3_6)] + temp_r30_6 + 0x8C;
        case 6:
            temp_r3_7 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_7 = temp_r3_7->unk92;
            return (&lbl_814FE850)[Lang_Get(temp_r3_7)] + temp_r30_7 + 0x90;
        case 7:
            temp_r3_8 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_8 = temp_r3_8->unk96;
            return (&lbl_814FE850)[Lang_Get(temp_r3_8)] + temp_r30_8 + 0x94;
        case 8:
            temp_r3_9 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_9 = temp_r3_9->unk9A;
            return (&lbl_814FE850)[Lang_Get(temp_r3_9)] + temp_r30_9 + 0x98;
        case 9:
            temp_r3_10 = (&lbl_814FE850)[Lang_Get()];
            temp_r30_10 = temp_r3_10->unk9E;
            return (&lbl_814FE850)[Lang_Get(temp_r3_10)] + temp_r30_10 + 0x9C;
        }
    } else {
        return NULL;
    }
}

/* 0x81332FB8 */
void Fx_InitCubeModel(void) {
    u16 **var_r30;
    u32 var_r29;

    lbl_814FE868 = ResGet(0x68);
    J3D_LoadModelData(&lbl_814FE868);
    var_r29 = 0U;
    var_r30 = &lbl_814FE868 + 0x38;
    do {
        var_r30->unk68 = &lbl_814FE868;
        J3D_InitModel(var_r30, 0);
        var_r29 += 1;
        var_r30 += 0x78;
    } while (var_r29 < 0x861U);
}
