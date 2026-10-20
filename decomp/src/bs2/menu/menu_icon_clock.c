// Non-matching: functionally equivalent decompilation
//
// Submenu icon cubes, 0x81326114..0x813275CC: Calendar icon (clock made of cubes, hands follow the time).
// Each icon has Init / UpdateFace (shown on the cube face) / UpdateEnter
// (zooming into the submenu) / UpdateHide / CalcMtx / Draw.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x81326114 */
void ClockIcon_Init(s32 arg0, s32 arg1) {
    s16 var_r29;
    s16 var_r29_2;
    s16 var_r29_3;
    void *var_r30;
    void *var_r30_2;
    void *var_r30_3;

    if (arg1 != 0) {
        lbl_814DDE40.unk608 = arg0;
        J3D_InitModel(&lbl_814DDE40 + 0x5A0, 0);
        var_r30 = &lbl_814DDE40 + (0 * 0x78);
        var_r29 = 0;
loop_3:
        if (var_r29 < 5) {
            J3D_CopyInstance(var_r30, &lbl_814DDE40 + 0x5A0);
            var_r30 += 0x78;
            var_r29 += 1;
            goto loop_3;
        }
        var_r29_2 = 0;
        var_r30_2 = &lbl_814DDE40 + (0 * 0x78) + 0x258;
loop_6:
        if (var_r29_2 < 4) {
            J3D_CopyInstance(var_r30_2, &lbl_814DDE40 + 0x5A0);
            var_r30_2 += 0x78;
            var_r29_2 += 1;
            goto loop_6;
        }
        var_r29_3 = 0;
        var_r30_3 = &lbl_814DDE40 + (0 * 0x78) + 0x438;
loop_9:
        if (var_r29_3 < 3) {
            J3D_CopyInstance(var_r30_3, &lbl_814DDE40 + 0x5A0);
            var_r30_3 += 0x78;
            var_r29_3 += 1;
            goto loop_9;
        }
    }
    lbl_814AD568 = 0;
    sClockIconSettled = 0;
    lbl_814DDE40.unk614 = 0;
    lbl_814DDE40.unk74 = 0;
    lbl_814DDE40.unkEC = 0;
    lbl_814DDE40.unk164 = 0;
    lbl_814DDE40.unk1DC = 0;
    lbl_814DDE40.unk254 = 0;
    lbl_814DDE40.unk2CC = 0;
    lbl_814DDE40.unk344 = 0;
    lbl_814DDE40.unk3BC = 0;
    lbl_814DDE40.unk434 = 0;
    lbl_814DDE40.unk4AC = 0;
    lbl_814DDE40.unk524 = 0;
    lbl_814DDE40.unk59C = 0;
    ClockIcon_InitHands();
}

/* 0x81326248 */
void ClockIcon_InitHands(void) {
    void *temp_r4;
    void *temp_r5;
    void *temp_r6;
    void *temp_r7;
    void *temp_r8;
    void *temp_r9;

    lbl_814AD568 = 0;
    temp_r4 = &lbl_814DDE40 + 0x618;
    temp_r5 = &lbl_814DDE40 + 0x61C;
    lbl_814DDE40.unk618 = 0.0f;
    temp_r6 = &lbl_814DDE40 + 0x640;
    temp_r7 = &lbl_814DDE40 + 0x644;
    lbl_814DDE40.unk61C = 0.0f;
    temp_r8 = &lbl_814DDE40 + 0x660;
    temp_r9 = &lbl_814DDE40 + 0x664;
    lbl_814DDE40.unk620 = 0.0f;
    temp_r5->unk8 = 0.0f;
    temp_r4->unk10 = 0.0f;
    temp_r5->unk10 = 0.0f;
    temp_r4->unk18 = 0.0f;
    temp_r5->unk18 = 0.0f;
    temp_r4->unk20 = 0.0f;
    temp_r5->unk20 = 0.0f;
    lbl_814DDE40.unk640 = 0.0f;
    lbl_814DDE40.unk644 = 0.0f;
    temp_r6->unk8 = 0.0f;
    temp_r7->unk8 = 0.0f;
    temp_r6->unk10 = 0.0f;
    temp_r7->unk10 = 0.0f;
    temp_r6->unk18 = 0.0f;
    temp_r7->unk18 = 0.0f;
    lbl_814DDE40.unk660 = 0.0f;
    lbl_814DDE40.unk664 = 0.0f;
    temp_r8->unk8 = 0.0f;
    temp_r9->unk8 = 0.0f;
    temp_r8->unk10 = 0.0f;
    temp_r9->unk10 = 0.0f;
    lbl_814AD560.unk0 = 0.0f;
    lbl_814AD560.unk4 = 0.0f;
}

