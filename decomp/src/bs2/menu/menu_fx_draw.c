// Non-matching: functionally equivalent decompilation
//
// Mosaic cube text ("fx") used inside Game Play, Options and Calendar,
// 0x81333030..0x813376A0: per-frame update, instanced cube drawing, sparks.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x81333030 */
void Fx_Draw(u8 arg0) {
    s16 sp1A;
    s16 sp18;
    s16 sp16;
    s16 sp14;
    s16 sp12;
    s16 sp10;
    s16 spE;
    s16 spC;
    void *var_r24;
    f32 *var_r23;
    f32 temp_f0;
    f32 temp_f30;
    f32 temp_f30_2;
    f32 temp_f31;
    s16 *temp_r20;
    s16 var_r18;
    s32 temp_r19_2;
    u32 temp_r28;
    u32 var_r21;
    u8 *var_r27;
    void *temp_r18;
    void *temp_r19;
    void *temp_r3;
    void *temp_r3_2;
    void *var_r25;
    void *var_r26;

    var_r27 = Fx_GetDrawBuf();
    temp_r28 = Fx_GetCount();
    temp_r18 = fn_81329B54();
    temp_r19 = fn_81329B60();
    temp_f30 = *Menu_Get();
    temp_f0 = Menu_Get()->unk4;
    temp_f31 = temp_f30 - temp_f0;
    var_r23 = &lbl_8153D618;
    temp_f30_2 = temp_f30 + temp_f0;
    var_r26 = temp_r18;
    var_r24 = &lbl_814FE8A0;
    var_r25 = temp_r19;
    var_r18 = 0;
    var_r21 = 0U;
loop_13:
    if (var_r21 < temp_r28) {
        sp14 = (s16) var_r26->unk0;
        sp16 = (s16) var_r26->unk1;
        sp18 = (s16) var_r26->unk2;
        sp1A = 0xFF;
        spC = (s16) var_r25->unk0;
        spE = (s16) var_r25->unk1;
        sp10 = (s16) var_r25->unk2;
        sp12 = 0xFF;
        temp_r3 = var_r24->unk68->unk10;
        temp_r19_2 = temp_r3->unk34;
        temp_r20 = temp_r3->unk30;
        temp_r3->unk30 = &sp14;
        var_r24->unk68->unk10->unk34 = &spC;
        var_r24->unk74 = (s16) (u8) ((var_r26->unk3 * (arg0 * *var_r27)) / 65025);
        if ((s16) var_r24->unk74 > 0xA) {
            if (((u32) lbl_814AD600 == 0) && (*var_r23 > temp_f31)) {
                if (var_r18 == 0) {
                    J3D_DrawPacket(var_r24, var_r24->unk68->unk8 + 0x18);
                    var_r18 += 1;
                } else {
                    fn_813052E8(var_r24, var_r24->unk68->unk8 + 0x18);
                }
            }
            if (((u32) lbl_814AD600 == 1) && (*var_r23 < temp_f30_2)) {
                if (var_r18 == 0) {
                    J3D_DrawPacket(var_r24, var_r24->unk68->unk8 + 0x18);
                    var_r18 += 1;
                } else {
                    fn_813052E8(var_r24, var_r24->unk68->unk8 + 0x18);
                }
            }
        }
        var_r26 += 4;
        var_r25 += 4;
        var_r23 += 4;
        var_r21 += 1;
        var_r24->unk68->unk10->unk30 = temp_r20;
        var_r27 += 1;
        temp_r3_2 = var_r24->unk68;
        var_r24 += 0x78;
        temp_r3_2->unk10->unk34 = temp_r19_2;
        goto loop_13;
    }
    lbl_814AD600 ^= 1;
}

/* 0x81333250 */
void Fx_Update(void) {
    u8 sp20[0x4];
    f32 sp1C;
    f32 sp18;
    f32 sp14;
    f32 sp10;
    f32 spC;
    f32 sp8;
    f32 *var_r28;
    u32 temp_r31;
    u32 var_r26;
    void *var_r27;
    void *var_r29;

    temp_r31 = Fx_GetCount();
    var_r26 = 0U;
    var_r28 = &lbl_8153D618;
    var_r27 = &lbl_814FE8A0 + (0 * 0x78);
    var_r29 = Fx_GetCubes();
loop_2:
    if (var_r26 < temp_r31) {
        MtxRotTrans(var_r29->unkC, var_r29->unkE, var_r29->unk10, &sp20, var_r29->unk14, var_r29->unk18, var_r29->unk1C);
        *var_r28 = var_r29->unk18;
        sp14 = var_r29->unk0;
        sp18 = var_r29->unk4;
        sp1C = var_r29->unk8;
        sp8 = sp14;
        spC = sp18;
        sp10 = sp1C;
        J3D_SetModelMtx(var_r27, &sp20, &sp8, sp18);
        J3D_SetViewMtx(var_r27, fn_81302F94());
        J3D_CalcJoints(var_r27);
        var_r29 += 0x20;
        var_r28 += 4;
        var_r27 += 0x78;
        var_r26 += 1;
        goto loop_2;
    }
}

/* 0x81333344 */
void fn_81333344(void) {
    lbl_814AD608.unk0 = 0;
    lbl_814AD608.unk2 = 0;
    memset(&lbl_81541920, 0, 0xC80);
}

/* 0x81333384 */
void fn_81333384(void) {
    s16 temp_r0;
    s16 temp_r31;

    lbl_814AD608.unk2 = (u16) (lbl_814AD608.unk2 + 1);
    switch ((s16) lbl_814AD608.unk0) {              /* switch 1; irregular */
    case 1:                                         /* switch 1 */
        if ((s32) lbl_814AD608.unk2 >= (s16) Menu_Get()->unk124) {
            lbl_814AD608.unk2 = (u16) Menu_Get()->unk124;
        }
        break;
    case 2:                                         /* switch 1 */
        if ((s32) lbl_814AD608.unk2 >= (s16) Menu_Get()->unk126) {
            lbl_814AD608.unk2 = 0U;
            lbl_814AD608.unk0 = 0;
        }
        break;
    }
    temp_r31 = fn_8132CFF4()->unk6;
    if ((s16) fn_8132CFE8()->unk6 != temp_r31) {
        temp_r0 = fn_8132CFE8()->unk6;
        switch (temp_r0) {                          /* switch 2; irregular */
        case 2:                                     /* switch 2 */
            lbl_814AD608.unk0 = 1;
            lbl_814AD608.unk2 = 0U;
            return;
        case 0:                                     /* switch 2 */
            if ((s16) fn_8132CFF4()->unk6 == 2) {
                lbl_814AD608.unk0 = 2;
                lbl_814AD608.unk2 = 0U;
                return;
            }
            lbl_814AD608.unk0 = 0;
            return;
        default:                                    /* switch 2 */
        case 1:                                     /* switch 2 */
            lbl_814AD608.unk0 = 0;
            break;
        }
    }
}

/* 0x813334AC */
void *fn_813334AC(void) {
    u8 sp8[0x4];
    f32 temp_f2;
    f32 temp_f3;

    switch ((s16) lbl_814AD608.unk0) {              /* irregular */
    case 1:
        temp_f2 = (f32) Menu_Get()->unk124;
        Fx_EvalShape(3, &lbl_81541920, &sp8, (f32) lbl_814AD608.unk2 / temp_f2, temp_f2);
        break;
    case 2:
        temp_f3 = (f32) Menu_Get()->unk126;
        Fx_EvalShape(4, &lbl_81541920, &sp8, 1.0f - ((f32) lbl_814AD608.unk2 / temp_f3), temp_f3);
        break;
    }
    return &lbl_81541920;
}

/* 0x813335AC */
s32 fn_813335AC(void) {
    switch ((s16) lbl_814AD608) {                   /* irregular */
    case 1:
        return fn_81329B24(3);
    case 2:
        return fn_81329B24(4);
    case 0:
        return 0;
    default:
        return 0;
    }
}

/* 0x81333614 */
s32 fn_81333614(void) {
    switch ((s16) lbl_814AD608.unk0) {              /* irregular */
    case 2:
        return (s32) (255.0f - ((255.0f * (f32) lbl_814AD608.unk2) / (f32) Menu_Get()->unk126));
    case 1:
        return (s32) ((255.0f * (f32) lbl_814AD608.unk2) / (f32) Menu_Get()->unk124);
    default:
        return 0;
    }
}

/* 0x8133370C */
void fn_8133370C(void) {
    lbl_814AD610 = 0;
    lbl_814AD614 = NULL;
    memset(&lbl_815425A0, 0, 0x10C20);
}

/* 0x8133374C */
void fn_8133374C(void *arg0, u16 arg1) {
    f32 sp44;
    f32 sp40;
    u16 sp3E;
    u16 sp3C;
    f32 sp30;
    u8 sp10[0x20];
    void *var_r30;
    f32 temp_f2;
    u32 var_r29;
    void *var_r31;

    temp_f2 = (f32) Menu_Get()->unkC8;
    fn_8133498C(&sp44, &sp40, &sp3E, &sp3C, (f32) lbl_814AD610 / temp_f2, temp_f2);
    fn_8132C830(&sp30, arg0, NULL, arg1);
    var_r31 = arg0;
    var_r30 = &lbl_815425A0;
    var_r29 = 0U;
loop_2:
    if (var_r29 < arg1) {
        var_r31->unk14 = (f32) (var_r31->unk14 + sp44);
        var_r31->unk18 = (f32) (var_r31->unk18 + sp40);
        fn_8132C750(0, sp3E, sp3C, var_r31, &sp10, sp30, sp34, sp38);
        var_r31 += 0x20;
        var_r29 += 1;
        var_r30->unk14 = sp24;
        var_r30->unk18 = sp28;
        var_r30->unk1C = sp2C;
        var_r30->unkC = sp1C;
        var_r30->unkE = sp1E;
        var_r30->unk10 = sp20;
        var_r30 += 0x20;
        goto loop_2;
    }
    fn_81334928();
}

/* 0x81333898 */
void Fx_EmitSparks(void *arg0) {
    f32 sp20;
    f32 sp1C;
    u16 sp1A;
    u16 sp18;
    f32 spC;
    f32 temp_f2;
    s32 temp_r0;
    s32 temp_r0_10;
    s32 temp_r0_12;
    s32 temp_r0_13;
    s32 temp_r0_14;
    s32 temp_r0_16;
    s32 temp_r0_17;
    s32 temp_r0_18;
    s32 temp_r0_20;
    s32 temp_r0_2;
    s32 temp_r0_4;
    s32 temp_r0_5;
    s32 temp_r0_6;
    s32 temp_r0_8;
    s32 temp_r0_9;
    s32 var_r27;
    s32 var_r28;
    s32 var_r28_2;
    s32 var_r28_3;
    s32 var_r28_4;
    u16 *temp_r28;
    u16 *temp_r29;
    u16 *temp_r29_2;
    u16 *temp_r29_3;
    u16 *temp_r29_4;
    u16 *var_r29;
    u16 *var_r29_2;
    u16 *var_r29_3;
    u16 *var_r29_4;
    u16 *var_r29_5;
    u16 temp_r0_11;
    u16 temp_r0_15;
    u16 temp_r0_19;
    u16 temp_r0_3;
    u16 temp_r0_7;

    temp_f2 = (f32) Menu_Get(&lbl_815425A0)->unkC8;
    fn_8133498C(&sp20, &sp1C, &sp1A, &sp18, (f32) lbl_814AD610 / temp_f2, temp_f2);
    temp_r28 = fn_81332528();
    fn_8132C830(&spC, arg0, temp_r28, fn_81332590());
    var_r27 = 0;
    var_r29 = temp_r28;
loop_2:
    if (var_r27 < (s32) fn_81332590()) {
        temp_r0 = *var_r29 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0, &lbl_815425A0 + temp_r0, spC, sp10, sp14);
        var_r27 += 1;
        temp_r0_2 = (*var_r29 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_2) = *(&lbl_815425A0 + temp_r0_2) + sp20;
        temp_r0_3 = *var_r29;
        var_r29 += 2;
        temp_r0_4 = (temp_r0_3 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_4) = *(&lbl_815425A0 + temp_r0_4) + sp1C;
        goto loop_2;
    }
    temp_r29 = fn_8133285C();
    fn_8132C830(&spC, arg0, temp_r29, fn_813328C4());
    var_r28 = 0;
    var_r29_2 = temp_r29;
loop_5:
    if (var_r28 < (s32) fn_813328C4()) {
        temp_r0_5 = *var_r29_2 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_5, &lbl_815425A0 + temp_r0_5, spC, sp10, sp14);
        var_r28 += 1;
        temp_r0_6 = (*var_r29_2 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_6) = *(&lbl_815425A0 + temp_r0_6) + sp20;
        temp_r0_7 = *var_r29_2;
        var_r29_2 += 2;
        temp_r0_8 = (temp_r0_7 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_8) = *(&lbl_815425A0 + temp_r0_8) + sp1C;
        goto loop_5;
    }
    temp_r29_2 = fn_81332900();
    fn_8132C830(&spC, arg0, temp_r29_2, fn_81332968());
    var_r28_2 = 0;
    var_r29_3 = temp_r29_2;
loop_8:
    if (var_r28_2 < (s32) fn_81332968()) {
        temp_r0_9 = *var_r29_3 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_9, &lbl_815425A0 + temp_r0_9, spC, sp10, sp14);
        var_r28_2 += 1;
        temp_r0_10 = (*var_r29_3 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_10) = *(&lbl_815425A0 + temp_r0_10) + sp20;
        temp_r0_11 = *var_r29_3;
        var_r29_3 += 2;
        temp_r0_12 = (temp_r0_11 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_12) = *(&lbl_815425A0 + temp_r0_12) + sp1C;
        goto loop_8;
    }
    temp_r29_3 = fn_81332C34();
    fn_8132C830(&spC, arg0, temp_r29_3, fn_81332C9C());
    var_r28_3 = 0;
    var_r29_4 = temp_r29_3;
