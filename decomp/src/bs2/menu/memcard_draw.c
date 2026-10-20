// Non-matching: functionally equivalent decompilation
//
// Memory Card icon cubes and file grid (0x8131ECC4..0x81321730): the 6x6 grid
// of cubes behind the browser, one per file block, and their textures.
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x8131ECC4 */
void fn_8131ECC4(void) {
    u8 spC[0x4];
    u8 sp8[0x4];
    void *var_r30;
    s16 var_r26;
    s32 *var_r31;
    s32 temp_r3;
    s32 var_r27;
    s32 var_r28;

    var_r30 = &gCardSlots;
    var_r28 = 0;
    var_r31 = &lbl_814AD4B8;
    do {
        if ((s32) *var_r31 != 0) {
            var_r26 = 0;
            var_r27 = 0;
loop_3:
            temp_r3 = fn_813730DC(var_r28, var_r27, &sp8);
            if ((temp_r3 < 0) && (temp_r3 != -4)) {
                var_r26 = -1;
            } else {
                if ((temp_r3 == 0) && (fn_8137BC98(&sp8, 0x80000000, 4) == 0) && (fn_8137BC98(&spC, 0x80000004, 2) == 0)) {
                    var_r26 += sp40;
                }
                var_r27 += 1;
                if (var_r27 < 0x7F) {
                    goto loop_3;
                }
            }
            var_r30->unk12 = var_r26;
        }
        var_r28 += 1;
        var_r31 += 4;
        var_r30 += 0x14;
    } while (var_r28 < 2);
    fn_8131CB64();
    OSSendMessage(&sCardReplyQueue, 0xC, 1);
}

/* 0x8131EDBC */
void fn_8131EDBC(void *arg0) {
    s32 sp14;
    s32 sp10;
    s32 spC;
    void *temp_r31;

    spC = lbl_8137ED40.unk0;
    sp10 = lbl_8137ED40.unk4;
    sp14 = lbl_8137ED40.unk8;
    temp_r31 = Card_GetParams(lbl_8137ED40.unk0, &lbl_8137ED40);
    fn_8132005C(arg0);
    arg0->unk20 = spC;
    arg0->unk24 = sp10;
    arg0->unk28 = sp14;
    arg0->unk2C = spC;
    arg0->unk30 = sp10;
    arg0->unk34 = sp14;
    arg0->unk44 = spC;
    arg0->unk48 = sp10;
    arg0->unk4C = sp14;
    arg0->unk38 = spC;
    arg0->unk3C = sp10;
    arg0->unk40 = sp14;
    arg0->unk50 = spC;
    arg0->unk54 = sp10;
    arg0->unk58 = sp14;
    arg0->unkE = 0;
    arg0->unk8 = 0;
    arg0->unkA = 0;
    arg0->unk10 = 6;
    arg0->unk1C = (f32) temp_r31->unk48;
    arg0->unk18 = (f32) temp_r31->unk4C;
    arg0->unk14 = (f32) arg0->unk18;
}

/* 0x8131EEC4 */
void fn_8131EEC4(s32 arg0, s32 arg1, s32 arg2) {
    s32 var_r23;
    s32 var_r23_2;
    s32 var_r24;
    s32 var_r27_2;
    s32 var_r28_2;
    void *temp_r30;
    void *var_r25;
    void *var_r25_2;
    void *var_r25_3;
    void *var_r26;
    void *var_r26_2;
    void *var_r27;
    void *var_r28;

    if (arg0 != 0) {
        lbl_814C4E20.unk5B48 = ResGet(0x57);
        lbl_814C4E20.unk5B80 = ResGet(0x58);
        J3D_LoadModelData(&lbl_814C4E20 + 0x5B48);
        J3D_LoadModelData(&lbl_814C4E20 + 0x5B80);
        var_r26 = &lbl_814C4E20 + 0x5BB8;
        var_r24 = 0;
        var_r25 = &lbl_814C4E20 + 0xD2C8;
        do {
            var_r28 = var_r26;
            var_r27 = var_r25;
            var_r23 = 0;
loop_3:
            var_r28->unk68 = (void *) (&lbl_814C4E20 + 0x5B48);
            var_r27->unk68 = (void *) (&lbl_814C4E20 + 0x5B80);
            J3D_InitModel(var_r28, 0);
            J3D_InitModel(var_r27, 0);
            var_r23 += 1;
            var_r28 += 0x78;
            var_r27 += 0x78;
            if (var_r23 < 0x7F) {
                goto loop_3;
            }
            var_r24 += 1;
            var_r26 += 0x3B88;
            var_r25 += 0x3B88;
        } while (var_r24 < 2);
        temp_r30 = &lbl_814C4E20 + 0x5B80;
        lbl_814C4E20.unk14A40 = temp_r30;
        J3D_InitModel(&lbl_814C4E20 + 0x149D8, 0);
        var_r23_2 = 0;
        var_r25_2 = &lbl_814C4E20 + 0x14A50;
        do {
            var_r25_2->unk68 = temp_r30;
            J3D_InitModel(var_r25_2, 0);
            var_r23_2 += 1;
            var_r25_2 += 0x78;
        } while (var_r23_2 < 0xC);
    }
    var_r27_2 = 0;
    lbl_814AD4E8 = arg1;
    lbl_814AD4EC = arg2;
    var_r26_2 = &lbl_814C4E20 + (0 * 0x2DA4);
    do {
        var_r28_2 = 0;
        var_r25_3 = var_r26_2 + (0 * 0x5C);
loop_9:
        fn_8131EDBC(var_r25_3);
        var_r28_2 += 1;
        var_r25_3 += 0x5C;
        if (var_r28_2 < 0x7F) {
            goto loop_9;
        }
        var_r27_2 += 1;
        var_r26_2 += 0x2DA4;
    } while (var_r27_2 < 2);
    lbl_814AD4F8 = 0;
    lbl_814AD4FC = 1;
    lbl_814AD500 = 0;
    lbl_814AD504 = 0;
    lbl_814AD508 = 0;
    fn_8131EDBC(&lbl_814C4E20 + 0x14FF0, 0);
}

