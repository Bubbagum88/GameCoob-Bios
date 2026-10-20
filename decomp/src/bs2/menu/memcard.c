// Non-matching: functionally equivalent decompilation
//
// Memory Card submenu UI (PAL 1.0 BS2, 0x813157B8..0x8131BF50): parameters,
// the browse / operation / confirm / format / message state machine, result
// messages, and the 2D drawing of the file browser.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x813157B8 */
void Card_InitParams(s32 arg0) {
    if (arg0 != 0) {
        gCardParams = &gCardParamsBuf;
        CardParams_SetDefaults(gCardParams);
    }
    Card_Init(arg0);
}

/* 0x81315800 */
void CardParams_SetDefaults(void *arg0) {
    arg0->unk0 = -0x2D;
    arg0->unk4 = -0x28;
    arg0->unk8 = 0x32;
    arg0->unkC = 0xFF;
    arg0->unk10 = 0.7f;
    arg0->unk14 = 5;
    arg0->unk16 = 8;
    arg0->unk18 = 0;
    arg0->unk1C = 0x96;
    arg0->unk20 = 0xFF;
    arg0->unk24 = 0;
    arg0->unk28 = 0x32;
    arg0->unk2C = 0xC8;
    arg0->unk30 = 0;
    arg0->unk34 = 0x1E;
    arg0->unk38 = 0x5A;
    arg0->unk3C = 0;
    arg0->unk40 = 0x14;
    arg0->unk44 = 0x3C;
    arg0->unk1A = 0;
    arg0->unk1E = 0x96;
    arg0->unk22 = 0xFF;
    arg0->unk26 = 0;
    arg0->unk2A = 0x32;
    arg0->unk2E = 0xC8;
    arg0->unk32 = 0;
    arg0->unk36 = 0x1E;
    arg0->unk3A = 0x5A;
    arg0->unk3E = 0;
    arg0->unk42 = 0x14;
    arg0->unk46 = 0x3C;
    arg0->unk48 = 2.0f;
    arg0->unk4C = 1.3f;
    arg0->unk50 = 0xFF;
    arg0->unk52 = 0xFF;
    arg0->unk54 = 0xFF;
    arg0->unk56 = 0xF0;
    arg0->unk58 = 0xE6;
    arg0->unk5A = 0;
    arg0->unk5C = 0x46;
    arg0->unk5E = 5;
    arg0->unk60 = 0xD;
    arg0->unk64 = 10.0f;
    arg0->unk68 = 5.0f;
    arg0->unk6C = 1;
    arg0->unk6E = 0x80;
    arg0->unk70 = 320.0f;
}

/* 0x81315934 */
void *Card_GetParams(void) {
    return gCardParams;
}

/* 0x8131593C */
void CardParams_SetColors(void *arg0) {
    s32 temp_r12;
    s32 temp_r12_2;
    s32 temp_r31;
    s32 temp_r31_2;
    s32 temp_r3;
    s32 var_ctr;
    s32 var_r3;
    u32 var_r31;
    u32 var_r31_2;
    u32 var_r8;
    u32 var_r8_2;
    u32 var_r8_3;
    void *temp_r0;
    void *var_r4;
    void *var_r4_2;
    void *var_r6;
    void *var_r9;
    void *var_r9_2;

    var_ctr = 0x10;
    temp_r0 = arg0 + arg0->unk4;
    var_r3 = 0;
    do {
        temp_r12 = (var_r3 * 2) & 0x1FE0;
        temp_r31 = (var_r3 * 2) & 0x1E;
        var_r9 = temp_r0;
        var_r31 = 0U;
loop_5:
        if (var_r31 < (u16) sCardGlh->unk10) {
            if ((u32) var_r9->unk0 == (u32) (((var_r3 << 0xC) & 0xFF0000) + 0x69610000 + (((var_r3 & 0xF) / 10) << 8) + ((var_r3 & 0xF) % 10) + 0x3030)) {
                *(&gCardUi + temp_r12 + temp_r31) = var_r9->unk4;
                *(&lbl_814B6438 + temp_r12 + temp_r31) = var_r9->unk6;
            }
            var_r9 += 0x10;
            var_r31 += 1;
            goto loop_5;
        }
        temp_r3 = var_r3 + 1;
        temp_r12_2 = (temp_r3 * 2) & 0x1FE0;
        temp_r31_2 = (temp_r3 * 2) & 0x1E;
        var_r9_2 = temp_r0;
        var_r31_2 = 0U;
loop_10:
        if (var_r31_2 < (u16) sCardGlh->unk10) {
            if ((u32) var_r9_2->unk0 == (u32) (((temp_r3 << 0xC) & 0xFF0000) + 0x69610000 + (((temp_r3 & 0xF) / 10) << 8) + ((temp_r3 & 0xF) % 10) + 0x3030)) {
                *(&gCardUi + temp_r12_2 + temp_r31_2) = var_r9_2->unk4;
                *(&lbl_814B6438 + temp_r12_2 + temp_r31_2) = var_r9_2->unk6;
            }
            var_r9_2 += 0x10;
            var_r31_2 += 1;
            goto loop_10;
        }
        var_r3 = temp_r3 + 1;
        var_ctr -= 1;
    } while (var_ctr != 0);
    var_r4 = temp_r0;
    var_r8 = 0U;
loop_16:
    if (var_r8 < (u16) sCardGlh->unk10) {
        if ((u32) var_r4->unk0 == 'fnma'U) {
            lbl_814AD490.unk0 = (u16) var_r4->unk4;
            lbl_814AD498.unk0 = (u16) var_r4->unk6;
        }
        var_r4 += 0x10;
        var_r8 += 1;
        goto loop_16;
    }
    var_r4_2 = temp_r0;
    var_r8_2 = 0U;
loop_21:
    if (var_r8_2 < (u16) sCardGlh->unk10) {
        if ((u32) var_r4_2->unk0 == 'fnmb'U) {
            lbl_814AD490.unk2 = (u16) var_r4_2->unk4;
            lbl_814AD498.unk2 = (u16) var_r4_2->unk6;
        }
        var_r4_2 += 0x10;
        var_r8_2 += 1;
        goto loop_21;
    }
    var_r6 = temp_r0;
    var_r8_3 = 0U;
loop_26:
    if (var_r8_3 < (u16) sCardGlh->unk10) {
        if ((u32) (var_r6->unk0 + 0x91930000) == 0x6331U) {
            lbl_814AD490.unk4 = (u16) var_r6->unk4;
            lbl_814AD498.unk4 = (u16) var_r6->unk6;
        }
        var_r6 += 0x10;
        var_r8_3 += 1;
        goto loop_26;
    }
}

/* 0x81315BA4 */
void Card_LoadLangRes(void) {
    Res_CardStrings();
    sCardSth = ResGet();
}
/* 0x81315BCC */
void Card_Init(s32 arg0) {
    s16 *var_r21;
    s16 *var_r22;
    s16 *var_r3;
    s32 var_ctr;
    s32 var_ctr_2;
    s32 var_ctr_3;
    s32 var_ctr_4;
    s32 var_r10;
    s32 var_r10_2;
    s32 var_r24;
    s32 var_r26;
    u32 var_r20;
    u32 var_r20_2;
    void *temp_r25;
    void *temp_r27;
    void *temp_r28;
    void *temp_r29;
    void *var_r11;
    void *var_r11_2;
    void *var_r23;
    void *var_r30;
    void *var_r30_2;
    void *var_r3_2;

    if (arg0 != 0) {
        Res_CardLayout();
        sCardGlh = ResGet();
        Res_CardStrings();
        sCardSth = ResGet();
        CardParams_SetColors(sCardGlh);
        Res_CardBarTex();
        lbl_814AD48C = ResGet();
    }
    fn_8131EEC4(arg0, &gCardUi, &gCardUi + 0x40);
    var_r10 = 0;
    var_ctr = 3;
    var_r11 = &gCardUi + 0x80;
    do {
        var_r11->unk0 = 0;
        var_r10 += 0x28;
        var_r11->unk2 = 0;
        var_r11->unk4 = 0;
        var_r11->unk6 = 0;
        var_r11->unk8 = 0;
        var_r11->unkA = 0;
        var_r11->unkC = 0;
        var_r11->unkE = 0;
        var_r11->unk10 = 0;
        var_r11->unk12 = 0;
        var_r11->unk14 = 0;
        var_r11->unk16 = 0;
        var_r11->unk18 = 0;
        var_r11->unk1A = 0;
        var_r11->unk1C = 0;
        var_r11->unk1E = 0;
        var_r11->unk20 = 0;
        var_r11->unk22 = 0;
        var_r11->unk24 = 0;
        var_r11->unk26 = 0;
        var_r11->unk28 = 0;
        var_r11->unk2A = 0;
        var_r11->unk2C = 0;
        var_r11->unk2E = 0;
        var_r11->unk30 = 0;
        var_r11->unk32 = 0;
        var_r11->unk34 = 0;
        var_r11->unk36 = 0;
        var_r11->unk38 = 0;
        var_r11->unk3A = 0;
        var_r11->unk3C = 0;
        var_r11->unk3E = 0;
        var_r11->unk40 = 0;
        var_r11->unk42 = 0;
        var_r11->unk44 = 0;
        var_r11->unk46 = 0;
        var_r11->unk48 = 0;
        var_r11->unk4A = 0;
        var_r11->unk4C = 0;
        var_r11->unk4E = 0;
        var_r11 += 0x50;
        var_ctr -= 1;
    } while (var_ctr != 0);
    var_r3 = &gCardUi + (var_r10 * 2) + 0x80;
    var_ctr_2 = 0x7F - var_r10;
    if (var_r10 < 0x7F) {
        do {
            *var_r3 = 0;
            var_r3 += 2;
            var_ctr_2 -= 1;
        } while (var_ctr_2 != 0);
    }
    var_r10_2 = 0;
    var_ctr_3 = 3;
    var_r11_2 = &gCardUi + 0x80;
    do {
        var_r11_2->unkFE = 0;
        var_r10_2 += 0x28;
        var_r11_2->unk100 = 0;
        var_r11_2->unk102 = 0;
        var_r11_2->unk104 = 0;
        var_r11_2->unk106 = 0;
        var_r11_2->unk108 = 0;
        var_r11_2->unk10A = 0;
        var_r11_2->unk10C = 0;
        var_r11_2->unk10E = 0;
        var_r11_2->unk110 = 0;
        var_r11_2->unk112 = 0;
        var_r11_2->unk114 = 0;
        var_r11_2->unk116 = 0;
        var_r11_2->unk118 = 0;
        var_r11_2->unk11A = 0;
        var_r11_2->unk11C = 0;
        var_r11_2->unk11E = 0;
        var_r11_2->unk120 = 0;
        var_r11_2->unk122 = 0;
        var_r11_2->unk124 = 0;
        var_r11_2->unk126 = 0;
        var_r11_2->unk128 = 0;
        var_r11_2->unk12A = 0;
        var_r11_2->unk12C = 0;
        var_r11_2->unk12E = 0;
        var_r11_2->unk130 = 0;
        var_r11_2->unk132 = 0;
        var_r11_2->unk134 = 0;
        var_r11_2->unk136 = 0;
        var_r11_2->unk138 = 0;
        var_r11_2->unk13A = 0;
        var_r11_2->unk13C = 0;
        var_r11_2->unk13E = 0;
        var_r11_2->unk140 = 0;
        var_r11_2->unk142 = 0;
        var_r11_2->unk144 = 0;
        var_r11_2->unk146 = 0;
        var_r11_2->unk148 = 0;
        var_r11_2->unk14A = 0;
        var_r11_2->unk14C = 0;
        var_r11_2 += 0x50;
        var_ctr_3 -= 1;
    } while (var_ctr_3 != 0);
    var_r3_2 = &gCardUi + (var_r10_2 * 2) + 0x80;
    var_ctr_4 = 0x7F - var_r10_2;
    if (var_r10_2 < 0x7F) {
        do {
            var_r3_2->unkFE = 0;
            var_r3_2 += 2;
            var_ctr_4 -= 1;
        } while (var_ctr_4 != 0);
    }
    sCardSlot = 0;
    var_r20 = 0U;
    lbl_814AD454.unk0 = 0;
    lbl_814AD454.unk1 = 0;
    lbl_814AD458.unk0 = 0;
    lbl_814AD458.unk1 = 0;
    var_r30 = &gCardUi + 0xA6C;
    lbl_814AD45C.unk0 = 0;
    lbl_814AD45C.unk1 = 0;
    sCardUiState = 1;
    sCardOp = 2;
    sCardUiNext = 1;
    sCardOpNext = 2;
    sCardOpSlot = 0;
    sCardOpKind = 0;
    sCardOpError = 0;
    sCardYesNo = 1;
    do {
        Fader_Init(var_r30);
        var_r30->unkC = var_r20;
        var_r20 += 1;
        var_r30 += 0x10;
    } while (var_r20 < 0x2FU);
    var_r20_2 = 0U;
    var_r26 = 'arau';
    var_r24 = 'arad';
    var_r22 = &lbl_814AD478;
    var_r21 = &lbl_814AD47C;
    var_r30_2 = &gCardUi + 0xD5C;
    var_r23 = &gCardUi + 0xDBC;
    do {
        temp_r29 = var_r30_2 + 0x20;
        temp_r28 = temp_r29 + 0x20;
        temp_r27 = temp_r28 + 0x60;
        temp_r25 = temp_r27 + 0x20;
        Fader_Init(var_r30_2);
        Fader_Init(temp_r29);
        Fader_Init(temp_r28);
        Fader_Init(temp_r27);
        temp_r27->unkC = var_r26;
        Fader_Init(temp_r25);
        temp_r25->unkC = var_r24;
        fn_8131A644(var_r23);
        *var_r22 = 1;
        var_r20_2 += 1;
        var_r24 += 0x100;
        var_r23 += 0x20;
        *var_r21 = *var_r22;
        var_r22 += 2;
        var_r21 += 2;
        var_r30_2 += 0x10;
        var_r26 += 0x100;
    } while (var_r20_2 < 2U);
    lbl_814AD4A6 = 1;
    lbl_814AD4A0 = 0U;
    lbl_814AD4A4 = 0;
    lbl_814AD4A8 = 0xF;
    lbl_814AD4AA = 0;
    lbl_814AD4AC = 1;
    lbl_814AD4AE = 0x1E;
    Fader_Init(&gCardUi + 0xE3C, 0xF, 0);
    gCardUi.unkE3E = 0xA;
    Fader_Init(&gCardUi + 0xE4C);
    gCardUi.unkE4E = 0xA;
    Fader_Init(&gCardUi + 0xE5C);
    gCardUi.unkE5E = 0xA;
}

/* 0x81315FDC */
s32 Card_CheckCopy(u8 arg0, s16 arg1, s32 *arg2) {
    s32 sp14;
    s32 temp_r30;
    s32 var_r0;
    s32 var_r0_2;
    s32 var_r28;
    s32 var_r29;
    u8 temp_r24;
    u8 temp_r24_2;
    void *temp_r31;
    void *temp_r3;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r5;
    void *temp_r5_2;

    var_r29 = saved_reg_r29;
    var_r28 = 0;
    temp_r30 = CardThread_GetFiles();
    temp_r31 = CardThread_GetSlots();
    temp_r24 = sCardSlot ^ 1;
    if ((s16) (CardThread_GetSlots() + (temp_r24 * 0x14))->unk2 == 4) {
        var_r0 = 1;
    } else {
        var_r0 = 0;
    }
    if ((var_r0 != 0) && (temp_r5 = temp_r30 + (arg0 * 0x5F4) + (arg1 * 0xC), (((u8) temp_r5->unk4 == 0) == 0)) && (temp_r4 = temp_r31 + ((arg0 ^ 1) * 0x14), (((u32) (temp_r31 + (arg0 * 0x14))->unk8 == (u32) temp_r4->unk8) != 0)) && ((u16) temp_r4->unk10 >= (u16) temp_r5->unk2) && ((u16) temp_r4->unkE != 0) && ((u16) temp_r5->unk6 == 0)) {
        var_r29 = -0x15;
        var_r28 = 1;
    } else {
        temp_r24_2 = sCardSlot ^ 1;
        temp_r3 = CardThread_GetSlots();
        if (&sp14 != NULL) {
            sp14 = (s32) (temp_r3 + (temp_r24_2 * 0x14))->unk2;
        }
        if ((s16) (temp_r3 + (temp_r24_2 * 0x14))->unk2 == 4) {
            var_r0_2 = 1;
        } else {
            var_r0_2 = 0;
        }
        if (var_r0_2 == 0) {
            switch (sp14) {                         /* irregular */
            case 0:
                var_r29 = -0x17;
                break;
            case 5:
            case 7:
            case 9:
                var_r29 = -0x16;
                break;
            default:
                var_r29 = -0x1C;
                break;
            }
        } else {
            temp_r5_2 = temp_r30 + (arg0 * 0x5F4) + (arg1 * 0xC);
            if ((u8) temp_r5_2->unk4 == 0) {
                var_r29 = -0x1A;
            } else {
                temp_r4_2 = temp_r31 + ((arg0 ^ 1) * 0x14);
                if ((u32) (temp_r31 + (arg0 * 0x14))->unk8 != (u32) temp_r4_2->unk8) {
                    var_r29 = -0x1B;
                } else if (((u16) temp_r4_2->unk10 < (u16) temp_r5_2->unk2) || ((u16) temp_r4_2->unkE == 0)) {
                    var_r29 = -0x19;
                } else if ((u16) temp_r5_2->unk6 != 0) {
                    var_r29 = -0x18;
                }
            }
        }
    }
    if (arg2 != NULL) {
        *arg2 = var_r29;
    }
    return var_r28;
}