loop_11:
    if (var_r28_3 < (s32) fn_81332C9C()) {
        temp_r0_13 = *var_r29_4 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_13, &lbl_815425A0 + temp_r0_13, spC, sp10, sp14);
        var_r28_3 += 1;
        temp_r0_14 = (*var_r29_4 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_14) = *(&lbl_815425A0 + temp_r0_14) + sp20;
        temp_r0_15 = *var_r29_4;
        var_r29_4 += 2;
        temp_r0_16 = (temp_r0_15 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_16) = *(&lbl_815425A0 + temp_r0_16) + sp1C;
        goto loop_11;
    }
    temp_r29_4 = fn_81331008();
    fn_8132C830(&spC, arg0, temp_r29_4, fn_81331070());
    var_r28_4 = 0;
    var_r29_5 = temp_r29_4;
loop_14:
    if (var_r28_4 < (s32) fn_81331070()) {
        temp_r0_17 = *var_r29_5 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_17, &lbl_815425A0 + temp_r0_17, spC, sp10, sp14);
        var_r28_4 += 1;
        temp_r0_18 = (*var_r29_5 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_18) = *(&lbl_815425A0 + temp_r0_18) + sp20;
        temp_r0_19 = *var_r29_5;
        var_r29_5 += 2;
        temp_r0_20 = (temp_r0_19 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_20) = *(&lbl_815425A0 + temp_r0_20) + sp1C;
        goto loop_14;
    }
    fn_81334928();
}

/* 0x81333C7C */
void fn_81333C7C(void *arg0) {
    f32 sp20;
    f32 sp1C;
    u16 sp1A;
    u16 sp18;
    f32 spC;
    f32 temp_f2;
    s32 temp_r0;
    s32 temp_r0_10;
    s32 temp_r0_12;
    s32 temp_r0_13;
    s32 temp_r0_14;
    s32 temp_r0_16;
    s32 temp_r0_17;
    s32 temp_r0_18;
    s32 temp_r0_20;
    s32 temp_r0_21;
    s32 temp_r0_22;
    s32 temp_r0_24;
    s32 temp_r0_25;
    s32 temp_r0_26;
    s32 temp_r0_28;
    s32 temp_r0_29;
    s32 temp_r0_2;
    s32 temp_r0_30;
    s32 temp_r0_32;
    s32 temp_r0_33;
    s32 temp_r0_34;
    s32 temp_r0_36;
    s32 temp_r0_37;
    s32 temp_r0_38;
    s32 temp_r0_40;
    s32 temp_r0_41;
    s32 temp_r0_42;
    s32 temp_r0_44;
    s32 temp_r0_45;
    s32 temp_r0_46;
    s32 temp_r0_48;
    s32 temp_r0_49;
    s32 temp_r0_4;
    s32 temp_r0_50;
    s32 temp_r0_52;
    s32 temp_r0_53;
    s32 temp_r0_54;
    s32 temp_r0_56;
    s32 temp_r0_57;
    s32 temp_r0_58;
    s32 temp_r0_5;
    s32 temp_r0_60;
    s32 temp_r0_61;
    s32 temp_r0_62;
    s32 temp_r0_64;
    s32 temp_r0_65;
    s32 temp_r0_66;
    s32 temp_r0_68;
    s32 temp_r0_69;
    s32 temp_r0_6;
    s32 temp_r0_70;
    s32 temp_r0_72;
    s32 temp_r0_8;
    s32 temp_r0_9;
    s32 var_r27;
    s32 var_r28;
    s32 var_r28_10;
    s32 var_r28_11;
    s32 var_r28_12;
    s32 var_r28_13;
    s32 var_r28_14;
    s32 var_r28_15;
    s32 var_r28_16;
    s32 var_r28_17;
    s32 var_r28_2;
    s32 var_r28_3;
    s32 var_r28_4;
    s32 var_r28_5;
    s32 var_r28_6;
    s32 var_r28_7;
    s32 var_r28_8;
    s32 var_r28_9;
    u16 *temp_r28;
    u16 *temp_r29;
    u16 *temp_r29_10;
    u16 *temp_r29_11;
    u16 *temp_r29_12;
    u16 *temp_r29_13;
    u16 *temp_r29_14;
    u16 *temp_r29_15;
    u16 *temp_r29_16;
    u16 *temp_r29_17;
    u16 *temp_r29_2;
    u16 *temp_r29_3;
    u16 *temp_r29_4;
    u16 *temp_r29_5;
    u16 *temp_r29_6;
    u16 *temp_r29_7;
    u16 *temp_r29_8;
    u16 *temp_r29_9;
    u16 *var_r29;
    u16 *var_r29_10;
    u16 *var_r29_11;
    u16 *var_r29_12;
    u16 *var_r29_13;
    u16 *var_r29_14;
    u16 *var_r29_15;
    u16 *var_r29_16;
    u16 *var_r29_17;
    u16 *var_r29_18;
    u16 *var_r29_2;
    u16 *var_r29_3;
    u16 *var_r29_4;
    u16 *var_r29_5;
    u16 *var_r29_6;
    u16 *var_r29_7;
    u16 *var_r29_8;
    u16 *var_r29_9;
    u16 temp_r0_11;
    u16 temp_r0_15;
    u16 temp_r0_19;
    u16 temp_r0_23;
    u16 temp_r0_27;
    u16 temp_r0_31;
    u16 temp_r0_35;
    u16 temp_r0_39;
    u16 temp_r0_3;
    u16 temp_r0_43;
    u16 temp_r0_47;
    u16 temp_r0_51;
    u16 temp_r0_55;
    u16 temp_r0_59;
    u16 temp_r0_63;
    u16 temp_r0_67;
    u16 temp_r0_71;
    u16 temp_r0_7;

    temp_f2 = (f32) Menu_Get(&lbl_815425A0)->unkC8;
    fn_8133498C(&sp20, &sp1C, &sp1A, &sp18, (f32) lbl_814AD610 / temp_f2, temp_f2);
    temp_r28 = fn_81331338();
    fn_8132C830(&spC, arg0, temp_r28, fn_813313A0());
    var_r27 = 0;
    var_r29 = temp_r28;
loop_2:
    if (var_r27 < (s32) fn_813313A0()) {
        temp_r0 = *var_r29 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0, &lbl_815425A0 + temp_r0, spC, sp10, sp14);
        var_r27 += 1;
        temp_r0_2 = (*var_r29 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_2) = *(&lbl_815425A0 + temp_r0_2) + sp20;
        temp_r0_3 = *var_r29;
        var_r29 += 2;
        temp_r0_4 = (temp_r0_3 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_4) = *(&lbl_815425A0 + temp_r0_4) + sp1C;
        goto loop_2;
    }
    temp_r29 = fn_813313DC();
    fn_8132C830(&spC, arg0, temp_r29, fn_81331444());
    var_r28 = 0;
    var_r29_2 = temp_r29;
loop_5:
    if (var_r28 < (s32) fn_81331444()) {
        temp_r0_5 = *var_r29_2 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_5, &lbl_815425A0 + temp_r0_5, spC, sp10, sp14);
        var_r28 += 1;
        temp_r0_6 = (*var_r29_2 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_6) = *(&lbl_815425A0 + temp_r0_6) + sp20;
        temp_r0_7 = *var_r29_2;
        var_r29_2 += 2;
        temp_r0_8 = (temp_r0_7 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_8) = *(&lbl_815425A0 + temp_r0_8) + sp1C;
        goto loop_5;
    }
    temp_r29_2 = fn_81331480();
    fn_8132C830(&spC, arg0, temp_r29_2, fn_813314E8());
    var_r28_2 = 0;
    var_r29_3 = temp_r29_2;
loop_8:
    if (var_r28_2 < (s32) fn_813314E8()) {
        temp_r0_9 = *var_r29_3 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_9, &lbl_815425A0 + temp_r0_9, spC, sp10, sp14);
        var_r28_2 += 1;
        temp_r0_10 = (*var_r29_3 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_10) = *(&lbl_815425A0 + temp_r0_10) + sp20;
        temp_r0_11 = *var_r29_3;
        var_r29_3 += 2;
        temp_r0_12 = (temp_r0_11 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_12) = *(&lbl_815425A0 + temp_r0_12) + sp1C;
        goto loop_8;
    }
    temp_r29_3 = fn_81331524();
    fn_8132C830(&spC, arg0, temp_r29_3, fn_8133158C());
    var_r28_3 = 0;
    var_r29_4 = temp_r29_3;
loop_11:
    if (var_r28_3 < (s32) fn_8133158C()) {
        temp_r0_13 = *var_r29_4 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_13, &lbl_815425A0 + temp_r0_13, spC, sp10, sp14);
        var_r28_3 += 1;
        temp_r0_14 = (*var_r29_4 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_14) = *(&lbl_815425A0 + temp_r0_14) + sp20;
        temp_r0_15 = *var_r29_4;
        var_r29_4 += 2;
        temp_r0_16 = (temp_r0_15 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_16) = *(&lbl_815425A0 + temp_r0_16) + sp1C;
        goto loop_11;
    }
    temp_r29_4 = fn_813320AC();
    fn_8132C830(&spC, arg0, temp_r29_4, fn_81332114());
    var_r28_4 = 0;
    var_r29_5 = temp_r29_4;
loop_14:
    if (var_r28_4 < (s32) fn_81332114()) {
        temp_r0_17 = *var_r29_5 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_17, &lbl_815425A0 + temp_r0_17, spC, sp10, sp14);
        var_r28_4 += 1;
        temp_r0_18 = (*var_r29_5 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_18) = *(&lbl_815425A0 + temp_r0_18) + sp20;
        temp_r0_19 = *var_r29_5;
        var_r29_5 += 2;
        temp_r0_20 = (temp_r0_19 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_20) = *(&lbl_815425A0 + temp_r0_20) + sp1C;
        goto loop_14;
    }
    temp_r29_5 = fn_81332150();
    fn_8132C830(&spC, arg0, temp_r29_5, fn_813321B8());
    var_r28_5 = 0;
    var_r29_6 = temp_r29_5;
loop_17:
    if (var_r28_5 < (s32) fn_813321B8()) {
        temp_r0_21 = *var_r29_6 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_21, &lbl_815425A0 + temp_r0_21, spC, sp10, sp14);
        var_r28_5 += 1;
        temp_r0_22 = (*var_r29_6 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_22) = *(&lbl_815425A0 + temp_r0_22) + sp20;
        temp_r0_23 = *var_r29_6;
        var_r29_6 += 2;
        temp_r0_24 = (temp_r0_23 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_24) = *(&lbl_815425A0 + temp_r0_24) + sp1C;
        goto loop_17;
    }
    temp_r29_6 = fn_813321F4();
    fn_8132C830(&spC, arg0, temp_r29_6, fn_8133225C());
    var_r28_6 = 0;
    var_r29_7 = temp_r29_6;
loop_20:
    if (var_r28_6 < (s32) fn_8133225C()) {
        temp_r0_25 = *var_r29_7 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_25, &lbl_815425A0 + temp_r0_25, spC, sp10, sp14);
        var_r28_6 += 1;
        temp_r0_26 = (*var_r29_7 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_26) = *(&lbl_815425A0 + temp_r0_26) + sp20;
        temp_r0_27 = *var_r29_7;
        var_r29_7 += 2;
        temp_r0_28 = (temp_r0_27 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_28) = *(&lbl_815425A0 + temp_r0_28) + sp1C;
        goto loop_20;
    }
    temp_r29_7 = fn_81332298();
    fn_8132C830(&spC, arg0, temp_r29_7, fn_81332300());
    var_r28_7 = 0;
    var_r29_8 = temp_r29_7;
loop_23:
    if (var_r28_7 < (s32) fn_81332300()) {
        temp_r0_29 = *var_r29_8 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_29, &lbl_815425A0 + temp_r0_29, spC, sp10, sp14);
        var_r28_7 += 1;
        temp_r0_30 = (*var_r29_8 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_30) = *(&lbl_815425A0 + temp_r0_30) + sp20;
        temp_r0_31 = *var_r29_8;
        var_r29_8 += 2;
        temp_r0_32 = (temp_r0_31 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_32) = *(&lbl_815425A0 + temp_r0_32) + sp1C;
        goto loop_23;
    }
    temp_r29_8 = fn_81331CD4();
    fn_8132C830(&spC, arg0, temp_r29_8, fn_81331D3C());
    var_r28_8 = 0;
    var_r29_9 = temp_r29_8;
loop_26:
    if (var_r28_8 < (s32) fn_81331D3C()) {
        temp_r0_33 = *var_r29_9 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_33, &lbl_815425A0 + temp_r0_33, spC, sp10, sp14);
        var_r28_8 += 1;
        temp_r0_34 = (*var_r29_9 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_34) = *(&lbl_815425A0 + temp_r0_34) + sp20;
        temp_r0_35 = *var_r29_9;
        var_r29_9 += 2;
        temp_r0_36 = (temp_r0_35 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_36) = *(&lbl_815425A0 + temp_r0_36) + sp1C;
        goto loop_26;
    }
    temp_r29_9 = fn_81331D78();
    fn_8132C830(&spC, arg0, temp_r29_9, fn_81331DE0());
    var_r28_9 = 0;
    var_r29_10 = temp_r29_9;
