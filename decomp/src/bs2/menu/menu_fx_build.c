// Non-matching: functionally equivalent decompilation
//
// Mosaic cube text ("fx") used inside Game Play, Options and Calendar,
// 0x8132D000..0x81330F30: text / row builders (glyph strings to cube targets).
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x8132D000 */
void fn_8132D000(void) {
    lbl_814DE638.unkA7B4 = (s32) lbl_814DE638.unkA798;
    lbl_814DE638.unkA7B8 = (s32) lbl_814DE638.unkA79C;
    lbl_814DE638.unkA7BC = (s32) lbl_814DE638.unkA7A0;
    lbl_814DE638.unkA7C0 = (s32) lbl_814DE638.unkA7A4;
    lbl_814DE638.unkA7C4 = (s32) lbl_814DE638.unkA7A8;
    lbl_814DE638.unkA7C8 = (s32) lbl_814DE638.unkA7AC;
    lbl_814DE638.unkA7CC = (s32) lbl_814DE638.unkA7B0;
    lbl_814DE638.unkA798 = 0;
    lbl_814DE638.unkA79C = 0x7F;
    lbl_814DE638.unkA79E = 0x7F;
    lbl_814DE638.unkA7A0 = 0x7F;
    lbl_814DE638.unkA7A2 = 0x7F;
    lbl_814DE638.unkA7A4 = 0x7F;
    lbl_814DE638.unkA7A6 = 0x7F;
    lbl_814DE638.unkA7A8 = 0x7F;
    lbl_814DE638.unkA7AA = 0x7F;
    lbl_814DE638.unkA7AC = 0x7F;
    lbl_814DE638.unkA7AE = 0x7F;
    lbl_814DE638.unkA7B0 = 0x7F;
    lbl_814DE638.unkA7B2 = 0x7F;
}

/* 0x8132D084 */
void fn_8132D084(s16 arg0, s16 arg1) {
    lbl_814DE638.unkA7B4 = (s32) lbl_814DE638.unkA798;
    lbl_814DE638.unkA7B8 = (s32) lbl_814DE638.unkA79C;
    lbl_814DE638.unkA7BC = (s32) lbl_814DE638.unkA7A0;
    lbl_814DE638.unkA7C0 = (s32) lbl_814DE638.unkA7A4;
    lbl_814DE638.unkA7C4 = (s32) lbl_814DE638.unkA7A8;
    lbl_814DE638.unkA7C8 = (s32) lbl_814DE638.unkA7AC;
    lbl_814DE638.unkA7CC = (s32) lbl_814DE638.unkA7B0;
    lbl_814DE638.unkA798 = 3;
    lbl_814DE638.unkA79A = arg0;
    lbl_814DE638.unkA79C = arg1;
    lbl_814DE638.unkA79E = 0x7F;
    lbl_814DE638.unkA7A0 = 0x7F;
    lbl_814DE638.unkA7A2 = 0x7F;
    lbl_814DE638.unkA7A4 = 0x7F;
    lbl_814DE638.unkA7A6 = 0x7F;
    lbl_814DE638.unkA7A8 = 0x7F;
    lbl_814DE638.unkA7AA = 0x7F;
    lbl_814DE638.unkA7AC = 0x7F;
    lbl_814DE638.unkA7AE = 0x7F;
    lbl_814DE638.unkA7B0 = 0x7F;
    lbl_814DE638.unkA7B2 = 0x7F;
}

/* 0x8132D10C */
void FxOption_SetState(s16 arg0, s16 arg1, s16 arg2, s16 arg3) {
    lbl_814DE638.unkA7B4 = (s32) lbl_814DE638.unkA798;
    lbl_814DE638.unkA7B8 = (s32) lbl_814DE638.unkA79C;
    lbl_814DE638.unkA7BC = (s32) lbl_814DE638.unkA7A0;
    lbl_814DE638.unkA7C0 = (s32) lbl_814DE638.unkA7A4;
    lbl_814DE638.unkA7C4 = (s32) lbl_814DE638.unkA7A8;
    lbl_814DE638.unkA7C8 = (s32) lbl_814DE638.unkA7AC;
    lbl_814DE638.unkA7CC = (s32) lbl_814DE638.unkA7B0;
    lbl_814DE638.unkA798 = 1;
    lbl_814DE638.unkA79A = 0x7F;
    lbl_814DE638.unkA79C = 0x7F;
    lbl_814DE638.unkA79E = arg0;
    lbl_814DE638.unkA7A0 = arg1;
    lbl_814DE638.unkA7A2 = arg2;
    lbl_814DE638.unkA7A4 = arg3;
    lbl_814DE638.unkA7A6 = 0x7F;
    lbl_814DE638.unkA7A8 = 0x7F;
    lbl_814DE638.unkA7AA = 0x7F;
    lbl_814DE638.unkA7AC = 0x7F;
    lbl_814DE638.unkA7AE = 0x7F;
    lbl_814DE638.unkA7B0 = 0x7F;
    lbl_814DE638.unkA7B2 = 0x7F;
}

/* 0x8132D194 */
void fn_8132D194(s16 arg0, s16 arg1, s16 arg2, s16 arg3, void *arg4) {
    lbl_814DE638.unkA7B4 = (s32) lbl_814DE638.unkA798;
    lbl_814DE638.unkA7B8 = (s32) lbl_814DE638.unkA79C;
    lbl_814DE638.unkA7BC = (s32) lbl_814DE638.unkA7A0;
    lbl_814DE638.unkA7C0 = (s32) lbl_814DE638.unkA7A4;
    lbl_814DE638.unkA7C4 = (s32) lbl_814DE638.unkA7A8;
    lbl_814DE638.unkA7C8 = (s32) lbl_814DE638.unkA7AC;
    lbl_814DE638.unkA7CC = (s32) lbl_814DE638.unkA7B0;
    lbl_814DE638.unkA798 = 2;
    lbl_814DE638.unkA79A = 0x7F;
    lbl_814DE638.unkA79C = 0x7F;
    lbl_814DE638.unkA79E = 0x7F;
    lbl_814DE638.unkA7A0 = 0x7F;
    lbl_814DE638.unkA7A2 = 0x7F;
    lbl_814DE638.unkA7A4 = 0x7F;
    lbl_814DE638.unkA7A6 = arg0;
    lbl_814DE638.unkA7A8 = arg1;
    lbl_814DE638.unkA7AA = arg2;
    lbl_814DE638.unkA7AC = arg3;
    lbl_814DE638.unkA7AE = (s16) arg4->unk0;
    lbl_814DE638.unkA7B0 = (s16) arg4->unk4;
    lbl_814DE638.unkA7B2 = (s16) arg4->unk8;
}