/* 0x8131F048 */
void fn_8131F048(void) {
    void *sp180;
    void *sp17C;
    u16 *sp178;
    void *sp174;
    u8 sp144[0x30];
    u8 sp114[0x30];
    u8 spE4[0x30];
    s32 spE0;
    void *spDC;
    s32 spD8;
    s32 spD4;
    s32 spD0;
    s32 spCC;
    s32 spC8;
    s32 spC4;
    f32 spC0;
    f32 spBC;
    f32 spB8;
    f32 spB4;
    f32 spB0;
    f32 spAC;
    s32 spA8;
    void *spA4;
    s32 spA0;
    s32 sp9C;
    s32 sp98;
    s32 sp94;
    s32 sp90;
    s32 sp8C;
    f32 sp88;
    f32 sp84;
    f32 sp80;
    f32 sp7C;
    f32 sp78;
    f32 sp74;
    f32 sp70;
    f32 sp6C;
    f32 sp68;
    f32 sp64;
    f32 sp60;
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    f32 sp50;
    f32 sp4C;
    f32 sp48;
    f32 sp44;
    f32 sp40;
    f32 sp3C;
    f32 sp38;
    f32 sp34;
    f32 sp30;
    f32 sp2C;
    f32 sp10;
    f32 spC;
    f32 sp8;
    void *var_r30;
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f28;
    f32 temp_f2;
    f32 temp_f2_2;
    f32 temp_f2_3;
    f32 temp_f2_4;
    f32 temp_f2_5;
    f32 temp_f2_6;
    f32 temp_f3;
    f32 temp_f3_2;
    f32 temp_f4;
    f32 temp_f4_2;
    f32 temp_f4_3;
    f32 var_f28;
    s16 temp_r15;
    s16 temp_r19;
    s16 temp_r24;
    s32 temp_r0;
    s32 temp_r16_2;
    s32 temp_r17;
    s32 temp_r22;
    s32 temp_r23;
    s32 temp_r3_2;
    s32 var_r17;
    s32 var_r25;
    u16 *temp_r16;
    u16 *var_r20;
    void *temp_r18;
    void *temp_r29;
    void *temp_r3;
    void *var_r14;
    void *var_r26;
    void *var_r27;

    var_f28 = saved_reg_f28;
    sp180 = &lbl_814C4E20;
    temp_r29 = Menu_Get(&lbl_8137ED40);
    temp_r18 = Card_GetParams();
    sp8C = lbl_8137ED40.unkC;
    sp90 = lbl_8137ED40.unk10;
    sp94 = lbl_8137ED40.unk14;
    sp98 = lbl_8137ED40.unk18;
    sp9C = lbl_8137ED40.unk1C;
    spA0 = lbl_8137ED40.unk20;
    spA4 = lbl_8137ED40.unk24;
    spA8 = lbl_8137ED40.unk28;
    spC4 = sp8C;
    spC8 = lbl_8137ED40.unk10;
    spCC = lbl_8137ED40.unk14;
    spD0 = lbl_8137ED40.unk18;
    spD4 = lbl_8137ED40.unk1C;
    spD8 = lbl_8137ED40.unk20;
    spDC = lbl_8137ED40.unk24;
    spE0 = lbl_8137ED40.unk28;
    temp_r15 = lbl_814AD508;
    temp_r16 = CardThread_GetFiles(lbl_8137ED40.unk24, lbl_8137ED40.unkC);
    MtxFromSRT(&spC4, &sp114);
    var_r17 = 0;
    temp_r0 = 0 * 0x3B88;
    sp178 = temp_r16;
    temp_r3 = sp180 + 0x10000 + temp_r0;
    sp174 = temp_r3;
    temp_r24 = temp_r15 * 0x23;
    temp_r19 = temp_r15 * 0x46;
    sp17C = sp180;
    sp174 = temp_r3 - 0x2D38;
    var_r14 = sp180 + temp_r0 + 0x5BB8;
    do {
        var_r25 = 0;
        temp_r3_2 = 0 * 0x78;
        var_r30 = sp17C;
        var_r20 = sp178;
        var_r26 = sp174 + temp_r3_2;
        var_r27 = var_r14 + temp_r3_2;
loop_2:
        sp80 = lbl_8137ED40.unk2C;
        sp84 = lbl_8137ED40.unk30;
        sp88 = lbl_8137ED40.unk34;
        temp_f1 = var_r30->unk14;
        sp80 = temp_f1;
        sp84 = temp_f1;
        sp88 = temp_f1;
        spB8 = sp80;
        spBC = sp84;
        spC0 = sp88;
        if ((var_r17 == (s32) lbl_814AD4F0) && (var_r25 == (s32) lbl_814AD4F4) && ((s32) lbl_814AD4F8 != 0)) {
            if ((s32) lbl_814AD500 == 0) {
                temp_f2 = sp180->unk15008;
                sp74 = lbl_8137ED40.unk38;
                temp_f2_2 = (sp180->unk15004 - temp_f2) / (sp180->unk1500C - temp_f2);
                sp78 = lbl_8137ED40.unk3C;
                sp7C = lbl_8137ED40.unk40;
                var_f28 = temp_f2_2;
                sp74 = sp180->unk15010 * 0.0625f;
                sp78 = -sp180->unk15014 * 0.0625f;
                sp7C = 2.0f * temp_f2_2;
                spAC = sp74;
                spB0 = sp78;
                spB4 = sp7C;
            }
        } else {
            temp_f2_3 = var_r30->unk18;
            sp68 = lbl_8137ED40.unk44;
            temp_f2_4 = (temp_f1 - temp_f2_3) / (var_r30->unk1C - temp_f2_3);
            sp6C = lbl_8137ED40.unk48;
            sp70 = lbl_8137ED40.unk4C;
            var_f28 = temp_f2_4;
            sp68 = var_r30->unk20 * 0.0625f;
            sp6C = -var_r30->unk24 * 0.0625f;
            sp70 = 2.0f * temp_f2_4;
            spAC = sp68;
            spB0 = sp6C;
            spB4 = sp70;
        }
        temp_r22 = (s32) (var_f28 * (f32) temp_r29->unk24 * CosS(temp_r24));
        temp_r23 = (s32) (var_f28 * (f32) temp_r29->unk22 * CosS(temp_r29->unk2E + temp_r24));
        MtxRot((s32) (var_f28 * (f32) temp_r29->unk20 * CosS(temp_r19)), temp_r23, temp_r22, &spE4);
        temp_f2_5 = var_f28 * temp_r18->unk68;
        temp_f1_2 = temp_f2_5 * SinS(temp_r19);
        temp_f4 = var_f28 * temp_r18->unk64;
        PSMTXTrans(&sp144, spAC + (temp_f4 * SinS(temp_r29->unk30 + temp_r24, temp_f1_2, temp_f2_5)), spB0 - temp_f1_2, spB4, temp_f4);
        PSMTXConcat(&sp144, &sp114, &sp144);
        PSMTXConcat(&sp144, &spE4, &sp144);
        if ((u16) *var_r20 != 0) {
            sp5C = spB8;
            sp60 = spBC;
            sp64 = spC0;
            J3D_SetModelMtx(var_r26, &sp144, &sp5C, spBC);
            J3D_SetViewMtx(var_r26, fn_81302F94());
            J3D_CalcJoints(var_r26);
        } else {
            sp50 = spB8;
            sp54 = spBC;
            sp58 = spC0;
            J3D_SetModelMtx(var_r27, &sp144, &sp50, spBC);
            J3D_SetViewMtx(var_r27, fn_81302F94());
            J3D_CalcJoints(var_r27);
        }
        var_r25 += 1;
        var_r30 += 0x5C;
        var_r20 += 0xC;
        var_r26 += 0x78;
        var_r27 += 0x78;
        if (var_r25 < 0x7F) {
            goto loop_2;
        }
        var_r17 += 1;
        sp17C += 0x2DA4;
        var_r14 += 0x3B88;
        sp178 += 0x5F4;
        sp174 += 0x3B88;
    } while (var_r17 < 2);
    if ((s32) lbl_814AD500 != 0) {
        sp44 = lbl_8137ED40.unk50;
        sp48 = lbl_8137ED40.unk54;
        sp4C = lbl_8137ED40.unk58;
        temp_f3 = sp180->unk15004;
        sp44 = temp_f3;
        sp48 = temp_f3;
        sp4C = temp_f3;
        sp8 = sp44;
        spC = sp48;
        sp10 = sp4C;
        temp_f4_2 = sp180->unk15008;
        temp_f3_2 = temp_f3 - temp_f4_2;
        sp38 = lbl_8137ED40.unk5C;
        temp_f28 = temp_f3_2 / (sp180->unk1500C - temp_f4_2);
        sp3C = lbl_8137ED40.unk60;
        sp40 = lbl_8137ED40.unk64;
        sp38 = sp180->unk15010 * 0.0625f;
        temp_f1_3 = -sp180->unk15014 * 0.0625f;
        sp3C = temp_f1_3;
        sp40 = 2.0f * temp_f28;
        spAC = sp38;
        spB0 = sp3C;
        spB4 = sp40;
        temp_r16_2 = (s32) (temp_f28 * (f32) temp_r29->unk24 * CosS(temp_r24, sp3C, temp_f1_3, 0.0625f, temp_f3_2, temp_f4_2));
        temp_r17 = (s32) (temp_f28 * (f32) temp_r29->unk22 * CosS(temp_r29->unk2E + temp_r24));
        MtxRot((s32) (temp_f28 * (f32) temp_r29->unk20 * CosS(temp_r19)), temp_r17, temp_r16_2, &spE4);
        temp_f2_6 = temp_f28 * temp_r18->unk68;
        temp_f1_4 = temp_f2_6 * SinS(temp_r19);
        temp_f4_3 = temp_f28 * temp_r18->unk64;
        PSMTXTrans(&sp144, spAC + (temp_f4_3 * SinS(temp_r29->unk30 + temp_r24, temp_f1_4, temp_f2_6)), spB0 - temp_f1_4, spB4, temp_f4_3);
        PSMTXConcat(&sp144, &sp114, &sp144);
        PSMTXConcat(&sp144, &spE4, &sp144);
        if ((s32) lbl_814AD504 == 0) {
            sp2C = sp8;
            sp30 = spC;
            sp34 = sp10;
            J3D_SetModelMtx(sp180 + 0x149D8, &sp144, &sp2C);
            J3D_SetViewMtx(sp180 + 0x149D8, fn_81302F94());
            J3D_CalcJoints(sp180 + 0x149D8);
            return;
        }
        PSMTXCopy(&sp144, sp180 + 0x15094);
    }
}

