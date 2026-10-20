// Non-matching: functionally equivalent decompilation
//
// Submenu icon cubes, 0x813218C8..0x813230F0: Memory Card icon (6x6 cube grid, inner 12 hidden).
// Each icon has Init / UpdateFace (shown on the cube face) / UpdateEnter
// (zooming into the submenu) / UpdateHide / CalcMtx / Draw.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x813218C8 */
void CardIcon_Init(s32 arg0, s32 arg1) {
    void *var_r12;
    void *var_r30;
    s16 var_r29;
    s16 var_r30_2;
    void *var_r4;

    if (arg1 != 0) {
        var_r30 = &lbl_814D9F60;
        var_r29 = 0;
loop_3:
        if (var_r29 < 0x24) {
            var_r30->unk68 = arg0;
            J3D_InitModel(var_r30, 0);
            var_r30 += 0x78;
            var_r29 += 1;
            goto loop_3;
        }
    }
    var_r30_2 = 0;
    var_r12 = &lbl_814D9F60;
loop_6:
    if (var_r30_2 < 0x1C) {
        var_r12->unk74 = 0;
        var_r30_2 += 8;
        var_r12->unkEC = 0;
        var_r12->unk164 = 0;
        var_r12->unk1DC = 0;
        var_r12->unk254 = 0;
        var_r12->unk2CC = 0;
        var_r12->unk344 = 0;
        var_r12->unk3BC = 0;
        var_r12 += 0x3C0;
        goto loop_6;
    }
    var_r4 = &lbl_814D9F60 + (var_r30_2 * 0x78);
loop_10:
    if (var_r30_2 < 0x24) {
        var_r4->unk74 = 0;
        var_r4 += 0x78;
        var_r30_2 += 1;
        goto loop_10;
    }
    lbl_814AD520 = 0;
    lbl_814AD524 = 0;
    lbl_814AD530 = 0;
    CardIcon_InitGrid();
}

/* 0x813219F4 */
void CardIcon_InitGrid(void) {
    void *var_r29;
    void *var_r31;
    void *var_r7;
    s16 temp_r0;
    s16 var_r12;
    s16 var_r30;
    s16 var_r9;
    void *var_r4;

    var_r9 = 0;
    var_r31 = &lbl_814DB040;
    var_r7 = &lbl_814DB040;
loop_2:
    if (var_r9 < 0x24) {
        var_r7->unk6 = 0;
        var_r7->unk8 = 0;
        var_r7->unkA = 0;
        var_r7->unkE = (s16) (((var_r9 % 6) * 0x14) - 0x32);
        var_r7->unk10 = (s16) (0x32 - ((var_r9 / 6) * 0x14));
        var_r9 += 1;
        var_r7->unk12 = 0;
        var_r7->unk18 = 0;
        var_r7->unk19 = 0x80;
        var_r7->unk0 = (s16) var_r7->unk6;
        var_r7->unk2 = (s16) var_r7->unk8;
        var_r7->unk4 = (s16) var_r7->unkA;
        var_r7 += 0x1A;
        lbl_814AD520 = 0;
        goto loop_2;
    }
    var_r29 = &lbl_814DB040;
    var_r30 = 0;
loop_5:
    if (var_r30 < 0x24) {
        temp_r0 = Rand() & 0x1F;
        var_r29->unkC = temp_r0;
        var_r30 += 1;
        var_r29->unk14 = (s16) (temp_r0 + 0x14);
        var_r29 += 0x1A;
        goto loop_5;
    }
    var_r12 = 0;
loop_8:
    if (var_r12 < 0x1C) {
        var_r31->unk16 = 0;
        var_r12 += 8;
        var_r31->unk30 = 0;
        var_r31->unk4A = 0;
        var_r31->unk64 = 0;
        var_r31->unk7E = 0;
        var_r31->unk98 = 0;
        var_r31->unkB2 = 0;
        var_r31->unkCC = 0;
        var_r31 += 0xD0;
        goto loop_8;
    }
    var_r4 = &lbl_814DB040 + (var_r12 * 0x1A);
loop_12:
    if (var_r12 < 0x24) {
        var_r4->unk16 = 0;
        var_r4 += 0x1A;
        var_r12 += 1;
        goto loop_12;
    }
}

/* 0x81321B98 */
s32 fn_81321B98(s16 arg0) {
    if (((arg0 >= 7) && (arg0 <= 0xA)) || ((arg0 >= 0xD) && (arg0 <= 0x10)) || ((arg0 >= 0x13) && (arg0 <= 0x16))) {
        return 1;
    }
    return 0;
}

