// Memory Card worker thread (PAL 1.0 BS2, 0x8131BF6C..0x8131ECC4). The UI
// posts commands (cmd | slot << 16 | file << 24) to a message queue; the
// thread mounts the cards, reads the directory, icons and comments, and
// copies / moves / erases files and formats cards with the SDK CARD API,
// then replies with (cmd | result << 8 | slot << 16 | file << 24).
//
// Partial: the mount, copy / move, format and erase workers are not written
// yet.

#include <dolphin/card.h>
#include "bs2/menu.h"

#define CARD_MSG(cmd, res, slot, file)                                                     \
    (((cmd) & 0xFF) | (((res) << 8) & 0xFF00) | (((slot) << 16) & 0x10000) | ((file) << 24))
#define CARD_MSG_CMD(msg) ((s8)(u8)(msg))
#define CARD_MSG_RES(msg) ((s8)(u8)((msg) >> 8))
#define CARD_MSG_SLOT(msg) (((msg) >> 16) & 1)
#define CARD_MSG_FILE(msg) ((u8)((msg) >> 24))

/* BS2's own result: the destination file has a different size */
#define CARD_RESULT_SIZE -27

enum {
    CARD_CMD_MOUNT = 0,
    CARD_CMD_FORMAT = 1,
    CARD_CMD_COPY = 2,
    CARD_CMD_MOVE = 3,
    CARD_CMD_ERASE = 4,
    CARD_CMD_UNMOUNT = 9,
    CARD_CMD_ENABLE = 10,
    CARD_CMD_DISABLE = 11,
    CARD_CMD_REFRESH = 12
};

/* Icon and banner of one file, decoded from its directory entry (0x40 bytes) */
typedef struct CardIconInfo {
    u8 hasBanner;          /* 0x00 */
    u8 hasIcon;            /* 0x01 */
    u8 frames;             /* 0x02 */
    u8 bannerFormat;       /* 0x03 9: CI8, 5: RGB5A3 */
    u8 pad4;
    u8 pad5;
    u8 pingPong;           /* 0x06 */
    u8 pad7;
    u8 format[8];          /* 0x08 per frame, 9: CI8, 5: RGB5A3, else as the banner */
    u16 speeds;            /* 0x10 CARDDir.iconSpeed */
    s16 duration;          /* 0x12 */
    u8* banner;            /* 0x14 */
    u8* bannerTlut;        /* 0x18 */
    u8* data;              /* 0x1C start of the icon data in sCardIconBuf */
    u8* frame[7];          /* 0x20 */
    u8* frameTlut;         /* 0x3C shared palette of the CI8 frames */
} CardIconInfo;

CardSlot gCardSlots[2];
static CardFile sCardFiles[2][CARD_MAX_FILES];
static OSMessageQueue sCardReplyQueue;
static OSMessageQueue sCardCmdQueue;
static OSMessage sCardReplyMsgs[16];
static OSMessage sCardCmdMsgs[16];
static OSThread sCardThread;
static u8 sCardStack[0x8000];
static u8 sCardUnk8FE0[0x20];
static CARDFileInfo sCardSrcInfo;
static CARDFileInfo sCardDstInfo;
static CardIconInfo sCardIcons[2][CARD_MAX_FILES];
static u8* sCardIconBuf[2][CARD_MAX_FILES];   /* 0x5A00 each: banner and icon images */
static u8* sCardComment[2][CARD_MAX_FILES];   /* 0x40 each: the two comment lines */
static u8 sCardCommentRead[0x400] ATTRIBUTE_ALIGN(32);

static CARDMemoryCard* sCardWork[2];
static BOOL sCardMounted[2];
static u8* sCardCopyBuf;    /* 0x40000 */
static u8* sCardReadBuf;    /* 0x5A00 */
static u8 sCardCmdSlot;
static u8 sCardEnabled;
static u8 sCardDestFile;
static s32 sCardCmd;
static s32 sCardResult;
static s32 sCardDestResult;
static BOOL sCardRefreshAsked;
static BOOL sCardRefreshing;
static u32 sCardUnk4E0;