loop_29:
    if (var_r28_9 < (s32) fn_81331DE0()) {
        temp_r0_37 = *var_r29_10 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_37, &lbl_815425A0 + temp_r0_37, spC, sp10, sp14);
        var_r28_9 += 1;
        temp_r0_38 = (*var_r29_10 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_38) = *(&lbl_815425A0 + temp_r0_38) + sp20;
        temp_r0_39 = *var_r29_10;
        var_r29_10 += 2;
        temp_r0_40 = (temp_r0_39 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_40) = *(&lbl_815425A0 + temp_r0_40) + sp1C;
        goto loop_29;
    }
    temp_r29_10 = fn_81331710();
    fn_8132C830(&spC, arg0, temp_r29_10, fn_81331778());
    var_r28_10 = 0;
    var_r29_11 = temp_r29_10;
loop_32:
    if (var_r28_10 < (s32) fn_81331778()) {
        temp_r0_41 = *var_r29_11 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_41, &lbl_815425A0 + temp_r0_41, spC, sp10, sp14);
        var_r28_10 += 1;
        temp_r0_42 = (*var_r29_11 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_42) = *(&lbl_815425A0 + temp_r0_42) + sp20;
        temp_r0_43 = *var_r29_11;
        var_r29_11 += 2;
        temp_r0_44 = (temp_r0_43 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_44) = *(&lbl_815425A0 + temp_r0_44) + sp1C;
        goto loop_32;
    }
    temp_r29_11 = fn_813317B4();
    fn_8132C830(&spC, arg0, temp_r29_11, fn_8133181C());
    var_r28_11 = 0;
    var_r29_12 = temp_r29_11;
loop_35:
    if (var_r28_11 < (s32) fn_8133181C()) {
        temp_r0_45 = *var_r29_12 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_45, &lbl_815425A0 + temp_r0_45, spC, sp10, sp14);
        var_r28_11 += 1;
        temp_r0_46 = (*var_r29_12 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_46) = *(&lbl_815425A0 + temp_r0_46) + sp20;
        temp_r0_47 = *var_r29_12;
        var_r29_12 += 2;
        temp_r0_48 = (temp_r0_47 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_48) = *(&lbl_815425A0 + temp_r0_48) + sp1C;
        goto loop_35;
    }
    temp_r29_12 = fn_813318FC();
    fn_8132C830(&spC, arg0, temp_r29_12, fn_81331964());
    var_r28_12 = 0;
    var_r29_13 = temp_r29_12;
loop_38:
    if (var_r28_12 < (s32) fn_81331964()) {
        temp_r0_49 = *var_r29_13 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_49, &lbl_815425A0 + temp_r0_49, spC, sp10, sp14);
        var_r28_12 += 1;
        temp_r0_50 = (*var_r29_13 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_50) = *(&lbl_815425A0 + temp_r0_50) + sp20;
        temp_r0_51 = *var_r29_13;
        var_r29_13 += 2;
        temp_r0_52 = (temp_r0_51 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_52) = *(&lbl_815425A0 + temp_r0_52) + sp1C;
        goto loop_38;
    }
    temp_r29_13 = fn_813319A0();
    fn_8132C830(&spC, arg0, temp_r29_13, fn_81331A08());
    var_r28_13 = 0;
    var_r29_14 = temp_r29_13;
loop_41:
    if (var_r28_13 < (s32) fn_81331A08()) {
        temp_r0_53 = *var_r29_14 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_53, &lbl_815425A0 + temp_r0_53, spC, sp10, sp14);
        var_r28_13 += 1;
        temp_r0_54 = (*var_r29_14 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_54) = *(&lbl_815425A0 + temp_r0_54) + sp20;
        temp_r0_55 = *var_r29_14;
        var_r29_14 += 2;
        temp_r0_56 = (temp_r0_55 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_56) = *(&lbl_815425A0 + temp_r0_56) + sp1C;
        goto loop_41;
    }
    temp_r29_14 = fn_81331AE8();
    fn_8132C830(&spC, arg0, temp_r29_14, fn_81331B50());
    var_r28_14 = 0;
    var_r29_15 = temp_r29_14;
loop_44:
    if (var_r28_14 < (s32) fn_81331B50()) {
        temp_r0_57 = *var_r29_15 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_57, &lbl_815425A0 + temp_r0_57, spC, sp10, sp14);
        var_r28_14 += 1;
        temp_r0_58 = (*var_r29_15 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_58) = *(&lbl_815425A0 + temp_r0_58) + sp20;
        temp_r0_59 = *var_r29_15;
        var_r29_15 += 2;
        temp_r0_60 = (temp_r0_59 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_60) = *(&lbl_815425A0 + temp_r0_60) + sp1C;
        goto loop_44;
    }
    temp_r29_15 = fn_81331B8C();
    fn_8132C830(&spC, arg0, temp_r29_15, fn_81331BF4());
    var_r28_15 = 0;
    var_r29_16 = temp_r29_15;
loop_47:
    if (var_r28_15 < (s32) fn_81331BF4()) {
        temp_r0_61 = *var_r29_16 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_61, &lbl_815425A0 + temp_r0_61, spC, sp10, sp14);
        var_r28_15 += 1;
        temp_r0_62 = (*var_r29_16 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_62) = *(&lbl_815425A0 + temp_r0_62) + sp20;
        temp_r0_63 = *var_r29_16;
        var_r29_16 += 2;
        temp_r0_64 = (temp_r0_63 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_64) = *(&lbl_815425A0 + temp_r0_64) + sp1C;
        goto loop_47;
    }
    temp_r29_16 = fn_81331EC0();
    fn_8132C830(&spC, arg0, temp_r29_16, fn_81331F28());
    var_r28_16 = 0;
    var_r29_17 = temp_r29_16;
loop_50:
    if (var_r28_16 < (s32) fn_81331F28()) {
        temp_r0_65 = *var_r29_17 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_65, &lbl_815425A0 + temp_r0_65, spC, sp10, sp14);
        var_r28_16 += 1;
        temp_r0_66 = (*var_r29_17 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_66) = *(&lbl_815425A0 + temp_r0_66) + sp20;
        temp_r0_67 = *var_r29_17;
        var_r29_17 += 2;
        temp_r0_68 = (temp_r0_67 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_68) = *(&lbl_815425A0 + temp_r0_68) + sp1C;
        goto loop_50;
    }
    temp_r29_17 = fn_81331F64();
    fn_8132C830(&spC, arg0, temp_r29_17, fn_81331FCC());
    var_r28_17 = 0;
    var_r29_18 = temp_r29_17;
loop_53:
    if (var_r28_17 < (s32) fn_81331FCC()) {
        temp_r0_69 = *var_r29_18 << 5;
        fn_8132C750(0, sp1A, sp18, arg0 + temp_r0_69, &lbl_815425A0 + temp_r0_69, spC, sp10, sp14);
        var_r28_17 += 1;
        temp_r0_70 = (*var_r29_18 << 5) + 0x14;
        *(&lbl_815425A0 + temp_r0_70) = *(&lbl_815425A0 + temp_r0_70) + sp20;
        temp_r0_71 = *var_r29_18;
        var_r29_18 += 2;
        temp_r0_72 = (temp_r0_71 << 5) + 0x18;
        *(&lbl_815425A0 + temp_r0_72) = *(&lbl_815425A0 + temp_r0_72) + sp1C;
        goto loop_53;
    }
    fn_81334928();
}

/* 0x8133491C */
void *fn_8133491C(void) {
    return &lbl_815425A0;
}

/* 0x81334928 */
void fn_81334928(void) {
    void *temp_r3;
    u32 temp_r4;

    temp_r3 = lbl_814AD614;
    temp_r4 = lbl_814AD610 + 1;
    lbl_814AD610 = temp_r4;
    lbl_814AD614 = temp_r3 + 1;
    if ((u32) lbl_814AD610 >= (u32) Menu_Get(temp_r3, temp_r4)->unkC8) {
        lbl_814AD610 = 0;
    }
    if ((void *) lbl_814AD614 >= 0x64U) {
        lbl_814AD614 = NULL;
    }
}

/* 0x8133498C */
void fn_8133498C(f32 *arg0, f32 *arg1, u16 *arg2, u16 *arg3, f32 farg0) {
    f32 *temp_r31;
    f32 *temp_r31_2;
    f32 *temp_r31_3;
    f32 *temp_r31_4;
    f32 temp_f29;
    f32 temp_f30;
    f32 temp_f31;
    f32 var_f2;
    f32 var_f30;

    temp_f31 = fn_81334BA8(farg0 + (Menu_Get()->unkC4 * 0.5f), 1.0f);
    if (fabs(1.0f) > fabs(farg0)) {
        var_f2 = farg0;
    } else {
        fn_8137B2C8(farg0 / 1.0f);
        var_f2 = farg0 - (1.0f * fn_8137B214());
    }
    temp_f29 = fn_81334BA8(0.25f + (2.0f * var_f2), 1.0f);
    var_f30 = 0.25f + farg0;
    if (fabs(1.0f) > fabs(var_f30)) {

    } else {
        fn_8137B2C8(var_f30 / 1.0f);
        var_f30 -= 1.0f * fn_8137B214();
    }
    temp_r31 = Menu_Get();
    temp_f30 = -Menu_Get()->unkB4;
    *arg0 = fn_81334B40(Menu_Get()->unkC4, temp_f31, temp_f30, temp_r31->unkB4);
    temp_r31_2 = Menu_Get();
    *arg1 = fn_81334B40(0.5f, temp_f29, -Menu_Get()->unkB8, temp_r31_2->unkB8);
    temp_r31_3 = Menu_Get();
    fn_81334B40(0.5f, var_f30, -Menu_Get()->unkBC, temp_r31_3->unkBC);
    *arg2 = fn_8132CB60();
    temp_r31_4 = Menu_Get();
    fn_81334B40(0.5f, var_f30, -Menu_Get()->unkC0, temp_r31_4->unkC0);
    *arg3 = fn_8132CB60();
}

/* 0x81334B40 */
void fn_81334B40(f32 farg0, f32 farg1, f32 farg2, f32 farg3) {
    if (farg1 < farg0) {
        Hermite(0.0f, farg2, 0.0f, 0.0f);
        return;
    }
    Hermite(farg0, farg3, 0.0f, 1.0f, farg2, 0.0f);
}

/* 0x81334BA8 */
f32 fn_81334BA8(f32 farg0, f32 farg1) {
    if (fabs(farg1) > fabs(farg0)) {
        return farg0;
    }
    fn_8137B2C8(farg0 / farg1);
    return farg0 - (farg1 * fn_8137B214());
}

/* 0x81334C08 */
s16 fn_81334C08(void) {
    return lbl_814AD624;
}

/* 0x81334C10 */
void *fn_81334C10(void) {
    return &lbl_81553FC0;
}

/* 0x81334C1C */
s16 *fn_81334C1C(void) {
    return &lbl_815531C0;
}

/* 0x81334C28 */
void fn_81334C28(void) {
    lbl_814AD620.unk0 = Menu_Get()->unkD4;
    lbl_814AD620.unk2 = (s16) Menu_Get()->unkD4;
}