/* 0x8131F770 */
void fn_8131F770(s32 arg0) {
    u8 sp38[0x4];
    s32 sp24;
    f32 sp20;
    f32 sp1C;
    s32 sp18;
    f32 sp14;
    f32 sp10;
    f32 temp_f30;
    f32 temp_f31;
    f32 var_f30;
    s16 temp_r0;
    s16 temp_r6;
    s16 temp_r7;
    s32 temp_r0_2;
    s32 temp_r5;
    void *temp_r29;
    void *temp_r29_2;
    void *temp_r30;
    void *temp_r3;

    temp_r5 = arg0 * 2;
    temp_r3 = &lbl_814C4E20 + 0x10000 + temp_r5;
    temp_r6 = temp_r3->unk504C;
    temp_r0 = temp_r3->unk507C;
    if (temp_r6 > temp_r0) {
        temp_r7 = temp_r3->unk5064;
        temp_r0_2 = temp_r6 - temp_r0;
        if (temp_r0_2 <= temp_r7) {
            var_f30 = (f32) temp_r0_2 / (f32) temp_r7;
        } else {
            var_f30 = 0.0f;
        }
    } else {
        var_f30 = 0.0f;
    }
    temp_r30 = &lbl_814C4E20 + 0x10000;
    temp_r29 = temp_r30 + temp_r5;
    temp_f30 = 48.0f * (0.25f + (0.75f * var_f30));
    temp_f31 = temp_f30 * SinS(temp_r29->unk50C4);
    PSMTXTrans(&sp38, temp_f31, temp_f30 * CosS(temp_r29->unk50C4, 0.25f, 48.0f), 0.0f);
    PSMTXConcat(&lbl_814C4E20 + 0x15094, &sp38, &sp38);
    sp10 = lbl_8137EDB4.unk0;
    temp_r29_2 = temp_r30 + (arg0 * 0x78) + 0x4A50;
    sp14 = lbl_8137EDB4.unk4;
    sp18 = lbl_8137EDB4.unk8;
    sp1C = sp10;
    sp20 = sp14;
    sp24 = lbl_8137EDB4.unk8;
    J3D_SetModelMtx(temp_r29_2, &sp38, &sp1C, sp14);
    J3D_SetViewMtx(temp_r29_2, fn_81302F94());
    J3D_CalcJoints(temp_r29_2);
}

/* 0x8131F8F8 */
void fn_8131F8F8(void) {
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
    s32 spC;
    s32 sp8;
    void *var_r27_3;
    void *var_r28_3;
    s16 *var_r24_2;
    s16 *var_r28;
    s16 *var_r28_2;
    s16 *var_r30;
    s16 *var_r30_2;
    s16 *var_r31;
    s16 temp_r0;
    s16 temp_r3;
    s16 temp_r3_2;
    s16 temp_r3_3;
    s16 temp_r3_4;
    s16 temp_r4;
    s16 temp_r4_2;
    s32 var_r25;
    s32 var_r25_2;
    s32 var_r26;
    s32 var_r26_2;
    s32 var_r26_3;
    s32 var_r26_4;
    s32 var_r27;
    s32 var_r27_2;
    s32 var_r27_4;
    s32 var_r4;
    u16 *var_r24;
    void *temp_r24;
    void *temp_r28;
    void *temp_r3_5;
    void *var_r24_3;
    void *var_r25_3;

    if (fn_813230E8(&lbl_814C4E20) != 0) {
        fn_8131F048();
        if ((s32) lbl_814AD500 != 0) {
            if ((s32) lbl_814AD4F8 != 0) {
                lbl_814C4E20.unk15004 = Hermite((f32) lbl_814C4E20.unk14FFA, 0.0f, lbl_814C4E20.unk15008, 0.0f, (f32) lbl_814C4E20.unk15000, lbl_814C4E20.unk1500C, 0.0f);
                lbl_814C4E20.unk14FFA = (s16) (lbl_814C4E20.unk14FFA - 1);
                if ((s16) lbl_814C4E20.unk14FFA > (s16) lbl_814C4E20.unk15000) {
                    lbl_814C4E20.unk14FFA = (s16) lbl_814C4E20.unk15000;
                }
                if ((s16) lbl_814C4E20.unk14FFA < 0) {
                    lbl_814C4E20.unk14FFA = 0;
                }
            } else {
                var_r26 = 0;
                lbl_814C4E20.unk15004 = Hermite((f32) lbl_814C4E20.unk14FFA, 0.0f, lbl_814C4E20.unk15008, 0.0f, (f32) lbl_814C4E20.unk15000, lbl_814C4E20.unk1500C, 0.0f);
                lbl_814C4E20.unk14FFA = (s16) (lbl_814C4E20.unk14FFA - 1);
                if ((s16) lbl_814C4E20.unk14FFA > (s16) lbl_814C4E20.unk15000) {
                    lbl_814C4E20.unk14FFA = (s16) lbl_814C4E20.unk15000;
                }
                if ((s16) lbl_814C4E20.unk14FFA < 0) {
                    lbl_814C4E20.unk14FFA = 0;
                    var_r26 = 1;
                }
                if (var_r26 != 0) {
                    if ((s32) lbl_814AD504 == 0) {
                        var_r27 = 0;
                        var_r28 = &lbl_814C4E20 + 0x1504C;
                        var_r30 = &lbl_814C4E20 + 0x15064;
                        var_r31 = &lbl_814C4E20 + 0x1507C;
                        var_r24 = &lbl_814C4E20 + 0x150C4;
                        do {
                            *var_r28 = 0;
                            *var_r30 = 0xA;
                            *var_r31 = Rand() & 3;
                            var_r27 += 1;
                            *var_r24 = Rand();
                            var_r28 += 2;
                            var_r30 += 2;
                            var_r31 += 2;
                            var_r24 += 2;
                        } while (var_r27 < 0xC);
                    }
                    var_r27_2 = 0;
                    lbl_814AD504 = 1;
                    var_r25 = 0;
                    var_r26_2 = 0;
                    var_r24_2 = &lbl_814C4E20 + 0x1504C;
                    var_r28_2 = &lbl_814C4E20 + 0x1507C;
                    var_r30_2 = &lbl_814C4E20 + 0x15064;
                    do {
                        *var_r24_2 += 1;
                        temp_r0 = *var_r24_2;
                        temp_r3 = *var_r28_2;
                        if (temp_r0 > temp_r3) {
                            temp_r4 = *var_r30_2;
                            if ((s32) (temp_r0 - temp_r3) > temp_r4) {
                                *var_r24_2 = temp_r4 + temp_r3;
                                var_r25 += 1;
                            }
                        }
                        fn_8131F770(var_r26_2);
                        var_r26_2 += 1;
                        var_r24_2 += 2;
                        var_r28_2 += 2;
                        var_r30_2 += 2;
                    } while (var_r26_2 < 0xC);
                    if (var_r25 == 0xC) {
                        var_r27_2 = 1;
                    }
                    if (var_r27_2 != 0) {
                        lbl_814AD504 = 0;
                        lbl_814AD500 = 0;
                    }
                }
            }
        } else {
            var_r27_3 = &lbl_814C4E20;
            var_r26_3 = 0;
            do {
                var_r25_2 = 0;
                var_r28_3 = var_r27_3;
loop_26:
                if ((s32) lbl_814AD4FC != 0) {
                    var_r4 = 0x19;
                } else {
                    var_r4 = 8;
                }
                if ((s16) var_r28_3->unk6 >= (s16) var_r28_3->unk0) {
                    if ((s16) var_r28_3->unkE == 0xFF) {
                        var_r28_3->unk8 = (s16) (var_r28_3->unk8 + var_r4);
                    } else {
                        var_r28_3->unk8 = (s16) (var_r28_3->unk8 - var_r4);
                    }
                }
                if ((s16) var_r28_3->unk8 > 0xFF) {
                    var_r28_3->unk8 = 0xFF;
                }
                if ((s16) var_r28_3->unk8 < 0) {
                    var_r28_3->unk8 = 0;
                }
                sp2C = lbl_8137EDA8.unk0;
                sp30 = lbl_8137EDA8.unk4;
                sp34 = lbl_8137EDA8.unk8;
                temp_r4_2 = var_r28_3->unk6;
                temp_r3_2 = var_r28_3->unk0;
                if (temp_r4_2 >= temp_r3_2) {
                    VecHermite(var_r28_3 + 0x44, var_r28_3 + 0x38, var_r28_3 + 0x50, &sp2C, &sp20, (f32) temp_r4_2, (f32) temp_r3_2, (f32) var_r28_3->unkC);
                } else {
                    sp20 = var_r28_3->unk44;
                    sp24 = var_r28_3->unk48;
                    sp28 = var_r28_3->unk4C;
                }
                fn_813648E8(&sp20, var_r28_3 + 0x20, var_r28_3 + 0x2C);
                M2C_STRUCT_COPY(var_r28_3, sp - 0x80, 0xC);
                var_r28_3->unk6 = (s16) (var_r28_3->unk6 + 1);
                temp_r3_3 = var_r28_3->unkC;
                if ((s16) var_r28_3->unk6 > temp_r3_3) {
                    var_r28_3->unk6 = temp_r3_3;
                }
                var_r28_3->unk14 = Hermite((f32) var_r28_3->unkA, 0.0f, var_r28_3->unk18, 0.0f, (f32) var_r28_3->unk10, var_r28_3->unk1C, 0.0f);
                if ((var_r26_3 == Card_GetSlot()) && (var_r25_2 == Card_GetSelectedFile()) && ((s32) lbl_814AD4FC == 0)) {
                    var_r28_3->unkA = (s16) (var_r28_3->unkA + 1);
                } else {
                    var_r28_3->unkA = (s16) (var_r28_3->unkA - 1);
                }
                temp_r3_4 = var_r28_3->unk10;
                if ((s16) var_r28_3->unkA > temp_r3_4) {
                    var_r28_3->unkA = temp_r3_4;
                }
                if ((s16) var_r28_3->unkA < 0) {
                    var_r28_3->unkA = 0;
                }
                var_r25_2 += 1;
                var_r28_3 += 0x5C;
                if (var_r25_2 < 0x7F) {
                    goto loop_26;
                }
                var_r26_3 += 1;
                var_r27_3 += 0x2DA4;
            } while (var_r26_3 < 2);
            if ((s32) lbl_814AD4F8 != 0) {
                sp14 = lbl_8137EDA8.unk0;
                sp18 = lbl_8137EDA8.unk4;
                sp1C = lbl_8137EDA8.unk8;
                if ((s16) lbl_814C4E20.unk14FF6 >= (s16) lbl_814C4E20.unk14FF0) {
                    VecHermite(&lbl_814C4E20 + 0x15034, &lbl_814C4E20 + 0x15028, &lbl_814C4E20 + 0x15040, &sp14, &sp8, (f32) lbl_814C4E20.unk14FF6, (f32) lbl_814C4E20.unk14FF0, (f32) lbl_814C4E20.unk14FFC);
                } else {
                    sp8 = lbl_814C4E20.unk15034;
                    spC = lbl_814C4E20.unk15038;
                    sp10 = lbl_814C4E20.unk1503C;
                }
                temp_r24 = &lbl_814C4E20 + 0x15010;
                temp_r28 = &lbl_814C4E20 + 0x1501C;
                fn_813648E8(&sp8, temp_r24, temp_r28);
                lbl_814C4E20.unk15010 = sp8;
                temp_r24->unk4 = spC;
                temp_r24->unk8 = sp10;
                lbl_814C4E20.unk14FF6 = (s16) (lbl_814C4E20.unk14FF6 + 1);
                if ((s16) lbl_814C4E20.unk14FF6 > (s16) lbl_814C4E20.unk14FFC) {
                    lbl_814C4E20.unk14FF6 = (s16) lbl_814C4E20.unk14FFC;
                }
                if ((s16) lbl_814C4E20.unk14FF6 >= (s32) (lbl_814C4E20.unk14FFC - 5)) {
                    lbl_814C4E20.unk15034 = (s32) lbl_814C4E20.unk15010;
                    lbl_814C4E20.unk15038 = (s32) temp_r24->unk4;
                    lbl_814C4E20.unk1503C = (s32) temp_r24->unk8;
                    lbl_814C4E20.unk15028 = (s32) lbl_814C4E20.unk1501C;
                    lbl_814C4E20.unk1502C = (s32) temp_r28->unk4;
                    lbl_814C4E20.unk15030 = (s32) temp_r28->unk8;
                    M2C_ERROR(/* unknown instruction: cror eq, gt, eq */);
                    if (lbl_814C4E20.unk15040 == 4672.0f) {
                        lbl_814C4E20.unk15040 = (f32) (lbl_814C4E20.unk15040 - (2.0f * (lbl_814C4E20.unk15040 - 4672.0f)));
                    } else {
                        lbl_814C4E20.unk15040 = (f32) (lbl_814C4E20.unk15040 + (2.0f * (4672.0f - lbl_814C4E20.unk15040)));
                    }
                    temp_r3_5 = &lbl_814C4E20 + 0x10000;
                    M2C_ERROR(/* unknown instruction: cror eq, gt, eq */);
                    if (lbl_814C4E20.unk15044 == 3200.0f) {
                        temp_r3_5->unk5044 = (f32) (lbl_814C4E20.unk15044 - (2.0f * (lbl_814C4E20.unk15044 - 3200.0f)));
                    } else {
                        temp_r3_5->unk5044 = (f32) (lbl_814C4E20.unk15044 + (2.0f * (3200.0f - lbl_814C4E20.unk15044)));
                    }
                    lbl_814C4E20.unk14FF6 = 0;
                    lbl_814C4E20.unk14FFC = 0xB4;
                }
            }
        }
        if (((s32) lbl_814AD4FC != 0) && (fn_81320ABC() != 0)) {
            var_r26_4 = 0;
            var_r25_3 = &lbl_814C4E20 + (0 * 0x2DA4);
            do {
                var_r27_4 = 0;
                var_r24_3 = var_r25_3 + (0 * 0x5C);
loop_71:
                fn_81320078(var_r24_3);
                var_r27_4 += 1;
                var_r24_3 += 0x5C;
                if (var_r27_4 < 0x7F) {
                    goto loop_71;
                }
                var_r26_4 += 1;
                var_r25_3 += 0x2DA4;
            } while (var_r26_4 < 2);
            lbl_814AD4FC = 0;
        }
        lbl_814AD508 += 7;
    }
}