void* HeapAlloc(u32 size);
OSMessageQueue* CardThread_CmdQueue(void);
OSMessageQueue* CardThread_ReplyQueue(void);
void* CardThread_Main(void* arg);
void CardThread_MarkConflicts(void);
void CardThread_SetEnabled(u8 cmd, BOOL enabled);
s32 CardThread_Fail(u8 slot, s32 cmd, s32 result);
void CardThread_ClearFiles(u8 slot);
void CardThread_DoUnmount(u8 slot);
s32 CardThread_DoMount(u8 slot);
void CardThread_DoFormat(u8 slot);
void CardThread_DoCopyMove(u8 slot, u8 file);
void CardThread_DoErase(u8 slot, u8 file);
void fn_8131ECC4(void);
s32 fn_8131E044(u8 slot, s16 file);

static void CardThread_Send(u32 msg) {
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)msg, OS_MESSAGE_NOBLOCK);
}

/* 0x8131BF6C */
void* CardThread_GetFiles(void) {
    return sCardFiles;
}

/* 0x8131BF78 */
CardSlot* CardThread_GetSlots(void) {
    return gCardSlots;
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
void CardThread_Format(u8 slot) {
    u32 msg;

    sCardCmdSlot = slot;
    msg = (slot << 16) | CARD_CMD_FORMAT;
    sCardResult = CARD_UI_OK;
    sCardCmd = CARD_CMD_FORMAT;
    sCardDestResult = CARD_UI_OK;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)msg, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C000 */
void CardThread_Copy(u8 slot, s16 file) {
    sCardCmdSlot = slot;
    sCardResult = CARD_UI_OK;
    sCardDestResult = CARD_UI_OK;
    sCardCmd = CARD_CMD_COPY;
    CardThread_Send(CARD_MSG(CARD_CMD_COPY, 0, slot, file));
}

/* 0x8131C05C */
void CardThread_Move(u8 slot, s16 file) {
    sCardCmdSlot = slot;
    sCardResult = CARD_UI_OK;
    sCardDestResult = CARD_UI_OK;
    sCardCmd = CARD_CMD_MOVE;
    CardThread_Send(CARD_MSG(CARD_CMD_MOVE, 0, slot, file));
}

/* 0x8131C0B8 */
void CardThread_Erase(u8 slot, s16 file) {
    sCardCmdSlot = slot;
    sCardResult = CARD_UI_OK;
    sCardDestResult = CARD_UI_OK;
    sCardCmd = CARD_CMD_ERASE;
    CardThread_Send(CARD_MSG(CARD_CMD_ERASE, 0, slot, file));
}

/* 0x8131C114 */
void CardThread_Enable(void) {
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_CMD_ENABLE, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C140 */
void CardThread_UnmountAll(void) {
    sCardCmdSlot = 0;
    sCardCmd = CARD_CMD_UNMOUNT;
    sCardResult = CARD_UI_OK;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_MSG(CARD_CMD_UNMOUNT, 0, 0, 0), OS_MESSAGE_NOBLOCK);
    sCardCmd = CARD_CMD_UNMOUNT;
    sCardCmdSlot = 1;
    sCardResult = CARD_UI_OK;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_MSG(CARD_CMD_UNMOUNT, 0, 1, 0), OS_MESSAGE_NOBLOCK);
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_CMD_DISABLE, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C1CC: SDK result to the message shown by the UI. */
s32 Card_MapError(s8 result) {
    switch (result) {
    case CARD_RESULT_READY:
        return 0;
    case CARD_RESULT_WRONGDEVICE:
        return CARD_UI_BROKEN;
    case CARD_RESULT_NOCARD:
        return CARD_UI_BROKEN;
    case CARD_RESULT_NOFILE:
        return CARD_UI_BROKEN;
    case CARD_RESULT_IOERROR:
        return CARD_UI_BROKEN;
    case CARD_RESULT_BROKEN:
        return CARD_UI_BROKEN;
    case CARD_RESULT_EXIST:
        return CARD_UI_SAME_FILE;
    case CARD_RESULT_NOENT:
        return CARD_UI_NO_SPACE;
    case CARD_RESULT_INSSPACE:
        return CARD_UI_NO_SPACE;
    case CARD_RESULT_NOPERM:
        return CARD_UI_BROKEN;
    case CARD_RESULT_LIMIT:
        return CARD_UI_BROKEN;
    case CARD_RESULT_NAMETOOLONG:
        return CARD_UI_BROKEN;
    case CARD_RESULT_ENCODING:
        return CARD_UI_BROKEN;
    case CARD_RESULT_SIZE:
        return CARD_UI_DIFFERENT_SIZE;
    default:
        return CARD_UI_BROKEN;
    }
}

/* 0x8131C268: called every frame by the UI. Mounts and unmounts cards as they
   come and go, and takes the thread's replies. */
void CardThread_Poll(void) {
    OSMessage msg;
    s32 present;
    s16 slot;
    s8 res;

    if (sCardEnabled == TRUE) {
        present = CARDProbe(0);
        if (present && gCardSlots[0].state < 2) {
            sCardCmdSlot = 0;
            sCardCmd = CARD_CMD_MOUNT;
            sCardResult = CARD_UI_OK;
            if (OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_MSG(CARD_CMD_MOUNT, 0, 0, 0), OS_MESSAGE_NOBLOCK)) {
                gCardSlots[0].state = 3;
                gCardSlots[0].dirty = FALSE;
            }
        } else if (!present && gCardSlots[0].state > 2) {
            sCardCmdSlot = 0;
            sCardCmd = CARD_CMD_UNMOUNT;
            sCardResult = CARD_UI_OK;
            if (OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_MSG(CARD_CMD_UNMOUNT, 0, 0, 0), OS_MESSAGE_NOBLOCK)) {
                gCardSlots[0].dirty = FALSE;
            }
        }
    }
    if (sCardEnabled == TRUE) {
        present = CARDProbe(1);
        if (present && gCardSlots[1].state < 2) {
            sCardCmdSlot = 1;
            sCardCmd = CARD_CMD_MOUNT;
            sCardResult = CARD_UI_OK;
            if (OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_MSG(CARD_CMD_MOUNT, 0, 1, 0), OS_MESSAGE_NOBLOCK)) {
                gCardSlots[1].state = 3;
                gCardSlots[1].dirty = FALSE;
            }
        } else if (!present && gCardSlots[1].state > 2) {
            sCardCmdSlot = 1;
            sCardCmd = CARD_CMD_UNMOUNT;
            sCardResult = CARD_UI_OK;
            if (OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_MSG(CARD_CMD_UNMOUNT, 0, 1, 0), OS_MESSAGE_NOBLOCK)) {
                gCardSlots[1].dirty = FALSE;
            }
        }
    }

    while (OSReceiveMessage(CardThread_ReplyQueue(), &msg, OS_MESSAGE_NOBLOCK)) {
        slot = CARD_MSG_SLOT((u32)msg);
        switch (CARD_MSG_CMD((u32)msg)) {
        case CARD_CMD_ENABLE:
        case CARD_CMD_DISABLE:
            sCardEnabled = (u8)((u32)msg >> 8);
            break;
        case CARD_CMD_REFRESH:
            sCardRefreshAsked = TRUE;
            sCardRefreshing = TRUE;
            break;
        default:
            res = CARD_MSG_RES((u32)msg);
            switch (res) {
            case CARD_RESULT_SIZE:
            case CARD_RESULT_EXIST:
            case CARD_RESULT_READY:
                gCardSlots[slot].state = CARD_SLOT_READY;
                break;
            case CARD_RESULT_INSSPACE:
            case CARD_RESULT_NOENT:
                gCardSlots[slot].state = CARD_SLOT_READY;
                break;
            case CARD_RESULT_WRONGDEVICE:
                gCardSlots[slot].state = CARD_SLOT_WRONG;
                break;
            case CARD_RESULT_BROKEN:
                gCardSlots[slot].state = CARD_SLOT_NEED_FORMAT;
                break;
            case CARD_RESULT_ENCODING:
                gCardSlots[slot].state = CARD_SLOT_ENCODING;
                break;
            case CARD_RESULT_NOCARD:
                gCardSlots[slot].state = CARD_SLOT_EMPTY;
                break;
            default:
                gCardSlots[slot].state = 10;
                break;
            }
            sCardResult = Card_MapError(res);
            if (CARD_MSG_CMD((u32)msg) != CARD_CMD_MOUNT && CARD_MSG_CMD((u32)msg) != CARD_CMD_UNMOUNT) {
                sCardDestResult = Card_MapError(CARD_MSG_RES((u32)msg));
                if (sCardCmdSlot != (s16)CARD_MSG_SLOT((u32)msg)) {
                    sCardDestFile = CARD_MSG_FILE((u32)msg);
                }
            }
            gCardSlots[slot].dirty = TRUE;
            sCardRefreshing = FALSE;
            break;
        }
    }
}