/* 0x813262E4 */
void ClockIcon_UpdateFace(void) {
    s32 sp8;
    void *var_r26;
    s16 temp_r25;
    s16 var_r0;
    s16 var_r22;
    s16 var_r22_2;
    s16 var_r22_3;
    s32 temp_r23;
    s32 temp_r23_2;
    s32 var_r0_2;
    s32 var_r0_3;
    s32 var_r0_4;
    s32 var_r23;
    s32 var_r26_2;
    s32 var_r26_3;
    s32 var_r26_4;
    s32 var_r26_5;
    u32 temp_r4_2;
    u32 temp_r4_4;
    u8 temp_r0;
    u8 temp_r4;
    u8 temp_r4_3;
    void *var_r24;
    void *var_r24_2;
    void *var_r25;
    void *var_r25_2;
    void *var_r29;

    OSGetTime(&lbl_814DDE40);
    OSTicksToCalendarTime(&sp8);
    lbl_814AD568 += 1;
    if ((s16) lbl_814AD568 > 0x32) {
        lbl_814AD568 = 0x32;
    }
    var_r26 = &lbl_814DDE40;
    var_r29 = &lbl_814DDE40 + 0x618;
    var_r22 = 0;
loop_8:
    if (var_r22 < 5) {
        var_r23 = 0x64;
        if (var_r22 == 0) {
            var_r23 = 0;
        }
        var_r0 = lbl_814AD568;
        if (var_r0 > 0x1E) {
            var_r0 = 0x1E;
        }
        temp_r0 = (var_r0 * 0xFF) / 30;
        var_r26->unk74 = (s16) temp_r0;
        temp_r25 = (var_r22 - 1) << 0xE;
        var_r29->unk0 = (f32) ((f32) var_r23 * SinS(temp_r25, temp_r0));
        var_r26 += 0x78;
        var_r22 += 1;
        var_r29->unk4 = (f32) ((f32) var_r23 * CosS(temp_r25));
        var_r29 += 8;
        goto loop_8;
    }
    var_r25 = &lbl_814DDE40 + 0x258;
    var_r24 = &lbl_814DDE40 + 0x640;
    var_r22_2 = 0;
loop_20:
    if (var_r22_2 < 4) {
        if ((s16) lbl_814AD568 > 0x28) {
            var_r0_2 = lbl_814AD568 - 0x28;
        } else {
            var_r0_2 = 0;
        }
        var_r25->unk74 = (s16) (u8) ((var_r0_2 * 0xFF) / 10);
        if ((s16) lbl_814AD568 > 0x28) {
            var_r26_2 = lbl_814AD568 - 0x28;
        } else {
            var_r26_2 = 0;
        }
        temp_r4 = (sp8 << 0x10) / 3600;
        temp_r23 = (var_r22_2 + 1) * 0x14;
        var_r24->unk0 = (f32) (((f32) (temp_r23 * var_r26_2) * SinS(((spC * -0x10000) / 60) - temp_r4, temp_r4, (u8) ((sp8 << 0x10) / 3600) >> 0x1FU)) / 10.0f);
        if ((s16) lbl_814AD568 > 0x28) {
            var_r26_3 = lbl_814AD568 - 0x28;
        } else {
            var_r26_3 = 0;
        }
        temp_r4_2 = (sp8 << 0x10) / 3600;
        var_r25 += 0x78;
        var_r22_2 += 1;
        var_r24->unk4 = (f32) (((f32) (temp_r23 * var_r26_3) * CosS(((spC * -0x10000) / 60) - temp_r4_2, temp_r4_2, (u32) ((sp8 << 0x10) / 3600) >> 0x1FU)) / 10.0f);
        var_r24 += 8;
        goto loop_20;
    }
    var_r24_2 = &lbl_814DDE40 + 0x438;
    var_r25_2 = &lbl_814DDE40 + 0x660;
    var_r22_3 = 0;
loop_32:
    if (var_r22_3 < 3) {
        if ((s16) lbl_814AD568 > 0x28) {
            var_r0_3 = lbl_814AD568 - 0x28;
        } else {
            var_r0_3 = 0;
        }
        var_r24_2->unk74 = (s16) (u8) ((var_r0_3 * 0xFF) / 10);
        if ((s16) lbl_814AD568 > 0x28) {
            var_r26_4 = lbl_814AD568 - 0x28;
        } else {
            var_r26_4 = 0;
        }
        temp_r4_3 = (spC << 0x10) / 720;
        temp_r23_2 = (var_r22_3 + 1) * 0x14;
        var_r25_2->unk0 = (f32) (((f32) (temp_r23_2 * var_r26_4) * SinS((((sp10 % 12) * -0x10000) / 12) - temp_r4_3, temp_r4_3, (u32) ((spC << 0x10) / 720))) / 10.0f);
        if ((s16) lbl_814AD568 > 0x28) {
            var_r26_5 = lbl_814AD568 - 0x28;
        } else {
            var_r26_5 = 0;
        }
        temp_r4_4 = (spC << 0x10) / 720;
        var_r24_2 += 0x78;
        var_r22_3 += 1;
        var_r25_2->unk4 = (f32) (((f32) (temp_r23_2 * var_r26_5) * CosS((((sp10 % 12) * -0x10000) / 12) - temp_r4_4, temp_r4_4, (spC << 0x10) / 720)) / 10.0f);
        var_r25_2 += 8;
        goto loop_32;
    }
    if ((s16) lbl_814AD568 > 0x28) {
        var_r0_4 = lbl_814AD568 - 0x28;
    } else {
        var_r0_4 = 0;
    }
    lbl_814DDE40.unk614 = (s16) (u8) ((var_r0_4 * 0xFF) / 10);
    lbl_814AD560.unk0 = (f32) (100.0f * SinS(((sp8 * -0x10000) / 60) - var_r22_3));
    lbl_814AD560.unk4 = (f32) (100.0f * CosS(((sp8 * -0x10000) / 60) - var_r22_3));
}