/* 0x8132005C */
void fn_8132005C(void *arg0) {
    arg0->unk0 = 0;
    arg0->unk6 = (s16) arg0->unk0;
    arg0->unkC = 0x28;
}

/* 0x81320078 */
void fn_81320078(void *arg0) {
    arg0->unk0 = 0;
    arg0->unkC = 0x28;
    arg0->unk6 = (s16) arg0->unkC;
}

/* 0x81320094 */
void fn_81320094(u8 arg0, u16 arg1, u16 arg2, u16 arg3) {
    s32 sp34;
    f32 sp30;
    f32 sp2C;
    s32 sp24;
    s32 sp20;
    s32 sp1C;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    f32 temp_f1;
    s32 temp_r0;
    s32 temp_r0_2;
    s32 temp_r0_3;
    s32 temp_r0_4;
    s32 temp_r3;
    s32 temp_r3_2;
    s32 temp_r7;
    s32 temp_r7_2;
    s32 temp_r8;
    s32 temp_r8_2;
    u16 temp_r29;
    u16 temp_r3_3;
    void *temp_r5;
    void *temp_r5_2;
    void *temp_r5_3;
    void *temp_r5_4;
    void *temp_r6;
    void *temp_r6_2;

    lbl_814AD4F2 = arg1;
    lbl_814AD4F0 = arg0;
    lbl_814AD4F4 = arg2;
    lbl_814AD4F6 = arg3;
    if ((u16) (arg3 - 2) <= 2U) {
        if (arg3 != 4) {
            temp_r8 = arg0 * 0x2DA4;
            temp_r7 = arg2 * 0x5C;
            temp_r6 = &lbl_814C4E20 + 0x14FE8;
            temp_r5 = (&lbl_814C4E20 + (temp_r8 + temp_r7)) - 8;
            M2C_STRUCT_COPY((s32) temp_r6 + 8, (s32) temp_r5 + 8, 0x58);
            temp_r5_2 = &lbl_814C4E20 + temp_r8 + temp_r7;
            (temp_r6 + 0x58)->unk8 = (s32) (temp_r5 + 0x58)->unk8;
            temp_r3 = temp_r5_2->unk20;
            temp_r0 = temp_r5_2->unk24;
            sp1C = temp_r3;
            sp20 = temp_r0;
            temp_r0_2 = temp_r5_2->unk28;
            sp24 = temp_r0_2;
            lbl_814C4E20.unk15034 = temp_r3;
            lbl_814C4E20.unk15038 = temp_r0;
            lbl_814C4E20.unk1503C = temp_r0_2;
            temp_r3_2 = temp_r5_2->unk2C;
            temp_r0_3 = temp_r5_2->unk30;
            sp10 = temp_r3_2;
            sp14 = temp_r0_3;
            temp_r0_4 = temp_r5_2->unk34;
            sp18 = temp_r0_4;
            lbl_814C4E20.unk15028 = temp_r3_2;
            lbl_814C4E20.unk1502C = temp_r0_3;
            lbl_814C4E20.unk15030 = temp_r0_4;
            lbl_814C4E20.unk15010 = temp_r3;
            lbl_814C4E20.unk15014 = temp_r0;
            lbl_814C4E20.unk15018 = temp_r0_2;
            lbl_814C4E20.unk1501C = temp_r3_2;
            lbl_814C4E20.unk15020 = temp_r0_3;
            lbl_814C4E20.unk15024 = temp_r0_4;
            temp_r5_2->unk8 = 0;
            temp_r5_2->unkE = 0;
            temp_r29 = Rand(temp_r3_2, temp_r5_2, temp_r7, temp_r8);
            temp_r3_3 = temp_r29;
            sp2C = lbl_8137EDC0.unk0;
            sp30 = lbl_8137EDC0.unk4;
            sp34 = lbl_8137EDC0.unk8;
            temp_f1 = 292.0f + (40.0f * SinS((s16) temp_r3_3, lbl_8137EDC0.unk0, (bitwise f32) &lbl_8137EDC0));
            sp2C = 16.0f * temp_f1;
            sp30 = 16.0f * (200.0f + (40.0f * CosS((s16) temp_r29, temp_f1, 292.0f)));
            lbl_814C4E20.unk15040 = sp2C;
            lbl_814C4E20.unk15044 = sp30;
            lbl_814C4E20.unk15048 = sp34;
            lbl_814C4E20.unk14FF6 = 0;
            lbl_814C4E20.unk14FFC = 0x28;
        } else {
            temp_r8_2 = arg0 * 0x2DA4;
            temp_r7_2 = arg2 * 0x5C;
            temp_r6_2 = &lbl_814C4E20 + 0x14FE8;
            temp_r5_3 = (&lbl_814C4E20 + (temp_r8_2 + temp_r7_2)) - 8;
            M2C_STRUCT_COPY((s32) temp_r6_2 + 8, (s32) temp_r5_3 + 8, 0x58);
            temp_r5_4 = &lbl_814C4E20 + temp_r8_2 + temp_r7_2;
            (temp_r6_2 + 0x58)->unk8 = (s32) (temp_r5_3 + 0x58)->unk8;
            temp_r5_4->unk8 = 0;
            temp_r5_4->unkE = 0;
            lbl_814C4E20.unk14FFA = 0x14;
            lbl_814C4E20.unk15000 = 0x14;
            lbl_814C4E20.unk15008 = 0.0f;
            lbl_814AD500 = 1;
            lbl_814AD504 = 0;
        }
        lbl_814AD4F8 = 1;
    }
}