/* 0x8131C5C4 */
void CardThread_Init(void) {
    u8* icons = HeapAlloc(2 * CARD_MAX_FILES * 0x5A00);
    u8* comments = HeapAlloc(2 * CARD_MAX_FILES * 0x40);
    s32 i;

    for (i = 0; i < CARD_MAX_FILES; i++) {
        sCardIconBuf[0][i] = icons + i * 0x5A00;
        sCardComment[0][i] = comments + i * 0x40;
    }
    for (i = 0; i < CARD_MAX_FILES; i++) {
        sCardIconBuf[1][i] = icons + (i + CARD_MAX_FILES) * 0x5A00;
        sCardComment[1][i] = comments + (i + CARD_MAX_FILES) * 0x40;
    }
    sCardReadBuf = HeapAlloc(0x5A00);
    sCardCopyBuf = HeapAlloc(0x40000);
    sCardWork[0] = HeapAlloc(CARD_WORKAREA_SIZE);
    sCardWork[1] = HeapAlloc(CARD_WORKAREA_SIZE);
    OSInitMessageQueue(&sCardReplyQueue, sCardReplyMsgs, 16);
    OSInitMessageQueue(&sCardCmdQueue, sCardCmdMsgs, 16);
    OSCreateThread(&sCardThread, CardThread_Main, NULL, sCardStack + sizeof(sCardStack), sizeof(sCardStack), 17,
                   OS_THREAD_ATTR_DETACH);
    OSResumeThread(&sCardThread);
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_CMD_ENABLE, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C910 */
OSMessageQueue* CardThread_CmdQueue(void) {
    return &sCardCmdQueue;
}

/* 0x8131C91C */
OSMessageQueue* CardThread_ReplyQueue(void) {
    return &sCardReplyQueue;
}

/* 0x8131C928 */
void* CardThread_GetIcons(void) {
    return sCardIcons;
}

/* 0x8131C934 */
u8* CardThread_GetComment(u8 slot, s16 file) {
    return sCardComment[slot][file];
}

/* 0x8131C95C */
void CardFile_Clear(u8 slot, s16 file) {
    BOOL enabled = OSDisableInterrupts();

    sCardFiles[slot][file].used = 0;
    sCardFiles[slot][file].blocks = 0;
    sCardFiles[slot][file].canCopy = TRUE;
    sCardFiles[slot][file].canMove = TRUE;
    sCardFiles[slot][file].opened = 0;
    sCardIcons[slot][file].hasIcon = FALSE;
    sCardIcons[slot][file].hasBanner = FALSE;
    OSRestoreInterrupts(enabled);
}

/* 0x8131C9F8 */
void* CardThread_Main(void* arg) {
    OSMessage msg;
    BOOL enabled = FALSE;

    sCardMounted[0] = FALSE;
    sCardMounted[1] = FALSE;
    for (;;) {
        OSReceiveMessage(&sCardCmdQueue, &msg, OS_MESSAGE_BLOCK);
        if ((u8)enabled == TRUE) {
            switch (CARD_MSG_CMD((u32)msg)) {
            case CARD_CMD_ENABLE:
                enabled = TRUE;
                CardThread_SetEnabled(CARD_MSG_CMD((u32)msg), TRUE);
                break;
            case CARD_CMD_DISABLE:
                enabled = FALSE;
                CardThread_SetEnabled(CARD_MSG_CMD((u32)msg), FALSE);
                break;
            case CARD_CMD_MOUNT:
                CardThread_DoMount(CARD_MSG_SLOT((u32)msg));
                break;
            case CARD_CMD_UNMOUNT:
                CardThread_DoUnmount(CARD_MSG_SLOT((u32)msg));
                break;
            case CARD_CMD_FORMAT:
                CardThread_DoFormat(CARD_MSG_SLOT((u32)msg));
                break;
            case CARD_CMD_COPY:
                CardThread_DoCopyMove(CARD_MSG_SLOT((u32)msg), CARD_MSG_FILE((u32)msg));
                break;
            case CARD_CMD_MOVE:
                CardThread_DoCopyMove(CARD_MSG_SLOT((u32)msg), CARD_MSG_FILE((u32)msg));
                break;
            case CARD_CMD_ERASE:
                CardThread_DoErase(CARD_MSG_SLOT((u32)msg), CARD_MSG_FILE((u32)msg));
                break;
            case CARD_CMD_REFRESH:
                fn_8131ECC4();
                break;
            }
        } else {
            switch (CARD_MSG_CMD((u32)msg)) {
            case CARD_CMD_ENABLE:
                enabled = TRUE;
                CardThread_SetEnabled(CARD_MSG_CMD((u32)msg), TRUE);
                break;
            case CARD_CMD_DISABLE:
                enabled = FALSE;
                CardThread_SetEnabled(CARD_MSG_CMD((u32)msg), FALSE);
                break;
            case CARD_CMD_REFRESH:
                fn_8131ECC4();
                break;
            default:
                CardThread_Fail(CARD_MSG_SLOT((u32)msg), CARD_MSG_CMD((u32)msg), CARD_RESULT_NOCARD);
                break;
            }
        }
    }
}

/* 0x8131CB64: a file can't be copied to the other card if that card already
   holds a file of the same name. */
void CardThread_MarkConflicts(void) {
    u8 slot;
    s16 file;

    for (slot = 0; slot < 2; slot++) {
        for (file = 0; file < CARD_MAX_FILES; file++) {
            sCardFiles[slot][file].opened = 0;
            if (sCardMounted[slot] && sCardMounted[slot ^ 1] && fn_8131E044(slot, file) < 0) {
                sCardFiles[slot][file].opened = 1;
            }
        }
    }
}

/* 0x8131CC10 */
void CardThread_SetEnabled(u8 cmd, BOOL enabled) {
    OSSendMessage(&sCardReplyQueue, (OSMessage)CARD_MSG(cmd, enabled, 0, 0), OS_MESSAGE_BLOCK);
}

/* 0x8131CC48: a command failed. Forget the card unless it is only full. */
s32 CardThread_Fail(u8 slot, s32 cmd, s32 result) {
    if (result != CARD_RESULT_EXIST && result != CARD_RESULT_SIZE && result != CARD_RESULT_NOENT &&
        result != CARD_RESULT_INSSPACE) {
        CardThread_ClearFiles(slot);
        if (result != CARD_RESULT_BROKEN && result != CARD_RESULT_ENCODING) {
            CARDUnmount(slot);
            sCardMounted[slot] = FALSE;
        }
    }
    CardThread_MarkConflicts();
    OSSendMessage(&sCardReplyQueue, (OSMessage)CARD_MSG(cmd, result, slot, 0), OS_MESSAGE_BLOCK);
    return 0;
}

/* 0x8131CD74 */
void CardThread_ClearFiles(u8 slot) {
    s16 file;

    for (file = 0; file < CARD_MAX_FILES; file++) {
        CardFile_Clear(slot, file);
    }
}

/* 0x8131D670 */
void CardThread_DoUnmount(u8 slot) {
    s16 file;

    for (file = 0; file < CARD_MAX_FILES; file++) {
        CardFile_Clear(slot, file);
    }
    CARDUnmount(slot);
    sCardMounted[slot] = FALSE;
    CardThread_MarkConflicts();
    OSSendMessage(&sCardReplyQueue, (OSMessage)CARD_MSG(CARD_CMD_UNMOUNT, CARD_RESULT_NOCARD, slot, 0),
                  OS_MESSAGE_BLOCK);
}

/* 0x8131DAE8 */
void fn_8131DAE8(void) {
    sCardUnk4E0 = 0;
}
