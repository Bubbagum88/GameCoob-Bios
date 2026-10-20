// Non-matching: functionally equivalent decompilation
//
// Mosaic cube text ("fx") used inside Game Play, Options and Calendar,
// 0x81329890..0x8132D000: core (shape curves, pool) and the per-submenu layout builders.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x81329890 */
void Fx_SetShapes(void *arg0) {
    lbl_814AD5C0 = arg0;
    lbl_814AD5C4 = arg0 + arg0->unkC;
    lbl_814AD5C8 = arg0 + arg0->unk8;
}

/* 0x813298B0 */
void Fx_EvalShape(s32 arg0, s16 *arg1, u32 *arg2, f32 farg0) {
    f32 sp18;
    f32 temp_f0;
    f32 temp_f0_2;
    f32 temp_f23;
    f32 temp_f26;
    f32 temp_f2;
    f32 var_f22;
    f32 var_f25;
    s16 *var_r29;
    s32 temp_r28;
    u16 temp_r4;
    u32 var_r25;
    u32 var_r27;
    u8 temp_r3;
    void *temp_r24;
    void *temp_r3_2;
    void *temp_r4_2;
    void *temp_r5;
    void *temp_r6;
    void *var_r30;

    var_f25 = farg0;
    temp_r28 = arg0 * 8;
    *arg2 = (u32) *(lbl_814AD5C4 + temp_r28);
    temp_r6 = lbl_814AD5C4 + temp_r28;
    temp_f26 = 1.0f / (f32) (temp_r6->unk2 - 1);
    if (var_f25 < 0.0f) {
        var_f25 = 0.0f;
    }
    if (var_f25 > 1.0f) {
        var_f25 = 1.0f;
    }
    var_r30 = lbl_814AD5C4 + (temp_r28 + temp_r6->unk4);
    var_r29 = arg1;
    var_r27 = 0U;
loop_12:
    if (var_r27 < (u32) *arg2) {
        temp_r3 = var_r30->unk0;
        temp_r24 = var_r30 + var_r30->unk4;
        temp_f2 = 0.4f * (1.0f - ((f32) temp_r3 / 255.0f));
        temp_f0 = (var_f25 - temp_f2) / ((0.6f + (0.4f * (1.0f - ((f32) var_r30->unk1 / 255.0f)))) - temp_f2);
        temp_f23 = temp_f0 / temp_f26;
        var_f22 = temp_f0;
        var_r25 = __cvt_fp2unsigned(temp_r3, temp_f23, temp_f2);
        temp_r4 = *(temp_r28 + (lbl_814AD5C4 + 2));
        if (__cvt_fp2unsigned((bitwise u8) temp_f23) >= temp_r4) {
            var_r25 = temp_r4 - 1;
        }
        if (var_f22 < 0.0f) {
            var_f22 = 0.0f;
        }
        if (var_f22 > 1.0f) {
            var_f22 = 1.0f;
        }
        temp_r4_2 = temp_r24 + (var_r25 * 2);
        temp_r5 = lbl_814AD5C8 + (temp_r4_2->unk2 * 0x24);
        temp_r3_2 = lbl_814AD5C8 + (temp_r4_2->unk0 * 0x24);
        VecHermite(temp_r3_2, temp_r3_2 + 0x18, temp_r5, temp_r5 + 0xC, &sp18, var_f22, (f32) var_r25 * temp_f26, (f32) (var_r25 + 1) * temp_f26);
        var_r29->unk14 = sp18;
        var_r29->unk18 = sp1C;
        var_r29->unk1C = sp20;
        var_r29->unk10 = 0;
        var_r29->unkE = 0;
        var_r29->unkC = 0;
        temp_f0_2 = Menu_Get()->unkAC;
        var_r30 += 8;
        var_r27 += 1;
        var_r29->unk8 = temp_f0_2;
        var_r29->unk4 = temp_f0_2;
        var_r29->unk0 = temp_f0_2;
        var_r29 += 0x20;
        goto loop_12;
    }
}

/* 0x81329B24 */
u16 fn_81329B24(s32 arg0) {
    return *(lbl_814AD5C4 + (arg0 * 8));
}

/* 0x81329B34 */
void *Fx_GetCubes(void) {
    return &lbl_814E8E08;
}

/* 0x81329B40 */
void *Fx_GetDrawBuf(void) {
    return &lbl_814F9A28;
}

/* 0x81329B4C */
u32 Fx_GetCount(void) {
    return lbl_814AD5F0;
}

/* 0x81329B54 */
void *fn_81329B54(void) {
    return &lbl_814FA28C;
}

/* 0x81329B60 */
void *fn_81329B60(void) {
    return &lbl_814FC410;
}

/* 0x81329B6C */
s32 Fx_IsReady(s32 arg0) {
    if ((u32) lbl_814DE638.unkA794 < (u32) (Menu_Get()->unkD6 - arg0)) {
        return 0;
    }
    return 1;
}

/* 0x81329BC4 */
void Fx_Init(s32 arg0, void *arg1) {
    memset(&lbl_814DE638 + 0xA798, 0, 0x1C);
    memset(&lbl_814DE638, 0, 0xA798);
    memset(&lbl_814DE638 + 0x201F0, 0, 0xC);
    lbl_814DE638.unkA798 = 0;
    lbl_814DE638.unkA7B4 = (s32) lbl_814DE638.unkA798;
    lbl_814DE638.unkA7B8 = (s32) lbl_814DE638.unkA79C;
    lbl_814DE638.unkA7BC = (s32) lbl_814DE638.unkA7A0;
    lbl_814DE638.unkA7C0 = (s32) lbl_814DE638.unkA7A4;
    lbl_814DE638.unkA7C4 = (s32) lbl_814DE638.unkA7A8;
    lbl_814DE638.unkA7C8 = (s32) lbl_814DE638.unkA7AC;
    lbl_814DE638.unkA7CC = (s32) lbl_814DE638.unkA7B0;
    fn_81330F30(arg0, lbl_814DE638.unkA7A8, &lbl_814DE638 + 0x10000);
    Fx_SetShapes(arg1);
}

/* 0x81329C98 */
void Fx_Clear(void) {
    fn_8132D000();
    memset(&lbl_814DE638, 0, 0xA798);
    memset(&lbl_814FE828, 0, 0xC);
}

/* 0x81329CE4 */
void FxOption_Update(s16 arg0, s32 arg1, s32 arg2, s32 arg3) {
    f32 temp_f31;
    s16 temp_r3;
    s32 temp_r28;
    s32 temp_r28_2;
    s32 temp_r28_3;
    s32 temp_r29;
    s32 temp_r29_2;
    s8 *var_r28;
    u16 temp_r3_2;
    u32 var_r29;

    if ((s32) lbl_814AD5F4 != Option_GetLang(&lbl_814DE638)) {
        FxOption_ChangeLang(lbl_814AD5F4, Option_GetLang());
    }
    lbl_814AD5F4 = (s32) Option_GetLang();
    if ((s32) lbl_814AD5F8 != Lang_Get()) {
        FxOption_ChangeUiLang(lbl_814AD5F8, Lang_Get());
    }
    lbl_814AD5F8 = (s32) Lang_Get();
    FxOption_SetState(arg0, arg1, arg2, arg3);
    if ((s16) lbl_814DE638.unkA7B4 != 1) {
        fn_81333344();
        fn_8133370C();
        memset(&lbl_814DE638, 0, 0xA798);
        memset(&lbl_814DE638 + 0x201F0, 0, 0xC);
        temp_r3 = Menu_Get()->unkD6;
        lbl_814DE638.unkA796 = temp_r3;
        lbl_814AD5D0 = 0;
        lbl_814AD5F4 = (s32) Option_GetLang((s16 *) temp_r3);
        FxOption_ChangeLang(lbl_814AD5F4, (s16) lbl_814AD5F4);
        fn_813306C8(Option_GetLang());
    }
    lbl_814AD5D8.unk0 = (s8) Menu_Get()->unk136;
    lbl_814AD5D8.unk1 = (s8) Menu_Get()->unk13A;
    lbl_814AD5D8.unk2 = (s8) Menu_Get()->unk13E;
    lbl_814AD5D8.unk3 = 0xFF;
    lbl_814AD5DC.unk0 = (s8) Menu_Get()->unk138;
    lbl_814AD5DC.unk1 = (s8) Menu_Get()->unk13C;
    lbl_814AD5DC.unk2 = (s8) Menu_Get()->unk140;
    lbl_814AD5DC.unk3 = 0xFF;
    lbl_814AD5E0.unk0 = (s8) Menu_Get()->unk142;
    lbl_814AD5E0.unk1 = (s8) Menu_Get()->unk146;
    lbl_814AD5E0.unk2 = (s8) Menu_Get()->unk14A;
    lbl_814AD5E0.unk3 = 0xFF;
    lbl_814AD5E4.unk0 = (s8) Menu_Get()->unk144;
    lbl_814AD5E4.unk1 = (s8) Menu_Get()->unk148;
    lbl_814AD5E4.unk2 = (s8) Menu_Get()->unk14C;
    lbl_814AD5E4.unk3 = 0xFF;
    lbl_814AD5E8.unk0 = (s8) Menu_Get()->unk14E;
    lbl_814AD5E8.unk1 = (s8) Menu_Get()->unk152;
    lbl_814AD5E8.unk2 = (s8) Menu_Get()->unk156;
    lbl_814AD5E8.unk3 = 0xFF;
    lbl_814AD5EC.unk0 = (s8) Menu_Get()->unk150;
    lbl_814AD5EC.unk1 = (s8) Menu_Get()->unk154;
    lbl_814AD5EC.unk2 = (s8) Menu_Get()->unk158;
    lbl_814AD5EC.unk3 = 0xFF;
    fn_8132FFF0();
    fn_8132ADCC();
    fn_8132A75C();
    fn_8132C024();
    fn_8132C2FC();
    fn_8132CCF4();
    Fx_EvalShape(2, &lbl_814DE638 + 0xA7D0, &lbl_814AD5F0);
    fn_8132F010();
    fn_8132D234();
    temp_f31 = Menu_Get()->unkD8;
    if (fn_8132CCF4() > temp_f31) {
        Fx_EmitSparks(&lbl_814DE638 + 0xA7D0);
    }
    fn_8132BD00();
    fn_81333384();
    temp_r28 = fn_813335AC() << 5;
    memcpy((&lbl_814DE638 + 0x10000 + (lbl_814AD5F0 << 5)) - 0x5830, fn_813334AC(), temp_r28);
    var_r29 = lbl_814AD5F0;
    var_r28 = (&lbl_814DE638 + 0x20000 + lbl_814AD5F0) - 0x4C10;
loop_10:
    if (var_r29 < (u32) (lbl_814AD5F0 + fn_813335AC())) {
        *var_r28 = fn_81333614();
        var_r28 += 1;
        var_r29 += 1;
        goto loop_10;
    }
    lbl_814AD5F0 += fn_813335AC();
    fn_81334C60(&lbl_814DE638 + 0xA7D0);
    temp_r28_2 = fn_81334C08() << 5;
    memcpy((&lbl_814DE638 + 0x10000 + (lbl_814AD5F0 << 5)) - 0x5830, fn_81334C1C(), temp_r28_2);
    temp_r28_3 = fn_81334C08();
    memcpy((&lbl_814DE638 + 0x20000 + lbl_814AD5F0) - 0x4C10, fn_81334C10(), temp_r28_3);
    lbl_814AD5F0 += fn_81334C08();
    if (((s16) lbl_814DE638.unkA79E == 2) && ((s16) lbl_814DE638.unkA7BA != 2)) {
        fn_81334C28();
    }
    fn_813369B0(&lbl_814DE638 + 0xA7D0);
    temp_r29 = fn_81336968() << 5;
    memcpy((&lbl_814DE638 + 0x10000 + (lbl_814AD5F0 << 5)) - 0x5830, fn_8133697C(), temp_r29);
    temp_r29_2 = fn_81336968();
    memcpy((&lbl_814DE638 + 0x20000 + lbl_814AD5F0) - 0x4C10, fn_81336970(), temp_r29_2);
    lbl_814AD5F0 += fn_81336968();
    if (((s16) lbl_814DE638.unkA79E == 1) && ((s16) lbl_814DE638.unkA7BA != 1)) {
        fn_81336988();
    }
    if ((s16) lbl_814DE638.unkA79E == 3) {
        fn_813307B4();
    }
    fn_8132BF20();
    temp_r3_2 = lbl_814AD5D0;
    lbl_814AD5D0 = temp_r3_2 + 1;
    if ((s32) lbl_814AD5D0 >= (s16) Menu_Get(temp_r3_2)->unkEC) {
        lbl_814AD5D0 = 0;
    }
}