/* 0x8132030C */
void fn_8132030C(u8 arg0) {
    s32 sp34;
    s32 sp30;
    s32 sp2C;
    s32 sp28;
    s32 sp24;
    s32 sp20;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s32 temp_r0;
    s32 temp_r3;
    void *temp_r3_2;
    void *temp_r7;
    void *temp_r8;
    void *temp_r8_2;
    void *temp_r8_3;

    if ((u16) (lbl_814AD4F6 - 2) <= 2U) {
        lbl_814AD4F8 = 0;
        if ((u16) lbl_814AD4F6 != 4) {
            temp_r3 = (lbl_814AD4F0 ^ 1) * 0x2DA4;
            spC = lbl_814C4E20.unk15010;
            sp10 = lbl_814C4E20.unk15014;
            temp_r0 = arg0 * 0x5C;
            sp14 = lbl_814C4E20.unk15018;
            temp_r8 = &lbl_814C4E20 + temp_r3 + temp_r0;
            temp_r8->unk20 = (s32) lbl_814C4E20.unk15010;
            temp_r8->unk24 = (s32) lbl_814C4E20.unk15014;
            temp_r8->unk28 = sp14;
            if ((u16) lbl_814AD4F6 == 2) {
                temp_r8_2 = &lbl_814C4E20 + (lbl_814AD4F0 * 0x2DA4) + (lbl_814AD4F4 * 0x5C);
                temp_r8_2->unk20 = spC;
                temp_r8_2->unk24 = sp10;
                temp_r8_2->unk28 = sp14;
            }
            if ((u16) lbl_814AD4F2 < 8U) {
                sp2C = lbl_8137EDCC.unk0;
                sp30 = lbl_8137EDCC.unk4;
                sp34 = lbl_8137EDCC.unk8;
                M2C_STRUCT_COPY(&lbl_814C4E20 + temp_r3 + temp_r0, sp - 0x40, 0xC);
            } else {
                temp_r8_3 = &lbl_814C4E20 + temp_r3 + temp_r0;
                sp20 = lbl_8137EDD8.unk0;
                sp24 = lbl_8137EDD8.unk4;
                sp28 = lbl_8137EDD8.unk8;
                temp_r8_3->unk2C = sp20;
                temp_r8_3->unk30 = sp24;
                temp_r8_3->unk34 = (s32) lbl_8137EDD8.unk8;
            }
            temp_r7 = &lbl_814C4E20 + temp_r3 + temp_r0;
            temp_r7->unk14 = (f32) temp_r7->unk1C;
            temp_r7->unkA = (s16) temp_r7->unk10;
            temp_r7->unk8 = 0xFF;
            temp_r7->unkE = 0xFF;
            temp_r7->unkC = 0x3C;
        }
        if ((u16) lbl_814AD4F6 != 2) {
            temp_r3_2 = &lbl_814C4E20 + (lbl_814AD4F0 * 0x2DA4) + (lbl_814AD4F4 * 0x5C);
            temp_r3_2->unk8 = 0;
            temp_r3_2->unkE = 0;
        }
    }
}

/* 0x813204B0 */
void fn_813204B0(void) {
    s32 sp10;
    s32 spC;
    s32 sp8;
    void *temp_r7;

    if ((s32) lbl_814AD4F8 != 0) {
        lbl_814AD4F8 = 0;
        if ((s32) lbl_814AD500 != 0) {
            lbl_814AD500 = 0;
            lbl_814AD504 = 0;
        }
        sp8 = lbl_814C4E20.unk15010;
        spC = lbl_814C4E20.unk15014;
        temp_r7 = &lbl_814C4E20 + (lbl_814AD4F0 * 0x2DA4) + (lbl_814AD4F4 * 0x5C);
        sp10 = lbl_814C4E20.unk15018;
        temp_r7->unk20 = sp8;
        temp_r7->unk24 = spC;
        temp_r7->unk28 = sp10;
        temp_r7->unk44 = sp8;
        temp_r7->unk48 = spC;
        temp_r7->unk4C = sp10;
        temp_r7->unk6 = 0;
        temp_r7->unkC = 0x28;
        temp_r7->unk8 = 0xFF;
        temp_r7->unkE = 0xFF;
    }
}

/* 0x8132056C */
void fn_8132056C(s32 *arg0, s32 arg1) {
    void *var_r9;
    s32 *var_r3;
    s32 *var_r5;
    s32 temp_r30_2;
    s32 temp_r30_3;
    s32 temp_r7;
    s32 temp_r8;
    s32 var_ctr;
    s32 var_r10;
    u32 temp_r30;
    u32 var_r12;
    u32 var_r31;
    u8 *temp_r6;
    void *temp_r11;

    var_r3 = arg0;
    var_r9 = &lbl_814C4E20;
    var_r12 = 0U;
    var_r10 = 0;
    do {
        var_ctr = 0x7F;
        var_r5 = var_r3;
        temp_r6 = arg1 + var_r12;
        temp_r7 = lbl_814AD4E8 + var_r10;
        temp_r8 = lbl_814AD4EC + var_r10;
        var_r31 = 0;
loop_2:
        temp_r11 = var_r9 + (*var_r5 * 0x5C);
        temp_r11->unk0 = 0;
        temp_r11->unk6 = (s16) temp_r11->unk0;
        temp_r11->unkC = 0x28;
        temp_r30 = (s8) *temp_r6 * 4;
        if ((var_r31 >= temp_r30) && (var_r31 <= (u32) (temp_r30 + 0xF))) {
            temp_r11->unkE = 0xFF;
            temp_r11->unk44 = (s32) temp_r11->unk20;
            temp_r11->unk48 = (s32) temp_r11->unk24;
            temp_r11->unk4C = (s32) temp_r11->unk28;
            temp_r11->unk38 = (s32) temp_r11->unk2C;
            temp_r11->unk3C = (s32) temp_r11->unk30;
            temp_r11->unk40 = (s32) temp_r11->unk34;
            temp_r11->unk50 = (f32) *(temp_r7 + ((var_r31 - ((s8) *temp_r6 * 4)) * 2));
            temp_r11->unk54 = (f32) *(temp_r8 + ((var_r31 - ((s8) *temp_r6 * 4)) * 2));
            temp_r11->unk58 = 0.0f;
        } else {
            temp_r11->unkE = 0;
            temp_r11->unk44 = (s32) temp_r11->unk20;
            temp_r11->unk48 = (s32) temp_r11->unk24;
            temp_r11->unk4C = (s32) temp_r11->unk28;
            temp_r11->unk38 = (s32) temp_r11->unk2C;
            temp_r11->unk3C = (s32) temp_r11->unk30;
            temp_r11->unk40 = (s32) temp_r11->unk34;
            if (var_r31 < (u32) ((s8) *temp_r6 * 4)) {
                temp_r30_2 = (var_r31 * 2) & 6;
                temp_r11->unk50 = (f32) *(temp_r7 + temp_r30_2);
                temp_r11->unk54 = (f32) ((f32) *(temp_r8 + temp_r30_2) - 512.0f);
            }
            if (var_r31 > (u32) (((s8) *temp_r6 * 4) + 0xF)) {
                temp_r30_3 = ((var_r31 * 2) & 6) + 0x18;
                temp_r11->unk50 = (f32) *(temp_r7 + temp_r30_3);
                temp_r11->unk54 = (f32) (512.0f + (f32) *(temp_r8 + temp_r30_3));
            }
            temp_r11->unk58 = 0.0f;
        }
        var_r5 += 8;
        var_r31 += 1;
        var_ctr -= 1;
        if (var_ctr != 0) {
            goto loop_2;
        }
        var_r12 += 1;
        var_r3 += 0x3F8;
        var_r9 += 0x2DA4;
        var_r10 += 0x20;
    } while (var_r12 < 2U);
}

