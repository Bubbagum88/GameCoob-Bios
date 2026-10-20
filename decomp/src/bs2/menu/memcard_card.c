// Non-matching: functionally equivalent decompilation
//
// Memory Card worker thread (0x8131BF50..0x8131ECC4). The UI posts commands
// (cmd | slot<<16 | file<<24) to a message queue; the thread mounts cards,
// reads the directory, icons and banners, and copies / moves / erases files
// and formats cards with the SDK CARD API, then replies with
// (cmd | result<<8 | slot<<16 | file<<24).
//
// Semi-automatic: m2c output with the working names from
// config/PAL10/names_menu.txt applied, FourCC pane tags written as
// character constants and an address comment before each function.
// Struct accesses are still raw offsets (unkXX); see include/bs2/menu.h
// and docs/menu.md for the layouts that are known.

#include "bs2/menu.h"

/* 0x8131BF50 */
s32 Card_IsBrowsing(void) {
    if ((s16) sCardUiState == 1) {
        return 1;
    }
    return 0;
}

/* 0x8131BF6C */
void *CardThread_GetFiles(void) {
    return &lbl_814B72A8;
}

/* 0x8131BF78 */
void *CardThread_GetSlots(void) {
    return &gCardSlots;
}

/* 0x8131BF84 */
u8 CardThread_IsEnabled(void) {
    return sCardEnabled;
}

/* 0x8131BF8C */
s32 CardThread_GetCmd(void) {
    return sCardCmd;
}

/* 0x8131BF94 */
s32 CardThread_GetResult(void) {
    return sCardResult;
}

/* 0x8131BF9C */
s32 CardThread_GetDestResult(void) {
    return sCardDestResult;
}

/* 0x8131BFA4 */
u8 CardThread_GetDestFile(void) {
    return sCardDestFile;
}

/* 0x8131BFAC */
void CardThread_Format(u8 arg0) {
    s32 temp_r3;

    sCardCmdSlot = arg0;
    temp_r3 = (arg0 << 0x10) & 0xFF0000;
    sCardResult = -0x15;
    sCardCmd = 1;
    sCardDestResult = -0x15;
    CardThread_CmdQueue(temp_r3, 1);
    OSSendMessage(temp_r3 | 1, 0);
}

/* 0x8131C000 */
void CardThread_Copy(u8 arg0, s16 arg1) {
    sCardCmdSlot = arg0;
    sCardResult = -0x15;
    sCardDestResult = -0x15;
    sCardCmd = 2;
    CardThread_CmdQueue(2, arg1);
    OSSendMessage(((arg0 << 0x10) & 0x10000 & ~0xFF000000) | (arg1 << 0x18) | 2, 0);
}

/* 0x8131C05C */
void CardThread_Move(u8 arg0, s16 arg1) {
    sCardCmdSlot = arg0;
    sCardResult = -0x15;
    sCardDestResult = -0x15;
    sCardCmd = 3;
    CardThread_CmdQueue(3, arg1);
    OSSendMessage(((arg0 << 0x10) & 0x10000 & ~0xFF000000) | (arg1 << 0x18) | 3, 0);
}

/* 0x8131C0B8 */
void CardThread_Erase(u8 arg0, s16 arg1) {
    sCardCmdSlot = arg0;
    sCardResult = -0x15;
    sCardDestResult = -0x15;
    sCardCmd = 4;
    CardThread_CmdQueue(4, arg1);
    OSSendMessage(((arg0 << 0x10) & 0x10000 & ~0xFF000000) | (arg1 << 0x18) | 4, 0);
}

/* 0x8131C114 */
void CardThread_Enable(void) {
    CardThread_CmdQueue();
    OSSendMessage(0xA, 0);
}

/* 0x8131C140 */
void CardThread_UnmountAll(void) {
    sCardCmdSlot = 0;
    sCardCmd = 9;
    sCardResult = -0x15;
    CardThread_CmdQueue();
    OSSendMessage(9, 0);
    sCardCmd = 9;
    sCardCmdSlot = 1;
    sCardResult = -0x15;
    CardThread_CmdQueue(0x10000);
    OSSendMessage(0x10009, 0);
    CardThread_CmdQueue();
    OSSendMessage(0xB, 0);
}

/* 0x8131C1CC */
s32 Card_MapError(s8 arg0) {
    switch (arg0) {
    case 0:
        return 0;
    case -2:
        return -0x1C;
    case -3:
        return -0x1C;
    case -4:
        return -0x1C;
    case -5:
        return -0x1C;
    case -6:
        return -0x1C;
    case -7:
        return -0x18;
    case -8:
        return -0x19;
    case -9:
        return -0x19;
    case -10:
        return -0x1C;
    case -11:
        return -0x1C;
    case -12:
        return -0x1C;
    case -13:
        return -0x1C;
    case -27:
        return -0x1B;
    default:
        return -0x1C;
    }
}

/* 0x8131C268 */
void CardThread_Poll(void) {
    u32 sp8;
    s16 temp_r29;
    s32 temp_r3;
    s32 temp_r3_2;
    u8 temp_r0;

    if ((u8) sCardEnabled == 1) {
        temp_r3 = CARDProbe(0U);
        if ((temp_r3 != 0) && ((s16) gCardSlots.unk2 < 2)) {
            sCardCmdSlot = 0;
            sCardCmd = 0;
            sCardResult = -0x15;
            CardThread_CmdQueue();
            if (OSSendMessage(0, 0) != 0) {
                gCardSlots.unk2 = 3;
                gCardSlots.unk1 = 0;
            }
        } else if ((temp_r3 == 0) && ((s16) gCardSlots.unk2 > 2)) {
            sCardCmdSlot = 0;
            sCardCmd = 9;
            sCardResult = -0x15;
            CardThread_CmdQueue(9);
            if (OSSendMessage(9, 0) != 0) {
                gCardSlots.unk1 = 0;
            }
        }
    }
    if ((u8) sCardEnabled == 1) {
        temp_r3_2 = CARDProbe(1U);
        if ((temp_r3_2 != 0) && ((s16) gCardSlots.unk16 < 2)) {
            sCardCmdSlot = 1;
            sCardCmd = 0;
            sCardResult = -0x15;
            CardThread_CmdQueue();
            if (OSSendMessage(0x10000, 0) != 0) {
                gCardSlots.unk16 = 3;
                gCardSlots.unk15 = 0;
            }
        } else if ((temp_r3_2 == 0) && ((s16) gCardSlots.unk16 > 2)) {
            sCardCmdSlot = 1;
            sCardCmd = 9;
            sCardResult = -0x15;
            CardThread_CmdQueue(9, 0x10000);
            if (OSSendMessage(0x10009, 0) != 0) {
                gCardSlots.unk15 = 0;
            }
        }
    }
loop_38:
    CardThread_ReplyQueue();
    if (OSReceiveMessage(&sp8, 0) != 0) {
        temp_r29 = (sp8 >> 0x10U) & 1;
        switch ((s8) (u8) sp8) {                    /* switch 1; irregular */
        case 10:                                    /* switch 1 */
        case 11:                                    /* switch 1 */
            sCardEnabled = (u8) (sp8 >> 8U);
            break;
        case 12:                                    /* switch 1 */
            lbl_814AD4D8 = 1;
            lbl_814AD4DC = 1;
            break;
        default:                                    /* switch 1 */
            temp_r0 = (u8) (sp8 >> 8U);
            switch ((s8) temp_r0) {                 /* switch 2 */
            case -27:                               /* switch 2 */
            case -7:                                /* switch 2 */
            case 0:                                 /* switch 2 */
                *(&gCardSlots + ((temp_r29 * 0x14) + 2)) = 4;
                break;
            case -9:                                /* switch 2 */
            case -8:                                /* switch 2 */
                *(&gCardSlots + ((temp_r29 * 0x14) + 2)) = 4;
                break;
            case -2:                                /* switch 2 */
                *(&gCardSlots + ((temp_r29 * 0x14) + 2)) = 5;
                break;
            case -6:                                /* switch 2 */
                *(&gCardSlots + ((temp_r29 * 0x14) + 2)) = 6;
                break;
            case -13:                               /* switch 2 */
                *(&gCardSlots + ((temp_r29 * 0x14) + 2)) = 8;
                break;
            case -3:                                /* switch 2 */
                *(&gCardSlots + ((temp_r29 * 0x14) + 2)) = 0;
                break;
            default:                                /* switch 2 */
                *(&gCardSlots + ((temp_r29 * 0x14) + 2)) = 0xA;
                break;
            }
            sCardResult = Card_MapError((s8) temp_r0);
            if (((s8) (u8) sp8 != 0) && ((s8) (u8) sp8 != 9)) {
                sCardDestResult = Card_MapError((s8) (u8) (sp8 >> 8U));
                if ((s32) sCardCmdSlot != (s16) ((sp8 >> 0x10U) & 1)) {
                    sCardDestFile = (u8) (sp8 >> 0x18U);
                }
            }
            *(&gCardSlots + ((temp_r29 * 0x14) + 1)) = 1;
            lbl_814AD4DC = 0;
            break;
        }
        goto loop_38;
    }
}