/* 0x813161F8 */
s32 Card_CheckMove(u8 arg0, s16 arg1, s32 *arg2) {
    s32 sp14;
    s32 temp_r30;
    s32 var_r0;
    s32 var_r0_2;
    s32 var_r28;
    s32 var_r29;
    u8 temp_r24;
    u8 temp_r24_2;
    void *temp_r31;
    void *temp_r3;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r5;
    void *temp_r5_2;

    var_r29 = saved_reg_r29;
    var_r28 = 0;
    temp_r30 = CardThread_GetFiles();
    temp_r31 = CardThread_GetSlots();
    temp_r24 = sCardSlot ^ 1;
    if ((s16) (CardThread_GetSlots() + (temp_r24 * 0x14))->unk2 == 4) {
        var_r0 = 1;
    } else {
        var_r0 = 0;
    }
    if ((var_r0 != 0) && (temp_r5 = temp_r30 + (arg0 * 0x5F4) + (arg1 * 0xC), (((u8) temp_r5->unk5 == 0) == 0)) && (temp_r4 = temp_r31 + ((arg0 ^ 1) * 0x14), (((u32) (temp_r31 + (arg0 * 0x14))->unk8 == (u32) temp_r4->unk8) != 0)) && ((u16) temp_r4->unk10 >= (u16) temp_r5->unk2) && ((u16) temp_r4->unkE != 0) && ((u16) temp_r5->unk6 == 0)) {
        var_r29 = -0x15;
        var_r28 = 1;
    } else {
        temp_r24_2 = sCardSlot ^ 1;
        temp_r3 = CardThread_GetSlots();
        if (&sp14 != NULL) {
            sp14 = (s32) (temp_r3 + (temp_r24_2 * 0x14))->unk2;
        }
        if ((s16) (temp_r3 + (temp_r24_2 * 0x14))->unk2 == 4) {
            var_r0_2 = 1;
        } else {
            var_r0_2 = 0;
        }
        if (var_r0_2 == 0) {
            switch (sp14) {                         /* irregular */
            case 0:
                var_r29 = -0x17;
                break;
            case 5:
            case 7:
            case 9:
                var_r29 = -0x16;
                break;
            default:
                var_r29 = -0x1C;
                break;
            }
        } else {
            temp_r5_2 = temp_r30 + (arg0 * 0x5F4) + (arg1 * 0xC);
            if ((u8) temp_r5_2->unk5 == 0) {
                var_r29 = -0x1A;
            } else {
                temp_r4_2 = temp_r31 + ((arg0 ^ 1) * 0x14);
                if ((u32) (temp_r31 + (arg0 * 0x14))->unk8 != (u32) temp_r4_2->unk8) {
                    var_r29 = -0x1B;
                } else if (((u16) temp_r4_2->unk10 < (u16) temp_r5_2->unk2) || ((u16) temp_r4_2->unkE == 0)) {
                    var_r29 = -0x19;
                } else if ((u16) temp_r5_2->unk6 != 0) {
                    var_r29 = -0x18;
                }
            }
        }
    }
    if (arg2 != NULL) {
        *arg2 = var_r29;
    }
    return var_r28;
}

/* 0x81316414 */
void CardUi_Browse_Enter(s32 arg0) {
    s32 var_r31;
    u8 temp_r30;
    void *temp_r3;
    void *temp_r4;

    temp_r3 = CardThread_GetSlots();
    if (((s16) sCardUiState != 5) || (arg0 != 0)) {
        temp_r30 = sCardSlot ^ 1;
        temp_r4 = temp_r3 + (temp_r30 * 0x14);
        if (((s16) temp_r4->unk2 == 8) || ((s16) temp_r4->unk2 == 6)) {
            sCardUiNext = 4;
            sCardUiState = 4;
            sCardSlot ^= 1;
            return;
        }
        var_r31 = 1;
        sCardUiNext = 1;
        sCardUiState = 1;
        if ((s16) (temp_r3 + (sCardSlot * 0x14))->unk2 != 4) {
            if ((s16) (CardThread_GetSlots() + (temp_r30 * 0x14))->unk2 == 4) {

            } else {
                var_r31 = 0;
            }
            if (var_r31 != 0) {
                sCardSlot ^= 1;
            }
        }
    }
}

/* 0x813164FC */
s32 CardUi_Browse(void) {
    s32 temp_r26;
    s32 temp_r31;
    s32 temp_r6_2;
    s32 var_r0;
    s32 var_r27;
    s8 var_r3;
    u8 *temp_r28;
    u8 *temp_r4;
    u8 *temp_r5_2;
    u8 *temp_r6;
    u8 *temp_r6_3;
    u8 *temp_r7;
    u8 *temp_r7_2;
    u8 *temp_r8;
    u8 temp_r0;
    u8 temp_r3_2;
    u8 temp_r3_3;
    u8 temp_r4_2;
    u8 temp_r4_3;
    u8 temp_r5;
    u8 temp_r5_3;
    u8 temp_r5_4;
    u8 temp_r8_2;
    void *temp_r30;
    void *temp_r3;
    void *temp_r3_4;
    void *temp_r3_5;
    void *temp_r3_6;

    var_r27 = 4;
    temp_r30 = CardThread_GetSlots(&gCardUi);
    temp_r31 = CardThread_GetFiles();
    if (fn_81321714() == 0) {
        return 4;
    }
    temp_r26 = sCardSlot * 0x14;
    if ((s16) (temp_r30 + temp_r26)->unk2 == 4) {
        temp_r28 = &(&lbl_814AD460)[sCardSlot];
        temp_r3 = CardThread_GetSlots() + temp_r26;
        if ((s16) temp_r3->unk2 != 4) {
            var_r0 = 0;
        } else {
            var_r3 = ((s32) (0x7F - temp_r3->unkE) / 4) - 3;
            if (((0x7F - temp_r3->unkE) % 4) == 0) {
                var_r3 -= 1;
            }
            if (var_r3 < 0) {
                var_r3 = 0;
            }
            *temp_r28 = (u8) var_r3;
            var_r0 = 1;
        }
        if (var_r0 != 0) {
            if (gMenuPad.unk12 & 0x1000) {
                temp_r4 = &lbl_814AD458 + sCardSlot;
                *temp_r4 -= 1;
                if ((s8) *temp_r4 < 0) {
                    *temp_r4 = 0;
                    temp_r6 = &(&lbl_814AD45C)[sCardSlot];
                    temp_r3_2 = *temp_r6;
                    if ((s8) temp_r3_2 > 0) {
                        *temp_r6 = temp_r3_2 - 1;
                        fn_8132056C(&gCardUi + 0x27C, &lbl_814AD45C);
                        Sound_Play(0xB);
                    } else if (Pad_IsRepeat() == 0) {
                        Sound_Play(0xD);
                    }
                } else {
                    Sound_Play(0xB);
                }
            }
            if (gMenuPad.unk12 & 0x2000) {
                temp_r7 = &lbl_814AD458 + sCardSlot;
                temp_r8 = &(&lbl_814AD45C)[sCardSlot];
                *temp_r7 += 1;
                temp_r0 = *temp_r7;
                temp_r4_2 = *temp_r8;
                if ((s32) ((s8) *(&lbl_814AD454 + sCardSlot) + (((s8) temp_r0 + (s8) temp_r4_2) * 4)) >= 0x7F) {
                    if (Pad_IsRepeat() == 0) {
                        Sound_Play(0xD);
                    }
                    *(&lbl_814AD458 + sCardSlot) = *(&lbl_814AD458 + sCardSlot) - 1;
                } else if ((s8) temp_r0 > 3) {
                    *temp_r7 = 3;
                    if ((s8) temp_r4_2 < (s8) (&lbl_814AD460)[sCardSlot]) {
                        temp_r5 = *temp_r8;
                        *temp_r8 = temp_r5 + 1;
                        fn_8132056C(&gCardUi + 0x27C, &lbl_814AD45C, temp_r5);
                        Sound_Play(0xB);
                    } else if (Pad_IsRepeat() == 0) {
                        Sound_Play(0xD);
                    }
                } else {
                    Sound_Play(0xB);
                }
            }
            if (gMenuPad.unk12 & 0x4000) {
                if ((u8) sCardSlot == 0) {
                    if ((s8) *(&lbl_814AD454 + sCardSlot) > 0) {
                        Sound_Play(0xB);
                        *(&lbl_814AD454 + sCardSlot) = *(&lbl_814AD454 + sCardSlot) - 1;
                    } else if (Pad_IsRepeat() == 0) {
                        Sound_Play(0xD);
                    }
                } else if ((u8) sCardSlot == 1) {
                    temp_r5_2 = &lbl_814AD454 + sCardSlot;
                    *temp_r5_2 -= 1;
                    if ((s8) *temp_r5_2 < 0) {
                        temp_r6_2 = sCardSlot ^ 1;
                        *temp_r5_2 = 0;
                        if ((s16) (temp_r30 + (temp_r6_2 * 0x14))->unk2 == 4) {
                            if ((s32) ((s8) *(&lbl_814AD454 + temp_r6_2) + (((s8) *(&lbl_814AD458 + sCardSlot) + (s8) (&lbl_814AD45C)[temp_r6_2]) * 4)) >= 0x7F) {
                                if (Pad_IsRepeat() == 0) {
                                    Sound_Play(0xD);
                                }
                            } else {
                                sCardSlot ^= 1;
                                temp_r4_3 = *(&lbl_814AD458 + (sCardSlot ^ 1));
                                *(&lbl_814AD458 + sCardSlot) = temp_r4_3;
                                *(&lbl_814AD454 + sCardSlot) = 3;
                                Sound_Play(0xB, temp_r4_3, sCardSlot);
                            }
                        } else if (Pad_IsRepeat() == 0) {
                            Sound_Play(0xD);
                        }
                    } else {
                        Sound_Play(0xB);
                    }
                }
            }
            if (gMenuPad.unk12 & 0x8000) {
                if ((u8) sCardSlot == 1) {
                    temp_r6_3 = &lbl_814AD454 + sCardSlot;
                    temp_r3_3 = *temp_r6_3;
                    if ((s8) temp_r3_3 < 3) {
                        *temp_r6_3 = temp_r3_3 + 1;
                        temp_r5_3 = *temp_r6_3;
                        if ((s32) ((s8) temp_r5_3 + (((s8) *(&lbl_814AD458 + sCardSlot) + (s8) (&lbl_814AD45C)[sCardSlot]) * 4)) >= 0x7F) {
                            *temp_r6_3 = temp_r5_3 - 1;
                            if (Pad_IsRepeat() == 0) {
                                Sound_Play(0xD);
                            }
                        } else {
                            Sound_Play(0xB);
                        }
                    } else if (Pad_IsRepeat() == 0) {
                        Sound_Play(0xD);
                    }
                } else if ((u8) sCardSlot == 0) {
                    temp_r7_2 = &lbl_814AD454 + sCardSlot;
                    *temp_r7_2 += 1;
                    temp_r8_2 = *temp_r7_2;
                    if ((s32) ((s8) temp_r8_2 + (((s8) *(&lbl_814AD458 + sCardSlot) + (s8) (&lbl_814AD45C)[sCardSlot]) * 4)) >= 0x7F) {
                        *temp_r7_2 = temp_r8_2 - 1;
                        if (Pad_IsRepeat() == 0) {
                            Sound_Play(0xD);
                        }
                    } else if ((s8) temp_r8_2 > 3) {
                        *temp_r7_2 = 3;
                        if ((s16) (temp_r30 + ((sCardSlot ^ 1) * 0x14))->unk2 == 4) {
                            sCardSlot ^= 1;
                            temp_r5_4 = *(&lbl_814AD458 + (sCardSlot ^ 1));
                            *(&lbl_814AD458 + sCardSlot) = temp_r5_4;
                            *(&lbl_814AD454 + sCardSlot) = 0;
                            Sound_Play(0xB, (u8) &lbl_814AD454, temp_r5_4, sCardSlot);
                        } else if (Pad_IsRepeat() == 0) {
                            Sound_Play(0xD);
                        }
                    } else {
                        Sound_Play(0xB);
                    }
                }
            }
            if ((fn_81320ABC() != 0) && (gMenuPad.unkC & 0x100) && ((u16) *(temp_r31 + ((sCardSlot * 0x5F4) + ((&gCardUi + (sCardSlot * 0x3F8) + (((s8) *(&lbl_814AD454 + sCardSlot) + (((s8) *(&lbl_814AD458 + sCardSlot) * 4) + ((s8) (&lbl_814AD45C)[sCardSlot] * 4))) * 8))->unk27C * 0xC))) != 0)) {
                sCardUiNext = 2;
                Sound_Play(8);
            }
        }
    }
    if ((gMenuPad.unkC & 0x200) && ((temp_r3_4 = Menu_Get(), (((s16) temp_r3_4->unk18 == 0x7FFF) != 0)) || ((u16) temp_r3_4->unk32 != 0)) && (fn_81320ABC() != 0)) {
        Sound_Play(6);
        temp_r3_5 = CardThread_GetSlots();
        if ((u8) temp_r3_5->unk1 == 0) {
            if ((s16) temp_r3_5->unk2 == 7) {
                temp_r3_5->unk2 = 6;
                temp_r3_5->unk1 = 1U;
            } else if ((s16) temp_r3_5->unk2 == 9) {
                temp_r3_5->unk2 = 8;
                temp_r3_5->unk1 = 1U;
            }
        }
        temp_r3_6 = temp_r3_5 + 0x14;
        if ((u8) temp_r3_6->unk1 == 0) {
            if ((s16) temp_r3_6->unk2 == 7) {
                temp_r3_6->unk2 = 6;
                temp_r3_6->unk1 = 1U;
            } else if ((s16) temp_r3_6->unk2 == 9) {
                temp_r3_6->unk2 = 8;
                temp_r3_6->unk1 = 1U;
            }
        }
        var_r27 = 3;
    }
    return var_r27;
}

/* 0x81316BAC */
s32 CardUi_OpMenu(void) {
    s32 sp14;
    s32 spC;
    s32 temp_r30;

    CardThread_GetFiles();
    if (gMenuPad.unk12 & 0x1000) {
        Sound_Play(0xA);
        sCardOpNext -= 1;
        if ((s16) sCardOpNext < 0) {
            sCardOpNext = 2;
        }
    }
    if (gMenuPad.unk12 & 0x2000) {
        Sound_Play(0xA);
        sCardOpNext += 1;
        if ((s16) sCardOpNext >= 3) {
            sCardOpNext = 0;
        }
    }
    if ((gMenuPad.unkC & 0x100) && ((s16) sCardOpNext == (s16) sCardOp)) {
        temp_r30 = *(&lbl_814B6674 + (sCardSlot * 0x3F8) + (((s8) *(&lbl_814AD454 + sCardSlot) + (((s8) *(&lbl_814AD458 + sCardSlot) + (s8) (&lbl_814AD45C)[sCardSlot]) * 4)) * 8));
        sCardOpSlot = (s32) sCardSlot;
        sCardYesNo = 1;
        switch ((s16) sCardOp) {               /* irregular */
        case 1:
            CardThread_GetFiles();
            if (Card_CheckCopy(sCardSlot, (s16) temp_r30, &sp14) != 0) {
                Sound_Play(8);
                sCardUiNext = 3;
            } else {
                Sound_Play(0xD);
                sCardOpSlot = (s32) sCardSlot;
                sCardOpKind = 5;
                sCardOpError = sp14;
                sCardUiNext = 5;
            }
            break;
        case 0:
            CardThread_GetFiles();
            if (Card_CheckMove(sCardSlot, (s16) temp_r30, &spC) != 0) {
                Sound_Play(8);
                sCardUiNext = 3;
            } else {
                Sound_Play(0xD);
                sCardOpSlot = (s32) sCardSlot;
                sCardOpKind = 6;
                sCardOpError = spC;
                sCardUiNext = 5;
            }
            break;
        case 2:
            Sound_Play(8);
            sCardUiNext = 3;
            break;
        }
    }
    if (gMenuPad.unkC & 0x200) {
        Sound_Play(7);
        CardUi_Browse_Enter(0);
    }
    return 4;
}

/* 0x81316DDC */
s32 CardUi_Confirm(void) {
    s32 temp_r28;
    s32 var_r0;
    s32 var_r0_2;
    s32 var_r0_3;
    u8 temp_r27;
    u8 temp_r30;

    CardThread_GetSlots();
    if (gMenuPad.unk12 & 0x1000) {
        if ((s16) sCardYesNo != 0) {
            Sound_Play(9);
        }
        sCardYesNo = 0;
    }
    if (gMenuPad.unk12 & 0x2000) {
        if ((s16) sCardYesNo != 1) {
            Sound_Play(9);
        }
        sCardYesNo = 1;
    }
    if (gMenuPad.unkC & 0x100) {
        if ((s16) sCardYesNo == 0) {
            temp_r30 = sCardSlot;
            temp_r28 = *(&lbl_814B6674 + (temp_r30 * 0x3F8) + (((s8) *(&lbl_814AD454 + temp_r30) + (((s8) *(&lbl_814AD458 + temp_r30) + (s8) (&lbl_814AD45C)[temp_r30]) * 4)) * 8));
            sCardOpSlot = (s32) temp_r30;
            switch ((s16) sCardOp) {           /* irregular */
            case 1:
                sCardOpSlot = (s32) temp_r30;
                sCardOpKind = 2;
                if ((s16) (CardThread_GetSlots() + ((u8) (temp_r30 ^ 1) * 0x14))->unk2 == 4) {
                    var_r0 = 1;
                } else {
                    var_r0 = 0;
                }
                if (var_r0 != 0) {
                    Sound_Play(8);
                    fn_81320094(sCardSlot, (u16) ((s8) *(&lbl_814AD454 + sCardSlot) + ((s8) *(&lbl_814AD458 + sCardSlot) * 4)), (u16) temp_r28, 2);
                    CardThread_Copy(sCardSlot, (s16) temp_r28);
                    sCardOpError = -0x15;
                    lbl_814AD4A0 = 0U;
                } else {
                    sCardOpError = -0x17;
                }
                sCardUiNext = 5;
                break;
            case 0:
                sCardOpSlot = (s32) temp_r30;
                sCardOpKind = 3;
                if ((s16) (CardThread_GetSlots() + ((u8) (temp_r30 ^ 1) * 0x14))->unk2 == 4) {
                    var_r0_2 = 1;
                } else {
                    var_r0_2 = 0;
                }
                if (var_r0_2 != 0) {
                    Sound_Play(8);
                    fn_81320094(sCardSlot, (u16) ((s8) *(&lbl_814AD454 + sCardSlot) + ((s8) *(&lbl_814AD458 + sCardSlot) * 4)), (u16) temp_r28, 3);
                    CardThread_Move(sCardSlot, (s16) temp_r28);
                    sCardOpError = -0x15;
                    lbl_814AD4A0 = 0U;
                } else {
                    sCardOpError = -0x17;
                }
                sCardUiNext = 5;
                break;
            case 2:
                sCardOpSlot = (s32) temp_r30;
                sCardOpKind = 4;
                Sound_Play(8);
                fn_81320094(sCardSlot, (u16) ((s8) *(&lbl_814AD454 + sCardSlot) + ((s8) *(&lbl_814AD458 + sCardSlot) * 4)), (u16) temp_r28, 4);
                CardThread_Erase(sCardSlot, (s16) temp_r28);
                sCardUiNext = 5;
                lbl_814AD4A0 = 0U;
                sCardOpError = -0x15;
                break;
            }
        } else {
            Sound_Play(7);
            sCardUiNext = 2;
        }
    }
    if (gMenuPad.unkC & 0x200) {
        Sound_Play(7);
        sCardUiNext = 2;
    }
    temp_r27 = sCardSlot ^ 1;
    if ((s16) (CardThread_GetSlots() + (temp_r27 * 0x14))->unk2 == 4) {
        var_r0_3 = 1;
    } else {
        var_r0_3 = 0;
    }
    if ((var_r0_3 == 0) && ((s16) sCardOp < 2)) {
        if ((s16) sCardOp < 0) {

        } else {
            sCardUiNext = 2;
        }
    }
    return 4;
}