/* 0x813207C8 */
void CardIcon_ResetFiles(void) {
    s32 sp24;
    s32 sp20;
    s32 sp1C;
    s32 sp18;
    f32 sp14;
    f32 sp10;
    void *var_r28;
    void *var_r29;
    f32 temp_r4;
    s16 temp_r3;
    s16 temp_r3_3;
    s32 temp_r3_2;
    s32 var_r26;
    s32 var_r27;
    u16 temp_r25;

    var_r28 = &lbl_814C4E20;
    var_r27 = 0;
    do {
        var_r26 = 0;
        var_r29 = var_r28;
loop_2:
        sp1C = lbl_8137EDE4.unk0;
        sp20 = lbl_8137EDE4.unk4;
        sp24 = lbl_8137EDE4.unk8;
        var_r29->unk20 = (s32) lbl_8137EDE4.unk0;
        var_r29->unk24 = (s32) lbl_8137EDE4.unk4;
        var_r29->unk28 = (s32) lbl_8137EDE4.unk8;
        temp_r25 = Rand(lbl_8137EDE4.unk0);
        temp_r3 = (s16) temp_r25;
        sp10 = lbl_8137EDF0.unk0;
        sp14 = lbl_8137EDF0.unk4;
        sp18 = lbl_8137EDF0.unk8;
        sp10 = 350.0f * SinS(temp_r3, lbl_8137EDF0.unk0);
        sp14 = 350.0f * CosS((s16) temp_r25);
        var_r29->unk2C = sp10;
        var_r29->unk30 = sp14;
        var_r29->unk34 = sp18;
        var_r29->unk44 = (s32) var_r29->unk20;
        var_r29->unk48 = (s32) var_r29->unk24;
        var_r29->unk4C = (s32) var_r29->unk28;
        temp_r4 = var_r29->unk2C;
        var_r29->unk38 = temp_r4;
        var_r29->unk3C = (f32) var_r29->unk30;
        temp_r3_2 = var_r29->unk34;
        var_r29->unk40 = temp_r3_2;
        var_r29->unk8 = 0;
        var_r29->unk6 = 0;
        temp_r3_3 = Rand(temp_r3_2, (bitwise void *) temp_r4) & 0xF;
        var_r29->unk0 = temp_r3_3;
        var_r26 += 1;
        var_r29->unkC = (s16) (temp_r3_3 + 0x21);
        var_r29 += 0x5C;
        if (var_r26 < 0x7F) {
            goto loop_2;
        }
        var_r27 += 1;
        var_r28 += 0x2DA4;
    } while (var_r27 < 2);
    lbl_814AD4FC = 1;
}

/* 0x81320920 */
void fn_81320920(s32 arg0) {
    s32 sp40;
    f32 sp3C;
    f32 sp38;
    s32 sp20;
    f32 sp1C;
    f32 sp18;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s32 temp_r10;
    s32 var_ctr;
    s32 var_ctr_2;
    u32 var_r7;
    void *temp_r3;
    void *temp_r4;
    void *var_r5;
    void *var_r6;

    var_r5 = &lbl_814C4E20 + (arg0 * 0x2DA4);
    var_ctr = 0x7F;
    var_r6 = var_r5;
    var_r7 = 0U;
    do {
        var_r6->unkE = 0;
        if ((s16) var_r6->unk8 == 0) {
            var_r7 += 1;
        }
        var_r6 += 0x5C;
        var_ctr -= 1;
    } while (var_ctr != 0);
    if (var_r7 == 0x7FU) {
        temp_r10 = arg0 << 5;
        var_ctr_2 = 0x7F;
        sp18 = lbl_8137EDFC.unk0;
        sp1C = lbl_8137EDFC.unk4;
        temp_r3 = lbl_814AD4E8 + temp_r10;
        temp_r4 = lbl_814AD4EC + temp_r10;
        sp20 = lbl_8137EDFC.unk8;
        spC = lbl_8137EE08.unk0;
        sp10 = lbl_8137EE08.unk4;
        sp14 = lbl_8137EE08.unk8;
        do {
            sp38 = sp18;
            sp3C = sp1C;
            sp40 = sp20;
            sp38 = (f32) ((s32) (temp_r3->unkA + temp_r3->unkC) / 2);
            sp3C = (f32) ((s32) (temp_r4->unkA + temp_r4->unk12) / 2);
            var_r5->unk20 = sp38;
            var_r5->unk24 = sp3C;
            var_r5->unk28 = sp20;
            var_r5->unk2C = spC;
            var_r5->unk30 = sp10;
            var_r5->unk34 = sp14;
            var_r5->unk44 = (f32) var_r5->unk20;
            var_r5->unk48 = (f32) var_r5->unk24;
            var_r5->unk4C = (s32) var_r5->unk28;
            var_r5->unk38 = (s32) var_r5->unk2C;
            var_r5->unk3C = (s32) var_r5->unk30;
            var_r5->unk40 = (s32) var_r5->unk34;
            var_r5->unk8 = 0;
            var_r5->unk6 = 0;
            var_r5 += 0x5C;
            var_ctr_2 -= 1;
        } while (var_ctr_2 != 0);
    }
}