/* 0x81321BE4 */
s16 fn_81321BE4(s16 arg0) {
    if ((u32) arg0 <= 0x23U) {
        switch (arg0) {
        case 0:
            return 0;
        case 1:
        case 6:
            return 1;
        case 2:
        case 12:
            return 2;
        case 3:
        case 18:
            return 3;
        case 4:
        case 24:
            return 4;
        case 5:
        case 25:
        case 30:
            return 5;
        case 11:
        case 26:
        case 31:
            return 6;
        case 17:
        case 27:
        case 32:
            return 7;
        case 23:
        case 28:
        case 33:
            return 8;
        case 29:
        case 34:
            return 9;
        case 35:
            return 0xA;
        }
    } else {
    case 7:
    case 8:
    case 9:
    case 10:
    case 13:
    case 14:
    case 15:
    case 16:
    case 19:
    case 20:
    case 21:
    case 22:
        return 0;
    }
}

/* 0x81321C68 */
void fn_81321C68(void) {
    s32 sp4C;
    s32 sp48;
    void *var_r29;
    void *var_r30;
    void *var_r4;
    f32 temp_f26;
    f32 temp_f27;
    f32 temp_f28;
    f32 temp_f29;
    f32 temp_f30;
    f32 temp_f3;
    f32 temp_f6;
    s16 temp_r0;
    s16 temp_r3;
    s16 temp_r5;
    s16 temp_r6;
    s16 temp_r6_2;
    s16 temp_r7;
    s16 var_r28;
    s16 var_r8;
    s32 var_r0;

    var_r29 = &lbl_814DB040;
    var_r30 = &lbl_814DB040;
    var_r28 = 0;
loop_9:
    if (var_r28 < 0x24) {
        if ((s32) lbl_814AD520 <= (s16) var_r30->unkC) {
            var_r30->unk0 = (s16) var_r30->unk6;
            var_r30->unk2 = (s16) var_r30->unk8;
            var_r30->unk4 = (s16) var_r30->unkA;
        }
        if ((s32) lbl_814AD520 >= (s16) var_r30->unk14) {
            var_r30->unk0 = (s16) var_r30->unkE;
            var_r30->unk2 = (s16) var_r30->unk10;
            var_r30->unk4 = (s16) var_r30->unk12;
        }
        temp_r0 = var_r30->unkC;
        if ((s32) lbl_814AD520 > temp_r0) {
            temp_r3 = var_r30->unk14;
            if ((s32) lbl_814AD520 < temp_r3) {
                temp_r6 = var_r30->unk8;
                temp_r5 = var_r30->unk10;
                temp_f3 = (f32) var_r30->unk6;
                temp_f6 = (f32) var_r30->unkE;
                temp_f28 = (f32) temp_r0;
                sp4C = temp_r6 ^ 0x80000000;
                temp_f26 = (f32) temp_r3;
                temp_f30 = temp_f26 - temp_f28;
                sp48 = 0x43300000;
                temp_f27 = (f32) temp_r5;
                temp_f29 = (f32) temp_r6;
                var_r30->unk0 = (s16) Hermite(temp_r5, (f32) lbl_814AD520, temp_f28, temp_f3, (temp_f6 - temp_f3) / temp_f30, temp_f26, temp_f6, 0.0f);
                var_r30->unk2 = (s16) Hermite((bitwise s16) (f32) lbl_814AD520, temp_f28, temp_f29, (temp_f27 - temp_f29) / temp_f30, temp_f26, temp_f27, 0.0f);
                var_r30->unk4 = 0;
            }
        }
        var_r30 += 0x1A;
        var_r28 += 1;
        goto loop_9;
    }
    var_r4 = &lbl_814D9F60;
    var_r8 = 0;
loop_28:
    if (var_r8 < 0x24) {
        if (((var_r8 >= 7) && (var_r8 <= 0xA)) || ((var_r8 >= 0xD) && (var_r8 <= 0x10)) || ((var_r8 >= 0x13) && (var_r8 <= 0x16))) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        if (var_r0 == 0) {
            temp_r6_2 = var_r29->unkC;
            if (((s32) lbl_814AD520 > temp_r6_2) && (temp_r7 = var_r29->unk14, (((s32) lbl_814AD520 < temp_r7) != 0))) {
                var_r4->unk74 = (s16) ((s32) ((lbl_814AD520 - temp_r6_2) * 0xFF) / (s32) (temp_r7 - temp_r6_2));
            } else if ((s32) lbl_814AD520 <= temp_r6_2) {
                var_r4->unk74 = 0;
            } else if ((s32) lbl_814AD520 >= (s16) var_r29->unk14) {
                var_r4->unk74 = 0xFF;
            }
        }
        var_r29 += 0x1A;
        var_r4 += 0x78;
        var_r8 += 1;
        goto loop_28;
    }
}