/* 0x81334C60 */
void fn_81334C60(s32 arg0) {
    s32 sp2C;
    s16 *sp28;
    f32 sp24;
    s32 sp20;
    s16 *sp1C;
    f32 sp18;
    f32 temp_f1;
    f32 temp_f1_10;
    f32 temp_f1_11;
    f32 temp_f1_12;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f1_7;
    f32 temp_f1_8;
    f32 temp_f1_9;
    f32 temp_f28;
    f32 temp_f28_2;
    f32 temp_f28_3;
    f32 temp_f2;
    f32 temp_f2_2;
    f32 temp_f2_3;
    f32 temp_f2_4;
    f32 temp_f2_5;
    f32 temp_f2_6;
    f32 temp_f2_7;
    f32 temp_f2_8;
    f32 temp_f2_9;
    f32 temp_f31;
    f32 temp_f31_2;
    f32 temp_f31_3;
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
    s16 *temp_r19_2;
    s16 *var_r24;
    s16 *var_r26;
    s16 *var_r3;
    s16 temp_r3;
    s16 temp_r3_3;
    s16 temp_r3_4;
    s16 temp_r3_5;
    s16 temp_r3_6;
    s16 temp_r3_7;
    s16 temp_r3_8;
    s16 temp_r3_9;
    s16 temp_r4;
    s16 temp_r4_2;
    s16 temp_r4_4;
    s16 temp_r4_5;
    s16 temp_r4_6;
    s32 temp_r4_3;
    s32 var_ctr;
    s32 var_r19;
    s32 var_r19_2;
    s32 var_r20;
    s32 var_r20_2;
    s32 var_r20_3;
    s32 var_r21;
    s32 var_r21_2;
    s32 var_r21_3;
    s32 var_r21_4;
    s32 var_r27;
    u16 *temp_r22;
    u16 *temp_r29;
    u16 *var_r23;
    u16 *var_r23_2;
    u16 *var_r23_3;
    u16 *var_r23_4;
    u16 *var_r24_2;
    u16 *var_r24_3;
    u16 *var_r24_4;
    u16 *var_r25;
    u16 temp_r0;
    u16 temp_r0_2;
    u16 temp_r0_3;
    u16 temp_r19;
    void *temp_r22_2;
    void *temp_r22_3;
    void *temp_r22_4;
    void *temp_r26;
    void *temp_r26_2;
    void *temp_r26_3;
    void *temp_r3_2;

    if ((s16) fn_8132CFE8(&lbl_815531C0)->unk6 == 2) {
        if ((s16) fn_8132CFF4()->unk6 != 2) {
            fn_813368F0();
            lbl_814AD624 = 0;
            return;
        }
        lbl_814AD624 = 0x70;
        temp_r22 = fn_8133285C();
        temp_r19 = fn_813328C4();
        var_ctr = 2;
        temp_r29 = fn_81332900();
        var_r3 = &lbl_815531C0 + 0xE00;
        do {
            var_r3->unk0 = 0;
            var_r3->unk1 = 0;
            var_r3->unk2 = 0;
            var_r3->unk3 = 0;
            var_r3->unk4 = 0;
            var_r3->unk5 = 0;
            var_r3->unk6 = 0;
            var_r3->unk7 = 0;
            var_r3->unk8 = 0;
            var_r3->unk9 = 0;
            var_r3->unkA = 0;
            var_r3->unkB = 0;
            var_r3->unkC = 0;
            var_r3->unkD = 0;
            var_r3->unkE = 0;
            var_r3->unkF = 0;
            var_r3->unk10 = 0;
            var_r3->unk11 = 0;
            var_r3->unk12 = 0;
            var_r3->unk13 = 0;
            var_r3->unk14 = 0;
            var_r3->unk15 = 0;
            var_r3->unk16 = 0;
            var_r3->unk17 = 0;
            var_r3->unk18 = 0;
            var_r3->unk19 = 0;
            var_r3->unk1A = 0;
            var_r3->unk1B = 0;
            var_r3->unk1C = 0;
            var_r3->unk1D = 0;
            var_r3->unk1E = 0;
            var_r3->unk1F = 0;
            var_r3->unk20 = 0;
            var_r3->unk21 = 0;
            var_r3->unk22 = 0;
            var_r3->unk23 = 0;
            var_r3->unk24 = 0;
            var_r3->unk25 = 0;
            var_r3->unk26 = 0;
            var_r3->unk27 = 0;
            var_r3->unk28 = 0;
            var_r3->unk29 = 0;
            var_r3->unk2A = 0;
            var_r3->unk2B = 0;
            var_r3->unk2C = 0;
            var_r3->unk2D = 0;
            var_r3->unk2E = 0;
            var_r3->unk2F = 0;
            var_r3->unk30 = 0;
            var_r3->unk31 = 0;
            var_r3->unk32 = 0;
            var_r3->unk33 = 0;
            var_r3->unk34 = 0;
            var_r3->unk35 = 0;
            var_r3->unk36 = 0;
            var_r3->unk37 = 0;
            var_r3 += 0x38;
            var_ctr -= 1;
        } while (var_ctr != 0);
        var_r25 = temp_r22;
        var_r24 = &lbl_815531C0;
        var_r23 = fn_81332D7C(lbl_814AD61C.unk2);
        var_r26 = &lbl_815531C0 + 0xE70;
        var_r21 = 0;
loop_9:
        if (var_r21 < (s32) temp_r19) {
            temp_f31 = fn_81336594();
            temp_r4 = *var_r25 << 5;
            temp_f3 = Menu_Get()->unkD0;
            temp_f2 = (f32) *var_r26 * temp_f3;
            temp_f1 = -lbl_815531C0.unkF50 * temp_f2 * temp_f31;
            var_r24->unk14 = (f32) (*(arg0 + (temp_r4 + 0x14)) + temp_f1);
            temp_f31_2 = fn_81336594(temp_r4, temp_f1, temp_f2, temp_f3);
            temp_r4_2 = *var_r25 << 5;
            temp_f3_2 = Menu_Get()->unkD0;
            temp_f2_2 = (f32) *var_r26 * temp_f3_2;
            temp_f1_2 = -lbl_815531C0.unkF54 * temp_f2_2 * temp_f31_2;
            var_r24->unk18 = (f32) (*(arg0 + (temp_r4_2 + 0x18)) + temp_f1_2);
            temp_f31_3 = fn_81336594(temp_r4_2, temp_f1_2, temp_f2_2, temp_f3_2);
            temp_r4_3 = *var_r25 << 5;
            temp_f3_3 = Menu_Get()->unkD0;
            temp_f2_3 = (f32) *var_r26 * temp_f3_3;
            temp_f1_3 = -lbl_815531C0.unkF58 * temp_f2_3 * temp_f31_3;
            var_r24->unk1C = (f32) (*(arg0 + (temp_r4_3 + 0x1C)) + temp_f1_3);
            var_r24[6] = *(arg0 + ((*var_r25 << 5) + 0xC));
            var_r24[7] = *(arg0 + ((*var_r25 << 5) + 0xE));
            var_r24[8] = *(arg0 + ((*var_r25 << 5) + 0x10));
            var_r24->unk0 = (f32) *(arg0 + (*var_r25 << 5));
            var_r24->unk4 = (f32) *(arg0 + ((*var_r25 << 5) + 4));
            temp_r3 = *var_r25 << 5;
            var_r24->unk8 = (f32) *(arg0 + (temp_r3 + 8));
            var_r26 += 2;
            temp_r3_2 = &lbl_815531C0 + *var_r23;
            var_r25 += 2;
            var_r24 += 0x20;
            var_r23 += 2;
            var_r21 += 1;
            temp_r3_2->unkE00 = (s8) (255.0f * (1.0f - fn_81336594(temp_r3, (bitwise f32) temp_r4_3, temp_f1_3, temp_f2_3, temp_f3_3)));
            goto loop_9;
        }
        var_r23_2 = temp_r22;
        var_r24_2 = fn_81332D7C(lbl_814AD618.unk2);
        var_r20 = temp_r19 * 2;
        var_r19 = temp_r19 << 5;
        var_r21_2 = 0;
loop_12:
        if (var_r21_2 < (s32) temp_r19) {
            temp_f28 = 1.0f - fn_81336594();
            temp_r26 = &lbl_815531C0 + var_r20;
            temp_r4_4 = temp_r26->unkE70;
            temp_r22_2 = &lbl_815531C0 + var_r19;
            temp_f3_4 = Menu_Get()->unkD0;
            temp_f2_4 = (f32) temp_r4_4 * temp_f3_4;
            temp_f1_4 = lbl_815531C0.unkF50 * temp_f2_4 * temp_f28;
            temp_r22_2->unk14 = (f32) (*(arg0 + ((*var_r23_2 << 5) + 0x14)) + temp_f1_4);
            temp_f28_2 = 1.0f - fn_81336594(temp_r4_4, temp_f1_4, temp_f2_4, temp_f3_4);
            temp_f3_5 = Menu_Get()->unkD0;
            temp_r3_3 = *var_r23_2 << 5;
            temp_f2_5 = (f32) temp_r26->unkE70 * temp_f3_5;
            temp_f1_5 = lbl_815531C0.unkF54 * temp_f2_5 * temp_f28_2;
            temp_r22_2->unk18 = (f32) (*(arg0 + (temp_r3_3 + 0x18)) + temp_f1_5);
            temp_f28_3 = 1.0f - fn_81336594(temp_r3_3, temp_f1_5, temp_f2_5, temp_f3_5);
            temp_f3_6 = Menu_Get()->unkD0;
            temp_f2_6 = (f32) temp_r26->unkE70 * temp_f3_6;
            temp_f1_6 = lbl_815531C0.unkF58 * temp_f2_6 * temp_f28_3;
            temp_r22_2->unk1C = (f32) (*(arg0 + ((*var_r23_2 << 5) + 0x1C)) + temp_f1_6);
            temp_r22_2->unkC = (s16) *(arg0 + ((*var_r23_2 << 5) + 0xC));
            temp_r22_2->unkE = (s16) *(arg0 + ((*var_r23_2 << 5) + 0xE));
            temp_r22_2->unk10 = (s16) *(arg0 + ((*var_r23_2 << 5) + 0x10));
            temp_r22_2->unk0 = (f32) *(arg0 + (*var_r23_2 << 5));
            temp_r22_2->unk4 = (f32) *(arg0 + ((*var_r23_2 << 5) + 4));
            temp_r3_4 = *var_r23_2 << 5;
            temp_r22_2->unk8 = (f32) *(arg0 + (temp_r3_4 + 8));
            temp_r0 = *var_r24_2;
            var_r19 += 0x20;
            var_r20 += 2;
            var_r23_2 += 2;
            var_r24_2 += 2;
            var_r21_2 += 1;
            *(&lbl_815531C0 + temp_r19 + 0xE00 + temp_r0) = (s8) (255.0f * fn_81336594(temp_r3_4, temp_f1_6, temp_f2_6, temp_f3_6));
            goto loop_12;
        }
        var_r23_3 = temp_r29;
        var_r24_3 = fn_81332D7C(lbl_814AD61C.unk0);
        var_r19_2 = temp_r19 * 4;
        var_r20_2 = temp_r19 << 6;
        var_r21_3 = 0;
loop_15:
        if (var_r21_3 < (s32) temp_r19) {
            temp_r22_3 = &lbl_815531C0 + var_r19_2;
            temp_r4_5 = temp_r22_3->unkE70;
            temp_r26_2 = &lbl_815531C0 + var_r20_2;
            temp_r3_5 = *var_r23_3 << 5;
            temp_f3_7 = -lbl_815531C0.unkF50;
            temp_f2_7 = temp_f3_7 * (f32) temp_r4_5;
            temp_f1_7 = temp_f2_7 * fn_813365F4();
            temp_r26_2->unk14 = (f32) (*(arg0 + (temp_r3_5 + 0x14)) + temp_f1_7);
            temp_f3_8 = -lbl_815531C0.unkF54;
            temp_r3_6 = *var_r23_3 << 5;
            temp_f2_8 = temp_f3_8 * (f32) temp_r22_3->unkE70;
            temp_f1_8 = temp_f2_8 * fn_813365F4(temp_r3_5, temp_r4_5, temp_f1_7, temp_f2_7, temp_f3_7);
            temp_r26_2->unk18 = (f32) (*(arg0 + (temp_r3_6 + 0x18)) + temp_f1_8);
            temp_f3_9 = -lbl_815531C0.unkF58;
            temp_f2_9 = temp_f3_9 * (f32) temp_r22_3->unkE70;
            temp_f1_9 = temp_f2_9 * fn_813365F4(temp_r3_6, (bitwise s16) temp_f1_8, temp_f2_8, temp_f3_8);
            temp_r26_2->unk1C = (f32) (*(arg0 + ((*var_r23_3 << 5) + 0x1C)) + temp_f1_9);
            temp_r26_2->unkC = (s16) *(arg0 + ((*var_r23_3 << 5) + 0xC));
            temp_r26_2->unkE = (s16) *(arg0 + ((*var_r23_3 << 5) + 0xE));
            temp_r26_2->unk10 = (s16) *(arg0 + ((*var_r23_3 << 5) + 0x10));
            temp_r26_2->unk0 = (f32) *(arg0 + (*var_r23_3 << 5));
            temp_r26_2->unk4 = (f32) *(arg0 + ((*var_r23_3 << 5) + 4));
            temp_r3_7 = *var_r23_3 << 5;
            temp_r26_2->unk8 = (f32) *(arg0 + (temp_r3_7 + 8));
            temp_r0_2 = *var_r24_3;
            var_r20_2 += 0x20;
            var_r19_2 += 2;
            var_r23_3 += 2;
            var_r24_3 += 2;
            var_r21_3 += 1;
            *(&(&lbl_815531C0)[temp_r19] + 0xE00 + temp_r0_2) = (s8) (255.0f * (1.0f - fn_813365F4(temp_r3_7, (bitwise s16) temp_f1_9, temp_f2_9, temp_f3_9)));
            goto loop_15;
        }
        var_r23_4 = temp_r29;
        var_r24_4 = fn_81332D7C(lbl_814AD618.unk0);
        var_r20_3 = temp_r19 * 6;
        var_r21_4 = temp_r19 * 0x60;
        var_r27 = 0;
loop_18:
        if (var_r27 < (s32) temp_r19) {
            temp_r22_4 = &lbl_815531C0 + var_r20_3;
            temp_r4_6 = temp_r22_4->unkE70;
            temp_f3_10 = 1.0f - fn_813365F4();
            temp_r26_3 = &lbl_815531C0 + var_r21_4;
            temp_f1_10 = lbl_815531C0.unkF50 * (f32) temp_r4_6 * temp_f3_10;
            temp_r26_3->unk14 = (f32) (*(arg0 + ((*var_r23_4 << 5) + 0x14)) + temp_f1_10);
            temp_f3_11 = 1.0f - fn_813365F4(temp_r4_6, (bitwise s16) temp_f1_10, lbl_815531C0.unkF50, temp_f3_10);
            temp_r3_8 = *var_r23_4 << 5;
            temp_f1_11 = lbl_815531C0.unkF54 * (f32) temp_r22_4->unkE70 * temp_f3_11;
            temp_r26_3->unk18 = (f32) (*(arg0 + (temp_r3_8 + 0x18)) + temp_f1_11);
            temp_f3_12 = 1.0f - fn_813365F4(temp_r3_8, (bitwise s16) temp_f1_11, lbl_815531C0.unkF54, temp_f3_11);
            temp_f1_12 = lbl_815531C0.unkF58 * (f32) temp_r22_4->unkE70 * temp_f3_12;
            temp_r26_3->unk1C = (f32) (*(arg0 + ((*var_r23_4 << 5) + 0x1C)) + temp_f1_12);
            temp_r26_3->unkC = (s16) *(arg0 + ((*var_r23_4 << 5) + 0xC));
            temp_r26_3->unkE = (s16) *(arg0 + ((*var_r23_4 << 5) + 0xE));
            temp_r26_3->unk10 = (s16) *(arg0 + ((*var_r23_4 << 5) + 0x10));
            temp_r26_3->unk0 = (f32) *(arg0 + (*var_r23_4 << 5));
            temp_r26_3->unk4 = (f32) *(arg0 + ((*var_r23_4 << 5) + 4));
            temp_r3_9 = *var_r23_4 << 5;
            temp_r26_3->unk8 = (f32) *(arg0 + (temp_r3_9 + 8));
            temp_r0_3 = *var_r24_4;
            var_r21_4 += 0x20;
            var_r20_3 += 2;
            var_r23_4 += 2;
            var_r24_4 += 2;
            var_r27 += 1;
            *(&lbl_815531C0 + (temp_r19 * 3) + 0xE00 + temp_r0_3) = (s8) (255.0f * fn_813365F4(temp_r3_9, (bitwise s16) temp_f1_12, lbl_815531C0.unkF58, temp_f3_12));
            goto loop_18;
        }
        sp18 = lbl_8137EF90->unk0;
        sp1C = lbl_8137EF90->unk4;
        sp20 = lbl_8137EF90->unk8;
        sp24 = sp18;
        sp28 = sp1C;
        sp2C = lbl_8137EF90->unk8;
        temp_r19_2 = fn_8132CFE8(sp1C, lbl_8137EF90);
        fn_81336650(&sp24, fn_8132CFF4()->unkA, temp_r19_2->unkA);
        if ((s16) lbl_814AD620.unk0 < (s16) Menu_Get()->unkD4) {
            lbl_814AD620.unk0 += 1;
        }
        if ((s16) lbl_814AD620.unk2 < (s16) Menu_Get()->unkD4) {
            lbl_814AD620.unk2 = (s16) (lbl_814AD620.unk2 + 1);
        }
        return;
    }
    lbl_814AD624 = 0;
}