/* 0x81320ABC */
s32 fn_81320ABC(void) {
    void *var_r4;
    void *var_r6;
    void *var_r6_2;
    void *var_r7_2;
    s32 var_ctr;
    s32 var_ctr_2;
    s32 var_ctr_3;
    s32 var_r0;
    s32 var_r31;
    s32 var_r7;
    u32 var_r4_2;
    u32 var_r5_2;
    void *temp_r30;
    void *var_r3;
    void *var_r5;
    void *var_r8;

    var_r31 = 0;
    temp_r30 = CardThread_GetSlots();
    if (Menu_CardFadeDone() == 0) {
        return 0;
    }
    var_r6 = &lbl_814C4E20;
    var_r5 = temp_r30;
    var_r7 = 0;
    do {
        if ((s16) var_r5->unk2 == 4) {
            var_ctr = 0x7F;
            var_r4 = var_r6;
            do {
                if ((s16) var_r4->unk6 == (s16) var_r4->unkC) {
                    var_r0 = 1;
                } else {
                    var_r0 = 0;
                }
                if (var_r0 != 0) {
                    var_r31 += 1;
                }
                var_r4 += 0x5C;
                var_ctr -= 1;
            } while (var_ctr != 0);
        } else {
            var_r31 += 0x7F;
        }
        var_r7 += 1;
        var_r5 += 0x14;
        var_r6 += 0x2DA4;
    } while (var_r7 < 2);
    if ((var_r31 == 0xFE) && ((s32) lbl_814AD500 == 0)) {
        var_r3 = CardThread_GetSlots();
        var_r7_2 = &lbl_814C4E20;
        var_r5_2 = 0U;
        do {
            if ((s16) var_r3->unk2 == 4) {
                var_r4_2 = 0;
                if (0x7FU > 8U) {
                    var_ctr_2 = 0xF;
                    var_r6_2 = var_r7_2;
                    do {
                        var_r4_2 += 8;
                        var_r6_2->unk44 = (s32) var_r6_2->unk20;
                        var_r6_2->unk48 = (s32) var_r6_2->unk24;
                        var_r6_2->unk4C = (s32) var_r6_2->unk28;
                        var_r6_2->unk38 = (s32) var_r6_2->unk2C;
                        var_r6_2->unk3C = (s32) var_r6_2->unk30;
                        var_r6_2->unk40 = (s32) var_r6_2->unk34;
                        var_r6_2->unkA0 = (s32) var_r6_2->unk7C;
                        var_r6_2->unkA4 = (s32) var_r6_2->unk80;
                        var_r6_2->unkA8 = (s32) var_r6_2->unk84;
                        var_r6_2->unk94 = (s32) var_r6_2->unk88;
                        var_r6_2->unk98 = (s32) var_r6_2->unk8C;
                        var_r6_2->unk9C = (s32) var_r6_2->unk90;
                        var_r6_2->unkFC = (s32) var_r6_2->unkD8;
                        var_r6_2->unk100 = (s32) var_r6_2->unkDC;
                        var_r6_2->unk104 = (s32) var_r6_2->unkE0;
                        var_r6_2->unkF0 = (s32) var_r6_2->unkE4;
                        var_r6_2->unkF4 = (s32) var_r6_2->unkE8;
                        var_r6_2->unkF8 = (s32) var_r6_2->unkEC;
                        var_r6_2->unk158 = (s32) var_r6_2->unk134;
                        var_r6_2->unk15C = (s32) var_r6_2->unk138;
                        var_r6_2->unk160 = (s32) var_r6_2->unk13C;
                        var_r6_2->unk14C = (s32) var_r6_2->unk140;
                        var_r6_2->unk150 = (s32) var_r6_2->unk144;
                        var_r6_2->unk154 = (s32) var_r6_2->unk148;
                        var_r6_2->unk1B4 = (s32) var_r6_2->unk190;
                        var_r6_2->unk1B8 = (s32) var_r6_2->unk194;
                        var_r6_2->unk1BC = (s32) var_r6_2->unk198;
                        var_r6_2->unk1A8 = (s32) var_r6_2->unk19C;
                        var_r6_2->unk1AC = (s32) var_r6_2->unk1A0;
                        var_r6_2->unk1B0 = (s32) var_r6_2->unk1A4;
                        var_r6_2->unk210 = (s32) var_r6_2->unk1EC;
                        var_r6_2->unk214 = (s32) var_r6_2->unk1F0;
                        var_r6_2->unk218 = (s32) var_r6_2->unk1F4;
                        var_r6_2->unk204 = (s32) var_r6_2->unk1F8;
                        var_r6_2->unk208 = (s32) var_r6_2->unk1FC;
                        var_r6_2->unk20C = (s32) var_r6_2->unk200;
                        var_r6_2->unk26C = (s32) var_r6_2->unk248;
                        var_r6_2->unk270 = (s32) var_r6_2->unk24C;
                        var_r6_2->unk274 = (s32) var_r6_2->unk250;
                        var_r6_2->unk260 = (s32) var_r6_2->unk254;
                        var_r6_2->unk264 = (s32) var_r6_2->unk258;
                        var_r6_2->unk268 = (s32) var_r6_2->unk25C;
                        var_r6_2->unk2C8 = (s32) var_r6_2->unk2A4;
                        var_r6_2->unk2CC = (s32) var_r6_2->unk2A8;
                        var_r6_2->unk2D0 = (s32) var_r6_2->unk2AC;
                        var_r6_2->unk2BC = (s32) var_r6_2->unk2B0;
                        var_r6_2->unk2C0 = (s32) var_r6_2->unk2B4;
                        var_r6_2->unk2C4 = (s32) var_r6_2->unk2B8;
                        var_r6_2 += 0x2E0;
                        var_ctr_2 -= 1;
                    } while (var_ctr_2 != 0);
                }
                var_r8 = var_r7_2 + (var_r4_2 * 0x5C);
                var_ctr_3 = 0x7F - var_r4_2;
                if (var_r4_2 < 0x7FU) {
                    do {
                        var_r8->unk44 = (s32) var_r8->unk20;
                        var_r8->unk48 = (s32) var_r8->unk24;
                        var_r8->unk4C = (s32) var_r8->unk28;
                        var_r8->unk38 = (s32) var_r8->unk2C;
                        var_r8->unk3C = (s32) var_r8->unk30;
                        var_r8->unk40 = (s32) var_r8->unk34;
                        var_r8 += 0x5C;
                        var_ctr_3 -= 1;
                    } while (var_ctr_3 != 0);
                }
            }
            var_r5_2 += 1;
            var_r3 += 0x14;
            var_r7_2 += 0x2DA4;
        } while (var_r5_2 < 2U);
        return 1;
    }
    return 0;
}

/* 0x81320DD0 */
void *fn_81320DD0(void *arg0, s32 arg1) {
    if (arg0 != (void *)0x10000) {
        if (arg0 < 0x10000) {
            if (arg0 != (void *)0x4000) {
                if (arg0 < 0x4000) {
                    if (arg0 != (void *)0x2000) {
                        return arg0;
                    }
                    if (arg1 != 2) {
                        if (arg1 < 2) {
                            if (arg1 != 0) {
                                if (arg1 < 0) {
                                    return arg0;
                                }
                                return &lbl_814AD018;
                            }
                            return &lbl_814AD020;
                        }
                        if (arg1 < 4) {
                            return &lbl_814AD008;
                        }
                        /* Duplicate return node #79. Try simplifying control flow for better match */
                        return arg0;
                    }
                    return &lbl_814AD010;
                }
                if (arg0 != (void *)0x8000) {
                    return arg0;
                }
                if (arg1 != 2) {
                    if (arg1 < 2) {
                        if (arg1 != 0) {
                            if (arg1 < 0) {
                                return arg0;
                            }
                            return &lbl_814AD058;
                        }
                        return &lbl_814AD060;
                    }
                    if (arg1 < 4) {
                        return &lbl_814AD048;
                    }
                    /* Duplicate return node #79. Try simplifying control flow for better match */
                    return arg0;
                }
                return &lbl_814AD050;
            }
            if (arg1 != 2) {
                if (arg1 < 2) {
                    if (arg1 != 0) {
                        if (arg1 < 0) {
                            return arg0;
                        }
                        return &lbl_814AD038;
                    }
                    return &lbl_814AD040;
                }
                if (arg1 < 4) {
                    return &lbl_814AD028;
                }
                /* Duplicate return node #79. Try simplifying control flow for better match */
                return arg0;
            }
            return &lbl_814AD030;
        }
        if (arg0 != (void *)0x40000) {
            if (arg0 < 0x40000) {
                if (arg0 != (void *)0x20000) {
                    return arg0;
                }
                if (arg1 != 2) {
                    if (arg1 < 2) {
                        if (arg1 != 0) {
                            if (arg1 < 0) {
                                return arg0;
                            }
                            return &lbl_814AD098;
                        }
                        return &lbl_814AD0A0;
                    }
                    if (arg1 < 4) {
                        return &lbl_814AD088;
                    }
                    /* Duplicate return node #79. Try simplifying control flow for better match */
                    return arg0;
                }
                return &lbl_814AD090;
            }
            /* Duplicate return node #79. Try simplifying control flow for better match */
            return arg0;
        }
        if (arg1 != 2) {
            if (arg1 < 2) {
                if (arg1 != 0) {
                    if (arg1 < 0) {
                        return arg0;
                    }
                    return &lbl_814AD0B8;
                }
                return &lbl_814AD0C0;
            }
            if (arg1 < 4) {
                return &lbl_814AD0A8;
            }
            /* Duplicate return node #79. Try simplifying control flow for better match */
            return arg0;
        }
        return &lbl_814AD0B0;
    }
    if (arg1 != 2) {
        if (arg1 < 2) {
            if (arg1 != 0) {
                if (arg1 < 0) {
                    return arg0;
                }
                return &lbl_814AD078;
            }
            return &lbl_814AD080;
        }
        if (arg1 < 4) {
            return &lbl_814AD068;
        }
        return arg0;
    }
    return &lbl_814AD070;
}

/* 0x81320FD8 */
void fn_81320FD8(void *arg0, u8 arg1, s32 arg2) {
    s32 temp_r30;
    s32 temp_r31;
    u16 temp_r25;
    void *temp_r26;
    void *temp_r26_2;
    void *temp_r26_3;
    void *temp_r3;
    void *temp_r4;
    void *temp_r5;
    void *temp_r6;
    void *temp_r6_2;

    temp_r26 = CardThread_GetSlots();
    if ((s16) arg0->unk74 > 0) {
        temp_r3 = arg0->unk68->unk10;
        temp_r31 = temp_r3->unk30;
        temp_r30 = temp_r3->unk1E0;
        if (((s32) arg1 == Card_GetSlot(temp_r3)) && (arg2 == Card_GetSelectedFile()) && ((s32) lbl_814AD4FC == 0)) {
            temp_r26_2 = temp_r26 + (arg1 * 0x14);
            temp_r6 = arg0->unk68->unk10;
            temp_r6->unk30 = fn_81320DD0(temp_r26_2->unk8, 0, arg1);
            arg0->unk68->unk10->unk1E0 = fn_81320DD0(temp_r26_2->unk8, 0, arg1, temp_r6);
        } else {
            temp_r26_3 = temp_r26 + (arg1 * 0x14);
            temp_r6_2 = arg0->unk68->unk10;
            temp_r6_2->unk30 = fn_81320DD0(temp_r26_3->unk8, 1, arg1);
            arg0->unk68->unk10->unk1E0 = fn_81320DD0(temp_r26_3->unk8, 1, arg1, temp_r6_2);
        }
        J3D_DrawPacket(arg0, arg0->unk68->unk8 + 0x30);
        J3D_DrawPacket(arg0, arg0->unk68->unk8 + 0xF0);
        arg0->unk68->unk10->unk30 = temp_r31;
        temp_r5 = arg0->unk68->unk10;
        temp_r5->unk1E0 = temp_r30;
        if (CardDraw_IconFrameCount(arg1, arg2, temp_r5) != 0) {
            temp_r4 = arg0->unk68->unk10;
            temp_r25 = temp_r4->unkE0;
            temp_r4->unkE0 = 0xFFFFU;
            J3D_DrawPacket(arg0, arg0->unk68->unk8 + 0x90);
            arg0->unk68->unk10->unkE0 = temp_r25;
        }
    }
}