/* 0x8132A14C */
void FxClock_Update(s16 arg0, s16 arg1, s16 arg2, s16 arg3) {
    f32 temp_f31;
    s32 temp_r31;
    s32 temp_r31_2;

    fn_8132D194(arg0, arg1, arg2, arg3, &lbl_814DE638);
    if ((s16) lbl_814DE638.unkA7B4 != 2) {
        memset(&lbl_814DE638, 0, 0xA798);
        memset(&lbl_814DE638 + 0x201F0, 0, 0xC);
        lbl_814DE638.unkA796 = (s16) Menu_Get()->unkD6;
        fn_8133370C();
    }
    lbl_814AD5D8.unk0 = (s8) Menu_Get()->unk15A;
    lbl_814AD5D8.unk1 = (s8) Menu_Get()->unk15E;
    lbl_814AD5D8.unk2 = (s8) Menu_Get()->unk162;
    lbl_814AD5D8.unk3 = 0xFF;
    lbl_814AD5DC.unk0 = (s8) Menu_Get()->unk15C;
    lbl_814AD5DC.unk1 = (s8) Menu_Get()->unk160;
    lbl_814AD5DC.unk2 = (s8) Menu_Get()->unk164;
    lbl_814AD5DC.unk3 = 0xFF;
    lbl_814AD5E0.unk0 = (s8) Menu_Get()->unk166;
    lbl_814AD5E0.unk1 = (s8) Menu_Get()->unk16A;
    lbl_814AD5E0.unk2 = (s8) Menu_Get()->unk16E;
    lbl_814AD5E0.unk3 = 0xFF;
    lbl_814AD5E4.unk0 = (s8) Menu_Get()->unk168;
    lbl_814AD5E4.unk1 = (s8) Menu_Get()->unk16C;
    lbl_814AD5E4.unk2 = (s8) Menu_Get()->unk170;
    lbl_814AD5E4.unk3 = 0xFF;
    lbl_814AD5E8.unk0 = (s8) Menu_Get()->unk172;
    lbl_814AD5E8.unk1 = (s8) Menu_Get()->unk176;
    lbl_814AD5E8.unk2 = (s8) Menu_Get()->unk17A;
    lbl_814AD5E8.unk3 = 0xFF;
    lbl_814AD5EC.unk0 = (s8) Menu_Get()->unk174;
    lbl_814AD5EC.unk1 = (s8) Menu_Get()->unk178;
    lbl_814AD5EC.unk2 = (s8) Menu_Get()->unk17C;
    lbl_814AD5EC.unk3 = 0xFF;
    fn_8132FFF0();
    fn_8132B114();
    fn_8132B678();
    fn_8132CCF4();
    Fx_EvalShape(0, &lbl_814DE638 + 0xA7D0, &lbl_814AD5F0);
    fn_8132DEFC();
    temp_f31 = Menu_Get()->unkD8;
    if (fn_8132CCF4() > temp_f31) {
        fn_81333C7C(&lbl_814DE638 + 0xA7D0);
    }
    fn_8132BD00();
    fn_813355E0(&lbl_814DE638 + 0xA7D0);
    temp_r31 = fn_81334C08() << 5;
    memcpy((&lbl_814DE638 + 0x10000 + (lbl_814AD5F0 << 5)) - 0x5830, fn_81334C1C(), temp_r31);
    temp_r31_2 = fn_81334C08();
    memcpy((&lbl_814DE638 + 0x20000 + lbl_814AD5F0) - 0x4C10, fn_81334C10(), temp_r31_2);
    lbl_814AD5F0 += fn_81334C08();
    if (((s16) lbl_814DE638.unkA7A6 != 0) && ((s16) lbl_814DE638.unkA7C2 == 0)) {
        fn_81334C28();
    }
    fn_8132BF20();
}

/* 0x8132A3D4 */
void FxGame_Update(s16 arg0, s16 arg1) {
    f32 temp_f31;
    s16 *var_r28;
    s16 temp_r0;
    s16 temp_r0_2;
    s16 temp_r0_3;
    s16 temp_r0_4;
    s16 temp_r0_5;
    s16 temp_r0_6;
    s32 temp_r30;
    s32 temp_r30_2;
    s32 var_r29;

    fn_8132D084(arg0, arg1, &lbl_814DE638);
    if ((s16) lbl_814DE638.unkA7B4 != 3) {
        memset(&lbl_814DE638, 0, 0xA798);
        memset(&lbl_814DE638 + 0x201F0, 0, 0xC);
        var_r29 = 0;
        var_r28 = &lbl_814DE638 + 0x1B3F0;
        do {
            *var_r28 = 0xFF;
            fn_8132CEA4(var_r29, 1);
            fn_8132CE3C(var_r29, 1);
            fn_8132CF7C(var_r29, 1);
            var_r29 += 1;
            var_r28 += 1;
        } while (var_r29 < 0x861);
        fn_8133370C();
        lbl_814DE638.unkA796 = (s16) Menu_Get()->unkD6;
    }
    temp_r0 = Menu_Get()->unk12A;
    lbl_814AD5E8.unk0 = (s8) temp_r0;
    lbl_814AD5E0.unk0 = (s8) temp_r0;
    lbl_814AD5D8.unk0 = (s8) temp_r0;
    temp_r0_2 = Menu_Get()->unk12E;
    lbl_814AD5E8.unk1 = (s8) temp_r0_2;
    lbl_814AD5E0.unk1 = (s8) temp_r0_2;
    lbl_814AD5D8.unk1 = (s8) temp_r0_2;
    temp_r0_3 = Menu_Get()->unk132;
    lbl_814AD5E8.unk2 = (s8) temp_r0_3;
    lbl_814AD5E0.unk2 = (s8) temp_r0_3;
    lbl_814AD5D8.unk2 = (s8) temp_r0_3;
    lbl_814AD5D8.unk3 = 0xFF;
    temp_r0_4 = Menu_Get()->unk12C;
    lbl_814AD5EC.unk0 = (s8) temp_r0_4;
    lbl_814AD5E4.unk0 = (s8) temp_r0_4;
    lbl_814AD5DC.unk0 = (s8) temp_r0_4;
    temp_r0_5 = Menu_Get()->unk130;
    lbl_814AD5EC.unk1 = (s8) temp_r0_5;
    lbl_814AD5E4.unk1 = (s8) temp_r0_5;
    lbl_814AD5DC.unk1 = (s8) temp_r0_5;
    temp_r0_6 = Menu_Get()->unk134;
    lbl_814AD5EC.unk2 = (s8) temp_r0_6;
    lbl_814AD5E4.unk2 = (s8) temp_r0_6;
    lbl_814AD5DC.unk2 = (s8) temp_r0_6;
    lbl_814AD5DC.unk3 = 0xFF;
    if ((s16) Menu_Get()->unk128 >= 0) {
        lbl_814DE638.unkA794 = (s16) Menu_Get()->unk128;
    }
    if ((s16) lbl_814DE638.unkA79A == 0) {
        fn_8132CCF4();
        Fx_EvalShape(1, &lbl_814DE638 + 0xA7D0, &lbl_814AD5F0);
        fn_8132EC54();
    } else {
        if ((s16) lbl_814DE638.unkA7B6 == 0) {
            lbl_814DE638.unkA794 = 0;
            lbl_814DE638.unkA796 = (s16) Menu_Get()->unkD6;
            lbl_814AD5D4 = Rand() % 5;
        }
        fn_8132CCF4();
        fn_81330374(&lbl_814DE638 + 0xA7D0, &lbl_814AD5F0);
        fn_8132EC54();
    }
    fn_8132FFF0();
    fn_8132C5E0();
    temp_f31 = Menu_Get()->unkD8;
    if (fn_8132CCF4() > temp_f31) {
        fn_8133374C(&lbl_814DE638 + 0xA7D0, lbl_814AD5F0);
    }
    fn_8132BD00();
    fn_81336F9C(&lbl_814DE638 + 0xA7D0);
    temp_r30 = fn_81336968() << 5;
    memcpy((&lbl_814DE638 + 0x10000 + (lbl_814AD5F0 << 5)) - 0x5830, fn_8133697C(), temp_r30);
    temp_r30_2 = fn_81336968();
    memcpy((&lbl_814DE638 + 0x20000 + lbl_814AD5F0) - 0x4C10, fn_81336970(), temp_r30_2);
    lbl_814AD5F0 += fn_81336968();
    fn_8132BF20();
}