/* 0x813355E0 */
void fn_813355E0(s32 arg0) {
    s16 sp178;
    s16 sp16C;
    s16 sp164;
    s32 sp160;
    s32 sp15C;
    f32 sp158;
    s32 sp154;
    s32 sp150;
    f32 sp14C;
    s32 sp148;
    s32 sp144;
    f32 sp140;
    s32 sp13C;
    s32 sp138;
    f32 sp134;
    s32 sp130;
    s32 sp12C;
    f32 sp128;
    s32 sp124;
    s32 sp120;
    f32 sp11C;
    s32 sp118;
    s32 sp114;
    f32 sp110;
    s32 sp10C;
    s32 sp108;
    f32 sp104;
    s32 sp100;
    s32 spFC;
    f32 spF8;
    s32 spF4;
    s32 spF0;
    f32 spEC;
    s32 spE8;
    s32 spE4;
    f32 spE0;
    s32 spDC;
    s32 spD8;
    f32 spD4;
    s32 spD0;
    s32 spCC;
    f32 spC8;
    s32 spC4;
    s32 spC0;
    f32 spBC;
    s32 spB8;
    s32 spB4;
    f32 spB0;
    s32 spAC;
    s32 spA8;
    f32 spA4;
    s32 spA0;
    s32 sp9C;
    f32 sp98;
    s32 sp94;
    s32 sp90;
    f32 sp8C;
    s32 sp88;
    s32 sp84;
    f32 sp80;
    s32 sp7C;
    s32 sp78;
    f32 sp74;
    s32 sp70;
    s32 sp6C;
    f32 sp68;
    s32 sp64;
    s32 sp60;
    f32 sp5C;
    s32 sp58;
    s32 sp54;
    f32 sp50;
    s32 sp4C;
    s32 sp48;
    f32 sp44;
    f32 temp_f1;
    f32 temp_f1_10;
    f32 temp_f1_11;
    f32 temp_f1_12;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f1_7;
    f32 temp_f1_8;
    f32 temp_f1_9;
    f32 temp_f28;
    f32 temp_f28_2;
    f32 temp_f28_3;
    f32 temp_f2;
    f32 temp_f2_2;
    f32 temp_f2_3;
    f32 temp_f2_4;
    f32 temp_f2_5;
    f32 temp_f2_6;
    f32 temp_f2_7;
    f32 temp_f2_8;
    f32 temp_f2_9;
    f32 temp_f31;
    f32 temp_f31_2;
    f32 temp_f31_3;
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
    s16 *temp_r19;
    s16 *temp_r19_2;
    s16 *var_r23;
    s16 *var_r25;
    s16 *var_r3;
    s16 temp_r0;
    s16 temp_r0_2;
    s16 temp_r0_3;
    s16 temp_r0_4;
    s16 temp_r19_3;
    s16 temp_r19_4;
    s16 temp_r3;
    s16 temp_r3_3;
    s16 temp_r3_4;
    s16 temp_r3_5;
    s16 temp_r3_6;
    s16 temp_r3_7;
    s16 temp_r3_8;
    s16 temp_r3_9;
    s16 temp_r4;
    s16 temp_r4_2;
    s16 temp_r4_4;
    s16 temp_r4_5;
    s16 temp_r4_6;
    s32 temp_r4_3;
    s32 var_ctr;
    s32 var_r19;
    s32 var_r19_2;
    s32 var_r20;
    s32 var_r20_2;
    s32 var_r20_3;
    s32 var_r21;
    s32 var_r21_2;
    s32 var_r21_3;
    s32 var_r21_4;
    s32 var_r29;
    u16 *var_r22;
    u16 *var_r22_2;
    u16 *var_r22_3;
    u16 *var_r22_4;
    u16 *var_r23_2;
    u16 *var_r23_3;
    u16 *var_r23_4;
    u16 *var_r24;
    u16 *var_r26;
    u16 *var_r28;
    u16 temp_r0_5;
    u16 temp_r0_6;
    u16 temp_r0_7;
    u16 temp_r27;
    void *temp_r25;
    void *temp_r25_2;
    void *temp_r25_3;
    void *temp_r28;
    void *temp_r29;
    void *temp_r29_2;
    void *temp_r3_2;

    var_r26 = saved_reg_r26;
    var_r28 = saved_reg_r28;
    if ((s16) fn_8132CFE8(lbl_8137EF90)->unkE != 0) {
        if ((s16) fn_8132CFF4()->unkE != 0) {
            lbl_814AD624 = 0x70;
            temp_r27 = fn_81332114();
            temp_r19 = fn_8132CFE8();
            fn_8132A690(&sp178, fn_8132CFE8()->unkE, temp_r19 + 0x16);
            temp_r19_2 = fn_8132CFF4();
            fn_8132A690(&sp16C, fn_8132CFF4()->unkE, temp_r19_2 + 0x16);
            if ((s16) fn_8132CFE8()->unkE == 1) {
                temp_r0 = fn_8132CFE8()->unk12;
                switch (temp_r0) {                  /* switch 1; irregular */
                case 0:                             /* switch 1 */
                    if (((sp178 == 0x833) && (sp16C == 0x7D0)) || ((sp178 == 0x7D0) && (sp16C == 0x833))) {
                        sp14C = lbl_8137EF90->unkC;
                        sp150 = lbl_8137EF90->unk10;
                        sp154 = lbl_8137EF90->unk14;
                        sp158 = sp14C;
                        sp15C = sp150;
                        sp160 = lbl_8137EF90->unk14;
                        fn_81336650(&sp158, sp16C, (s16) sp150);
                    } else {
                        sp134 = lbl_8137EF90->unk18;
                        sp138 = lbl_8137EF90->unk1C;
                        sp13C = lbl_8137EF90->unk20;
                        sp140 = sp134;
                        sp144 = sp138;
                        sp148 = lbl_8137EF90->unk20;
                        fn_81336650(&sp140, sp16C, (s16) sp138);
                    }
                    var_r28 = fn_813321F4();
                    var_r26 = fn_81332298();
                    break;
                case 1:                             /* switch 1 */
                    if (((sp17A == 0xC) && (sp16E == 1)) || ((sp17A == 1) && (sp16E == 0xC))) {
                        sp11C = lbl_8137EF90->unk24;
                        sp120 = lbl_8137EF90->unk28;
                        sp124 = lbl_8137EF90->unk2C;
                        sp128 = sp11C;
                        sp12C = sp120;
                        sp130 = lbl_8137EF90->unk2C;
                        fn_81336650(&sp128, sp16E, (s16) sp120);
                    } else {
                        sp104 = lbl_8137EF90->unk30;
                        sp108 = lbl_8137EF90->unk34;
                        sp10C = lbl_8137EF90->unk38;
                        sp110 = sp104;
                        sp114 = sp108;
                        sp118 = lbl_8137EF90->unk38;
                        fn_81336650(&sp110, sp16E, (s16) sp108);
                    }
                    var_r28 = fn_81331CD4();
                    var_r26 = fn_81331D78();
                    break;
                case 2:                             /* switch 1 */
                    if ((((sp17C == 0x1F) || ((u16) (sp17C - 0x1C) <= 2U)) && (sp170 == 1)) || ((sp17C == 1) && ((sp170 == 0x1F) || ((u16) (sp170 - 0x1C) <= 2U)))) {
                        spEC = lbl_8137EF90->unk3C;
                        spF0 = lbl_8137EF90->unk40;
                        spF4 = lbl_8137EF90->unk44;
                        spF8 = spEC;
                        spFC = spF0;
                        sp100 = lbl_8137EF90->unk44;
                        fn_81336650(&spF8, sp170, (s16) spF0);
                    } else {
                        spD4 = lbl_8137EF90->unk48;
                        spD8 = lbl_8137EF90->unk4C;
                        spDC = lbl_8137EF90->unk50;
                        spE0 = spD4;
                        spE4 = spD8;
                        spE8 = lbl_8137EF90->unk50;
                        fn_81336650(&spE0, sp170, (s16) spD8);
                    }
                    var_r28 = fn_81331710();
                    var_r26 = fn_813317B4();
                    break;
                }
                goto block_64;
            }
            if ((s16) fn_8132CFE8()->unkE == 2) {
                temp_r0_2 = fn_8132CFE8()->unk14;
                switch (temp_r0_2) {                /* switch 2; irregular */
                case 0:                             /* switch 2 */
                    if (((sp17E == 0x17) && (sp172 == 0)) || ((sp17E == 0) && (sp172 == 0x17))) {
                        spBC = lbl_8137EF90->unk54;
                        spC0 = lbl_8137EF90->unk58;
                        spC4 = lbl_8137EF90->unk5C;
                        spC8 = spBC;
                        spCC = spC0;
                        spD0 = lbl_8137EF90->unk5C;
                        fn_81336650(&spC8, sp172, (s16) spC0);
                    } else {
                        spA4 = lbl_8137EF90->unk60;
                        spA8 = lbl_8137EF90->unk64;
                        spAC = lbl_8137EF90->unk68;
                        spB0 = spA4;
                        spB4 = spA8;
                        spB8 = lbl_8137EF90->unk68;
                        fn_81336650(&spB0, sp172, (s16) spA8);
                    }
                    var_r28 = fn_813318FC();
                    var_r26 = fn_813319A0();
                    break;
                case 1:                             /* switch 2 */
                    if (((sp180 == 0x3B) && (sp174 == 0)) || ((sp180 == 0) && (sp174 == 0x3B))) {
                        sp8C = lbl_8137EF90->unk6C;
                        sp90 = lbl_8137EF90->unk70;
                        sp94 = lbl_8137EF90->unk74;
                        sp98 = sp8C;
                        sp9C = sp90;
                        spA0 = lbl_8137EF90->unk74;
                        fn_81336650(&sp98, sp174, (s16) sp90);
                    } else {
                        sp74 = lbl_8137EF90->unk78;
                        sp78 = lbl_8137EF90->unk7C;
                        sp7C = lbl_8137EF90->unk80;
                        sp80 = sp74;
                        sp84 = sp78;
                        sp88 = lbl_8137EF90->unk80;
                        fn_81336650(&sp80, sp174, (s16) sp78);
                    }
                    var_r28 = fn_81331AE8();
                    var_r26 = fn_81331B8C();
                    break;
                case 2:                             /* switch 2 */
                    if (((sp182 == 0x3B) && (sp176 == 0)) || ((sp182 == 0) && (sp176 == 0x3B))) {
                        sp5C = lbl_8137EF90->unk84;
                        sp60 = lbl_8137EF90->unk88;
                        sp64 = lbl_8137EF90->unk8C;
                        sp68 = sp5C;
                        sp6C = sp60;
                        sp70 = lbl_8137EF90->unk8C;
                        fn_81336650(&sp68, sp176, (s16) sp60);
                    } else {
                        sp44 = lbl_8137EF90->unk90;
                        sp48 = lbl_8137EF90->unk94;
                        sp4C = lbl_8137EF90->unk98;
                        sp50 = sp44;
                        sp54 = sp48;
                        sp58 = lbl_8137EF90->unk98;
                        fn_81336650(&sp50, sp176, (s16) sp48);
                    }
                    var_r28 = fn_81331EC0();
                    var_r26 = fn_81331F64();
                    break;
                }
block_64:
                var_ctr = 2;
                var_r3 = &lbl_815531C0 + 0xE00;
                do {
                    var_r3->unk0 = 0;
                    var_r3->unk1 = 0;
                    var_r3->unk2 = 0;
                    var_r3->unk3 = 0;
                    var_r3->unk4 = 0;
                    var_r3->unk5 = 0;
                    var_r3->unk6 = 0;
                    var_r3->unk7 = 0;
                    var_r3->unk8 = 0;
                    var_r3->unk9 = 0;
                    var_r3->unkA = 0;
                    var_r3->unkB = 0;
                    var_r3->unkC = 0;
                    var_r3->unkD = 0;
                    var_r3->unkE = 0;
                    var_r3->unkF = 0;
                    var_r3->unk10 = 0;
                    var_r3->unk11 = 0;
                    var_r3->unk12 = 0;
                    var_r3->unk13 = 0;
                    var_r3->unk14 = 0;
                    var_r3->unk15 = 0;
                    var_r3->unk16 = 0;
                    var_r3->unk17 = 0;
                    var_r3->unk18 = 0;
                    var_r3->unk19 = 0;
                    var_r3->unk1A = 0;
                    var_r3->unk1B = 0;
                    var_r3->unk1C = 0;
                    var_r3->unk1D = 0;
                    var_r3->unk1E = 0;
                    var_r3->unk1F = 0;
                    var_r3->unk20 = 0;
                    var_r3->unk21 = 0;
                    var_r3->unk22 = 0;
                    var_r3->unk23 = 0;
                    var_r3->unk24 = 0;
                    var_r3->unk25 = 0;
                    var_r3->unk26 = 0;
                    var_r3->unk27 = 0;
                    var_r3->unk28 = 0;
                    var_r3->unk29 = 0;
                    var_r3->unk2A = 0;
                    var_r3->unk2B = 0;
                    var_r3->unk2C = 0;
                    var_r3->unk2D = 0;
                    var_r3->unk2E = 0;
                    var_r3->unk2F = 0;
                    var_r3->unk30 = 0;
                    var_r3->unk31 = 0;
                    var_r3->unk32 = 0;
                    var_r3->unk33 = 0;
                    var_r3->unk34 = 0;
                    var_r3->unk35 = 0;
                    var_r3->unk36 = 0;
                    var_r3->unk37 = 0;
                    var_r3 += 0x38;
                    var_ctr -= 1;
                } while (var_ctr != 0);
                temp_r19_3 = fn_8132CFF4()->unk12;
                if ((s16) fn_8132CFE8()->unk12 != temp_r19_3) {
                    lbl_814AD620.unk0 = Menu_Get()->unkD4;
                    (&lbl_814AD620)[1] = Menu_Get()->unkD4;
                    temp_r0_3 = fn_8132CFE8(&lbl_814AD620)->unk12;
                    switch (temp_r0_3) {            /* switch 3; irregular */
                    case 0:                         /* switch 3 */
                        fn_8132C660(&sp164, sp178);
                        break;
                    case 1:                         /* switch 3 */
                        fn_8132C660(&sp164, sp17A);
                        break;
                    case 2:                         /* switch 3 */
                        fn_8132C660(&sp164, sp17C);
                        break;
                    }
                    lbl_814AD618.unk0 = sp164;
                    lbl_814AD61C.unk0 = sp164;
                    lbl_814AD618.unk2 = sp166;
                    lbl_814AD61C.unk2 = sp166;
                }
                temp_r19_4 = fn_8132CFF4()->unk14;
                if ((s16) fn_8132CFE8()->unk14 != temp_r19_4) {
                    lbl_814AD620.unk0 = Menu_Get()->unkD4;
                    (&lbl_814AD620)[1] = Menu_Get()->unkD4;
                    temp_r0_4 = fn_8132CFE8(&lbl_814AD620)->unk14;
                    switch (temp_r0_4) {            /* switch 4; irregular */
                    case 0:                         /* switch 4 */
                        fn_8132C660(&sp164, sp17E);
                        break;
                    case 1:                         /* switch 4 */
                        fn_8132C660(&sp164, sp180);
                        break;
                    case 2:                         /* switch 4 */
                        fn_8132C660(&sp164, sp182);
                        break;
                    }
                    lbl_814AD618.unk0 = sp164;
                    lbl_814AD61C.unk0 = sp164;
                    lbl_814AD618.unk2 = sp166;
                    lbl_814AD61C.unk2 = sp166;
                }
                var_r24 = var_r28;
                var_r23 = &lbl_815531C0;
                var_r22 = fn_81332D7C(lbl_814AD61C.unk2);
                var_r25 = &lbl_815531C0 + 0xE70;
                var_r21 = 0;
loop_90:
                if (var_r21 < (s32) temp_r27) {
                    temp_f31 = fn_81336594();
                    temp_r4 = *var_r24 << 5;
                    temp_f3 = Menu_Get()->unkD0;
                    temp_f2 = (f32) *var_r25 * temp_f3;
                    temp_f1 = -lbl_815531C0.unkF50 * temp_f2 * temp_f31;
                    var_r23->unk14 = (f32) (*(arg0 + (temp_r4 + 0x14)) + temp_f1);
                    temp_f31_2 = fn_81336594(temp_r4, temp_f1, temp_f2, temp_f3);
                    temp_r4_2 = *var_r24 << 5;
                    temp_f3_2 = Menu_Get()->unkD0;
                    temp_f2_2 = (f32) *var_r25 * temp_f3_2;
                    temp_f1_2 = -lbl_815531C0.unkF54 * temp_f2_2 * temp_f31_2;
                    var_r23->unk18 = (f32) (*(arg0 + (temp_r4_2 + 0x18)) + temp_f1_2);
                    temp_f31_3 = fn_81336594(temp_r4_2, temp_f1_2, temp_f2_2, temp_f3_2);
                    temp_r4_3 = *var_r24 << 5;
                    temp_f3_3 = Menu_Get()->unkD0;
                    temp_f2_3 = (f32) *var_r25 * temp_f3_3;
                    temp_f1_3 = -lbl_815531C0.unkF58 * temp_f2_3 * temp_f31_3;
                    var_r23->unk1C = (f32) (*(arg0 + (temp_r4_3 + 0x1C)) + temp_f1_3);
                    var_r23->unkC = (s16) *(arg0 + ((*var_r24 << 5) + 0xC));
                    var_r23->unkE = (s16) *(arg0 + ((*var_r24 << 5) + 0xE));
                    var_r23->unk10 = (s16) *(arg0 + ((*var_r24 << 5) + 0x10));
                    var_r23->unk0 = (f32) *(arg0 + (*var_r24 << 5));
                    var_r23->unk4 = (f32) *(arg0 + ((*var_r24 << 5) + 4));
                    temp_r3 = *var_r24 << 5;
                    var_r23->unk8 = (f32) *(arg0 + (temp_r3 + 8));
                    var_r25 += 2;
                    temp_r3_2 = &lbl_815531C0 + *var_r22;
                    var_r24 += 2;
                    var_r23 += 0x20;
                    var_r22 += 2;
                    var_r21 += 1;
                    temp_r3_2->unkE00 = (s8) (255.0f * (1.0f - fn_81336594(temp_r3, (bitwise f32) temp_r4_3, temp_f1_3, temp_f2_3, temp_f3_3)));
                    goto loop_90;
                }
                var_r22_2 = var_r28;
                var_r23_2 = fn_81332D7C(lbl_814AD618.unk2);
                var_r20 = temp_r27 * 2;
                var_r19 = temp_r27 << 5;
                var_r21_2 = 0;
loop_93:
                if (var_r21_2 < (s32) temp_r27) {
                    temp_f28 = 1.0f - fn_81336594();
                    temp_r25 = &lbl_815531C0 + var_r20;
                    temp_r4_4 = temp_r25->unkE70;
                    temp_r29 = &lbl_815531C0 + var_r19;
                    temp_f3_4 = Menu_Get()->unkD0;
                    temp_f2_4 = (f32) temp_r4_4 * temp_f3_4;
                    temp_f1_4 = lbl_815531C0.unkF50 * temp_f2_4 * temp_f28;
                    temp_r29->unk14 = (f32) (*(arg0 + ((*var_r22_2 << 5) + 0x14)) + temp_f1_4);
                    temp_f28_2 = 1.0f - fn_81336594(temp_r4_4, temp_f1_4, temp_f2_4, temp_f3_4);
                    temp_f3_5 = Menu_Get()->unkD0;
                    temp_r3_3 = *var_r22_2 << 5;
                    temp_f2_5 = (f32) temp_r25->unkE70 * temp_f3_5;
                    temp_f1_5 = lbl_815531C0.unkF54 * temp_f2_5 * temp_f28_2;
                    temp_r29->unk18 = (f32) (*(arg0 + (temp_r3_3 + 0x18)) + temp_f1_5);
                    temp_f28_3 = 1.0f - fn_81336594(temp_r3_3, temp_f1_5, temp_f2_5, temp_f3_5);
                    temp_f3_6 = Menu_Get()->unkD0;
                    temp_f2_6 = (f32) temp_r25->unkE70 * temp_f3_6;
                    temp_f1_6 = lbl_815531C0.unkF58 * temp_f2_6 * temp_f28_3;
                    temp_r29->unk1C = (f32) (*(arg0 + ((*var_r22_2 << 5) + 0x1C)) + temp_f1_6);
                    temp_r29->unkC = (s16) *(arg0 + ((*var_r22_2 << 5) + 0xC));
                    temp_r29->unkE = (s16) *(arg0 + ((*var_r22_2 << 5) + 0xE));
                    temp_r29->unk10 = (s16) *(arg0 + ((*var_r22_2 << 5) + 0x10));
                    temp_r29->unk0 = (f32) *(arg0 + (*var_r22_2 << 5));
                    temp_r29->unk4 = (f32) *(arg0 + ((*var_r22_2 << 5) + 4));
                    temp_r3_4 = *var_r22_2 << 5;
                    temp_r29->unk8 = (f32) *(arg0 + (temp_r3_4 + 8));
                    temp_r0_5 = *var_r23_2;
                    var_r19 += 0x20;
                    var_r20 += 2;
                    var_r22_2 += 2;
                    var_r23_2 += 2;
                    var_r21_2 += 1;
                    *(&lbl_815531C0 + temp_r27 + 0xE00 + temp_r0_5) = (s8) (255.0f * fn_81336594(temp_r3_4, temp_f1_6, temp_f2_6, temp_f3_6));
                    goto loop_93;
                }
                var_r22_3 = var_r26;
                var_r23_3 = fn_81332D7C(lbl_814AD61C.unk0);
                var_r19_2 = temp_r27 * 4;
                var_r20_2 = temp_r27 << 6;
                var_r21_3 = 0;
loop_96:
                if (var_r21_3 < (s32) temp_r27) {
                    temp_r25_2 = &lbl_815531C0 + var_r19_2;
                    temp_r4_5 = temp_r25_2->unkE70;
                    temp_r29_2 = &lbl_815531C0 + var_r20_2;
                    temp_r3_5 = *var_r22_3 << 5;
                    temp_f3_7 = -lbl_815531C0.unkF50;
                    temp_f2_7 = temp_f3_7 * (f32) temp_r4_5;
                    temp_f1_7 = temp_f2_7 * fn_813365F4();
                    temp_r29_2->unk14 = (f32) (*(arg0 + (temp_r3_5 + 0x14)) + temp_f1_7);
                    temp_f3_8 = -lbl_815531C0.unkF54;
                    temp_r3_6 = *var_r22_3 << 5;
                    temp_f2_8 = temp_f3_8 * (f32) temp_r25_2->unkE70;
                    temp_f1_8 = temp_f2_8 * fn_813365F4(temp_r3_5, temp_r4_5, temp_f1_7, temp_f2_7, temp_f3_7);
                    temp_r29_2->unk18 = (f32) (*(arg0 + (temp_r3_6 + 0x18)) + temp_f1_8);
                    temp_f3_9 = -lbl_815531C0.unkF58;
                    temp_f2_9 = temp_f3_9 * (f32) temp_r25_2->unkE70;
                    temp_f1_9 = temp_f2_9 * fn_813365F4(temp_r3_6, (bitwise s16) temp_f1_8, temp_f2_8, temp_f3_8);
                    temp_r29_2->unk1C = (f32) (*(arg0 + ((*var_r22_3 << 5) + 0x1C)) + temp_f1_9);
                    temp_r29_2->unkC = (s16) *(arg0 + ((*var_r22_3 << 5) + 0xC));
                    temp_r29_2->unkE = (s16) *(arg0 + ((*var_r22_3 << 5) + 0xE));
                    temp_r29_2->unk10 = (s16) *(arg0 + ((*var_r22_3 << 5) + 0x10));
                    temp_r29_2->unk0 = (f32) *(arg0 + (*var_r22_3 << 5));
                    temp_r29_2->unk4 = (f32) *(arg0 + ((*var_r22_3 << 5) + 4));
                    temp_r3_7 = *var_r22_3 << 5;
                    temp_r29_2->unk8 = (f32) *(arg0 + (temp_r3_7 + 8));
                    temp_r0_6 = *var_r23_3;
                    var_r20_2 += 0x20;
                    var_r19_2 += 2;
                    var_r22_3 += 2;
                    var_r23_3 += 2;
                    var_r21_3 += 1;
                    *(&(&lbl_815531C0)[temp_r27] + 0xE00 + temp_r0_6) = (s8) (255.0f * (1.0f - fn_813365F4(temp_r3_7, (bitwise s16) temp_f1_9, temp_f2_9, temp_f3_9)));
                    goto loop_96;
                }
                var_r22_4 = var_r26;
                var_r23_4 = fn_81332D7C(lbl_814AD618.unk0);
                var_r20_3 = temp_r27 * 6;
                var_r21_4 = temp_r27 * 0x60;
                var_r29 = 0;
loop_99:
                if (var_r29 < (s32) temp_r27) {
                    temp_r25_3 = &lbl_815531C0 + var_r20_3;
                    temp_r4_6 = temp_r25_3->unkE70;
                    temp_f3_10 = 1.0f - fn_813365F4();
                    temp_r28 = &lbl_815531C0 + var_r21_4;
                    temp_f1_10 = lbl_815531C0.unkF50 * (f32) temp_r4_6 * temp_f3_10;
                    temp_r28->unk14 = (f32) (*(arg0 + ((*var_r22_4 << 5) + 0x14)) + temp_f1_10);
                    temp_f3_11 = 1.0f - fn_813365F4(temp_r4_6, (bitwise s16) temp_f1_10, lbl_815531C0.unkF50, temp_f3_10);
                    temp_r3_8 = *var_r22_4 << 5;
                    temp_f1_11 = lbl_815531C0.unkF54 * (f32) temp_r25_3->unkE70 * temp_f3_11;
                    temp_r28->unk18 = (f32) (*(arg0 + (temp_r3_8 + 0x18)) + temp_f1_11);
                    temp_f3_12 = 1.0f - fn_813365F4(temp_r3_8, (bitwise s16) temp_f1_11, lbl_815531C0.unkF54, temp_f3_11);
                    temp_f1_12 = lbl_815531C0.unkF58 * (f32) temp_r25_3->unkE70 * temp_f3_12;
                    temp_r28->unk1C = (f32) (*(arg0 + ((*var_r22_4 << 5) + 0x1C)) + temp_f1_12);
                    temp_r28->unkC = (s16) *(arg0 + ((*var_r22_4 << 5) + 0xC));
                    temp_r28->unkE = (s16) *(arg0 + ((*var_r22_4 << 5) + 0xE));
                    temp_r28->unk10 = (s16) *(arg0 + ((*var_r22_4 << 5) + 0x10));
                    temp_r28->unk0 = (f32) *(arg0 + (*var_r22_4 << 5));
                    temp_r28->unk4 = (f32) *(arg0 + ((*var_r22_4 << 5) + 4));
                    temp_r3_9 = *var_r22_4 << 5;
                    temp_r28->unk8 = (f32) *(arg0 + (temp_r3_9 + 8));
                    temp_r0_7 = *var_r23_4;
                    var_r21_4 += 0x20;
                    var_r20_3 += 2;
                    var_r22_4 += 2;
                    var_r23_4 += 2;
                    var_r29 += 1;
                    *(&lbl_815531C0 + (temp_r27 * 3) + 0xE00 + temp_r0_7) = (s8) (255.0f * fn_813365F4(temp_r3_9, (bitwise s16) temp_f1_12, lbl_815531C0.unkF58, temp_f3_12));
                    goto loop_99;
                }
                if ((s16) lbl_814AD620.unk0 < (s16) Menu_Get()->unkD4) {
                    lbl_814AD620.unk0 += 1;
                }
                if ((s16) lbl_814AD620.unk2 < (s16) Menu_Get()->unkD4) {
                    lbl_814AD620.unk2 = (s16) (lbl_814AD620.unk2 + 1);
                }
            }
            return;
        }
        fn_8133682C();
        lbl_814AD624 = 0;
        return;
    }
    lbl_814AD624 = 0;
}