/* 0x81317150 */
s32 CardUi_Format(void) {
    s16 spA;
    s16 sp8;
    s16 temp_r0;
    s16 temp_r0_2;
    s16 temp_r3;
    s16 temp_r3_2;
    s32 temp_r0_3;
    s32 var_r0;
    void *temp_r3_3;
    void *temp_r3_4;

    CardThread_GetSlots();
    if ((fn_81320ABC() != 0) && ((Fader_GetAlpha(&lbl_814B6E64 + ((sCardSlot + 0x27) * 0x10), &sp8, NULL), Fader_GetAlpha(&lbl_814B6E64 + ((sCardSlot + 0x29) * 0x10), &spA, NULL), ((sp8 > 0xAC) != 0)) || (spA > 0xAC))) {
        var_r0 = 1;
    } else {
        var_r0 = 0;
    }
    if (var_r0 == 0) {
        return 4;
    }
    if (gMenuPad.unk12 & 0x1000) {
        temp_r0 = (&lbl_814AD47C)[sCardSlot];
        temp_r3 = temp_r0 & 0xFE;
        if (temp_r3 != temp_r0) {
            Sound_Play(0xB);
        }
        (&lbl_814AD47C)[sCardSlot] = temp_r3;
    }
    if (gMenuPad.unk12 & 0x2000) {
        temp_r0_2 = (&lbl_814AD47C)[sCardSlot];
        temp_r3_2 = temp_r0_2 | 1;
        if (temp_r3_2 != temp_r0_2) {
            Sound_Play(0xB);
        }
        (&lbl_814AD47C)[sCardSlot] = temp_r3_2;
    }
    if (gMenuPad.unkC & 0x100) {
        temp_r0_3 = (&lbl_814AD478)[sCardSlot] & 3;
        switch (temp_r0_3) {                        /* irregular */
        case 0:
            (&lbl_814AD47C)[sCardSlot] = 3;
            Sound_Play(8);
            break;
        case 2:
            Sound_Play(8);
            CardThread_Format(sCardSlot);
            sCardUiNext = 5;
            sCardOpSlot = (s32) sCardSlot;
            sCardOpKind = 1;
            sCardOpError = -0x15;
            break;
        case 3:
            Sound_Play(7);
            temp_r3_3 = CardThread_GetSlots() + (sCardSlot * 0x14);
            if ((s16) temp_r3_3->unk2 == 6) {
                temp_r3_3->unk2 = 7;
            } else {
                temp_r3_3->unk2 = 9;
            }
            (&lbl_814AD47C)[sCardSlot] = 1;
            CardUi_Browse_Enter(0, 1);
            break;
        }
    }
    if (gMenuPad.unkC & 0x200) {
        Sound_Play(7);
        temp_r3_4 = CardThread_GetSlots() + (sCardSlot * 0x14);
        if ((s16) temp_r3_4->unk2 == 6) {
            temp_r3_4->unk2 = 7;
        } else {
            temp_r3_4->unk2 = 9;
        }
        (&lbl_814AD47C)[sCardSlot] = 1;
        CardUi_Browse_Enter(0, 1);
    }
    return 4;
}

/* 0x813173EC */
s32 CardUi_Message(void) {
    CardThread_GetSlots();
    if (fn_81320ABC() == 0) {
        return 4;
    }
    if (CardThread_GetResult() != -0x15) {
        if ((u32) lbl_814AD4A0 != 0U) {
            switch ((s32) sCardOpKind) {           /* irregular */
            case 3:
                break;
            case 5:
            case 2:
                if ((s32) sCardOpError == 0) {
                    Sound_Play(0x10);
                }
                break;
            case 7:
            case 4:
                if ((s32) sCardOpError == 0) {
                    Sound_Play(0xF);
                }
                break;
            }
        }
        lbl_814AD4A0 = 0U;
        if (gMenuPad.unkC & 0x100) {
            sCardOpKind = 0;
            sCardYesNo = 1;
            sCardOpError = 0;
            CardUi_Browse_Enter(1);
            Sound_Play(7);
        }
        if (gMenuPad.unkC & 0x200) {
            sCardOpKind = 0;
            sCardYesNo = 1;
            sCardOpError = 0;
            CardUi_Browse_Enter(1, 1);
            Sound_Play(7);
        }
    }
    return 4;
}

/* 0x81317514 */
s32 CardFile_Compare(void *arg0, void *arg1) {
    u32 temp_r0;
    u32 temp_r0_2;
    u32 temp_r3;
    u32 temp_r5;

    temp_r5 = arg0->unk4;
    temp_r0 = arg1->unk4;
    if (temp_r5 > temp_r0) {
        return 1;
    }
    if (temp_r5 < temp_r0) {
        return -1;
    }
    temp_r3 = arg0->unk0;
    temp_r0_2 = arg1->unk0;
    if (temp_r3 > temp_r0_2) {
        return 1;
    }
    if (temp_r3 < temp_r0_2) {
        return -1;
    }
    return 0;
}

/* 0x81317564 */
void Card_SortFiles(u8 arg0) {
    s32 var_ctr;
    s32 var_r6;
    s8 var_r4_2;
    u8 *temp_r5;
    u8 temp_r4;
    void *temp_r3;
    void *temp_r7;
    void *var_r4;
    void *var_r5;

    temp_r7 = &lbl_814B6674 + (arg0 * 0x3F8);
    var_ctr = 0x7F;
    var_r5 = temp_r7;
    var_r4 = CardThread_GetFiles() + (arg0 * 0x5F4);
    var_r6 = 0;
    do {
        if ((u16) var_r4->unk0 != 0) {
            var_r5->unk0 = var_r6;
            var_r5->unk4 = (s32) var_r4->unk8;
        } else {
            var_r5->unk0 = var_r6;
            var_r5->unk4 = (s32) ((0xFF - var_r6) - 0x100);
        }
        var_r4 += 0xC;
        var_r5 += 8;
        var_r6 += 1;
        var_ctr -= 1;
    } while (var_ctr != 0);
    qsort(temp_r7, 0x7F, 8, CardFile_Compare);
    temp_r3 = CardThread_GetSlots() + (arg0 * 0x14);
    if ((s16) temp_r3->unk2 == 4) {
        var_r4_2 = ((s32) (0x7F - temp_r3->unkE) / 4) - 3;
        if (((0x7F - temp_r3->unkE) % 4) == 0) {
            var_r4_2 -= 1;
        }
        if (var_r4_2 < 0) {
            var_r4_2 = 0;
        }
        (&lbl_814AD460)[arg0] = (u8) var_r4_2;
    }
    temp_r5 = &(&lbl_814AD45C)[arg0];
    temp_r4 = (&lbl_814AD460)[arg0];
    if ((s8) temp_r4 < (s8) *temp_r5) {
        *temp_r5 = temp_r4;
    }
}

/* 0x813176AC */
void CardUi_OnSlotRefresh(u8 arg0) {
    void *temp_r3;
    s32 temp_r31;
    s32 var_r30;
    s8 var_r4;
    void *temp_r3_2;

    temp_r31 = arg0 * 0x14;
    temp_r3 = CardThread_GetSlots() + temp_r31;
    var_r30 = 0;
    temp_r3->unk1 = 0;
    temp_r3_2 = CardThread_GetSlots(temp_r3) + temp_r31;
    if ((s16) temp_r3_2->unk2 != 4) {

    } else {
        var_r4 = ((s32) (0x7F - temp_r3_2->unkE) / 4) - 3;
        if (((0x7F - temp_r3_2->unkE) % 4) == 0) {
            var_r4 -= 1;
        }
        if (var_r4 < 0) {
            var_r4 = 0;
        }
        (&lbl_814AD460)[arg0] = (u8) var_r4;
        var_r30 = 1;
    }
    if (var_r30 != 0) {
        Card_SortFiles(arg0);
        if ((u32) (sCardOpKind - 2) <= 2U) {
            if ((s32) sCardOpError == -0x15) {
                sCardOpError = CardThread_GetDestResult();
            }
            if ((s32) sCardOpError == 0) {
                CardThread_GetDestFile();
                fn_8132030C();
                switch ((s32) sCardOpKind) {       /* irregular */
                case 3:
                    if ((s32) sCardOpSlot == 0) {
                        Sound_Play(0x14);
                    } else {
                        Sound_Play(0x15);
                    }
                    sCardOpKind = 6;
                    break;
                case 4:
                    sCardOpKind = 7;
                    break;
                case 2:
                    Sound_Play(0x13);
                    sCardOpKind = 5;
                    break;
                }
            } else {
                fn_813204B0();
            }
        }
        fn_8132056C(&lbl_814B6674, &lbl_814AD45C);
    }
}

/* 0x8131783C */
void CardUi_Reset(void) {
    fn_8132056C(&lbl_814B6674, &lbl_814AD45C);
}

/* 0x81317868 */
void CardUi_PollOpResult(void) {
    if ((u32) (sCardOpKind - 2) <= 2U) {
        if ((s32) sCardOpError == -0x15) {
            if ((s32) sCardOpSlot == 0) {
                if ((s32) sCardOpKind < 4) {
                    if ((s32) sCardOpKind < 2) {

                    } else if ((u32) lbl_814AD4A0 == 0xAU) {
                        Sound_Play(0x11);
                    }
                }
            } else if ((s32) sCardOpKind < 4) {
                if ((s32) sCardOpKind < 2) {

                } else if ((u32) lbl_814AD4A0 == 0xAU) {
                    Sound_Play(0x12);
                }
            }
            lbl_814AD4A0 += 1;
            if ((u32) lbl_814AD4A0 > 0x14U) {
                lbl_814AD4A0 = 0x14U;
            }
            sCardOpError = CardThread_GetDestResult();
        }
        if (((s32) sCardOpError != -0x15) && ((s32) sCardOpError != 0)) {
            switch ((s32) sCardOpKind) {           /* irregular */
            case 3:
                sCardOpKind = 6;
                break;
            case 4:
                sCardOpKind = 7;
                break;
            case 2:
                sCardOpKind = 5;
                break;
            }
            fn_813204B0();
            Sound_Play(0xD);
        }
    } else if ((s32) sCardOpKind == 1) {
        if ((s32) sCardOpError == -0x15) {
            sCardOpError = CardThread_GetDestResult();
        }
        if ((s32) sCardOpError != -0x15) {
            if ((s32) sCardOpError == 0) {
                Sound_Play(0x10);
            } else {
                Sound_Play(0xD);
            }
            sCardOpKind = 8;
        }
    }
}

/* 0x813179E0 */
void CardUi_PollSlot(u8 arg0) {
    s32 temp_r29;
    s32 var_r0;
    u8 temp_r30;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;
    void *temp_r3_4;

    temp_r3 = CardThread_GetSlots();
    temp_r29 = arg0 * 0x14;
    temp_r3_2 = temp_r3 + temp_r29;
    if ((u8) temp_r3_2->unk1 == 1) {
        switch ((s16) temp_r3_2->unk2) {            /* switch 1; irregular */
        case 4:                                     /* switch 1 */
            CardUi_OnSlotRefresh(arg0);
            break;
        case 8:                                     /* switch 1 */
        case 6:                                     /* switch 1 */
            temp_r3_3 = CardThread_GetSlots();
            sCardSlot = arg0;
            sCardUiNext = 4;
            sCardUiState = 4;
            (&lbl_814AD47C)[arg0] = 1;
            (temp_r3_3 + temp_r29)->unk1 = 0;
            break;
        }
        if (((s32) sCardOpError == 0) && (CardThread_GetCmd() == 9) && ((s16) sCardUiState == 5)) {
            CardUi_Browse_Enter(1);
        }
    }
    CardUi_PollOpResult();
    if ((s16) (temp_r3 + temp_r29)->unk2 != 4) {
        fn_81320920(arg0);
    }
    temp_r3_4 = temp_r3 + (sCardSlot * 0x14);
    switch ((s16) temp_r3_4->unk2) {                /* switch 2; irregular */
    case 0:                                         /* switch 2 */
        CardUi_Browse_Enter(0);
        temp_r30 = sCardSlot ^ 1;
        if ((s16) (CardThread_GetSlots() + (temp_r30 * 0x14))->unk2 == 4) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        if (var_r0 != 0) {
            sCardSlot ^= 1;
            return;
        }
    case 4:                                         /* switch 2 */
    case 6:                                         /* switch 2 */
    case 8:                                         /* switch 2 */
        return;
    default:                                        /* switch 2 */
        CardUi_Browse_Enter(0);
        break;
    }
}

/* 0x81317B80 */
s32 CardUi_Update(void) {
    s32 var_r30;
    void *temp_r31;

    var_r30 = 4;
    temp_r31 = Scene_Get();
    sCardUiState = sCardUiNext;
    sCardOp = sCardOpNext;
    lbl_814AD478.unk0 = lbl_814AD47C.unk0;
    lbl_814AD478.unk2 = (s16) lbl_814AD47C.unk2;
    CardUi_PollSlot(0U, &lbl_814AD478, &lbl_814AD47C, sCardOpNext);
    CardUi_PollSlot(1U);
    fn_8131F8F8();
    if ((s16) temp_r31->unkB0 == 0) {
        switch ((s16) sCardUiState) {               /* irregular */
        case 1:
            var_r30 = CardUi_Browse();
            break;
        case 2:
            var_r30 = CardUi_OpMenu();
            break;
        case 3:
            var_r30 = CardUi_Confirm();
            break;
        case 4:
            var_r30 = CardUi_Format();
            break;
        case 5:
            var_r30 = CardUi_Message();
            break;
        }
    }
    return var_r30;
}

/* 0x81317C6C */
void CardUi_UpdateOpFaders(u8 arg0) {
    s32 temp_r28;
    s32 temp_r6;

    CardThread_GetFiles(&gCardUi);
    temp_r6 = arg0 * 0x3F8;
    temp_r28 = (&gCardUi + temp_r6 + (((s8) *(&lbl_814AD454 + arg0) + (((s8) *(&lbl_814AD458 + arg0) + (s8) (&lbl_814AD45C)[arg0]) * 4)) * 8))->unk27C;
    if (Card_CheckCopy(arg0, (s16) temp_r28, NULL, temp_r6) != 0) {
        Fader_Step(&gCardUi + (arg0 * 0x10) + 0xD7C, 0);
    } else {
        Fader_Step(&gCardUi + (arg0 * 0x10) + 0xD7C, 1);
    }
    if (Card_CheckMove(arg0, (s16) temp_r28, NULL) != 0) {
        Fader_Step(&gCardUi + (arg0 * 0x10) + 0xD9C, 0);
        return;
    }
    Fader_Step(&gCardUi + (arg0 * 0x10) + 0xD9C, 1);
}

/* 0x81317D88 */
void CardUi_UpdateFormatMsg(void *arg0, u8 arg1, s32 arg2) {
    void *temp_r4;

    temp_r4 = arg0 + (arg1 * 0x14);
    if (((s16) temp_r4->unk2 == 6) || ((s16) temp_r4->unk2 == 8)) {
        if (((s16) sCardUiState == 4) || (arg2 != 0)) {
            if (!((&lbl_814AD478)[arg1] & 2)) {
                if (arg1 == (u8) sCardSlot) {
                    Fader_Step(&gCardUi + ((arg1 + 0x27) * 0x10) + 0xA6C, 0);
                } else {
                    Fader_Step(&gCardUi + ((arg1 + 0x27) * 0x10) + 0xA6C, 2);
                }
                Fader_Step(&gCardUi + ((arg1 + 0x29) * 0x10) + 0xA6C, 1);
                return;
            }
            Fader_Step(&gCardUi + ((arg1 + 0x27) * 0x10) + 0xA6C, 1);
            if (arg1 == (u8) sCardSlot) {
                Fader_Step(&gCardUi + ((arg1 + 0x29) * 0x10) + 0xA6C, 0);
                return;
            }
            Fader_Step(&gCardUi + ((arg1 + 0x29) * 0x10) + 0xA6C, 2);
            return;
        }
        Fader_Step(&gCardUi + ((arg1 + 0x27) * 0x10) + 0xA6C, 1);
        Fader_Step(&gCardUi + ((arg1 + 0x29) * 0x10) + 0xA6C, 1);
        return;
    }
    Fader_Step(&gCardUi + ((arg1 + 0x27) * 0x10) + 0xA6C, 1);
    Fader_Step(&gCardUi + ((arg1 + 0x29) * 0x10) + 0xA6C, 1);
}