/* 0x8132A690 */
void fn_8132A690(s16 *arg0, s16 arg1, void *arg2) {
    s32 sp14;
    s32 var_r3;
    s32 var_r4;
    s32 var_r5;
    s32 var_r6;
    s32 var_r7;
    s32 var_r8;

    OSGetTime();
    OSTicksToCalendarTime(&sp14);
    var_r3 = sp28;
    var_r4 = sp24;
    var_r5 = sp20;
    var_r6 = sp1C;
    var_r7 = sp18;
    var_r8 = sp14;
    switch (arg1) {                                 /* irregular */
    case 0:
        break;
    case 1:
        var_r3 = (s32) arg2->unk0;
        var_r4 = (s32) arg2->unk2;
        var_r5 = (s32) arg2->unk4;
        break;
    case 2:
        var_r6 = (s32) arg2->unk0;
        var_r7 = (s32) arg2->unk2;
        var_r8 = (s32) arg2->unk4;
        break;
    }
    arg0->unk0 = (s16) var_r3;
    arg0->unk2 = (s16) (var_r4 + 1);
    arg0->unk4 = (s16) var_r5;
    arg0->unk6 = (s16) var_r6;
    arg0->unk8 = (s16) var_r7;
    arg0->unkA = (s16) var_r8;
}

/* 0x8132A75C */
void fn_8132A75C(void) {
    s32 var_r18;
    s32 var_r18_10;
    s32 var_r18_11;
    s32 var_r18_2;
    s32 var_r18_3;
    s32 var_r18_4;
    s32 var_r18_5;
    s32 var_r18_6;
    s32 var_r18_7;
    s32 var_r18_8;
    s32 var_r18_9;
    s32 var_r21;
    s32 var_r21_2;
    s32 var_r21_3;
    s32 var_r22;
    s32 var_r22_2;
    s32 var_r22_3;
    s32 var_r24;
    u16 *temp_r22;
    u16 *temp_r24;
    u16 *temp_r26;
    u16 *temp_r27;
    u16 *temp_r30;
    u16 *var_r19;
    u16 *var_r19_10;
    u16 *var_r19_11;
    u16 *var_r19_12;
    u16 *var_r19_13;
    u16 *var_r19_14;
    u16 *var_r19_15;
    u16 *var_r19_16;
    u16 *var_r19_17;
    u16 *var_r19_2;
    u16 *var_r19_3;
    u16 *var_r19_4;
    u16 *var_r19_5;
    u16 *var_r19_6;
    u16 *var_r19_7;
    u16 *var_r19_8;
    u16 *var_r19_9;
    u16 *var_r20;
    u16 temp_r23;
    u16 temp_r25;
    u16 temp_r28;
    u16 temp_r29;
    u16 temp_r31;

    temp_r22 = fn_813327B8(&lbl_814DE638);
    temp_r23 = fn_81332820();
    temp_r24 = fn_81332C34();
    temp_r25 = fn_81332C9C();
    temp_r26 = fn_8133285C();
    temp_r27 = fn_81332900();
    temp_r28 = fn_813328C4();
    temp_r29 = fn_81332968();
    temp_r30 = fn_81331008();
    temp_r31 = fn_81331070();
    switch ((s16) lbl_814DE638.unkA7A0) {           /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        var_r18 = 0;
        var_r20 = temp_r24;
loop_8:
        if (var_r18 < (s32) temp_r25) {
            fn_8132CF14(*var_r20, 1);
            var_r20 += 2;
            var_r18 += 1;
            goto loop_8;
        }
        break;
    case 1:                                         /* switch 1 */
        var_r18_2 = 0;
        var_r19 = temp_r22;
loop_12:
        if (var_r18_2 < (s32) temp_r23) {
            fn_8132CF14(*var_r19, 1);
            var_r19 += 2;
            var_r18_2 += 1;
            goto loop_12;
        }
        break;
    case 2:                                         /* switch 1 */
        var_r18_3 = 0;
        var_r19_2 = temp_r30;
loop_16:
        if (var_r18_3 < (s32) temp_r31) {
            fn_8132CF14(*var_r19_2, 1);
            var_r19_2 += 2;
            var_r18_3 += 1;
            goto loop_16;
        }
        break;
    }
    switch ((s16) lbl_814DE638.unkA79E) {           /* switch 2; irregular */
    case 0:                                         /* switch 2 */
        var_r18_4 = 0;
        var_r19_3 = temp_r24;
loop_26:
        if (var_r18_4 < (s32) temp_r25) {
            fn_8132CE3C((s32) *var_r19_3, 1);
            var_r19_3 += 2;
            var_r18_4 += 1;
            goto loop_26;
        }
        var_r18_5 = 0;
        var_r19_4 = temp_r22;
loop_29:
        if (var_r18_5 < (s32) temp_r23) {
            fn_8132CE3C((s32) *var_r19_4, 1);
            var_r19_4 += 2;
            var_r18_5 += 1;
            goto loop_29;
        }
        var_r18_6 = 0;
        var_r19_5 = temp_r30;
loop_32:
        if (var_r18_6 < (s32) temp_r31) {
            fn_8132CE3C((s32) *var_r19_5, 1);
            var_r19_5 += 2;
            var_r18_6 += 1;
            goto loop_32;
        }
        fn_8132EEB0(0, 0);
        fn_8132EEB0(1, 0);
        fn_8132EEB0(2, 0);
        fn_8132EEB0(3, 0);
        fn_8132EEB0(4, 0);
        fn_8132EEB0(5, 0);
        return;
    case 1:                                         /* switch 2 */
        var_r18_7 = 0;
        var_r19_6 = temp_r24;
loop_36:
        if (var_r18_7 < (s32) temp_r25) {
            fn_8132CE3C((s32) *var_r19_6, 1);
            var_r19_6 += 2;
            var_r18_7 += 1;
            goto loop_36;
        }
        var_r18_8 = 0;
        var_r19_7 = temp_r22;
loop_39:
        if (var_r18_8 < (s32) temp_r23) {
            fn_8132CE3C((s32) *var_r19_7, 0);
            var_r19_7 += 2;
            var_r18_8 += 1;
            goto loop_39;
        }
        var_r18_9 = 0;
        var_r19_8 = temp_r30;
loop_42:
        if (var_r18_9 < (s32) temp_r31) {
            fn_8132CE3C((s32) *var_r19_8, 0);
            var_r19_8 += 2;
            var_r18_9 += 1;
            goto loop_42;
        }
        switch ((s16) lbl_814DE638.unkA7A4) {       /* switch 3; irregular */
        case 0:                                     /* switch 3 */
            fn_8132EEB0(0, 0);
            fn_8132EEB0(1, 1);
            fn_8132EEB0(2, 0);
            fn_8132EEB0(3, 0);
            fn_8132EEB0(4, 0);
            fn_8132EEB0(5, 0);
            break;
        case 1:                                     /* switch 3 */
            fn_8132EEB0(0, 1);
            fn_8132EEB0(1, 0);
            fn_8132EEB0(2, 0);
            fn_8132EEB0(3, 0);
            fn_8132EEB0(4, 0);
            fn_8132EEB0(5, 0);
            break;
        }
        if ((s16) lbl_814DE638.unkA7BA == 1) {
            var_r18_10 = 0;
            var_r19_9 = temp_r24;
loop_52:
            if (var_r18_10 < (s32) temp_r25) {
                fn_8132CF7C((s32) *var_r19_9, 0);
                var_r19_9 += 2;
                var_r18_10 += 1;
                goto loop_52;
            }
            return;
        }
        return;
    case 2:                                         /* switch 2 */
        var_r18_11 = 0;
        var_r19_10 = temp_r24;
loop_56:
        if (var_r18_11 < (s32) temp_r25) {
            fn_8132CE3C((s32) *var_r19_10, 0);
            var_r19_10 += 2;
            var_r18_11 += 1;
            goto loop_56;
        }
        var_r24 = 0;
        var_r19_11 = temp_r22;
loop_59:
        if (var_r24 < (s32) temp_r23) {
            fn_8132CE3C((s32) *var_r19_11, 1);
            var_r19_11 += 2;
            var_r24 += 1;
            goto loop_59;
        }
        var_r22 = 0;
        var_r19_12 = temp_r30;
loop_62:
        if (var_r22 < (s32) temp_r31) {
            fn_8132CE3C((s32) *var_r19_12, 0);
            var_r19_12 += 2;
            var_r22 += 1;
            goto loop_62;
        }
        if ((s16) lbl_814DE638.unkA7BA == 2) {
            var_r22_2 = 0;
            var_r19_13 = temp_r26;
loop_66:
            if (var_r22_2 < (s32) temp_r28) {
                fn_8132CF7C((s32) *var_r19_13, 0);
                var_r19_13 += 2;
                var_r22_2 += 1;
                goto loop_66;
            }
            var_r22_3 = 0;
            var_r19_14 = temp_r27;
loop_69:
            if (var_r22_3 < (s32) temp_r29) {
                fn_8132CF7C((s32) *var_r19_14, 0);
                var_r19_14 += 2;
                var_r22_3 += 1;
                goto loop_69;
            }
        }
        fn_8132EEB0(0, 0);
        fn_8132EEB0(1, 0);
        if ((s16) lbl_814DE638.unkA7A2 <= -0x20) {
            fn_8132EEB0(2, 0);
        } else {
            fn_8132EEB0(2, 1);
        }
        if ((s16) lbl_814DE638.unkA7A2 >= 0x20) {
            fn_8132EEB0(3, 0);
        } else {
            fn_8132EEB0(3, 1);
        }
        fn_8132EEB0(4, 0);
        fn_8132EEB0(5, 0);
        return;
    case 3:                                         /* switch 2 */
        var_r21 = 0;
        var_r19_15 = temp_r24;
loop_79:
        if (var_r21 < (s32) temp_r25) {
            fn_8132CE3C((s32) *var_r19_15, 0);
            var_r19_15 += 2;
            var_r21 += 1;
            goto loop_79;
        }
        var_r21_2 = 0;
        var_r19_16 = temp_r22;
loop_82:
        if (var_r21_2 < (s32) temp_r23) {
            fn_8132CE3C((s32) *var_r19_16, 0);
            var_r19_16 += 2;
            var_r21_2 += 1;
            goto loop_82;
        }
        var_r21_3 = 0;
        var_r19_17 = temp_r30;
loop_85:
        if (var_r21_3 < (s32) temp_r31) {
            fn_8132CE3C((s32) *var_r19_17, 1);
            var_r19_17 += 2;
            var_r21_3 += 1;
            goto loop_85;
        }
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
            return;
        }
        fn_8132EEB0(5, 1);
        break;
    }
}