/* 0x81336594 */
f32 fn_81336594(void) {
    return (f32) lbl_814AD620.unk2 / (f32) Menu_Get()->unkD4;
}

/* 0x813365F4 */
f32 fn_813365F4(void) {
    return (f32) lbl_814AD620 / (f32) Menu_Get()->unkD4;
}

/* 0x81336650 */
void fn_81336650(f32 *arg0, s16 arg1, s16 arg2) {
    s16 sp28;
    s16 sp20;
    s16 *var_r27;
    s16 var_r26;
    s32 temp_r3;

    if (arg1 != arg2) {
        if ((s16) (arg2 - arg1) > 0) {
            lbl_815531C0.unkF50 = (f32) arg0->unk0;
            lbl_815531C0.unkF54 = (f32) arg0->unk4;
            lbl_815531C0.unkF58 = (f32) arg0->unk8;
        } else {
            lbl_815531C0.unkF50 = (f32) -arg0->unk0;
            lbl_815531C0.unkF54 = (f32) -arg0->unk4;
            lbl_815531C0.unkF58 = (f32) -arg0->unk8;
        }
        var_r27 = &lbl_815531C0 + 0xE70;
        var_r26 = 0;
loop_6:
        if (var_r26 < 0x70) {
            temp_r3 = Rand() % 600;
            var_r26 += 1;
            *var_r27 = (s16) ((Menu_Get((void *) temp_r3)->unkCC * (f32) (temp_r3 + 0xC8)) / 10.0f);
            var_r27 += 2;
            goto loop_6;
        }
        fn_8132C660(&sp28, (s16) fabs((f64) arg1));
        fn_8132C660(&sp20, (s16) fabs((f64) arg2));
        if (sp2A != sp22) {
            lbl_814AD620.unk2 = 0;
            lbl_814AD61C.unk2 = sp2A;
            lbl_814AD618.unk2 = sp22;
        }
        if (sp28 != sp20) {
            lbl_814AD61C.unk0 = sp28;
            lbl_814AD620.unk0 = 0;
            lbl_814AD618.unk0 = sp20;
        }
    }
}