/* 0x81317F40 */
void CardUi_UpdateFaders(void) {
    s16 *var_r3;
    s16 *var_r3_2;
    s16 *var_r3_3;
    s16 *var_r3_4;
    s32 temp_cr0_eq;
    s32 var_ctr;
    s32 var_ctr_2;
    s32 var_ctr_3;
    s32 var_ctr_4;
    s32 var_ctr_5;
    s32 var_ctr_6;
    s32 var_ctr_7;
    s32 var_ctr_8;
    s32 var_r10;
    s32 var_r10_2;
    s32 var_r10_3;
    s32 var_r10_4;
    u8 *var_r25;
    u8 *var_r27;
    u8 var_r20;
    void *temp_r24;
    void *temp_r26;
    void *temp_r3;
    void *var_r11;
    void *var_r11_2;
    void *var_r11_3;
    void *var_r11_4;
    void *var_r28;
    void *var_r29;
    void *var_r30;

    temp_r3 = CardThread_GetSlots(&gCardUi);
    var_r30 = temp_r3;
    var_r28 = &gCardUi + 0x80;
    var_r20 = 0;
    var_r27 = &lbl_814AD45C;
    var_r25 = &lbl_814AD460;
    var_r29 = &gCardUi + 0xD5C;
loop_63:
    if (var_r20 < 2U) {
        temp_r26 = var_r29 + 0xA0;
        temp_r24 = temp_r26 + 0x20;
        if ((s16) var_r30->unk2 == 4) {
            Fader_Step(var_r29, 0);
            CardUi_UpdateOpFaders(0U);
            CardUi_UpdateOpFaders(1U);
        } else {
            Fader_Step(var_r29, 1);
        }
        if ((s16) sCardUiState != 5) {
            if ((s16) var_r30->unk2 == 0) {
                Fader_Step(&gCardUi + ((var_r20 + 0x21) * 0x10) + 0xA6C, 0);
                var_r10 = 0;
                if (0x7F > 8) {
                    var_ctr = 3;
                    var_r11 = var_r28;
                    do {
                        var_r11->unk0 = 0;
                        var_r10 += 0x28;
                        var_r11->unk2 = 0;
                        var_r11->unk4 = 0;
                        var_r11->unk6 = 0;
                        var_r11->unk8 = 0;
                        var_r11->unkA = 0;
                        var_r11->unkC = 0;
                        var_r11->unkE = 0;
                        var_r11->unk10 = 0;
                        var_r11->unk12 = 0;
                        var_r11->unk14 = 0;
                        var_r11->unk16 = 0;
                        var_r11->unk18 = 0;
                        var_r11->unk1A = 0;
                        var_r11->unk1C = 0;
                        var_r11->unk1E = 0;
                        var_r11->unk20 = 0;
                        var_r11->unk22 = 0;
                        var_r11->unk24 = 0;
                        var_r11->unk26 = 0;
                        var_r11->unk28 = 0;
                        var_r11->unk2A = 0;
                        var_r11->unk2C = 0;
                        var_r11->unk2E = 0;
                        var_r11->unk30 = 0;
                        var_r11->unk32 = 0;
                        var_r11->unk34 = 0;
                        var_r11->unk36 = 0;
                        var_r11->unk38 = 0;
                        var_r11->unk3A = 0;
                        var_r11->unk3C = 0;
                        var_r11->unk3E = 0;
                        var_r11->unk40 = 0;
                        var_r11->unk42 = 0;
                        var_r11->unk44 = 0;
                        var_r11->unk46 = 0;
                        var_r11->unk48 = 0;
                        var_r11->unk4A = 0;
                        var_r11->unk4C = 0;
                        var_r11->unk4E = 0;
                        var_r11 += 0x50;
                        var_ctr -= 1;
                    } while (var_ctr != 0);
                }
                var_r3 = var_r28 + (var_r10 * 2);
                var_ctr_2 = 0x7F - var_r10;
                if (var_r10 < 0x7F) {
                    do {
                        *var_r3 = 0;
                        var_r3 += 2;
                        var_ctr_2 -= 1;
                    } while (var_ctr_2 != 0);
                }
            } else {
                Fader_Step(&gCardUi + ((var_r20 + 0x21) * 0x10) + 0xA6C, 1);
            }
            if (((s16) var_r30->unk2 == 5) || ((s16) var_r30->unk2 == 0xA) || ((s16) var_r30->unk2 == 7) || ((s16) var_r30->unk2 == 9)) {
                Fader_Step(&gCardUi + ((var_r20 + 0x23) * 0x10) + 0xA6C, 0);
                var_r10_2 = 0;
                if (0x7F > 8) {
                    var_ctr_3 = 3;
                    var_r11_2 = var_r28;
                    do {
                        var_r11_2->unk0 = 0;
                        var_r10_2 += 0x28;
                        var_r11_2->unk2 = 0;
                        var_r11_2->unk4 = 0;
                        var_r11_2->unk6 = 0;
                        var_r11_2->unk8 = 0;
                        var_r11_2->unkA = 0;
                        var_r11_2->unkC = 0;
                        var_r11_2->unkE = 0;
                        var_r11_2->unk10 = 0;
                        var_r11_2->unk12 = 0;
                        var_r11_2->unk14 = 0;
                        var_r11_2->unk16 = 0;
                        var_r11_2->unk18 = 0;
                        var_r11_2->unk1A = 0;
                        var_r11_2->unk1C = 0;
                        var_r11_2->unk1E = 0;
                        var_r11_2->unk20 = 0;
                        var_r11_2->unk22 = 0;
                        var_r11_2->unk24 = 0;
                        var_r11_2->unk26 = 0;
                        var_r11_2->unk28 = 0;
                        var_r11_2->unk2A = 0;
                        var_r11_2->unk2C = 0;
                        var_r11_2->unk2E = 0;
                        var_r11_2->unk30 = 0;
                        var_r11_2->unk32 = 0;
                        var_r11_2->unk34 = 0;
                        var_r11_2->unk36 = 0;
                        var_r11_2->unk38 = 0;
                        var_r11_2->unk3A = 0;
                        var_r11_2->unk3C = 0;
                        var_r11_2->unk3E = 0;
                        var_r11_2->unk40 = 0;
                        var_r11_2->unk42 = 0;
                        var_r11_2->unk44 = 0;
                        var_r11_2->unk46 = 0;
                        var_r11_2->unk48 = 0;
                        var_r11_2->unk4A = 0;
                        var_r11_2->unk4C = 0;
                        var_r11_2->unk4E = 0;
                        var_r11_2 += 0x50;
                        var_ctr_3 -= 1;
                    } while (var_ctr_3 != 0);
                }
                var_r3_2 = var_r28 + (var_r10_2 * 2);
                var_ctr_4 = 0x7F - var_r10_2;
                if (var_r10_2 < 0x7F) {
                    do {
                        *var_r3_2 = 0;
                        var_r3_2 += 2;
                        var_ctr_4 -= 1;
                    } while (var_ctr_4 != 0);
                }
            } else {
                Fader_Step(&gCardUi + ((var_r20 + 0x23) * 0x10) + 0xA6C, 1);
            }
            CardUi_UpdateFormatMsg(temp_r3, var_r20, 0);
        } else if ((((s32) sCardOpKind != 1) && ((s32) sCardOpKind != 8)) || (temp_cr0_eq = (s32) sCardOpSlot == (s32) var_r20, (temp_cr0_eq != 0))) {
            Fader_Step(&gCardUi + ((var_r20 + 0x21) * 0x10) + 0xA6C, 1);
            Fader_Step(&gCardUi + ((var_r20 + 0x23) * 0x10) + 0xA6C, 1);
            Fader_Step(&gCardUi + ((var_r20 + 0x27) * 0x10) + 0xA6C, 1);
            Fader_Step(&gCardUi + ((var_r20 + 0x29) * 0x10) + 0xA6C, 1);
        } else if (temp_cr0_eq == 0) {
            if ((s16) var_r30->unk2 == 0) {
                Fader_Step(&gCardUi + ((var_r20 + 0x21) * 0x10) + 0xA6C, 0);
                var_r10_3 = 0;
                if (0x7F > 8) {
                    var_ctr_5 = 3;
                    var_r11_3 = var_r28;
                    do {
                        var_r11_3->unk0 = 0;
                        var_r10_3 += 0x28;
                        var_r11_3->unk2 = 0;
                        var_r11_3->unk4 = 0;
                        var_r11_3->unk6 = 0;
                        var_r11_3->unk8 = 0;
                        var_r11_3->unkA = 0;
                        var_r11_3->unkC = 0;
                        var_r11_3->unkE = 0;
                        var_r11_3->unk10 = 0;
                        var_r11_3->unk12 = 0;
                        var_r11_3->unk14 = 0;
                        var_r11_3->unk16 = 0;
                        var_r11_3->unk18 = 0;
                        var_r11_3->unk1A = 0;
                        var_r11_3->unk1C = 0;
                        var_r11_3->unk1E = 0;
                        var_r11_3->unk20 = 0;
                        var_r11_3->unk22 = 0;
                        var_r11_3->unk24 = 0;
                        var_r11_3->unk26 = 0;
                        var_r11_3->unk28 = 0;
                        var_r11_3->unk2A = 0;
                        var_r11_3->unk2C = 0;
                        var_r11_3->unk2E = 0;
                        var_r11_3->unk30 = 0;
                        var_r11_3->unk32 = 0;
                        var_r11_3->unk34 = 0;
                        var_r11_3->unk36 = 0;
                        var_r11_3->unk38 = 0;
                        var_r11_3->unk3A = 0;
                        var_r11_3->unk3C = 0;
                        var_r11_3->unk3E = 0;
                        var_r11_3->unk40 = 0;
                        var_r11_3->unk42 = 0;
                        var_r11_3->unk44 = 0;
                        var_r11_3->unk46 = 0;
                        var_r11_3->unk48 = 0;
                        var_r11_3->unk4A = 0;
                        var_r11_3->unk4C = 0;
                        var_r11_3->unk4E = 0;
                        var_r11_3 += 0x50;
                        var_ctr_5 -= 1;
                    } while (var_ctr_5 != 0);
                }
                var_r3_3 = var_r28 + (var_r10_3 * 2);
                var_ctr_6 = 0x7F - var_r10_3;
                if (var_r10_3 < 0x7F) {
                    do {
                        *var_r3_3 = 0;
                        var_r3_3 += 2;
                        var_ctr_6 -= 1;
                    } while (var_ctr_6 != 0);
                }
            } else {
                Fader_Step(&gCardUi + ((var_r20 + 0x21) * 0x10) + 0xA6C, 1);
            }
            if (((s16) var_r30->unk2 == 5) || ((s16) var_r30->unk2 == 0xA) || ((s16) var_r30->unk2 == 7) || ((s16) var_r30->unk2 == 9)) {
                Fader_Step(&gCardUi + ((var_r20 + 0x23) * 0x10) + 0xA6C, 0);
                var_r10_4 = 0;
                if (0x7F > 8) {
                    var_ctr_7 = 3;
                    var_r11_4 = var_r28;
                    do {
                        var_r11_4->unk0 = 0;
                        var_r10_4 += 0x28;
                        var_r11_4->unk2 = 0;
                        var_r11_4->unk4 = 0;
                        var_r11_4->unk6 = 0;
                        var_r11_4->unk8 = 0;
                        var_r11_4->unkA = 0;
                        var_r11_4->unkC = 0;
                        var_r11_4->unkE = 0;
                        var_r11_4->unk10 = 0;
                        var_r11_4->unk12 = 0;
                        var_r11_4->unk14 = 0;
                        var_r11_4->unk16 = 0;
                        var_r11_4->unk18 = 0;
                        var_r11_4->unk1A = 0;
                        var_r11_4->unk1C = 0;
                        var_r11_4->unk1E = 0;
                        var_r11_4->unk20 = 0;
                        var_r11_4->unk22 = 0;
                        var_r11_4->unk24 = 0;
                        var_r11_4->unk26 = 0;
                        var_r11_4->unk28 = 0;
                        var_r11_4->unk2A = 0;
                        var_r11_4->unk2C = 0;
                        var_r11_4->unk2E = 0;
                        var_r11_4->unk30 = 0;
                        var_r11_4->unk32 = 0;
                        var_r11_4->unk34 = 0;
                        var_r11_4->unk36 = 0;
                        var_r11_4->unk38 = 0;
                        var_r11_4->unk3A = 0;
                        var_r11_4->unk3C = 0;
                        var_r11_4->unk3E = 0;
                        var_r11_4->unk40 = 0;
                        var_r11_4->unk42 = 0;
                        var_r11_4->unk44 = 0;
                        var_r11_4->unk46 = 0;
                        var_r11_4->unk48 = 0;
                        var_r11_4->unk4A = 0;
                        var_r11_4->unk4C = 0;
                        var_r11_4->unk4E = 0;
                        var_r11_4 += 0x50;
                        var_ctr_7 -= 1;
                    } while (var_ctr_7 != 0);
                }
                var_r3_4 = var_r28 + (var_r10_4 * 2);
                var_ctr_8 = 0x7F - var_r10_4;
                if (var_r10_4 < 0x7F) {
                    do {
                        *var_r3_4 = 0;
                        var_r3_4 += 2;
                        var_ctr_8 -= 1;
                    } while (var_ctr_8 != 0);
                }
            } else {
                Fader_Step(&gCardUi + ((var_r20 + 0x23) * 0x10) + 0xA6C, 1);
            }
            CardUi_UpdateFormatMsg(temp_r3, var_r20, 1);
        }
        if ((s16) sCardUiState == 1) {
            if ((s8) *var_r27 > 0) {
                Fader_Step(temp_r26, 0);
            } else {
                Fader_Step(temp_r26, 1);
            }
            if ((s8) *var_r25 > (s8) *var_r27) {
                Fader_Step(temp_r24, 0);
            } else {
                Fader_Step(temp_r24, 1);
            }
        } else {
            Fader_Step(temp_r26, 1);
            Fader_Step(temp_r24, 1);
        }
        var_r30 += 0x14;
        var_r29 += 0x10;
        var_r28 += 0xFE;
        var_r27 += 1;
        var_r25 += 1;
        var_r20 += 1;
        goto loop_63;
    }
}

/* 0x813186F4 */
void CardUi_UpdateIconAnim(void) {
    void *var_r4;
    void *var_r7;
    s16 *temp_r9_2;
    s32 temp_r30;
    s32 temp_r9;
    s32 var_ctr;
    s32 var_r12;
    s32 var_r3;
    s32 var_r6;
    u8 *var_r5;
    u8 var_r11;
    void *temp_r8;

    temp_r30 = CardThread_GetIcons();
    var_r3 = CardThread_GetFiles();
    var_r7 = &lbl_814B6478;
    var_r4 = &lbl_814B6674;
    var_r6 = temp_r30;
    var_r11 = 0;
    var_r5 = &lbl_814AD45C;
loop_14:
    if (var_r11 < 2U) {
        var_ctr = 0x10;
        var_r12 = 0;
        do {
            temp_r9 = *(var_r4 + ((var_r12 + ((s8) *var_r5 * 4)) * 8));
            if ((u16) *(var_r3 + (temp_r9 * 0xC)) != 0) {
                temp_r8 = var_r6 + (temp_r9 << 6);
                temp_r9_2 = var_r7 + (temp_r9 * 2);
                *temp_r9_2 += (s8) temp_r8->unk4;
                if ((s16) *temp_r9_2 >= (s16) temp_r8->unk12) {
                    if ((u8) temp_r8->unk5 == 4) {
                        *temp_r9_2 = (temp_r8->unk12 - temp_r8->unk7) - 1;
                        temp_r8->unk4 = -1U;
                    } else {
                        *temp_r9_2 = 0;
                        temp_r8->unk4 = 1U;
                    }
                }
                if ((s16) *temp_r9_2 < 0) {
                    if ((u8) temp_r8->unk5 == 4) {
                        *temp_r9_2 = (s16) temp_r8->unk6;
                    } else {
                        *temp_r9_2 = 0;
                    }
                    temp_r8->unk4 = 1U;
                }
            }
            var_r12 += 1;
            var_ctr -= 1;
        } while (var_ctr != 0);
        var_r4 += 0x3F8;
        var_r5 += 1;
        var_r3 += 0x5F4;
        var_r6 += 0x1FC0;
        var_r7 += 0xFE;
        var_r11 += 1;
        goto loop_14;
    }
}

/* 0x81318858 */
void CardUi_CopyResultMsg(s32 arg0) {
    if (arg0 != 0) {
        if ((s32) sCardOpError == 0) {
            Fader_Step(&gCardUi + 0xB9C, 0);
        } else {
            Fader_Step(&gCardUi + 0xB9C, 1);
        }
        if ((s32) sCardOpError == -0x1A) {
            Fader_Step(&gCardUi + 0xBAC, 0);
        } else {
            Fader_Step(&gCardUi + 0xBAC, 1);
        }
        if ((s32) sCardOpError == -0x17) {
            Fader_Step(&gCardUi + ((0x16 - sCardOpSlot) * 0x10) + 0xA6C, 0);
        } else {
            Fader_Step(&gCardUi + ((0x16 - sCardOpSlot) * 0x10) + 0xA6C, 1);
        }
        if ((s32) sCardOpError == -0x16) {
            Fader_Step(&gCardUi + ((0x18 - sCardOpSlot) * 0x10) + 0xA6C, 0);
        } else {
            Fader_Step(&gCardUi + ((0x18 - sCardOpSlot) * 0x10) + 0xA6C, 1);
        }
        if ((s32) sCardOpError == -0x1B) {
            Fader_Step(&gCardUi + 0xC1C, 0);
        } else {
            Fader_Step(&gCardUi + 0xC1C, 1);
        }
        if ((s32) sCardOpError == -0x18) {
            Fader_Step(&gCardUi + 0xC2C, 0);
        } else {
            Fader_Step(&gCardUi + 0xC2C, 1);
        }
        if ((s32) sCardOpError == -0x19) {
            Fader_Step(&gCardUi + 0xC3C, 0);
        } else {
            Fader_Step(&gCardUi + 0xC3C, 1);
        }
        if ((s32) sCardOpError == -0x1C) {
            Fader_Step(&gCardUi + 0xC4C, 0);
            return;
        }
        Fader_Step(&gCardUi + 0xC4C, 1);
        return;
    }
    Fader_Step(&gCardUi + 0xB9C, 1);
    Fader_Step(&gCardUi + 0xBAC, 1);
    Fader_Step(&gCardUi + 0xBBC, 1);
    Fader_Step(&gCardUi + 0xBCC, 1);
    Fader_Step(&gCardUi + 0xBDC, 1);
    Fader_Step(&gCardUi + 0xBEC, 1);
    Fader_Step(&gCardUi + 0xC1C, 1);
    Fader_Step(&gCardUi + 0xC2C, 1);
    Fader_Step(&gCardUi + 0xC3C, 1);
    Fader_Step(&gCardUi + 0xC4C, 1);
}