/* 0x8132ADCC */
void fn_8132ADCC(void) {
    s16 sp10;
    s32 var_r22;
    s32 var_r22_2;
    s32 var_r22_3;
    s32 var_r23;
    s32 var_r24_3;
    s32 var_r24_4;
    s32 var_r25;
    s32 var_r26;
    s32 var_r26_2;
    s32 var_r26_3;
    u16 *temp_r24;
    u16 *temp_r26;
    u16 *temp_r28;
    u16 *temp_r28_2;
    u16 *temp_r28_3;
    u16 *temp_r29;
    u16 *var_r23_2;
    u16 *var_r23_3;
    u16 *var_r23_4;
    u16 *var_r23_5;
    u16 *var_r23_6;
    u16 *var_r23_7;
    u16 *var_r24;
    u16 *var_r24_2;
    u16 temp_r25;
    u16 temp_r27;
    u16 temp_r30;
    u16 temp_r31;

    var_r22 = 0;
    var_r24 = fn_813327B8();
loop_2:
    if (var_r22 < (s32) fn_81332820()) {
        fn_8132CF7C((s32) *var_r24, 0);
        var_r24 += 2;
        var_r22 += 1;
        goto loop_2;
    }
    var_r22_2 = 0;
    var_r24_2 = fn_81332C34();
loop_5:
    if (var_r22_2 < (s32) fn_81332C9C()) {
        fn_8132CF7C((s32) *var_r24_2, 0);
        var_r24_2 += 2;
        var_r22_2 += 1;
        goto loop_5;
    }
    temp_r28 = fn_81332670();
    temp_r29 = fn_813325CC();
    temp_r30 = fn_813326D8();
    temp_r31 = fn_81332634();
    temp_r24 = fn_81332714();
    temp_r25 = fn_8133277C();
    temp_r26 = fn_81332CD8();
    var_r22_3 = 0;
    temp_r27 = fn_81332D40();
    var_r23 = 0;
loop_8:
    if (var_r22_3 < (s32) fn_81331070()) {
        fn_8132CF7C((s32) *(fn_81331008() + var_r23), 1);
        var_r23 += 2;
        var_r22_3 += 1;
        goto loop_8;
    }
    switch ((s16) lbl_814E8DD0.unkC) {              /* irregular */
    case 0:
        var_r26 = 0;
        var_r23_2 = temp_r24;
loop_15:
        if (var_r26 < (s32) temp_r25) {
            fn_8132CF7C((s32) *var_r23_2, 1);
            var_r23_2 += 2;
            var_r26 += 1;
            goto loop_15;
        }
        fn_8132EEB0(0, 0);
        fn_8132EEB0(1, 1);
        fn_8132EEB0(2, 0);
        fn_8132EEB0(3, 0);
        break;
    case 1:
        var_r25 = 0;
        var_r23_3 = temp_r26;
loop_19:
        if (var_r25 < (s32) temp_r27) {
            fn_8132CF7C((s32) *var_r23_3, 1);
            var_r23_3 += 2;
            var_r25 += 1;
            goto loop_19;
        }
        fn_8132EEB0(0, 1);
        fn_8132EEB0(1, 0);
        fn_8132EEB0(2, 0);
        fn_8132EEB0(3, 0);
        break;
    }
    if ((s16) lbl_814E8DD0.unkA < 0) {
        var_r26_2 = 0;
        var_r23_4 = temp_r29;
loop_24:
        if (var_r26_2 < (s32) temp_r31) {
            fn_8132CF7C((s32) *var_r23_4, 1);
            var_r23_4 += 2;
            var_r26_2 += 1;
            goto loop_24;
        }
    } else if ((s16) lbl_814E8DD0.unkA > 0) {
        var_r26_3 = 0;
        var_r23_5 = temp_r28;
loop_29:
        if (var_r26_3 < (s32) temp_r30) {
            fn_8132CF7C((s32) *var_r23_5, 1);
            var_r23_5 += 2;
            var_r26_3 += 1;
            goto loop_29;
        }
    }
    fn_8132C660(&sp10, (s16) fabs((f64) lbl_814E8DD0.unkA));
    temp_r28_2 = fn_8133285C();
    var_r24_3 = 0;
    var_r23_6 = fn_81332D7C(sp12);
loop_32:
    if (var_r24_3 < (s32) fn_813328C4()) {
        fn_8132CF7C((s32) temp_r28_2[*var_r23_6], 1);
        var_r23_6 += 2;
        var_r24_3 += 1;
        goto loop_32;
    }
    temp_r28_3 = fn_81332900();
    var_r24_4 = 0;
    var_r23_7 = fn_81332D7C(sp10);
loop_35:
    if (var_r24_4 < (s32) fn_81332968()) {
        fn_8132CF7C((s32) temp_r28_3[*var_r23_7], 1);
        var_r23_7 += 2;
        var_r24_4 += 1;
        goto loop_35;
    }
}

