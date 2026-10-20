// Non-matching: functionally equivalent decompilation
//
// Submenu icon cubes, 0x813230F0..0x81323C30: Game Play icon (ring of 16 cubes).
// Each icon has Init / UpdateFace (shown on the cube face) / UpdateEnter
// (zooming into the submenu) / UpdateHide / CalcMtx / Draw.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x813230F0 */
void GameIcon_Init(s32 arg0, s32 arg1) {
    s16 var_r12;
    s16 var_r29;
    void *var_r11;
    void *var_r30;

    if (arg1 != 0) {
        lbl_814DB3E8.unkA0 = arg0;
        J3D_InitModel(&lbl_814DB3E8 + 0x38, 0);
        var_r29 = 1;
        var_r30 = &lbl_814DB3E8 + (1 * 0x78) + 0x38;
loop_3:
        if (var_r29 < 0x10) {
            J3D_CopyInstance(var_r30, &lbl_814DB3E8 + 0x38);
            var_r30 += 0x78;
            var_r29 += 1;
            goto loop_3;
        }
    }
    var_r12 = 0;
    var_r11 = &lbl_814DB3E8 + 0x38;
loop_6:
    if (var_r12 < 0x10) {
        var_r11->unk74 = 0;
        var_r12 += 8;
        var_r11->unkEC = 0;
        var_r11->unk164 = 0;
        var_r11->unk1DC = 0;
        var_r11->unk254 = 0;
        var_r11->unk2CC = 0;
        var_r11->unk344 = 0;
        var_r11->unk3BC = 0;
        var_r11 += 0x3C0;
        goto loop_6;
    }
    lbl_814AD0CA = 0x32;
    lbl_814AD0C8 = 0x64;
    lbl_814AD538 = 0;
    GameIcon_InitRing(0x64);
}

/* 0x813231F8 */
void GameIcon_InitRing(void) {
    void *var_r4;
    s16 temp_r10;
    s16 temp_r10_2;
    s16 temp_r10_3;
    s16 temp_r10_4;
    s16 temp_r11;
    s16 temp_r11_2;
    s16 temp_r11_3;
    s16 temp_r11_4;
    s16 temp_r11_5;
    s16 temp_r12;
    s16 temp_r31;
    s16 temp_r9;
    s16 temp_r9_2;
    s16 temp_r9_3;
    s16 var_r3;
    s16 var_r5;
    s16 var_r7;
    s32 var_r6;

    var_r4 = &lbl_814DBBA0;
    var_r3 = 0;
    var_r6 = 0;
    var_r7 = 0;
    var_r5 = 0x14;
loop_2:
    if (var_r7 < 0x10) {
        temp_r10 = var_r7 + 1;
        var_r4->unk0 = var_r3;
        var_r4->unk2 = var_r5;
        var_r4->unk4 = (s16) (var_r6 / 16);
        temp_r9 = temp_r10 * 0xA;
        temp_r11 = var_r7 + 2;
        var_r4->unk14 = temp_r9;
        var_r4->unk16 = (s16) (temp_r9 + 0x14);
        var_r4->unk18 = (s16) ((s32) (temp_r10 << 0x10) / 16);
        temp_r10_2 = temp_r11 * 0xA;
        temp_r11_2 = var_r7 + 3;
        var_r4->unk28 = temp_r10_2;
        var_r4->unk2A = (s16) (temp_r10_2 + 0x14);
        var_r4->unk2C = (s16) ((s32) (temp_r11 << 0x10) / 16);
        temp_r9_2 = temp_r11_2 * 0xA;
        temp_r11_3 = var_r7 + 4;
        var_r4->unk3C = temp_r9_2;
        var_r4->unk3E = (s16) (temp_r9_2 + 0x14);
        var_r4->unk40 = (s16) ((s32) (temp_r11_2 << 0x10) / 16);
        temp_r10_3 = temp_r11_3 * 0xA;
        temp_r11_4 = var_r7 + 5;
        var_r4->unk50 = temp_r10_3;
        var_r4->unk52 = (s16) (temp_r10_3 + 0x14);
        var_r4->unk54 = (s16) ((s32) (temp_r11_3 << 0x10) / 16);
        temp_r9_3 = temp_r11_4 * 0xA;
        temp_r11_5 = var_r7 + 6;
        var_r4->unk64 = temp_r9_3;
        temp_r31 = var_r7 + 7;
        var_r4->unk66 = (s16) (temp_r9_3 + 0x14);
        var_r4->unk68 = (s16) ((s32) (temp_r11_4 << 0x10) / 16);
        temp_r10_4 = temp_r11_5 * 0xA;
        var_r4->unk78 = temp_r10_4;
        temp_r12 = temp_r31 * 0xA;
        var_r4->unk7A = (s16) (temp_r10_4 + 0x14);
        var_r4->unk7C = (s16) ((s32) (temp_r11_5 << 0x10) / 16);
        var_r4->unk8C = temp_r12;
        var_r4->unk8E = (s16) (temp_r12 + 0x14);
        var_r4->unk90 = (s16) ((s32) (temp_r31 << 0x10) / 16);
        var_r6 += 0x80000;
        var_r7 += 8;
        var_r3 += 0x50;
        var_r4 += 0xA0;
        var_r5 += 0x50;
        goto loop_2;
    }
    lbl_814AD53A = 0;
    lbl_814AD53C = 0;
    lbl_814AD53E = 0;
}