/* 0x8132D234 */
void fn_8132D234(void) {
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    f32 temp_f1;
    f32 temp_f1_10;
    f32 temp_f1_11;
    f32 temp_f1_12;
    f32 temp_f1_13;
    f32 temp_f1_14;
    f32 temp_f1_15;
    f32 temp_f1_16;
    f32 temp_f1_17;
    f32 temp_f1_18;
    f32 temp_f1_19;
    f32 temp_f1_20;
    f32 temp_f1_21;
    f32 temp_f1_22;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f1_7;
    f32 temp_f1_8;
    f32 temp_f1_9;
    f32 temp_f22;
    f32 temp_f23;
    f32 temp_f23_10;
    f32 temp_f23_11;
    f32 temp_f23_12;
    f32 temp_f23_2;
    f32 temp_f23_3;
    f32 temp_f23_4;
    f32 temp_f23_5;
    f32 temp_f23_6;
    f32 temp_f23_7;
    f32 temp_f23_8;
    f32 temp_f23_9;
    f32 temp_f28;
    f32 temp_f28_2;
    f32 temp_f28_3;
    f32 temp_f28_4;
    f32 temp_f28_5;
    f32 temp_f29;
    f32 temp_f29_2;
    f32 temp_f29_3;
    f32 temp_f29_4;
    f32 temp_f29_5;
    f32 temp_f29_6;
    f32 temp_f31;
    f32 temp_f31_2;
    f32 temp_f31_3;
    f32 temp_f3;
    f32 temp_f3_2;
    f32 temp_f3_3;
    f32 temp_f3_4;
    f32 temp_f3_5;
    f32 var_f22;
    s32 var_r26;
    s32 var_r26_2;
    s32 var_r26_3;
    s32 var_r26_4;
    s32 var_r26_5;
    s32 var_r26_6;
    u16 *temp_r25;
    u16 *var_r27;
    u16 *var_r27_2;
    u16 *var_r27_3;
    u16 *var_r27_4;
    u16 *var_r27_5;
    u16 *var_r28;
    u16 temp_r0;
    u16 temp_r25_10;
    u16 temp_r25_2;
    u16 temp_r25_3;
    u16 temp_r25_4;
    u16 temp_r25_5;
    u16 temp_r25_6;
    u16 temp_r25_7;
    u16 temp_r25_8;
    u16 temp_r25_9;
    void *temp_r27;
    void *temp_r28;
    void *temp_r28_2;
    void *temp_r28_3;
    void *temp_r28_4;
    void *temp_r28_5;
    void *temp_r29;
    void *temp_r29_2;
    void *temp_r29_3;
    void *temp_r29_4;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;
    void *temp_r3_4;
    void *temp_r3_5;
    void *temp_r3_6;
    void *temp_r3_7;

    sp5C = 0.0f;
    sp58 = 0.0f;
    sp54 = 0.0f;
    temp_r25 = fn_81332C34(&lbl_814DE638);
    fn_8132C830(&sp54, &lbl_814DE638 + 0xA7D0, temp_r25, fn_81332C9C());
    var_r28 = temp_r25;
    temp_r29 = &lbl_814DE638 + 0x10000;
    var_r26 = 0;
loop_2:
    if (var_r26 < (s32) fn_81332C9C()) {
        temp_f1 = (f32) Menu_Get()->unkD6;
        temp_f23 = (f32) lbl_814DE638.unkA794 / temp_f1;
        temp_r3 = temp_r29 + (*var_r28 << 5);
        temp_f1_2 = Menu_Get(temp_f1)->unk34 * temp_f23;
        temp_r3->unk-581C = (f32) (temp_r3->unk-581C + temp_f1_2);
        temp_f1_3 = (f32) Menu_Get((bitwise f32) temp_r3, temp_f1_2)->unkD6;
        temp_f23_2 = (f32) *((&lbl_814DE638 + 0x10000) - 0x586C) / temp_f1_3;
        temp_r3_2 = temp_r29 + (*var_r28 << 5);
        temp_f1_4 = Menu_Get(temp_f1_3)->unk38 * temp_f23_2;
        temp_r3_2->unk-5818 = (f32) (temp_r3_2->unk-5818 + temp_f1_4);
        temp_r27 = (temp_r29 + (*var_r28 << 5)) - 0x5830;
        temp_f29 = Menu_Get((bitwise f32) temp_r3_2, temp_f1_4)->unk3C;
        temp_f31 = Menu_Get()->unk3C;
        var_r28 += 2;
        var_r26 += 1;
        temp_f3 = Menu_Get()->unk3C;
        temp_r27->unk14 = (f32) (sp54 + (temp_f3 * (temp_r27->unk14 - sp54)));
        temp_r27->unk18 = (f32) (sp58 + (temp_f31 * (temp_r27->unk18 - sp58)));
        temp_r27->unk1C = (f32) (sp5C + (temp_f29 * (temp_r27->unk1C - sp5C)));
        temp_r27->unk0 = (f32) (temp_r27->unk0 * temp_f3);
        temp_r27->unk4 = (f32) (temp_r27->unk4 * temp_f31);
        temp_r27->unk8 = (f32) (temp_r27->unk8 * temp_f29);
        goto loop_2;
    }
    var_r27 = fn_813327B8();
    fn_8132C830(&sp54, &lbl_814DE638 + 0xA7D0, var_r27, fn_81332820());
    temp_r29_2 = &lbl_814DE638 + 0x10000;
    var_r26_2 = 0;
loop_5:
    if (var_r26_2 < (s32) fn_81332820()) {
        temp_f1_5 = (f32) Menu_Get()->unkD6;
        temp_f23_3 = (f32) lbl_814DE638.unkA794 / temp_f1_5;
        temp_r3_3 = temp_r29_2 + (*var_r27 << 5);
        temp_f1_6 = Menu_Get(temp_f1_5)->unk44 * temp_f23_3;
        temp_r3_3->unk-581C = (f32) (temp_r3_3->unk-581C + temp_f1_6);
        temp_f1_7 = (f32) Menu_Get((bitwise f32) temp_r3_3, temp_f1_6)->unkD6;
        temp_f23_4 = (f32) *((&lbl_814DE638 + 0x10000) - 0x586C) / temp_f1_7;
        temp_r3_4 = temp_r29_2 + (*var_r27 << 5);
        temp_f1_8 = Menu_Get(temp_f1_7)->unk48 * temp_f23_4;
        temp_r3_4->unk-5818 = (f32) (temp_r3_4->unk-5818 + temp_f1_8);
        temp_r28 = (temp_r29_2 + (*var_r27 << 5)) - 0x5830;
        temp_f31_2 = Menu_Get((bitwise f32) temp_r3_4, temp_f1_8)->unk4C;
        temp_f29_2 = Menu_Get()->unk4C;
        var_r27 += 2;
        var_r26_2 += 1;
        temp_f3_2 = Menu_Get()->unk4C;
        temp_r28->unk14 = (f32) (sp54 + (temp_f3_2 * (temp_r28->unk14 - sp54)));
        temp_r28->unk18 = (f32) (sp58 + (temp_f29_2 * (temp_r28->unk18 - sp58)));
        temp_r28->unk1C = (f32) (sp5C + (temp_f31_2 * (temp_r28->unk1C - sp5C)));
        temp_r28->unk0 = (f32) (temp_r28->unk0 * temp_f3_2);
        temp_r28->unk4 = (f32) (temp_r28->unk4 * temp_f29_2);
        temp_r28->unk8 = (f32) (temp_r28->unk8 * temp_f31_2);
        goto loop_5;
    }
    var_r27_2 = fn_81331008();
    fn_8132C830(&sp54, &lbl_814DE638 + 0xA7D0, var_r27_2, fn_81331070());
    temp_r29_3 = &lbl_814DE638 + 0x10000;
    var_r26_3 = 0;
    sp54 = 5.0f;
    sp58 = -135.0f;
loop_8:
    if (var_r26_3 < (s32) fn_81331070()) {
        temp_f1_9 = (f32) Menu_Get()->unkD6;
        temp_f23_5 = (f32) lbl_814DE638.unkA794 / temp_f1_9;
        temp_r3_5 = temp_r29_3 + (*var_r27_2 << 5);
        temp_f1_10 = Menu_Get(temp_f1_9)->unk54 * temp_f23_5;
        temp_r3_5->unk-581C = (f32) (temp_r3_5->unk-581C + temp_f1_10);
        temp_f1_11 = (f32) Menu_Get((bitwise f32) temp_r3_5, temp_f1_10)->unkD6;
        temp_f23_6 = (f32) *((&lbl_814DE638 + 0x10000) - 0x586C) / temp_f1_11;
        temp_r3_6 = temp_r29_3 + (*var_r27_2 << 5);
        temp_f1_12 = Menu_Get(temp_f1_11)->unk58 * temp_f23_6;
        temp_r3_6->unk-5818 = (f32) (temp_r3_6->unk-5818 + temp_f1_12);
        temp_r28_2 = (temp_r29_3 + (*var_r27_2 << 5)) - 0x5830;
        temp_f31_3 = Menu_Get((bitwise f32) temp_r3_6, temp_f1_12)->unk5C;
        temp_f29_3 = Menu_Get()->unk5C;
        var_r27_2 += 2;
        var_r26_3 += 1;
        temp_f3_3 = Menu_Get()->unk5C;
        temp_r28_2->unk14 = (f32) (sp54 + (temp_f3_3 * (temp_r28_2->unk14 - sp54)));
        temp_r28_2->unk18 = (f32) (sp58 + (temp_f29_3 * (temp_r28_2->unk18 - sp58)));
        temp_r28_2->unk1C = (f32) (sp5C + (temp_f31_3 * (temp_r28_2->unk1C - sp5C)));
        temp_r28_2->unk0 = (f32) (temp_r28_2->unk0 * temp_f3_3);
        temp_r28_2->unk4 = (f32) (temp_r28_2->unk4 * temp_f29_3);
        temp_r28_2->unk8 = (f32) (temp_r28_2->unk8 * temp_f31_3);
        goto loop_8;
    }
    var_r27_3 = fn_81332C34();
    fn_8132C830(&sp54, &lbl_814DE638 + 0xA7D0, var_r27_3, fn_81332C9C());
    var_r26_4 = 0;
loop_11:
    if (var_r26_4 < (s32) fn_81332C9C()) {
        temp_r25_2 = *var_r27_3;
        temp_r28_3 = (&lbl_814DE638 + 0x10000 + (temp_r25_2 << 5)) - 0x5830;
        temp_f1_13 = (f32) Menu_Get()->unkAA;
        temp_f23_7 = (f32) *(&lbl_814DE638 + (temp_r25_2 * 2)) / temp_f1_13;
        temp_r25_3 = *var_r27_3;
        temp_f28 = 1.0f + (temp_f23_7 * Menu_Get(temp_f1_13)->unk40);
        temp_f1_14 = (f32) Menu_Get()->unkAA;
        temp_f23_8 = (f32) *(&lbl_814DE638 + (temp_r25_3 * 2)) / temp_f1_14;
        temp_r25_4 = *var_r27_3;
        temp_f29_4 = 1.0f + (temp_f23_8 * Menu_Get(temp_f1_14)->unk40);
        temp_f1_15 = (f32) Menu_Get()->unkAA;
        temp_f23_9 = (f32) *(&lbl_814DE638 + (temp_r25_4 * 2)) / temp_f1_15;
        var_r27_3 += 2;
        var_r26_4 += 1;
        temp_f3_4 = 1.0f + (temp_f23_9 * Menu_Get(temp_f1_15)->unk40);
        temp_r28_3->unk14 = (f32) (sp54 + (temp_f3_4 * (temp_r28_3->unk14 - sp54)));
        temp_r28_3->unk18 = (f32) (sp58 + (temp_f29_4 * (temp_r28_3->unk18 - sp58)));
        temp_r28_3->unk1C = (f32) (sp5C + (temp_f28 * (temp_r28_3->unk1C - sp5C)));
        temp_r28_3->unk0 = (f32) (temp_r28_3->unk0 * temp_f3_4);
        temp_r28_3->unk4 = (f32) (temp_r28_3->unk4 * temp_f29_4);
        temp_r28_3->unk8 = (f32) (temp_r28_3->unk8 * temp_f28);
        goto loop_11;
    }
    var_r27_4 = fn_813327B8();
    fn_8132C830(&sp54, &lbl_814DE638 + 0xA7D0, var_r27_4, fn_81332820());
    var_r26_5 = 0;
loop_14:
    if (var_r26_5 < (s32) fn_81332820()) {
        temp_r25_5 = *var_r27_4;
        temp_r28_4 = (&lbl_814DE638 + 0x10000 + (temp_r25_5 << 5)) - 0x5830;
        temp_f1_16 = (f32) Menu_Get()->unkAA;
        temp_f23_10 = (f32) *(&lbl_814DE638 + (temp_r25_5 * 2)) / temp_f1_16;
        temp_r25_6 = *var_r27_4;
        temp_f29_5 = 1.0f + (temp_f23_10 * Menu_Get(temp_f1_16)->unk50);
        temp_f1_17 = (f32) Menu_Get()->unkAA;
        temp_f23_11 = (f32) *(&lbl_814DE638 + (temp_r25_6 * 2)) / temp_f1_17;
        temp_r25_7 = *var_r27_4;
        temp_f28_2 = 1.0f + (temp_f23_11 * Menu_Get(temp_f1_17)->unk50);
        temp_f1_18 = (f32) Menu_Get()->unkAA;
        temp_f23_12 = (f32) *(&lbl_814DE638 + (temp_r25_7 * 2)) / temp_f1_18;
        var_r27_4 += 2;
        var_r26_5 += 1;
        temp_f3_5 = 1.0f + (temp_f23_12 * Menu_Get(temp_f1_18)->unk50);
        temp_r28_4->unk14 = (f32) (sp54 + (temp_f3_5 * (temp_r28_4->unk14 - sp54)));
        temp_r28_4->unk18 = (f32) (sp58 + (temp_f28_2 * (temp_r28_4->unk18 - sp58)));
        temp_r28_4->unk1C = (f32) (sp5C + (temp_f29_5 * (temp_r28_4->unk1C - sp5C)));
        temp_r28_4->unk0 = (f32) (temp_r28_4->unk0 * temp_f3_5);
        temp_r28_4->unk4 = (f32) (temp_r28_4->unk4 * temp_f28_2);
        temp_r28_4->unk8 = (f32) (temp_r28_4->unk8 * temp_f29_5);
        goto loop_14;
    }
    var_r27_5 = fn_81331008();
    fn_8132C830(&sp54, &lbl_814DE638 + 0xA7D0, var_r27_5, fn_81331070());
    temp_r29_4 = &lbl_814DE638 + 0x10000;
    var_r26_6 = 0;
loop_21:
    if (var_r26_6 < (s32) fn_81331070()) {
        temp_r25_8 = *var_r27_5;
        temp_r28_5 = (temp_r29_4 + (temp_r25_8 << 5)) - 0x5830;
        temp_f1_19 = (f32) Menu_Get()->unkAA;
        temp_f28_3 = (f32) *(&lbl_814DE638 + (temp_r25_8 * 2)) / temp_f1_19;
        temp_r25_9 = *var_r27_5;
        temp_f29_6 = 1.0f + (temp_f28_3 * Menu_Get(temp_f1_19)->unk60);
        temp_f1_20 = (f32) Menu_Get()->unkAA;
        temp_f28_4 = (f32) *(&lbl_814DE638 + (temp_r25_9 * 2)) / temp_f1_20;
        temp_r25_10 = *var_r27_5;
        temp_f28_5 = 1.0f + (temp_f28_4 * Menu_Get(temp_f1_20)->unk60);
        temp_f1_21 = (f32) Menu_Get()->unkAA;
        temp_f22 = (f32) *(&lbl_814DE638 + (temp_r25_10 * 2)) / temp_f1_21;
        temp_f1_22 = 1.0f + (temp_f22 * Menu_Get(temp_f1_21)->unk60);
        temp_r28_5->unk14 = (f32) (sp54 + (temp_f1_22 * (temp_r28_5->unk14 - sp54)));
        temp_r28_5->unk18 = (f32) (sp58 + (temp_f28_5 * (temp_r28_5->unk18 - sp58)));
        temp_r28_5->unk1C = (f32) (sp5C + (temp_f29_6 * (temp_r28_5->unk1C - sp5C)));
        temp_r28_5->unk0 = (f32) (temp_r28_5->unk0 * temp_f1_22);
        temp_r28_5->unk4 = (f32) (temp_r28_5->unk4 * temp_f28_5);
        temp_r28_5->unk8 = (f32) (temp_r28_5->unk8 * temp_f29_6);
        var_f22 = (((f32) lbl_814DE638.unkA794 / (f32) Menu_Get(temp_f1_22, sp58, sp5C, sp54)->unkD6) - 0.9f) / 0.100000024f;
        if (var_f22 < 0.0f) {
            var_f22 = 0.0f;
        } else if (var_f22 > 1.0f) {
            var_f22 = 1.0f;
        }
        temp_r0 = *var_r27_5;
        var_r27_5 += 2;
        temp_r3_7 = temp_r29_4 + (temp_r0 << 5);
        var_r26_6 += 1;
        temp_r3_7->unk-581C = (f32) (temp_r3_7->unk-581C + (f32) ((f64) var_f22 * (-271.0 + (0.9155405405405406 * fn_8132DE98()))));
        goto loop_21;
    }
}

/* 0x8132DD84 */
void FxOption_ChangeLang(s16 arg0, s16 arg1) {
    s32 var_r30;

    var_r30 = saved_reg_r30;
    switch (arg1) {                                 /* irregular */
    case 0:
        var_r30 = Menu_Get()->unk64;
        break;
    case 1:
        var_r30 = Menu_Get()->unk68;
        break;
    case 2:
        var_r30 = Menu_Get()->unk6C;
        break;
    case 3:
        var_r30 = Menu_Get()->unk70;
        break;
    case 4:
        var_r30 = Menu_Get()->unk74;
        break;
    case 5:
        var_r30 = Menu_Get()->unk78;
        break;
    }
    lbl_814DE638.unk201F4 = fn_8132DE98();
    lbl_814DE638.unk201F8 = (f32) (var_r30 + Option_GetLangRow());
    lbl_814DE638.unk201F0 = 0;
    fn_813306FC(arg0, arg1);
}

/* 0x8132DE94 */
void FxOption_ChangeUiLang(void) {

}

/* 0x8132DE98 */
void fn_8132DE98(void) {
    Hermite(&lbl_814DE638 + 0x20000, (f32) lbl_814DE638.unk201F0, 0.0f, lbl_814DE638.unk201F4, 0.0f, 10.0f, lbl_814DE638.unk201F8, 0.0f);
}