/* 0x8132B114 */
void fn_8132B114(void) {
    s16 sp10;
    s16 sp8;
    s32 temp_r31_10;
    s32 temp_r31_11;
    s32 temp_r31_12;
    s32 temp_r31_13;
    s32 temp_r31_14;
    s32 temp_r31_5;
    s32 temp_r31_6;
    s32 temp_r31_7;
    s32 temp_r31_8;
    s32 temp_r31_9;
    s32 var_r29;
    s32 var_r29_10;
    s32 var_r29_11;
    s32 var_r29_12;
    s32 var_r29_13;
    s32 var_r29_14;
    s32 var_r29_15;
    s32 var_r29_16;
    s32 var_r29_2;
    s32 var_r29_3;
    s32 var_r29_4;
    s32 var_r29_5;
    s32 var_r29_6;
    s32 var_r29_7;
    s32 var_r29_8;
    s32 var_r29_9;
    s32 var_r31_3;
    u16 *temp_r31;
    u16 *temp_r31_2;
    u16 *temp_r31_3;
    u16 *temp_r31_4;
    u16 *var_r30;
    u16 *var_r30_10;
    u16 *var_r30_11;
    u16 *var_r30_12;
    u16 *var_r30_13;
    u16 *var_r30_14;
    u16 *var_r30_15;
    u16 *var_r30_2;
    u16 *var_r30_3;
    u16 *var_r30_4;
    u16 *var_r30_5;
    u16 *var_r30_6;
    u16 *var_r30_7;
    u16 *var_r30_8;
    u16 *var_r30_9;
    u16 *var_r31;
    u16 *var_r31_2;

    fn_8132A690(&sp10, lbl_814E8DD0.unkE, &lbl_814E8DD0 + 0x16);
    var_r29 = 0;
    var_r31 = fn_813315C8();
loop_2:
    if (var_r29 < (s32) fn_81331630()) {
        fn_8132CF7C((s32) *var_r31, 0);
        var_r31 += 2;
        var_r29 += 1;
        goto loop_2;
    }
    var_r29_2 = 0;
    var_r31_2 = fn_813311F4();
loop_5:
    if (var_r29_2 < (s32) fn_81331258()) {
        fn_8132CF7C((s32) *var_r31_2, 0);
        var_r31_2 += 2;
        var_r29_2 += 1;
        goto loop_5;
    }
    fn_8132C660(&sp8, sp10);
    temp_r31 = fn_813320AC();
    var_r29_3 = 0;
    var_r30 = fn_81332D7C(spE);
loop_8:
    if (var_r29_3 < (s32) fn_81332114()) {
        fn_8132CF7C((s32) temp_r31[*var_r30], 1);
        var_r30 += 2;
        var_r29_3 += 1;
        goto loop_8;
    }
    temp_r31_2 = fn_81332150();
    var_r29_4 = 0;
    var_r30_2 = fn_81332D7C(spC);
loop_11:
    if (var_r29_4 < (s32) fn_813321B8()) {
        fn_8132CF7C((s32) temp_r31_2[*var_r30_2], 1);
        var_r30_2 += 2;
        var_r29_4 += 1;
        goto loop_11;
    }
    temp_r31_3 = fn_813321F4();
    var_r29_5 = 0;
    var_r30_3 = fn_81332D7C(spA);
loop_14:
    if (var_r29_5 < (s32) fn_8133225C()) {
        fn_8132CF7C((s32) temp_r31_3[*var_r30_3], 1);
        var_r30_3 += 2;
        var_r29_5 += 1;
        goto loop_14;
    }
    temp_r31_4 = fn_81332298();
    var_r29_6 = 0;
    var_r30_4 = fn_81332D7C(sp8);
loop_17:
    if (var_r29_6 < (s32) fn_81332300()) {
        fn_8132CF7C((s32) temp_r31_4[*var_r30_4], 1);
        var_r30_4 += 2;
        var_r29_6 += 1;
        goto loop_17;
    }
    fn_8132C660(&sp8, sp12);
    temp_r31_5 = fn_81331CD4();
    var_r29_7 = 0;
    var_r30_5 = fn_81332D7C(spA);
loop_20:
    if (var_r29_7 < (s32) fn_81331D3C()) {
        fn_8132CF7C((s32) *(temp_r31_5 + (*var_r30_5 * 2)), 1);
        var_r30_5 += 2;
        var_r29_7 += 1;
        goto loop_20;
    }
    temp_r31_6 = fn_81331D78();
    var_r29_8 = 0;
    var_r30_6 = fn_81332D7C(sp8);
loop_23:
    if (var_r29_8 < (s32) fn_81331DE0()) {
        fn_8132CF7C((s32) *(temp_r31_6 + (*var_r30_6 * 2)), 1);
        var_r30_6 += 2;
        var_r29_8 += 1;
        goto loop_23;
    }
    fn_8132C660(&sp8, sp14);
    temp_r31_7 = fn_81331710();
    var_r29_9 = 0;
    var_r30_7 = fn_81332D7C(spA);
loop_26:
    if (var_r29_9 < (s32) fn_81331778()) {
        fn_8132CF7C((s32) *(temp_r31_7 + (*var_r30_7 * 2)), 1);
        var_r30_7 += 2;
        var_r29_9 += 1;
        goto loop_26;
    }
    temp_r31_8 = fn_813317B4();
    var_r29_10 = 0;
    var_r30_8 = fn_81332D7C(sp8);
loop_29:
    if (var_r29_10 < (s32) fn_8133181C()) {
        fn_8132CF7C((s32) *(temp_r31_8 + (*var_r30_8 * 2)), 1);
        var_r30_8 += 2;
        var_r29_10 += 1;
        goto loop_29;
    }
    fn_8132C660(&sp8, sp16);
    temp_r31_9 = fn_813318FC();
    var_r29_11 = 0;
    var_r30_9 = fn_81332D7C(spA);
loop_32:
    if (var_r29_11 < (s32) fn_81331964()) {
        fn_8132CF7C((s32) *(temp_r31_9 + (*var_r30_9 * 2)), 1);
        var_r30_9 += 2;
        var_r29_11 += 1;
        goto loop_32;
    }
    temp_r31_10 = fn_813319A0();
    var_r29_12 = 0;
    var_r30_10 = fn_81332D7C(sp8);
loop_35:
    if (var_r29_12 < (s32) fn_81331A08()) {
        fn_8132CF7C((s32) *(temp_r31_10 + (*var_r30_10 * 2)), 1);
        var_r30_10 += 2;
        var_r29_12 += 1;
        goto loop_35;
    }
    fn_8132C660(&sp8, sp18);
    temp_r31_11 = fn_81331AE8();
    var_r29_13 = 0;
    var_r30_11 = fn_81332D7C(spA);
loop_38:
    if (var_r29_13 < (s32) fn_81331B50()) {
        fn_8132CF7C((s32) *(temp_r31_11 + (*var_r30_11 * 2)), 1);
        var_r30_11 += 2;
        var_r29_13 += 1;
        goto loop_38;
    }
    temp_r31_12 = fn_81331B8C();
    var_r29_14 = 0;
    var_r30_12 = fn_81332D7C(sp8);
loop_41:
    if (var_r29_14 < (s32) fn_81331BF4()) {
        fn_8132CF7C((s32) *(temp_r31_12 + (*var_r30_12 * 2)), 1);
        var_r30_12 += 2;
        var_r29_14 += 1;
        goto loop_41;
    }
    fn_8132C660(&sp8, sp1A);
    temp_r31_13 = fn_81331EC0();
    var_r29_15 = 0;
    var_r30_13 = fn_81332D7C(spA);
loop_44:
    if (var_r29_15 < (s32) fn_81331F28()) {
        fn_8132CF7C((s32) *(temp_r31_13 + (*var_r30_13 * 2)), 1);
        var_r30_13 += 2;
        var_r29_15 += 1;
        goto loop_44;
    }
    temp_r31_14 = fn_81331F64();
    var_r29_16 = 0;
    var_r30_14 = fn_81332D7C(sp8);
loop_47:
    if (var_r29_16 < (s32) fn_81331FCC()) {
        fn_8132CF7C((s32) *(temp_r31_14 + (*var_r30_14 * 2)), 1);
        var_r30_14 += 2;
        var_r29_16 += 1;
        goto loop_47;
    }
    var_r31_3 = 0;
    var_r30_15 = fn_81331294();
loop_50:
    if (var_r31_3 < (s32) fn_813312FC()) {
        fn_8132CF7C((s32) *var_r30_15, 1);
        var_r30_15 += 2;
        var_r31_3 += 1;
        goto loop_50;
    }
}

/* 0x8132B678 */
void fn_8132B678(void) {
    s32 var_r28;
    s32 var_r28_2;
    s32 var_r28_3;
    s32 var_r28_4;
    s32 var_r28_5;
    s32 var_r28_6;
    s32 var_r28_7;
    s32 var_r28_8;
    s32 var_r30_10;
    s32 var_r30_11;
    s32 var_r30_12;
    s32 var_r30_13;
    s32 var_r30_14;
    s32 var_r30_8;
    s32 var_r30_9;
    s32 var_r31;
    s32 var_r31_2;
    u16 *var_r29;
    u16 *var_r29_10;
    u16 *var_r29_2;
    u16 *var_r29_3;
    u16 *var_r29_4;
    u16 *var_r29_5;
    u16 *var_r29_6;
    u16 *var_r29_7;
    u16 *var_r29_8;
    u16 *var_r29_9;
    u16 *var_r30;
    u16 *var_r30_2;
    u16 *var_r30_3;
    u16 *var_r30_4;
    u16 *var_r30_5;
    u16 *var_r30_6;
    u16 *var_r30_7;

    if ((s16) lbl_814DE638.unkA7A8 != 0) {
        var_r28 = 0;
        var_r30 = fn_813315C8();
loop_3:
        if (var_r28 < (s32) fn_81331630()) {
            fn_8132CDD4(*var_r30, 0);
            fn_8132CEA4((s32) *var_r30, 0);
            var_r30 += 2;
            var_r28 += 1;
            goto loop_3;
        }
        var_r28_2 = 0;
        var_r30_2 = fn_813311F4();
loop_6:
        if (var_r28_2 < (s32) fn_81331258()) {
            fn_8132CDD4(*var_r30_2, 0);
            fn_8132CEA4((s32) *var_r30_2, 1);
            fn_8132CF14(*var_r30_2, 1);
            var_r30_2 += 2;
            var_r28_2 += 1;
            goto loop_6;
        }
    } else {
        var_r28_3 = 0;
        var_r30_3 = fn_813315C8();
loop_10:
        if (var_r28_3 < (s32) fn_81331630()) {
            fn_8132CDD4(*var_r30_3, 0);
            fn_8132CEA4((s32) *var_r30_3, 1);
            fn_8132CF14(*var_r30_3, 1);
            var_r30_3 += 2;
            var_r28_3 += 1;
            goto loop_10;
        }
        var_r28_4 = 0;
        var_r30_4 = fn_813311F4();
loop_13:
        if (var_r28_4 < (s32) fn_81331258()) {
            fn_8132CDD4(*var_r30_4, 0);
            fn_8132CEA4((s32) *var_r30_4, 0);
            var_r30_4 += 2;
            var_r28_4 += 1;
            goto loop_13;
        }
    }
    if ((s16) lbl_814DE638.unkA7A6 == 1) {
        if ((u16) Menu_Get()->unk17E == 0) {
            var_r28_5 = 0;
            var_r30_5 = fn_813315C8();
loop_18:
            if (var_r28_5 < (s32) fn_81331630()) {
                fn_8132CEA4((s32) *var_r30_5, 0);
                var_r30_5 += 2;
                var_r28_5 += 1;
                goto loop_18;
            }
        }
        switch ((s16) lbl_814DE638.unkA7AA) {       /* switch 1; irregular */
        case 0:                                     /* switch 1 */
            var_r28_6 = 0;
            var_r30_6 = fn_813320AC();
loop_27:
            if (var_r28_6 < (s32) fn_81332114()) {
                fn_8132CE3C((s32) *var_r30_6, 1);
                fn_8132CDD4(*var_r30_6, 1);
                fn_8132CEA4((s32) *var_r30_6, 0);
                var_r30_6 += 2;
                var_r28_6 += 1;
                goto loop_27;
            }
            var_r28_7 = 0;
            var_r30_7 = fn_81332150();
loop_30:
            if (var_r28_7 < (s32) fn_813321B8()) {
                fn_8132CE3C((s32) *var_r30_7, 1);
                fn_8132CDD4(*var_r30_7, 1);
                fn_8132CEA4((s32) *var_r30_7, 0);
                var_r30_7 += 2;
                var_r28_7 += 1;
                goto loop_30;
            }
            var_r28_8 = 0;
            var_r29 = fn_813321F4();
loop_35:
            if (var_r28_8 < (s32) fn_8133225C()) {
                if ((s16) lbl_814DE638.unkA7C2 == 1) {
                    fn_8132CF7C((s32) *var_r29, 0);
                }
                fn_8132CE3C((s32) *var_r29, 1);
                fn_8132CDD4(*var_r29, 1);
                fn_8132CEA4((s32) *var_r29, 0);
                var_r29 += 2;
                var_r28_8 += 1;
                goto loop_35;
            }
            var_r30_8 = 0;
            var_r29_2 = fn_81332298();
loop_40:
            if (var_r30_8 < (s32) fn_81332300()) {
                if ((s16) lbl_814DE638.unkA7C2 == 1) {
                    fn_8132CF7C((s32) *var_r29_2, 0);
                }
                fn_8132CE3C((s32) *var_r29_2, 1);
                fn_8132CDD4(*var_r29_2, 1);
                fn_8132CEA4((s32) *var_r29_2, 0);
                var_r29_2 += 2;
                var_r30_8 += 1;
                goto loop_40;
            }
            return;
        case 1:                                     /* switch 1 */
            var_r31 = 0;
            var_r29_3 = fn_81331C30();
loop_46:
            if (var_r31 < (s32) fn_81331C98()) {
                if ((s16) lbl_814DE638.unkA7C2 == 1) {
                    fn_8132CF7C((s32) *var_r29_3, 0);
                }
                fn_8132CE3C((s32) *var_r29_3, 1);
                fn_8132CDD4(*var_r29_3, 1);
                fn_8132CEA4((s32) *var_r29_3, 0);
                var_r29_3 += 2;
                var_r31 += 1;
                goto loop_46;
            }
            return;
        case 2:                                     /* switch 1 */
            var_r31_2 = 0;
            var_r29_4 = fn_8133166C();
loop_52:
            if (var_r31_2 < (s32) fn_813316D4()) {
                if ((s16) lbl_814DE638.unkA7C2 == 1) {
                    fn_8132CF7C((s32) *var_r29_4, 0);
                }
                fn_8132CE3C((s32) *var_r29_4, 1);
                fn_8132CDD4(*var_r29_4, 1);
                fn_8132CEA4((s32) *var_r29_4, 0);
                var_r29_4 += 2;
                var_r31_2 += 1;
                goto loop_52;
            }
            return;
        }
    } else if ((s16) lbl_814DE638.unkA7A6 == 2) {
        if ((u16) Menu_Get()->unk17E == 0) {
            var_r30_9 = 0;
            var_r29_5 = fn_813311F4();
loop_58:
            if (var_r30_9 < (s32) fn_81331258()) {
                fn_8132CEA4((s32) *var_r29_5, 0);
                var_r29_5 += 2;
                var_r30_9 += 1;
                goto loop_58;
            }
        }
        switch ((s16) lbl_814DE638.unkA7AC) {       /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            var_r30_10 = 0;
            var_r29_6 = fn_81331858();
loop_69:
            if (var_r30_10 < (s32) fn_813318C0()) {
                if ((s16) lbl_814DE638.unkA7C2 == 2) {
                    fn_8132CF7C((s32) *var_r29_6, 0);
                }
                fn_8132CE3C((s32) *var_r29_6, 1);
                fn_8132CDD4(*var_r29_6, 1);
                fn_8132CEA4((s32) *var_r29_6, 0);
                var_r29_6 += 2;
                var_r30_10 += 1;
                goto loop_69;
            }
            return;
        case 1:                                     /* switch 2 */
            var_r30_11 = 0;
            var_r29_7 = fn_81331A44();
loop_75:
            if (var_r30_11 < (s32) fn_81331AAC()) {
                if ((s16) lbl_814DE638.unkA7C2 == 2) {
                    fn_8132CF7C((s32) *var_r29_7, 0);
                }
                fn_8132CE3C((s32) *var_r29_7, 1);
                fn_8132CDD4(*var_r29_7, 1);
                fn_8132CEA4((s32) *var_r29_7, 0);
                var_r29_7 += 2;
                var_r30_11 += 1;
                goto loop_75;
            }
            return;
        case 2:                                     /* switch 2 */
            var_r30_12 = 0;
            var_r29_8 = fn_81331E1C();
loop_81:
            if (var_r30_12 < (s32) fn_81331E84()) {
                if ((s16) lbl_814DE638.unkA7C2 == 2) {
                    fn_8132CF7C((s32) *var_r29_8, 0);
                }
                fn_8132CE3C((s32) *var_r29_8, 1);
                fn_8132CDD4(*var_r29_8, 1);
                fn_8132CEA4((s32) *var_r29_8, 0);
                var_r29_8 += 2;
                var_r30_12 += 1;
                goto loop_81;
            }
            return;
        }
    } else {
        var_r30_13 = 0;
        var_r29_9 = fn_813315C8();
loop_85:
        if (var_r30_13 < (s32) fn_81331630()) {
            fn_8132CE3C((s32) *var_r29_9, 1);
            var_r29_9 += 2;
            var_r30_13 += 1;
            goto loop_85;
        }
        var_r30_14 = 0;
        var_r29_10 = fn_813311F4();
loop_88:
        if (var_r30_14 < (s32) fn_81331258()) {
            fn_8132CE3C((s32) *var_r29_10, 1);
            var_r29_10 += 2;
            var_r30_14 += 1;
            goto loop_88;
        }
    }
}