/* 0x81326808 */
void ClockIcon_CalcMtx(void) {
    u8 sp108[0x4];
    u8 spD8[0x30];
    u8 spA8[0x30];
    u8 sp78[0x30];
    f32 sp74;
    f32 sp70;
    f32 sp6C;
    f32 sp68;
    f32 sp64;
    f32 sp60;
    s32 sp5C;
    f32 sp58;
    f32 sp54;
    s32 sp50;
    f32 sp4C;
    f32 sp48;
    s32 sp44;
    f32 sp40;
    f32 sp3C;
    s32 sp38;
    f32 sp34;
    f32 sp30;
    f32 sp2C;
    f32 sp28;
    f32 sp24;
    f32 sp20;
    f32 sp1C;
    f32 sp18;
    f32 temp_f1;
    s16 var_r0;
    s16 var_r0_2;
    s16 var_r25;
    s16 var_r28;
    s16 var_r28_2;
    s16 var_r4;
    s32 var_r0_3;
    s32 var_r0_4;
    s32 var_r0_5;
    void *var_r26;
    void *var_r26_2;
    void *var_r26_3;
    void *var_r27;
    void *var_r27_2;
    void *var_r27_3;

    MtxRotTrans(0, -0x4000, 0, &spD8, 0.0f, 0.0f, 0.0f);
    Menu_GetCubeMtx();
    PSMTXConcat(&spD8, &sp78);
    var_r26 = &lbl_814DDE40 + (0 * 0x78);
    var_r25 = 0;
    var_r27 = &lbl_814DDE40 + 0x618;
loop_10:
    if (var_r25 < 5) {
        PSMTXTrans(&spA8, var_r27->unk0, var_r27->unk4, 0.0f);
        PSMTXConcat(&sp78, &spA8, &sp108);
        var_r4 = lbl_814AD568;
        sp60 = lbl_8137EE58.unk0;
        sp64 = lbl_8137EE58.unk4;
        sp68 = lbl_8137EE58.unk8;
        if (var_r4 > 0x1E) {
            var_r0 = 0x1E;
        } else {
            var_r0 = var_r4;
        }
        sp60 = (0.7f * (f32) var_r0) / 30.0f;
        if (var_r4 > 0x1E) {
            var_r0_2 = 0x1E;
        } else {
            var_r0_2 = var_r4;
        }
        sp64 = (0.7f * (f32) var_r0_2) / 30.0f;
        if (var_r4 > 0x1E) {
            var_r4 = 0x1E;
        }
        sp68 = (0.7f * (f32) var_r4) / 30.0f;
        sp6C = sp60;
        sp70 = sp64;
        sp74 = sp68;
        J3D_SetModelMtx(var_r26, &sp108, &sp6C, sp64);
        J3D_SetViewMtx(var_r26, Camera_GetView());
        J3D_CalcJoints(var_r26);
        var_r27 += 8;
        var_r26 += 0x78;
        var_r25 += 1;
        goto loop_10;
    }
    var_r28 = 0;
    var_r26_2 = &lbl_814DDE40 + 0x640;
    var_r27_2 = &lbl_814DDE40 + (0 * 0x78) + 0x258;
loop_13:
    if (var_r28 < 4) {
        PSMTXTrans(&spA8, var_r26_2->unk0, var_r26_2->unk4, 0.0f);
        PSMTXConcat(&sp78, &spA8, &sp108);
        sp48 = lbl_8137EE58.unkC;
        sp4C = lbl_8137EE58.unk10;
        sp50 = lbl_8137EE58.unk14;
        sp54 = sp48;
        sp58 = sp4C;
        sp5C = lbl_8137EE58.unk14;
        J3D_SetModelMtx(var_r27_2, &sp108, &sp54, sp4C);
        J3D_SetViewMtx(var_r27_2, Camera_GetView());
        J3D_CalcJoints(var_r27_2);
        var_r26_2 += 8;
        var_r27_2 += 0x78;
        var_r28 += 1;
        goto loop_13;
    }
    var_r28_2 = 0;
    var_r26_3 = &lbl_814DDE40 + 0x660;
    var_r27_3 = &lbl_814DDE40 + (0 * 0x78) + 0x438;
loop_16:
    if (var_r28_2 < 3) {
        PSMTXTrans(&spA8, var_r26_3->unk0, var_r26_3->unk4, 0.0f);
        PSMTXConcat(&sp78, &spA8, &sp108);
        sp30 = lbl_8137EE58.unk18;
        sp34 = lbl_8137EE58.unk1C;
        sp38 = lbl_8137EE58.unk20;
        sp3C = sp30;
        sp40 = sp34;
        sp44 = lbl_8137EE58.unk20;
        J3D_SetModelMtx(var_r27_3, &sp108, &sp3C, sp34);
        J3D_SetViewMtx(var_r27_3, Camera_GetView());
        J3D_CalcJoints(var_r27_3);
        var_r26_3 += 8;
        var_r27_3 += 0x78;
        var_r28_2 += 1;
        goto loop_16;
    }
    PSMTXTrans(&spA8, lbl_814AD560.unk0, lbl_814AD560.unk4, 0.0f);
    PSMTXConcat(&sp78, &spA8, &sp108);
    sp18 = lbl_8137EE58.unk24;
    sp1C = lbl_8137EE58.unk28;
    sp20 = lbl_8137EE58.unk2C;
    if ((s16) lbl_814AD568 > 0x28) {
        var_r0_3 = lbl_814AD568 - 0x28;
    } else {
        var_r0_3 = 0;
    }
    sp18 = (0.55f * (f32) var_r0_3) / 10.0f;
    if ((s16) lbl_814AD568 > 0x28) {
        var_r0_4 = lbl_814AD568 - 0x28;
    } else {
        var_r0_4 = 0;
    }
    sp1C = (0.55f * (f32) var_r0_4) / 10.0f;
    if ((s16) lbl_814AD568 > 0x28) {
        var_r0_5 = lbl_814AD568 - 0x28;
    } else {
        var_r0_5 = 0;
    }
    temp_f1 = 0.55f * (f32) var_r0_5;
    sp20 = temp_f1 / 10.0f;
    sp24 = sp18;
    sp28 = sp1C;
    sp2C = sp20;
    J3D_SetModelMtx(&lbl_814DDE40 + 0x5A0, &sp108, &sp24, sp1C, temp_f1, 0.55f);
    J3D_SetViewMtx(&lbl_814DDE40 + 0x5A0, Camera_GetView());
    J3D_CalcJoints(&lbl_814DDE40 + 0x5A0);
}