/* 0x8132DEFC */
void fn_8132DEFC(void) {
    f32 sp88;
    f32 temp_f1;
    f32 temp_f1_10;
    f32 temp_f1_11;
    f32 temp_f1_12;
    f32 temp_f1_13;
    f32 temp_f1_14;
    f32 temp_f1_15;
    f32 temp_f1_16;
    f32 temp_f1_17;
    f32 temp_f1_18;
    f32 temp_f1_19;
    f32 temp_f1_20;
    f32 temp_f1_21;
    f32 temp_f1_22;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f1_7;
    f32 temp_f1_8;
    f32 temp_f1_9;
    f32 temp_f27;
    f32 temp_f27_10;
    f32 temp_f27_11;
    f32 temp_f27_12;
    f32 temp_f27_13;
    f32 temp_f27_14;
    f32 temp_f27_15;
    f32 temp_f27_16;
    f32 temp_f27_2;
    f32 temp_f27_3;
    f32 temp_f27_4;
    f32 temp_f27_5;
    f32 temp_f27_6;
    f32 temp_f27_7;
    f32 temp_f27_8;
    f32 temp_f27_9;
    f32 temp_f28;
    f32 temp_f28_2;
    f32 temp_f28_3;
    f32 temp_f28_4;
    f32 temp_f28_5;
    f32 temp_f28_6;
    f32 temp_f28_7;
    f32 temp_f29;
    f32 temp_f29_2;
    f32 temp_f29_3;
    f32 temp_f29_4;
    f32 temp_f29_5;
    f32 temp_f29_6;
    f32 temp_f29_7;
    f32 temp_f30;
    f32 temp_f30_2;
    f32 temp_f31;
    f32 temp_f31_2;
    f32 temp_f3;
    f32 temp_f3_2;
    f32 temp_f3_3;
    f32 temp_f3_4;
    f32 temp_f3_5;
    f32 temp_f3_6;
    f32 temp_f3_7;
    f32 temp_f3_8;
    s32 var_r26;
    s32 var_r26_2;
    s32 var_r26_3;
    s32 var_r26_4;
    s32 var_r26_5;
    s32 var_r26_6;
    s32 var_r28;
    s32 var_r28_2;
    u16 *var_r25;
    u16 *var_r25_2;
    u16 *var_r27;
    u16 *var_r27_2;
    u16 *var_r27_3;
    u16 *var_r27_4;
    u16 *var_r27_5;
    u16 *var_r27_6;
    u16 temp_r25;
    u16 temp_r25_10;
    u16 temp_r25_11;
    u16 temp_r25_12;
    u16 temp_r25_13;
    u16 temp_r25_14;
    u16 temp_r25_15;
    u16 temp_r25_16;
    u16 temp_r25_17;
    u16 temp_r25_18;
    u16 temp_r25_2;
    u16 temp_r25_3;
    u16 temp_r25_4;
    u16 temp_r25_5;
    u16 temp_r25_6;
    u16 temp_r25_7;
    u16 temp_r25_8;
    u16 temp_r25_9;
    void *temp_r26;
    void *temp_r26_2;
    void *temp_r27;
    void *temp_r27_2;
    void *temp_r28;
    void *temp_r28_2;
    void *temp_r28_3;
    void *temp_r28_4;
    void *temp_r28_5;
    void *temp_r28_6;
    void *temp_r30;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;
    void *temp_r3_4;

    var_r25 = fn_813315C8(&lbl_814DE638);
    fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r25, fn_81331630());
    temp_r26 = &lbl_814DE638 + 0x10000;
    var_r28 = 0;
loop_2:
    if (var_r28 < (s32) fn_81331630()) {
        temp_f1 = Menu_Get()->unk7C;
        temp_r3 = temp_r26 + (*var_r25 << 5);
        temp_r3->unk-581C = (f32) (temp_r3->unk-581C + temp_f1);
        temp_f1_2 = Menu_Get((bitwise f32) temp_r3, temp_f1)->unk80;
        temp_r3_2 = temp_r26 + (*var_r25 << 5);
        temp_r3_2->unk-5818 = (f32) (temp_r3_2->unk-5818 + temp_f1_2);
        temp_r27 = (temp_r26 + (*var_r25 << 5)) - 0x5830;
        temp_f31 = Menu_Get((bitwise f32) temp_r3_2, temp_f1_2)->unk84;
        temp_f30 = Menu_Get()->unk84;
        var_r25 += 2;
        var_r28 += 1;
        temp_f3 = Menu_Get()->unk84;
        temp_r27->unk14 = (f32) (sp88 + (temp_f3 * (temp_r27->unk14 - sp88)));
        temp_r27->unk18 = (f32) (sp8C + (temp_f30 * (temp_r27->unk18 - sp8C)));
        temp_r27->unk1C = (f32) (sp90 + (temp_f31 * (temp_r27->unk1C - sp90)));
        temp_r27->unk0 = (f32) (temp_r27->unk0 * temp_f3);
        temp_r27->unk4 = (f32) (temp_r27->unk4 * temp_f30);
        temp_r27->unk8 = (f32) (temp_r27->unk8 * temp_f31);
        goto loop_2;
    }
    var_r25_2 = fn_813311F4();
    fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r25_2, fn_81331258());
    temp_r26_2 = &lbl_814DE638 + 0x10000;
    var_r28_2 = 0;
loop_5:
    if (var_r28_2 < (s32) fn_81331258()) {
        temp_f1_3 = Menu_Get()->unk8C;
        temp_r3_3 = temp_r26_2 + (*var_r25_2 << 5);
        temp_r3_3->unk-581C = (f32) (temp_r3_3->unk-581C + temp_f1_3);
        temp_f1_4 = Menu_Get((bitwise f32) temp_r3_3, temp_f1_3)->unk90;
        temp_r3_4 = temp_r26_2 + (*var_r25_2 << 5);
        temp_r3_4->unk-5818 = (f32) (temp_r3_4->unk-5818 + temp_f1_4);
        temp_r27_2 = (temp_r26_2 + (*var_r25_2 << 5)) - 0x5830;
        temp_f31_2 = Menu_Get((bitwise f32) temp_r3_4, temp_f1_4)->unk94;
        temp_f30_2 = Menu_Get()->unk94;
        var_r25_2 += 2;
        var_r28_2 += 1;
        temp_f3_2 = Menu_Get()->unk94;
        temp_r27_2->unk14 = (f32) (sp88 + (temp_f3_2 * (temp_r27_2->unk14 - sp88)));
        temp_r27_2->unk18 = (f32) (sp8C + (temp_f30_2 * (temp_r27_2->unk18 - sp8C)));
        temp_r27_2->unk1C = (f32) (sp90 + (temp_f31_2 * (temp_r27_2->unk1C - sp90)));
        temp_r27_2->unk0 = (f32) (temp_r27_2->unk0 * temp_f3_2);
        temp_r27_2->unk4 = (f32) (temp_r27_2->unk4 * temp_f30_2);
        temp_r27_2->unk8 = (f32) (temp_r27_2->unk8 * temp_f31_2);
        goto loop_5;
    }
    temp_r30 = &lbl_814DE638 + 0x10000;
    switch ((s16) lbl_814DE638.unkA7A6) {           /* switch 1; irregular */
    case 1:                                         /* switch 1 */
        switch ((s16) lbl_814DE638.unkA7AA) {       /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            var_r27 = fn_81332008();
            fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r27, fn_81332070());
            var_r26 = 0;
loop_15:
            if (var_r26 < (s32) fn_81332070()) {
                temp_r25 = *var_r27;
                temp_r28 = (temp_r30 + (temp_r25 << 5)) - 0x5830;
                temp_f1_5 = (f32) Menu_Get()->unkAA;
                temp_f28 = (f32) *(&lbl_814DE638 + (temp_r25 * 2)) / temp_f1_5;
                temp_r25_2 = *var_r27;
                temp_f28_2 = 1.0f + (temp_f28 * Menu_Get(temp_f1_5)->unk88);
                temp_f1_6 = (f32) Menu_Get()->unkAA;
                temp_f29 = (f32) *(&lbl_814DE638 + (temp_r25_2 * 2)) / temp_f1_6;
                temp_r25_3 = *var_r27;
                temp_f29_2 = 1.0f + (temp_f29 * Menu_Get(temp_f1_6)->unk88);
                temp_f1_7 = (f32) Menu_Get()->unkAA;
                temp_f27 = (f32) *(&lbl_814DE638 + (temp_r25_3 * 2)) / temp_f1_7;
                var_r27 += 2;
                var_r26 += 1;
                temp_f3_3 = 1.0f + (temp_f27 * Menu_Get(temp_f1_7)->unk88);
                temp_r28->unk14 = (f32) (sp88 + (temp_f3_3 * (temp_r28->unk14 - sp88)));
                temp_r28->unk18 = (f32) (sp8C + (temp_f29_2 * (temp_r28->unk18 - sp8C)));
                temp_r28->unk1C = (f32) (sp90 + (temp_f28_2 * (temp_r28->unk1C - sp90)));
                temp_r28->unk0 = (f32) (temp_r28->unk0 * temp_f3_3);
                temp_r28->unk4 = (f32) (temp_r28->unk4 * temp_f29_2);
                temp_r28->unk8 = (f32) (temp_r28->unk8 * temp_f28_2);
                goto loop_15;
            }
            return;
        case 1:                                     /* switch 2 */
            var_r27_2 = fn_81331C30();
            fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r27_2, fn_81331C98());
            var_r26_2 = 0;
loop_19:
            if (var_r26_2 < (s32) fn_81331C98()) {
                temp_r25_4 = *var_r27_2;
                temp_r28_2 = (temp_r30 + (temp_r25_4 << 5)) - 0x5830;
                temp_f1_8 = (f32) Menu_Get()->unkAA;
                temp_f27_2 = (f32) *(&lbl_814DE638 + (temp_r25_4 * 2)) / temp_f1_8;
                temp_r25_5 = *var_r27_2;
                temp_f29_3 = 1.0f + (temp_f27_2 * Menu_Get(temp_f1_8)->unk88);
                temp_f1_9 = (f32) Menu_Get()->unkAA;
                temp_f27_3 = (f32) *(&lbl_814DE638 + (temp_r25_5 * 2)) / temp_f1_9;
                temp_r25_6 = *var_r27_2;
                temp_f28_3 = 1.0f + (temp_f27_3 * Menu_Get(temp_f1_9)->unk88);
                temp_f1_10 = (f32) Menu_Get()->unkAA;
                temp_f27_4 = (f32) *(&lbl_814DE638 + (temp_r25_6 * 2)) / temp_f1_10;
                var_r27_2 += 2;
                var_r26_2 += 1;
                temp_f3_4 = 1.0f + (temp_f27_4 * Menu_Get(temp_f1_10)->unk88);
                temp_r28_2->unk14 = (f32) (sp88 + (temp_f3_4 * (temp_r28_2->unk14 - sp88)));
                temp_r28_2->unk18 = (f32) (sp8C + (temp_f28_3 * (temp_r28_2->unk18 - sp8C)));
                temp_r28_2->unk1C = (f32) (sp90 + (temp_f29_3 * (temp_r28_2->unk1C - sp90)));
                temp_r28_2->unk0 = (f32) (temp_r28_2->unk0 * temp_f3_4);
                temp_r28_2->unk4 = (f32) (temp_r28_2->unk4 * temp_f28_3);
                temp_r28_2->unk8 = (f32) (temp_r28_2->unk8 * temp_f29_3);
                goto loop_19;
            }
            return;
        case 2:                                     /* switch 2 */
            var_r27_3 = fn_8133166C();
            fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r27_3, fn_813316D4());
            var_r26_3 = 0;
loop_23:
            if (var_r26_3 < (s32) fn_813316D4()) {
                temp_r25_7 = *var_r27_3;
                temp_r28_3 = (temp_r30 + (temp_r25_7 << 5)) - 0x5830;
                temp_f1_11 = (f32) Menu_Get()->unkAA;
                temp_f27_5 = (f32) *(&lbl_814DE638 + (temp_r25_7 * 2)) / temp_f1_11;
                temp_r25_8 = *var_r27_3;
                temp_f29_4 = 1.0f + (temp_f27_5 * Menu_Get(temp_f1_11)->unk88);
                temp_f1_12 = (f32) Menu_Get()->unkAA;
                temp_f27_6 = (f32) *(&lbl_814DE638 + (temp_r25_8 * 2)) / temp_f1_12;
                temp_r25_9 = *var_r27_3;
                temp_f28_4 = 1.0f + (temp_f27_6 * Menu_Get(temp_f1_12)->unk88);
                temp_f1_13 = (f32) Menu_Get()->unkAA;
                temp_f27_7 = (f32) *(&lbl_814DE638 + (temp_r25_9 * 2)) / temp_f1_13;
                var_r27_3 += 2;
                var_r26_3 += 1;
                temp_f3_5 = 1.0f + (temp_f27_7 * Menu_Get(temp_f1_13)->unk88);
                temp_r28_3->unk14 = (f32) (sp88 + (temp_f3_5 * (temp_r28_3->unk14 - sp88)));
                temp_r28_3->unk18 = (f32) (sp8C + (temp_f28_4 * (temp_r28_3->unk18 - sp8C)));
                temp_r28_3->unk1C = (f32) (sp90 + (temp_f29_4 * (temp_r28_3->unk1C - sp90)));
                temp_r28_3->unk0 = (f32) (temp_r28_3->unk0 * temp_f3_5);
                temp_r28_3->unk4 = (f32) (temp_r28_3->unk4 * temp_f28_4);
                temp_r28_3->unk8 = (f32) (temp_r28_3->unk8 * temp_f29_4);
                goto loop_23;
            }
            return;
        }
        break;
    case 2:                                         /* switch 1 */
        switch ((s16) lbl_814DE638.unkA7AC) {       /* switch 3; irregular */
        case 0:                                     /* switch 3 */
            var_r27_4 = fn_81331858();
            fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r27_4, fn_813318C0());
            var_r26_4 = 0;