/* 0x81321F60 */
void fn_81321F60(s32 arg0) {
    s16 var_r3;
    s16 var_r3_2;
    s16 var_r4;
    s16 var_r4_10;
    s16 var_r4_11;
    s16 var_r4_12;
    s16 var_r4_13;
    s16 var_r4_14;
    s16 var_r4_15;
    s16 var_r4_16;
    s16 var_r4_17;
    s16 var_r4_18;
    s16 var_r4_19;
    s16 var_r4_20;
    s16 var_r4_21;
    s16 var_r4_22;
    s16 var_r4_2;
    s16 var_r4_3;
    s16 var_r4_4;
    s16 var_r4_5;
    s16 var_r4_6;
    s16 var_r4_7;
    s16 var_r4_8;
    s16 var_r4_9;

    if (arg0 != 0) {
        lbl_814D9F60.unk3BC = (s16) (lbl_814D9F60.unk3BC + 0x1E);
        var_r4 = lbl_814D9F60.unk3BC;
        if (var_r4 > 0xFF) {
            var_r4 = 0xFF;
        }
        if (var_r4 < 0) {
            var_r4 = 0;
        }
        lbl_814D9F60.unk3BC = var_r4;
        if ((s16) lbl_814D9F60.unk3BC > 0x64) {
            lbl_814D9F60.unk434 = (s16) (lbl_814D9F60.unk434 + 0x1E);
            var_r4_2 = lbl_814D9F60.unk434;
            if (var_r4_2 > 0xFF) {
                var_r4_2 = 0xFF;
            }
            if (var_r4_2 < 0) {
                var_r4_2 = 0;
            }
            lbl_814D9F60.unk434 = var_r4_2;
            lbl_814D9F60.unk68C = (s16) (lbl_814D9F60.unk68C + 0x1E);
            var_r4_3 = lbl_814D9F60.unk68C;
            if (var_r4_3 > 0xFF) {
                var_r4_3 = 0xFF;
            }
            if (var_r4_3 < 0) {
                var_r4_3 = 0;
            }
            lbl_814D9F60.unk68C = var_r4_3;
        }
        if ((s16) lbl_814D9F60.unk434 > 0x64) {
            lbl_814D9F60.unk4AC = (s16) (lbl_814D9F60.unk4AC + 0x1E);
            var_r4_4 = lbl_814D9F60.unk4AC;
            if (var_r4_4 > 0xFF) {
                var_r4_4 = 0xFF;
            }
            if (var_r4_4 < 0) {
                var_r4_4 = 0;
            }
            lbl_814D9F60.unk4AC = var_r4_4;
            lbl_814D9F60.unk704 = (s16) (lbl_814D9F60.unk704 + 0x1E);
            var_r4_5 = lbl_814D9F60.unk704;
            if (var_r4_5 > 0xFF) {
                var_r4_5 = 0xFF;
            }
            if (var_r4_5 < 0) {
                var_r4_5 = 0;
            }
            lbl_814D9F60.unk704 = var_r4_5;
            lbl_814D9F60.unk95C = (s16) (lbl_814D9F60.unk95C + 0x1E);
            var_r4_6 = lbl_814D9F60.unk95C;
            if (var_r4_6 > 0xFF) {
                var_r4_6 = 0xFF;
            }
            if (var_r4_6 < 0) {
                var_r4_6 = 0;
            }
            lbl_814D9F60.unk95C = var_r4_6;
        }
        if ((s16) lbl_814D9F60.unk4AC > 0x64) {
            lbl_814D9F60.unk524 = (s16) (lbl_814D9F60.unk524 + 0x1E);
            var_r4_7 = lbl_814D9F60.unk524;
            if (var_r4_7 > 0xFF) {
                var_r4_7 = 0xFF;
            }
            if (var_r4_7 < 0) {
                var_r4_7 = 0;
            }
            lbl_814D9F60.unk524 = var_r4_7;
            lbl_814D9F60.unk77C = (s16) (lbl_814D9F60.unk77C + 0x1E);
            var_r4_8 = lbl_814D9F60.unk77C;
            if (var_r4_8 > 0xFF) {
                var_r4_8 = 0xFF;
            }
            if (var_r4_8 < 0) {
                var_r4_8 = 0;
            }
            lbl_814D9F60.unk77C = var_r4_8;
            lbl_814D9F60.unk9D4 = (s16) (lbl_814D9F60.unk9D4 + 0x1E);
            var_r4_9 = lbl_814D9F60.unk9D4;
            if (var_r4_9 > 0xFF) {
                var_r4_9 = 0xFF;
            }
            if (var_r4_9 < 0) {
                var_r4_9 = 0;
            }
            lbl_814D9F60.unk9D4 = var_r4_9;
        }
        if ((s16) lbl_814D9F60.unk524 > 0x64) {
            lbl_814D9F60.unk7F4 = (s16) (lbl_814D9F60.unk7F4 + 0x1E);
            var_r4_10 = lbl_814D9F60.unk7F4;
            if (var_r4_10 > 0xFF) {
                var_r4_10 = 0xFF;
            }
            if (var_r4_10 < 0) {
                var_r4_10 = 0;
            }
            lbl_814D9F60.unk7F4 = var_r4_10;
            lbl_814D9F60.unkA4C = (s16) (lbl_814D9F60.unkA4C + 0x1E);
            var_r4_11 = lbl_814D9F60.unkA4C;
            if (var_r4_11 > 0xFF) {
                var_r4_11 = 0xFF;
            }
            if (var_r4_11 < 0) {
                var_r4_11 = 0;
            }
            lbl_814D9F60.unkA4C = var_r4_11;
        }
        if ((s16) lbl_814D9F60.unk7F4 > 0x64) {
            lbl_814D9F60.unkAC4 = (s16) (lbl_814D9F60.unkAC4 + 0x1E);
            var_r3 = lbl_814D9F60.unkAC4;
            if (var_r3 > 0xFF) {
                var_r3 = 0xFF;
            }
            if (var_r3 < 0) {
                var_r3 = 0;
            }
            lbl_814D9F60.unkAC4 = var_r3;
            return;
        }
        return;
    }
    lbl_814D9F60.unk3BC = (s16) (lbl_814D9F60.unk3BC - 0xF);
    var_r4_12 = lbl_814D9F60.unk3BC;
    if (var_r4_12 > 0xFF) {
        var_r4_12 = 0xFF;
    }
    if (var_r4_12 < 0) {
        var_r4_12 = 0;
    }
    lbl_814D9F60.unk3BC = var_r4_12;
    lbl_814D9F60.unk434 = (s16) (lbl_814D9F60.unk434 - 0xF);
    var_r4_13 = lbl_814D9F60.unk434;
    if (var_r4_13 > 0xFF) {
        var_r4_13 = 0xFF;
    }
    if (var_r4_13 < 0) {
        var_r4_13 = 0;
    }
    lbl_814D9F60.unk434 = var_r4_13;
    lbl_814D9F60.unk68C = (s16) (lbl_814D9F60.unk68C - 0xF);
    var_r4_14 = lbl_814D9F60.unk68C;
    if (var_r4_14 > 0xFF) {
        var_r4_14 = 0xFF;
    }
    if (var_r4_14 < 0) {
        var_r4_14 = 0;
    }
    lbl_814D9F60.unk68C = var_r4_14;
    lbl_814D9F60.unk4AC = (s16) (lbl_814D9F60.unk4AC - 0xF);
    var_r4_15 = lbl_814D9F60.unk4AC;
    if (var_r4_15 > 0xFF) {
        var_r4_15 = 0xFF;
    }
    if (var_r4_15 < 0) {
        var_r4_15 = 0;
    }
    lbl_814D9F60.unk4AC = var_r4_15;
    lbl_814D9F60.unk704 = (s16) (lbl_814D9F60.unk704 - 0xF);
    var_r4_16 = lbl_814D9F60.unk704;
    if (var_r4_16 > 0xFF) {
        var_r4_16 = 0xFF;
    }
    if (var_r4_16 < 0) {
        var_r4_16 = 0;
    }
    lbl_814D9F60.unk704 = var_r4_16;
    lbl_814D9F60.unk95C = (s16) (lbl_814D9F60.unk95C - 0xF);
    var_r4_17 = lbl_814D9F60.unk95C;
    if (var_r4_17 > 0xFF) {
        var_r4_17 = 0xFF;
    }
    if (var_r4_17 < 0) {
        var_r4_17 = 0;
    }
    lbl_814D9F60.unk95C = var_r4_17;
    lbl_814D9F60.unk524 = (s16) (lbl_814D9F60.unk524 - 0xF);
    var_r4_18 = lbl_814D9F60.unk524;
    if (var_r4_18 > 0xFF) {
        var_r4_18 = 0xFF;
    }
    if (var_r4_18 < 0) {
        var_r4_18 = 0;
    }
    lbl_814D9F60.unk524 = var_r4_18;
    lbl_814D9F60.unk77C = (s16) (lbl_814D9F60.unk77C - 0xF);
    var_r4_19 = lbl_814D9F60.unk77C;
    if (var_r4_19 > 0xFF) {
        var_r4_19 = 0xFF;
    }
    if (var_r4_19 < 0) {
        var_r4_19 = 0;
    }
    lbl_814D9F60.unk77C = var_r4_19;
    lbl_814D9F60.unk9D4 = (s16) (lbl_814D9F60.unk9D4 - 0xF);
    var_r4_20 = lbl_814D9F60.unk9D4;
    if (var_r4_20 > 0xFF) {
        var_r4_20 = 0xFF;
    }
    if (var_r4_20 < 0) {
        var_r4_20 = 0;
    }
    lbl_814D9F60.unk9D4 = var_r4_20;
    lbl_814D9F60.unk7F4 = (s16) (lbl_814D9F60.unk7F4 - 0xF);
    var_r4_21 = lbl_814D9F60.unk7F4;
    if (var_r4_21 > 0xFF) {
        var_r4_21 = 0xFF;
    }
    if (var_r4_21 < 0) {
        var_r4_21 = 0;
    }
    lbl_814D9F60.unk7F4 = var_r4_21;
    lbl_814D9F60.unkA4C = (s16) (lbl_814D9F60.unkA4C - 0xF);
    var_r4_22 = lbl_814D9F60.unkA4C;
    if (var_r4_22 > 0xFF) {
        var_r4_22 = 0xFF;
    }
    if (var_r4_22 < 0) {
        var_r4_22 = 0;
    }
    lbl_814D9F60.unkA4C = var_r4_22;
    lbl_814D9F60.unkAC4 = (s16) (lbl_814D9F60.unkAC4 - 0xF);
    var_r3_2 = lbl_814D9F60.unkAC4;
    if (var_r3_2 > 0xFF) {
        var_r3_2 = 0xFF;
    }
    if (var_r3_2 < 0) {
        var_r3_2 = 0;
    }
    lbl_814D9F60.unkAC4 = var_r3_2;
}