/* 0x81326CA4 */
void ClockIcon_Draw(void) {
    s16 sp16;
    s16 sp14;
    s16 sp12;
    s16 sp10;
    s16 spE;
    s16 spC;
    s16 spA;
    s16 sp8;
    void *temp_r3;
    s16 *temp_r29;
    s16 var_r27;
    s16 var_r27_2;
    s16 var_r27_3;
    void *temp_r3_2;
    void *var_r28;
    void *var_r28_2;
    void *var_r28_3;

    temp_r3 = Clock_GetParams(&lbl_814DDE40);
    sp10 = temp_r3->unk8;
    var_r28 = &lbl_814DDE40 + (0 * 0x78);
    sp12 = temp_r3->unkA;
    var_r27 = 0;
    sp14 = temp_r3->unkC;
    sp16 = temp_r3->unkE;
    sp8 = temp_r3->unk0;
    spA = temp_r3->unk2;
    spC = temp_r3->unk4;
    spE = temp_r3->unk6;
    temp_r3_2 = lbl_814DDE40.unk68->unk10;
    temp_r29 = temp_r3_2->unk30;
    temp_r3_2->unk30 = &sp10;
loop_2:
    if (var_r27 < 5) {
        J3D_Draw(var_r28);
        var_r28 += 0x78;
        var_r27 += 1;
        goto loop_2;
    }
    var_r27_2 = 0;
    lbl_814DDE40.unk68->unk10->unk30 = &sp8;
    var_r28_2 = &lbl_814DDE40 + (0 * 0x78) + 0x258;
loop_5:
    if (var_r27_2 < 4) {
        J3D_Draw(var_r28_2);
        var_r28_2 += 0x78;
        var_r27_2 += 1;
        goto loop_5;
    }
    var_r27_3 = 0;
    var_r28_3 = &lbl_814DDE40 + (0 * 0x78) + 0x438;
loop_8:
    if (var_r27_3 < 3) {
        J3D_Draw(var_r28_3);
        var_r28_3 += 0x78;
        var_r27_3 += 1;
        goto loop_8;
    }
    J3D_Draw(&lbl_814DDE40 + 0x5A0);
    lbl_814DDE40.unk68->unk10->unk30 = temp_r29;
}