/* 0x8131C5C4 */
void CardThread_Init(void) {
    s32 temp_r0_2;
    s32 temp_r10;
    s32 temp_r11;
    s32 temp_r5_2;
    s32 temp_r7_2;
    s32 temp_r8_2;
    s32 temp_r8_3;
    s32 temp_r8_4;
    s32 temp_r9_2;
    s32 var_ctr;
    s32 var_ctr_2;
    s32 var_ctr_3;
    s32 var_ctr_4;
    s32 var_r4;
    s32 var_r6_2;
    void *temp_r0;
    void *temp_r30;
    void *temp_r3;
    void *temp_r5;
    void *temp_r6;
    void *temp_r7;
    void *temp_r8;
    void *temp_r9;
    void *var_r12;
    void *var_r27;
    void *var_r28;
    void *var_r29;
    void *var_r4_2;
    void *var_r5;
    void *var_r5_2;
    void *var_r6;
    void *var_r7;
    void *var_r7_2;

    temp_r30 = HeapAlloc(0x594C00, &gCardSlots);
    temp_r3 = HeapAlloc(0x3F80);
    var_ctr = 0xF;
    var_r12 = temp_r30;
    var_r28 = temp_r3;
    var_r4 = 0;
    var_r29 = &gCardSlots + 0xCFA8;
    var_r27 = &gCardSlots + 0xD3A0;
    do {
        var_r29->unk0 = var_r12;
        var_r27->unk0 = var_r28;
        var_r29->unk4 = (void *) (var_r12 + 0x5A00);
        var_r27->unk4 = (void *) (var_r28 + 0x40);
        var_r29->unk8 = (void *) (var_r12 + 0xB400);
        var_r27->unk8 = (void *) (var_r28 + 0x80);
        temp_r9 = var_r12 + 0x1C200;
        var_r29->unkC = (void *) (var_r12 + 0x10E00);
        temp_r7 = var_r12 + 0x21C00;
        temp_r8 = var_r28 + 0x140;
        var_r27->unkC = (void *) (var_r28 + 0xC0);
        temp_r6 = var_r28 + 0x180;
        temp_r0 = var_r28 + 0x1C0;
        var_r29->unk10 = (void *) (var_r12 + 0x16800);
        temp_r5 = var_r12 + 0x27600;
        var_r12 += 0x2D000;
        var_r27->unk10 = (void *) (var_r28 + 0x100);
        var_r28 += 0x200;
        var_r4 += 8;
        var_r29->unk14 = temp_r9;
        var_r27->unk14 = temp_r8;
        var_r29->unk18 = temp_r7;
        var_r27->unk18 = temp_r6;
        var_r29->unk1C = temp_r5;
        var_r29 += 0x20;
        var_r27->unk1C = temp_r0;
        var_r27 += 0x20;
        var_ctr -= 1;
    } while (var_ctr != 0);
    var_r5 = temp_r30 + (var_r4 * 0x5A00);
    var_r6 = temp_r3 + (var_r4 << 6);
    var_r7 = (&gCardSlots + 0x10000 + (var_r4 * 4)) - 0x3058;
    var_ctr_2 = 0x7F - var_r4;
    if (var_r4 < 0x7F) {
        do {
            var_r7->unk0 = var_r5;
            var_r5 += 0x5A00;
            var_r7->unk3F8 = var_r6;
            var_r7 += 4;
            var_r6 += 0x40;
            var_ctr_2 -= 1;
        } while (var_ctr_2 != 0);
    }
    var_ctr_3 = 0xF;
    var_r6_2 = 0;
    var_r4_2 = &gCardSlots + 0xCFA8;
    var_r5_2 = &gCardSlots + 0xD3A0;
    do {
        temp_r8_2 = var_r6_2 + 0x7F;
        var_r4_2->unk1FC = (void *) (temp_r30 + (temp_r8_2 * 0x5A00));
        temp_r7_2 = var_r6_2 + 0x80;
        var_r5_2->unk1FC = (void *) (temp_r3 + (temp_r8_2 << 6));
        temp_r8_3 = var_r6_2 + 0x81;
        var_r4_2->unk200 = (void *) (temp_r30 + (temp_r7_2 * 0x5A00));
        var_r5_2->unk200 = (void *) (temp_r3 + (temp_r7_2 << 6));
        var_r4_2->unk204 = (void *) (temp_r30 + (temp_r8_3 * 0x5A00));
        temp_r11 = var_r6_2 + 0x82;
        var_r5_2->unk204 = (void *) (temp_r3 + (temp_r8_3 << 6));
        var_r4_2->unk208 = (void *) (temp_r30 + (temp_r11 * 0x5A00));
        temp_r10 = var_r6_2 + 0x83;
        var_r5_2->unk208 = (void *) (temp_r3 + (temp_r11 << 6));
        temp_r9_2 = var_r6_2 + 0x84;
        var_r4_2->unk20C = (void *) (temp_r30 + (temp_r10 * 0x5A00));
        var_r5_2->unk20C = (void *) (temp_r3 + (temp_r10 << 6));
        temp_r8_4 = var_r6_2 + 0x85;
        var_r4_2->unk210 = (void *) (temp_r30 + (temp_r9_2 * 0x5A00));
        var_r5_2->unk210 = (void *) (temp_r3 + (temp_r9_2 << 6));
        temp_r0_2 = var_r6_2 + 0x86;
        var_r4_2->unk214 = (void *) (temp_r30 + (temp_r8_4 * 0x5A00));
        var_r5_2->unk214 = (void *) (temp_r3 + (temp_r8_4 << 6));
        var_r4_2->unk218 = (void *) (temp_r30 + (temp_r0_2 * 0x5A00));
        var_r6_2 += 8;
        var_r5_2->unk218 = (void *) (temp_r3 + (temp_r0_2 << 6));
        var_r5_2 += 0x20;
        var_r4_2 += 0x20;
        var_ctr_3 -= 1;
    } while (var_ctr_3 != 0);
    var_r7_2 = (&gCardSlots + 0x10000 + (var_r6_2 * 4)) - 0x3058;
    var_ctr_4 = 0x7F - var_r6_2;
    if (var_r6_2 < 0x7F) {
        do {
            temp_r5_2 = var_r6_2 + 0x7F;
            var_r7_2->unk1FC = (void *) (temp_r30 + (temp_r5_2 * 0x5A00));
            var_r7_2->unk5F4 = (void *) (temp_r3 + (temp_r5_2 << 6));
            var_r7_2 += 4;
            var_r6_2 += 1;
            var_ctr_4 -= 1;
        } while (var_ctr_4 != 0);
    }
    lbl_814AD4C4 = HeapAlloc(0x5A00);
    lbl_814AD4C0 = HeapAlloc(0x40000);
    lbl_814AD4B0.unk0 = HeapAlloc(0xA000);
    lbl_814AD4B0.unk4 = HeapAlloc(0xA000);
    OSInitMessageQueue(&gCardSlots + 0xC10, &gCardSlots + 0xC50, 0x10);
    OSInitMessageQueue(&gCardSlots + 0xC30, &gCardSlots + 0xC90, 0x10);
    OSCreateThread(&gCardSlots + 0xCD0, CardThread_Main, 0, &gCardSlots + 0x8FE0, 0x8000, 0x11, 1);
    OSResumeThread(&gCardSlots + 0xCD0);
    CardThread_CmdQueue();
    OSSendMessage(0xA, 0);
}

/* 0x8131C910 */
void *CardThread_CmdQueue(void) {
    return &sCardCmdQueue;
}

/* 0x8131C91C */
void *CardThread_ReplyQueue(void) {
    return &sCardReplyQueue;
}

/* 0x8131C928 */
void *CardThread_GetIcons(void) {
    return &lbl_814C02A8;
}

/* 0x8131C934 */
s32 CardThread_GetBanner(u8 arg0, s16 arg1) {
    return *(&lbl_814C4620 + (arg0 * 0x1FC) + (arg1 * 4));
}

/* 0x8131C95C */
void CardFile_Clear(u8 arg0, s16 arg1) {
    s32 temp_r8;
    void *temp_r4;
    void *temp_r5;

    OSDisableInterrupts(&gCardSlots);
    temp_r5 = &gCardSlots + (arg0 * 0x5F4) + (arg1 * 0xC);
    temp_r5->unk28 = 0;
    temp_r8 = arg0 * 0x1FC0;
    temp_r5->unk2A = 0;
    temp_r5->unk2C = 1;
    temp_r5->unk2D = 1;
    temp_r4 = &gCardSlots + 0x10000 + temp_r8 + (arg1 << 6);
    temp_r5->unk2E = 0;
    temp_r4->unk-6FD7 = 0;
    temp_r4->unk-6FD8 = 0;
    OSRestoreInterrupts(temp_r4, temp_r5, 0, arg1, temp_r8);
}