/* 0x813233C8 */
void GameIcon_UpdateFace(void) {
    void *var_r29;
    void *var_r30;
    s16 temp_r4;
    s16 temp_r5;
    s16 var_r28;
    s32 temp_r5_2;
    void *temp_r31;

    var_r30 = &lbl_814DBBA0;
    var_r29 = &lbl_814DB420;
    temp_r31 = Game_GetParams();
    var_r28 = 0;
loop_9:
    if (var_r28 < 0x10) {
        temp_r4 = var_r30->unk0;
        temp_r5 = var_r30->unk2;
        if (((s16) lbl_814AD538 > temp_r4) && ((s16) lbl_814AD538 < temp_r5)) {
            temp_r5_2 = temp_r5 - temp_r4;
            var_r30->unk6 = (s16) ((s32) (temp_r31->unk10 * (lbl_814AD538 - temp_r4)) / temp_r5_2);
            var_r29->unk74 = (s16) ((s32) ((lbl_814AD538 - temp_r4) * 0xFF) / temp_r5_2);
        } else if ((s16) lbl_814AD538 <= temp_r4) {
            var_r30->unk6 = 0;
            var_r29->unk74 = 0;
        } else if ((s16) lbl_814AD538 >= temp_r5) {
            var_r30->unk6 = (s16) temp_r31->unk10;
            var_r29->unk74 = 0xFF;
        }
        var_r30->unkC = (f32) ((f32) var_r30->unk6 * SinS(var_r30->unk4 - lbl_814AD53E));
        var_r29 += 0x78;
        var_r28 += 1;
        var_r30->unk10 = (f32) ((f32) var_r30->unk6 * CosS(var_r30->unk4 - lbl_814AD53E));
        var_r30 += 0x14;
        goto loop_9;
    }
    lbl_814AD538 += 1;
    lbl_814AD53E += 0x199;
    if ((s16) lbl_814AD538 > 0x12C) {
        lbl_814AD53A += lbl_814AD0C8;
        lbl_814AD538 = 0x12C;
        lbl_814AD53C += lbl_814AD0CA;
        if ((s16) lbl_814AD53A > 0x7D00) {
            lbl_814AD0C8 = -0x64;
        }
        if ((s16) lbl_814AD53A < 0x3E8) {
            lbl_814AD0C8 = 0x64;
        }
        if ((s16) lbl_814AD53C > 0x2000) {
            lbl_814AD0CA = -0x32;
        }
        if ((s16) lbl_814AD53C < -0x2000) {
            lbl_814AD0CA = 0x32;
        }
        if ((s16) lbl_814AD53E > 0x1000) {
            lbl_814AD53E %= 0x1000;
        }
    }
}