/* 0x81322434 */
void fn_81322434(s32 arg0) {
    if (arg0 < 0x32) {
        lbl_814AD524 = (s16) Hermite((bitwise s16) (f32) arg0, 0.0f, 0.0f, 0.0f, (f32) 0x32, -8000.0f, 0.0f);
        return;
    }
    if (arg0 < 0x5A) {
        lbl_814AD524 = (s16) Hermite((bitwise s16) (f32) arg0, (f32) 0x32, -8000.0f, 0.0f, (f32) 0x5A, 32768.0f, 0.0f);
        return;
    }
    if (arg0 < 0x8C) {
        lbl_814AD524 = -0x8000;
        return;
    }
    if (arg0 < 0xBE) {
        lbl_814AD524 = (s16) Hermite((bitwise s16) (f32) arg0, (f32) 0x8C, 32768.0f, 0.0f, (f32) 0xBE, 40768.0f, 0.0f);
        return;
    }
    if (arg0 < 0xE6) {
        lbl_814AD524 = (s16) Hermite((bitwise s16) (f32) arg0, (f32) 0xBE, 40768.0f, 0.0f, (f32) 0xE6, 0.0f, 0.0f);
        return;
    }
    if (arg0 < 0x118) {
        lbl_814AD524 = 0;
    }
}