/* 0x8132BD00 */
void fn_8132BD00(void) {
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f28;
    f32 temp_f28_2;
    f32 temp_f29;
    f32 temp_f2;
    f32 temp_f2_2;
    f32 temp_f2_3;
    s16 *var_r26;
    s16 *var_r27;
    s16 *var_r29;
    u32 var_r31;
    void *temp_r25;
    void *var_r28;

    temp_r25 = fn_8133491C(&lbl_814DE638);
    var_r31 = 0;
    temp_f29 = fn_8132CCF4();
    var_r28 = temp_r25;
    var_r29 = &lbl_814DE638 + 0x1B3F0;
    var_r27 = &lbl_814DE638 + 0xA7D0;
    var_r26 = &lbl_814DE638 + 0x1BC54;
loop_2:
    if (var_r31 < (u32) lbl_814AD5F0) {
        *var_r29 = (s8) (255.0f * (temp_f29 * fn_8132CC00(var_r31)));
        temp_f2 = var_r28->unk14;
        temp_f1 = temp_f2 * fn_8132CC78(var_r31);
        var_r27->unk14 = (f32) (var_r27->unk14 + temp_f1);
        temp_f2_2 = var_r28->unk18;
        temp_f1_2 = temp_f2_2 * fn_8132CC78(var_r31, temp_f1, temp_f2);
        var_r27->unk18 = (f32) (var_r27->unk18 + temp_f1_2);
        temp_f2_3 = var_r28->unk1C;
        temp_f1_3 = temp_f2_3 * fn_8132CC78(var_r31, temp_f1_2, temp_f2_2);
        var_r27->unk1C = (f32) (var_r27->unk1C + temp_f1_3);
        var_r27->unkC = (s16) ((f32) var_r28->unkC * fn_8132CC78(var_r31, temp_f1_3, temp_f2_3));
        var_r27->unkE = (s16) ((f32) var_r28->unkE * fn_8132CC78(var_r31));
        var_r27->unk10 = (s16) ((f32) var_r28->unk10 * fn_8132CC78(var_r31));
        temp_f28 = fn_8132CB88(var_r31);
        fn_8132CD5C(var_r31);
        fn_81330518(&lbl_814AD5E0, &lbl_814AD5E8, &lbl_814AD5D8, var_r26, temp_f28);
        temp_f28_2 = fn_8132CB88(var_r31);
        fn_8132CD5C(var_r31);
        fn_81330518(&lbl_814AD5E4, &lbl_814AD5EC, &lbl_814AD5DC, var_r26 + 0x2184, temp_f28_2);
        var_r29 += 1;
        var_r28 += 0x20;
        var_r27 += 0x20;
        var_r26 += 4;
        var_r31 += 1;
        goto loop_2;
    }
}

/* 0x8132BF20 */
void fn_8132BF20(void) {
    s16 *var_r30;
    u32 var_r29;

    var_r29 = 0U;
    var_r30 = &lbl_814DE638;
loop_2:
    if (var_r29 < (u32) lbl_814AD5F0) {
        fn_8132BFF8(var_r30, var_r30->unk10C2);
        fn_8132BFF8(var_r30 + 0x2184, var_r30->unk3246);
        fn_8132BFF8(var_r30 + 0x4308, var_r30->unk53CA);
        fn_8132BFF8(var_r30 + 0x8610, var_r30->unk96D2);
        fn_8132BFF8(var_r30 + 0x648C, var_r30->unk754E);
        var_r30 += 2;
        var_r29 += 1;
        goto loop_2;
    }
    fn_8132BFF8(&lbl_814DE638 + 0xA794, lbl_814DE638.unkA796);
    fn_8132BFF8(&lbl_814DE638 + 0x201F0, 0xA);
    fn_8132BFF8(&lbl_814DE638 + 0x1FF60, 0xA);
}

/* 0x8132BFF8 */
void fn_8132BFF8(s16 *arg0, s16 arg1) {
    s16 temp_r5;

    temp_r5 = *arg0;
    if (temp_r5 > arg1) {
        *arg0 = temp_r5 - 1;
        return;
    }
    if (temp_r5 < arg1) {
        *arg0 = temp_r5 + 1;
    }
}