/* 0x813235D4 */
void GameIcon_CalcMtx(void) {
    u8 sp54[0x4];
    u8 sp24[0x30];
    f32 sp20;
    f32 sp1C;
    f32 sp18;
    f32 sp14;
    f32 sp10;
    f32 spC;
    f32 temp_f0;
    s16 var_r26;
    void *temp_r30;
    void *var_r27;
    void *var_r28;

    var_r28 = &lbl_814DBBA0 + (0 * 0x14);
    var_r27 = &lbl_814DB420 + (0 * 0x78);
    temp_r30 = Game_GetParams();
    var_r26 = 0;
loop_2:
    if (var_r26 < 0x10) {
        PSMTXTrans(&sp54, var_r28->unkC, var_r28->unk10, 0.0f);
        MtxRotTrans(lbl_814AD53A, 0, lbl_814AD53C, &sp24, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(&sp24, &sp54, &sp54);
        MtxRotTrans(-0x4000, 0, 0, &sp24, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(&sp24, &sp54, &sp54);
        Menu_GetCubeMtx();
        PSMTXConcat(&sp54, &sp54);
        spC = lbl_8137EE30.unk0;
        sp10 = lbl_8137EE30.unk4;
        sp14 = lbl_8137EE30.unk8;
        temp_f0 = temp_r30->unk14;
        spC = temp_f0;
        sp10 = temp_f0;
        sp14 = temp_f0;
        sp18 = spC;
        sp1C = sp10;
        sp20 = sp14;
        J3D_SetModelMtx(var_r27, &sp54, &sp18, sp10);
        J3D_SetViewMtx(var_r27, Camera_GetView());
        J3D_CalcJoints(var_r27);
        var_r28 += 0x14;
        var_r27 += 0x78;
        var_r26 += 1;
        goto loop_2;
    }
}

/* 0x8132373C */
void GameIcon_Draw(void) {
    void *temp_r29;
    s16 var_r26;
    s16 var_r26_2;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;
    void *var_r28;
    void *var_r31;

    temp_r3 = Game_GetParams(&lbl_814DB3E8);
    lbl_814AD540.unk0 = (s16) temp_r3->unk0;
    lbl_814AD540.unk2 = (s16) temp_r3->unk4;
    lbl_814AD540.unk4 = (s16) temp_r3->unk8;
    lbl_814AD540.unk6 = (s16) temp_r3->unkC;
    temp_r3_2 = lbl_814DB3E8.unkA0->unk10;
    temp_r29 = temp_r3_2->unk30;
    temp_r3_2->unk30 = &lbl_814AD540;
    if ((s16) lbl_814AD538 < 0x12C) {
        var_r31 = &lbl_814DB3E8 + 0x38;
        var_r26 = 0;
loop_5:
        if (var_r26 < 0x10) {
            if ((s16) var_r31->unk74 > 0) {
                J3D_Draw(var_r31);
            }
            var_r31 += 0x78;
            var_r26 += 1;
            goto loop_5;
        }
    } else {
        var_r28 = &lbl_814DB3E8 + 0x38;
        var_r26_2 = 0;
loop_13:
        if (var_r26_2 < 8) {
            temp_r3_3 = &lbl_814DB3E8 + ((7 - var_r26_2) * 0x78);
            if ((s16) temp_r3_3->unkAC > 0) {
                J3D_Draw(temp_r3_3 + 0x38);
            }
            if ((s16) var_r28->unk434 > 0) {
                J3D_Draw(&lbl_814DB3E8 + ((var_r26_2 + 8) * 0x78) + 0x38);
            }
            var_r28 += 0x78;
            var_r26_2 += 1;
            goto loop_13;
        }
    }
    lbl_814DB3E8.unkA0->unk10->unk30 = temp_r29;
}

/* 0x81323868 */
void GameIcon_UpdateHide(void) {
    void *var_r4;
    s32 var_ctr;
    s32 var_r6;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r4_3;
    void *temp_r4_4;
    void *temp_r4_5;
    void *temp_r4_6;
    void *temp_r4_7;

    var_ctr = 2;
    var_r4 = &lbl_814DB420;
    var_r6 = 0;
    do {
        var_r4->unk74 = (s16) (var_r4->unk74 - 0xA);
        if ((s16) var_r4->unk74 < 0) {
            var_r4->unk74 = 0;
            var_r6 += 1;
        }
        var_r4->unkEC = (s16) (var_r4->unkEC - 0xA);
        temp_r4 = var_r4 + 0x78;
        if ((s16) var_r4->unkEC < 0) {
            temp_r4->unk74 = 0;
            var_r6 += 1;
        }
        temp_r4->unkEC = (s16) (temp_r4->unkEC - 0xA);
        temp_r4_2 = temp_r4 + 0x78;
        if ((s16) temp_r4->unkEC < 0) {
            temp_r4_2->unk74 = 0;
            var_r6 += 1;
        }
        temp_r4_2->unkEC = (s16) (temp_r4_2->unkEC - 0xA);
        temp_r4_3 = temp_r4_2 + 0x78;
        if ((s16) temp_r4_2->unkEC < 0) {
            temp_r4_3->unk74 = 0;
            var_r6 += 1;
        }
        temp_r4_3->unkEC = (s16) (temp_r4_3->unkEC - 0xA);
        temp_r4_4 = temp_r4_3 + 0x78;
        if ((s16) temp_r4_3->unkEC < 0) {
            temp_r4_4->unk74 = 0;
            var_r6 += 1;
        }
        temp_r4_4->unkEC = (s16) (temp_r4_4->unkEC - 0xA);
        temp_r4_5 = temp_r4_4 + 0x78;
        if ((s16) temp_r4_4->unkEC < 0) {
            temp_r4_5->unk74 = 0;
            var_r6 += 1;
        }
        temp_r4_5->unkEC = (s16) (temp_r4_5->unkEC - 0xA);
        temp_r4_6 = temp_r4_5 + 0x78;
        if ((s16) temp_r4_5->unkEC < 0) {
            temp_r4_6->unk74 = 0;
            var_r6 += 1;
        }
        temp_r4_6->unkEC = (s16) (temp_r4_6->unkEC - 0xA);
        temp_r4_7 = temp_r4_6 + 0x78;
        if ((s16) temp_r4_6->unkEC < 0) {
            temp_r4_7->unk74 = 0;
            var_r6 += 1;
        }
        var_r4 = temp_r4_7 + 0x78;
        var_ctr -= 1;
    } while (var_ctr != 0);
    lbl_814AD538 = 0;
    if (var_r6 == 0x10) {
        lbl_814AD53A = 0;
        lbl_814AD53C = 0;
        lbl_814AD53E = 0;
    }
}

/* 0x813239E4 */
void GameIcon_UpdateEnter(void) {
    void *var_r30;
    void *var_r31;
    f32 var_f29;
    f32 var_f29_2;
    s16 temp_r26;
    s16 var_r26;
    s16 var_r28;
    s16 var_r29;
    s32 var_r0;
    s32 var_r0_2;
    void *temp_r3;
    void *temp_r3_2;

    var_r31 = &lbl_814DB420;
    var_r30 = &lbl_814DBBA0;
    var_r29 = 0;
    var_r28 = 0;
loop_22:
    if (var_r28 < 0x10) {
        temp_r26 = var_r31->unk74;
        var_r26 = temp_r26 - Game_GetParams()->unk1C;
        if (var_r26 < 0) {
            var_r26 = 0;
        }
        var_r31->unk74 = var_r26;
        var_f29 = var_r30->unkC;
        temp_r3 = Game_GetParams();
        if (var_f29 > 0.0f) {
            var_f29 -= (f32) temp_r3->unk1A;
            if (var_f29 < 0.0f) {
                var_f29 = 0.0f;
            }
        } else if (var_f29 < 0.0f) {
            var_f29 += (f32) temp_r3->unk1A;
            if (var_f29 > 0.0f) {
                var_f29 = 0.0f;
            }
        }
        var_r30->unkC = var_f29;
        if (var_f29 == 0.0f) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        var_f29_2 = var_r30->unk10;
        temp_r3_2 = Game_GetParams();
        if (var_f29_2 > 0.0f) {
            var_f29_2 -= (f32) temp_r3_2->unk1A;
            if (var_f29_2 < 0.0f) {
                var_f29_2 = 0.0f;
            }
        } else if (var_f29_2 < 0.0f) {
            var_f29_2 += (f32) temp_r3_2->unk1A;
            if (var_f29_2 > 0.0f) {
                var_f29_2 = 0.0f;
            }
        }
        var_r30->unk10 = var_f29_2;
        if (var_f29_2 == 0.0f) {
            var_r0_2 = 1;
        } else {
            var_r0_2 = 0;
        }
        var_r29 = var_r29 + var_r0 + var_r0_2;
        var_r31 += 0x78;
        var_r30 += 0x14;
        var_r28 += 1;
        goto loop_22;
    }
    if (var_r29 == 0x20) {
        lbl_814AD53A = 0;
        lbl_814AD548 = 1;
        lbl_814AD53C = 0;
        lbl_814AD53E = 0;
    } else {
        lbl_814AD548 = 0;
    }
    lbl_814AD538 = 0;
}

/* 0x81323BC0 */
s32 GameIcon_IsSettled(void) {
    void *var_r3;
    s16 var_r4;
    s16 var_r5;

    var_r3 = &lbl_814DBBA0;
    var_r4 = 0;
    var_r5 = 0;
loop_5:
    if (var_r5 < 0x10) {
        if ((var_r3->unkC == 0.0f) && (var_r3->unk10 == 0.0f)) {
            var_r4 += 1;
        }
        var_r3 += 0x14;
        var_r5 += 1;
        goto loop_5;
    }
    if (var_r4 == 0x10) {
        lbl_814AD548 = 1;
    } else {
        lbl_814AD548 = 0;
    }
    return lbl_814AD548;
}