/* 0x8133682C */
void fn_8133682C(void) {
    s16 sp10;
    s16 sp8;
    s16 *temp_r31;

    temp_r31 = fn_8132CFE8();
    fn_8132A690(&sp10, fn_8132CFE8()->unkE, temp_r31 + 0x16);
    if ((s16) fn_8132CFE8()->unkE == 1) {
        fn_8132C660(&sp8, (&sp10)[fn_8132CFE8()->unk12]);
        goto block_5;
    }
    if ((s16) fn_8132CFE8()->unkE == 2) {
        fn_8132C660(&sp8, (&sp16[0])[fn_8132CFE8()->unk14]);
block_5:
        lbl_814AD61C.unk0 = sp8;
        lbl_814AD618.unk0 = sp8;
        lbl_814AD61C.unk2 = spA;
        lbl_814AD618.unk2 = spA;
    }
}

/* 0x813368F0 */
void fn_813368F0(void) {
    s16 sp18;

    fn_8132C660(&sp18, (s16) fabs((f64) fn_8132CFE8()->unkA));
    lbl_814AD618.unk0 = sp18;
    lbl_814AD61C.unk0 = sp18;
    lbl_814AD618.unk2 = sp1A;
    lbl_814AD61C.unk2 = sp1A;
}

/* 0x81336968 */
s16 fn_81336968(void) {
    return lbl_814AD62E;
}

/* 0x81336970 */
void *fn_81336970(void) {
    return &lbl_8155F3E0;
}

/* 0x8133697C */
s16 *fn_8133697C(void) {
    return &lbl_81554120;
}

/* 0x81336988 */
void fn_81336988(void) {
    lbl_814AD628 = Menu_Get()->unkD4;
}

/* 0x813369B0 */
void fn_813369B0(s32 arg0) {
    s32 sp44;
    s16 *sp40;
    f32 sp3C;
    s32 sp38;
    s16 *sp34;
    f32 sp30;
    s32 sp2C;
    s16 *sp28;
    f32 sp24;
    s32 sp20;
    s16 *sp1C;
    f32 sp18;
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f2;
    f32 temp_f2_2;
    f32 temp_f2_3;
    f32 temp_f3;
    f32 temp_f3_2;
    f32 temp_f3_3;
    f32 temp_f3_4;
    f32 temp_f3_5;
    f32 temp_f3_6;
    s16 *temp_r19;
    s16 *temp_r19_2;
    s16 *temp_r24_2;
    s16 *temp_r27;
    s16 *var_r21;
    s16 *var_r22;
    s16 *var_r24;
    s16 *var_r3;
    s16 temp_r3;
    s16 temp_r3_2;
    s16 temp_r3_3;
    s16 temp_r3_4;
    s16 temp_r3_5;
    s16 temp_r3_6;
    s16 temp_r4;
    s32 var_ctr;
    s32 var_ctr_2;
    s32 var_r19;
    s32 var_r20_2;
    s32 var_r20_3;
    s32 var_r30_2;
    s32 var_r4;
    s8 *var_r22_2;
    s8 *var_r3_2;
    u16 *var_r21_2;
    u16 *var_r23;
    u16 *var_r30;
    u16 var_r20;
    u16 var_r29;
    void *temp_r24;
    void *temp_r26;

    var_r20 = saved_reg_r20;
    var_r29 = saved_reg_r29;
    var_r30 = saved_reg_r30;
    if (((s16) fn_8132CFE8(&lbl_81554120)->unk6 == 1) && ((s16) fn_8132CFF4()->unk6 == 1)) {
        lbl_814AD62E = 0x596;
        var_ctr = 0x59;
        var_r4 = 0;
        var_r3 = &lbl_81554120 + 0xB2C0;
        do {
            var_r3->unk0 = 0;
            var_r4 += 0x10;
            var_r3->unk1 = 0;
            var_r3->unk2 = 0;
            var_r3->unk3 = 0;
            var_r3->unk4 = 0;
            var_r3->unk5 = 0;
            var_r3->unk6 = 0;
            var_r3->unk7 = 0;
            var_r3->unk8 = 0;
            var_r3->unk9 = 0;
            var_r3->unkA = 0;
            var_r3->unkB = 0;
            var_r3->unkC = 0;
            var_r3->unkD = 0;
            var_r3->unkE = 0;
            var_r3->unkF = 0;
            var_r3 += 0x10;
            var_ctr -= 1;
        } while (var_ctr != 0);
        var_r3_2 = (&lbl_81554120 + 0x10000 + var_r4) - 0x4D40;
        var_ctr_2 = 0x596 - var_r4;
        if (var_r4 < 0x596) {
            do {
                *var_r3_2 = 0;
                var_r3_2 += 1;
                var_ctr_2 -= 1;
            } while (var_ctr_2 != 0);
        }
        switch ((s16) lbl_814AD62A) {               /* switch 1; irregular */
        case 0:                                     /* switch 1 */
            var_r30 = fn_81332714();
            var_r20 = fn_8133277C();
            break;
        case 1:                                     /* switch 1 */
            var_r30 = fn_81332CD8();
            var_r20 = fn_81332D40();
            break;
        }
        var_r23 = var_r30;
        var_r22 = &lbl_81554120;
        var_r20_2 = 0;
        var_r24 = &lbl_81554120 + 0xB858;
        var_r21 = &lbl_81554120 + 0xB2C0;
loop_17:
        if (var_r20_2 < (s32) var_r20) {
            temp_f3 = -lbl_81554120.unkC384;
            temp_r3 = *var_r23 << 5;
            temp_f2 = temp_f3 * (f32) *var_r24;
            temp_f1 = temp_f2 * fn_81337530();
            var_r22->unk14 = (f32) (*(arg0 + (temp_r3 + 0x14)) + temp_f1);
            temp_f3_2 = -lbl_81554120.unkC388;
            temp_r3_2 = *var_r23 << 5;
            temp_f2_2 = temp_f3_2 * (f32) *var_r24;
            temp_f1_2 = temp_f2_2 * fn_81337530(temp_r3, temp_f1, temp_f2, temp_f3);
            var_r22->unk18 = (f32) (*(arg0 + (temp_r3_2 + 0x18)) + temp_f1_2);
            temp_f3_3 = -lbl_81554120.unkC38C;
            temp_f2_3 = temp_f3_3 * (f32) *var_r24;
            temp_f1_3 = temp_f2_3 * fn_81337530(temp_r3_2, temp_f1_2, temp_f2_2, temp_f3_2);
            var_r22->unk1C = (f32) (*(arg0 + ((*var_r23 << 5) + 0x1C)) + temp_f1_3);
            var_r22[6] = *(arg0 + ((*var_r23 << 5) + 0xC));
            var_r22[7] = *(arg0 + ((*var_r23 << 5) + 0xE));
            var_r22[8] = *(arg0 + ((*var_r23 << 5) + 0x10));
            var_r22->unk0 = (f32) *(arg0 + (*var_r23 << 5));
            var_r22->unk4 = (f32) *(arg0 + ((*var_r23 << 5) + 4));
            temp_r3_3 = *var_r23 << 5;
            var_r22->unk8 = (f32) *(arg0 + (temp_r3_3 + 8));
            var_r24 += 2;
            var_r23 += 2;
            var_r22 += 0x20;
            var_r20_2 += 1;
            *var_r21 = (s8) (255.0f * (1.0f - fn_81337530(temp_r3_3, temp_f1_3, temp_f2_3, temp_f3_3)));
            var_r21 += 1;
            goto loop_17;
        }
        switch ((s16) lbl_814AD62C) {               /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            var_r30 = fn_81332714();
            var_r29 = fn_8133277C();
            break;
        case 1:                                     /* switch 2 */
            var_r30 = fn_81332CD8();
            var_r29 = fn_81332D40();
            break;
        }
        temp_r27 = &lbl_81554120 + 0x10000;
        var_r21_2 = var_r30;
        var_r19 = var_r20 * 2;
        var_r20_3 = var_r20 << 5;
        var_r30_2 = 0;
        var_r22_2 = (temp_r27 + var_r20) - 0x4D40;
loop_26:
        if (var_r30_2 < (s32) var_r29) {
            temp_r24 = temp_r27 + var_r19;
            temp_r4 = temp_r24->unk-47A8;
            temp_r24_2 = temp_r24 - 0x47A8;
            temp_f3_4 = 1.0f - fn_81337530();
            temp_r26 = &lbl_81554120 + var_r20_3;
            temp_f1_4 = lbl_81554120.unkC384 * (f32) temp_r4 * temp_f3_4;
            temp_r26->unk14 = (f32) (*(arg0 + ((*var_r21_2 << 5) + 0x14)) + temp_f1_4);
            temp_f3_5 = 1.0f - fn_81337530(temp_r4, temp_f1_4, lbl_81554120.unkC384, temp_f3_4);
            temp_r3_4 = *var_r21_2 << 5;
            temp_f1_5 = lbl_81554120.unkC388 * (f32) *temp_r24_2 * temp_f3_5;
            temp_r26->unk18 = (f32) (*(arg0 + (temp_r3_4 + 0x18)) + temp_f1_5);
            temp_f3_6 = 1.0f - fn_81337530(temp_r3_4, temp_f1_5, lbl_81554120.unkC388, temp_f3_5);
            temp_f1_6 = lbl_81554120.unkC38C * (f32) *temp_r24_2 * temp_f3_6;
            temp_r26->unk1C = (f32) (*(arg0 + ((*var_r21_2 << 5) + 0x1C)) + temp_f1_6);
            temp_r26->unkC = (s16) *(arg0 + ((*var_r21_2 << 5) + 0xC));
            temp_r26->unkE = (s16) *(arg0 + ((*var_r21_2 << 5) + 0xE));
            temp_r26->unk10 = (s16) *(arg0 + ((*var_r21_2 << 5) + 0x10));
            temp_r26->unk0 = (f32) *(arg0 + (*var_r21_2 << 5));
            temp_r26->unk4 = (f32) *(arg0 + ((*var_r21_2 << 5) + 4));
            temp_r3_5 = *var_r21_2 << 5;
            temp_r26->unk8 = (f32) *(arg0 + (temp_r3_5 + 8));
            var_r20_3 += 0x20;
            var_r19 += 2;
            var_r21_2 += 2;
            var_r30_2 += 1;
            *var_r22_2 = (s8) (255.0f * fn_81337530(temp_r3_5, temp_f1_6, lbl_81554120.unkC38C, temp_f3_6));
            var_r22_2 += 1;
            goto loop_26;
        }
        if ((s16) fn_8132CFE8()->unkC != 0) {
            sp30 = lbl_8137F030.unk0;
            sp34 = lbl_8137F030.unk4;
            sp38 = lbl_8137F030.unk8;
            sp3C = sp30;
            sp40 = sp34;
            sp44 = lbl_8137F030.unk8;
            temp_r19 = fn_8132CFE8(sp34, &lbl_8137F030);
            fn_8133758C(&sp3C, fn_8132CFF4()->unkC, temp_r19->unkC);
        } else {
            sp18 = lbl_8137F03C.unk0;
            sp1C = lbl_8137F03C.unk4;
            sp20 = lbl_8137F03C.unk8;
            sp24 = sp18;
            sp28 = sp1C;
            sp2C = lbl_8137F03C.unk8;
            temp_r19_2 = fn_8132CFE8(sp1C, &lbl_8137F03C);
            fn_8133758C(&sp24, fn_8132CFF4()->unkC, temp_r19_2->unkC);
        }
        if ((s16) lbl_814AD628 < (s16) Menu_Get()->unkD4) {
            lbl_814AD628 += 1;
            return;
        }
        return;
    }
    temp_r3_6 = fn_8132CFE8()->unkC;
    lbl_814AD62A = temp_r3_6;
    lbl_814AD62C = temp_r3_6;
    lbl_814AD62E = 0;
}