/*
Decompilation failure in function CardThread_Main:

Unable to determine jump table for bctr instruction at _sel.s line 826.

Found multiple candidates: jumptable_81381370, sCardCmdQueue
You can help m2c's analysis by putting a copy of the jump table
setup instructions within the same block as the jump instruction.
*/

/* 0x8131CB64 */
void fn_8131CB64(void) {
    void *var_r27;
    void *var_r29;
    s16 var_r24;
    s32 *var_r26;
    u8 var_r25;

    var_r27 = &lbl_814B72A8;
    var_r25 = 0;
    var_r26 = &lbl_814AD4B8;
    do {
        var_r29 = var_r27;
        var_r24 = 0;
loop_2:
        var_r29->unk6 = 0;
        if (((s32) *var_r26 != 0) && ((s32) (&lbl_814AD4B8)[var_r25 ^ 1] != 0) && (fn_8131E044(var_r25, var_r24) < 0)) {
            var_r29->unk6 = 1;
        }
        var_r24 += 1;
        var_r29 += 0xC;
        if (var_r24 < 0x7F) {
            goto loop_2;
        }
        var_r25 += 1;
        var_r27 += 0x5F4;
        var_r26 += 4;
    } while ((s32) var_r25 < 2);
}

/* 0x8131CC10 */
void CardThread_SetEnabled(u8 arg0, s32 arg1) {
    OSSendMessage((s32) &sCardReplyQueue, (arg0 & ~0xFF00) | ((arg1 << 8) & 0xFF00), 1);
}