/* 0x81322650 */
void CardIcon_UpdateFace(void) {
    s16 temp_r0;
    s16 var_r12;
    s16 var_r28;
    s16 var_r30;
    s16 var_r30_2;
    s16 var_r31;
    s32 temp_r30;
    s32 temp_r31;
    s32 temp_r31_2;
    void *var_r28_2;
    void *var_r29;
    void *var_r29_2;
    void *var_r29_3;
    void *var_r29_4;
    void *var_r4;

    Card_GetParams(&lbl_814D9F60);
    if ((s32) lbl_814AD520 == 0) {
        var_r29 = &lbl_814D9F60 + 0x10E0;
        var_r28 = 0;
loop_3:
        if (var_r28 < 0x24) {
            temp_r0 = Rand() & 0x1F;
            var_r29->unkC = temp_r0;
            var_r28 += 1;
            var_r29->unk14 = (s16) (temp_r0 + 0x14);
            var_r29 += 0x1A;
            goto loop_3;
        }
    }
    if ((s32) lbl_814AD520 <= 0x71) {
        fn_81321C68();
    }
    if (((s32) lbl_814AD520 >= 0x96) && ((s32) lbl_814AD520 <= 0xC3)) {
        temp_r30 = lbl_814AD520 - 0x96;
        var_r29_2 = &lbl_814D9F60 + 0x10E0;
        var_r31 = 0;
loop_14:
        if (var_r31 < 0x24) {
            if ((fn_81321B98(var_r31) == 0) && (temp_r30 >= (s32) (fn_81321BE4(var_r31) * 3)) && (temp_r30 <= (s32) ((fn_81321BE4(var_r31) * 3) + 0xF))) {
                var_r29_2->unk16 = (s16) (((temp_r30 - (fn_81321BE4(var_r31) * 3)) << 0xE) / 15);
            }
            var_r29_2 += 0x1A;
            var_r31 += 1;
            goto loop_14;
        }
    } else if (((s32) lbl_814AD520 >= 0x12C) && ((s32) lbl_814AD520 <= 0x159)) {
        var_r29_3 = &lbl_814D9F60 + 0x10E0;
        temp_r31 = lbl_814AD520 - 0x12C;
        var_r30 = 0;
loop_24:
        if (var_r30 < 0x24) {
            if ((fn_81321B98(var_r30) == 0) && (temp_r31 >= (s32) (fn_81321BE4(var_r30) * 3)) && (temp_r31 <= (s32) ((fn_81321BE4(var_r30) * 3) + 0xF))) {
                var_r29_3->unk16 = (s16) (((temp_r31 - (fn_81321BE4(var_r30) * 3)) << 0xE) / 15);
            }
            var_r29_3 += 0x1A;
            var_r30 += 1;
            goto loop_24;
        }
    } else if (((s32) lbl_814AD520 >= 0x1B8) && ((s32) lbl_814AD520 <= 0x1E5)) {
        var_r29_4 = &lbl_814D9F60 + 0x10E0;
        temp_r31_2 = lbl_814AD520 - 0x1AE;
        var_r30_2 = 0;
loop_34:
        if (var_r30_2 < 0x24) {
            if ((fn_81321B98(var_r30_2) == 0) && (temp_r31_2 >= (s32) (fn_81321BE4(var_r30_2) * 3)) && (temp_r31_2 <= (s32) ((fn_81321BE4(var_r30_2) * 3) + 0xF))) {
                var_r29_4->unk16 = (s16) (((temp_r31_2 - (fn_81321BE4(var_r30_2) * 3)) << 0xE) / 15);
            }
            var_r29_4 += 0x1A;
            var_r30_2 += 1;
            goto loop_34;
        }
    } else {
        var_r12 = 0;
        var_r28_2 = &lbl_814D9F60 + 0x10E0;
loop_38:
        if (var_r12 < 0x1C) {
            var_r28_2->unk16 = 0;
            var_r12 += 8;
            var_r28_2->unk30 = 0;
            var_r28_2->unk4A = 0;
            var_r28_2->unk64 = 0;
            var_r28_2->unk7E = 0;
            var_r28_2->unk98 = 0;
            var_r28_2->unkB2 = 0;
            var_r28_2->unkCC = 0;
            var_r28_2 += 0xD0;
            goto loop_38;
        }
        var_r4 = &lbl_814D9F60 + (var_r12 * 0x1A) + 0x10E0;
loop_42:
        if (var_r12 < 0x24) {
            var_r4->unk16 = 0;
            var_r4 += 0x1A;
            var_r12 += 1;
            goto loop_42;
        }
    }
    if ((s32) lbl_814AD520 >= 0x154) {
        fn_81321F60(1);
    } else {
        fn_81321F60(0);
    }
    if (((s32) lbl_814AD520 >= 0xD2) && ((s32) lbl_814AD520 <= 0x1EA)) {
        fn_81322434(lbl_814AD520 - 0xD2);
    }
    lbl_814AD520 += 1;
    if ((s32) lbl_814AD520 > 0x1EA) {
        lbl_814AD520 = 0xD2;
    }
}