/* 0x81318A88 */
void CardUi_MoveResultMsg(s32 arg0) {
    if (arg0 != 0) {
        if ((s32) sCardOpError == 0) {
            Fader_Step(&gCardUi + 0xADC, 0);
        } else {
            Fader_Step(&gCardUi + 0xADC, 1);
        }
        if ((s32) sCardOpError == -0x1A) {
            Fader_Step(&gCardUi + 0xAEC, 0);
        } else {
            Fader_Step(&gCardUi + 0xAEC, 1);
        }
        if ((s32) sCardOpError == -0x17) {
            Fader_Step(&gCardUi + ((0xA - sCardOpSlot) * 0x10) + 0xA6C, 0);
        } else {
            Fader_Step(&gCardUi + ((0xA - sCardOpSlot) * 0x10) + 0xA6C, 1);
        }
        if ((s32) sCardOpError == -0x16) {
            Fader_Step(&gCardUi + ((0xC - sCardOpSlot) * 0x10) + 0xA6C, 0);
        } else {
            Fader_Step(&gCardUi + ((0xC - sCardOpSlot) * 0x10) + 0xA6C, 1);
        }
        if ((s32) sCardOpError == -0x1B) {
            Fader_Step(&gCardUi + 0xB5C, 0);
        } else {
            Fader_Step(&gCardUi + 0xB5C, 1);
        }
        if ((s32) sCardOpError == -0x18) {
            Fader_Step(&gCardUi + 0xB6C, 0);
        } else {
            Fader_Step(&gCardUi + 0xB6C, 1);
        }
        if ((s32) sCardOpError == -0x19) {
            Fader_Step(&gCardUi + 0xB7C, 0);
        } else {
            Fader_Step(&gCardUi + 0xB7C, 1);
        }
        if ((s32) sCardOpError == -0x1C) {
            Fader_Step(&gCardUi + 0xB8C, 0);
            return;
        }
        Fader_Step(&gCardUi + 0xB8C, 1);
        return;
    }
    Fader_Step(&gCardUi + 0xADC, 1);
    Fader_Step(&gCardUi + 0xAEC, 1);
    Fader_Step(&gCardUi + 0xAFC, 1);
    Fader_Step(&gCardUi + 0xB0C, 1);
    Fader_Step(&gCardUi + 0xB1C, 1);
    Fader_Step(&gCardUi + 0xB2C, 1);
    Fader_Step(&gCardUi + 0xB5C, 1);
    Fader_Step(&gCardUi + 0xB6C, 1);
    Fader_Step(&gCardUi + 0xB7C, 1);
    Fader_Step(&gCardUi + 0xB8C, 1);
}

/* 0x81318CB8 */
void CardUi_UpdateResultMsg(void) {
    if ((s16) sCardUiState == 5) {
        if ((s32) sCardOpError != -0x15) {
            if (((s32) sCardOpKind == 2) || ((s32) sCardOpKind == 5)) {
                CardUi_CopyResultMsg(1);
            }
            if (((s32) sCardOpKind == 3) || ((s32) sCardOpKind == 6)) {
                CardUi_MoveResultMsg(1);
            }
            if (((s32) sCardOpKind == 4) || ((s32) sCardOpKind == 7)) {
                if ((s32) sCardOpError == 0) {
                    Fader_Step(&gCardUi + 0xC5C, 0);
                    Fader_Step(&gCardUi + 0xC6C, 1);
                } else {
                    Fader_Step(&gCardUi + 0xC5C, 1);
                    Fader_Step(&gCardUi + 0xC6C, 0);
                }
            }
            if (((s32) sCardOpKind == 1) || ((s32) sCardOpKind == 8)) {
                if ((s32) sCardOpError == 0) {
                    Fader_Step(&gCardUi + ((sCardOpSlot + 0x2B) * 0x10) + 0xA6C, 0);
                    Fader_Step(&gCardUi + ((sCardOpSlot + 0x2D) * 0x10) + 0xA6C, 1);
                    return;
                }
                Fader_Step(&gCardUi + ((sCardOpSlot + 0x2B) * 0x10) + 0xA6C, 1);
                Fader_Step(&gCardUi + ((sCardOpSlot + 0x2D) * 0x10) + 0xA6C, 0);
            }
        }
    } else {
        CardUi_CopyResultMsg(0);
        CardUi_MoveResultMsg(0);
        Fader_Step(&gCardUi + 0xC5C, 1);
        Fader_Step(&gCardUi + 0xC6C, 1);
        Fader_Step(&gCardUi + ((sCardOpSlot + 0x2B) * 0x10) + 0xA6C, 1);
        Fader_Step(&gCardUi + ((sCardOpSlot + 0x2D) * 0x10) + 0xA6C, 1);
    }
}

/* 0x81318E80 */
void CardUi_UpdateCursor(void) {
    s32 var_r27;
    s32 var_r28;
    void *temp_r3;
    void *var_r29;
    void *var_r30;

    temp_r3 = CardThread_GetSlots(&gCardUi);
    lbl_814AD4AA += lbl_814AD4AC;
    if ((s16) lbl_814AD4AA < 0) {
        lbl_814AD4AA = 0;
        lbl_814AD4AC = 1;
    }
    if ((s16) lbl_814AD4AA > (s16) lbl_814AD4AE) {
        lbl_814AD4AA = lbl_814AD4AE;
        lbl_814AD4AC = -1;
    }
    var_r28 = 0;
    var_r30 = temp_r3;
    var_r27 = 0;
    var_r29 = &gCardUi + 0xE3C;
    do {
        if ((var_r28 == (s32) sCardSlot) && ((s16) var_r30->unk2 == 4)) {
            Fader_Step(var_r29, 0);
            var_r27 += 1;
        } else {
            Fader_Step(var_r29, 1);
        }
        var_r28 += 1;
        var_r30 += 0x14;
        var_r29 += 0x10;
    } while (var_r28 < 2);
    if (var_r27 == 0) {
        Fader_Step(&gCardUi + 0xE5C, 0);
        return;
    }
    Fader_Step(&gCardUi + 0xE5C, 1);
}

/* 0x81318F80 */
void Card_Update(void) {
    void *temp_r3;

    CardUi_UpdateFaders(&gCardUi);
    CardUi_UpdateIconAnim();
    temp_r3 = CardThread_GetSlots() + (sCardSlot * 0x14);
    if (((s16) temp_r3->unk2 == 4) && ((s16) sCardUiState == 2)) {
        CardUi_StepSlotFader(&gCardUi + (sCardSlot << 5) + 0xDBC, 0U);
        CardUi_StepSlotFader(&gCardUi + ((sCardSlot ^ 1) << 5) + 0xDBC, 1U);
    } else if (((s16) temp_r3->unk2 == 4) && ((s16) sCardUiState == 3)) {
        CardUi_StepSlotFader(&gCardUi + (sCardSlot << 5) + 0xDBC, 2U);
        CardUi_StepSlotFader(&gCardUi + ((sCardSlot ^ 1) << 5) + 0xDBC, 1U);
    } else {
        CardUi_StepSlotFader(&gCardUi + 0xDBC, 1U);
        CardUi_StepSlotFader(&gCardUi + 0xDDC, 1U);
    }
    CardUi_UpdateResultMsg();
    lbl_814AD4A4 += lbl_814AD4A6;
    if ((s16) lbl_814AD4A4 < 0) {
        lbl_814AD4A4 = 0;
        lbl_814AD4A6 = 1;
    }
    if ((s16) lbl_814AD4A4 > (s16) lbl_814AD4A8) {
        lbl_814AD4A4 = lbl_814AD4A8;
        lbl_814AD4A6 = -1;
    }
    CardUi_UpdateCursor();
    CardUi_Update();
}

/* 0x813190C8 */
s32 Card_GetSelectedFile(void) {
    return *(&lbl_814B6674 + (sCardSlot * 0x3F8) + (((s8) *(&lbl_814AD454 + sCardSlot) + (((s8) *(&lbl_814AD458 + sCardSlot) + (s8) (&lbl_814AD45C)[sCardSlot]) * 4)) * 8));
}

/* 0x8131911C */
u8 Card_GetSlot(void) {
    return sCardSlot;
}

/* 0x81319124 */
void Card_Enter(void) {
    Fader_Reset(&gCardUi + 0xB9C);
    Fader_Reset(&gCardUi + 0xBAC);
    Fader_Reset(&gCardUi + 0xBBC);
    Fader_Reset(&gCardUi + 0xBCC);
    Fader_Reset(&gCardUi + 0xBDC);
    Fader_Reset(&gCardUi + 0xBEC);
    Fader_Reset(&gCardUi + 0xC1C);
    Fader_Reset(&gCardUi + 0xC2C);
    Fader_Reset(&gCardUi + 0xC3C);
    Fader_Reset(&gCardUi + 0xC4C);
    Fader_Reset(&gCardUi + 0xADC);
    Fader_Reset(&gCardUi + 0xAEC);
    Fader_Reset(&gCardUi + 0xAFC);
    Fader_Reset(&gCardUi + 0xB0C);
    Fader_Reset(&gCardUi + 0xB1C);
    Fader_Reset(&gCardUi + 0xB2C);
    Fader_Reset(&gCardUi + 0xB5C);
    Fader_Reset(&gCardUi + 0xB6C);
    Fader_Reset(&gCardUi + 0xB7C);
    Fader_Reset(&gCardUi + 0xB8C);
    Fader_Reset(&gCardUi + 0xC5C);
    Fader_Reset(&gCardUi + 0xC6C);
    Fader_Reset(&gCardUi + 0xD1C);
    Fader_Reset(&gCardUi + 0xD2C);
    Fader_Reset(&gCardUi + 0xD3C);
    Fader_Reset(&gCardUi + 0xD4C);
    Fader_Reset(&gCardUi + 0xC7C);
    Fader_Reset(&gCardUi + 0xC8C);
    Fader_Reset(&gCardUi + 0xC9C);
    Fader_Reset(&gCardUi + 0xCAC);
}

