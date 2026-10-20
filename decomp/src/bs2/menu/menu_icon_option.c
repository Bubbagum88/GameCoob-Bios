// Non-matching: functionally equivalent decompilation
//
// Submenu icon cubes, 0x81323C30..0x81326114: Options icon (cubes forming animated shapes).
// Each icon has Init / UpdateFace (shown on the cube face) / UpdateEnter
// (zooming into the submenu) / UpdateHide / CalcMtx / Draw.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x81323C30 */
void OptionIcon_Init(s32 arg0, s32 arg1) {
    void *var_r12;
    void *var_r30;
    void *var_r30_2;
    s16 var_r29;
    s16 var_r29_2;
    s16 var_r30_3;
    s16 var_r30_4;
    void *var_r12_2;
    void *var_r4;
    void *var_r4_2;

    if (arg1 != 0) {
        var_r30 = &lbl_814DBCE0;
        var_r29 = 0;
loop_3:
        if (var_r29 < 0x1E) {
            var_r30->unk68 = arg0;
            J3D_InitModel(var_r30, 0);
            var_r30 += 0x78;
            var_r29 += 1;
            goto loop_3;
        }
        var_r30_2 = &lbl_814DBCE0 + 0xE10;
        var_r29_2 = 0;
loop_6:
        if (var_r29_2 < 0x1F) {
            var_r30_2->unk68 = arg0;
            J3D_InitModel(var_r30_2, 0);
            var_r30_2 += 0x78;
            var_r29_2 += 1;
            goto loop_6;
        }
    }
    var_r30_3 = 0;
    var_r12 = &lbl_814DBCE0;
loop_9:
    if (var_r30_3 < 0x16) {
        var_r12->unk74 = 0;
        var_r30_3 += 8;
        var_r12->unkEC = 0;
        var_r12->unk164 = 0;
        var_r12->unk1DC = 0;
        var_r12->unk254 = 0;
        var_r12->unk2CC = 0;
        var_r12->unk344 = 0;
        var_r12->unk3BC = 0;
        var_r12 += 0x3C0;
        goto loop_9;
    }
    var_r4 = &lbl_814DBCE0 + (var_r30_3 * 0x78);
loop_13:
    if (var_r30_3 < 0x1E) {
        var_r4->unk74 = 0;
        var_r4 += 0x78;
        var_r30_3 += 1;
        goto loop_13;
    }
    var_r30_4 = 0;
    var_r12_2 = &lbl_814DBCE0 + 0xE10;
loop_16:
    if (var_r30_4 < 0x17) {
        var_r12_2->unk74 = 0;
        var_r30_4 += 8;
        var_r12_2->unkEC = 0;
        var_r12_2->unk164 = 0;
        var_r12_2->unk1DC = 0;
        var_r12_2->unk254 = 0;
        var_r12_2->unk2CC = 0;
        var_r12_2->unk344 = 0;
        var_r12_2->unk3BC = 0;
        var_r12_2 += 0x3C0;
        goto loop_16;
    }
    var_r4_2 = &lbl_814DBCE0 + (var_r30_4 * 0x78) + 0xE10;
loop_20:
    if (var_r30_4 < 0x1F) {
        var_r4_2->unk74 = 0;
        var_r4_2 += 0x78;
        var_r30_4 += 1;
        goto loop_20;
    }
    lbl_814AD550 = 0;
    lbl_814AD55C = 0;
    OptionIcon_ResetShape();
}

/* 0x81323E20 */
void OptionIcon_ResetShape(void) {
    void *var_r9;
    s16 var_r10;
    s16 var_r5_2;
    s16 var_r6;
    s32 var_r7;
    u8 temp_r7;
    void *var_r3;
    void *var_r4;
    void *var_r5;

    var_r9 = &lbl_814DBCE0;
    var_r5 = &lbl_814DBCE0 + 0x1C98;
    var_r6 = 0;
    var_r10 = 0;
    var_r7 = 0;
loop_15:
    if (var_r10 < 0x1E) {
        if ((var_r10 >= 1) && (var_r10 <= 6)) {
            var_r5->unk0 = 80.0f;
            var_r5->unk4 = (f32) (0x46 - var_r7);
        } else if ((var_r10 >= 8) && (var_r10 <= 0xE)) {
            var_r5->unk0 = (f32) (((0xF - var_r10) * 0x14) - 0x50);
            var_r5->unk4 = -70.0f;
        } else if ((var_r10 >= 0x10) && (var_r10 <= 0x15)) {
            var_r5->unk0 = -80.0f;
            var_r5->unk4 = (f32) (0x46 - ((0x16 - var_r10) * 0x14));
        } else if ((var_r10 >= 0x17) && (var_r10 <= 0x1D)) {
            var_r5->unk0 = (f32) (((var_r10 - 0x16) * 0x14) - 0x50);
            var_r5->unk4 = 70.0f;
        } else {
            var_r5->unk0 = 0.0f;
            var_r5->unk4 = 0.0f;
        }
        var_r5->unk10 = 0;
        var_r7 += 0x14;
        var_r10 += 1;
        var_r5->unk12 = 0;
        var_r5 += 0x14;
        var_r9->unk74 = 0;
        var_r9 += 0x78;
        goto loop_15;
    }
    var_r3 = &lbl_814DBCE0 + 0x1EF0;
    var_r4 = &lbl_814DBCE0 + 0xE10;
    var_r5_2 = 0;
loop_20:
    if (var_r5_2 < 0x1F) {
loop_17:
        temp_r7 = *(&lbl_81381450 + var_r6);
        var_r6 += 1;
        if ((s8) temp_r7 == 0) {
            if (var_r6 < 0x48) {
                goto loop_17;
            }
        }
        var_r5_2 += 1;
        var_r3->unk0 = (f32) ((((var_r6 - 1) % 9) * 0x14) - 0x50);
        var_r3->unk4 = (f32) (0x46 - (((var_r6 - 1) / 9) * 0x14));
        var_r3 += 0x14;
        var_r4->unk74 = 0;
        var_r4 += 0x78;
        goto loop_20;
    }
}

/* 0x8132406C */
void fn_8132406C(s16 *arg0, s32 arg1) {
    s16 var_r5;

    var_r5 = *arg0 + arg1;
    if (var_r5 > 0xFF) {
        var_r5 = 0xFF;
    }
    if (var_r5 < 0) {
        var_r5 = 0;
    }
    *arg0 = var_r5;
}