/* 0x81322A18 */
void CardIcon_CalcMtx(void) {
    u8 spA0[0x4];
    u8 sp70[0x30];
    u8 sp40[0x30];
    f32 sp3C;
    f32 sp38;
    f32 sp34;
    f32 sp30;
    f32 sp2C;
    f32 sp28;
    f32 sp24;
    f32 sp20;
    f32 sp1C;
    f32 sp18;
    f32 sp14;
    f32 sp10;
    f32 temp_f0;
    f32 temp_f0_2;
    s16 temp_r3;
    s16 var_r23;
    s32 var_r0;
    void *temp_r28;
    void *var_r24;
    void *var_r25;

    temp_r28 = Card_GetParams();
    MtxRotTrans(0x4000, 0, 0, &sp40, 0.0f, 0.0f, 0.0f);
    var_r25 = &lbl_814DB040 + (0 * 0x1A);
    var_r24 = &lbl_814D9F60 + (0 * 0x78);
    var_r23 = 0;
loop_13:
    if (var_r23 < 0x24) {
        temp_r3 = var_r25->unk16;
        MtxRotTrans(temp_r3, temp_r3, 0, &spA0, (f32) var_r25->unk0, (f32) var_r25->unk2, (f32) var_r25->unk4);
        MtxRot(0, lbl_814AD524, 0, &sp70);
        PSMTXConcat(&sp70, &spA0, &spA0);
        PSMTXConcat(&sp40, &spA0, &spA0);
        Menu_GetCubeMtx();
        PSMTXConcat(&spA0, &spA0);
        if (((var_r23 >= 7) && (var_r23 <= 0xA)) || ((var_r23 >= 0xD) && (var_r23 <= 0x10)) || ((var_r23 >= 0x13) && (var_r23 <= 0x16))) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        if (var_r0 != 0) {
            sp28 = lbl_8137EE18.unk0;
            sp2C = lbl_8137EE18.unk4;
            sp30 = lbl_8137EE18.unk8;
            temp_f0 = temp_r28->unk10 * 0.5f;
            sp28 = temp_f0;
            sp2C = temp_f0;
            sp30 = temp_f0;
            sp34 = sp28;
            sp38 = sp2C;
            sp3C = sp30;
            J3D_SetModelMtx(var_r24, &spA0, &sp34, sp2C);
        } else {
            sp10 = lbl_8137EE24.unk0;
            sp14 = lbl_8137EE24.unk4;
            sp18 = lbl_8137EE24.unk8;
            temp_f0_2 = temp_r28->unk10;
            sp10 = temp_f0_2;
            sp14 = temp_f0_2;
            sp18 = temp_f0_2;
            sp1C = sp10;
            sp20 = sp14;
            sp24 = sp18;
            J3D_SetModelMtx(var_r24, &spA0, &sp1C, sp14);
        }
        J3D_SetViewMtx(var_r24, Camera_GetView());
        J3D_CalcJoints(var_r24);
        var_r25 += 0x1A;
        var_r24 += 0x78;
        var_r23 += 1;
        goto loop_13;
    }
}