/* 0x8131CC48 */
s32 fn_8131CC48(u8 arg0, s32 arg1, s32 arg2) {
    void *var_r22;
    void *var_r24;
    s16 var_r26;
    s32 *var_r21;
    u8 var_r25;

    if ((arg2 != -7) && (arg2 != -0x1B) && (arg2 != -8) && (arg2 != -9)) {
        fn_8131CD74(arg0);
        if ((arg2 != -6) && (arg2 != -0xD)) {
            fn_81371784(arg0);
            *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
        }
    }
    var_r22 = &lbl_814B72A8;
    var_r25 = 0;
    var_r21 = &lbl_814AD4B8;
    do {
        var_r26 = 0;
        var_r24 = var_r22;
loop_9:
        var_r24->unk6 = 0;
        if (((s32) *var_r21 != 0) && ((s32) (&lbl_814AD4B8)[var_r25 ^ 1] != 0) && (fn_8131E044(var_r25, var_r26) < 0)) {
            var_r24->unk6 = 1;
        }
        var_r26 += 1;
        var_r24 += 0xC;
        if (var_r26 < 0x7F) {
            goto loop_9;
        }
        var_r25 += 1;
        var_r22 += 0x5F4;
        var_r21 += 4;
    } while ((s32) var_r25 < 2);
    OSSendMessage((s32) &sCardReplyQueue, ((((arg2 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000)) & ~0xFF) | (arg1 & 0xFF), 1);
    return 0;
}

/* 0x8131CD74 */
void fn_8131CD74(u8 arg0) {
    s16 var_r27;
    void *var_r28;
    void *var_r29;

    var_r29 = &lbl_814B72A8 + (arg0 * 0x5F4);
    var_r28 = &lbl_814C02A8 + (arg0 * 0x1FC0);
    var_r27 = 0;
loop_2:
    if (var_r27 < 0x7F) {
        OSDisableInterrupts();
        var_r29->unk0 = 0;
        var_r29->unk2 = 0;
        var_r29->unk4 = 1;
        var_r29->unk5 = 1;
        var_r29->unk6 = 0;
        var_r28->unk1 = 0;
        var_r28->unk0 = 0;
        OSRestoreInterrupts();
        var_r29 += 0xC;
        var_r28 += 0x40;
        var_r27 += 1;
        goto loop_2;
    }
}

/* 0x8131CE08 */
s32 fn_8131CE08(u8 arg0, s16 arg1, void *arg2) {
    s32 temp_r4_2;
    s32 temp_r5;
    s32 temp_r5_2;
    s32 temp_r5_3;
    s32 var_r0;
    void *temp_r3;
    void *temp_r3_2;
    void *temp_r4;
    void *temp_r4_3;
    void *temp_r4_4;
    void *temp_r5_4;
    void *temp_r6;
    void *temp_r6_2;
    void *temp_r6_3;
    void *temp_r7;
    void *temp_r7_2;
    void *temp_r8;

    temp_r5 = arg2->unk7 & 3;
    var_r0 = 0;
    switch (temp_r5) {                              /* irregular */
    case 1:
        temp_r6 = &gCardSlots + 0x10000;
        temp_r3 = temp_r6 + (arg0 * 0x1FC0);
        temp_r5_2 = arg1 << 6;
        temp_r4 = temp_r3 + temp_r5_2;
        temp_r7 = temp_r3 + temp_r5_2;
        temp_r7->unk-6FD8 = 1;
        temp_r4->unk-6FD5 = 9;
        temp_r4->unk-6FC4 = (s32) ((temp_r6 + (arg0 * 0x1FC) + (arg1 * 4))->unk-3058 - (temp_r7 - 0x6FD8));
        var_r0 = 0xE00;
        temp_r4->unk-6FC0 = (s32) (*(temp_r4 - 0x6FC4) + 0xC00);
        temp_r4->unk-6FBC = (s32) (temp_r4->unk-6FC0 + 0x200);
        break;
    case 2:
        temp_r6_2 = &gCardSlots + 0x10000;
        temp_r3_2 = temp_r6_2 + (arg0 * 0x1FC0);
        temp_r4_2 = arg1 << 6;
        temp_r7_2 = temp_r3_2 + temp_r4_2;
        temp_r4_3 = temp_r3_2 + temp_r4_2;
        temp_r7_2->unk-6FD8 = 1;
        temp_r4_3->unk-6FD5 = 5;
        var_r0 = 0x1800;
        temp_r4_3->unk-6FC4 = (s32) ((temp_r6_2 + (arg0 * 0x1FC) + (arg1 * 4))->unk-3058 - (temp_r7_2 - 0x6FD8));
        temp_r4_3->unk-6FBC = (s32) (temp_r4_3->unk-6FC4 + 0x1800);
        break;
    default:
        temp_r6_3 = &gCardSlots + 0x10000;
        temp_r4_4 = temp_r6_3 + (arg0 * 0x1FC0);
        temp_r5_3 = arg1 << 6;
        temp_r8 = temp_r4_4 + temp_r5_3;
        temp_r8->unk-6FD8 = 0;
        temp_r5_4 = temp_r4_4 + temp_r5_3;
        temp_r5_4->unk-6FD5 = 0;
        temp_r5_4->unk-6FBC = (s32) ((temp_r6_3 + (arg0 * 0x1FC) + (arg1 * 4))->unk-3058 - (temp_r8 - 0x6FD8));
        break;
    }
    return var_r0;
}

/* 0x8131CF54 */
s32 fn_8131CF54(u8 arg0, s16 arg1, s32 *arg2) {
    s32 temp_r0;
    s32 temp_r11;
    s32 temp_r30;
    s32 temp_r30_2;
    s32 temp_r4;
    s32 temp_r4_2;
    s32 temp_r9;
    s32 var_ctr;
    s32 var_ctr_2;
    s32 var_r11;
    s32 var_r11_2;
    s32 var_r12;
    s32 var_r31;
    s32 var_r3;
    s32 var_r4;
    s32 var_r8;
    u16 temp_r7_2;
    void *temp_r10;
    void *temp_r4_3;
    void *temp_r7;
    void *temp_r7_3;
    void *temp_r7_4;
    void *temp_r7_5;
    void *temp_r8;
    void *var_r7;
    void *var_r8_2;

    var_r31 = saved_reg_r31;
    temp_r9 = arg0 * 0x1FC0;
    temp_r0 = arg1 << 6;
    temp_r7 = &gCardSlots + 0x10000 + temp_r9 + temp_r0;
    var_r8 = 0;
    temp_r7->unk-6FD6 = 0;
    var_ctr = 8;
    temp_r10 = temp_r7 - 0x6FD8;
    temp_r7->unk-6FC6 = 0;
    var_r12 = 0;
    var_r3 = 0;
    var_r11 = 0;
    temp_r7->unk-6FC8 = (u16) arg2->unk32;
    temp_r7->unk-6FD2 = (s8) ((arg2->unk32 * 4) & 0xC);
loop_1:
    temp_r7_2 = arg2->unk32;
    temp_r4 = ((s32) temp_r7_2 >> var_r8) & 3;
    if (temp_r4 != 0) {
        temp_r10->unk12 = (s16) (temp_r10->unk12 + (temp_r4 * 4));
        var_r8 += 2;
        var_r11 += 1;
        var_ctr -= 1;
        if (var_ctr == 0) {
            (&gCardSlots + 0x10000 + temp_r9 + temp_r0)->unk-6FD1 = (s8) (((u16) arg2->unk32 >> 0xCU) & 0xC);
        } else {
            goto loop_1;
        }
    } else {
        (&gCardSlots + 0x10000 + temp_r9 + temp_r0)->unk-6FD1 = (s8) ((((s32) temp_r7_2 >> ((var_r11 - 1) * 2)) * 4) & 0xC);
    }
    if (!(arg2->unk32 & 3) || !(arg2->unk30 & 3)) {
        (&gCardSlots + 0x10000 + temp_r9 + temp_r0)->unk-6FD7 = 0;
        return 0;
    }
    var_ctr_2 = 4;
    var_r11_2 = 0;
    var_r4 = 0;
    var_r7 = temp_r10;
    var_r8_2 = temp_r10;
loop_10:
    if (((s32) arg2->unk32 >> var_r4) & 3) {
        temp_r30 = ((s32) arg2->unk30 >> var_r4) & 3;
        switch (temp_r30) {                         /* switch 1; irregular */
        case 1:                                     /* switch 1 */
            var_r7->unk8 = 9U;
            var_r31 = 0x400;
            var_r12 = 1;
            break;
        case 2:                                     /* switch 1 */
            var_r7->unk8 = 5U;
            var_r31 = 0x800;
            break;
        case 0:                                     /* switch 1 */
            var_r31 = 0;
            var_r7->unk8 = (u8) var_r7->unk7;
            break;
        }
        if (var_r11_2 < 7) {
            var_r8_2->unk20 = (s32) (var_r31 + var_r8_2->unk1C);
        } else {
            temp_r10->unk3C = (s32) (var_r31 + var_r8_2->unk1C);
        }
        var_r3 += var_r31;
        temp_r10->unk2 = (u8) (temp_r10->unk2 + 1);
        temp_r4_2 = var_r4 + 2;
        temp_r8 = var_r8_2 + 4;
        temp_r11 = var_r11_2 + 1;
        temp_r7_3 = var_r7 + 1;
        if (((s32) arg2->unk32 >> temp_r4_2) & 3) {
            temp_r30_2 = ((s32) arg2->unk30 >> temp_r4_2) & 3;
            switch (temp_r30_2) {                   /* switch 2; irregular */
            case 1:                                 /* switch 2 */
                temp_r7_3->unk8 = 9U;
                var_r31 = 0x400;
                var_r12 = 1;
                break;
            case 2:                                 /* switch 2 */
                temp_r7_3->unk8 = 5U;
                var_r31 = 0x800;
                break;
            case 0:                                 /* switch 2 */
                var_r31 = 0;
                temp_r7_3->unk8 = (u8) temp_r7_3->unk7;
                break;
            }
            if (temp_r11 < 7) {
                temp_r8->unk20 = (s32) (var_r31 + temp_r8->unk1C);
            } else {
                temp_r10->unk3C = (s32) (var_r31 + temp_r8->unk1C);
            }
            var_r3 += var_r31;
            temp_r10->unk2 = (u8) (temp_r10->unk2 + 1);
            var_r4 = temp_r4_2 + 2;
            var_r7 = temp_r7_3 + 1;
            var_r8_2 = temp_r8 + 4;
            var_r11_2 = temp_r11 + 1;
            var_ctr_2 -= 1;
            if (var_ctr_2 != 0) {
                goto loop_10;
            }
        } else {
            temp_r7_4 = &gCardSlots + 0x10000 + temp_r9 + temp_r0;
            temp_r7_4->unk-6F9C = (s32) (temp_r7_4 + (temp_r11 * 4))->unk-6FBC;
        }
    } else {
        temp_r7_5 = &gCardSlots + 0x10000 + temp_r9 + temp_r0;
        temp_r7_5->unk-6F9C = (s32) (temp_r7_5 + (var_r11_2 * 4))->unk-6FBC;
    }
    if (var_r12 != 0) {
        var_r3 += 0x200;
    }
    temp_r4_3 = &gCardSlots + 0x10000 + temp_r9 + temp_r0;
    temp_r4_3->unk-6FD7 = 1;
    temp_r4_3->unk-6FD3 = (s8) (arg2->unk7 & 4);
    temp_r4_3->unk-6FD4 = 1;
    return var_r3;
}

/* 0x8131D268 */
s32 fn_8131D268(u8 arg0, s16 arg1, s32 *arg2, void *arg3) {
    u32 sp14;
    s32 temp_r28;
    s32 temp_r28_2;
    s32 temp_r29;
    s32 temp_r30;
    s32 temp_r3;
    s32 temp_r8;
    u32 temp_r0;
    u32 temp_r3_2;
    void *temp_r25;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r4_3;

    temp_r8 = arg2->unk2C;
    temp_r0 = temp_r8 & 0xFFFFFE00;
    temp_r30 = temp_r8 - temp_r0;
    temp_r28 = fn_8131CE08((u8) &gCardSlots, (s16) temp_r8);
    temp_r29 = temp_r28 + fn_8131CF54(arg0, arg1, arg2);
    temp_r28_2 = (temp_r29 + (temp_r30 + 0x1FF)) & 0xFFFFFE00;
    if (fn_8136F64C(arg0, &sp14) < 0) {
        temp_r4 = &gCardSlots + 0x10000 + (arg0 * 0x1FC0) + (arg1 << 6);
        temp_r4->unk-6FD8 = 0;
        temp_r4->unk-6FD7 = 0;
        return 0;
    }
    temp_r3_2 = arg2->unk38 * sp14;
    if (temp_r0 > temp_r3_2) {
        temp_r4_2 = &gCardSlots + 0x10000 + (arg0 * 0x1FC0) + (arg1 << 6);
        temp_r4_2->unk-6FD8 = 0;
        temp_r4_2->unk-6FD7 = 0;
        return 0;
    }
    if ((u32) (temp_r0 + temp_r28_2) > temp_r3_2) {
        temp_r4_3 = &gCardSlots + 0x10000 + (arg0 * 0x1FC0) + (arg1 << 6);
        temp_r4_3->unk-6FD8 = 0;
        temp_r4_3->unk-6FD7 = 0;
        return 0;
    }
    if (temp_r29 > 0) {
        temp_r3 = fn_81372AC4(arg3, lbl_814AD4C4, temp_r28_2, temp_r0);
        if (temp_r3 < 0) {
            return temp_r3;
        }
        temp_r25 = &gCardSlots + 0x10000 + (arg0 * 0x1FC) + (arg1 * 4);
        memcpy(temp_r25->unk-3058, lbl_814AD4C4 + temp_r30, temp_r29);
        DCStoreRange(*(temp_r25 - 0x3058), temp_r28_2);
        goto block_10;
    }
block_10:
    return 0;
}

/* 0x8131D408 */
s32 fn_8131D408(u8 arg0, s16 arg1, s32 *arg2, void *arg3) {
    u32 sp14;
    s32 temp_r25;
    s32 temp_r26;
    s32 temp_r5;
    s32 var_r27;
    u32 temp_r3;
    void *temp_r28;
    void *temp_r29;

    temp_r5 = arg2->unk3C;
    temp_r26 = temp_r5 & 0xFFFFFE00;
    temp_r25 = ((temp_r5 + 0x23F) & 0xFFFFFE00) - temp_r26;
    var_r27 = fn_8136F64C(arg0, &sp14, temp_r5);
    if (var_r27 >= 0) {
        if ((temp_r26 < 0) || (temp_r3 = arg2->unk38 * sp14, (((u32) temp_r26 > temp_r3) != 0))) {
            var_r27 = 0;
            goto block_9;
        }
        if (((temp_r26 + temp_r25) < 0) || ((u32) (temp_r26 + temp_r25) > temp_r3)) {
            var_r27 = 0;
            goto block_9;
        }
        var_r27 = fn_81372AC4(arg3, &gCardSlots + 0xD7A0, temp_r25, (u32) temp_r26);
        if (var_r27 >= 0) {
            temp_r29 = &gCardSlots + 0x10000;
            temp_r28 = temp_r29 + (arg0 * 0x1FC) + (arg1 * 4);
            memset(temp_r28->unk-2C60, 0, 0x40);
            memcpy(*(temp_r28 - 0x2C60), (temp_r29 + (temp_r5 - temp_r26)) - 0x2860, 0x40);
            return 0;
        }
        goto block_9;
    }
block_9:
    memset((&gCardSlots + 0x10000 + (arg0 * 0x1FC) + (arg1 * 4))->unk-2C60, 0, 0x40);
    return var_r27;
}

/* 0x8131D548 */
s32 CardThread_ReadEntry(u8 arg0, s16 arg1, s32 *arg2) {
    u8 sp10[0x4];
    s32 temp_r3;
    s32 temp_r3_2;
    s32 temp_r3_3;
    s32 temp_r3_4;
    s32 temp_r5;
    s32 temp_r6;
    s8 var_r4;
    s8 var_r4_2;
    void *temp_r3_5;

    temp_r3_4 = fn_81372138(arg0, arg1, &sp10);
    if (temp_r3_4 < 0) {
        return temp_r3_4;
    }
    temp_r3_3 = fn_8131D268(arg0, arg1, arg2, &sp10);
    if (temp_r3_3 < 0) {
        return temp_r3_3;
    }
    temp_r3_2 = fn_8131D408(arg0, arg1, arg2, &sp10);
    if (temp_r3_2 < 0) {
        return temp_r3_2;
    }
    temp_r3 = fn_813722AC(&sp10);
    if (temp_r3 < 0) {
        return temp_r3;
    }
    temp_r6 = arg0 * 0x5F4;
    temp_r5 = arg1 * 0xC;
    temp_r3_5 = &gCardSlots + temp_r6 + temp_r5;
    var_r4 = 1;
    temp_r3_5->unk28 = 1;
    temp_r3_5->unk2A = (u16) arg2->unk38;
    temp_r3_5->unk30 = (s32) arg2->unk28;
    if ((u32) (arg2->unk34 & 8) != 8) {

    } else {
        var_r4 = 0;
    }
    (&gCardSlots + temp_r6 + temp_r5)->unk2C = var_r4;
    if ((u32) (arg2->unk34 & 0x10) != 0x10) {
        var_r4_2 = 1;
    } else {
        var_r4_2 = 0;
    }
    (&gCardSlots + temp_r6 + temp_r5)->unk2D = var_r4_2;
    return 0;
}

/* 0x8131D670 */
void CardThread_DoUnmount(u8 arg0) {
    s16 var_r31;

    var_r31 = 0;
loop_2:
    if (var_r31 < 0x7F) {
        CardFile_Clear(arg0, var_r31);
        var_r31 += 1;
        goto loop_2;
    }
    fn_81371784(arg0);
    *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
    fn_8131CB64(&lbl_814AD4B8, 0);
    OSSendMessage((s32) &sCardReplyQueue, ((arg0 << 0x10) & 0x10000) | 0xFD09, 1);
}

/* 0x8131D6FC */
void CardThread_DoMount(u8 arg0) {
    u8 sp94[0x4];
    s32 sp90;
    s32 sp8C;
    s32 sp88;
    u16 sp84;
    u16 sp80;
    s32 sp78;
    u32 sp74;
    u32 sp70;
    u8 sp38[0x38];
    s32 sp30;
    s8 sp2C;
    u8 spC[0x20];
    s16 var_r26;
    s16 var_r26_2;
    s16 var_r28;
    s32 temp_r24;
    s32 temp_r3;
    s32 temp_r3_2;
    s32 temp_r3_3;
    s32 temp_r3_4;
    s32 temp_r6;
    s32 var_r0;
    s32 var_r23;
    s32 var_r3;
    s32 var_r3_2;
    u16 var_r25;
    void *temp_r4;
    void *var_r23_2;
    void *var_r29;

    temp_r24 = (arg0 * 4) & 0x3FC;
    temp_r3 = fn_813716A0(arg0, *(&lbl_814AD4B0 + temp_r24), 0);
    if (temp_r3 < 0) {
        switch (temp_r3) {                          /* switch 1; irregular */
        case -4:                                    /* switch 1 */
            var_r3 = 1;
            break;
        case -6:                                    /* switch 1 */
            temp_r3_2 = fn_81370FB4(arg0);
            if (temp_r3_2 < 0) {
                var_r3 = fn_8131CC48(arg0, 0, temp_r3_2);
            } else {
                var_r3 = 1;
            }
            break;
        default:                                    /* switch 1 */
        case -5:                                    /* switch 1 */
            var_r3 = fn_8131CC48(arg0, 0);
            break;
        }
        if (var_r3 == 0) {
            return;
        }
        goto block_14;
    }
    var_r23 = fn_81370FB4(arg0);
    if (var_r23 >= 0) {
block_14:
        var_r28 = 0;
        var_r29 = &gCardSlots + (arg0 * 0x5F4) + 0x28;
        var_r23_2 = (&gCardSlots + 0x10000 + (arg0 * 0x1FC0)) - 0x6FD8;
loop_16:
        if (var_r28 < 0x7F) {
            OSDisableInterrupts();
            var_r29->unk0 = 0;
            var_r29->unk2 = 0;
            var_r29->unk4 = 1;
            var_r29->unk5 = 1;
            var_r29->unk6 = 0;
            var_r23_2->unk1 = 0;
            var_r23_2->unk0 = 0;
            OSRestoreInterrupts();
            var_r29 += 0xC;
            var_r23_2 += 0x40;
            var_r28 += 1;
            goto loop_16;
        }
        var_r26 = 0;
loop_41:
        if (var_r26 >= 0x7F) {
            fn_8136F5C8(arg0, &sp80);
            fn_8136F64C(arg0, &sp70);
            fn_8136F478(arg0, &sp74, &sp78);
            if (fn_8136F448(arg0) >= 0) {
                OSDisableInterrupts();
                temp_r6 = arg0 * 0x14;
                temp_r4 = &gCardSlots + temp_r6;
                temp_r4->unkC = (s16) (((u32) (sp80 << 0x11) / sp70) - 5);
                temp_r4->unk8 = sp70;
                temp_r4->unkE = (s16) sp78;
                temp_r4->unk10 = (s16) (sp74 / sp70);
                OSRestoreInterrupts(temp_r4, (void *) sp70, temp_r6);
            }
            *(&lbl_814AD4B8 + temp_r24) = 1;
            fn_8131CB64(&lbl_814AD4B8);
            OSSendMessage((s32) (&gCardSlots + 0xC10), (arg0 << 0x10) & 0x10000, 1);
            var_r25 = 0;
loop_48:
            if (var_r25 < 0x7FU) {
                if (fn_813730DC(arg0, (s16) var_r25, &sp30) == 0) {
                    memcpy(&spC, &sp38, 0x20);
                    sp2C = 0;
                }
                var_r25 += 1;
                goto loop_48;
            }
            return;
        }
        temp_r3_3 = fn_813730DC(arg0, var_r26, &sp8C);
        if (temp_r3_3 < 0) {
            switch (temp_r3_3) {                    /* switch 2; irregular */
            case -4:                                /* switch 2 */
                var_r3_2 = 1;
                break;
            case -6:                                /* switch 2 */
                temp_r3_4 = fn_81370FB4(arg0);
                if (temp_r3_4 < 0) {
                    var_r3_2 = fn_8131CC48(arg0, 0, temp_r3_4);
                } else {
                    var_r3_2 = 1;
                }
                break;
            default:                                /* switch 2 */
            case -5:                                /* switch 2 */
                var_r3_2 = fn_8131CC48(arg0, 0);
                break;
            }
            if (var_r3_2 == 0) {
                return;
            }
            goto block_40;
        }
        sp84 = lbl_814ADF40;
        sp88 = lbl_814ADF44;
        if ((fn_8137DC14(&sp94, &lbl_813813A4, 0xB) == 0) && (fn_8137BC98(&sp8C, &sp88, 4) == 0) && (fn_8137BC98(&sp90, (s32 *) &sp84, 2) == 0)) {
            var_r0 = 1;
        } else {
            var_r0 = 0;
        }
        if (var_r0 != 0) {
            var_r23 = fn_81373094(arg0, var_r26);
            if (var_r23 >= 0) {
                goto block_40;
            }
            goto block_50;
        }
        var_r23 = CardThread_ReadEntry(arg0, var_r26, &sp8C);
        if (var_r23 >= 0) {
block_40:
            var_r26 += 1;
            goto loop_41;
        }
        goto block_50;
    }
block_50:
    if ((var_r23 != -7) && (var_r23 != -0x1B) && (var_r23 != -8) && (var_r23 != -9)) {
        var_r26_2 = 0;
loop_56:
        if (var_r26_2 < 0x7F) {
            CardFile_Clear(arg0, var_r26_2);
            var_r26_2 += 1;
            goto loop_56;
        }
        if ((var_r23 != -6) && (var_r23 != -0xD)) {
            fn_81371784(arg0);
            *(&lbl_814AD4B8 + temp_r24) = 0;
        }
    }
    fn_8131CB64();
    OSSendMessage((s32) (&gCardSlots + 0xC10), ((var_r23 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000), 1);
}

/* 0x8131DAE8 */
void fn_8131DAE8(void) {
    lbl_814AD4E0 = 0U;
}

/* 0x8131DAF4 */
s32 CardThread_CheckDest(u8 arg0, s16 arg1) {
    u32 sp50;
    s32 sp10;
    s32 temp_r23;
    s32 var_r24;
    s32 var_r29;
    u16 var_r27;
    u32 temp_r22;
    u32 temp_r28;
    u32 var_r0;

    var_r27 = 0;
    var_r24 = fn_813730DC(arg0, arg1, &sp10);
    if (var_r24 >= 0) {
        var_r24 = fn_8136F64C(arg0, &sp50);
        if (var_r24 >= 0) {
            temp_r28 = 0x40000U / sp50;
            var_r24 = fn_81372138(arg0, arg1, &gCardSlots + 0x9000);
            if (var_r24 >= 0) {
                var_r29 = 0;
loop_14:
                if (var_r29 >= (s32) sp48) {
                    var_r24 = fn_813722AC(&gCardSlots + 0x9000);
                    if (var_r24 >= 0) {
                        var_r24 = fn_813722AC(&gCardSlots + 0x9014);
                        if (var_r24 >= 0) {
                            return 0;
                        }
                    }
                    goto block_18;
                }
                var_r0 = sp48 - var_r29;
                if (var_r0 > temp_r28) {
                    var_r0 = temp_r28;
                }
                temp_r22 = var_r29 * sp50;
                temp_r23 = var_r0 * sp50;
                var_r24 = fn_81372AC4(&gCardSlots + 0x9000, lbl_814AD4C0, temp_r23, temp_r22);
                if (var_r24 >= 0) {
                    lbl_814AD4E0 = 1U;
                    var_r24 = fn_81372DB8(&gCardSlots + 0x9014, lbl_814AD4C0, temp_r23, temp_r22, fn_8131DAE8);
                    if (var_r24 >= 0) {
loop_12:
                        if ((u32) lbl_814AD4E0 != 0U) {
                            if ((CARDProbe(arg0) == 0) && (var_r27 == 0)) {
                                fn_81372B0C(&gCardSlots + 0x9014);
                                var_r27 = 1;
                            }
                            goto loop_12;
                        }
                        var_r29 += temp_r28;
                        goto loop_14;
                    }
                }
                goto block_18;
            }
        }
    }
block_18:
    fn_81373094(arg0 ^ 1, (s16) gCardSlots.unk9018);
    return var_r24;
}

/* 0x8131DC8C */
s32 CardThread_CopyFile(u8 arg0, s16 arg1, s16 arg2, s32 *arg3) {
    u8 sp49;
    u32 sp3C;
    s32 sp14;
    s32 temp_r3;
    s32 temp_r3_2;
    s32 temp_r3_3;
    s32 temp_r3_4;
    u32 temp_r25;
    u32 temp_r27;
    u8 temp_r27_2;

    temp_r3_4 = fn_813730DC(arg0, arg1, &sp14);
    if (temp_r3_4 < 0) {
        return temp_r3_4;
    }
    sp49 += 1;
    temp_r3_3 = fn_813733E0(arg0, arg1, &sp14);
    if (temp_r3_3 < 0) {
        return temp_r3_3;
    }
    temp_r25 = sp3C;
    temp_r27 = (u32) *(u32 *)0x800000F8 >> 2U;
    OSGetTime(0x80000000);
    sp3C = (u32) __div2i(0, temp_r27);
    temp_r27_2 = arg0 ^ 1;
    temp_r3_2 = fn_813733E0(temp_r27_2, arg2, &sp14);
    if (temp_r3_2 < 0) {
        if (temp_r3_2 != -3) {
            sp49 -= 1;
        }
        sp3C = temp_r25;
        fn_813733E0(arg0, arg1, &sp14);
        return temp_r3_2;
    }
    *arg3 = 1;
    temp_r3 = CardThread_ReadEntry(temp_r27_2, arg2, &sp14);
    if (temp_r3 < 0) {
        return temp_r3;
    }
    return 0;
}

/* 0x8131DD9C */
s32 CardThread_MoveFile(u8 arg0, s16 arg1, s16 arg2, s32 *arg3) {
    s32 sp58;
    s32 sp54;
    s32 sp50;
    s32 sp4C;
    s32 sp48;
    s32 sp44;
    s32 sp40;
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
    s16 temp_r7;
    s16 var_r26;
    s32 var_r25;
    u8 temp_r26;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r5;
    void *temp_r5_2;

    var_r26 = 0;
    var_r25 = fn_813730DC(arg0, arg1, &sp58, &gCardSlots);
    if (var_r25 >= 0) {
        sp18 = sp58;
        sp1C = sp5C;
        sp20 = sp60;
        sp24 = sp64;
        sp28 = sp68;
        sp2C = sp6C;
        sp30 = sp70;
        sp34 = sp74;
        sp38 = sp78;
        sp3C = sp7C;
        sp40 = sp80;
        sp44 = sp84;
        sp48 = sp88;
        sp4C = sp8C;
        sp50 = sp90;
        sp54 = sp94;
        memset(&sp1C, 0, 2, sp90);
        memset(&sp18, 0, 4);
loop_2:
        memset(&sp20, 0, 0x20);
        strcpy_(&sp20, &lbl_813813B0, var_r26);
        var_r25 = fn_813733E0(arg0, arg1, &sp18);
        if ((var_r25 >= 0) || (var_r25 == -7)) {
            var_r26 += 1;
            if ((var_r25 == 0) || (var_r26 >= 0x80)) {
                temp_r26 = arg0 ^ 1;
                var_r25 = fn_813733E0(temp_r26, arg2, &sp58);
                if (var_r25 < 0) {
                    if (var_r25 == -3) {
                        fn_81373094(arg0, arg1);
                        OSDisableInterrupts();
                        temp_r5 = &gCardSlots + (arg0 * 0x5F4) + (arg1 * 0xC);
                        temp_r5->unk28 = 0;
                        temp_r7 = arg0 * 0x1FC0;
                        temp_r5->unk2A = 0;
                        temp_r5->unk2C = 1;
                        temp_r5->unk2D = 1;
                        temp_r4 = &gCardSlots + 0x10000 + temp_r7 + (arg1 << 6);
                        temp_r5->unk2E = 0;
                        temp_r4->unk-6FD7 = 0;
                        temp_r4->unk-6FD8 = 0;
                        OSRestoreInterrupts(temp_r4, temp_r5, 0, temp_r7);
                    } else {
                        fn_813733E0(arg0, arg1, &sp58);
                    }
                    goto block_14;
                }
                *arg3 = 1;
                var_r25 = fn_81373094(arg0, arg1);
                if (var_r25 >= 0) {
                    OSDisableInterrupts();
                    temp_r5_2 = &gCardSlots + (arg0 * 0x5F4) + (arg1 * 0xC);
                    temp_r5_2->unk28 = 0;
                    temp_r5_2->unk2A = 0;
                    temp_r5_2->unk2C = 1;
                    temp_r5_2->unk2D = 1;
                    temp_r4_2 = &gCardSlots + 0x10000 + (arg0 * 0x1FC0) + (arg1 << 6);
                    temp_r5_2->unk2E = 0;
                    temp_r4_2->unk-6FD7 = 0;
                    temp_r4_2->unk-6FD8 = 0;
                    OSRestoreInterrupts(temp_r4_2, temp_r5_2, 0);
                    var_r25 = fn_813730DC(temp_r26, arg2, &sp58);
                    if (var_r25 >= 0) {
                        var_r25 = CardThread_ReadEntry(temp_r26, arg2, &sp58);
                        if (var_r25 >= 0) {
                            return 0;
                        }
                    }
                }
                goto block_14;
            }
            goto loop_2;
        }
        goto block_14;
    }
block_14:
    return var_r25;
}

/* 0x8131E044 */
s32 fn_8131E044(u8 arg0, s16 arg1) {
    s32 sp54;
    s32 sp50;
    s32 sp4C;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s16 var_r26;
    s32 temp_r3;
    s32 temp_r3_2;

    temp_r3_2 = fn_813730DC(arg0, arg1, &sp4C);
    if (temp_r3_2 < 0) {
        return temp_r3_2;
    }
    var_r26 = 0;
loop_3:
    temp_r3 = fn_813730DC(arg0 ^ 1, var_r26, &spC);
    if ((temp_r3 < 0) && (temp_r3 != -4)) {
        return temp_r3;
    }
    if ((temp_r3 == 0) && (fn_8137BC98(&sp4C, &spC, 4) == 0) && (fn_8137BC98(&sp50, &sp10, 2) == 0) && (fn_8137BC98(&sp54, &sp14, 0x20) == 0)) {
        return -7;
    }
    var_r26 += 1;
    if (var_r26 >= 0x7F) {
        return 0;
    }
    goto loop_3;
}

/* 0x8131E130 */
void CardThread_DoCopyMove(u8 arg0, s16 arg1, s32 arg2) {
    s32 spAC;
    s32 spA8;
    u32 spA4;
    u32 spA0;
    s32 sp60;
    u16 sp5C;
    s32 sp54;
    u32 sp50;
    u32 sp4C;
    u16 sp48;
    s32 sp40;
    u32 sp3C;
    u32 sp38;
    u16 sp34;
    s32 sp2C;
    u32 sp28;
    u32 sp24;
    u16 sp20;
    s32 sp18;
    u32 sp14;
    u32 sp10;
    s16 temp_r3_4;
    s16 var_r21;
    s16 var_r21_3;
    s16 var_r21_4;
    s16 var_r22;
    s16 var_r22_2;
    s16 var_r23_2;
    s16 var_r23_3;
    s16 var_r23_4;
    s16 var_r24;
    s32 temp_r3;
    s32 temp_r3_2;
    s32 temp_r3_3;
    s32 temp_r3_5;
    s32 temp_r3_6;
    s32 temp_r6;
    s32 temp_r6_2;
    s32 temp_r6_3;
    s32 temp_r6_4;
    s32 var_r21_2;
    s32 var_r25;
    u32 var_r23;
    u8 temp_r22;
    u8 temp_r28;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r4_3;
    void *temp_r4_4;

    var_r22 = 0;
    var_r24 = -1;
    var_r25 = 0;
    spA8 = 0;
    temp_r3 = fn_8131E044((u8) &gCardSlots);
    if (temp_r3 < 0) {
        if ((temp_r3 != -7) && (temp_r3 != -0x1B) && (temp_r3 != -8) && (temp_r3 != -9)) {
loop_7:
            if (var_r22 < 0x7F) {
                CardFile_Clear(arg0, var_r22);
                var_r22 += 1;
                goto loop_7;
            }
            if ((temp_r3 != -6) && (temp_r3 != -0xD)) {
                fn_81371784(arg0);
                *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
            }
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((((temp_r3 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000)) & ~0xFF) | (arg2 & 0xFF), 1);
        return;
    }
    var_r23 = 0;
    temp_r3_2 = fn_8136F64C(0U, &spA0);
    if (temp_r3_2 < 0) {
        var_r22 = (s16) temp_r3_2;
    } else {
        temp_r3_3 = fn_8136F64C(1U, &spA4);
        if (temp_r3_3 < 0) {
            var_r22 = (s16) temp_r3_3;
        } else if (spA0 != spA4) {
            var_r22 = -0x1B;
        } else {
            var_r23 = spA0;
        }
    }
    if (var_r22 < 0) {
        if ((var_r22 != -7) && (var_r22 != -0x1B) && (var_r22 != -8) && (var_r22 != -9)) {
            var_r21 = 0;
loop_26:
            if (var_r21 < 0x7F) {
                CardFile_Clear(arg0, var_r21);
                var_r21 += 1;
                goto loop_26;
            }
            if ((var_r22 != -6) && (var_r22 != -0xD)) {
                fn_81371784(arg0);
                *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
            }
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((((var_r22 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000)) & ~0xFF) | (arg2 & 0xFF), 1);
        return;
    }
    var_r21_2 = 0;
    if (fn_813730DC(arg0, arg1, &sp60) >= 0) {
        switch (arg2) {                             /* irregular */
        case 2:
            if (sp94 & 8) {
                var_r21_2 = -0xA;
            }
            break;
        case 3:
            if (sp94 & 0x10) {
                var_r21_2 = -0xA;
            }
            break;
        }
    } else {
        var_r21_2 = fn_8136F448(arg0);
    }
    if (var_r21_2 < 0) {
        if ((var_r21_2 != -7) && (var_r21_2 != -0x1B) && (var_r21_2 != -8) && (var_r21_2 != -9)) {
            var_r22_2 = 0;
loop_48:
            if (var_r22_2 < 0x7F) {
                CardFile_Clear(arg0, var_r22_2);
                var_r22_2 += 1;
                goto loop_48;
            }
            if ((var_r21_2 != -6) && (var_r21_2 != -0xD)) {
                fn_81371784(arg0);
                *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
            }
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((((var_r21_2 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000)) & ~0xFF) | (arg2 & 0xFF), 1);
        return;
    }
    if (fn_813730DC(arg0, arg1, &spAC) >= 0) {
        temp_r28 = arg0 ^ 1;
        var_r23_2 = 0;
loop_55:
        strcpy_(&gCardSlots + 0x8FE0, &lbl_813813B0, var_r23_2);
        temp_r3_4 = fn_81372650(temp_r28, &gCardSlots + 0x8FE0, spE4 * var_r23, &gCardSlots + 0x9014);
        if ((temp_r3_4 < 0) && (temp_r3_4 != -7)) {
            var_r23_3 = temp_r3_4;
        } else {
            var_r23_2 += 1;
            if ((temp_r3_4 == 0) || (var_r23_2 >= 0x80)) {
                if (var_r23_2 < 0x80) {
                    var_r23_3 = (s16) gCardSlots.unk9018;
                } else {
                    var_r23_3 = -0x80;
                }
            } else {
                goto loop_55;
            }
        }
        var_r24 = var_r23_3;
        if (var_r23_3 < 0) {
            if ((var_r23_3 != -7) && (var_r23_3 != -0x1B) && (var_r23_3 != -8) && (var_r23_3 != -9)) {
                var_r21_3 = 0;
loop_70:
                if (var_r21_3 < 0x7F) {
                    CardFile_Clear(temp_r28, var_r21_3);
                    var_r21_3 += 1;
                    goto loop_70;
                }
                if ((var_r23_3 != -6) && (var_r23_3 != -0xD)) {
                    fn_81371784(temp_r28);
                    *(&lbl_814AD4B8 + ((temp_r28 * 4) & 0x3FC)) = 0;
                }
            }
            fn_8131CB64();
            OSSendMessage((s32) (&gCardSlots + 0xC10), ((((var_r23_3 << 8) & 0xFF00 & ~0x10000) | ((temp_r28 << 0x10) & 0x10000)) & ~0xFF) | (arg2 & 0xFF), 1);
            return;
        }
        var_r25 = 1;
        if (CardThread_CheckDest(arg0, arg1) >= 0) {
            if (arg2 == 2) {
                if (CardThread_CopyFile(arg0, arg1, var_r24, &spA8) >= 0) {
                    goto block_81;
                }
                goto block_86;
            }
            if ((arg2 != 3) || (CardThread_MoveFile(arg0, arg1, var_r24, &spA8) >= 0)) {
block_81:
                fn_8136F5C8(arg0, &sp5C);
                fn_8136F64C(arg0, &sp4C);
                fn_8136F478(arg0, &sp50, &sp54);
                if (fn_8136F448(arg0) >= 0) {
                    OSDisableInterrupts();
                    temp_r6 = arg0 * 0x14;
                    temp_r4 = &gCardSlots + temp_r6;
                    temp_r4->unkC = (s16) (((u32) (sp5C << 0x11) / sp4C) - 5);
                    temp_r4->unk8 = sp4C;
                    temp_r4->unkE = (s16) sp54;
                    temp_r4->unk10 = (s16) (sp50 / sp4C);
                    OSRestoreInterrupts(temp_r4, (void *) sp4C, temp_r6);
                }
                fn_8136F5C8(temp_r28, &sp48);
                fn_8136F64C(temp_r28, &sp38);
                fn_8136F478(temp_r28, &sp3C, &sp40);
                if (fn_8136F448(temp_r28) >= 0) {
                    OSDisableInterrupts();
                    temp_r6_2 = temp_r28 * 0x14;
                    temp_r4_2 = &gCardSlots + temp_r6_2;
                    temp_r4_2->unkC = (s16) (((u32) (sp48 << 0x11) / sp38) - 5);
                    temp_r4_2->unk8 = sp38;
                    temp_r4_2->unkE = (s16) sp40;
                    temp_r4_2->unk10 = (s16) (sp3C / sp38);
                    OSRestoreInterrupts(temp_r4_2, (void *) sp38, temp_r6_2);
                }
                fn_8131CB64();
                OSSendMessage((s32) (&gCardSlots + 0xC10), ((arg0 << 0x10) & 0x10000 & ~0xFF000000) | (arg1 << 0x18), 1);
                fn_8131CB64();
                OSSendMessage((s32) (&gCardSlots + 0xC10), ((((temp_r28 << 0x10) & 0x10000 & ~0xFF000000) | (var_r23_3 << 0x18)) & ~0xFF) | (arg2 & 0xFF), 1);
                return;
            }
            goto block_86;
        }
        goto block_86;
    }
block_86:
    temp_r22 = arg0 ^ 1;
    temp_r3_5 = fn_8136F448(temp_r22);
    if (temp_r3_5 < 0) {
        if ((temp_r3_5 != -7) && (temp_r3_5 != -0x1B) && (temp_r3_5 != -8) && (temp_r3_5 != -9)) {
            var_r23_4 = 0;
loop_93:
            if (var_r23_4 < 0x7F) {
                CardFile_Clear(temp_r22, var_r23_4);
                var_r23_4 += 1;
                goto loop_93;
            }
            if ((temp_r3_5 != -6) && (temp_r3_5 != -0xD)) {
                fn_81371784(temp_r22);
                (&lbl_814AD4B8)[temp_r22] = 0;
            }
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((((temp_r3_5 << 8) & 0xFF00 & ~0x10000) | ((temp_r22 << 0x10) & 0x10000)) & ~0xFF) | (arg2 & 0xFF), 1);
    } else {
        if ((var_r25 != 0) && (spA8 == 0)) {
            fn_81373094(temp_r22, var_r24);
        }
        if ((spA8 != 0) && (fn_813730DC(temp_r22, var_r24, &spAC) == 0)) {
            CardThread_ReadEntry(temp_r22, var_r24, &spAC);
        }
        fn_8136F5C8(temp_r22, &sp34);
        fn_8136F64C(temp_r22, &sp24);
        fn_8136F478(temp_r22, &sp28, &sp2C);
        if (fn_8136F448(temp_r22) >= 0) {
            OSDisableInterrupts();
            temp_r6_3 = temp_r22 * 0x14;
            temp_r4_3 = &gCardSlots + temp_r6_3;
            temp_r4_3->unkC = (s16) (((u32) (sp34 << 0x11) / sp24) - 5);
            temp_r4_3->unk8 = sp24;
            temp_r4_3->unkE = (s16) sp2C;
            temp_r4_3->unk10 = (s16) (sp28 / sp24);
            OSRestoreInterrupts(temp_r4_3, (void *) sp24, temp_r6_3);
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((temp_r22 << 0x10) & 0x10000 & ~0xFF000000) | (arg1 << 0x18), 1);
    }
    temp_r3_6 = fn_8136F448(arg0);
    if (temp_r3_6 < 0) {
        if ((temp_r3_6 != -7) && (temp_r3_6 != -0x1B) && (temp_r3_6 != -8) && (temp_r3_6 != -9)) {
            var_r21_4 = 0;
loop_114:
            if (var_r21_4 < 0x7F) {
                CardFile_Clear(arg0, var_r21_4);
                var_r21_4 += 1;
                goto loop_114;
            }
            if ((temp_r3_6 != -6) && (temp_r3_6 != -0xD)) {
                fn_81371784(arg0);
                *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
            }
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((((temp_r3_6 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000)) & ~0xFF) | (arg2 & 0xFF), 1);
        return;
    }
    fn_8136F5C8(arg0, &sp20);
    fn_8136F64C(arg0, &sp10);
    fn_8136F478(arg0, &sp14, &sp18);
    if (fn_8136F448(arg0) >= 0) {
        OSDisableInterrupts();
        temp_r6_4 = arg0 * 0x14;
        temp_r4_4 = &gCardSlots + temp_r6_4;
        temp_r4_4->unkC = (s16) (((u32) (sp20 << 0x11) / sp10) - 5);
        temp_r4_4->unk8 = sp10;
        temp_r4_4->unkE = (s16) sp18;
        temp_r4_4->unk10 = (s16) (sp14 / sp10);
        OSRestoreInterrupts(temp_r4_4, (void *) sp10, temp_r6_4);
    }
    fn_8131CB64();
    OSSendMessage((s32) (&gCardSlots + 0xC10), ((arg0 << 0x10) & 0x10000 & ~0xFF000000) | (arg1 << 0x18), 1);
}

/* 0x8131E994 */
void CardThread_DoFormat(u8 arg0) {
    u16 sp1C;
    s32 sp14;
    u32 sp10;
    u32 spC;
    s16 var_r28;
    s32 temp_r3;
    s32 temp_r6;
    void *temp_r4;

    temp_r3 = fn_81371FCC(arg0, &gCardSlots);
    if (temp_r3 < 0) {
        if ((temp_r3 != -7) && (temp_r3 != -0x1B) && (temp_r3 != -8) && (temp_r3 != -9)) {
            var_r28 = 0;
loop_7:
            if (var_r28 < 0x7F) {
                CardFile_Clear(arg0, var_r28);
                var_r28 += 1;
                goto loop_7;
            }
            if ((temp_r3 != -6) && (temp_r3 != -0xD)) {
                fn_81371784(arg0);
                *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
            }
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((temp_r3 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000) | 1, 1);
        return;
    }
    (&lbl_814AD4B8)[arg0] = 1;
    fn_8136F5C8(arg0, &sp1C);
    fn_8136F64C(arg0, &spC);
    fn_8136F478(arg0, &sp10, &sp14);
    if (fn_8136F448(arg0) >= 0) {
        OSDisableInterrupts();
        temp_r6 = arg0 * 0x14;
        temp_r4 = &gCardSlots + temp_r6;
        temp_r4->unkC = (s16) (((u32) (sp1C << 0x11) / spC) - 5);
        temp_r4->unk8 = spC;
        temp_r4->unkE = (s16) sp14;
        temp_r4->unk10 = (s16) (sp10 / spC);
        OSRestoreInterrupts(temp_r4, (void *) spC, temp_r6);
    }
    fn_8131CB64();
    OSSendMessage((s32) (&gCardSlots + 0xC10), ((arg0 << 0x10) & 0x10000) | 1, 1);
}

/* 0x8131EB04 */
void CardThread_DoErase(u8 arg0, s16 arg1) {
    u16 sp1C;
    s32 sp14;
    u32 sp10;
    u32 spC;
    s16 temp_r7;
    s16 var_r29;
    s32 temp_r3;
    s32 temp_r6;
    void *temp_r4;
    void *temp_r4_2;
    void *temp_r5;

    temp_r3 = fn_81373094(arg0, arg1, &gCardSlots);
    if (temp_r3 < 0) {
        if ((temp_r3 != -7) && (temp_r3 != -0x1B) && (temp_r3 != -8) && (temp_r3 != -9)) {
            var_r29 = 0;
loop_7:
            if (var_r29 < 0x7F) {
                CardFile_Clear(arg0, var_r29);
                var_r29 += 1;
                goto loop_7;
            }
            if ((temp_r3 != -6) && (temp_r3 != -0xD)) {
                fn_81371784(arg0);
                *(&lbl_814AD4B8 + ((arg0 * 4) & 0x3FC)) = 0;
            }
        }
        fn_8131CB64();
        OSSendMessage((s32) (&gCardSlots + 0xC10), ((temp_r3 << 8) & 0xFF00 & ~0x10000) | ((arg0 << 0x10) & 0x10000) | 4, 1);
        return;
    }
    OSDisableInterrupts();
    temp_r5 = &gCardSlots + (arg0 * 0x5F4) + (arg1 * 0xC);
    temp_r5->unk28 = 0;
    temp_r7 = arg0 * 0x1FC0;
    temp_r5->unk2A = 0;
    temp_r5->unk2C = 1;
    temp_r5->unk2D = 1;
    temp_r4 = &gCardSlots + 0x10000 + temp_r7 + (arg1 << 6);
    temp_r5->unk2E = 0;
    temp_r4->unk-6FD7 = 0;
    temp_r4->unk-6FD8 = 0;
    OSRestoreInterrupts(temp_r4, temp_r5, 0, temp_r7);
    fn_8136F5C8(arg0, &sp1C);
    fn_8136F64C(arg0, &spC);
    fn_8136F478(arg0, &sp10, &sp14);
    if (fn_8136F448(arg0) >= 0) {
        OSDisableInterrupts();
        temp_r6 = arg0 * 0x14;
        temp_r4_2 = &gCardSlots + temp_r6;
        temp_r4_2->unkC = (s16) (((u32) (sp1C << 0x11) / spC) - 5);
        temp_r4_2->unk8 = spC;
        temp_r4_2->unkE = (s16) sp14;
        temp_r4_2->unk10 = (s16) (sp10 / spC);
        OSRestoreInterrupts(temp_r4_2, (void *) spC, temp_r6);
    }
    fn_8131CB64();
    OSSendMessage((s32) (&gCardSlots + 0xC10), ((arg0 << 0x10) & 0x10000 & ~0xFF000000) | (arg1 << 0x18) | 4, 1);
}