/* 0x81326DE0 */
void ClockIcon_UpdateHide(void) {
    s32 var_r6;
    void *temp_r5;
    void *temp_r5_2;
    void *temp_r5_3;
    void *temp_r5_4;
    void *temp_r5_5;
    void *temp_r5_6;
    void *temp_r5_7;
    void *temp_r5_8;

    var_r6 = 0;
    lbl_814DDE40.unk74 = (s16) (lbl_814DDE40.unk74 - 0xA);
    if ((s16) lbl_814DDE40.unk74 < 0) {
        lbl_814DDE40.unk74 = 0;
        var_r6 = 1;
    }
    temp_r5 = &lbl_814DDE40 + 0x78;
    lbl_814DDE40.unkEC = (s16) (lbl_814DDE40.unkEC - 0xA);
    if ((s16) lbl_814DDE40.unkEC < 0) {
        temp_r5->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5->unkEC = (s16) (temp_r5->unkEC - 0xA);
    temp_r5_2 = temp_r5 + 0x78;
    if ((s16) temp_r5->unkEC < 0) {
        temp_r5_2->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_2->unkEC = (s16) (temp_r5_2->unkEC - 0xA);
    temp_r5_3 = temp_r5_2 + 0x78;
    if ((s16) temp_r5_2->unkEC < 0) {
        temp_r5_3->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_3->unkEC = (s16) (temp_r5_3->unkEC - 0xA);
    if ((s16) temp_r5_3->unkEC < 0) {
        (temp_r5_3 + 0x78)->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_4 = &lbl_814DDE40 + 0x258;
    lbl_814DDE40.unk2CC = (s16) (lbl_814DDE40.unk2CC - 0xA);
    if ((s16) lbl_814DDE40.unk2CC < 0) {
        temp_r5_4->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_4->unkEC = (s16) (temp_r5_4->unkEC - 0xA);
    temp_r5_5 = temp_r5_4 + 0x78;
    if ((s16) temp_r5_4->unkEC < 0) {
        temp_r5_5->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_5->unkEC = (s16) (temp_r5_5->unkEC - 0xA);
    temp_r5_6 = temp_r5_5 + 0x78;
    if ((s16) temp_r5_5->unkEC < 0) {
        temp_r5_6->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_6->unkEC = (s16) (temp_r5_6->unkEC - 0xA);
    if ((s16) temp_r5_6->unkEC < 0) {
        (temp_r5_6 + 0x78)->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_7 = &lbl_814DDE40 + 0x438;
    lbl_814DDE40.unk4AC = (s16) (lbl_814DDE40.unk4AC - 0xA);
    if ((s16) lbl_814DDE40.unk4AC < 0) {
        temp_r5_7->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_7->unkEC = (s16) (temp_r5_7->unkEC - 0xA);
    temp_r5_8 = temp_r5_7 + 0x78;
    if ((s16) temp_r5_7->unkEC < 0) {
        temp_r5_8->unk74 = 0;
        var_r6 += 1;
    }
    temp_r5_8->unkEC = (s16) (temp_r5_8->unkEC - 0xA);
    if ((s16) temp_r5_8->unkEC < 0) {
        (temp_r5_8 + 0x78)->unk74 = 0;
        var_r6 += 1;
    }
    lbl_814DDE40.unk614 = (s16) (lbl_814DDE40.unk614 - 0xA);
    if ((s16) lbl_814DDE40.unk614 < 0) {
        lbl_814DDE40.unk614 = 0;
        var_r6 += 1;
    }
    if (var_r6 == 0xD) {
        lbl_814AD568 = 0;
    }
}

/* 0x81326FDC */
void ClockIcon_UpdateEnter(void) {
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;
    void *temp_r3_4;
    void *temp_r3_5;
    void *temp_r3_6;
    void *temp_r3_9;
    void *temp_ret;
    void *var_r30;
    void *var_r3;
    f32 var_f29;
    f32 var_f29_2;
    f32 var_f29_3;
    f32 var_f29_4;
    f32 var_f29_5;
    f32 var_f29_6;
    f32 var_f29_7;
    f32 var_f29_8;
    f64 var_f1;
    s16 temp_r25;
    s16 temp_r25_2;
    s16 temp_r25_3;
    s16 temp_r25_4;
    s16 temp_r3_7;
    s16 temp_r3_8;
    s16 var_r0;
    s16 var_r0_2;
    s16 var_r0_3;
    s16 var_r0_4;
    s16 var_r0_5;
    s16 var_r0_6;
    s16 var_r0_7;
    s16 var_r0_8;
    s16 var_r25;
    s16 var_r25_2;
    s16 var_r25_3;
    s16 var_r25_4;
    s32 var_r27;
    s32 var_r28;
    s32 var_r28_2;
    s32 var_r28_3;
    void *var_r29;
    void *var_r29_2;
    void *var_r29_3;
    void *var_r30_2;
    void *var_r30_3;

    var_r30 = &lbl_814DDE40;
    var_r29 = &lbl_814DDE40 + 0x618;
    var_r27 = 0;
    var_r28 = 0;
    do {
        temp_r25 = var_r30->unk74;
        var_r25 = temp_r25 - Clock_GetParams()->unk24;
        if (var_r25 < 0) {
            var_r25 = 0;
        }
        var_r30->unk74 = var_r25;
        var_f29 = var_r29->unk0;
        temp_r3 = Clock_GetParams();
        if (var_f29 > 0.0f) {
            var_f29 -= (f32) temp_r3->unk22;
            if (var_f29 < 0.0f) {
                var_f29 = 0.0f;
            }
        } else if (var_f29 < 0.0f) {
            var_f29 += (f32) temp_r3->unk22;
            if (var_f29 > 0.0f) {
                var_f29 = 0.0f;
            }
        }
        var_r29->unk0 = var_f29;
        if (var_f29 == 0.0f) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        var_f29_2 = var_r29->unk4;
        temp_r3_2 = Clock_GetParams();
        if (var_f29_2 > 0.0f) {
            var_f29_2 -= (f32) temp_r3_2->unk22;
            if (var_f29_2 < 0.0f) {
                var_f29_2 = 0.0f;
            }
        } else if (var_f29_2 < 0.0f) {
            var_f29_2 += (f32) temp_r3_2->unk22;
            if (var_f29_2 > 0.0f) {
                var_f29_2 = 0.0f;
            }
        }
        var_r29->unk4 = var_f29_2;
        if (var_f29_2 == 0.0f) {
            var_r0_2 = 1;
        } else {
            var_r0_2 = 0;
        }
        var_r28 += 1;
        var_r27 = var_r27 + var_r0 + var_r0_2;
        var_r30 += 0x78;
        var_r29 += 8;
    } while (var_r28 < 5);
    var_r29_2 = &lbl_814DDE40 + 0x258;
    var_r30_2 = &lbl_814DDE40 + 0x640;
    var_r28_2 = 0;
    do {
        temp_r25_2 = var_r29_2->unk74;
        var_r25_2 = temp_r25_2 - Clock_GetParams()->unk24;
        if (var_r25_2 < 0) {
            var_r25_2 = 0;
        }
        var_r29_2->unk74 = var_r25_2;
        var_f29_3 = var_r30_2->unk0;
        temp_r3_3 = Clock_GetParams();
        if (var_f29_3 > 0.0f) {
            var_f29_3 -= (f32) temp_r3_3->unk22;
            if (var_f29_3 < 0.0f) {
                var_f29_3 = 0.0f;
            }
        } else if (var_f29_3 < 0.0f) {
            var_f29_3 += (f32) temp_r3_3->unk22;
            if (var_f29_3 > 0.0f) {
                var_f29_3 = 0.0f;
            }
        }
        var_r30_2->unk0 = var_f29_3;
        if (var_f29_3 == 0.0f) {
            var_r0_3 = 1;
        } else {
            var_r0_3 = 0;
        }
        var_f29_4 = var_r30_2->unk4;
        temp_r3_4 = Clock_GetParams();
        if (var_f29_4 > 0.0f) {
            var_f29_4 -= (f32) temp_r3_4->unk22;
            if (var_f29_4 < 0.0f) {
                var_f29_4 = 0.0f;
            }
        } else if (var_f29_4 < 0.0f) {
            var_f29_4 += (f32) temp_r3_4->unk22;
            if (var_f29_4 > 0.0f) {
                var_f29_4 = 0.0f;
            }
        }
        var_r30_2->unk4 = var_f29_4;
        if (var_f29_4 == 0.0f) {
            var_r0_4 = 1;
        } else {
            var_r0_4 = 0;
        }
        var_r28_2 += 1;
        var_r27 = var_r27 + var_r0_3 + var_r0_4;
        var_r29_2 += 0x78;
        var_r30_2 += 8;
    } while (var_r28_2 < 4);
    var_r29_3 = &lbl_814DDE40 + 0x438;
    var_r30_3 = &lbl_814DDE40 + 0x660;
    var_r28_3 = 0;
    do {
        temp_r25_3 = var_r29_3->unk74;
        var_r25_3 = temp_r25_3 - Clock_GetParams()->unk24;
        if (var_r25_3 < 0) {
            var_r25_3 = 0;
        }
        var_r29_3->unk74 = var_r25_3;
        var_f29_5 = var_r30_3->unk0;
        temp_r3_5 = Clock_GetParams();
        if (var_f29_5 > 0.0f) {
            var_f29_5 -= (f32) temp_r3_5->unk22;
            if (var_f29_5 < 0.0f) {
                var_f29_5 = 0.0f;
            }
        } else if (var_f29_5 < 0.0f) {
            var_f29_5 += (f32) temp_r3_5->unk22;
            if (var_f29_5 > 0.0f) {
                var_f29_5 = 0.0f;
            }
        }
        var_r30_3->unk0 = var_f29_5;
        if (var_f29_5 == 0.0f) {
            var_r0_5 = 1;
        } else {
            var_r0_5 = 0;
        }
        var_f29_6 = var_r30_3->unk4;
        temp_r3_6 = Clock_GetParams();
        if (var_f29_6 > 0.0f) {
            var_f29_6 -= (f32) temp_r3_6->unk22;
            if (var_f29_6 < 0.0f) {
                var_f29_6 = 0.0f;
            }
        } else if (var_f29_6 < 0.0f) {
            var_f29_6 += (f32) temp_r3_6->unk22;
            if (var_f29_6 > 0.0f) {
                var_f29_6 = 0.0f;
            }
        }
        var_r30_3->unk4 = var_f29_6;
        if (var_f29_6 == 0.0f) {
            var_r0_6 = 1;
        } else {
            var_r0_6 = 0;
        }
        var_r28_3 += 1;
        var_r27 = var_r27 + var_r0_5 + var_r0_6;
        var_r29_3 += 0x78;
        var_r30_3 += 8;
    } while (var_r28_3 < 3);
    temp_r25_4 = lbl_814DDE40.unk614;
    var_r25_4 = temp_r25_4 - Clock_GetParams()->unk24;
    if (var_r25_4 < 0) {
        var_r25_4 = 0;
    }
    lbl_814DDE40.unk614 = var_r25_4;
    var_f29_7 = lbl_814AD560.unk0;
    temp_ret = Clock_GetParams();
    var_f1 = (bitwise f64) temp_ret;
    var_r3 = temp_ret;
    if (var_f29_7 > 0.0f) {
        temp_r3_7 = var_r3->unk22;
        var_f1 = 4503601774854144.0;
        var_r3 = (void *) (temp_r3_7 ^ 0x80000000);
        var_f29_7 -= (f32) temp_r3_7;
        if (var_f29_7 < 0.0f) {
            var_f29_7 = 0.0f;
        }
    } else if (var_f29_7 < 0.0f) {
        temp_r3_8 = var_r3->unk22;
        var_f1 = 4503601774854144.0;
        var_r3 = (void *) (temp_r3_8 ^ 0x80000000);
        var_f29_7 += (f32) temp_r3_8;
        if (var_f29_7 > 0.0f) {
            var_f29_7 = 0.0f;
        }
    }
    lbl_814AD560.unk0 = var_f29_7;
    if (var_f29_7 == 0.0f) {
        var_r0_7 = 1;
    } else {
        var_r0_7 = 0;
    }
    var_f29_8 = lbl_814AD560.unk4;
    temp_r3_9 = Clock_GetParams(var_r3, var_f1);
    if (var_f29_8 > 0.0f) {
        var_f29_8 -= (f32) temp_r3_9->unk22;
        if (var_f29_8 < 0.0f) {
            var_f29_8 = 0.0f;
        }
    } else if (var_f29_8 < 0.0f) {
        var_f29_8 += (f32) temp_r3_9->unk22;
        if (var_f29_8 > 0.0f) {
            var_f29_8 = 0.0f;
        }
    }
    lbl_814AD560.unk4 = var_f29_8;
    if (var_f29_8 == 0.0f) {
        var_r0_8 = 1;
    } else {
        var_r0_8 = 0;
    }
    if ((s32) (var_r27 + var_r0_7 + var_r0_8) == 0x1A) {
        lbl_814AD568 = 0;
        sClockIconSettled = 1;
        return;
    }
    sClockIconSettled = 0;
}

/* 0x813275C4 */
s32 ClockIcon_IsSettled(void) {
    return sClockIconSettled;
}