/* 0x81322C8C */
void CardIcon_Draw(void) {
    void *temp_r28;
    void *var_r31;
    s16 var_r29;
    void *temp_r3;
    void *temp_r3_2;

    temp_r3 = Card_GetParams();
    var_r31 = &lbl_814D9F60;
    lbl_814AD528.unk0 = (s16) temp_r3->unk0;
    var_r29 = 0;
    lbl_814AD528.unk2 = (s16) temp_r3->unk4;
    lbl_814AD528.unk4 = (s16) temp_r3->unk8;
    lbl_814AD528.unk6 = (s16) temp_r3->unkC;
    temp_r3_2 = lbl_814D9F60.unk68->unk10;
    temp_r28 = temp_r3_2->unk30;
    temp_r3_2->unk30 = &lbl_814AD528;
loop_4:
    if (var_r29 < 0x24) {
        if ((s16) var_r31->unk74 > 0) {
            J3D_Draw(var_r31);
        }
        var_r31 += 0x78;
        var_r29 += 1;
        goto loop_4;
    }
    lbl_814D9F60.unk68->unk10->unk30 = temp_r28;
}

/* 0x81322D58 */
void CardIcon_UpdateHide(void) {
    void *var_r4;
    s32 var_ctr;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r4_3;
    void *temp_r4_4;
    void *temp_r4_5;
    void *temp_r4_6;
    void *temp_r4_7;
    void *temp_r4_8;

    var_ctr = 4;
    var_r4 = &lbl_814D9F60;
    do {
        var_r4->unk74 = (s16) (var_r4->unk74 - 0xA);
        if ((s16) var_r4->unk74 < 0) {
            var_r4->unk74 = 0;
        }
        var_r4->unkEC = (s16) (var_r4->unkEC - 0xA);
        temp_r4 = var_r4 + 0x78;
        if ((s16) var_r4->unkEC < 0) {
            temp_r4->unk74 = 0;
        }
        temp_r4->unkEC = (s16) (temp_r4->unkEC - 0xA);
        temp_r4_2 = temp_r4 + 0x78;
        if ((s16) temp_r4->unkEC < 0) {
            temp_r4_2->unk74 = 0;
        }
        temp_r4_2->unkEC = (s16) (temp_r4_2->unkEC - 0xA);
        temp_r4_3 = temp_r4_2 + 0x78;
        if ((s16) temp_r4_2->unkEC < 0) {
            temp_r4_3->unk74 = 0;
        }
        temp_r4_3->unkEC = (s16) (temp_r4_3->unkEC - 0xA);
        temp_r4_4 = temp_r4_3 + 0x78;
        if ((s16) temp_r4_3->unkEC < 0) {
            temp_r4_4->unk74 = 0;
        }
        temp_r4_4->unkEC = (s16) (temp_r4_4->unkEC - 0xA);
        temp_r4_5 = temp_r4_4 + 0x78;
        if ((s16) temp_r4_4->unkEC < 0) {
            temp_r4_5->unk74 = 0;
        }
        temp_r4_5->unkEC = (s16) (temp_r4_5->unkEC - 0xA);
        temp_r4_6 = temp_r4_5 + 0x78;
        if ((s16) temp_r4_5->unkEC < 0) {
            temp_r4_6->unk74 = 0;
        }
        temp_r4_6->unkEC = (s16) (temp_r4_6->unkEC - 0xA);
        temp_r4_7 = temp_r4_6 + 0x78;
        if ((s16) temp_r4_6->unkEC < 0) {
            temp_r4_7->unk74 = 0;
        }
        temp_r4_7->unkEC = (s16) (temp_r4_7->unkEC - 0xA);
        temp_r4_8 = temp_r4_7 + 0x78;
        if ((s16) temp_r4_7->unkEC < 0) {
            temp_r4_8->unk74 = 0;
        }
        var_r4 = temp_r4_8 + 0x78;
        var_ctr -= 1;
    } while (var_ctr != 0);
    lbl_814AD520 = 0;
    lbl_814AD524 = 0;
}