/* 0x81321168 */
void fn_81321168(void *arg0, u8 arg1, s32 arg2) {
    s32 temp_r30;
    s32 temp_r31;
    void *temp_r28;
    void *temp_r28_2;
    void *temp_r28_3;
    void *temp_r3;
    void *temp_r6;
    void *temp_r6_2;

    temp_r28 = CardThread_GetSlots();
    if ((s16) arg0->unk74 > 0) {
        temp_r3 = arg0->unk68->unk10;
        temp_r31 = temp_r3->unk30;
        temp_r30 = temp_r3->unk108;
        if (((s32) arg1 == Card_GetSlot(temp_r3)) && (arg2 == Card_GetSelectedFile()) && ((s32) lbl_814AD4FC == 0)) {
            temp_r28_2 = temp_r28 + (arg1 * 0x14);
            temp_r6 = arg0->unk68->unk10;
            temp_r6->unk30 = fn_81320DD0(temp_r28_2->unk8, 2, arg1);
            arg0->unk68->unk10->unk108 = fn_81320DD0(temp_r28_2->unk8, 2, arg1, temp_r6);
        } else {
            temp_r28_3 = temp_r28 + (arg1 * 0x14);
            temp_r6_2 = arg0->unk68->unk10;
            temp_r6_2->unk30 = fn_81320DD0(temp_r28_3->unk8, 3, arg1);
            arg0->unk68->unk10->unk108 = fn_81320DD0(temp_r28_3->unk8, 3, arg1, temp_r6_2);
        }
        J3D_Draw(arg0);
        arg0->unk68->unk10->unk30 = temp_r31;
        arg0->unk68->unk10->unk108 = temp_r30;
    }
}

/* 0x8132128C */
void fn_8132128C(void *arg0) {
    s32 temp_r30;
    s32 temp_r31;
    void *temp_r28;
    void *temp_r6;

    temp_r28 = CardThread_GetSlots();
    if ((s16) arg0->unk74 > 0) {
        temp_r6 = arg0->unk68->unk10;
        temp_r31 = temp_r6->unk30;
        temp_r30 = temp_r6->unk1E0;
        arg0->unk68->unk10->unk30 = fn_81320DD0((temp_r28 + (lbl_814AD4F0 * 0x14))->unk8, 1, lbl_814AD4F0, temp_r6);
        arg0->unk68->unk10->unk1E0 = fn_81320DD0((temp_r28 + (lbl_814AD4F0 * 0x14))->unk8, 1, lbl_814AD4F0);
        J3D_DrawPacket(arg0, arg0->unk68->unk8 + 0x30);
        J3D_DrawPacket(arg0, arg0->unk68->unk8 + 0xF0);
        arg0->unk68->unk10->unk30 = temp_r31;
        arg0->unk68->unk10->unk1E0 = temp_r30;
    }
}

/* 0x81321378 */
void fn_81321378(u8 arg0, u8 arg1) {
    s16 *var_r22_2;
    s16 *var_r24;
    s16 *var_r25_2;
    s16 temp_r3_5;
    s16 temp_r4_2;
    s16 temp_r5;
    s32 temp_r3_3;
    s32 temp_r4;
    s32 temp_r6_2;
    s32 temp_r7;
    s32 var_r21;
    s32 var_r22;
    u16 *temp_r3;
    u16 *var_r25;
    u8 temp_r24;
    u8 temp_r6;
    u8 var_r0;
    void *temp_r3_2;
    void *temp_r3_4;
    void *var_r26;
    void *var_r26_2;
    void *var_r27;
    void *var_r28;

    temp_r3 = CardThread_GetFiles(&lbl_814C4E20);
    temp_r4 = arg0 * 0x3B88;
    var_r28 = (&lbl_814C4E20 + 0x10000 + temp_r4) - 0x2D38;
    var_r27 = &lbl_814C4E20 + (arg0 * 0x2DA4);
    var_r25 = temp_r3 + (arg0 * 0x5F4);
    var_r22 = 0;
    var_r26 = &lbl_814C4E20 + temp_r4 + 0x5BB8;
    do {
        if ((arg0 == (u8) lbl_814AD4F0) && (var_r22 == (s32) lbl_814AD4F4) && ((s32) lbl_814AD4F8 != 0)) {
            var_r28->unk74 = 0;
        } else if (((s32) arg0 == Card_GetSlot()) && (var_r22 == Card_GetSelectedFile()) && ((s32) lbl_814AD4FC == 0)) {
            var_r28->unk74 = 0;
        } else {
            var_r28->unk74 = (s16) (u8) ((arg1 * var_r27->unk8) / 255);
        }
        var_r26->unk74 = (s16) (u8) ((arg1 * var_r27->unk8) / 255);
        if ((u16) *var_r25 != 0) {
            fn_81320FD8(var_r28, arg0, var_r22);
        } else {
            fn_81321168(var_r26, arg0, var_r22);
        }
        var_r22 += 1;
        var_r28 += 0x78;
        var_r27 += 0x5C;
        var_r26 += 0x78;
        var_r25 += 0xC;
    } while (var_r22 < 0x7F);
    if ((s32) lbl_814AD4F8 != 0) {
        if (((s32) lbl_814AD500 == 0) && ((u16) *(temp_r3 + ((lbl_814AD4F0 * 0x5F4) + (lbl_814AD4F4 * 0xC))) != 0)) {
            temp_r7 = lbl_814AD4F0 * 0x3B88;
            temp_r6 = (lbl_814C4E20.unk14FF8 * arg1) / 255;
            temp_r3_2 = &lbl_814C4E20 + 0x10000 + temp_r7 + (lbl_814AD4F4 * 0x78);
            temp_r3_2->unk-2CC4 = (s16) temp_r6;
            fn_81320FD8(temp_r3_2 - 0x2D38, lbl_814AD4F0, (s32) temp_r6, temp_r7);
        }
    } else if ((s32) lbl_814AD4FC == 0) {
        temp_r24 = Card_GetSlot();
        temp_r3_3 = Card_GetSelectedFile();
        if ((u16) *(temp_r3 + ((temp_r24 * 0x5F4) + (temp_r3_3 * 0xC))) != 0) {
            temp_r6_2 = temp_r24 * 0x3B88;
            temp_r3_4 = &lbl_814C4E20 + 0x10000 + temp_r6_2 + (temp_r3_3 * 0x78);
            temp_r3_4->unk-2CC4 = (s16) (u8) ((arg1 * (&lbl_814C4E20 + (temp_r24 * 0x2DA4) + (temp_r3_3 * 0x5C))->unk8) / 255);
            fn_81320FD8(temp_r3_4 - 0x2D38, temp_r24, temp_r6_2);
        }
    }
    if ((s32) lbl_814AD500 != 0) {
        if ((s32) lbl_814AD504 == 0) {
            lbl_814C4E20.unk14A4C = (s16) (u8) ((lbl_814C4E20.unk14FF8 * arg1) / 255);
            fn_81320FD8(&lbl_814C4E20 + 0x149D8, lbl_814AD4F0, -1);
            return;
        }
        var_r21 = 0;
        var_r22_2 = &lbl_814C4E20 + 0x1507C;
        var_r24 = &lbl_814C4E20 + 0x1504C;
        var_r25_2 = &lbl_814C4E20 + 0x15064;
        var_r26_2 = &lbl_814C4E20 + 0x14A50;
        do {
            temp_r4_2 = *var_r24;
            temp_r3_5 = *var_r22_2;
            if (temp_r4_2 > temp_r3_5) {
                temp_r5 = *var_r25_2;
                if ((s32) (temp_r4_2 - temp_r3_5) <= temp_r5) {
                    var_r0 = (u8) ((s32) (arg1 * (temp_r3_5 + (temp_r5 - temp_r4_2))) / temp_r5);
                } else {
                    var_r0 = 0;
                }
            } else {
                var_r0 = 0;
            }
            if (var_r0 != 0) {
                var_r26_2->unk74 = (s16) var_r0;
                fn_8132128C(var_r26_2);
            }
            var_r21 += 1;
            var_r22_2 += 2;
            var_r24 += 2;
            var_r25_2 += 2;
            var_r26_2 += 0x78;
        } while (var_r21 < 0xC);
    }
}

/* 0x813216F4 */
void fn_813216F4(void) {
    fn_81321378();
}

/* 0x81321714 */
s32 fn_81321714(void) {
    if ((s32) lbl_814AD4FC != 0) {
        return 0;
    }
    return 1;
}