/* 0x8132C024 */
void fn_8132C024(void) {
    s32 var_r30;
    s32 var_r30_10;
    s32 var_r30_11;
    s32 var_r30_12;
    s32 var_r30_2;
    s32 var_r30_3;
    s32 var_r30_4;
    s32 var_r30_5;
    s32 var_r30_6;
    s32 var_r30_7;
    s32 var_r30_8;
    s32 var_r30_9;
    s32 var_r31;
    s32 var_r31_10;
    s32 var_r31_11;
    s32 var_r31_12;
    s32 var_r31_2;
    s32 var_r31_3;
    s32 var_r31_4;
    s32 var_r31_5;
    s32 var_r31_6;
    s32 var_r31_7;
    s32 var_r31_8;
    s32 var_r31_9;

    switch ((s16) lbl_814E8DD0.unk6) {              /* irregular */
    case 1:
        var_r30 = 0;
        var_r31 = 0;
loop_8:
        if (var_r30 < (s32) fn_81332C9C()) {
            fn_8132CDD4(*(fn_81332C34() + var_r31), 1);
            var_r31 += 2;
            var_r30 += 1;
            goto loop_8;
        }
        var_r30_2 = 0;
        var_r31_2 = 0;
loop_11:
        if (var_r30_2 < (s32) fn_81332820()) {
            fn_8132CDD4(*(fn_813327B8() + var_r31_2), 0);
            var_r31_2 += 2;
            var_r30_2 += 1;
            goto loop_11;
        }
        var_r30_3 = 0;
        var_r31_3 = 0;
loop_14:
        if (var_r30_3 < (s32) fn_81331070()) {
            fn_8132CDD4(*(fn_81331008() + var_r31_3), 0);
            var_r31_3 += 2;
            var_r30_3 += 1;
            goto loop_14;
        }
        return;
    case 2:
        var_r30_4 = 0;
        var_r31_4 = 0;
loop_18:
        if (var_r30_4 < (s32) fn_81332C9C()) {
            fn_8132CDD4(*(fn_81332C34() + var_r31_4), 0);
            var_r31_4 += 2;
            var_r30_4 += 1;
            goto loop_18;
        }
        var_r30_5 = 0;
        var_r31_5 = 0;
loop_21:
        if (var_r30_5 < (s32) fn_81332820()) {
            fn_8132CDD4(*(fn_813327B8() + var_r31_5), 1);
            var_r31_5 += 2;
            var_r30_5 += 1;
            goto loop_21;
        }
        var_r30_6 = 0;
        var_r31_6 = 0;
loop_24:
        if (var_r30_6 < (s32) fn_81331070()) {
            fn_8132CDD4(*(fn_81331008() + var_r31_6), 0);
            var_r31_6 += 2;
            var_r30_6 += 1;
            goto loop_24;
        }
        return;
    case 3:
        var_r30_7 = 0;
        var_r31_7 = 0;
loop_28:
        if (var_r30_7 < (s32) fn_81332C9C()) {
            fn_8132CDD4(*(fn_81332C34() + var_r31_7), 0);
            var_r31_7 += 2;
            var_r30_7 += 1;
            goto loop_28;
        }
        var_r30_8 = 0;
        var_r31_8 = 0;
loop_31:
        if (var_r30_8 < (s32) fn_81332820()) {
            fn_8132CDD4(*(fn_813327B8() + var_r31_8), 0);
            var_r31_8 += 2;
            var_r30_8 += 1;
            goto loop_31;
        }
        var_r30_9 = 0;
        var_r31_9 = 0;
loop_34:
        if (var_r30_9 < (s32) fn_81331070()) {
            fn_8132CDD4(*(fn_81331008() + var_r31_9), 1);
            var_r31_9 += 2;
            var_r30_9 += 1;
            goto loop_34;
        }
        return;
    default:
        var_r30_10 = 0;
        var_r31_10 = 0;
loop_38:
        if (var_r30_10 < (s32) fn_81332C9C()) {
            fn_8132CDD4(*(fn_81332C34() + var_r31_10), 0);
            var_r31_10 += 2;
            var_r30_10 += 1;
            goto loop_38;
        }
        var_r30_11 = 0;
        var_r31_11 = 0;
loop_41:
        if (var_r30_11 < (s32) fn_81332820()) {
            fn_8132CDD4(*(fn_813327B8() + var_r31_11), 0);
            var_r31_11 += 2;
            var_r30_11 += 1;
            goto loop_41;
        }
        var_r30_12 = 0;
        var_r31_12 = 0;
loop_44:
        if (var_r30_12 < (s32) fn_81331070()) {
            fn_8132CDD4(*(fn_81331008() + var_r31_12), 0);
            var_r31_12 += 2;
            var_r30_12 += 1;
            goto loop_44;
        }
        return;
    }
}

/* 0x8132C2FC */
void fn_8132C2FC(void) {
    s32 var_r30;
    s32 var_r30_10;
    s32 var_r30_11;
    s32 var_r30_12;
    s32 var_r30_2;
    s32 var_r30_3;
    s32 var_r30_4;
    s32 var_r30_5;
    s32 var_r30_6;
    s32 var_r30_7;
    s32 var_r30_8;
    s32 var_r30_9;
    s32 var_r31;
    s32 var_r31_10;
    s32 var_r31_11;
    s32 var_r31_12;
    s32 var_r31_2;
    s32 var_r31_3;
    s32 var_r31_4;
    s32 var_r31_5;
    s32 var_r31_6;
    s32 var_r31_7;
    s32 var_r31_8;
    s32 var_r31_9;

    if ((s16) lbl_814E8DD0.unk6 == 0) {
        switch ((s16) lbl_814E8DD0.unk8) {          /* irregular */
        case 0:
            var_r30 = 0;
            var_r31 = 0;
loop_9:
            if (var_r30 < (s32) fn_81332C9C()) {
                fn_8132CEA4((s32) *(fn_81332C34() + var_r31), 1);
                var_r31 += 2;
                var_r30 += 1;
                goto loop_9;
            }
            var_r30_2 = 0;
            var_r31_2 = 0;
loop_12:
            if (var_r30_2 < (s32) fn_81332820()) {
                fn_8132CEA4((s32) *(fn_813327B8() + var_r31_2), 0);
                var_r31_2 += 2;
                var_r30_2 += 1;
                goto loop_12;
            }
            var_r30_3 = 0;
            var_r31_3 = 0;
loop_15:
            if (var_r30_3 < (s32) fn_81331070()) {
                fn_8132CEA4((s32) *(fn_81331008() + var_r31_3), 0);
                var_r31_3 += 2;
                var_r30_3 += 1;
                goto loop_15;
            }
            return;
        case 1:
            var_r30_4 = 0;
            var_r31_4 = 0;
loop_19:
            if (var_r30_4 < (s32) fn_81332C9C()) {
                fn_8132CEA4((s32) *(fn_81332C34() + var_r31_4), 0);
                var_r31_4 += 2;
                var_r30_4 += 1;
                goto loop_19;
            }
            var_r30_5 = 0;
            var_r31_5 = 0;
loop_22:
            if (var_r30_5 < (s32) fn_81332820()) {
                fn_8132CEA4((s32) *(fn_813327B8() + var_r31_5), 1);
                var_r31_5 += 2;
                var_r30_5 += 1;
                goto loop_22;
            }
            var_r30_6 = 0;
            var_r31_6 = 0;
loop_25:
            if (var_r30_6 < (s32) fn_81331070()) {
                fn_8132CEA4((s32) *(fn_81331008() + var_r31_6), 0);
                var_r31_6 += 2;
                var_r30_6 += 1;
                goto loop_25;
            }
            return;
        case 2:
            var_r30_7 = 0;
            var_r31_7 = 0;
loop_29:
            if (var_r30_7 < (s32) fn_81332C9C()) {
                fn_8132CEA4((s32) *(fn_81332C34() + var_r31_7), 0);
                var_r31_7 += 2;
                var_r30_7 += 1;
                goto loop_29;
            }
            var_r30_8 = 0;
            var_r31_8 = 0;
loop_32:
            if (var_r30_8 < (s32) fn_81332820()) {
                fn_8132CEA4((s32) *(fn_813327B8() + var_r31_8), 0);
                var_r31_8 += 2;
                var_r30_8 += 1;
                goto loop_32;
            }
            var_r30_9 = 0;
            var_r31_9 = 0;
loop_35:
            if (var_r30_9 < (s32) fn_81331070()) {
                fn_8132CEA4((s32) *(fn_81331008() + var_r31_9), 1);
                var_r31_9 += 2;
                var_r30_9 += 1;
                goto loop_35;
            }
            return;
        }
    } else {
        var_r30_10 = 0;
        var_r31_10 = 0;
loop_39:
        if (var_r30_10 < (s32) fn_81332C9C()) {
            fn_8132CEA4((s32) *(fn_81332C34() + var_r31_10), 0);
            var_r31_10 += 2;
            var_r30_10 += 1;
            goto loop_39;
        }
        var_r30_11 = 0;
        var_r31_11 = 0;
loop_42:
        if (var_r30_11 < (s32) fn_81332820()) {
            fn_8132CEA4((s32) *(fn_813327B8() + var_r31_11), 0);
            var_r31_11 += 2;
            var_r30_11 += 1;
            goto loop_42;
        }
        var_r30_12 = 0;
        var_r31_12 = 0;
loop_45:
        if (var_r30_12 < (s32) fn_81331070()) {
            fn_8132CEA4((s32) *(fn_81331008() + var_r31_12), 0);
            var_r31_12 += 2;
            var_r30_12 += 1;
            goto loop_45;
        }
    }
}

/* 0x8132C5E0 */
void fn_8132C5E0(void) {
    s32 var_r31;
    s32 var_r31_2;

    var_r31 = 0;
    do {
        fn_8132CEA4(var_r31, 1);
        fn_8132CE3C(var_r31, 1);
        fn_8132CDD4((u16) var_r31, 0);
        fn_8132CF7C(var_r31, 0);
        var_r31 += 1;
    } while (var_r31 < 0x861);
    var_r31_2 = 0;
    do {
        fn_8132CF7C(var_r31_2, 0);
        var_r31_2 += 1;
    } while (var_r31_2 < 0x861);
}

/* 0x8132C660 */
void fn_8132C660(s16 *arg0, s16 arg1) {
    s16 var_r4;

    var_r4 = arg1;
    if ((var_r4 < 0) || (var_r4 > 0x270F)) {
        arg0->unk0 = 0;
        arg0->unk2 = 0;
        arg0->unk4 = 0;
        arg0->unk6 = 0;
    }
    if (var_r4 > 0x3E7) {
        arg0->unk6 = (s16) (arg1 / 1000);
        var_r4 -= arg0->unk6 * 0x3E8;
    } else {
        arg0->unk6 = 0;
    }
    if (var_r4 > 0x63) {
        arg0->unk4 = (s16) (var_r4 / 100);
        var_r4 -= arg0->unk4 * 0x64;
    } else {
        arg0->unk4 = 0;
    }
    if (var_r4 > 9) {
        arg0->unk2 = (s16) (var_r4 / 10);
        var_r4 -= arg0->unk2 * 0xA;
    } else {
        arg0->unk2 = 0;
    }
    arg0->unk0 = var_r4;
}