loop_34:
            if (var_r26_4 < (s32) fn_813318C0()) {
                temp_r25_10 = *var_r27_4;
                temp_r28_4 = (temp_r30 + (temp_r25_10 << 5)) - 0x5830;
                temp_f1_14 = (f32) Menu_Get()->unkAA;
                temp_f27_8 = (f32) *(&lbl_814DE638 + (temp_r25_10 * 2)) / temp_f1_14;
                temp_r25_11 = *var_r27_4;
                temp_f29_5 = 1.0f + (temp_f27_8 * Menu_Get(temp_f1_14)->unk98);
                temp_f1_15 = (f32) Menu_Get()->unkAA;
                temp_f27_9 = (f32) *(&lbl_814DE638 + (temp_r25_11 * 2)) / temp_f1_15;
                temp_r25_12 = *var_r27_4;
                temp_f28_5 = 1.0f + (temp_f27_9 * Menu_Get(temp_f1_15)->unk98);
                temp_f1_16 = (f32) Menu_Get()->unkAA;
                temp_f27_10 = (f32) *(&lbl_814DE638 + (temp_r25_12 * 2)) / temp_f1_16;
                var_r27_4 += 2;
                var_r26_4 += 1;
                temp_f3_6 = 1.0f + (temp_f27_10 * Menu_Get(temp_f1_16)->unk98);
                temp_r28_4->unk14 = (f32) (sp88 + (temp_f3_6 * (temp_r28_4->unk14 - sp88)));
                temp_r28_4->unk18 = (f32) (sp8C + (temp_f28_5 * (temp_r28_4->unk18 - sp8C)));
                temp_r28_4->unk1C = (f32) (sp90 + (temp_f29_5 * (temp_r28_4->unk1C - sp90)));
                temp_r28_4->unk0 = (f32) (temp_r28_4->unk0 * temp_f3_6);
                temp_r28_4->unk4 = (f32) (temp_r28_4->unk4 * temp_f28_5);
                temp_r28_4->unk8 = (f32) (temp_r28_4->unk8 * temp_f29_5);
                goto loop_34;
            }
            return;
        case 1:                                     /* switch 3 */
            var_r27_5 = fn_81331A44();
            fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r27_5, fn_81331AAC());
            var_r26_5 = 0;
loop_38:
            if (var_r26_5 < (s32) fn_81331AAC()) {
                temp_r25_13 = *var_r27_5;
                temp_r28_5 = (temp_r30 + (temp_r25_13 << 5)) - 0x5830;
                temp_f1_17 = (f32) Menu_Get()->unkAA;
                temp_f27_11 = (f32) *(&lbl_814DE638 + (temp_r25_13 * 2)) / temp_f1_17;
                temp_r25_14 = *var_r27_5;
                temp_f29_6 = 1.0f + (temp_f27_11 * Menu_Get(temp_f1_17)->unk98);
                temp_f1_18 = (f32) Menu_Get()->unkAA;
                temp_f27_12 = (f32) *(&lbl_814DE638 + (temp_r25_14 * 2)) / temp_f1_18;
                temp_r25_15 = *var_r27_5;
                temp_f28_6 = 1.0f + (temp_f27_12 * Menu_Get(temp_f1_18)->unk98);
                temp_f1_19 = (f32) Menu_Get()->unkAA;
                temp_f27_13 = (f32) *(&lbl_814DE638 + (temp_r25_15 * 2)) / temp_f1_19;
                var_r27_5 += 2;
                var_r26_5 += 1;
                temp_f3_7 = 1.0f + (temp_f27_13 * Menu_Get(temp_f1_19)->unk98);
                temp_r28_5->unk14 = (f32) (sp88 + (temp_f3_7 * (temp_r28_5->unk14 - sp88)));
                temp_r28_5->unk18 = (f32) (sp8C + (temp_f28_6 * (temp_r28_5->unk18 - sp8C)));
                temp_r28_5->unk1C = (f32) (sp90 + (temp_f29_6 * (temp_r28_5->unk1C - sp90)));
                temp_r28_5->unk0 = (f32) (temp_r28_5->unk0 * temp_f3_7);
                temp_r28_5->unk4 = (f32) (temp_r28_5->unk4 * temp_f28_6);
                temp_r28_5->unk8 = (f32) (temp_r28_5->unk8 * temp_f29_6);
                goto loop_38;
            }
            return;
        case 2:                                     /* switch 3 */
            var_r27_6 = fn_81331E1C();
            fn_8132C830(&sp88, &lbl_814DE638 + 0xA7D0, var_r27_6, fn_81331E84());
            var_r26_6 = 0;
loop_42:
            if (var_r26_6 < (s32) fn_81331E84()) {
                temp_r25_16 = *var_r27_6;
                temp_r28_6 = (temp_r30 + (temp_r25_16 << 5)) - 0x5830;
                temp_f1_20 = (f32) Menu_Get()->unkAA;
                temp_f27_14 = (f32) *(&lbl_814DE638 + (temp_r25_16 * 2)) / temp_f1_20;
                temp_r25_17 = *var_r27_6;
                temp_f29_7 = 1.0f + (temp_f27_14 * Menu_Get(temp_f1_20)->unk98);
                temp_f1_21 = (f32) Menu_Get()->unkAA;
                temp_f27_15 = (f32) *(&lbl_814DE638 + (temp_r25_17 * 2)) / temp_f1_21;
                temp_r25_18 = *var_r27_6;
                temp_f28_7 = 1.0f + (temp_f27_15 * Menu_Get(temp_f1_21)->unk98);
                temp_f1_22 = (f32) Menu_Get()->unkAA;
                temp_f27_16 = (f32) *(&lbl_814DE638 + (temp_r25_18 * 2)) / temp_f1_22;
                var_r27_6 += 2;
                var_r26_6 += 1;
                temp_f3_8 = 1.0f + (temp_f27_16 * Menu_Get(temp_f1_22)->unk98);
                temp_r28_6->unk14 = (f32) (sp88 + (temp_f3_8 * (temp_r28_6->unk14 - sp88)));
                temp_r28_6->unk18 = (f32) (sp8C + (temp_f28_7 * (temp_r28_6->unk18 - sp8C)));
                temp_r28_6->unk1C = (f32) (sp90 + (temp_f29_7 * (temp_r28_6->unk1C - sp90)));
                temp_r28_6->unk0 = (f32) (temp_r28_6->unk0 * temp_f3_8);
                temp_r28_6->unk4 = (f32) (temp_r28_6->unk4 * temp_f28_7);
                temp_r28_6->unk8 = (f32) (temp_r28_6->unk8 * temp_f29_7);
                goto loop_42;
            }
            break;
        }
        break;
    }
}

/* 0x8132EC54 */
void fn_8132EC54(void) {
    f32 sp14;
    void *var_r31;
    f32 temp_f30;
    f32 var_f29;
    f32 var_f31;
    s16 temp_r3;
    u32 temp_r0;
    u32 var_ctr;
    u32 var_ctr_2;
    u32 var_r3;

    var_f29 = saved_reg_f29;
    var_f31 = saved_reg_f31;
    temp_f30 = Menu_Get()->unkA0;
    temp_r3 = Option_GetLang();
    switch (temp_r3) {                              /* irregular */
    default:
    case 0:
        var_f31 = Menu_Get()->unk9C;
        var_f29 = Menu_Get()->unkA4;
        break;
    case 1:
        var_f31 = -36.0f;
        var_f29 = 0.65f;
        break;
    case 2:
        var_f31 = -36.0f;
        var_f29 = 0.6f;
        break;
    }
    var_r31 = &lbl_814E8E08;
    fn_8132C830(&sp14, &lbl_814E8E08, NULL, (u16) lbl_814AD5F0);
    var_r3 = lbl_814AD5F0;
    if (var_r3 > 0U) {
        temp_r0 = var_r3 >> 1U;
        var_ctr = temp_r0;
        if (temp_r0 != 0) {
            do {
                var_r31->unk14 = (f32) (var_r31->unk14 + var_f31);
                var_r31->unk18 = (f32) (var_r31->unk18 + temp_f30);
                var_r31->unk14 = (f32) (sp14 + (var_f29 * (var_r31->unk14 - sp14)));
                var_r31->unk18 = (f32) (sp18 + (var_f29 * (var_r31->unk18 - sp18)));
                var_r31->unk1C = (f32) (sp1C + (var_f29 * (var_r31->unk1C - sp1C)));
                var_r31->unk0 = (f32) (var_r31->unk0 * var_f29);
                var_r31->unk4 = (f32) (var_r31->unk4 * var_f29);
                var_r31->unk8 = (f32) (var_r31->unk8 * var_f29);
                var_r31->unk34 = (f32) (var_r31->unk34 + var_f31);
                var_r31->unk38 = (f32) (var_r31->unk38 + temp_f30);
                var_r31->unk34 = (f32) (sp14 + (var_f29 * (var_r31->unk34 - sp14)));
                var_r31->unk38 = (f32) (sp18 + (var_f29 * (var_r31->unk38 - sp18)));
                var_r31->unk3C = (f32) (sp1C + (var_f29 * (var_r31->unk3C - sp1C)));
                var_r31->unk20 = (f32) (var_r31->unk20 * var_f29);
                var_r31->unk24 = (f32) (var_r31->unk24 * var_f29);
                var_r31->unk28 = (f32) (var_r31->unk28 * var_f29);
                var_r31 += 0x40;
                var_ctr -= 1;
            } while (var_ctr != 0);
            var_r3 &= 1;
            if (var_r3 != 0) {
                goto block_13;
            }
        } else {
block_13:
            var_ctr_2 = var_r3;
            do {
                var_r31->unk14 = (f32) (var_r31->unk14 + var_f31);
                var_r31->unk18 = (f32) (var_r31->unk18 + temp_f30);
                var_r31->unk14 = (f32) (sp14 + (var_f29 * (var_r31->unk14 - sp14)));
                var_r31->unk18 = (f32) (sp18 + (var_f29 * (var_r31->unk18 - sp18)));
                var_r31->unk1C = (f32) (sp1C + (var_f29 * (var_r31->unk1C - sp1C)));
                var_r31->unk0 = (f32) (var_r31->unk0 * var_f29);
                var_r31->unk4 = (f32) (var_r31->unk4 * var_f29);
                var_r31->unk8 = (f32) (var_r31->unk8 * var_f29);
                var_r31 += 0x20;
                var_ctr_2 -= 1;
            } while (var_ctr_2 != 0);
        }
    }
}

/* 0x8132EEB0 */
void fn_8132EEB0(s32 arg0, s32 arg1) {
    u16 *sp30;
    u16 *sp2C;
    u16 *sp28;
    u16 *sp24;
    u16 *sp20;
    u16 *sp1C;
    u16 sp1A;
    u16 sp18;
    u16 sp16;
    u16 sp14;
    u16 sp12;
    u16 sp10;
    s16 temp_r3;
    s32 temp_r4;
    u16 *var_r30;
    u16 temp_r26;
    u16 temp_r26_2;
    u16 temp_r26_3;
    u16 temp_r26_4;
    u16 temp_r29;
    u16 var_r28;

    sp1C = fn_813329A4(&lbl_814DE638);
    sp10 = fn_81332A0C();
    sp20 = fn_81332A48();
    sp12 = fn_81332AB0();
    sp24 = fn_81332AEC();
    sp14 = fn_81332B54();
    sp28 = fn_81332B90();
    sp16 = fn_81332BF8();
    sp2C = fn_813310AC();
    sp18 = fn_81331114();
    sp30 = fn_81331150();
    sp1A = fn_813311B8();
    var_r30 = (&sp1C)[arg0];
    temp_r29 = (&sp10)[arg0];
    var_r28 = 0;
loop_8:
    if (var_r28 < temp_r29) {
        temp_r26 = *var_r30;
        if (arg1 != 0) {
            *(&lbl_814DE638 + ((temp_r26 * 2) + 0x53CA)) = Menu_Get()->unkA8;
        } else {
            *(&lbl_814DE638 + ((temp_r26 * 2) + 0x53CA)) = 0;
        }
        temp_r26_2 = *var_r30;
        if (arg1 != 0) {
            *(&lbl_814DE638 + ((temp_r26_2 * 2) + 0x3246)) = Menu_Get()->unkA8;
        } else {
            *(&lbl_814DE638 + ((temp_r26_2 * 2) + 0x3246)) = 0;
        }
        temp_r26_3 = *var_r30;
        temp_r4 = temp_r26_3 * 2;
        temp_r3 = Menu_Get()->unkA8;
        *(&lbl_814DE638 + (temp_r4 + 0x754E)) = temp_r3;
        temp_r26_4 = *var_r30;
        *(&lbl_814DE638 + ((temp_r26_4 * 2) + 0x10C2)) = Menu_Get((bitwise f32) temp_r3, (bitwise f32) temp_r4)->unkAA;
        var_r30 += 2;
        var_r28 += 1;
        goto loop_8;
    }
}