/* 0x81322EE8 */
void CardIcon_UpdateEnter(void) {
    void *var_r29;
    void *var_r30;
    s16 var_r26;
    s16 var_r26_2;
    s16 var_r26_3;
    s32 var_r0;
    s32 var_r0_2;
    s32 var_r0_3;
    s32 var_r27;
    s32 var_r28;
    void *temp_r31;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;

    var_r27 = 0;
    var_r30 = &lbl_814D9F60;
    var_r29 = &lbl_814DB040;
    temp_r31 = Card_GetParams();
    var_r28 = 0;
    do {
        var_r30->unk74 = (s16) (var_r30->unk74 - temp_r31->unk60);
        if ((s16) var_r30->unk74 < 0) {
            var_r30->unk74 = 0;
        }
        var_r26 = var_r29->unk0;
        temp_r3 = Card_GetParams();
        if (var_r26 > 0) {
            var_r26 -= temp_r3->unk5E;
            if (var_r26 < 0) {
                var_r26 = 0;
            }
        } else if (var_r26 < 0) {
            var_r26 += temp_r3->unk5E;
            if (var_r26 > 0) {
                var_r26 = 0;
            }
        }
        var_r29->unk0 = var_r26;
        if (var_r26 == 0) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        if ((var_r0 != 0) && ((s16) var_r30->unk74 == 0)) {
            var_r27 += 1;
        }
        var_r26_2 = var_r29->unk2;
        temp_r3_2 = Card_GetParams();
        if (var_r26_2 > 0) {
            var_r26_2 -= temp_r3_2->unk5E;
            if (var_r26_2 < 0) {
                var_r26_2 = 0;
            }
        } else if (var_r26_2 < 0) {
            var_r26_2 += temp_r3_2->unk5E;
            if (var_r26_2 > 0) {
                var_r26_2 = 0;
            }
        }
        var_r29->unk2 = var_r26_2;
        if (var_r26_2 == 0) {
            var_r0_2 = 1;
        } else {
            var_r0_2 = 0;
        }
        if ((var_r0_2 != 0) && ((s16) var_r30->unk74 == 0)) {
            var_r27 += 1;
        }
        var_r26_3 = var_r29->unk4;
        temp_r3_3 = Card_GetParams();
        if (var_r26_3 > 0) {
            var_r26_3 -= temp_r3_3->unk5E;
            if (var_r26_3 < 0) {
                var_r26_3 = 0;
            }
        } else if (var_r26_3 < 0) {
            var_r26_3 += temp_r3_3->unk5E;
            if (var_r26_3 > 0) {
                var_r26_3 = 0;
            }
        }
        var_r29->unk4 = var_r26_3;
        if (var_r26_3 == 0) {
            var_r0_3 = 1;
        } else {
            var_r0_3 = 0;
        }
        if ((var_r0_3 != 0) && ((s16) var_r30->unk74 == 0)) {
            var_r27 += 1;
        }
        var_r28 += 1;
        var_r30 += 0x78;
        var_r29 += 0x1A;
    } while (var_r28 < 0x24);
    if (var_r27 == 0x6C) {
        lbl_814AD530 = 1;
    } else {
        lbl_814AD530 = 0;
    }
    lbl_814AD520 = 0;
    lbl_814AD524 = 0;
}

/* 0x813230E8 */
s32 fn_813230E8(void) {
    return lbl_814AD530;
}