/* 0x81324098 */
void fn_81324098(void) {
    s16 var_r6;
    s16 var_r6_2;
    void *var_r3;
    void *var_r3_2;
    void *var_r5;
    void *var_r5_2;

    var_r5 = &lbl_814DBCE0 + 0x1C98;
    var_r6 = 0;
loop_2:
    if (var_r6 < 0x16) {
        var_r6 += 8;
        var_r5->unk8 = (f32) var_r5->unk0;
        var_r5->unkC = (f32) var_r5->unk4;
        var_r5->unk1C = (f32) var_r5->unk14;
        var_r5->unk20 = (f32) var_r5->unk18;
        var_r5->unk30 = (f32) var_r5->unk28;
        var_r5->unk34 = (f32) var_r5->unk2C;
        var_r5->unk44 = (f32) var_r5->unk3C;
        var_r5->unk48 = (f32) var_r5->unk40;
        var_r5->unk58 = (f32) var_r5->unk50;
        var_r5->unk5C = (f32) var_r5->unk54;
        var_r5->unk6C = (f32) var_r5->unk64;
        var_r5->unk70 = (f32) var_r5->unk68;
        var_r5->unk80 = (f32) var_r5->unk78;
        var_r5->unk84 = (f32) var_r5->unk7C;
        var_r5->unk94 = (f32) var_r5->unk8C;
        var_r5->unk98 = (f32) var_r5->unk90;
        var_r5 += 0xA0;
        goto loop_2;
    }
    var_r3 = &lbl_814DBCE0 + (var_r6 * 0x14) + 0x1C98;
loop_6:
    if (var_r6 < 0x1E) {
        var_r6 += 1;
        var_r3->unk8 = (f32) var_r3->unk0;
        var_r3->unkC = (f32) var_r3->unk4;
        var_r3 += 0x14;
        goto loop_6;
    }
    var_r5_2 = &lbl_814DBCE0 + 0x1EF0;
    var_r6_2 = 0;
loop_9:
    if (var_r6_2 < 0x17) {
        var_r6_2 += 8;
        var_r5_2->unk8 = (f32) var_r5_2->unk0;
        var_r5_2->unkC = (f32) var_r5_2->unk4;
        var_r5_2->unk1C = (f32) var_r5_2->unk14;
        var_r5_2->unk20 = (f32) var_r5_2->unk18;
        var_r5_2->unk30 = (f32) var_r5_2->unk28;
        var_r5_2->unk34 = (f32) var_r5_2->unk2C;
        var_r5_2->unk44 = (f32) var_r5_2->unk3C;
        var_r5_2->unk48 = (f32) var_r5_2->unk40;
        var_r5_2->unk58 = (f32) var_r5_2->unk50;
        var_r5_2->unk5C = (f32) var_r5_2->unk54;
        var_r5_2->unk6C = (f32) var_r5_2->unk64;
        var_r5_2->unk70 = (f32) var_r5_2->unk68;
        var_r5_2->unk80 = (f32) var_r5_2->unk78;
        var_r5_2->unk84 = (f32) var_r5_2->unk7C;
        var_r5_2->unk94 = (f32) var_r5_2->unk8C;
        var_r5_2->unk98 = (f32) var_r5_2->unk90;
        var_r5_2 += 0xA0;
        goto loop_9;
    }
    var_r3_2 = &lbl_814DBCE0 + (var_r6_2 * 0x14) + 0x1EF0;
loop_13:
    if (var_r6_2 < 0x1F) {
        var_r6_2 += 1;
        var_r3_2->unk8 = (f32) var_r3_2->unk0;
        var_r3_2->unkC = (f32) var_r3_2->unk4;
        var_r3_2 += 0x14;
        goto loop_13;
    }
}

/* 0x81324264 */
void fn_81324264(void) {

}