/* 0x8132F010 */
void fn_8132F010(void) {
    f32 sp54;
    f32 sp50;
    f32 sp48;
    f32 sp44;
    f32 sp3C;
    f32 sp38;
    f32 temp_f1;
    f32 temp_f1_10;
    f32 temp_f1_11;
    f32 temp_f1_12;
    f32 temp_f1_13;
    f32 temp_f1_14;
    f32 temp_f1_15;
    f32 temp_f1_16;
    f32 temp_f1_17;
    f32 temp_f1_18;
    f32 temp_f1_19;
    f32 temp_f1_20;
    f32 temp_f1_21;
    f32 temp_f1_22;
    f32 temp_f1_23;
    f32 temp_f1_24;
    f32 temp_f1_25;
    f32 temp_f1_26;
    f32 temp_f1_27;
    f32 temp_f1_28;
    f32 temp_f1_29;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f1_7;
    f32 temp_f1_8;
    f32 temp_f1_9;
    f32 temp_f28;
    f32 temp_f29;
    f32 temp_f29_10;
    f32 temp_f29_11;
    f32 temp_f29_12;
    f32 temp_f29_13;
    f32 temp_f29_14;
    f32 temp_f29_15;
    f32 temp_f29_16;
    f32 temp_f29_2;
    f32 temp_f29_3;
    f32 temp_f29_4;
    f32 temp_f29_5;
    f32 temp_f29_6;
    f32 temp_f29_7;
    f32 temp_f29_8;
    f32 temp_f29_9;
    f32 temp_f30;
    f32 temp_f30_10;
    f32 temp_f30_11;
    f32 temp_f30_12;
    f32 temp_f30_2;
    f32 temp_f30_3;
    f32 temp_f30_4;
    f32 temp_f30_5;
    f32 temp_f30_6;
    f32 temp_f30_7;
    f32 temp_f30_8;
    f32 temp_f30_9;
    f32 temp_f31;
    f32 temp_f3;
    f32 temp_f3_10;
    f32 temp_f3_11;
    f32 temp_f3_12;
    f32 temp_f3_2;
    f32 temp_f3_3;
    f32 temp_f3_4;
    f32 temp_f3_5;
    f32 temp_f3_6;
    f32 temp_f3_7;
    f32 temp_f3_8;
    f32 temp_f3_9;
    s32 var_r26;
    s32 var_r26_2;
    s32 var_r26_3;
    s32 var_r26_4;
    s32 var_r26_5;
    s32 var_r26_6;
    s32 var_r26_7;
    s32 var_r28;
    s32 var_r29_3;
    s32 var_r29_5;
    s32 var_r29_7;
    s32 var_r29_9;
    u16 *temp_r26;
    u16 *temp_r27;
    u16 *temp_r27_2;
    u16 *temp_r3;
    u16 *var_r27;
    u16 *var_r27_2;
    u16 *var_r27_3;
    u16 *var_r29;
    u16 *var_r29_2;
    u16 *var_r29_4;
    u16 *var_r29_6;
    u16 *var_r29_8;
    u16 *var_r30;
    u16 *var_r30_2;
    u16 *var_r30_3;
    u16 *var_r30_4;
    u16 temp_r0;
    u16 temp_r0_2;
    u16 temp_r0_3;
    u16 temp_r0_4;
    u16 temp_r0_5;
    u16 temp_r0_6;
    void *temp_r27_10;
    void *temp_r27_11;
    void *temp_r27_3;
    void *temp_r27_4;
    void *temp_r27_5;
    void *temp_r27_6;
    void *temp_r27_7;
    void *temp_r27_8;
    void *temp_r27_9;
    void *temp_r28;
    void *temp_r28_10;
    void *temp_r28_11;
    void *temp_r28_2;
    void *temp_r28_3;
    void *temp_r28_4;
    void *temp_r28_5;
    void *temp_r28_6;
    void *temp_r28_7;
    void *temp_r28_8;
    void *temp_r28_9;
    void *temp_r29;
    void *temp_r29_2;
    void *temp_r30;
    void *temp_r30_2;
    void *temp_r3_10;
    void *temp_r3_11;
    void *temp_r3_12;
    void *temp_r3_13;
    void *temp_r3_14;
    void *temp_r3_15;
    void *temp_r3_16;
    void *temp_r3_17;
    void *temp_r3_18;
    void *temp_r3_19;
    void *temp_r3_20;
    void *temp_r3_21;
    void *temp_r3_22;
    void *temp_r3_23;
    void *temp_r3_24;
    void *temp_r3_25;
    void *temp_r3_26;
    void *temp_r3_27;
    void *temp_r3_28;
    void *temp_r3_29;
    void *temp_r3_2;
    void *temp_r3_30;
    void *temp_r3_31;
    void *temp_r3_32;
    void *temp_r3_3;
    void *temp_r3_4;
    void *temp_r3_5;
    void *temp_r3_6;
    void *temp_r3_7;
    void *temp_r3_8;
    void *temp_r3_9;

    temp_r26 = fn_81332C34(&lbl_814DE638);
    fn_8132C830(&sp50, &lbl_814DE638 + 0xA7D0, temp_r26, fn_81332C9C());
    temp_f1 = Menu_Get()->unk34;
    sp50 += temp_f1;
    temp_f1_2 = Menu_Get(temp_f1)->unk38;
    sp54 += temp_f1_2;
    temp_r27 = fn_813327B8(temp_f1_2);
    fn_8132C830(&sp44, &lbl_814DE638 + 0xA7D0, temp_r27, fn_81332820());
    temp_f1_3 = Menu_Get()->unk44;
    sp44 += temp_f1_3;
    temp_f1_4 = Menu_Get(temp_f1_3)->unk48;
    sp48 += temp_f1_4;
    temp_r27_2 = fn_81331008(temp_f1_4);
    fn_8132C830(&sp38, &lbl_814DE638 + 0xA7D0, temp_r27_2, fn_81331070());
    temp_f1_5 = Menu_Get()->unk54;
    sp38 += temp_f1_5;
    temp_f1_6 = Menu_Get(temp_f1_5)->unk58;
    sp3C += temp_f1_6;
    temp_r3 = fn_813329A4((bitwise void *) temp_f1_6);
    var_r27 = temp_r3;
    temp_r28 = &lbl_814DE638 + 0x10000;
    var_r26 = 0;
loop_2:
    if (var_r26 < (s32) fn_81332A0C()) {
        temp_f1_7 = Menu_Get()->unk34;
        temp_r3_2 = temp_r28 + (*var_r27 << 5);
        temp_r3_2->unk-581C = (f32) (temp_r3_2->unk-581C + temp_f1_7);
        temp_f1_8 = Menu_Get((bitwise f32) temp_r3_2, temp_f1_7)->unk38;
        temp_r3_3 = temp_r28 + (*var_r27 << 5);
        temp_r3_3->unk-5818 = (f32) (temp_r3_3->unk-5818 + temp_f1_8);
        temp_r29 = (temp_r28 + (*var_r27 << 5)) - 0x5830;
        temp_f30 = Menu_Get((bitwise f32) temp_r3_3, temp_f1_8)->unk3C;
        temp_f31 = Menu_Get()->unk3C;
        var_r27 += 2;
        var_r26 += 1;
        temp_f3 = Menu_Get()->unk3C;
        temp_r29->unk14 = (f32) (sp50 + (temp_f3 * (temp_r29->unk14 - sp50)));
        temp_r29->unk18 = (f32) (sp54 + (temp_f31 * (temp_r29->unk18 - sp54)));
        temp_r29->unk1C = (f32) (sp58 + (temp_f30 * (temp_r29->unk1C - sp58)));
        temp_r29->unk0 = (f32) (temp_r29->unk0 * temp_f3);
        temp_r29->unk4 = (f32) (temp_r29->unk4 * temp_f31);
        temp_r29->unk8 = (f32) (temp_r29->unk8 * temp_f30);
        goto loop_2;
    }
    var_r27_2 = temp_r3;
    temp_r28_2 = &lbl_814DE638 + 0x10000;
    var_r26_2 = 0;
loop_5:
    if (var_r26_2 < (s32) fn_81332A0C()) {
        temp_r29_2 = (temp_r28_2 + (*var_r27_2 << 5)) - 0x5830;
        temp_f29 = 1.0f + Menu_Get()->unk40;
        temp_f30_2 = 1.0f + Menu_Get()->unk40;
        temp_f3_2 = 1.0f + Menu_Get()->unk40;
        temp_r29_2->unk14 = (f32) (sp50 + (temp_f3_2 * (temp_r29_2->unk14 - sp50)));
        temp_r29_2->unk18 = (f32) (sp54 + (temp_f30_2 * (temp_r29_2->unk18 - sp54)));
        temp_r29_2->unk1C = (f32) (sp58 + (temp_f29 * (temp_r29_2->unk1C - sp58)));
        temp_r29_2->unk0 = (f32) (temp_r29_2->unk0 * temp_f3_2);
        temp_r29_2->unk4 = (f32) (temp_r29_2->unk4 * temp_f30_2);
        temp_r29_2->unk8 = (f32) (temp_r29_2->unk8 * temp_f29);
        fn_8132FF6C(sp54, sp58, temp_f3_2, sp50);
        temp_f28 = Hermite(NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        temp_r3_4 = temp_r28_2 + (*var_r27_2 << 5);
        temp_f1_9 = Menu_Get()->unkF0 * temp_f28;
        temp_r3_4->unk-581C = (f32) (temp_r3_4->unk-581C - temp_f1_9);
        temp_f1_10 = Menu_Get((bitwise f32) temp_r3_4, temp_f1_9)->unkF4;
        temp_r3_5 = temp_r28_2 + (*var_r27_2 << 5);
        temp_r3_5->unk-581C = (f32) (temp_r3_5->unk-581C + temp_f1_10);
        temp_r0 = *var_r27_2;
        var_r27_2 += 2;
        var_r26_2 += 1;
        temp_r3_6 = temp_r28_2 + (temp_r0 << 5);
        temp_r3_6->unk-5818 = (f32) (temp_r3_6->unk-5818 + Menu_Get((bitwise f32) temp_r3_5, temp_f1_10)->unkF8);
        goto loop_5;
    }
    var_r29 = fn_81332A48();
    var_r27_3 = var_r29;
    temp_r28_3 = &lbl_814DE638 + 0x10000;
    var_r26_3 = 0;
loop_8:
    if (var_r26_3 < (s32) fn_81332AB0()) {
        temp_f1_11 = Menu_Get()->unk34;
        temp_r3_7 = temp_r28_3 + (*var_r27_3 << 5);
        temp_r3_7->unk-581C = (f32) (temp_r3_7->unk-581C + temp_f1_11);
        temp_f1_12 = Menu_Get((bitwise f32) temp_r3_7, temp_f1_11)->unk38;
        temp_r3_8 = temp_r28_3 + (*var_r27_3 << 5);
        temp_r3_8->unk-5818 = (f32) (temp_r3_8->unk-5818 + temp_f1_12);
        temp_r30 = (temp_r28_3 + (*var_r27_3 << 5)) - 0x5830;
        temp_f30_3 = Menu_Get((bitwise f32) temp_r3_8, temp_f1_12)->unk3C;
        temp_f29_2 = Menu_Get()->unk3C;
        var_r27_3 += 2;
        var_r26_3 += 1;
        temp_f3_3 = Menu_Get()->unk3C;
        temp_r30->unk14 = (f32) (sp50 + (temp_f3_3 * (temp_r30->unk14 - sp50)));
        temp_r30->unk18 = (f32) (sp54 + (temp_f29_2 * (temp_r30->unk18 - sp54)));
        temp_r30->unk1C = (f32) (sp58 + (temp_f30_3 * (temp_r30->unk1C - sp58)));
        temp_r30->unk0 = (f32) (temp_r30->unk0 * temp_f3_3);
        temp_r30->unk4 = (f32) (temp_r30->unk4 * temp_f29_2);
        temp_r30->unk8 = (f32) (temp_r30->unk8 * temp_f30_3);
        goto loop_8;
    }
    temp_r30_2 = &lbl_814DE638 + 0x10000;
    var_r28 = 0;
loop_14:
    if (var_r28 < (s32) fn_81332AB0()) {
        temp_r27_3 = (temp_r30_2 + (*var_r29 << 5)) - 0x5830;
        temp_f30_4 = 1.0f + Menu_Get()->unk40;
        temp_f29_3 = 1.0f + Menu_Get()->unk40;
        temp_f3_4 = 1.0f + Menu_Get()->unk40;
        temp_r27_3->unk14 = (f32) (sp50 + (temp_f3_4 * (temp_r27_3->unk14 - sp50)));
        temp_r27_3->unk18 = (f32) (sp54 + (temp_f29_3 * (temp_r27_3->unk18 - sp54)));
        temp_r27_3->unk1C = (f32) (sp58 + (temp_f30_4 * (temp_r27_3->unk1C - sp58)));
        temp_r27_3->unk0 = (f32) (temp_r27_3->unk0 * temp_f3_4);
        temp_r27_3->unk4 = (f32) (temp_r27_3->unk4 * temp_f29_3);
        temp_r27_3->unk8 = (f32) (temp_r27_3->unk8 * temp_f30_4);
        fn_8132FF6C(sp54, sp58, temp_f3_4, sp50);
        temp_f29_4 = Hermite(NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        temp_r3_9 = temp_r30_2 + (*var_r29 << 5);
        temp_f1_13 = Menu_Get()->unkF0 * temp_f29_4;
        temp_r3_9->unk-581C = (f32) (temp_r3_9->unk-581C + temp_f1_13);
        if (Option_GetLang(temp_r3_9, temp_f1_13) == 4) {
            temp_r3_10 = temp_r30_2 + (*var_r29 << 5);
            temp_r3_10->unk-581C = (f32) (temp_r3_10->unk-581C + -40.0f);
        } else {
            temp_r3_11 = temp_r30_2 + (*var_r29 << 5);
            temp_r3_11->unk-581C = (f32) (temp_r3_11->unk-581C + Menu_Get()->unkFC);
        }
        temp_r0_2 = *var_r29;
        var_r29 += 2;
        var_r28 += 1;
        temp_r3_12 = temp_r30_2 + (temp_r0_2 << 5);
        temp_r3_12->unk-5818 = (f32) (temp_r3_12->unk-5818 + Menu_Get()->unk100);
        goto loop_14;
    }
    var_r30 = fn_81332AEC();
    var_r29_2 = var_r30;
    temp_r28_4 = &lbl_814DE638 + 0x10000;
    var_r26_4 = 0;
loop_17:
    if (var_r26_4 < (s32) fn_81332B54()) {
        temp_f1_14 = Menu_Get()->unk44;
        temp_r3_13 = temp_r28_4 + (*var_r29_2 << 5);
        temp_r3_13->unk-581C = (f32) (temp_r3_13->unk-581C + temp_f1_14);
        temp_f1_15 = Menu_Get((bitwise f32) temp_r3_13, temp_f1_14)->unk48;
        temp_r3_14 = temp_r28_4 + (*var_r29_2 << 5);
        temp_r3_14->unk-5818 = (f32) (temp_r3_14->unk-5818 + temp_f1_15);
        temp_r27_4 = (temp_r28_4 + (*var_r29_2 << 5)) - 0x5830;
        temp_f30_5 = Menu_Get((bitwise f32) temp_r3_14, temp_f1_15)->unk4C;
        temp_f29_5 = Menu_Get()->unk4C;
        var_r29_2 += 2;
        var_r26_4 += 1;
        temp_f3_5 = Menu_Get()->unk4C;
        temp_r27_4->unk14 = (f32) (sp44 + (temp_f3_5 * (temp_r27_4->unk14 - sp44)));
        temp_r27_4->unk18 = (f32) (sp48 + (temp_f29_5 * (temp_r27_4->unk18 - sp48)));
        temp_r27_4->unk1C = (f32) (sp4C + (temp_f30_5 * (temp_r27_4->unk1C - sp4C)));
        temp_r27_4->unk0 = (f32) (temp_r27_4->unk0 * temp_f3_5);
        temp_r27_4->unk4 = (f32) (temp_r27_4->unk4 * temp_f29_5);
        temp_r27_4->unk8 = (f32) (temp_r27_4->unk8 * temp_f30_5);
        goto loop_17;
    }
    temp_r28_5 = &lbl_814DE638 + 0x10000;
    var_r29_3 = 0;
loop_20:
    if (var_r29_3 < (s32) fn_81332B54()) {
        temp_r27_5 = (temp_r28_5 + (*var_r30 << 5)) - 0x5830;
        temp_f30_6 = 1.0f + Menu_Get()->unk50;
        temp_f29_6 = 1.0f + Menu_Get()->unk50;
        temp_f3_6 = 1.0f + Menu_Get()->unk50;
        temp_r27_5->unk14 = (f32) (sp44 + (temp_f3_6 * (temp_r27_5->unk14 - sp44)));
        temp_r27_5->unk18 = (f32) (sp48 + (temp_f29_6 * (temp_r27_5->unk18 - sp48)));
        temp_r27_5->unk1C = (f32) (sp4C + (temp_f30_6 * (temp_r27_5->unk1C - sp4C)));
        temp_r27_5->unk0 = (f32) (temp_r27_5->unk0 * temp_f3_6);
        temp_r27_5->unk4 = (f32) (temp_r27_5->unk4 * temp_f29_6);
        temp_r27_5->unk8 = (f32) (temp_r27_5->unk8 * temp_f30_6);
        fn_8132FF6C(sp48, sp4C, temp_f3_6, sp44);
        temp_f29_7 = Hermite(NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        temp_r3_15 = temp_r28_5 + (*var_r30 << 5);
        temp_f1_16 = Menu_Get()->unkF0 * temp_f29_7;
        temp_r3_15->unk-581C = (f32) (temp_r3_15->unk-581C - temp_f1_16);
        temp_f1_17 = Menu_Get((bitwise f32) temp_r3_15, temp_f1_16)->unk104;
        temp_r3_16 = temp_r28_5 + (*var_r30 << 5);
        temp_r3_16->unk-581C = (f32) (temp_r3_16->unk-581C + temp_f1_17);
        temp_r0_3 = *var_r30;
        var_r30 += 2;
        var_r29_3 += 1;
        temp_r3_17 = temp_r28_5 + (temp_r0_3 << 5);
        temp_r3_17->unk-5818 = (f32) (temp_r3_17->unk-5818 + Menu_Get((bitwise f32) temp_r3_16, temp_f1_17)->unk108);
        goto loop_20;
    }
    var_r30_2 = fn_81332B90();
    var_r29_4 = var_r30_2;
    temp_r28_6 = &lbl_814DE638 + 0x10000;
    var_r26_5 = 0;
loop_23:
    if (var_r26_5 < (s32) fn_81332BF8()) {
        temp_f1_18 = Menu_Get()->unk44;
        temp_r3_18 = temp_r28_6 + (*var_r29_4 << 5);
        temp_r3_18->unk-581C = (f32) (temp_r3_18->unk-581C + temp_f1_18);
        temp_f1_19 = Menu_Get((bitwise f32) temp_r3_18, temp_f1_18)->unk48;
        temp_r3_19 = temp_r28_6 + (*var_r29_4 << 5);
        temp_r3_19->unk-5818 = (f32) (temp_r3_19->unk-5818 + temp_f1_19);
        temp_r27_6 = (temp_r28_6 + (*var_r29_4 << 5)) - 0x5830;
        temp_f30_7 = Menu_Get((bitwise f32) temp_r3_19, temp_f1_19)->unk4C;
        temp_f29_8 = Menu_Get()->unk4C;
        var_r29_4 += 2;
        var_r26_5 += 1;
        temp_f3_7 = Menu_Get()->unk4C;
        temp_r27_6->unk14 = (f32) (sp44 + (temp_f3_7 * (temp_r27_6->unk14 - sp44)));
        temp_r27_6->unk18 = (f32) (sp48 + (temp_f29_8 * (temp_r27_6->unk18 - sp48)));
        temp_r27_6->unk1C = (f32) (sp4C + (temp_f30_7 * (temp_r27_6->unk1C - sp4C)));
        temp_r27_6->unk0 = (f32) (temp_r27_6->unk0 * temp_f3_7);
        temp_r27_6->unk4 = (f32) (temp_r27_6->unk4 * temp_f29_8);
        temp_r27_6->unk8 = (f32) (temp_r27_6->unk8 * temp_f30_7);
        goto loop_23;
    }
    temp_r28_7 = &lbl_814DE638 + 0x10000;
    var_r29_5 = 0;
loop_26:
    if (var_r29_5 < (s32) fn_81332BF8()) {
        temp_r27_7 = (temp_r28_7 + (*var_r30_2 << 5)) - 0x5830;
        temp_f30_8 = 1.0f + Menu_Get()->unk50;
        temp_f29_9 = 1.0f + Menu_Get()->unk50;
        temp_f3_8 = 1.0f + Menu_Get()->unk50;
        temp_r27_7->unk14 = (f32) (sp44 + (temp_f3_8 * (temp_r27_7->unk14 - sp44)));
        temp_r27_7->unk18 = (f32) (sp48 + (temp_f29_9 * (temp_r27_7->unk18 - sp48)));
        temp_r27_7->unk1C = (f32) (sp4C + (temp_f30_8 * (temp_r27_7->unk1C - sp4C)));
        temp_r27_7->unk0 = (f32) (temp_r27_7->unk0 * temp_f3_8);
        temp_r27_7->unk4 = (f32) (temp_r27_7->unk4 * temp_f29_9);
        temp_r27_7->unk8 = (f32) (temp_r27_7->unk8 * temp_f30_8);
        fn_8132FF6C(sp48, sp4C, temp_f3_8, sp44);
        temp_f29_10 = Hermite(NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        temp_r3_20 = temp_r28_7 + (*var_r30_2 << 5);
        temp_f1_20 = Menu_Get()->unkF0 * temp_f29_10;
        temp_r3_20->unk-581C = (f32) (temp_r3_20->unk-581C + temp_f1_20);
        temp_f1_21 = Menu_Get((bitwise f32) temp_r3_20, temp_f1_20)->unk10C;
        temp_r3_21 = temp_r28_7 + (*var_r30_2 << 5);
        temp_r3_21->unk-581C = (f32) (temp_r3_21->unk-581C + temp_f1_21);
        temp_r0_4 = *var_r30_2;
        var_r30_2 += 2;
        var_r29_5 += 1;
        temp_r3_22 = temp_r28_7 + (temp_r0_4 << 5);
        temp_r3_22->unk-5818 = (f32) (temp_r3_22->unk-5818 + Menu_Get((bitwise f32) temp_r3_21, temp_f1_21)->unk110);
        goto loop_26;
    }
    var_r30_3 = fn_813310AC();
    var_r29_6 = var_r30_3;
    temp_r28_8 = &lbl_814DE638 + 0x10000;
    var_r26_6 = 0;
loop_29:
    if (var_r26_6 < (s32) fn_81331114()) {
        temp_f1_22 = Menu_Get()->unk54;
        temp_r3_23 = temp_r28_8 + (*var_r29_6 << 5);
        temp_r3_23->unk-581C = (f32) (temp_r3_23->unk-581C + temp_f1_22);
        temp_f1_23 = Menu_Get((bitwise f32) temp_r3_23, temp_f1_22)->unk58;
        temp_r3_24 = temp_r28_8 + (*var_r29_6 << 5);
        temp_r3_24->unk-5818 = (f32) (temp_r3_24->unk-5818 + temp_f1_23);
        temp_r27_8 = (temp_r28_8 + (*var_r29_6 << 5)) - 0x5830;
        temp_f30_9 = Menu_Get((bitwise f32) temp_r3_24, temp_f1_23)->unk5C;
        temp_f29_11 = Menu_Get()->unk5C;
        var_r29_6 += 2;
        var_r26_6 += 1;
        temp_f3_9 = Menu_Get()->unk5C;
        temp_r27_8->unk14 = (f32) (sp38 + (temp_f3_9 * (temp_r27_8->unk14 - sp38)));
        temp_r27_8->unk18 = (f32) (sp3C + (temp_f29_11 * (temp_r27_8->unk18 - sp3C)));
        temp_r27_8->unk1C = (f32) (sp40 + (temp_f30_9 * (temp_r27_8->unk1C - sp40)));
        temp_r27_8->unk0 = (f32) (temp_r27_8->unk0 * temp_f3_9);
        temp_r27_8->unk4 = (f32) (temp_r27_8->unk4 * temp_f29_11);
        temp_r27_8->unk8 = (f32) (temp_r27_8->unk8 * temp_f30_9);
        goto loop_29;
    }
    temp_r28_9 = &lbl_814DE638 + 0x10000;
    var_r29_7 = 0;
loop_32:
    if (var_r29_7 < (s32) fn_81331114()) {
        temp_r27_9 = (temp_r28_9 + (*var_r30_3 << 5)) - 0x5830;
        temp_f30_10 = 1.0f + Menu_Get()->unk60;
        temp_f29_12 = 1.0f + Menu_Get()->unk60;
        temp_f3_10 = 1.0f + Menu_Get()->unk60;
        temp_r27_9->unk14 = (f32) (sp38 + (temp_f3_10 * (temp_r27_9->unk14 - sp38)));
        temp_r27_9->unk18 = (f32) (sp3C + (temp_f29_12 * (temp_r27_9->unk18 - sp3C)));
        temp_r27_9->unk1C = (f32) (sp40 + (temp_f30_10 * (temp_r27_9->unk1C - sp40)));
        temp_r27_9->unk0 = (f32) (temp_r27_9->unk0 * temp_f3_10);
        temp_r27_9->unk4 = (f32) (temp_r27_9->unk4 * temp_f29_12);
        temp_r27_9->unk8 = (f32) (temp_r27_9->unk8 * temp_f30_10);
        fn_8132FF6C(sp3C, sp40, temp_f3_10, sp38);
        temp_f29_13 = Hermite(NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        temp_r3_25 = temp_r28_9 + (*var_r30_3 << 5);
        temp_f1_24 = Menu_Get()->unkF0 * temp_f29_13;
        temp_r3_25->unk-581C = (f32) (temp_r3_25->unk-581C - temp_f1_24);
        temp_f1_25 = Menu_Get((bitwise f32) temp_r3_25, temp_f1_24)->unk114;
        temp_r3_26 = temp_r28_9 + (*var_r30_3 << 5);
        temp_r3_26->unk-581C = (f32) (temp_r3_26->unk-581C + temp_f1_25);
        temp_r0_5 = *var_r30_3;
        var_r30_3 += 2;
        var_r29_7 += 1;
        temp_r3_27 = temp_r28_9 + (temp_r0_5 << 5);
        temp_r3_27->unk-5818 = (f32) (temp_r3_27->unk-5818 + Menu_Get((bitwise f32) temp_r3_26, temp_f1_25)->unk118);
        goto loop_32;
    }
    var_r30_4 = fn_81331150();
    var_r29_8 = var_r30_4;
    temp_r28_10 = &lbl_814DE638 + 0x10000;
    var_r26_7 = 0;
loop_35:
    if (var_r26_7 < (s32) fn_813311B8()) {
        temp_f1_26 = Menu_Get()->unk54;
        temp_r3_28 = temp_r28_10 + (*var_r29_8 << 5);
        temp_r3_28->unk-581C = (f32) (temp_r3_28->unk-581C + temp_f1_26);
        temp_f1_27 = Menu_Get((bitwise f32) temp_r3_28, temp_f1_26)->unk58;
        temp_r3_29 = temp_r28_10 + (*var_r29_8 << 5);
        temp_r3_29->unk-5818 = (f32) (temp_r3_29->unk-5818 + temp_f1_27);
        temp_r27_10 = (temp_r28_10 + (*var_r29_8 << 5)) - 0x5830;
        temp_f30_11 = Menu_Get((bitwise f32) temp_r3_29, temp_f1_27)->unk5C;
        temp_f29_14 = Menu_Get()->unk5C;
        var_r29_8 += 2;
        var_r26_7 += 1;
        temp_f3_11 = Menu_Get()->unk5C;
        temp_r27_10->unk14 = (f32) (sp38 + (temp_f3_11 * (temp_r27_10->unk14 - sp38)));
        temp_r27_10->unk18 = (f32) (sp3C + (temp_f29_14 * (temp_r27_10->unk18 - sp3C)));
        temp_r27_10->unk1C = (f32) (sp40 + (temp_f30_11 * (temp_r27_10->unk1C - sp40)));
        temp_r27_10->unk0 = (f32) (temp_r27_10->unk0 * temp_f3_11);
        temp_r27_10->unk4 = (f32) (temp_r27_10->unk4 * temp_f29_14);
        temp_r27_10->unk8 = (f32) (temp_r27_10->unk8 * temp_f30_11);
        goto loop_35;
    }
    temp_r28_11 = &lbl_814DE638 + 0x10000;
    var_r29_9 = 0;
loop_38:
    if (var_r29_9 < (s32) fn_813311B8()) {
        temp_r27_11 = (temp_r28_11 + (*var_r30_4 << 5)) - 0x5830;
        temp_f30_12 = 1.0f + Menu_Get()->unk60;
        temp_f29_15 = 1.0f + Menu_Get()->unk60;
        temp_f3_12 = 1.0f + Menu_Get()->unk60;
        temp_r27_11->unk14 = (f32) (sp38 + (temp_f3_12 * (temp_r27_11->unk14 - sp38)));
        temp_r27_11->unk18 = (f32) (sp3C + (temp_f29_15 * (temp_r27_11->unk18 - sp3C)));
        temp_r27_11->unk1C = (f32) (sp40 + (temp_f30_12 * (temp_r27_11->unk1C - sp40)));
        temp_r27_11->unk0 = (f32) (temp_r27_11->unk0 * temp_f3_12);
        temp_r27_11->unk4 = (f32) (temp_r27_11->unk4 * temp_f29_15);
        temp_r27_11->unk8 = (f32) (temp_r27_11->unk8 * temp_f30_12);
        fn_8132FF6C(sp3C, sp40, temp_f3_12, sp38);
        temp_f29_16 = Hermite(NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        temp_r3_30 = temp_r28_11 + (*var_r30_4 << 5);
        temp_f1_28 = Menu_Get()->unkF0 * temp_f29_16;
        temp_r3_30->unk-581C = (f32) (temp_r3_30->unk-581C + temp_f1_28);
        temp_f1_29 = Menu_Get((bitwise f32) temp_r3_30, temp_f1_28)->unk11C;
        temp_r3_31 = temp_r28_11 + (*var_r30_4 << 5);
        temp_r3_31->unk-581C = (f32) (temp_r3_31->unk-581C + temp_f1_29);
        temp_r0_6 = *var_r30_4;
        var_r30_4 += 2;
        var_r29_9 += 1;
        temp_r3_32 = temp_r28_11 + (temp_r0_6 << 5);
        temp_r3_32->unk-5818 = (f32) (temp_r3_32->unk-5818 + Menu_Get((bitwise f32) temp_r3_31, temp_f1_29)->unk120);
        goto loop_38;
    }
}

/* 0x8132FF6C */
f32 fn_8132FF6C(void) {
    f32 temp_f2;

    temp_f2 = (f32) lbl_814AD5D0 / (f32) Menu_Get()->unkEC;
    if (temp_f2 < (bitwise f32) 0.5) {
        return 2.0f * temp_f2;
    }
    return 2.0f * (1.0f - temp_f2);
}

/* 0x8132FFF0 */
void fn_8132FFF0(void) {
    void *var_r7;
    s32 var_ctr;
    void *var_r5;
    void *var_r6;

    var_r7 = &lbl_814DE638;
    var_ctr = 0x1AD;
    var_r5 = &lbl_814DE638 + 0x1BC54;
    var_r6 = &lbl_814DE638 + 0x1DDD8;
    do {
        var_r5->unk0 = (u8) lbl_814AD5D8.unk0;
        var_r5->unk1 = (u8) lbl_814AD5D8.unk1;
        var_r5->unk2 = (u8) lbl_814AD5D8.unk2;
        var_r5->unk3 = (u8) lbl_814AD5D8.unk3;
        var_r6->unk0 = (u8) lbl_814AD5DC.unk0;
        var_r6->unk1 = (u8) lbl_814AD5DC.unk1;
        var_r6->unk2 = (u8) lbl_814AD5DC.unk2;
        var_r6->unk3 = (u8) lbl_814AD5DC.unk3;
        var_r7->unk754E = 0;
        var_r7->unk53CA = 0;
        var_r5->unk4 = (u8) lbl_814AD5D8.unk0;
        var_r5->unk5 = (u8) lbl_814AD5D8.unk1;
        var_r5->unk6 = (u8) lbl_814AD5D8.unk2;
        var_r5->unk7 = (u8) lbl_814AD5D8.unk3;
        var_r6->unk4 = (u8) lbl_814AD5DC.unk0;
        var_r6->unk5 = (u8) lbl_814AD5DC.unk1;
        var_r6->unk6 = (u8) lbl_814AD5DC.unk2;
        var_r6->unk7 = (u8) lbl_814AD5DC.unk3;
        var_r7->unk7550 = 0;
        var_r7->unk53CC = 0;
        var_r5->unk8 = (u8) lbl_814AD5D8.unk0;
        var_r5->unk9 = (u8) lbl_814AD5D8.unk1;
        var_r5->unkA = (u8) lbl_814AD5D8.unk2;
        var_r5->unkB = (u8) lbl_814AD5D8.unk3;
        var_r6->unk8 = (u8) lbl_814AD5DC.unk0;
        var_r6->unk9 = (u8) lbl_814AD5DC.unk1;
        var_r6->unkA = (u8) lbl_814AD5DC.unk2;
        var_r6->unkB = (u8) lbl_814AD5DC.unk3;
        var_r7->unk7552 = 0;
        var_r7->unk53CE = 0;
        var_r5->unkC = (u8) lbl_814AD5D8.unk0;
        var_r5->unkD = (u8) lbl_814AD5D8.unk1;
        var_r5->unkE = (u8) lbl_814AD5D8.unk2;
        var_r5->unkF = (u8) lbl_814AD5D8.unk3;
        var_r6->unkC = (u8) lbl_814AD5DC.unk0;
        var_r6->unkD = (u8) lbl_814AD5DC.unk1;
        var_r6->unkE = (u8) lbl_814AD5DC.unk2;
        var_r6->unkF = (u8) lbl_814AD5DC.unk3;
        var_r7->unk7554 = 0;
        var_r7->unk53D0 = 0;
        var_r5->unk10 = (u8) lbl_814AD5D8.unk0;
        var_r5->unk11 = (u8) lbl_814AD5D8.unk1;
        var_r5->unk12 = (u8) lbl_814AD5D8.unk2;
        var_r5->unk13 = (u8) lbl_814AD5D8.unk3;
        var_r5 += 0x14;
        var_r6->unk10 = (u8) lbl_814AD5DC.unk0;
        var_r6->unk11 = (u8) lbl_814AD5DC.unk1;
        var_r6->unk12 = (u8) lbl_814AD5DC.unk2;
        var_r6->unk13 = (u8) lbl_814AD5DC.unk3;
        var_r6 += 0x14;
        var_r7->unk7556 = 0;
        var_r7->unk53D2 = 0;
        var_r7 += 0xA;
        var_ctr -= 1;
    } while (var_ctr != 0);
}

/* 0x81330128 */
void fn_81330128(void *arg0, u32 *arg1, f32 farg0) {
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    f32 sp50;
    u8 sp20[0x30];
    void *var_r28;
    f32 temp_f0;
    f32 temp_f0_2;
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f2;
    f32 temp_f3;
    f32 var_f4;
    s8 var_r0;
    u32 var_r27;
    u8 *var_r29;

    temp_f0 = 255.0f * (1.0f - (1.4f * farg0));
    var_r0 = (s8) temp_f0;
    if ((s16) temp_f0 < 0) {
        var_r0 = 0;
    }
    lbl_814AD5D8.unk3 = var_r0;
    var_r29 = &lbl_81381520;
    var_r28 = arg0;
    var_r27 = 0U;
loop_6:
    if (var_r27 < (u32) *arg1) {
        temp_f0_2 = farg0 - (0.4f * (1.0f - ((f32) *var_r29 / 255.0f)));
        var_f4 = temp_f0_2;
        if (temp_f0_2 < 0.0f) {
            var_f4 = 0.0f;
        }
        temp_f3 = 65535.0f * var_f4;
        var_r28->unk14 = (f32) (var_r28->unk14 + (-585.0f * var_f4));
        temp_f2 = var_r28->unk18;
        var_r28->unk18 = (f32) (temp_f2 + (200.0f * var_f4));
        temp_f1 = var_r28->unk1C;
        var_r28->unk1C = (f32) (temp_f1 - (500.0f * var_f4));
        MtxRot((s32) temp_f3, 0, (s32) temp_f3, &sp20, temp_f1, temp_f2, temp_f3);
        sp50 = var_r28->unk14 - 0.0f;
        sp54 = var_r28->unk18 - 0.0f;
        sp58 = var_r28->unk1C - 0.0f;
        PSMTXMultVec(&sp20, &sp50, &sp5C);
        var_r29 += 1;
        var_r27 += 1;
        temp_f1_2 = var_r28->unk14;
        var_r28->unk14 = (f32) (temp_f1_2 + ((sp5C + 0.0f) - temp_f1_2));
        var_r28->unk18 = (f32) (var_r28->unk18 + ((sp60 + 0.0f) - var_r28->unk18));
        var_r28->unk1C = (f32) (var_r28->unk1C + ((sp64 + 0.0f) - var_r28->unk1C));
        var_r28->unkC = (s16) (var_r28->unkC + (s32) temp_f3);
        var_r28->unk10 = (s16) (var_r28->unk10 + (s32) temp_f3);
        var_r28 += 0x20;
        goto loop_6;
    }
}

/* 0x81330374 */
void fn_81330374(void *arg0, u32 *arg1, f32 farg0) {
    f32 sp28;
    void *var_r4;
    f32 var_f1;
    u32 var_r3;

    Fx_EvalShape(1, arg0, arg1, 1.0f);
    if (farg0 < 0.1f) {
        fn_8132C830(&sp28, arg0, NULL, (u16) *arg1);
        if ((10.0f * farg0) < 0.1f) {
            var_f1 = 1.0f - (0.05f * Hermite(NULL, 0.0f, 0.0f, 1.0f, 0.0f));
        } else {
            var_f1 = 1.0f - (0.05f * Hermite((void *)0x3DCCCCCD, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f));
        }
        var_r4 = arg0;
        var_r3 = 0U;
loop_6:
        if (var_r3 < (u32) *arg1) {
            var_r3 += 1;
            var_r4->unk14 = (f32) (sp28 + (var_f1 * (var_r4->unk14 - sp28)));
            var_r4->unk18 = (f32) (sp2C + (var_f1 * (var_r4->unk18 - sp2C)));
            var_r4->unk1C = (f32) (sp30 + (var_f1 * (var_r4->unk1C - sp30)));
            var_r4->unk0 = (f32) (var_r4->unk0 * var_f1);
            var_r4->unk4 = (f32) (var_r4->unk4 * var_f1);
            var_r4->unk8 = (f32) (var_r4->unk8 * var_f1);
            var_r4->unk0 = (f32) (var_r4->unk0 * var_f1);
            var_r4->unk4 = (f32) (var_r4->unk4 * var_f1);
            var_r4->unk8 = (f32) (var_r4->unk8 * var_f1);
            var_r4 += 0x20;
            goto loop_6;
        }
        return;
    }
    fn_81330128(arg0, arg1, farg0 - 0.1f);
}

/* 0x81330518 */
void fn_81330518(void *arg0, void *arg1, void *arg2, void *arg3, f32 farg0, f32 farg1) {
    f32 temp_f3;
    f32 temp_f4;

    temp_f3 = 1.0f - farg0;
    temp_f4 = 1.0f - farg1;
    arg3->unk0 = (s8) ((farg1 * (f32) arg2->unk0) + (temp_f4 * ((farg0 * (f32) arg0->unk0) + (temp_f3 * (f32) arg1->unk0))));
    arg3->unk1 = (s8) ((farg1 * (f32) arg2->unk1) + (temp_f4 * ((farg0 * (f32) arg0->unk1) + (temp_f3 * (f32) arg1->unk1))));
    arg3->unk2 = (s8) ((farg1 * (f32) arg2->unk2) + (temp_f4 * ((farg0 * (f32) arg0->unk2) + (temp_f3 * (f32) arg1->unk2))));
    arg3->unk3 = (s8) ((farg1 * (f32) arg2->unk3) + (temp_f4 * ((farg0 * (f32) arg0->unk3) + (temp_f3 * (f32) arg1->unk3))));
}

/* 0x813306C8 */
void fn_813306C8(s16 arg0) {
    lbl_814DE638.unk1FF60 = 0xA;
    lbl_814DE638.unk1FF5E = arg0;
    lbl_814DE638.unk1FF5C = arg0;
    lbl_814DE638.unk1FF6C = 0.0f;
    lbl_814DE638.unk1FF68 = 0.0f;
    lbl_814DE638.unk1FF64 = 0.0f;
}

/* 0x813306FC */
void fn_813306FC(s16 arg0, s16 arg1) {
    s32 var_r29;
    void *var_r30;

    if (arg0 < arg1) {
        lbl_814DE638.unk1FF64 = -1.0f;
    } else {
        lbl_814DE638.unk1FF64 = 1.0f;
    }
    lbl_814DE638.unk1FF60 = 0;
    lbl_814DE638.unk1FF5C = arg0;
    var_r30 = &lbl_814DE638 + 0x1FF5C;
    lbl_814DE638.unk1FF5E = arg1;
    var_r29 = 0;
    do {
        var_r29 += 1;
        var_r30->unk14 = (s8) ((Rand() % 60) + 0x14);
        var_r30 += 1;
    } while (var_r29 < 0x280);
}

/* 0x813307B4 */
void fn_813307B4(void) {
    s32 sp14;
    s32 sp10;
    s32 spC;
    s32 sp8;
    f32 temp_f2;
    f32 temp_f9;
    s32 temp_r12;
    s32 temp_r26;
    s32 temp_r8;
    s32 temp_r9_2;
    s32 var_r24;
    s32 var_r25;
    s32 var_r27_2;
    s32 var_r6_2;
    s8 *var_r7;
    s8 *var_r7_2;
    u16 *var_r27;
    u16 *var_r29;
    u16 *var_r3;
    u16 *var_r5;
    u16 temp_r0;
    u16 var_ctr;
    u16 var_ctr_2;
    u16 var_r30;
    u16 var_r31;
    u32 temp_r3_3;
    u8 temp_r12_2;
    u8 temp_r3_2;
    void *temp_r0_2;
    void *temp_r0_3;
    void *temp_r22;
    void *temp_r26_2;
    void *temp_r27;
    void *temp_r28;
    void *temp_r3;
    void *temp_r4;
    void *temp_r9;
    void *temp_r9_3;
    void *var_r4;
    void *var_r6;

    var_r27 = saved_reg_r27;
    var_r29 = saved_reg_r29;
    var_r30 = saved_reg_r30;
    var_r31 = saved_reg_r31;
    switch ((s16) lbl_814DE638.unkA7A4) {           /* irregular */
    case 0:
        var_r27 = fn_81330F88(lbl_814DE638.unk1FF5C);
        var_r31 = fn_81330FAC(lbl_814DE638.unk1FF5C);
        var_r29 = fn_81330F88(lbl_814DE638.unk1FF5E);
        var_r30 = fn_81330FAC(*((&lbl_814DE638 + 0x20000) - 0xA2));
        break;
    case 1:
        var_r27 = fn_81330FC8(lbl_814DE638.unk1FF5C);
        var_r31 = fn_81330FEC(lbl_814DE638.unk1FF5C);
        var_r29 = fn_81330FC8(lbl_814DE638.unk1FF5E);
        var_r30 = fn_81330FEC(*((&lbl_814DE638 + 0x20000) - 0xA2));
        break;
    }
    var_r24 = 0;
    var_r25 = 0;
loop_8:
    if (var_r25 < (s32) fn_81332C9C()) {
        temp_r0 = *(fn_81332C34() + var_r24);
        var_r24 += 2;
        var_r25 += 1;
        (&lbl_814DE638 + 0x20000 + temp_r0)->unk-4C10 = 0;
        goto loop_8;
    }
    temp_r4 = &lbl_814DE638 + 0x20000;
    var_r5 = var_r27;
    var_ctr = var_r31;
    sp10 = lbl_814AD5EC;
    sp14 = lbl_814AD5E8;
    var_r7 = (temp_r4 + lbl_814AD5F0) - 0x4C10;
    temp_r0_2 = &lbl_814DE638 + 0x10000;
    var_r27_2 = 0;
    var_r6 = &lbl_814DE638 + 0x1FF5C;
    if (var_r31 > 0) {
        do {
            temp_r26 = lbl_814AD5F0 + var_r27_2;
            temp_r3 = temp_r0_2 + (*var_r5 << 5);
            temp_r9 = temp_r0_2 + (temp_r26 << 5);
            temp_r26_2 = temp_r4 + (temp_r26 * 4);
            temp_r9->unk-5830 = (s32) temp_r3->unk-5830;
            temp_f9 = (f32) lbl_814DE638.unk1FF60 / 10.0f;
            var_r5 += 2;
            temp_r9->unk-582C = (s32) temp_r3->unk-582C;
            var_r27_2 += 1;
            temp_r9->unk-5828 = (s32) temp_r3->unk-5828;
            temp_r9->unk-5824 = (s32) temp_r3->unk-5824;
            M2C_STRUCT_COPY(temp_r9, temp_r3, 8);
            temp_r9->unk-5818 = (s32) temp_r3->unk-5818;
            temp_r9->unk-5814 = (s32) temp_r3->unk-5814;
            temp_r3_2 = var_r6->unk14;
            var_r6 += 1;
            temp_r9->unk-581C = (f32) ((f64) temp_r9->unk-581C - (0.2 * (f64) (lbl_814DE638.unk1FF64 * ((f32) temp_r3_2 * temp_f9))));
            *var_r7 = (s8) (255.0f * (1.0f - temp_f9));
            var_r7 += 1;
            temp_r26_2->unk-43AC = sp14;
            temp_r26_2->unk-2228 = sp10;
            var_ctr -= 1;
        } while (var_ctr != 0);
    }
    lbl_814AD5F0 += var_r31;
    temp_r0_3 = &lbl_814DE638 + 0x20000;
    var_ctr_2 = var_r30;
    temp_r28 = &lbl_814DE638 + 0x10000;
    spC = lbl_814AD5E8;
    sp8 = lbl_814AD5EC;
    var_r3 = var_r29;
    var_r7_2 = (temp_r0_3 + lbl_814AD5F0) - 0x4C10;
    var_r6_2 = 0;
    var_r4 = (temp_r0_3 + var_r31) - 0xA4;
    if (var_r30 > 0) {
        do {
            temp_r12 = lbl_814AD5F0 + var_r6_2;
            temp_r27 = temp_r28 + (*var_r3 << 5);
            temp_r22 = temp_r28 + (temp_r12 << 5);
            temp_r9_2 = temp_r27->unk-5830;
            temp_r8 = temp_r27->unk-582C;
            var_r3 += 2;
            temp_r22->unk-5830 = temp_r9_2;
            temp_r9_3 = temp_r0_3 + (temp_r12 * 4);
            temp_r22->unk-582C = temp_r8;
            var_r6_2 += 1;
            temp_f2 = (f32) lbl_814DE638.unk1FF60 / 10.0f;
            M2C_STRUCT_COPY(temp_r22, temp_r27, 8);
            temp_r22->unk-5820 = (s32) temp_r27->unk-5820;
            temp_r22->unk-581C = (f32) temp_r27->unk-581C;
            temp_r22->unk-5818 = (s32) temp_r27->unk-5818;
            temp_r22->unk-5814 = (s32) temp_r27->unk-5814;
            temp_r12_2 = var_r4->unk14;
            var_r4 += 1;
            temp_r22->unk-581C = (f32) ((f64) temp_r22->unk-581C + (0.2 * (f64) (lbl_814DE638.unk1FF64 * ((f32) temp_r12_2 * (1.0f - temp_f2)))));
            *var_r7_2 = (s8) (255.0f * temp_f2);
            var_r7_2 += 1;
            temp_r9_3->unk-43AC = spC;
            temp_r9_3->unk-2228 = sp8;
            var_ctr_2 -= 1;
        } while (var_ctr_2 != 0);
    }
    temp_r3_3 = lbl_814AD5F0;
    lbl_814AD5F0 = temp_r3_3 + var_r30;
    fn_81330B7C(temp_r3_3);
}

/* 0x81330B7C */
void fn_81330B7C(void) {
    f32 temp_f11;
    f32 temp_f11_2;
    f32 temp_f11_3;
    f32 temp_f9;
    s32 temp_r24;
    s32 var_r25;
    s8 *var_r10;
    u16 *temp_r31;
    u16 *var_r8;
    u16 *var_r9_2;
    u16 temp_r0_3;
    u16 temp_r0_5;
    u16 temp_r3;
    u16 temp_r5_3;
    u16 var_ctr;
    u16 var_ctr_3;
    u16 var_r3;
    u32 temp_r0;
    u32 var_ctr_2;
    u8 temp_r0_2;
    u8 temp_r0_4;
    u8 temp_r5_2;
    void *temp_r12;
    void *temp_r24_2;
    void *temp_r4;
    void *temp_r5;
    void *temp_r6;
    void *temp_r7;
    void *temp_r7_2;
    void *temp_r7_3;
    void *temp_r7_4;
    void *temp_r7_5;
    void *temp_r7_6;
    void *temp_r7_7;
    void *temp_r8;
    void *var_r29;
    void *var_r9;

    temp_r31 = fn_813327B8((bitwise f32) &lbl_814DE638);
    temp_r3 = fn_81332820();
    temp_r7 = &lbl_814DE638 + 0x20000;
    var_ctr = temp_r3;
    var_r29 = &lbl_814DE638 + 0x1FF5C;
    var_r8 = temp_r31;
    var_r9 = var_r29;
    var_r10 = (temp_r7 + lbl_814AD5F0) - 0x4C10;
    temp_r6 = &lbl_814DE638 + 0x10000;
    var_r25 = 0;
    if (temp_r3 > 0) {
        do {
            temp_r24 = lbl_814AD5F0 + var_r25;
            temp_r5 = temp_r6 + (*var_r8 << 5);
            temp_r12 = temp_r6 + (temp_r24 << 5);
            temp_r12->unk-5830 = (s32) temp_r5->unk-5830;
            temp_r24_2 = temp_r7 + (temp_r24 * 4);
            var_r25 += 1;
            temp_r12->unk-582C = (s32) temp_r5->unk-582C;
            temp_f9 = (f32) lbl_814DE638.unk1FF60 / 10.0f;
            temp_r12->unk-5828 = (s32) temp_r5->unk-5828;
            temp_r12->unk-5824 = (s32) temp_r5->unk-5824;
            temp_r12->unk-5820 = (s32) temp_r5->unk-5820;
            temp_r12->unk-581C = (f32) temp_r5->unk-581C;
            M2C_STRUCT_COPY(temp_r12, temp_r5, 8);
            temp_r5_2 = var_r9->unk14;
            var_r9 += 1;
            temp_r12->unk-581C = (f32) ((f64) temp_r12->unk-581C - (0.2 * (f64) (lbl_814DE638.unk1FF64 * ((f32) temp_r5_2 * temp_f9))));
            *var_r10 = (s8) ((f32) (temp_r7 + *var_r8)->unk-4C10 * (1.0f - temp_f9));
            var_r10 += 1;
            temp_r24_2->unk-43AC = (s32) (temp_r7 + (*var_r8 * 4))->unk-43AC;
            temp_r5_3 = *var_r8;
            var_r8 += 2;
            temp_r24_2->unk-2228 = (s32) (temp_r7 + (temp_r5_3 * 4))->unk-2228;
            var_ctr -= 1;
        } while (var_ctr != 0);
    }
    lbl_814AD5F0 += temp_r3;
    var_r9_2 = temp_r31;
    var_r3 = temp_r3;
    temp_r8 = &lbl_814DE638 + 0x10000;
    temp_r4 = &lbl_814DE638 + 0x20000;
    if ((s32) temp_r3 > 0) {
        temp_r0 = var_r3 >> 1U;
        var_ctr_2 = temp_r0;
        if (temp_r0 != 0) {
            do {
                temp_r7_2 = temp_r8 + (var_r9_2->unk0 << 5);
                temp_f11 = (f32) lbl_814DE638.unk1FF60 / 10.0f;
                temp_r7_2->unk-581C = (f32) ((f64) temp_r7_2->unk-581C + (0.2 * (f64) (lbl_814DE638.unk1FF64 * ((f32) var_r29->unk14 * (1.0f - temp_f11)))));
                temp_r7_3 = temp_r4 + var_r9_2->unk0;
                temp_f11_2 = (f32) lbl_814DE638.unk1FF60 / 10.0f;
                temp_r7_3->unk-4C10 = (u8) (s32) ((f32) temp_r7_3->unk-4C10 * temp_f11);
                temp_r0_2 = var_r29->unk15;
                var_r29 += 2;
                temp_r7_4 = temp_r8 + (var_r9_2->unk2 << 5);
                temp_r7_4->unk-581C = (f32) ((f64) temp_r7_4->unk-581C + (0.2 * (f64) (lbl_814DE638.unk1FF64 * ((f32) temp_r0_2 * (1.0f - temp_f11_2)))));
                temp_r0_3 = var_r9_2->unk2;
                var_r9_2 += 4;
                temp_r7_5 = temp_r4 + temp_r0_3;
                temp_r7_5->unk-4C10 = (u8) (s32) ((f32) temp_r7_5->unk-4C10 * temp_f11_2);
                var_ctr_2 -= 1;
            } while (var_ctr_2 != 0);
            var_r3 &= 1;
            if (var_r3 != 0) {
                goto block_6;
            }
        } else {
block_6:
            var_ctr_3 = var_r3;
            do {
                temp_r0_4 = var_r29->unk14;
                var_r29 += 1;
                temp_r7_6 = temp_r8 + (*var_r9_2 << 5);
                temp_f11_3 = (f32) lbl_814DE638.unk1FF60 / 10.0f;
                temp_r7_6->unk-581C = (f32) ((f64) temp_r7_6->unk-581C + (0.2 * (f64) (lbl_814DE638.unk1FF64 * ((f32) temp_r0_4 * (1.0f - temp_f11_3)))));
                temp_r0_5 = *var_r9_2;
                var_r9_2 += 2;
                temp_r7_7 = temp_r4 + temp_r0_5;
                temp_r7_7->unk-4C10 = (u8) (s32) ((f32) temp_r7_7->unk-4C10 * temp_f11_3);
                var_ctr_3 -= 1;
            } while (var_ctr_3 != 0);
        }
    }
}