/* 0x8132C750 */
void fn_8132C750(s16 arg0, s16 arg1, s16 arg2, void *arg3, void *arg4, f32 farg0, f32 farg1, f32 farg2) {
    u8 sp3C[0x4];
    f32 sp38;
    f32 sp34;
    f32 sp30;
    f32 sp24;

    MtxRot(&sp3C);
    sp30 = arg3->unk14 - farg0;
    sp34 = arg3->unk18 - farg1;
    sp38 = arg3->unk1C - farg2;
    PSMTXMultVec(&sp3C, &sp30, &sp24);
    arg4->unk14 = (f32) ((sp24 + farg0) - arg3->unk14);
    arg4->unk18 = (f32) ((sp28 + farg1) - arg3->unk18);
    arg4->unk1C = (f32) ((sp2C + farg2) - arg3->unk1C);
    arg4->unkC = arg0;
    arg4->unkE = arg1;
    arg4->unk10 = arg2;
}

/* 0x8132C830 */
void fn_8132C830(void *arg0, void *arg1, u16 *arg2, u32 arg3) {
    f32 sp24;
    f32 sp20;
    f32 sp1C;
    f32 temp_f0;
    f32 temp_f0_2;
    f32 temp_f1;
    f32 temp_f5;
    f32 temp_f6;
    f32 var_f4;
    f32 var_f5;
    f32 var_f6;
    u16 *var_r5;
    u16 temp_r8;
    u32 temp_r0;
    u32 temp_r0_2;
    u32 var_ctr;
    u32 var_ctr_2;
    u32 var_ctr_3;
    u32 var_ctr_4;
    u32 var_r5_2;
    u32 var_r7;
    void *temp_r9;
    void *temp_r9_2;
    void *temp_r9_3;
    void *temp_r9_4;
    void *temp_r9_5;
    void *temp_r9_6;
    void *temp_r9_7;
    void *temp_r9_8;
    void *temp_r9_9;
    void *var_r4;

    var_r4 = arg1;
    var_r5 = arg2;
    var_f6 = 0.0f;
    var_f5 = 0.0f;
    var_f4 = 0.0f;
    if (var_r5 != NULL) {
        var_r7 = arg3;
        if (arg3 > 0U) {
            temp_r0 = var_r7 >> 3U;
            var_ctr = temp_r0;
            if (temp_r0 != 0) {
                do {
                    temp_r9 = var_r4 + (var_r5->unk0 << 5);
                    temp_r9_2 = var_r4 + (var_r5->unk2 << 5);
                    temp_r9_3 = var_r4 + (var_r5->unk4 << 5);
                    temp_r9_4 = var_r4 + (var_r5->unk6 << 5);
                    temp_r9_5 = var_r4 + (var_r5->unk8 << 5);
                    temp_r9_6 = var_r4 + (var_r5->unkA << 5);
                    temp_r9_7 = var_r4 + (var_r5->unkC << 5);
                    temp_r9_8 = var_r4 + (var_r5->unkE << 5);
                    var_f4 = var_f4 + temp_r9->unk14 + temp_r9_2->unk14 + temp_r9_3->unk14 + temp_r9_4->unk14 + temp_r9_5->unk14 + temp_r9_6->unk14 + temp_r9_7->unk14 + temp_r9_8->unk14;
                    var_f5 = var_f5 + temp_r9->unk18 + temp_r9_2->unk18 + temp_r9_3->unk18 + temp_r9_4->unk18 + temp_r9_5->unk18 + temp_r9_6->unk18 + temp_r9_7->unk18 + temp_r9_8->unk18;
                    var_r5 += 0x10;
                    var_f6 = var_f6 + temp_r9->unk1C + temp_r9_2->unk1C + temp_r9_3->unk1C + temp_r9_4->unk1C + temp_r9_5->unk1C + temp_r9_6->unk1C + temp_r9_7->unk1C + temp_r9_8->unk1C;
                    var_ctr -= 1;
                } while (var_ctr != 0);
                var_r7 &= 7;
                if (var_r7 != 0) {
                    goto block_5;
                }
            } else {
block_5:
                var_ctr_2 = var_r7;
                do {
                    temp_r8 = *var_r5;
                    var_r5 += 2;
                    temp_r9_9 = var_r4 + (temp_r8 << 5);
                    var_f4 += temp_r9_9->unk14;
                    var_f5 += temp_r9_9->unk18;
                    var_f6 += temp_r9_9->unk1C;
                    var_ctr_2 -= 1;
                } while (var_ctr_2 != 0);
            }
        }
    } else {
        var_r5_2 = arg3;
        if (arg3 > 0U) {
            temp_r0_2 = var_r5_2 >> 3U;
            var_ctr_3 = temp_r0_2;
            if (temp_r0_2 != 0) {
                do {
                    temp_f5 = var_f5 + var_r4->unk18 + var_r4->unk38 + var_r4->unk58 + var_r4->unk78 + var_r4->unk98 + var_r4->unkB8 + var_r4->unkD8;
                    temp_f6 = var_f6 + var_r4->unk1C + var_r4->unk3C + var_r4->unk5C + var_r4->unk7C + var_r4->unk9C + var_r4->unkBC + var_r4->unkDC;
                    temp_f1 = var_r4->unkF8;
                    temp_f0 = var_r4->unkFC;
                    var_f4 = var_f4 + var_r4->unk14 + var_r4->unk34 + var_r4->unk54 + var_r4->unk74 + var_r4->unk94 + var_r4->unkB4 + var_r4->unkD4 + var_r4->unkF4;
                    var_r4 += 0x100;
                    var_f5 = temp_f5 + temp_f1;
                    var_f6 = temp_f6 + temp_f0;
                    var_ctr_3 -= 1;
                } while (var_ctr_3 != 0);
                var_r5_2 &= 7;
                if (var_r5_2 != 0) {
                    goto block_12;
                }
            } else {
block_12:
                var_ctr_4 = var_r5_2;
                do {
                    temp_f0_2 = var_r4->unk1C;
                    var_f4 += var_r4->unk14;
                    var_f5 += var_r4->unk18;
                    var_r4 += 0x20;
                    var_f6 += temp_f0_2;
                    var_ctr_4 -= 1;
                } while (var_ctr_4 != 0);
            }
        }
    }
    sp1C = lbl_8137EF80.unk0;
    sp20 = lbl_8137EF80.unk4;
    sp24 = lbl_8137EF80.unk8;
    sp1C = var_f4 / (f32) arg3;
    sp20 = var_f5 / (f32) arg3;
    sp24 = var_f6 / (f32) arg3;
    arg0->unk0 = sp1C;
    arg0->unk4 = sp20;
    arg0->unk8 = sp24;
}

/* 0x8132CB60 */
s32 fn_8132CB60(f32 farg0) {
    return (s32) ((65535.0f * farg0) / 360.0f);
}

/* 0x8132CB88 */
f32 fn_8132CB88(u32 arg0) {
    return (f32) (&lbl_814DE638)[arg0].unk10C2 / (f32) Menu_Get()->unkAA;
}

/* 0x8132CC00 */
f32 fn_8132CC00(u32 arg0) {
    return (f32) (&lbl_814DE638)[arg0].unk2184 / (f32) Menu_Get()->unkA8;
}

/* 0x8132CC78 */
f32 fn_8132CC78(u32 arg0) {
    return (f32) (&lbl_814DE638 + 0x10000)[arg0].unk-79F0 / (f32) Menu_Get()->unkAA;
}

/* 0x8132CCF4 */
f32 fn_8132CCF4(void) {
    return (f32) lbl_814DE638.unkA794 / (f32) Menu_Get()->unkD6;
}

/* 0x8132CD5C */
f32 fn_8132CD5C(u32 arg0) {
    return (f32) (&lbl_814DE638)[arg0].unk648C / (f32) Menu_Get()->unkA8;
}

/* 0x8132CDD4 */
void fn_8132CDD4(u16 arg0, s32 arg1) {
    if (arg1 != 0) {
        (&lbl_814DE638)[arg0].unk10C2 = (s16) Menu_Get()->unkAA;
        return;
    }
    (&lbl_814DE638)[arg0].unk10C2 = 0;
}

/* 0x8132CE3C */
void fn_8132CE3C(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        (&lbl_814DE638)[arg0].unk53CA = (s16) Menu_Get()->unkA8;
        return;
    }
    (&lbl_814DE638)[arg0].unk53CA = 0;
}

/* 0x8132CEA4 */
void fn_8132CEA4(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        (&lbl_814DE638 + 0x10000)[arg0].unk-692E = (s16) Menu_Get()->unkAA;
        return;
    }
    (&lbl_814DE638 + 0x10000)[arg0].unk-692E = 0;
}

/* 0x8132CF14 */
void fn_8132CF14(u16 arg0, s32 arg1) {
    if (arg1 != 0) {
        (&lbl_814DE638)[arg0].unk754E = (s16) Menu_Get()->unkA8;
        return;
    }
    (&lbl_814DE638)[arg0].unk754E = 0;
}

/* 0x8132CF7C */
void fn_8132CF7C(s32 arg0, s32 arg1) {
    s16 *temp_r3;
    s16 *temp_r3_2;
    s16 temp_r0;

    if (arg1 != 0) {
        temp_r0 = Menu_Get()->unkA8;
        temp_r3 = &(&lbl_814DE638)[arg0];
        temp_r3->unk3246 = temp_r0;
        temp_r3->unk2184 = temp_r0;
        return;
    }
    temp_r3_2 = &(&lbl_814DE638)[arg0];
    temp_r3_2->unk3246 = 0;
    temp_r3_2->unk2184 = 0;
}

/* 0x8132CFE8 */
void *fn_8132CFE8(void) {
    return &lbl_814E8DD0;
}

/* 0x8132CFF4 */
void *fn_8132CFF4(void) {
    return &lbl_814E8DEC;
}