/* 0x81319240 */
void CardDraw_SetupTev(void) {
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(1, 9, 1, 3, 4);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(1, 0xD, 1, 4, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 0, 0, 2, 2);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOrder(1, 0xFF, 0xFF, 0xFF);
    GXSetTevOrder(2, 0xFF, 0xFF, 0xFF);
    GXSetTevOrder(3, 0xFF, 0xFF, 0xFF);
    GXSetTevOrder(4, 0xFF, 0xFF, 0xFF);
    GXSetTevOrder(5, 0xFF, 0xFF, 0xFF);
    GXSetTevOrder(6, 0xFF, 0xFF, 0xFF);
    GXSetTevOrder(7, 0xFF, 0xFF, 0xFF);
    GXSetTevColorIn(0, 0xF, 0xC, 0xA, 0xF);
    fn_81378EA0(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(0, 7, 4, 5, 7);
    fn_81378F60(0, 0, 0, 0, 1, 0);
    GXSetColorUpdate(1);
    fn_81379974(1);
    GXSetBlendMode(1, 4, 5, 0);
    GXSetZMode(0, 3, 0);
    GXSetCullMode(2);
}

/* 0x81319440 */
void CardDraw_LoadIconTex(u16 arg0, u16 arg1, s32 arg2) {
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f31;
    s16 temp_r0;
    s16 temp_r28;
    s16 temp_r29;
    s16 temp_r30;
    s16 temp_r8;
    u16 temp_r6;
    u32 temp_r5;

    temp_f1 = (f32) arg2;
    temp_r6 = lbl_814AD48C->unk2;
    temp_f31 = temp_f1 / 11.0f;
    temp_r5 = (temp_r6 * 0x10) / 11;
    temp_r30 = lbl_814AD48C->unk4 * 0x10;
    CardDraw_SetupTev(temp_r5, (u32) ((temp_r6 * 0x10) / 11) >> 0x1FU, lbl_814AD48C, temp_f1);
    temp_r28 = arg0 - (((temp_r6 * 0x10) / 22) + M2C_CARRY);
    temp_r29 = arg1 - (temp_r30 / 2);
    J3D_LoadTexture(0, lbl_814AD48C);
    GXBegin(0x80, 1, 4);
    *(s16 *)0xCC008000 = temp_r28;
    temp_r0 = temp_r28 + (s16) temp_r5;
    temp_f1_2 = temp_f31 + 0.09090909f;
    *(s16 *)0xCC008000 = temp_r29;
    *(s16 *)0xCC008000 = 0;
    temp_r8 = temp_r29 + temp_r30;
    *(s16 *)0xCC008000 = temp_f31;
    *(s16 *)0xCC008000 = 0.0f;
    *(s16 *)0xCC008000 = temp_r0;
    *(s16 *)0xCC008000 = temp_r29;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_f1_2;
    *(s16 *)0xCC008000 = 0.0f;
    *(s16 *)0xCC008000 = temp_r0;
    *(s16 *)0xCC008000 = temp_r8;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_f1_2;
    *(s16 *)0xCC008000 = 1.0f;
    *(s16 *)0xCC008000 = temp_r28;
    *(s16 *)0xCC008000 = temp_r8;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_f31;
    *(s16 *)0xCC008000 = 1.0f;
}

/* 0x8131959C */
void CardDraw_LoadTexture(u16 arg0, u16 arg1, s32 arg2) {
    s32 temp_r6;
    s32 temp_r6_2;
    u16 temp_r27;
    u16 temp_r27_2;
    u16 temp_r27_3;
    u16 temp_r27_4;
    u16 temp_r27_5;
    u16 temp_r27_6;
    u32 temp_r30;

    temp_r30 = (lbl_814AD48C->unk2 * 0x10) / 11;
    if (arg2 < 0xA) {
        CardDraw_LoadIconTex(arg0, arg1, arg2 % 10);
        return;
    }
    if (arg2 < 0x64) {
        temp_r6 = (s32) (temp_r30 + 0x20) / 2;
        temp_r27 = arg0 - temp_r6;
        CardDraw_LoadIconTex(temp_r27, arg1, (arg2 / 10) % 10, temp_r6);
        CardDraw_LoadIconTex(temp_r30 + temp_r27 + 0x20, arg1, arg2 % 10);
        return;
    }
    if (arg2 < 0x3E8) {
        temp_r27_2 = arg0 - (temp_r30 + 0x20);
        CardDraw_LoadIconTex(temp_r27_2, arg1, (arg2 / 100) % 10);
        temp_r27_3 = temp_r30 + temp_r27_2 + 0x20;
        CardDraw_LoadIconTex(temp_r27_3, arg1, (arg2 / 10) % 10);
        CardDraw_LoadIconTex(temp_r30 + temp_r27_3 + 0x20, arg1, arg2 % 10);
        return;
    }
    temp_r6_2 = (s32) ((temp_r30 + 0x20) * 3) / 2;
    temp_r27_4 = arg0 - temp_r6_2;
    CardDraw_LoadIconTex(temp_r27_4, arg1, (arg2 / 1000) % 10, temp_r6_2);
    temp_r27_5 = temp_r30 + temp_r27_4 + 0x20;
    CardDraw_LoadIconTex(temp_r27_5, arg1, (arg2 / 100) % 10);
    temp_r27_6 = temp_r30 + temp_r27_5 + 0x20;
    CardDraw_LoadIconTex(temp_r27_6, arg1, (arg2 / 10) % 10);
    CardDraw_LoadIconTex(temp_r30 + temp_r27_6 + 0x20, arg1, arg2 % 10);
}

/* 0x81319864 */
s32 CardDraw_SetIconFrame(s32 arg0, s32 arg1, u8 arg2) {
    s32 sp30;
    s16 sp2E;
    s8 sp2B;
    s8 sp2A;
    s8 sp29;
    s8 sp28;
    s8 sp27;
    s8 sp26;
    s8 sp25;
    s8 sp24;
    s32 sp20;
    s16 sp1E;
    s8 sp1D;
    s8 sp1C;
    s8 sp1B;
    s8 sp1A;
    s16 sp18;
    s16 sp16;
    u8 sp14;
    s32 temp_r3;
    s32 temp_r5;
    s32 temp_r5_2;
    s32 temp_r7;
    s32 temp_r7_2;
    s32 temp_r8;
    void *temp_r4;
    void *temp_r6;

    temp_r3 = CardThread_GetIcons();
    temp_r7 = arg0 * 0x1FC0;
    temp_r5 = arg1 << 6;
    temp_r6 = temp_r3 + temp_r7 + temp_r5;
    temp_r7_2 = temp_r3 + (temp_r7 + temp_r5);
    temp_r8 = temp_r7_2 + (temp_r6 + (arg2 * 4))->unk1C;
    if ((u8) temp_r6->unk1 == 0) {
        return 0;
    }
    sp24 = 0;
    temp_r4 = temp_r6 + arg2;
    sp25 = 0;
    sp26 = 0;
    sp27 = 0;
    sp16 = 0x20;
    sp18 = 0x20;
    sp1A = 0;
    sp1B = 0;
    sp14 = temp_r4->unk8;
    sp28 = 1;
    sp29 = 1;
    sp2A = 0;
    sp2B = 0;
    sp2E = 0;
    switch ((u8) temp_r4->unk8) {                   /* irregular */
    case 5:
        sp1C = 0;
        sp30 = temp_r8 - &sp14;
        J3D_LoadTexture(0, &sp14);
        return 1;
    case 9:
        sp1C = 1;
        sp1D = 2;
        temp_r5_2 = (temp_r7_2 + temp_r6->unk3C) - &sp14;
        sp1E = 0x100;
        sp20 = temp_r5_2;
        sp30 = temp_r8 - &sp14;
        J3D_LoadTexture(0, &sp14, temp_r5_2);
        return 1;
    default:
        return 0;
    }
}

/* 0x813199AC */
s32 CardDraw_IconFrameCount(s32 arg0, s32 arg1) {
    s16 var_r7;
    s32 temp_r3;
    s32 var_r3;
    s32 var_r8;

    temp_r3 = CardThread_GetIcons();
    var_r7 = 0;
    var_r8 = 0;
    if ((arg0 != 0) && (arg0 != 1)) {
        return 0;
    }
    if ((arg1 < 0) || (arg1 > 0x7F)) {
        return 0;
    }
    var_r3 = 0;
loop_7:
    var_r7 += 1;
    var_r8 += (((s32) (temp_r3 + ((arg0 * 0x1FC0) + (arg1 << 6)))->unk10 >> var_r3) * 4) & 0xC;
    var_r3 += 2;
    if ((s16) *(&lbl_814B6478 + (arg0 * 0xFE) + (arg1 * 2)) > var_r8) {
        if (var_r7 <= 8) {
            goto loop_7;
        }
    }
    return CardDraw_SetIconFrame(arg0, arg1, (u8) (var_r7 - 1));
}

/* 0x81319A8C */
void CardDraw_Icon(u8 arg0, s16 arg1, u8 arg2, u8 arg3) {
    s32 sp18;
    s32 sp14;
    s32 temp_r27;
    s32 temp_r3;
    s32 temp_r9;
    u32 temp_r28;
    u8 temp_r6;
    u8 temp_r7;

    temp_r3 = CardThread_GetIcons();
    sp14 = lbl_814ADF10;
    unksp17 = arg3;
    sp18 = lbl_814ADF10;
    temp_r6 = *(&lbl_814AD458 + sCardSlot);
    temp_r9 = sCardSlot * 0x3F8;
    temp_r7 = *(&lbl_814AD454 + sCardSlot);
    temp_r27 = *(&lbl_814B6674 + (arg0 * 0x3F8) + ((arg1 + ((s8) (&lbl_814AD45C)[arg0] * 4)) * 8));
    temp_r28 = *(&lbl_814B6674 + temp_r9 + (((s8) temp_r7 + (((s8) temp_r6 + (s8) (&lbl_814AD45C)[sCardSlot]) * 4)) * 8));
    if (CardDraw_SetIconFrame((s32) arg0, temp_r27, arg2, temp_r6, temp_r7, &lbl_814B6674, temp_r9, sCardSlot) != 0) {
        GX_Setup2D(1, 0, 1);
        if ((temp_r28 == (u32) temp_r27) && (arg0 == (u8) sCardSlot) && ((u8) *(temp_r3 + ((arg0 * 0x1FC0) + (temp_r27 << 6))) == 0)) {
            Glh_DrawPaneTex('ic00', sCardGlh, &sp18, 0);
        }
    }
}

/* 0x81319BC4 */
void CardDraw_SlotHeader(u8 arg0, void *arg1, s32 *arg2) {
    s16 sp1A;
    s32 sp14;
    s16 temp_r6;
    s16 temp_r6_2;
    s32 temp_r0;
    void *temp_r3;

    temp_r3 = Card_GetParams();
    if (temp_r3->unk6C & 1) {
        Fader_GetAlpha(&lbl_814B7234 + ((arg0 * 0x10) & 0xFF0), &sp1A, NULL);
        temp_r6 = temp_r3->unk6E;
        arg2->unk3 = (u8) ((arg2->unk3 * (u8) ((0xFF - temp_r6) + ((temp_r6 * sp1A) / 255))) / 255);
    }
    if ((temp_r3->unk6C & 2) && (arg0 == (u8) sCardSlot)) {
        temp_r6_2 = temp_r3->unk6E;
        arg2->unk3 = (u8) ((arg2->unk3 * (u8) ((0xFF - temp_r6_2) + ((s32) (temp_r6_2 * lbl_814AD4AA) / (s16) lbl_814AD4AE))) / 255);
    }
    Glh_DrawPane(arg0 + 'texa', sCardGlh, arg2);
    sp14 = arg2->unk0;
    fn_81377704(4, &sp14);
    temp_r0 = arg0 * 2;
    CardDraw_LoadTexture(*(&lbl_814AD490 + temp_r0), *(&lbl_814AD498 + temp_r0), (s32) (arg1 + (arg0 * 0x14))->unk10);
    Glh_DrawFrame(arg0 + 'ffra', sCardGlh, arg2);
    GX_Setup2DText();
    Glh_DrawText(arg0, sCardSth, sCardGlh, arg2);
}

/* 0x81319D70 */
void CardDraw_FileBanner(u8 arg0, void *arg1, s32 *arg2) {
    s16 sp20;
    s32 sp1C;
    s16 sp1A;
    s16 sp18;
    s32 temp_r25;
    u8 temp_r26;
    u8 temp_r27;
    void *temp_r31;

    GX_Setup2DTex(&gCardUi);
    temp_r25 = (arg0 * 0x10) & 0xFF0;
    Fader_GetAlpha(&gCardUi + temp_r25 + 0xD5C, &sp20, NULL);
    temp_r26 = arg2->unk3;
    arg2->unk3 = (u8) ((temp_r26 * sp20) / 255);
    sp1C = arg2->unk0;
    CardDraw_SlotHeader(arg0, arg1, &sp1C);
    GX_Setup2DTex();
    temp_r27 = arg2->unk3;
    Fader_GetAlpha(&gCardUi + temp_r25 + 0xDFC, &sp18, NULL);
    Fader_GetAlpha(&gCardUi + temp_r25 + 0xE1C, &sp1A, NULL);
    arg2->unk3 = (u8) ((temp_r27 * sp18) / 255);
    temp_r31 = &gCardUi + temp_r25;
    Glh_DrawPaneOfs(temp_r31->unkE08, sCardGlh, arg2, 0, (s32) Hermite((f32) lbl_814AD4A4, 0.0f, 0.0f, 0.0f, (f32) lbl_814AD4A8, 128.0f, 0.0f));
    arg2->unk3 = (u8) ((temp_r27 * sp1A) / 255);
    Glh_DrawPaneOfs(temp_r31->unkE28, sCardGlh, arg2, 0, -(s32) Hermite((f32) lbl_814AD4A4, 0.0f, 0.0f, 0.0f, (f32) lbl_814AD4A8, 128.0f, 0.0f));
    arg2->unk3 = temp_r27;
    if (arg0 == (u8) sCardSlot) {
        GX_Setup2D(1, 0, 1);
        if ((s16) (arg1 + (sCardSlot * 0x14))->unk2 == 4) {
            if (CardDraw_LoadFileIcon() != 0U) {
                Glh_DrawPaneTex('bana', sCardGlh, arg2, 0);
            }
            CardDraw_FileInfo(arg2->unk3);
        }
    }
    arg2->unk3 = temp_r26;
}

/* 0x81319FC8 */
void CardDraw_FileGrid(u8 arg0, u8 arg1) {
    s16 sp10;
    s16 var_r31;
    s16 var_r7;
    s32 temp_r0;
    s32 temp_r31;
    s32 var_r5;
    s32 var_r6;
    u8 *temp_r28;
    void *temp_r29;
    void *temp_r4;

    temp_r31 = CardThread_GetFiles(&gCardUi);
    Fader_GetAlpha(&gCardUi + ((arg0 * 0x10) & 0xFF0) + 0xD5C, &sp10, NULL);
    GX_Setup2DTex((void *) ((u8) ((arg1 * sp10) / 255) >> 0x1FU), arg1);
    temp_r28 = &(&lbl_814AD45C)[arg0];
    var_r31 = 0;
    temp_r29 = &gCardUi + (arg0 * 0x3F8) + 0x27C;
loop_7:
    if (var_r31 < 0x10) {
        if ((u16) *(temp_r31 + (arg0 * 0x5F4) + (*(temp_r29 + ((var_r31 + ((s8) *temp_r28 * 4)) * 8)) * 0xC)) != 0) {
            var_r7 = 0;
            var_r5 = 0;
            temp_r0 = *(temp_r29 + ((var_r31 + ((s8) *temp_r28 * 4)) * 8));
            var_r6 = 0;
            temp_r4 = CardThread_GetIcons() + ((arg0 * 0x1FC0) + (temp_r0 << 6));
loop_3:
            var_r7 += 1;
            var_r5 += (((s32) temp_r4->unk10 >> var_r6) * 4) & 0xC;
            var_r6 += 2;
            if ((s16) *(&gCardUi + (arg0 * 0xFE) + 0x80 + (temp_r0 * 2)) > var_r5) {
                if (var_r7 <= 8) {
                    goto loop_3;
                }
            }
            CardDraw_Icon(arg0, var_r31, var_r7 - 1, (u8) ((arg1 * sp10) / 255));
        }
        var_r31 += 1;
        goto loop_7;
    }
}

/* 0x8131A13C */
void CardDraw_OpMenu(u16 arg0, u8 arg1, u8 arg2) {
    u16 sp4E;
    u16 sp4C;
    u16 sp4A;
    s16 sp48;
    u8 sp44[0x4];
    s32 sp40;
    s32 sp3C;                                       /* compiler-managed */
    void *sp38;
    s32 sp34;
    s32 sp30;
    s32 sp2C;
    s32 sp28;
    s32 sp24;
    s32 sp20;
    u8 sp1F;
    s8 sp1E;
    s8 sp1D;
    s8 sp1C;
    u8 sp1B;
    s8 sp1A;
    s8 sp19;
    s8 sp18;
    u16 sp14;
    void *temp_r5_3;
    s16 *temp_r18;
    s16 temp_r3_3;
    s16 temp_r4;
    s16 temp_r4_2;
    s16 temp_r7;
    s32 temp_r0_3;
    s32 temp_r0_4;
    s32 temp_r30;
    s32 temp_r4_3;
    s32 temp_r5;
    s32 temp_r6_2;
    s32 var_r0;
    s32 var_r3;
    s32 var_r4;
    s32 var_r4_2;
    s32 var_r5;
    s32 var_r6;
    u16 temp_r0_2;
    u16 temp_r0_5;
    u16 temp_r16_2;
    u16 temp_r17;
    u16 temp_r3_2;
    u16 temp_r3_5;
    u16 temp_r5_2;
    u16 var_r19;
    u16 var_r19_2;
    u16 var_r20;
    u8 *temp_r16;
    u8 *temp_r21;
    u8 *temp_r22;
    u8 *temp_r26;
    u8 *temp_r27;
    u8 *var_r17;
    u8 *var_r18;
    u8 temp_r0;
    u8 temp_r9;
    void *temp_r24;
    void *temp_r29;
    void *temp_r31;
    void *temp_r3;
    void *temp_r3_4;
    void *temp_r6;

    temp_r24 = Disp_GetSettings(&gCardUi);
    temp_r3 = Card_GetParams();
    var_r19 = 0;
    sp4E = 0;
    temp_r26 = sCardSth + 8;
    sp4C = 0;
    sp4A = 0;
    temp_r5 = sCardGlh->unk8;
    temp_r27 = &sCardGlh[temp_r5];
    sp1C = (s8) temp_r3->unk50;
    sp1D = (s8) temp_r3->unk52;
    sp1E = (s8) temp_r3->unk54;
    sp1F = 0xFF;
    sp18 = (s8) temp_r3->unk56;
    sp19 = (s8) temp_r3->unk58;
    sp1A = (s8) temp_r3->unk5A;
    sp1B = 0xFF;
    GX_Setup2DView(0xFF, temp_r5, sCardGlh);
    temp_r17 = arg0 + arg1;
    temp_r30 = temp_r17 * 0x10;
    temp_r29 = &gCardUi + temp_r30;
    Fader_GetAlpha(temp_r29 + 0xA6C, &sp48, &sp44);
    temp_r0 = (arg2 * sp48) / 255;
    sp1F = temp_r0;
    sp1B = temp_r0;
    GX_Setup2DText((u8) ((arg2 * sp48) / 255) >> 0x1FU);
    temp_r31 = temp_r26 + (temp_r17 * 8);
    Glh_MeasureString(temp_r27 + (temp_r31->unk0 * 0x24), temp_r31 + temp_r31->unk4, &sp4E, &sp4C);
    var_r20 = 0;
    temp_r22 = sCardSth + 8;
    sp14 = 0;
    var_r18 = temp_r22;
    temp_r21 = &sCardGlh[sCardGlh->unk8];
    temp_r16 = &temp_r22[(temp_r17 * 8) & 0x7FFF8];
loop_4:
    if (var_r20 < 2U) {
        Glh_MeasureString(temp_r21 + (*temp_r16 * 0x24), &temp_r22[((var_r20 + 5) * 8) + var_r18->unk2C], &sp14, &sp4A);
        if (var_r19 < sp14) {
            var_r19 = sp14;
        }
        var_r18 += 8;
        var_r20 += 1;
        goto loop_4;
    }
    temp_r3_2 = sp4C;
    temp_r4 = (temp_r21 + (*temp_r16 * 0x24))->unk18;
    sp4C = temp_r3_2 + (temp_r24->unk2B << 5);
    temp_r16_2 = sp4A + (temp_r4 * 0x10) + (temp_r24->unk2B << 5);
    temp_r0_2 = sp4C + (temp_r16_2 + (Card_GetParams(temp_r3_2, temp_r4, sp4A)->unk16 * 0x10));
    Glh_DrawStringOfs(temp_r17, sCardSth, sCardGlh, &sp1C, 0, (s16) (((s32) -(s32) temp_r0_2 / 2) + ((s32) sp4C / 2)));
    temp_r4_2 = temp_r29->unkA6C;
    temp_r3_3 = (&gCardUi + temp_r30)->unkA6E;
    temp_r0_3 = temp_r3_3 / 2;
    if (temp_r4_2 > temp_r0_3) {
        temp_r0_4 = (s32) (arg2 * (temp_r4_2 - temp_r0_3) * 2) / temp_r3_3;
        sp1F = (u8) temp_r0_4;
        sp1B = (u8) temp_r0_4;
    } else {
        sp1F = 0;
        sp1B = 0;
    }
    temp_r9 = temp_r24->unk2B;
    temp_r3_4 = temp_r27 + (temp_r31->unk0 * 0x24);
    temp_r5_2 = temp_r3_4->unk4 - (((s32) var_r19 / 2) + (temp_r9 * 0x10));
    temp_r0_5 = (((s32) temp_r0_2 / 2) + temp_r3_4->unk6) - temp_r16_2;
    Glh_DrawPaneRect(temp_r3_4->unk1C, sCardGlh, &sp1C, temp_r5_2, temp_r0_5, var_r19 + (temp_r9 << 5), temp_r16_2);
    CardDraw_SetScissor(temp_r5_2, temp_r0_5, var_r19 + (temp_r24->unk2B << 5), temp_r16_2, (u8) ((&gCardUi + temp_r30)->unkA72 * 5), &sp1C);
    temp_r6 = temp_r27 + (temp_r31->unk0 * 0x24);
    temp_r4_3 = (s32) sp4A / 2;
    sp20 = temp_r6->unk0;
    sp24 = temp_r6->unk4;
    sp28 = temp_r6->unk8;
    sp2C = temp_r6->unkC;
    sp30 = temp_r6->unk10;
    sp34 = temp_r6->unk14;
    temp_r5_3 = temp_r6->unk18;
    sp38 = temp_r5_3;
    sp3C = temp_r6->unk1C;
    sp40 = temp_r6->unk20;
    temp_r3_5 = temp_r0_5 + ((temp_r24->unk2B * 0x10) + temp_r4_3);
    unksp26 = temp_r3_5;
    sp3C = 0xFFFF;
    GX_Setup2DText((u32) temp_r3_5, temp_r4_3, temp_r5_3, temp_r6);
    var_r17 = temp_r26;
    temp_r18 = &lbl_814AD478 + ((arg1 * 2) & 0x1FE);
    var_r19_2 = 0;
loop_31:
    if (var_r19_2 < 2U) {
        fn_813095A0(&sp18, &sp18);
        fn_81309554(&sp1C, &sp1C);
        if (arg0 == 0x27) {
            temp_r7 = *temp_r18;
            var_r4 = 0;
            var_r3 = 0;
            temp_r6_2 = temp_r7 & 2;
            var_r5 = 1;
            if ((temp_r6_2 == 2) && (var_r19_2 == 0)) {
                var_r3 = 1;
            }
            if (var_r3 == 0) {
                var_r6 = 0;
                if ((temp_r6_2 != 2) && ((s32) var_r19_2 == (s32) (temp_r7 & 1))) {
                    var_r6 = 1;
                }
                if (var_r6 == 0) {
                    var_r5 = 0;
                }
            }
            if ((var_r5 != 0) && ((u8) sCardSlot == arg1)) {
                var_r4 = 1;
            }
            var_r0 = var_r4;
        } else {
            var_r4_2 = 0;
            if (((s32) var_r19_2 == (s32) (*temp_r18 & 1)) && ((u8) sCardSlot == arg1)) {
                var_r4_2 = 1;
            }
            var_r0 = var_r4_2;
        }
        if (var_r0 != 0) {
            fn_813090A4(&sp20, &temp_r26[((var_r19_2 + 5) * 8) + var_r17->unk2C]);
        } else {
            fn_81308DB8(&sp20, &temp_r26[((var_r19_2 + 5) * 8) + var_r17->unk2C]);
        }
        var_r17 += 8;
        var_r19_2 += 1;
        unksp26 += (s16) sp38 * 0x10;
        goto loop_31;
    }
}

/* 0x8131A644 */
void fn_8131A644(void *arg0) {
    arg0->unk0 = 0;
    arg0->unk2 = 0x14;
    arg0->unk4 = 0;
    arg0->unk6 = 0x1E;
    arg0->unk8 = 0xA;
    arg0->unkA = 0x14;
    arg0->unkF = 0xFF;
    arg0->unk10 = 0x28;
    arg0->unk12 = 0x14;
    arg0->unk14 = 0;
}

/* 0x8131A688 */
void CardUi_StepSlotFader(void *arg0, u8 arg1) {
    s16 temp_r3;
    s16 temp_r3_2;
    s16 temp_r3_3;

    switch ((s32) arg1) {                           /* irregular */
    case 0:
        arg0->unk0 = (s16) (arg0->unk0 + 1);
        temp_r3 = arg0->unk2;
        if ((s16) arg0->unk0 > temp_r3) {
            arg0->unk0 = temp_r3;
        }
        arg0->unk4 = (s16) (arg0->unk4 - 1);
        if ((s16) arg0->unk4 < 0) {
            arg0->unk4 = 0;
        }
        break;
    case 2:
        arg0->unk0 = (s16) (arg0->unk0 + 1);
        temp_r3_2 = arg0->unk2;
        if ((s16) arg0->unk0 > temp_r3_2) {
            arg0->unk0 = temp_r3_2;
        }
        arg0->unk4 = (s16) (arg0->unk4 + 1);
        temp_r3_3 = arg0->unk6;
        if ((s16) arg0->unk4 > temp_r3_3) {
            arg0->unk4 = temp_r3_3;
        }
        break;
    case 1:
        arg0->unk0 = (s16) (arg0->unk0 - 1);
        if ((s16) arg0->unk0 < 0) {
            arg0->unk0 = 0;
        }
        arg0->unk4 = (s16) (arg0->unk4 - 1);
        if ((s16) arg0->unk4 < 0) {
            arg0->unk4 = 0;
        }
        break;
    }
    arg0->unkC = (u16) (arg0->unkC + Card_GetParams()->unk14);
}

/* 0x8131A7B4 */
void CardDraw_SetScissor(u16 arg0, u16 arg1, s32 arg2, u16 arg3, u8 arg4, s8 *arg5) {
    s16 temp_r0_2;
    s16 temp_r27;
    s16 temp_r28;
    s16 temp_r3_2;
    s32 temp_r0;
    s32 temp_r31;
    s32 temp_r3;
    s32 var_r4;

    fn_8130AB14(0xC0C10000);
    var_r4 = (s32) ((0x100 - arg4) * 0xFF) / 256;
    temp_r0 = arg4 + 0x10;
    temp_r3 = (arg4 * 2) + 0x20;
    temp_r27 = arg0 - temp_r0;
    temp_r28 = arg1 - temp_r0;
    if ((u32) var_r4 > 0xFFU) {
        var_r4 = 0xFF;
    }
    temp_r31 = (u8) ((var_r4 * arg5->unk3) / 255) + 0xC0C0C000;
    GXSetLineWidth(0x12, 0);
    GXBegin(0xA8, 1, 8);
    *(s16 *)0xCC008000 = temp_r27;
    temp_r3_2 = temp_r27 + (arg2 + temp_r3);
    *(s16 *)0xCC008000 = temp_r28;
    temp_r0_2 = temp_r28 + (arg3 + temp_r3);
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
    *(s16 *)0xCC008000 = temp_r3_2;
    *(s16 *)0xCC008000 = temp_r28;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
    *(s16 *)0xCC008000 = temp_r27;
    *(s16 *)0xCC008000 = temp_r28;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
    *(s16 *)0xCC008000 = temp_r27;
    *(s16 *)0xCC008000 = temp_r0_2;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
    *(s16 *)0xCC008000 = temp_r27;
    *(s16 *)0xCC008000 = temp_r0_2;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
    *(s16 *)0xCC008000 = temp_r3_2;
    *(s16 *)0xCC008000 = temp_r0_2;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
    *(s16 *)0xCC008000 = temp_r3_2;
    *(s16 *)0xCC008000 = temp_r28;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
    *(s16 *)0xCC008000 = temp_r3_2;
    *(s16 *)0xCC008000 = temp_r0_2;
    *(s16 *)0xCC008000 = 0;
    *(s16 *)0xCC008000 = temp_r31;
}

/* 0x8131A90C */
void CardDraw_MeasureLines(void *arg0, u8 *arg1, u8 *arg2, u16 *arg3) {
    u16 sp18;
    s16 temp_r0;
    s16 temp_r4_2;
    s16 temp_r5;
    s32 temp_cr0_lt;
    s32 var_r24;
    u16 var_r29;
    u16 var_r30;
    u16 var_r31;
    void *temp_r27;
    void *temp_r28;
    void *temp_r4;

    var_r29 = saved_reg_r29;
    temp_r27 = arg1 + 8;
    var_r24 = 0x10;
    var_r31 = 0;
    var_r30 = 0;
    temp_r28 = arg2 + arg2->unk8;
loop_4:
    if (var_r30 < 3U) {
        temp_r4 = temp_r27 + ((var_r30 + 2) * 8);
        Glh_MeasureString(temp_r28 + (*(temp_r27 + var_r24) * 0x24), temp_r4 + temp_r4->unk4, &sp18, arg0 + 0x18);
        if (var_r31 < sp18) {
            var_r31 = sp18;
        }
        var_r24 += 8;
        var_r30 += 1;
        goto loop_4;
    }
    arg0->unk1E = (s16) (arg0->unk18 + ((temp_r28 + (temp_r27->unk10 * 0x24))->unk18 << 5));
    temp_r4_2 = arg0->unk4;
    temp_r0 = arg0->unk8;
    temp_cr0_lt = temp_r4_2 < temp_r0;
    if (temp_cr0_lt != 0) {
        var_r29 = ((temp_r28 + (temp_r27->unk10 * 0x24))->unk18 << 5) & 0xFFE0;
    } else if ((temp_cr0_lt == 0) && (temp_r5 = arg0->unkA, ((temp_r4_2 < temp_r5) != 0))) {
        var_r29 = (u16) ((s32) ((temp_r5 - temp_r4_2) * ((temp_r28 + (temp_r27->unk10 * 0x24))->unk18 << 5)) / (s32) (temp_r5 - temp_r0));
    } else if (temp_r4_2 >= (s16) arg0->unkA) {
        var_r29 = 0;
    }
    arg0->unk1A = (s16) (arg0->unk18 + var_r29);
    arg0->unk1C = (s16) (arg0->unk18 + ((temp_r28 + (temp_r27->unk10 * 0x24))->unk18 * 0x10));
    *arg3 = var_r31;
}

/* 0x8131AA74 */
void CardDraw_TextBox(u8 arg0, void *arg1, u8 *arg2, u8 *arg3, s8 *arg4, s16 arg5, s32 arg6, u16 arg7) {
    s8 sp63;
    s8 sp62;
    s8 sp61;
    s8 sp60;
    u8 sp5F;
    s8 sp5E;
    s8 sp5D;
    s8 sp5C;
    s32 sp58;
    s32 sp54;                                       /* compiler-managed */
    s16 sp50;
    s16 sp4E;
    s16 sp4C;
    s32 sp48;
    s32 sp44;
    s32 sp40;
    s32 sp3C;
    s32 sp38;
    s32 sp34;
    s32 sp30;                                       /* compiler-managed */
    s32 sp2C;                                       /* compiler-managed */
    s32 sp28;
    s16 *var_r29;
    s16 temp_r0;
    s16 temp_r0_2;
    s16 temp_r3_2;
    s16 temp_r4_2;
    s16 temp_r4_3;
    s16 temp_r5;
    s16 temp_r5_2;
    s16 temp_r5_3;
    s16 var_r0;
    s32 temp_cr0_lt;
    s32 temp_r23;
    s32 temp_r3_4;
    s32 temp_r3_5;
    s32 temp_r3_6;
    s32 var_r23;
    u16 var_r26;
    u16 var_r27;
    u8 temp_r30;
    void *temp_r28;
    void *temp_r3;
    void *temp_r3_3;
    void *temp_r4;
    void *temp_r4_4;
    void *temp_r4_5;
    void *temp_r4_6;
    void *temp_r4_7;
    void *temp_r4_8;
    void *temp_r4_9;

    var_r26 = saved_reg_r26;
    temp_r3 = Card_GetParams();
    temp_r28 = arg2 + 8;
    sp60 = (s8) temp_r3->unk50;
    sp61 = (s8) temp_r3->unk52;
    sp62 = (s8) temp_r3->unk54;
    sp63 = 0xFF;
    sp5C = (s8) temp_r3->unk56;
    sp5D = (s8) temp_r3->unk58;
    sp5E = (s8) temp_r3->unk5A;
    sp5F = 0xFF;
    temp_r4 = arg3 + arg3->unk8 + (temp_r28->unk10 * 0x24);
    sp28 = temp_r4->unk0;
    sp2C = temp_r4->unk4;
    sp30 = temp_r4->unk8;
    sp34 = temp_r4->unkC;
    sp38 = temp_r4->unk10;
    sp3C = temp_r4->unk14;
    sp40 = temp_r4->unk18;
    sp44 = temp_r4->unk1C;
    sp48 = temp_r4->unk20;
    sp2C = arg5;
    unksp2E = arg6 + ((s32) arg1->unk18 / 2);
    sp30 = arg7;
    unksp32 = arg1->unk18;
    temp_r0 = arg1->unk4;
    temp_r4_2 = arg1->unk8;
    if (temp_r0 < temp_r4_2) {
        var_r0 = (s16) ((s32) (arg4->unk3 * (temp_r4_2 - temp_r0)) / temp_r4_2);
    } else {
        var_r0 = 0;
    }
    sp63 = (s8) var_r0;
    sp5F = arg4->unk3;
    GX_Setup2DText();
    temp_r3_2 = arg1->unk4;
    temp_r0_2 = arg1->unk8;
    temp_cr0_lt = temp_r3_2 < temp_r0_2;
    if (temp_cr0_lt != 0) {
        var_r26 = ((s16) sp40 * 0x10) & 0xFFF0;
    } else if ((temp_cr0_lt == 0) && (temp_r5 = arg1->unkA, ((temp_r3_2 < temp_r5) != 0))) {
        var_r26 = (u16) ((s32) ((s16) sp40 * 0x10 * (temp_r5 - temp_r3_2)) / (s32) (temp_r5 - temp_r0_2));
    } else if (temp_r3_2 >= (s16) arg1->unkA) {
        var_r26 = 0;
    }
    Fader_GetAlpha(&gCardUi + ((arg0 ^ 1) * 0x10) + 0xD5C, &sp50, NULL);
    temp_r23 = arg0 * 0x10;
    Fader_GetAlpha(&gCardUi + temp_r23 + 0xD7C, &sp4E, NULL);
    Fader_GetAlpha(&gCardUi + temp_r23 + 0xD9C, &sp4C, NULL);
    sp50 = (s16) (sp50 / 2);
    var_r29 = &sp4C;
    var_r23 = 0x10;
    var_r27 = 0;
    sp4E = (s16) (sp4E / 2);
    sp4C = (s16) (sp4C / 2);
loop_23:
    if (var_r27 < 3U) {
        if (var_r27 <= 1U) {
            sp58 = (s32) sp5C;
            sp54 = (s32) sp60;
            temp_r30 = (u8) *var_r29;
            temp_r3_3 = Card_GetParams((u16) (s32) sp5C);
            temp_r5_2 = temp_r3_3->unk5C;
            sp54 = (s8) ((s32) ((u8) sp54 * (temp_r5_2 + temp_r30)) / (s32) (temp_r5_2 + 0x7F));
            temp_r5_3 = temp_r3_3->unk5C;
            unksp55 = (s8) ((s32) (unksp55 * (temp_r5_3 + temp_r30)) / (s32) (temp_r5_3 + 0x7F));
            temp_r4_3 = temp_r3_3->unk5C;
            unksp56 = (s8) ((s32) (unksp56 * (temp_r4_3 + temp_r30)) / (s32) (temp_r4_3 + 0x7F));
            if ((s16) sCardOp == (s32) var_r27) {
                if ((s16) sCardUiState == 2) {
                    fn_813095A0((s8 *) &sp58, (s8 *) &sp58);
                    unksp57 = unksp5B;
                    fn_81309554(&sp54, &sp54);
                    temp_r4_4 = temp_r28 + var_r23;
                    fn_813090A4(&sp28, temp_r4_4 + temp_r4_4->unk4);
                } else {
                    temp_r3_4 = arg4->unk3 - 0x100;
                    fn_81309548(temp_r3_4, temp_r3_4);
                    temp_r4_5 = temp_r28 + var_r23;
                    fn_81308DB8(&sp28, temp_r4_5 + temp_r4_5->unk4);
                }
            } else {
                fn_81309554(&sp54, &sp54);
                temp_r4_6 = temp_r28 + var_r23;
                fn_81308DB8(&sp28, temp_r4_6 + temp_r4_6->unk4);
            }
        } else if ((s16) sCardOp == (s32) var_r27) {
            if ((s16) sCardUiState == 2) {
                fn_813095A0(&sp5C, &sp5C);
                temp_r3_5 = arg4->unk3 - 0x100;
                fn_81309548(temp_r3_5, temp_r3_5);
                temp_r4_7 = temp_r28 + var_r23;
                fn_813090A4(&sp28, temp_r4_7 + temp_r4_7->unk4);
            } else {
                temp_r3_6 = arg4->unk3 - 0x100;
                fn_81309548(temp_r3_6, temp_r3_6);
                temp_r4_8 = temp_r28 + var_r23;
                fn_81308DB8(&sp28, temp_r4_8 + temp_r4_8->unk4);
            }
        } else {
            fn_81309554(&sp60, &sp60);
            temp_r4_9 = temp_r28 + var_r23;
            fn_81308DB8(&sp28, temp_r4_9 + temp_r4_9->unk4);
        }
        var_r27 += 1;
        var_r29 += 2;
        unksp2E += var_r26;
        var_r23 += 8;
        goto loop_23;
    }
}

/* 0x8131AE80 */
void CardDraw_TextLines(void *arg0, u8 *arg1, u8 *arg2, s8 *arg3, s16 arg4, s32 arg5, u16 arg6) {
    u8 sp4F;
    s8 sp4E;
    s8 sp4D;
    s8 sp4C;
    u8 sp4B;
    s8 sp4A;
    s8 sp49;
    s8 sp48;
    s32 sp44;
    s32 sp40;
    u32 sp3C;
    s32 sp38;
    s32 sp34;
    s32 sp30;
    s32 sp2C;                                       /* compiler-managed */
    s32 sp28;                                       /* compiler-managed */
    s32 sp24;
    s32 var_r27;
    u16 var_r28;
    u32 temp_r3_2;
    void *temp_r31;
    void *temp_r3;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r4_3;

    temp_r3 = Card_GetParams();
    temp_r31 = arg1 + 8;
    sp4C = (s8) temp_r3->unk50;
    sp4D = (s8) temp_r3->unk52;
    sp4E = (s8) temp_r3->unk54;
    sp4F = arg3->unk3;
    sp48 = (s8) temp_r3->unk56;
    sp49 = (s8) temp_r3->unk58;
    sp4A = (s8) temp_r3->unk5A;
    sp4B = arg3->unk3;
    temp_r4 = arg2 + arg2->unk8 + (temp_r31->unk28 * 0x24);
    sp24 = temp_r4->unk0;
    sp28 = temp_r4->unk4;
    sp2C = temp_r4->unk8;
    sp30 = temp_r4->unkC;
    sp34 = temp_r4->unk10;
    sp38 = temp_r4->unk14;
    temp_r3_2 = temp_r4->unk18;
    sp3C = temp_r3_2;
    sp40 = temp_r4->unk1C;
    sp44 = temp_r4->unk20;
    sp28 = arg4;
    unksp2A = arg5 + ((s32) arg0->unk18 / 2);
    sp2C = arg6;
    unksp2E = arg0->unk18;
    GX_Setup2DText(temp_r3_2, (s32) temp_r4);
    var_r27 = 0x28;
    var_r28 = 0;
loop_5:
    if (var_r28 < 2U) {
        fn_81309554(&sp4C, &sp4C);
        fn_813095A0(&sp48, &sp48);
        if ((s16) sCardYesNo == (s32) var_r28) {
            temp_r4_2 = temp_r31 + var_r27;
            fn_813090A4(&sp24, temp_r4_2 + temp_r4_2->unk4);
        } else {
            temp_r4_3 = temp_r31 + var_r27;
            fn_81308DB8(&sp24, temp_r4_3 + temp_r4_3->unk4);
        }
        var_r27 += 8;
        var_r28 += 1;
        unksp2A += (s16) sp3C * 0x10;
        goto loop_5;
    }
}

/* 0x8131B018 */
s16 CardDraw_Dialog(u8 arg0, void *arg1, u8 *arg2, u8 *arg3, s8 *arg4) {
    u16 sp24;
    s16 temp_r0_2;
    s16 temp_r29;
    s16 temp_r5_2;
    s16 temp_r6_2;
    s16 temp_r6_3;
    s16 temp_r6_4;
    s16 temp_r7;
    s16 var_r0;
    s16 var_r28;
    s16 var_r3;
    s32 *temp_r22;
    s32 *var_r4;
    s32 temp_r0;
    s32 temp_r5;
    s32 temp_r5_3;
    s32 temp_r6;
    s32 temp_r7_2;
    s32 temp_r8;
    s32 temp_r9;
    s32 temp_r9_2;
    s32 temp_r9_3;
    s32 temp_r9_4;
    s32 var_r22;
    s32 var_r5;
    u16 temp_r22_3;
    u16 var_r29;
    u16 var_r30;
    u8 *temp_r22_2;
    u8 *temp_r31;
    u8 temp_r31_2;
    u8 temp_r6_5;
    void *temp_r23;

    temp_r22 = arg3 + arg3->unkC;
    temp_r23 = Disp_GetSettings();
    var_r4 = temp_r22;
    var_r30 = 0;
loop_3:
    if (var_r30 < (u16) arg3->unk14) {
        if ((u32) (*var_r4 + 0x929B0000) != 0x7330U) {
            var_r4 += 0x30;
            var_r30 += 1;
            goto loop_3;
        }
    }
    temp_r22_2 = &lbl_814AD458 + arg0;
    temp_r31 = &lbl_814AD454 + arg0;
    temp_r6 = (arg0 << 5) & 0x1FE0;
    temp_r5 = ((s8) *temp_r31 + ((s8) *temp_r22_2 * 4)) * 2;
    temp_r29 = *(&gCardUi + temp_r6 + temp_r5);
    var_r28 = *(&lbl_814B6438 + temp_r6 + temp_r5);
    CardDraw_MeasureLines(arg1, arg2, arg3, &sp24);
    temp_r5_2 = arg1->unk0;
    temp_r6_2 = arg1->unk2;
    temp_r0 = (s32) (arg1->unkF * temp_r5_2) / temp_r6_2;
    var_r5 = (s32) (arg1->unk10 * 0x10 * (temp_r6_2 - temp_r5_2)) / temp_r6_2;
    var_r3 = (s16) temp_r0;
    if (var_r3 != 0) {
        if ((s8) *temp_r31 < 2) {
            var_r29 = temp_r29 + 0x280;
        } else {
            var_r29 = temp_r29 - ((((s32) sp24 / 16) + ((temp_r23->unk2B * 2) + 0x28)) * 0x10);
        }
        if ((s8) *temp_r22_2 < 2) {

        } else {
            var_r5 = -var_r5;
            var_r28 -= arg1->unk1E + (temp_r23->unk2B * 8);
        }
        temp_r31_2 = arg4->unk3;
        temp_r22_3 = var_r28 + var_r5;
        arg4->unk3 = (u8) ((temp_r31_2 * (s16) temp_r0) / 255);
        temp_r9 = temp_r23->unk2B << 5;
        Glh_DrawPaneRect(var_r30, arg3, arg4, var_r29, temp_r22_3, sp24 + temp_r9, arg1->unk1A + temp_r9);
        temp_r9_2 = temp_r23->unk2B * 0x10;
        CardDraw_TextBox(arg0, arg1, arg2, arg3, arg4, var_r29 + (((s32) sp24 / 2) + temp_r9_2), temp_r22_3 + temp_r9_2, sp24);
        temp_r6_3 = arg1->unk6;
        arg4->unk3 = (u8) ((s32) (arg4->unk3 * (temp_r6_3 - arg1->unk4)) / temp_r6_3);
        temp_r9_3 = temp_r23->unk2B << 5;
        CardDraw_SetScissor(var_r29, temp_r22_3, sp24 + temp_r9_3, arg1->unk1A + temp_r9_3, (u8) arg1->unkC, arg4, temp_r9_3);
        arg4->unk3 = temp_r31_2;
        temp_r0_2 = arg1->unk4;
        temp_r6_4 = arg1->unkA;
        if (temp_r0_2 >= temp_r6_4) {
            temp_r7 = arg1->unk6;
            temp_r5_3 = temp_r7 - temp_r6_4;
            var_r22 = (s32) (arg1->unk12 * 0x10 * (temp_r7 - temp_r0_2)) / temp_r5_3;
            var_r0 = (s16) ((s32) (arg1->unkF * (temp_r0_2 - temp_r6_4)) / temp_r5_3);
        } else {
            var_r0 = 0;
            var_r22 = 0;
        }
        arg4->unk3 = (u8) ((arg4->unk3 * var_r0) / 255);
        temp_r9_4 = temp_r23->unk2B << 5;
        Glh_DrawPaneRect(var_r30, arg3, arg4, var_r29, var_r28 + (arg1->unk1A + temp_r9_4 + var_r22 + 0x80), sp24 + temp_r9_4, arg1->unk1C + temp_r9_4);
        temp_r7_2 = temp_r23->unk2B << 5;
        CardDraw_SetScissor(var_r29, var_r28 + (arg1->unk1A + temp_r7_2 + var_r22 + 0x80), sp24 + temp_r7_2, arg1->unk1C + temp_r7_2, (u8) arg1->unkC, arg4);
        temp_r6_5 = temp_r23->unk2B;
        temp_r8 = temp_r6_5 * 0x10;
        var_r3 = CardDraw_TextLines(arg1, arg2, arg3, arg4, var_r29 + (((s32) sp24 / 2) + temp_r8), var_r28 + (temp_r8 + arg1->unk18 + (temp_r6_5 << 5) + var_r22 + 0x80), sp24);
        arg4->unk3 = temp_r31_2;
    }
    return var_r3;
}

/* 0x8131B3CC */
u32 CardDraw_LoadFileIcon(void) {
    s32 sp24;
    s16 sp22;
    s8 sp1F;
    s8 sp1E;
    s8 sp1D;
    s8 sp1C;
    s8 sp1B;
    s8 sp1A;
    s8 sp19;
    s8 sp18;
    s32 sp14;
    s16 sp12;
    s8 sp11;
    s8 sp10;
    s8 spF;
    s8 spE;
    s16 spC;
    s16 spA;
    u8 sp8;
    s32 temp_r0;
    s32 temp_r31;
    s32 temp_r4;
    s32 temp_r5;
    s32 temp_r5_2;
    u8 *temp_r7;
    void *temp_r4_2;
    void *temp_r9;

    temp_r31 = CardThread_GetIcons();
    temp_r5 = *(&lbl_814B6674 + (sCardSlot * 0x3F8) + (((s8) *(&lbl_814AD454 + sCardSlot) + (((s8) *(&lbl_814AD458 + sCardSlot) + (s8) (&lbl_814AD45C)[sCardSlot]) * 4)) * 8));
    if ((u16) *(CardThread_GetFiles() + ((sCardSlot * 0x5F4) + (temp_r5 * 0xC))) == 0) {
        return 0U;
    }
    temp_r0 = sCardSlot * 0x1FC0;
    temp_r4 = temp_r5 << 6;
    temp_r7 = temp_r31 + (temp_r0 + temp_r4);
    temp_r4_2 = temp_r31 + temp_r0 + temp_r4;
    temp_r9 = temp_r7 + temp_r4_2->unk14;
    if ((u8) *temp_r7 == 0) {
        return 0U;
    }
    sp18 = 0;
    sp19 = 0;
    sp1A = 0;
    sp1B = 0;
    spA = 0x60;
    spC = 0x20;
    spE = 0;
    spF = 0;
    sp8 = temp_r4_2->unk3;
    sp1C = 1;
    sp1D = 1;
    sp1E = 0;
    sp1F = 0;
    sp22 = 0;
    switch ((u8) temp_r4_2->unk3) {                 /* irregular */
    case 5:
        sp10 = 0;
        sp24 = temp_r9 - &sp8;
        J3D_LoadTexture(0, &sp8);
        return 1U;
    case 9:
        sp10 = 1;
        sp11 = 2;
        temp_r5_2 = (temp_r7 + temp_r4_2->unk18) - &sp8;
        sp12 = 0x100;
        sp14 = temp_r5_2;
        sp24 = temp_r9 - &sp8;
        J3D_LoadTexture(0, &sp8, temp_r5_2);
        return 1U;
    default:
        return 0U;
    }
}

/* 0x8131B56C */
void CardDraw_FileInfo(u8 arg0) {
    s32 sp18;
    s32 sp14;
    s32 sp10;
    void *temp_r4;
    s32 temp_r29;
    s32 temp_r30_2;
    s32 temp_r31;
    s32 temp_r8;
    u32 temp_r5_2;
    u8 temp_r28;
    u8 temp_r30;
    u8 temp_r6;
    void *temp_r5;

    temp_r30 = sCardSlot;
    temp_r8 = temp_r30 * 0x3F8;
    temp_r6 = *(&lbl_814AD454 + temp_r30);
    temp_r5 = &lbl_814B6674 + temp_r8;
    temp_r4 = *(temp_r5 + (((s8) temp_r6 + (((s8) *(&lbl_814AD458 + temp_r30) + (s8) (&lbl_814AD45C)[temp_r30]) * 4)) * 8));
    temp_r28 = temp_r30;
    sp14 = lbl_814ADF24;
    unksp17 = arg0;
    sp18 = lbl_814ADF24;
    temp_r31 = CardThread_GetFiles(temp_r4, temp_r5, temp_r6, &lbl_814B6674, temp_r8);
    GX_Setup2DTex();
    temp_r29 = temp_r30 * 0x5F4;
    temp_r30_2 = (s16) temp_r4 * 0xC;
    if ((u16) *(temp_r31 + (temp_r29 + temp_r30_2)) != 0) {
        sp10 = lbl_814ADF20;
        temp_r5_2 = (u8) ((unksp13 * arg0) / 255) >> 0x1FU;
        unksp13 = (unksp13 * arg0) / 255;
        Glh_DrawBannerLine('titl', sCardGlh, &sp10, CardThread_GetBanner(temp_r28, (s16) temp_r4, temp_r5_2, 0x80808081), 0x1F);
        Glh_DrawBannerLine('info', sCardGlh, &sp10, CardThread_GetBanner(temp_r28, (s16) temp_r4) + 0x20, 0x1F);
        Glh_DrawFrame('frmc', sCardGlh, &sp18);
        CardDraw_LoadTexture(lbl_814AD490.unk4, lbl_814AD498.unk4, (s32) (temp_r31 + temp_r29 + temp_r30_2)->unk2, &lbl_814AD490);
    }
}

/* 0x8131B6E0 */
void CardDraw_Messages(u8 arg0) {
    s32 spC;
    u32 temp_r3;

    spC = lbl_814ADF2C;
    temp_r3 = (u8) ((unkspF * arg0) / 255) >> 0x1FU;
    unkspF = (unkspF * arg0) / 255;
    GX_Setup2DText(temp_r3, 0x80808081, &gCardUi);
    Fader_DrawString(&gCardUi + 0xB9C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xBAC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xBBC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xBCC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xBDC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xBEC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xC1C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xC2C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xC3C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xC4C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xADC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xAEC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xAFC, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xB0C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xB1C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xB2C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xB5C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xB6C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xB7C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xB8C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xC5C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xC6C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xD1C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xD2C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xD3C, sCardSth, sCardGlh, &spC);
    Fader_DrawString(&gCardUi + 0xD4C, sCardSth, sCardGlh, &spC);
}

/* 0x8131B94C */
void Card_Draw(s32 arg0, s32 arg1, u8 arg2) {
    s32 sp118;
    s32 sp114;
    s32 sp110;
    s32 sp10C;
    s32 sp108;
    s32 sp104;
    s32 sp100;
    s32 spFC;
    s32 spF8;
    s32 spF4;
    s32 spF0;
    f32 spEC;
    s32 spE8;
    s32 spE4;
    s32 spE0;
    s32 spDC;
    s32 spD8;
    f32 spD4;
    f32 spD0;
    f32 spCC;
    f32 spC8;
    f32 spC4;
    f32 spC0;
    s16 spBE;
    s16 spBC;
    s16 spB8;
    u8 sp88[0x30];
    u8 sp58[0x30];
    u8 sp28[0x30];
    s32 sp20;
    s32 sp1C;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    s32 spC;
    u32 temp_r6;
    u32 temp_r6_2;
    void *temp_r31;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r3_3;
    void *temp_r3_4;
    void *temp_r4;
    void *temp_r4_2;

    temp_r31 = Disp_GetSettings(&gCardUi, &lbl_8137ECF8);
    CardThread_GetSlots();
    spFC = lbl_8137ECF8.unk0;
    sp100 = lbl_8137ECF8.unk4;
    sp104 = lbl_8137ECF8.unk8;
    sp108 = lbl_8137ECF8.unkC;
    sp10C = lbl_8137ECF8.unk10;
    sp110 = lbl_8137ECF8.unk14;
    sp114 = lbl_8137ECF8.unk18;
    sp118 = lbl_8137ECF8.unk1C;
    spF0 = lbl_8137ECF8.unk20;
    spF4 = lbl_8137ECF8.unk24;
    spF8 = lbl_8137ECF8.unk28;
    spE4 = lbl_8137ECF8.unk2C;
    spE8 = lbl_8137ECF8.unk30;
    spEC = lbl_8137ECF8.unk34;
    spD8 = lbl_8137ECF8.unk38;
    spDC = lbl_8137ECF8.unk3C;
    spE0 = lbl_8137ECF8.unk40;
    spEC = 224.0f / tanf(lbl_8137ECF8.unk38, (3.141592f * temp_r31->unk8) / 360.0f, 3.141592f);
    C_MTXLookAt(&sp58, &spE4, &spF0, &spD8);
    MtxFromSRT(&spFC, &sp88);
    PSMTXConcat(&sp58, &sp88, &sp88);
    MtxInverse33(&sp88, &sp28);
    GXLoadPosMtxImm(&sp88, 0);
    GXLoadNrmMtxImm(&sp28, 0);
    GXSetCurrentMtx(0);
    temp_r3 = Card_GetParams();
    spC0 = (f32) (gCardUi.unkA + gCardUi.unkC) * 0.03125f;
    spC4 = (f32) (gCardUi.unk4A + gCardUi.unk52) * 0.03125f;
    spC8 = temp_r3->unk70;
    temp_r3_2 = Card_GetParams((u16) gCardUi.unk4A, 0x3D000000);
    spCC = (f32) (gCardUi.unk2A + gCardUi.unk2C) * 0.03125f;
    spD0 = (f32) (gCardUi.unk6A + gCardUi.unk72) * 0.03125f;
    spD4 = temp_r3_2->unk70;
    Fader_GetAlpha(&gCardUi + 0xE3C, &spBC, NULL, gCardUi.unk6A, 0.03125f);
    Fader_GetAlpha(&gCardUi + 0xE4C, &spBE, NULL);
    Fader_GetAlpha(&gCardUi + 0xE5C, &spB8, NULL);
    Backdrop_DrawSpots(&sp88, arg1, &spC0, &spCC, (u8) spBC, (u8) spBE, (u8) spB8);
    sp20 = lbl_814ADF28;
    CardThread_GetSlots();
    unksp23 = (unksp23 * arg2) / 255;
    Glh_DrawFrame('mes6', sCardGlh, &sp20);
    if ((u8) sCardSlot == 0) {
        fn_813216F4(0, arg0);
        fn_813216F4(1, arg0);
    } else {
        fn_813216F4(1, arg0);
        fn_813216F4(0, arg0);
    }
    GXLoadPosMtxImm(&sp88, 0);
    GXLoadNrmMtxImm(&sp28, 0);
    GXSetCurrentMtx(0);
    if ((u8) sCardSlot == 0) {
        temp_r3_3 = CardThread_GetSlots();
        sp1C = lbl_814ADF18;
        unksp1F = (unksp1F * arg2) / 255;
        CardDraw_FileBanner(1U, temp_r3_3, &sp1C);
        CardDraw_FileGrid(1U, arg2);
        GX_Setup2DText();
        Fader_DrawString(&gCardUi + 0xC8C, sCardSth, sCardGlh, &sp1C);
        Fader_DrawString(&gCardUi + 0xCAC, sCardSth, sCardGlh, &sp1C);
        CardDraw_OpMenu(0x27U, 1U, arg2);
        CardDraw_OpMenu(0x29U, 1U, arg2);
        temp_r4 = CardThread_GetSlots();
        sp18 = lbl_814ADF18;
        temp_r6 = (u8) ((unksp1B * arg2) / 255) >> 0x1FU;
        unksp1B = (unksp1B * arg2) / 255;
        CardDraw_FileBanner(0U, temp_r4, &sp18, temp_r6);
        CardDraw_FileGrid(0U, arg2);
        GX_Setup2DText();
        Fader_DrawString(&gCardUi + 0xC7C, sCardSth, sCardGlh, &sp18);
        Fader_DrawString(&gCardUi + 0xC9C, sCardSth, sCardGlh, &sp18);
        CardDraw_OpMenu(0x27U, 0U, arg2);
        CardDraw_OpMenu(0x29U, 0U, arg2);
    } else {
        temp_r3_4 = CardThread_GetSlots();
        sp14 = lbl_814ADF18;
        unksp17 = (unksp17 * arg2) / 255;
        CardDraw_FileBanner(0U, temp_r3_4, &sp14);
        CardDraw_FileGrid(0U, arg2);
        GX_Setup2DText();
        Fader_DrawString(&gCardUi + 0xC7C, sCardSth, sCardGlh, &sp14);
        Fader_DrawString(&gCardUi + 0xC9C, sCardSth, sCardGlh, &sp14);
        CardDraw_OpMenu(0x27U, 0U, arg2);
        CardDraw_OpMenu(0x29U, 0U, arg2);
        temp_r4_2 = CardThread_GetSlots();
        sp10 = lbl_814ADF18;
        temp_r6_2 = (u8) ((unksp13 * arg2) / 255) >> 0x1FU;
        unksp13 = (unksp13 * arg2) / 255;
        CardDraw_FileBanner(1U, temp_r4_2, &sp10, temp_r6_2);
        CardDraw_FileGrid(1U, arg2);
        GX_Setup2DText();
        Fader_DrawString(&gCardUi + 0xC8C, sCardSth, sCardGlh, &sp10);
        Fader_DrawString(&gCardUi + 0xCAC, sCardSth, sCardGlh, &sp10);
        CardDraw_OpMenu(0x27U, 1U, arg2);
        CardDraw_OpMenu(0x29U, 1U, arg2);
    }
    spC = lbl_814ADF1C;
    unkspF = (unkspF * arg2) / 255;
    CardDraw_Dialog(0U, &gCardUi + 0xDBC, sCardSth, sCardGlh, (s8 *) &spC);
    CardDraw_Dialog(1U, &gCardUi + 0xDDC, sCardSth, sCardGlh, (s8 *) &spC);
    CardDraw_Messages(arg2);
}