/* 0x81336F9C */
void fn_81336F9C(s32 arg0) {
    s32 sp28;
    s16 *sp24;
    f32 sp20;
    s32 sp1C;
    s16 *sp18;
    f32 sp14;
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f2;
    f32 temp_f2_2;
    f32 temp_f2_3;
    f32 temp_f3;
    f32 temp_f3_2;
    f32 temp_f3_3;
    f32 temp_f3_4;
    f32 temp_f3_5;
    f32 temp_f3_6;
    s16 *temp_r20;
    s16 *temp_r25_2;
    s16 *temp_r28;
    s16 *var_r23;
    s16 *var_r24;
    s16 *var_r26;
    s16 *var_r3;
    s16 temp_r3;
    s16 temp_r3_2;
    s16 temp_r3_3;
    s16 temp_r3_4;
    s16 temp_r3_5;
    s16 temp_r3_6;
    s16 temp_r4;
    s32 var_ctr;
    s32 var_ctr_2;
    s32 var_r20_2;
    s32 var_r21;
    s32 var_r22;
    s32 var_r27;
    s32 var_r4;
    s8 *var_r23_2;
    s8 *var_r3_2;
    u16 *var_r20;
    u16 *var_r22_2;
    u16 *var_r25;
    u16 var_r3_3;
    u16 var_r3_4;
    void *temp_r25;
    void *temp_r26;

    if (((s16) *fn_8132CFE8(&lbl_81554120) == 3) && ((s16) *fn_8132CFF4() == 3)) {
        lbl_814AD62E = 0x596;
        var_ctr = 0x59;
        var_r4 = 0;
        var_r3 = &lbl_81554120 + 0xB2C0;
        do {
            var_r3->unk0 = 0;
            var_r4 += 0x10;
            var_r3->unk1 = 0;
            var_r3->unk2 = 0;
            var_r3->unk3 = 0;
            var_r3->unk4 = 0;
            var_r3->unk5 = 0;
            var_r3->unk6 = 0;
            var_r3->unk7 = 0;
            var_r3->unk8 = 0;
            var_r3->unk9 = 0;
            var_r3->unkA = 0;
            var_r3->unkB = 0;
            var_r3->unkC = 0;
            var_r3->unkD = 0;
            var_r3->unkE = 0;
            var_r3->unkF = 0;
            var_r3 += 0x10;
            var_ctr -= 1;
        } while (var_ctr != 0);
        var_r3_2 = (&lbl_81554120 + 0x10000 + var_r4) - 0x4D40;
        var_ctr_2 = 0x596 - var_r4;
        if (var_r4 < 0x596) {
            do {
                *var_r3_2 = 0;
                var_r3_2 += 1;
                var_ctr_2 -= 1;
            } while (var_ctr_2 != 0);
        }
        switch ((s16) lbl_814AD62A) {               /* switch 1; irregular */
        default:                                    /* switch 1 */
            var_r20 = fn_813323E0();
            var_r3_3 = fn_81332448();
            break;
        case 1:                                     /* switch 1 */
            var_r20 = fn_8133233C();
            var_r3_3 = fn_813323A4();
            break;
        case 2:                                     /* switch 1 */
            var_r20 = fn_81332484();
            var_r3_3 = fn_813324EC();
            break;
        }
        var_r25 = var_r20;
        var_r24 = &lbl_81554120;
        var_r22 = 0;
        var_r26 = &lbl_81554120 + 0xB858;
        var_r23 = &lbl_81554120 + 0xB2C0;
loop_19:
        if (var_r22 < (s32) var_r3_3) {
            temp_f3 = -lbl_81554120.unkC384;
            temp_r3 = *var_r25 << 5;
            temp_f2 = temp_f3 * (f32) *var_r26;
            temp_f1 = temp_f2 * fn_81337530();
            var_r24->unk14 = (f32) (*(arg0 + (temp_r3 + 0x14)) + temp_f1);
            temp_f3_2 = -lbl_81554120.unkC388;
            temp_r3_2 = *var_r25 << 5;
            temp_f2_2 = temp_f3_2 * (f32) *var_r26;
            temp_f1_2 = temp_f2_2 * fn_81337530(temp_r3, temp_f1, temp_f2, temp_f3);
            var_r24->unk18 = (f32) (*(arg0 + (temp_r3_2 + 0x18)) + temp_f1_2);
            temp_f3_3 = -lbl_81554120.unkC38C;
            temp_f2_3 = temp_f3_3 * (f32) *var_r26;
            temp_f1_3 = temp_f2_3 * fn_81337530(temp_r3_2, temp_f1_2, temp_f2_2, temp_f3_2);
            var_r24->unk1C = (f32) (*(arg0 + ((*var_r25 << 5) + 0x1C)) + temp_f1_3);
            var_r24[6] = *(arg0 + ((*var_r25 << 5) + 0xC));
            var_r24[7] = *(arg0 + ((*var_r25 << 5) + 0xE));
            var_r24[8] = *(arg0 + ((*var_r25 << 5) + 0x10));
            var_r24->unk0 = (f32) *(arg0 + (*var_r25 << 5));
            var_r24->unk4 = (f32) *(arg0 + ((*var_r25 << 5) + 4));
            temp_r3_3 = *var_r25 << 5;
            var_r24->unk8 = (f32) *(arg0 + (temp_r3_3 + 8));
            var_r26 += 2;
            var_r25 += 2;
            var_r24 += 0x20;
            var_r22 += 1;
            *var_r23 = (s8) (255.0f * (1.0f - fn_81337530(temp_r3_3, temp_f1_3, temp_f2_3, temp_f3_3)));
            var_r23 += 1;
            goto loop_19;
        }
        switch ((s16) lbl_814AD62C) {               /* switch 2; irregular */
        default:                                    /* switch 2 */
            var_r22_2 = fn_813323E0();
            var_r3_4 = fn_81332448();
            break;
        case 1:                                     /* switch 2 */
            var_r22_2 = fn_8133233C();
            var_r3_4 = fn_813323A4();
            break;
        case 2:                                     /* switch 2 */
            var_r22_2 = fn_81332484();
            var_r3_4 = fn_813324EC();
            break;
        }
        temp_r28 = &lbl_81554120 + 0x10000;
        var_r20_2 = var_r3_3 * 2;
        var_r21 = var_r3_3 << 5;
        var_r27 = 0;
        var_r23_2 = (temp_r28 + var_r3_3) - 0x4D40;
loop_30:
        if (var_r27 < (s32) var_r3_4) {
            temp_r25 = temp_r28 + var_r20_2;
            temp_r4 = temp_r25->unk-47A8;
            temp_r25_2 = temp_r25 - 0x47A8;
            temp_f3_4 = 1.0f - fn_81337530();
            temp_r26 = &lbl_81554120 + var_r21;
            temp_f1_4 = lbl_81554120.unkC384 * (f32) temp_r4 * temp_f3_4;
            temp_r26->unk14 = (f32) (*(arg0 + ((*var_r22_2 << 5) + 0x14)) + temp_f1_4);
            temp_f3_5 = 1.0f - fn_81337530(temp_r4, temp_f1_4, lbl_81554120.unkC384, temp_f3_4);
            temp_r3_4 = *var_r22_2 << 5;
            temp_f1_5 = lbl_81554120.unkC388 * (f32) *temp_r25_2 * temp_f3_5;
            temp_r26->unk18 = (f32) (*(arg0 + (temp_r3_4 + 0x18)) + temp_f1_5);
            temp_f3_6 = 1.0f - fn_81337530(temp_r3_4, temp_f1_5, lbl_81554120.unkC388, temp_f3_5);
            temp_f1_6 = lbl_81554120.unkC38C * (f32) *temp_r25_2 * temp_f3_6;
            temp_r26->unk1C = (f32) (*(arg0 + ((*var_r22_2 << 5) + 0x1C)) + temp_f1_6);
            temp_r26->unkC = (s16) *(arg0 + ((*var_r22_2 << 5) + 0xC));
            temp_r26->unkE = (s16) *(arg0 + ((*var_r22_2 << 5) + 0xE));
            temp_r26->unk10 = (s16) *(arg0 + ((*var_r22_2 << 5) + 0x10));
            temp_r26->unk0 = (f32) *(arg0 + (*var_r22_2 << 5));
            temp_r26->unk4 = (f32) *(arg0 + ((*var_r22_2 << 5) + 4));
            temp_r3_5 = *var_r22_2 << 5;
            temp_r26->unk8 = (f32) *(arg0 + (temp_r3_5 + 8));
            var_r21 += 0x20;
            var_r20_2 += 2;
            var_r22_2 += 2;
            var_r27 += 1;
            *var_r23_2 = (s8) (255.0f * fn_81337530(temp_r3_5, temp_f1_6, lbl_81554120.unkC38C, temp_f3_6));
            var_r23_2 += 1;
            goto loop_30;
        }
        sp14 = lbl_8137F048.unk0;
        sp18 = lbl_8137F048.unk4;
        sp1C = lbl_8137F048.unk8;
        sp20 = sp14;
        sp24 = sp18;
        sp28 = lbl_8137F048.unk8;
        temp_r20 = fn_8132CFE8(sp18, &lbl_8137F048);
        fn_8133758C(&sp20, fn_8132CFF4()->unk4, temp_r20->unk4);
        if ((s16) lbl_814AD628 < (s16) Menu_Get()->unkD4) {
            lbl_814AD628 += 1;
            return;
        }
        return;
    }
    temp_r3_6 = fn_8132CFE8()->unk4;
    lbl_814AD62A = temp_r3_6;
    lbl_814AD62C = temp_r3_6;
    lbl_814AD62E = 0;
}

/* 0x81337530 */
f32 fn_81337530(void) {
    return (f32) lbl_814AD628 / (f32) Menu_Get()->unkD4;
}

/* 0x8133758C */
void fn_8133758C(f32 *arg0, s16 arg1, s16 arg2) {
    s16 *var_r27;
    s32 temp_r3;
    s32 var_r26;

    if (arg1 != arg2) {
        lbl_81554120.unkC384 = (f32) arg0->unk0;
        var_r27 = &lbl_81554120 + 0xB858;
        var_r26 = 0;
        lbl_81554120.unkC388 = (f32) arg0->unk4;
        lbl_81554120.unkC38C = (f32) arg0->unk8;
        do {
            temp_r3 = Rand() % 600;
            var_r26 += 1;
            *var_r27 = (s16) ((Menu_Get((void *) temp_r3)->unkCC * (f32) (temp_r3 + 0xC8)) / 10.0f);
            var_r27 += 2;
        } while (var_r26 < 0x596);
        lbl_814AD62A = arg1;
        lbl_814AD628 = 0;
        lbl_814AD62C = arg2;
    }
}