/* 0x81324268 */
void fn_81324268(s16 arg0, s16 arg1) {
    s32 sp18;
    s32 sp14;
    void *var_r6;
    void *var_r6_3;
    f32 temp_f0;
    f32 temp_f0_2;
    s16 *temp_r4_2;
    s16 temp_r0;
    s16 temp_r21;
    s16 temp_r22;
    s16 temp_r4;
    s16 var_r23;
    s16 var_r3;
    s16 var_r3_2;
    s16 var_r3_3;
    s16 var_r4;
    s16 var_r5;
    s16 var_r5_2;
    s16 var_r5_4;
    s32 *var_r26;
    s32 *var_r27;
    s32 temp_r3;
    s32 temp_r6;
    void *temp_r3_2;
    void *var_r3_4;
    void *var_r3_5;
    void *var_r4_2;
    void *var_r5_3;
    void *var_r6_2;
    void *var_r6_4;

    var_r27 = &sp14;
    var_r26 = &sp18;
    var_r23 = 0;
    sp18 = lbl_814ADFF4;
    sp14 = lbl_814ADFF8;
loop_40:
    if (var_r23 < 4) {
        temp_r22 = 0x46 - (((s32) *var_r27 / 2) * 0x8C);
        temp_r0 = ((*var_r27 % 2) * 0xA0) - 0x50;
        temp_r4 = ((var_r23 % 2) * 0xA0) - 0x50;
        temp_r3 = var_r23 / 2;
        temp_r21 = 0x46 - (temp_r3 * 0x8C);
        temp_r6 = *var_r26 * 0x14;
        temp_r3_2 = &lbl_814DBCE0 + temp_r6;
        temp_r3_2->unk1C98 = Hermite(temp_r3, temp_r4, (f32) arg0, 0.0f, (f32) temp_r0, (f32) ((s32) (temp_r4 - temp_r0) / arg1), (f32) arg1, (f32) temp_r4, 0.0f);
        (&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C9C = Hermite((s32) temp_r3_2, temp_r21, (bitwise f32) temp_r6, (f32) arg0, 0.0f, (f32) temp_r22, (f32) ((s32) (temp_r21 - temp_r22) / arg1), (f32) arg1, (f32) temp_r21, 0.0f);
        temp_r4_2 = &lbl_814DBCE0 + ((*var_r26 * 0x78) + 0x74);
        var_r3 = *temp_r4_2 + 6;
        if (var_r3 > 0xFF) {
            var_r3 = 0xFF;
        }
        if (var_r3 < 0) {
            var_r3 = 0;
        }
        *temp_r4_2 = var_r3;
        switch (var_r23) {                          /* irregular */
        case 0:
            var_r5 = 7;
loop_14:
            if (var_r5 > (s16) ((70.0f - (&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C9C) / 20.0f)) {
                *(&lbl_814DBCE0 + (((0x16 - var_r5) * 0x78) + 0x74)) = 0xFF;
                var_r5 -= 1;
                goto loop_14;
            }
            break;
        case 1:
            var_r3_2 = 0;
            temp_f0 = ((&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C98 - -80.0f) / 20.0f;
            if ((s16) temp_f0 > 0) {
                if ((s16) temp_f0 > 8) {
                    var_r6 = &lbl_814DBCE0;
loop_20:
                    if (var_r3_2 < (s16) ((s32) temp_f0 - 8)) {
                        var_r6->unkB3C = 0xFF;
                        var_r3_2 += 8;
                        var_r6->unkBB4 = 0xFF;
                        var_r6->unkC2C = 0xFF;
                        var_r6->unkCA4 = 0xFF;
                        var_r6->unkD1C = 0xFF;
                        var_r6->unkD94 = 0xFF;
                        var_r6->unkE0C = 0xFF;
                        var_r6->unkE84 = 0xFF;
                        var_r6 += 0x3C0;
                        goto loop_20;
                    }
                }
                var_r6_2 = &lbl_814DBCE0 + (var_r3_2 * 0x78);
loop_24:
                if (var_r3_2 < (s16) (((&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C98 - -80.0f) / 20.0f)) {
                    var_r6_2->unkB3C = 0xFF;
                    var_r6_2 += 0x78;
                    var_r3_2 += 1;
                    goto loop_24;
                }
            }
            break;
        case 2:
            var_r5_2 = 8;
loop_28:
            if (var_r5_2 > (s16) (((&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C98 - -80.0f) / 20.0f)) {
                *(&lbl_814DBCE0 + (((0xF - var_r5_2) * 0x78) + 0x74)) = 0xFF;
                var_r5_2 -= 1;
                goto loop_28;
            }
            break;
        case 3:
            var_r3_3 = 0;
            temp_f0_2 = (70.0f - (&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C9C) / 20.0f;
            if ((s16) temp_f0_2 > 0) {
                if ((s16) temp_f0_2 > 8) {
                    var_r6_3 = &lbl_814DBCE0;
loop_34:
                    if (var_r3_3 < (s16) ((s32) temp_f0_2 - 8)) {
                        var_r6_3->unkEC = 0xFF;
                        var_r3_3 += 8;
                        var_r6_3->unk164 = 0xFF;
                        var_r6_3->unk1DC = 0xFF;
                        var_r6_3->unk254 = 0xFF;
                        var_r6_3->unk2CC = 0xFF;
                        var_r6_3->unk344 = 0xFF;
                        var_r6_3->unk3BC = 0xFF;
                        var_r6_3->unk434 = 0xFF;
                        var_r6_3 += 0x3C0;
                        goto loop_34;
                    }
                }
                var_r6_4 = &lbl_814DBCE0 + (var_r3_3 * 0x78);
loop_38:
                if (var_r3_3 < (s16) ((70.0f - (&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C9C) / 20.0f)) {
                    var_r6_4->unkEC = 0xFF;
                    var_r6_4 += 0x78;
                    var_r3_3 += 1;
                    goto loop_38;
                }
            }
            break;
        }
        var_r27 += 1;
        var_r26 += 1;
        var_r23 += 1;
        goto loop_40;
    }
    var_r5_3 = &lbl_814DBCE0 + 0x1C98;
    var_r4 = 0;
loop_43:
    if (var_r4 < 0x16) {
        var_r4 += 8;
        var_r5_3->unk8 = (f32) var_r5_3->unk0;
        var_r5_3->unkC = (f32) var_r5_3->unk4;
        var_r5_3->unk1C = (f32) var_r5_3->unk14;
        var_r5_3->unk20 = (f32) var_r5_3->unk18;
        var_r5_3->unk30 = (f32) var_r5_3->unk28;
        var_r5_3->unk34 = (f32) var_r5_3->unk2C;
        var_r5_3->unk44 = (f32) var_r5_3->unk3C;
        var_r5_3->unk48 = (f32) var_r5_3->unk40;
        var_r5_3->unk58 = (f32) var_r5_3->unk50;
        var_r5_3->unk5C = (f32) var_r5_3->unk54;
        var_r5_3->unk6C = (f32) var_r5_3->unk64;
        var_r5_3->unk70 = (f32) var_r5_3->unk68;
        var_r5_3->unk80 = (f32) var_r5_3->unk78;
        var_r5_3->unk84 = (f32) var_r5_3->unk7C;
        var_r5_3->unk94 = (f32) var_r5_3->unk8C;
        var_r5_3->unk98 = (f32) var_r5_3->unk90;
        var_r5_3 += 0xA0;
        goto loop_43;
    }
    var_r3_4 = &lbl_814DBCE0 + (var_r4 * 0x14) + 0x1C98;
loop_47:
    if (var_r4 < 0x1E) {
        var_r4 += 1;
        var_r3_4->unk8 = (f32) var_r3_4->unk0;
        var_r3_4->unkC = (f32) var_r3_4->unk4;
        var_r3_4 += 0x14;
        goto loop_47;
    }
    var_r4_2 = &lbl_814DBCE0 + 0x1EF0;
    var_r5_4 = 0;
loop_50:
    if (var_r5_4 < 0x17) {
        var_r5_4 += 8;
        var_r4_2->unk8 = (f32) var_r4_2->unk0;
        var_r4_2->unkC = (f32) var_r4_2->unk4;
        var_r4_2->unk1C = (f32) var_r4_2->unk14;
        var_r4_2->unk20 = (f32) var_r4_2->unk18;
        var_r4_2->unk30 = (f32) var_r4_2->unk28;
        var_r4_2->unk34 = (f32) var_r4_2->unk2C;
        var_r4_2->unk44 = (f32) var_r4_2->unk3C;
        var_r4_2->unk48 = (f32) var_r4_2->unk40;
        var_r4_2->unk58 = (f32) var_r4_2->unk50;
        var_r4_2->unk5C = (f32) var_r4_2->unk54;
        var_r4_2->unk6C = (f32) var_r4_2->unk64;
        var_r4_2->unk70 = (f32) var_r4_2->unk68;
        var_r4_2->unk80 = (f32) var_r4_2->unk78;
        var_r4_2->unk84 = (f32) var_r4_2->unk7C;
        var_r4_2->unk94 = (f32) var_r4_2->unk8C;
        var_r4_2->unk98 = (f32) var_r4_2->unk90;
        var_r4_2 += 0xA0;
        goto loop_50;
    }
    var_r3_5 = &lbl_814DBCE0 + (var_r5_4 * 0x14) + 0x1EF0;
loop_54:
    if (var_r5_4 < 0x1F) {
        var_r5_4 += 1;
        var_r3_5->unk8 = (f32) var_r3_5->unk0;
        var_r3_5->unkC = (f32) var_r3_5->unk4;
        var_r3_5 += 0x14;
        goto loop_54;
    }
}

/* 0x81324938 */
void fn_81324938(s16 arg0) {
    s16 var_r5_2;
    s16 var_r6_2;
    s16 var_r9;
    s32 var_r3;
    s32 var_r5;
    void *var_r3_2;
    void *var_r3_3;
    void *var_r5_3;
    void *var_r6;
    void *var_r8;

    var_r8 = &lbl_814DBCE0 + 0x1C98;
    var_r6 = var_r8;
    var_r9 = 0;
    var_r3 = 0;
    var_r5 = 0xA;
loop_11:
    if (var_r9 < 0x1E) {
        if ((arg0 >= var_r3) && (arg0 <= var_r5)) {
            if (var_r9 < 7) {
                var_r6->unk10 = (s16) (((arg0 - var_r3) << 0xE) / 10);
                var_r6->unk12 = 0;
            } else if (var_r9 < 0xF) {
                var_r6->unk10 = 0;
                var_r6->unk12 = (s16) (-((arg0 - var_r3) << 0xE) / 10);
            } else if (var_r9 < 0x16) {
                var_r6->unk10 = (s16) (-((arg0 - var_r3) << 0xE) / 10);
                var_r6->unk12 = 0;
            } else {
                var_r6->unk10 = 0;
                var_r6->unk12 = (s16) (((arg0 - var_r3) << 0xE) / 10);
            }
        }
        var_r3 += 5;
        var_r5 += 5;
        var_r6 += 0x14;
        var_r9 += 1;
        goto loop_11;
    }
    var_r5_2 = 0;
loop_14:
    if (var_r5_2 < 0x16) {
        var_r5_2 += 8;
        var_r8->unk8 = (f32) var_r8->unk0;
        var_r8->unkC = (f32) var_r8->unk4;
        var_r8->unk1C = (f32) var_r8->unk14;
        var_r8->unk20 = (f32) var_r8->unk18;
        var_r8->unk30 = (f32) var_r8->unk28;
        var_r8->unk34 = (f32) var_r8->unk2C;
        var_r8->unk44 = (f32) var_r8->unk3C;
        var_r8->unk48 = (f32) var_r8->unk40;
        var_r8->unk58 = (f32) var_r8->unk50;
        var_r8->unk5C = (f32) var_r8->unk54;
        var_r8->unk6C = (f32) var_r8->unk64;
        var_r8->unk70 = (f32) var_r8->unk68;
        var_r8->unk80 = (f32) var_r8->unk78;
        var_r8->unk84 = (f32) var_r8->unk7C;
        var_r8->unk94 = (f32) var_r8->unk8C;
        var_r8->unk98 = (f32) var_r8->unk90;
        var_r8 += 0xA0;
        goto loop_14;
    }
    var_r3_2 = &lbl_814DBCE0 + (var_r5_2 * 0x14) + 0x1C98;
loop_18:
    if (var_r5_2 < 0x1E) {
        var_r5_2 += 1;
        var_r3_2->unk8 = (f32) var_r3_2->unk0;
        var_r3_2->unkC = (f32) var_r3_2->unk4;
        var_r3_2 += 0x14;
        goto loop_18;
    }
    var_r5_3 = &lbl_814DBCE0 + 0x1EF0;
    var_r6_2 = 0;
loop_21:
    if (var_r6_2 < 0x17) {
        var_r6_2 += 8;
        var_r5_3->unk8 = (f32) var_r5_3->unk0;
        var_r5_3->unkC = (f32) var_r5_3->unk4;
        var_r5_3->unk1C = (f32) var_r5_3->unk14;
        var_r5_3->unk20 = (f32) var_r5_3->unk18;
        var_r5_3->unk30 = (f32) var_r5_3->unk28;
        var_r5_3->unk34 = (f32) var_r5_3->unk2C;
        var_r5_3->unk44 = (f32) var_r5_3->unk3C;
        var_r5_3->unk48 = (f32) var_r5_3->unk40;
        var_r5_3->unk58 = (f32) var_r5_3->unk50;
        var_r5_3->unk5C = (f32) var_r5_3->unk54;
        var_r5_3->unk6C = (f32) var_r5_3->unk64;
        var_r5_3->unk70 = (f32) var_r5_3->unk68;
        var_r5_3->unk80 = (f32) var_r5_3->unk78;
        var_r5_3->unk84 = (f32) var_r5_3->unk7C;
        var_r5_3->unk94 = (f32) var_r5_3->unk8C;
        var_r5_3->unk98 = (f32) var_r5_3->unk90;
        var_r5_3 += 0xA0;
        goto loop_21;
    }
    var_r3_3 = &lbl_814DBCE0 + (var_r6_2 * 0x14) + 0x1EF0;
loop_25:
    if (var_r6_2 < 0x1F) {
        var_r6_2 += 1;
        var_r3_3->unk8 = (f32) var_r3_3->unk0;
        var_r3_3->unkC = (f32) var_r3_3->unk4;
        var_r3_3 += 0x14;
        goto loop_25;
    }
}

/* 0x81324C20 */
void fn_81324C20(void) {

}

/* 0x81324C24 */
void fn_81324C24(s16 arg0, s16 arg1) {
    void *var_r6;
    f32 temp_f3;
    s16 temp_r0;
    s16 temp_r3;
    s16 var_r5;
    s16 var_r6_2;
    s16 var_r7;
    void *var_r4;
    void *var_r4_2;
    void *var_r5_2;
    void *var_r6_3;

    temp_r0 = (s16) Hermite((s32) &lbl_814DBCE0, (bitwise s16) (f32) arg0, 0.0f, 0.0f, 0.0f, (f32) arg1, 80.0f, 0.0f);
    if (arg0 == 0) {
        var_r6 = &lbl_814DBCE0;
        var_r5 = 0;
loop_3:
        if (var_r5 < 0x16) {
            var_r5 += 8;
            var_r6->unk74 = (s16) (var_r6->unk74 ^ 0xFF);
            var_r6->unkEC = (s16) (var_r6->unkEC ^ 0xFF);
            var_r6->unk164 = (s16) (var_r6->unk164 ^ 0xFF);
            var_r6->unk1DC = (s16) (var_r6->unk1DC ^ 0xFF);
            var_r6->unk254 = (s16) (var_r6->unk254 ^ 0xFF);
            var_r6->unk2CC = (s16) (var_r6->unk2CC ^ 0xFF);
            var_r6->unk344 = (s16) (var_r6->unk344 ^ 0xFF);
            var_r6->unk3BC = (s16) (var_r6->unk3BC ^ 0xFF);
            var_r6 += 0x3C0;
            goto loop_3;
        }
        var_r4 = &lbl_814DBCE0 + (var_r5 * 0x78);
loop_7:
        if (var_r5 < 0x1E) {
            var_r5 += 1;
            var_r4->unk74 = (s16) (var_r4->unk74 ^ 0xFF);
            var_r4 += 0x78;
            goto loop_7;
        }
        var_r5_2 = &lbl_814DBCE0 + 0xE10;
        var_r6_2 = 0;
loop_10:
        if (var_r6_2 < 0x17) {
            var_r6_2 += 8;
            var_r5_2->unk74 = (s16) (var_r5_2->unk74 ^ 0xFF);
            var_r5_2->unkEC = (s16) (var_r5_2->unkEC ^ 0xFF);
            var_r5_2->unk164 = (s16) (var_r5_2->unk164 ^ 0xFF);
            var_r5_2->unk1DC = (s16) (var_r5_2->unk1DC ^ 0xFF);
            var_r5_2->unk254 = (s16) (var_r5_2->unk254 ^ 0xFF);
            var_r5_2->unk2CC = (s16) (var_r5_2->unk2CC ^ 0xFF);
            var_r5_2->unk344 = (s16) (var_r5_2->unk344 ^ 0xFF);
            var_r5_2->unk3BC = (s16) (var_r5_2->unk3BC ^ 0xFF);
            var_r5_2 += 0x3C0;
            goto loop_10;
        }
        var_r4_2 = &lbl_814DBCE0 + (var_r6_2 * 0x78) + 0xE10;
loop_14:
        if (var_r6_2 < 0x1F) {
            var_r6_2 += 1;
            var_r4_2->unk74 = (s16) (var_r4_2->unk74 ^ 0xFF);
            var_r4_2 += 0x78;
            goto loop_14;
        }
    }
    temp_r3 = -temp_r0;
    var_r6_3 = &lbl_814DBCE0 + 0x1EF0;
    var_r7 = 0;
loop_24:
    if (var_r7 < 0x1F) {
        temp_f3 = var_r6_3->unk0;
        if ((temp_f3 > 0.0f) && (temp_f3 > (f32) temp_r0)) {
            var_r6_3->unk8 = (f32) temp_r0;
        } else if ((temp_f3 < 0.0f) && (temp_f3 < (f32) temp_r3)) {
            var_r6_3->unk8 = (f32) temp_r3;
        } else {
            var_r6_3->unk8 = temp_f3;
        }
        var_r6_3 += 0x14;
        var_r7 += 1;
        goto loop_24;
    }
}

/* 0x81324EEC */
void fn_81324EEC(s16 arg0) {
    void *var_r31;
    f32 temp_f0;
    f32 temp_f1;
    s16 temp_r4;
    s16 var_r29;
    s32 temp_r3;
    s32 temp_r3_2;
    s32 temp_r3_3;
    s32 temp_r3_4;
    s32 temp_r3_5;
    s32 temp_r3_6;
    s32 temp_r4_2;

    var_r31 = &lbl_814DDBD0;
    var_r29 = 0;
loop_15:
    if (var_r29 < 0x1F) {
        temp_f1 = var_r31->unk4;
        temp_f0 = 7.0f - ((70.0f - temp_f1) / 20.0f);
        temp_r4 = (s16) temp_f0;
        if ((arg0 >= (s32) ((s16) temp_f0 * 4)) && (temp_r3 = temp_r4 * 4, ((arg0 < (s32) (temp_r3 + 0x10)) != 0))) {
            var_r31->unkC = (f32) (var_r31->unk4 + Hermite((bitwise s32) (f32) (arg0 - temp_r3), 0, 0.0f, 0.0f, 16.0f, 10.0f, 0.0f));
        } else {
            temp_r3_2 = temp_r4 * 4;
            if ((arg0 >= (s32) (temp_r3_2 + 0x10)) && (arg0 <= (s32) (temp_r3_2 + 0x20))) {
                temp_r3_3 = arg0 - temp_r3_2;
                var_r31->unkC = (f32) (var_r31->unk4 + Hermite(temp_r3_3, (bitwise s16) (f32) (temp_r3_3 - 0x10), 0.0f, 10.0f, 0.0f, 16.0f, 0.0f, 0.0f));
            } else {
                temp_r3_4 = temp_r4 + 8;
                temp_r4_2 = temp_r3_4 * 4;
                if ((arg0 >= temp_r4_2) && (arg0 < (s32) (temp_r4_2 + 0x10))) {
                    var_r31->unkC = (f32) (var_r31->unk4 + Hermite((bitwise s32) (f32) (arg0 - temp_r4_2), 0, 0.0f, 0.0f, 16.0f, 10.0f, 0.0f));
                } else {
                    temp_r3_5 = temp_r3_4 * 4;
                    if ((arg0 >= (s32) (temp_r3_5 + 0x10)) && (arg0 < (s32) (temp_r3_5 + 0x20))) {
                        temp_r3_6 = arg0 - temp_r4_2;
                        var_r31->unkC = (f32) (var_r31->unk4 + Hermite(temp_r3_6, (bitwise s16) (f32) (temp_r3_6 - 0x10), 0.0f, 10.0f, 0.0f, 16.0f, 0.0f, 0.0f));
                    } else {
                        var_r31->unkC = temp_f1;
                    }
                }
            }
        }
        var_r31 += 0x14;
        var_r29 += 1;
        goto loop_15;
    }
}

/* 0x81325140 */
void fn_81325140(s16 arg0, s16 arg1) {
    void *var_r5;
    f32 temp_f3;
    s16 temp_r0;
    s16 temp_r3;
    s16 var_r6;

    var_r5 = &lbl_814DDBD0;
    var_r6 = 0;
    temp_r0 = (s16) Hermite((bitwise s32) (f32) arg0, 0, 70.0f, 0.0f, (f32) arg1, 0.0f, 0.0f);
    temp_r3 = -temp_r0;
loop_9:
    if (var_r6 < 0x1F) {
        temp_f3 = var_r5->unk4;
        if ((temp_f3 > 0.0f) && (temp_f3 > (f32) temp_r0)) {
            var_r5->unkC = (f32) temp_r0;
        } else if ((temp_f3 < 0.0f) && (temp_f3 < (f32) temp_r3)) {
            var_r5->unkC = (f32) temp_r3;
        } else {
            var_r5->unkC = temp_f3;
        }
        var_r6 += 1;
        var_r5->unk8 = (f32) var_r5->unk0;
        var_r5 += 0x14;
        goto loop_9;
    }
}

/* 0x8132527C */
void fn_8132527C(s16 arg0, s16 arg1) {
    void *var_r6;
    f32 temp_f3;
    s16 temp_r0;
    s16 temp_r3;
    s16 var_r5;
    s16 var_r6_2;
    s16 var_r7;
    void *var_r4;
    void *var_r4_2;
    void *var_r5_2;
    void *var_r6_3;

    temp_r0 = (s16) Hermite((s32) &lbl_814DBCE0, (bitwise s16) (f32) arg0, 0.0f, 0.0f, 0.0f, (f32) arg1, 80.0f, 0.0f);
    if (arg0 == 0) {
        var_r6 = &lbl_814DBCE0;
        var_r5 = 0;
loop_3:
        if (var_r5 < 0x16) {
            var_r5 += 8;
            var_r6->unk74 = (s16) (var_r6->unk74 ^ 0xFF);
            var_r6->unkEC = (s16) (var_r6->unkEC ^ 0xFF);
            var_r6->unk164 = (s16) (var_r6->unk164 ^ 0xFF);
            var_r6->unk1DC = (s16) (var_r6->unk1DC ^ 0xFF);
            var_r6->unk254 = (s16) (var_r6->unk254 ^ 0xFF);
            var_r6->unk2CC = (s16) (var_r6->unk2CC ^ 0xFF);
            var_r6->unk344 = (s16) (var_r6->unk344 ^ 0xFF);
            var_r6->unk3BC = (s16) (var_r6->unk3BC ^ 0xFF);
            var_r6 += 0x3C0;
            goto loop_3;
        }
        var_r4 = &lbl_814DBCE0 + (var_r5 * 0x78);
loop_7:
        if (var_r5 < 0x1E) {
            var_r5 += 1;
            var_r4->unk74 = (s16) (var_r4->unk74 ^ 0xFF);
            var_r4 += 0x78;
            goto loop_7;
        }
        var_r5_2 = &lbl_814DBCE0 + 0xE10;
        var_r6_2 = 0;
loop_10:
        if (var_r6_2 < 0x17) {
            var_r6_2 += 8;
            var_r5_2->unk74 = (s16) (var_r5_2->unk74 ^ 0xFF);
            var_r5_2->unkEC = (s16) (var_r5_2->unkEC ^ 0xFF);
            var_r5_2->unk164 = (s16) (var_r5_2->unk164 ^ 0xFF);
            var_r5_2->unk1DC = (s16) (var_r5_2->unk1DC ^ 0xFF);
            var_r5_2->unk254 = (s16) (var_r5_2->unk254 ^ 0xFF);
            var_r5_2->unk2CC = (s16) (var_r5_2->unk2CC ^ 0xFF);
            var_r5_2->unk344 = (s16) (var_r5_2->unk344 ^ 0xFF);
            var_r5_2->unk3BC = (s16) (var_r5_2->unk3BC ^ 0xFF);
            var_r5_2 += 0x3C0;
            goto loop_10;
        }
        var_r4_2 = &lbl_814DBCE0 + (var_r6_2 * 0x78) + 0xE10;
loop_14:
        if (var_r6_2 < 0x1F) {
            var_r6_2 += 1;
            var_r4_2->unk74 = (s16) (var_r4_2->unk74 ^ 0xFF);
            var_r4_2 += 0x78;
            goto loop_14;
        }
    }
    temp_r3 = -temp_r0;
    var_r6_3 = &lbl_814DBCE0 + 0x1C98;
    var_r7 = 0;
loop_24:
    if (var_r7 < 0x1E) {
        temp_f3 = var_r6_3->unk4;
        if ((temp_f3 > 0.0f) && (temp_f3 > (f32) temp_r0)) {
            var_r6_3->unkC = (f32) temp_r0;
        } else if ((temp_f3 < 0.0f) && (temp_f3 < (f32) temp_r3)) {
            var_r6_3->unkC = (f32) temp_r3;
        } else {
            var_r6_3->unkC = temp_f3;
        }
        var_r7 += 1;
        var_r6_3->unk8 = (f32) var_r6_3->unk0;
        var_r6_3 += 0x14;
        goto loop_24;
    }
}

/* 0x8132554C */
void OptionIcon_UpdateFace(void) {
    s32 spC;
    f32 temp_f3;
    s16 *temp_r4;
    s16 temp_r0;
    s16 temp_r0_2;
    s16 temp_r0_3;
    s16 temp_r25;
    s16 temp_r3_4;
    s16 var_r27;
    s16 var_r3;
    s16 var_r5;
    s32 *var_r26;
    s32 temp_r3;
    s32 temp_r3_2;
    s32 temp_r5;
    void *temp_r3_3;
    void *var_r6;

    Option_GetParams(&lbl_814DBCE0);
    if ((s16) lbl_814AD550 == 0) {
        OptionIcon_ResetShape();
    }
    if ((s16) lbl_814AD550 < 0x14) {
        lbl_814DBCE0.unk1C98 = 0.0f;
        lbl_814DBCE0.unk1C9C = 0.0f;
        fn_8132406C(&lbl_814DBCE0 + 0x74, 6);
        lbl_814DBCE0.unk1D24 = 0.0f;
        lbl_814DBCE0.unk1D28 = 0.0f;
        fn_8132406C(&lbl_814DBCE0 + 0x3BC, 6);
        lbl_814DBCE0.unk1DC4 = 0.0f;
        lbl_814DBCE0.unk1DC8 = 0.0f;
        fn_8132406C(&lbl_814DBCE0 + 0x77C, 6);
        lbl_814DBCE0.unk1E50 = 0.0f;
        lbl_814DBCE0.unk1E54 = 0.0f;
        fn_8132406C(&lbl_814DBCE0 + 0xAC4, 6);
        fn_81324098();
    } else if ((s16) lbl_814AD550 < 0x28) {
        temp_r0 = lbl_814AD550 - 0x14;
        spC = lbl_814ADFF0;
        var_r26 = &spC;
        var_r27 = 0;
loop_11:
        if (var_r27 < 4) {
            temp_r3 = (var_r27 % 2) * 0xA0;
            temp_r0_2 = temp_r3 - 0x50;
            temp_r3_2 = (var_r27 / 2) * 0x8C;
            temp_r25 = 0x46 - temp_r3_2;
            temp_r5 = *var_r26 * 0x14;
            temp_r3_3 = &lbl_814DBCE0 + temp_r5;
            temp_r3_3->unk1C98 = Hermite((s32) temp_r0_2, var_r27, (f32) temp_r0, 0.0f, 0.0f, (f32) ((temp_r3 - 0x50) / 20), 20.0f, (f32) temp_r0_2, 0.0f);
            (&lbl_814DBCE0 + (*var_r26 * 0x14))->unk1C9C = Hermite((s32) temp_r3_3, temp_r25, (bitwise f32) temp_r5, (f32) temp_r0, 0.0f, 0.0f, (f32) ((0x46 - temp_r3_2) / 20), 20.0f, (f32) temp_r25, 0.0f);
            temp_r4 = &lbl_814DBCE0 + ((*var_r26 * 0x78) + 0x74);
            var_r3 = *temp_r4 + 6;
            if (var_r3 > 0xFF) {
                var_r3 = 0xFF;
            }
            if (var_r3 < 0) {
                var_r3 = 0;
            }
            *temp_r4 = var_r3;
            var_r26 += 1;
            var_r27 += 1;
            goto loop_11;
        }
        fn_81324098();
    } else if ((s16) lbl_814AD550 < 0x46) {
        fn_81324268(lbl_814AD550 - 0x28, 0x1E);
    } else if ((s16) lbl_814AD550 < 0xE1) {
        fn_81324938(lbl_814AD550 - 0x46);
    } else if ((s16) lbl_814AD550 < 0x109) {
        var_r6 = &lbl_814DBCE0 + 0x1C98;
        var_r5 = 0;
        temp_r0_3 = (s16) Hermite((bitwise s32) (f32) (s16) (lbl_814AD550 - 0xE1), 0, 80.0f, 0.0f, 40.0f, 0.0f, 0.0f);
        temp_r3_4 = -temp_r0_3;
loop_27:
        if (var_r5 < 0x1E) {
            temp_f3 = var_r6->unk0;
            if ((temp_f3 > 0.0f) && (temp_f3 > (f32) temp_r0_3)) {
                var_r6->unk8 = (f32) temp_r0_3;
            } else if ((temp_f3 < 0.0f) && (temp_f3 < (f32) temp_r3_4)) {
                var_r6->unk8 = (f32) temp_r3_4;
            } else {
                var_r6->unk8 = temp_f3;
            }
            var_r6 += 0x14;
            var_r5 += 1;
            goto loop_27;
        }
    } else if ((s16) lbl_814AD550 < 0x131) {
        fn_81324C24(lbl_814AD550 - 0x109, 0x28);
    } else if ((s16) lbl_814AD550 < 0x18D) {
        fn_81324EEC(lbl_814AD550 - 0x131);
    } else if ((s16) lbl_814AD550 < 0x207) {
        fn_81324EEC(lbl_814AD550 - 0x18D);
    } else if ((s16) lbl_814AD550 < 0x22F) {
        fn_81325140(lbl_814AD550 - 0x207, 0x28);
    } else if ((s16) lbl_814AD550 < 0x257) {
        fn_8132527C(lbl_814AD550 - 0x22F, 0x28);
    }
    lbl_814AD550 += 1;
    if ((s16) lbl_814AD550 > 0x257) {
        lbl_814AD550 = 0x46;
    }
}

/* 0x81325960 */
void OptionIcon_CalcMtx(void) {
    u8 spD0[0x4];
    u8 spA0[0x30];
    u8 sp70[0x30];
    u8 sp40[0x30];
    s32 sp3C;
    s32 sp38;
    s32 sp34;
    s32 sp30;
    s32 sp2C;
    s32 sp28;
    s32 sp24;
    s32 sp20;
    s32 sp1C;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    s16 var_r26;
    s16 var_r26_2;
    void *var_r27;
    void *var_r27_2;
    void *var_r28;
    void *var_r28_2;

    MtxRotTrans(0, -0x4000, 0, &spA0, 0.0f, 0.0f, 0.0f);
    Menu_GetCubeMtx();
    PSMTXConcat(&spA0, &sp40);
    var_r27 = &lbl_814DBCE0 + (0 * 0x78);
    var_r26 = 0;
    var_r28 = &lbl_814DBCE0 + (0 * 0x14) + 0x1C98;
loop_2:
    if (var_r26 < 0x1E) {
        MtxRotTrans(var_r28->unk10, var_r28->unk12, 0, &sp70, var_r28->unk8, var_r28->unkC, 0.0f);
        PSMTXConcat(&sp40, &sp70, &spD0);
        sp28 = lbl_8137EE40.unk0;
        sp2C = lbl_8137EE40.unk4;
        sp30 = lbl_8137EE40.unk8;
        sp34 = sp28;
        sp38 = sp2C;
        sp3C = lbl_8137EE40.unk8;
        J3D_SetModelMtx(var_r27, &spD0, &sp34, sp2C);
        J3D_SetViewMtx(var_r27, Camera_GetView());
        J3D_CalcJoints(var_r27);
        var_r28 += 0x14;
        var_r27 += 0x78;
        var_r26 += 1;
        goto loop_2;
    }
    var_r26_2 = 0;
    var_r27_2 = &lbl_814DBCE0 + (0 * 0x14) + 0x1EF0;
    var_r28_2 = &lbl_814DBCE0 + (0 * 0x78) + 0xE10;
loop_5:
    if (var_r26_2 < 0x1F) {
        PSMTXTrans(&sp70, var_r27_2->unk8, var_r27_2->unkC, 0.0f);
        PSMTXConcat(&sp40, &sp70, &spD0);
        sp10 = lbl_8137EE4C.unk0;
        sp14 = lbl_8137EE4C.unk4;
        sp18 = lbl_8137EE4C.unk8;
        sp1C = sp10;
        sp20 = sp14;
        sp24 = lbl_8137EE4C.unk8;
        J3D_SetModelMtx(var_r28_2, &spD0, &sp1C, sp14);
        J3D_SetViewMtx(var_r28_2, Camera_GetView());
        J3D_CalcJoints(var_r28_2);
        var_r27_2 += 0x14;
        var_r28_2 += 0x78;
        var_r26_2 += 1;
        goto loop_5;
    }
}

/* 0x81325B48 */
void OptionIcon_Draw(void) {
    void *temp_r27;
    s16 var_r28;
    s16 var_r28_2;
    void *temp_r3;
    void *temp_r3_2;
    void *var_r30;
    void *var_r30_2;

    temp_r3 = Option_GetParams(&lbl_814DBCE0);
    lbl_814AD554.unk0 = (s16) temp_r3->unk0;
    var_r30 = &lbl_814DBCE0 + (0 * 0x78);
    var_r28 = 0;
    lbl_814AD554.unk2 = (s16) temp_r3->unk4;
    lbl_814AD554.unk4 = (s16) temp_r3->unk8;
    lbl_814AD554.unk6 = (s16) temp_r3->unkC;
    temp_r3_2 = lbl_814DBCE0.unk68->unk10;
    temp_r27 = temp_r3_2->unk30;
    temp_r3_2->unk30 = &lbl_814AD554;
loop_2:
    if (var_r28 < 0x1E) {
        J3D_Draw(var_r30);
        var_r30 += 0x78;
        var_r28 += 1;
        goto loop_2;
    }
    var_r28_2 = 0;
    var_r30_2 = &lbl_814DBCE0 + (0 * 0x78) + 0xE10;
loop_5:
    if (var_r28_2 < 0x1F) {
        J3D_Draw(var_r30_2);
        var_r30_2 += 0x78;
        var_r28_2 += 1;
        goto loop_5;
    }
    lbl_814DBCE0.unk68->unk10->unk30 = temp_r27;
}

/* 0x81325C30 */
void OptionIcon_UpdateHide(void) {
    void *var_r4;
    void *var_r4_2;
    s16 var_r3;
    s16 var_r3_2;
    s16 var_r3_3;
    s16 var_r3_4;
    s16 var_r3_5;
    s16 var_r3_6;
    s16 var_r3_7;
    s32 var_ctr;
    s32 var_ctr_2;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r4_3;
    void *temp_r4_4;
    void *temp_r4_5;

    var_ctr = 5;
    var_r4 = &lbl_814DBCE0;
    do {
        var_r3 = var_r4->unk74 - 0xA;
        if (var_r3 > 0xFF) {
            var_r3 = 0xFF;
        }
        if (var_r3 < 0) {
            var_r3 = 0;
        }
        var_r4->unk74 = var_r3;
        temp_r4 = var_r4 + 0x78;
        var_r3_2 = var_r4->unkEC - 0xA;
        if (var_r3_2 > 0xFF) {
            var_r3_2 = 0xFF;
        }
        if (var_r3_2 < 0) {
            var_r3_2 = 0;
        }
        temp_r4->unk74 = var_r3_2;
        temp_r4_2 = temp_r4 + 0x78;
        var_r3_3 = temp_r4->unkEC - 0xA;
        if (var_r3_3 > 0xFF) {
            var_r3_3 = 0xFF;
        }
        if (var_r3_3 < 0) {
            var_r3_3 = 0;
        }
        temp_r4_2->unk74 = var_r3_3;
        temp_r4_3 = temp_r4_2 + 0x78;
        var_r3_4 = temp_r4_2->unkEC - 0xA;
        if (var_r3_4 > 0xFF) {
            var_r3_4 = 0xFF;
        }
        if (var_r3_4 < 0) {
            var_r3_4 = 0;
        }
        temp_r4_3->unk74 = var_r3_4;
        temp_r4_4 = temp_r4_3 + 0x78;
        var_r3_5 = temp_r4_3->unkEC - 0xA;
        if (var_r3_5 > 0xFF) {
            var_r3_5 = 0xFF;
        }
        if (var_r3_5 < 0) {
            var_r3_5 = 0;
        }
        temp_r4_4->unk74 = var_r3_5;
        temp_r4_5 = temp_r4_4 + 0x78;
        var_r3_6 = temp_r4_4->unkEC - 0xA;
        if (var_r3_6 > 0xFF) {
            var_r3_6 = 0xFF;
        }
        if (var_r3_6 < 0) {
            var_r3_6 = 0;
        }
        temp_r4_5->unk74 = var_r3_6;
        var_r4 = temp_r4_5 + 0x78;
        var_ctr -= 1;
    } while (var_ctr != 0);
    var_ctr_2 = 0x1F;
    var_r4_2 = &lbl_814DCAF0;
    do {
        var_r3_7 = var_r4_2->unk74 - 0xA;
        if (var_r3_7 > 0xFF) {
            var_r3_7 = 0xFF;
        }
        if (var_r3_7 < 0) {
            var_r3_7 = 0;
        }
        var_r4_2->unk74 = var_r3_7;
        var_r4_2 += 0x78;
        var_ctr_2 -= 1;
    } while (var_ctr_2 != 0);
    lbl_814AD550 = 0;
}

/* 0x81325DB0 */
void OptionIcon_UpdateEnter(void) {
    void *var_r29;
    f32 var_f29;
    f32 var_f29_2;
    f32 var_f29_3;
    f32 var_f29_4;
    s16 var_r3;
    s16 var_r3_2;
    s32 var_r0;
    s32 var_r0_2;
    s32 var_r0_3;
    s32 var_r0_4;
    s32 var_r26;
    s32 var_r27;
    s32 var_r30;
    void *temp_r31;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;
    void *temp_r3_4;
    void *var_r28;
    void *var_r28_2;
    void *var_r29_2;

    var_r26 = 0;
    temp_r31 = Option_GetParams(&lbl_814DBCE0);
    var_r29 = &lbl_814DBCE0;
    var_r28 = &lbl_814DBCE0 + 0x1C98;
    var_r27 = 0;
    do {
        var_r3 = var_r29->unk74 + -temp_r31->unk20;
        if (var_r3 > 0xFF) {
            var_r3 = 0xFF;
        }
        if (var_r3 < 0) {
            var_r3 = 0;
        }
        var_r29->unk74 = var_r3;
        var_f29 = var_r28->unk8;
        temp_r3 = Option_GetParams();
        if (var_f29 > 0.0f) {
            var_f29 -= (f32) temp_r3->unk1E;
            if (var_f29 < 0.0f) {
                var_f29 = 0.0f;
            }
        } else if (var_f29 < 0.0f) {
            var_f29 += (f32) temp_r3->unk1E;
            if (var_f29 > 0.0f) {
                var_f29 = 0.0f;
            }
        }
        var_r28->unk8 = var_f29;
        if (var_f29 == 0.0f) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        if (var_r0 != 0) {
            var_r26 += 1;
        }
        var_f29_2 = var_r28->unkC;
        temp_r3_2 = Option_GetParams();
        if (var_f29_2 > 0.0f) {
            var_f29_2 -= (f32) temp_r3_2->unk1E;
            if (var_f29_2 < 0.0f) {
                var_f29_2 = 0.0f;
            }
        } else if (var_f29_2 < 0.0f) {
            var_f29_2 += (f32) temp_r3_2->unk1E;
            if (var_f29_2 > 0.0f) {
                var_f29_2 = 0.0f;
            }
        }
        var_r28->unkC = var_f29_2;
        if (var_f29_2 == 0.0f) {
            var_r0_2 = 1;
        } else {
            var_r0_2 = 0;
        }
        if (var_r0_2 != 0) {
            var_r26 += 1;
        }
        var_r27 += 1;
        var_r29 += 0x78;
        var_r28 += 0x14;
    } while (var_r27 < 0x1E);
    var_r28_2 = &lbl_814DBCE0 + 0xE10;
    var_r29_2 = &lbl_814DBCE0 + 0x1EF0;
    var_r30 = 0;
    do {
        var_r3_2 = var_r28_2->unk74 + -temp_r31->unk20;
        if (var_r3_2 > 0xFF) {
            var_r3_2 = 0xFF;
        }
        if (var_r3_2 < 0) {
            var_r3_2 = 0;
        }
        var_r28_2->unk74 = var_r3_2;
        var_f29_3 = var_r29_2->unk8;
        temp_r3_3 = Option_GetParams();
        if (var_f29_3 > 0.0f) {
            var_f29_3 -= (f32) temp_r3_3->unk1E;
            if (var_f29_3 < 0.0f) {
                var_f29_3 = 0.0f;
            }
        } else if (var_f29_3 < 0.0f) {
            var_f29_3 += (f32) temp_r3_3->unk1E;
            if (var_f29_3 > 0.0f) {
                var_f29_3 = 0.0f;
            }
        }
        var_r29_2->unk8 = var_f29_3;
        if (var_f29_3 == 0.0f) {
            var_r0_3 = 1;
        } else {
            var_r0_3 = 0;
        }
        if (var_r0_3 != 0) {
            var_r26 += 1;
        }
        var_f29_4 = var_r29_2->unkC;
        temp_r3_4 = Option_GetParams();
        if (var_f29_4 > 0.0f) {
            var_f29_4 -= (f32) temp_r3_4->unk1E;
            if (var_f29_4 < 0.0f) {
                var_f29_4 = 0.0f;
            }
        } else if (var_f29_4 < 0.0f) {
            var_f29_4 += (f32) temp_r3_4->unk1E;
            if (var_f29_4 > 0.0f) {
                var_f29_4 = 0.0f;
            }
        }
        var_r29_2->unkC = var_f29_4;
        if (var_f29_4 == 0.0f) {
            var_r0_4 = 1;
        } else {
            var_r0_4 = 0;
        }
        if (var_r0_4 != 0) {
            var_r26 += 1;
        }
        var_r30 += 1;
        var_r28_2 += 0x78;
        var_r29_2 += 0x14;
    } while (var_r30 < 0x1F);
    if (var_r26 == 0x7A) {
        lbl_814AD55C = 1;
    } else {
        lbl_814AD55C = 0;
    }
    lbl_814AD550 = 0;
}

/* 0x8132610C */
s32 OptionIcon_IsSettled(void) {
    return lbl_814AD55C;
}
