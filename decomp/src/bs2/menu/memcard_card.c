// Memory Card worker thread (PAL 1.0 BS2, 0x8131BF6C..0x8131ECC4). The UI
// posts commands (cmd | slot << 16 | file << 24) to a message queue; the
// thread mounts the cards, reads the directory, icons and comments, and
// copies / moves / erases files and formats cards with the SDK CARD API,
// then replies with (cmd | result << 8 | slot << 16 | file << 24).
//

#include <dolphin/card.h>
#include "bs2/menu.h"
#include <string.h>
#include <stdio.h>

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
    u8 animating;          /* 0x04 */
    u8 bounce;             /* 0x05 CARD_STAT_ANIM_BOUNCE */
    u8 firstSpeed;         /* 0x06 frames x 4 */
    u8 lastSpeed;          /* 0x07 */
    u8 format[8];          /* 0x08 per frame, 9: CI8, 5: RGB5A3 */
    u16 speeds;            /* 0x10 CARDDir.iconSpeed */
    s16 duration;          /* 0x12 */
    u8* banner;            /* 0x14 */
    u8* bannerTlut;        /* 0x18 */
    u8* image[8];          /* 0x1C */
    u8* tlut;              /* 0x3C shared palette of the CI8 frames */
} CardIconInfo;

static CardSlot gCardSlots[2];
static CardFile sCardFiles[2][CARD_MAX_FILES];
static OSMessageQueue sCardReplyQueue;
static OSMessageQueue sCardCmdQueue;
static OSMessage sCardReplyMsgs[16];
static OSMessage sCardCmdMsgs[16];
static OSThread sCardThread;
static u8 sCardStack[0x8000];
static char sCardTmpName[CARD_FILENAME_MAX];  /* name of the file being created */
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
static u32 sCardWriting;

void* HeapAlloc(u32 size);
OSMessageQueue* CardThread_CmdQueue(void);
OSMessageQueue* CardThread_ReplyQueue(void);
void* CardThread_Main(void* arg);
void CardThread_MarkConflicts(void);
void CardThread_SetEnabled(s8 cmd, BOOL enabled);
s32 CardThread_Fail(u8 slot, s32 cmd, s32 result);
void CardThread_ClearFiles(u8 slot);
void CardThread_DoUnmount(u8 slot);
void CardThread_DoMount(u8 slot);
void CardThread_DoFormat(u8 slot);
void CardThread_DoCopyMove(u8 slot, s16 file, s32 cmd);
void CardThread_DoErase(u8 slot, s16 file);
void CardThread_DoRefresh(void);
s32 CardThread_FindInOther(u8 slot, s16 file);

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
    sCardCmd = CARD_CMD_FORMAT;
    msg = (slot << 16) | CARD_CMD_FORMAT;
    sCardResult = CARD_UI_OK;
    sCardDestResult = CARD_UI_OK;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)msg, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C000 */
void CardThread_Copy(u8 slot, s16 file) {
    u32 msg;

    sCardCmdSlot = slot;
    sCardResult = CARD_UI_OK;
    sCardDestResult = CARD_UI_OK;
    sCardCmd = CARD_CMD_COPY;
    msg = file;
    msg = (msg << 24) | ((slot << 16) & 0x10000) | CARD_CMD_COPY;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)msg, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C05C */
void CardThread_Move(u8 slot, s16 file) {
    u32 msg;

    sCardCmdSlot = slot;
    sCardResult = CARD_UI_OK;
    sCardDestResult = CARD_UI_OK;
    sCardCmd = CARD_CMD_MOVE;
    msg = file;
    msg = (msg << 24) | ((slot << 16) & 0x10000) | CARD_CMD_MOVE;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)msg, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C0B8 */
void CardThread_Erase(u8 slot, s16 file) {
    u32 msg;

    sCardCmdSlot = slot;
    sCardResult = CARD_UI_OK;
    sCardDestResult = CARD_UI_OK;
    sCardCmd = CARD_CMD_ERASE;
    msg = file;
    msg = (msg << 24) | ((slot << 16) & 0x10000) | CARD_CMD_ERASE;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)msg, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C114 */
void CardThread_Enable(void) {
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_CMD_ENABLE, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C140 */
void CardThread_UnmountAll(void) {
    u32 msg;

    sCardCmdSlot = 0;
    sCardCmd = CARD_CMD_UNMOUNT;
    sCardResult = CARD_UI_OK;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_MSG(CARD_CMD_UNMOUNT, 0, 0, 0), OS_MESSAGE_NOBLOCK);
    sCardCmd = CARD_CMD_UNMOUNT;
    sCardCmdSlot = 1;
    msg = CARD_MSG(CARD_CMD_UNMOUNT, 0, 1, 0);
    sCardResult = CARD_UI_OK;
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)msg, OS_MESSAGE_NOBLOCK);
    OSSendMessage(CardThread_CmdQueue(), (OSMessage)CARD_CMD_DISABLE, OS_MESSAGE_NOBLOCK);
}

/* 0x8131C1CC: SDK result to the message shown by the UI. */
s32 Card_MapError(s32 result) {
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
    u32 m;
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
            m = CARD_MSG(CARD_CMD_MOUNT, 0, 1, 0);
            sCardResult = CARD_UI_OK;
            if (OSSendMessage(CardThread_CmdQueue(), (OSMessage)m, OS_MESSAGE_NOBLOCK)) {
                gCardSlots[1].state = 3;
                gCardSlots[1].dirty = FALSE;
            }
        } else if (!present && gCardSlots[1].state > 2) {
            sCardCmdSlot = 1;
            sCardCmd = CARD_CMD_UNMOUNT;
            m = CARD_MSG(CARD_CMD_UNMOUNT, 0, 1, 0);
            sCardResult = CARD_UI_OK;
            if (OSSendMessage(CardThread_CmdQueue(), (OSMessage)m, OS_MESSAGE_NOBLOCK)) {
                gCardSlots[1].dirty = FALSE;
            }
        }
    }

    while (OSReceiveMessage(CardThread_ReplyQueue(), &msg, OS_MESSAGE_NOBLOCK)) {
        slot = CARD_MSG_SLOT((u32)msg);
        if (CARD_MSG_CMD((u32)msg) == CARD_CMD_ENABLE || CARD_MSG_CMD((u32)msg) == CARD_CMD_DISABLE) {
            sCardEnabled = (u8)((u32)msg >> 8);
        } else if (CARD_MSG_CMD((u32)msg) == CARD_CMD_REFRESH) {
            sCardRefreshAsked = TRUE;
            sCardRefreshing = TRUE;
        } else {
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
            m = (u8)(u32)msg;
            if ((s8)m != CARD_CMD_MOUNT && (s8)m != CARD_CMD_UNMOUNT) {
                sCardDestResult = Card_MapError(CARD_MSG_RES((u32)msg));
                if (sCardCmdSlot != (s16)CARD_MSG_SLOT((u32)msg)) {
                    sCardDestFile = CARD_MSG_FILE((u32)msg);
                }
            }
            gCardSlots[slot].dirty = TRUE;
            sCardRefreshing = FALSE;
        }
    }
}

/* 0x8131C5C4 */
void CardThread_Init(void) {
    u8* icons = HeapAlloc(2 * CARD_MAX_FILES * 0x5A00);
    u8* comments = HeapAlloc(2 * CARD_MAX_FILES * 0x40);
    int i;

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
                CardThread_DoCopyMove(CARD_MSG_SLOT((u32)msg), CARD_MSG_FILE((u32)msg), CARD_MSG_CMD((u32)msg));
                break;
            case CARD_CMD_MOVE:
                CardThread_DoCopyMove(CARD_MSG_SLOT((u32)msg), CARD_MSG_FILE((u32)msg), CARD_MSG_CMD((u32)msg));
                break;
            case CARD_CMD_ERASE:
                CardThread_DoErase(CARD_MSG_SLOT((u32)msg), CARD_MSG_FILE((u32)msg));
                break;
            case CARD_CMD_REFRESH:
                CardThread_DoRefresh();
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
                CardThread_DoRefresh();
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
    int slot;
    int file;

    for (slot = 0; slot < 2; slot++) {
        for (file = 0; file < CARD_MAX_FILES; file++) {
            sCardFiles[slot][file].opened = 0;
            if (sCardMounted[slot] && sCardMounted[slot ^ 1] && CardThread_FindInOther(slot, file) < 0) {
                sCardFiles[slot][file].opened = 1;
            }
        }
    }
}

/* 0x8131CC10 */
void CardThread_SetEnabled(s8 cmd, BOOL enabled) {
    u32 msg;

    msg = ((enabled << 8) & 0xFF00) | (u8)cmd;
    OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
}

/* 0x8131CC48: a command failed. Forget the card unless it is only full. */
/* inline copy of CardThread_MarkConflicts: only CardThread_Fail inlines it */
static inline void CardThread_MarkConflictsIn(void) {
    int slot;
    int file;

    for (slot = 0; slot < 2; slot++) {
        for (file = 0; file < CARD_MAX_FILES; file++) {
            sCardFiles[slot][file].opened = 0;
            if (sCardMounted[slot] && sCardMounted[slot ^ 1] && CardThread_FindInOther(slot, file) < 0) {
                sCardFiles[slot][file].opened = 1;
            }
        }
    }
}

s32 CardThread_Fail(u8 slot, s32 cmd, s32 result) {

    if (result != CARD_RESULT_EXIST && result != CARD_RESULT_SIZE && result != CARD_RESULT_NOENT &&
        result != CARD_RESULT_INSSPACE) {
        CardThread_ClearFiles(slot);
        if (result != CARD_RESULT_BROKEN && result != CARD_RESULT_ENCODING) {
            CARDUnmount(slot);
            sCardMounted[slot] = FALSE;
        }
    }
    CardThread_MarkConflictsIn();
    {
        u32 msg = ((slot << 16) & 0x10000) | ((result << 8) & 0xFF00) | (cmd & 0xFF);
        OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
    }
    return 0;
}

/* 0x8131CD74 */
void CardThread_ClearFiles(u8 slot) {
    BOOL enabled;
    s16 file;

    for (file = 0; file < CARD_MAX_FILES; file++) {
        enabled = OSDisableInterrupts();
        sCardFiles[slot][file].used = 0;
        sCardFiles[slot][file].blocks = 0;
        sCardFiles[slot][file].canCopy = TRUE;
        sCardFiles[slot][file].canMove = TRUE;
        sCardFiles[slot][file].opened = 0;
        sCardIcons[slot][file].hasIcon = FALSE;
        sCardIcons[slot][file].hasBanner = FALSE;
        OSRestoreInterrupts(enabled);
    }
}

/* 0x8131CE08: where the banner sits in the file's image buffer. Returns its
   size in bytes. */
s32 CardThread_SetBanner(u8 slot, s16 file, CARDDir* dir) {
    s32 size = 0;

    /* the code really stores the image buffer minus the record's own address
     * (matches byte for byte) */
#define INFO sCardIcons[slot][file]
    switch (dir->bannerFormat & CARD_STAT_BANNER_MASK) {
    case CARD_STAT_BANNER_C8:
        INFO.hasBanner = TRUE;
        INFO.bannerFormat = GX_TF_C8;
        INFO.banner = (u8*)(sCardIconBuf[slot][file] - &INFO.hasBanner);
        size = CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT + 0x200;
        INFO.bannerTlut = INFO.banner + CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT;
        INFO.image[0] = INFO.bannerTlut + 0x200;
        break;
    case CARD_STAT_BANNER_RGB5A3:
        INFO.hasBanner = TRUE;
        INFO.bannerFormat = GX_TF_RGB5A3;
        size = CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT * 2;
        INFO.banner = (u8*)(sCardIconBuf[slot][file] - &INFO.hasBanner);
        INFO.image[0] = INFO.banner + CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT * 2;
        break;
    default:
        INFO.hasBanner = FALSE;
        INFO.bannerFormat = 0;
        INFO.image[0] = (u8*)(sCardIconBuf[slot][file] - &INFO.hasBanner);
        break;
    }
#undef INFO
    return size;
}

/* 0x8131CF54: lay out the icon frames after the banner. Returns their size
   in bytes, palette included. */
s32 CardThread_SetIcons(u8 slot, s16 file, CARDDir* dir) {
    BOOL ci8 = FALSE;
    s32 total = 0;
    s32 size;
    s32 speed;
    int i;

#define INFO sCardIcons[slot][file]
    INFO.frames = 0;
    INFO.duration = 0;
    INFO.speeds = dir->iconSpeed;
    INFO.firstSpeed = (dir->iconSpeed & 3) << 2;
    for (i = 0; i < CARD_ICON_MAX; i++) {
        speed = (dir->iconSpeed >> (i * 2)) & 3;
        if (speed != 0) {
            INFO.duration += speed << 2;
        } else {
            INFO.lastSpeed = ((dir->iconSpeed >> ((i - 1) * 2)) & 3) << 2;
            goto done;
        }
    }
    INFO.lastSpeed = ((dir->iconSpeed >> ((CARD_ICON_MAX - 1) * 2)) & 3) << 2;
done:

    if (!(dir->iconSpeed & 3) || !(dir->iconFormat & 3)) {
        INFO.hasIcon = FALSE;
        return 0;
    }
    for (i = 0; i < CARD_ICON_MAX; i++) {
        if ((dir->iconSpeed >> (i * 2)) & 3) {
        switch ((dir->iconFormat >> (i * 2)) & 3) {
        case CARD_STAT_ICON_C8:
            INFO.format[i] = GX_TF_C8;
            size = CARD_ICON_WIDTH * CARD_ICON_HEIGHT;
            ci8 = TRUE;
            break;
        case CARD_STAT_ICON_RGB5A3:
            INFO.format[i] = GX_TF_RGB5A3;
            size = CARD_ICON_WIDTH * CARD_ICON_HEIGHT * 2;
            break;
        case CARD_STAT_ICON_NONE:
            size = 0;
            INFO.format[i] = (&INFO.lastSpeed)[i]; /* the previous frame's format */
            break;
        }
        if (i < CARD_ICON_MAX - 1) {
            INFO.image[i + 1] = (u8*)(size + (u32)INFO.image[i]);
        } else {
            INFO.tlut = (u8*)(size + (u32)INFO.image[i]);
        }
        total += size;
        INFO.frames++;
        } else {
            INFO.tlut = INFO.image[i];
            goto icons_done;
        }
    }
icons_done:
    if (ci8) {
        total += 0x200;
    }
    INFO.hasIcon = TRUE;
    INFO.bounce = dir->bannerFormat & CARD_STAT_ANIM_BOUNCE;
    INFO.animating = TRUE;
#undef INFO
    return total;
}

/* 0x8131D268: read the banner and icon images of a file. */
s32 CardThread_ReadIcon(u8 slot, s16 file, CARDDir* dir, CARDFileInfo* fi) {
    u32 addr = dir->iconAddr;
    u32 base = addr & ~0x1FF;
    s32 ofs = addr - base;
    s32 size;
    s32 len;
    u32 sector;
    u32 limit;
    s32 result;

    size = CardThread_SetBanner(slot, file, dir);
    size += CardThread_SetIcons(slot, file, dir);
    len = (size + ofs + 0x1FF) & ~0x1FF;
    if (CARDGetSectorSize(slot, &sector) < 0) {
        sCardIcons[slot][file].hasBanner = FALSE;
        sCardIcons[slot][file].hasIcon = FALSE;
        return 0;
    }
    limit = dir->length * sector;
    if (base > limit) {
        sCardIcons[slot][file].hasBanner = FALSE;
        sCardIcons[slot][file].hasIcon = FALSE;
        return 0;
    }
    if (base + len > limit) {
        sCardIcons[slot][file].hasBanner = FALSE;
        sCardIcons[slot][file].hasIcon = FALSE;
        return 0;
    }
    if (size > 0) {
        result = CARDRead(fi, sCardReadBuf, len, base);
        if (result < 0) {
            return result;
        }
        memcpy(sCardIconBuf[slot][file], sCardReadBuf + ofs, size);
        DCStoreRange(sCardIconBuf[slot][file], len);
    }
    return 0;
}

/* 0x8131D408: read the two comment lines of a file. */
s32 CardThread_ReadComment(u8 slot, s16 file, CARDDir* dir, CARDFileInfo* fi) {
    s32 result;
    u32 addr = dir->commentAddr;
    s32 base = addr & ~0x1FF;
    s32 ofs = addr - base;
    u32 sector;
    s32 len = ((addr + CARD_COMMENT_SIZE + 0x1FF) & ~0x1FF) - base;

    result = CARDGetSectorSize(slot, &sector);
    if (result >= 0) {
        if (base < 0 || (u32)base > dir->length * sector) {
            result = 0;
        } else if (base + len < 0 || (u32)(base + len) > dir->length * sector) {
            result = 0;
        } else {
            result = CARDRead(fi, sCardCommentRead, len, base);
            if (result >= 0) {
                memset(sCardComment[slot][file], 0, CARD_COMMENT_SIZE);
                memcpy(sCardComment[slot][file], sCardCommentRead + ofs, CARD_COMMENT_SIZE);
                return 0;
            }
        }
    }
    memset(sCardComment[slot][file], 0, CARD_COMMENT_SIZE);
    return result;
}

/* 0x8131D548: everything the browser shows about one file. */
s32 CardThread_ReadEntry(u8 slot, s16 file, CARDDir* dir) {
    CARDFileInfo fi;
    s32 result;

    result = CARDFastOpen(slot, file, &fi);
    if (result < 0) {
        return result;
    }
    result = CardThread_ReadIcon(slot, file, dir, &fi);
    if (result < 0) {
        return result;
    }
    result = CardThread_ReadComment(slot, file, dir, &fi);
    if (result < 0) {
        return result;
    }
    result = CARDClose(&fi);
    if (result < 0) {
        return result;
    }
    sCardFiles[slot][file].used = TRUE;
    sCardFiles[slot][file].blocks = dir->length;
    sCardFiles[slot][file].time = dir->time;
    sCardFiles[slot][file].canCopy = (dir->permission & CARD_ATTR_NO_COPY) != CARD_ATTR_NO_COPY ? TRUE : FALSE;
    sCardFiles[slot][file].canMove = (dir->permission & CARD_ATTR_NO_MOVE) != CARD_ATTR_NO_MOVE ? TRUE : FALSE;
    return 0;
}

/* 0x8131D670 */
void CardThread_DoUnmount(u8 slot) {
    s16 file;
    u32 msg;

    for (file = 0; file < CARD_MAX_FILES; file++) {
        CardFile_Clear(slot, file);
    }
    CARDUnmount(slot);
    sCardMounted[slot] = FALSE;
    CardThread_MarkConflicts();
    msg = CARD_MSG(CARD_CMD_UNMOUNT, CARD_RESULT_NOCARD, slot, 0);
    OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
}

/* A CARD call failed while mounting: carry on with the next file, check a
   broken card, or give up (FALSE). */
inline BOOL CardThread_Recover(u8 slot, s32 result) {
    switch (result) {
    case CARD_RESULT_NOFILE:
        return TRUE;
    case CARD_RESULT_BROKEN:
        result = CARDCheck(slot);
        if (result < 0) {
            return CardThread_Fail(slot, CARD_CMD_MOUNT, result);
        }
        return TRUE;
    case CARD_RESULT_IOERROR:
    default:
        return CardThread_Fail(slot, CARD_CMD_MOUNT, result);
    }
}

/* 0x8131D6FC */
void CardThread_DoMount(u8 slot) {
    static const u8 noGame[4] = { 0, 0, 0, 0 };
    static const u8 noCompany[2] = { 0, 0 };
    CARDDir dir;
    CARDDir dir2;
    char name[CARD_FILENAME_MAX + 1];
    u8 game[4];
    u8 company[2];
    u16 memSize;
    u32 sector;
    s32 freeBytes;
    s32 freeFiles;
    BOOL enabled;
    s32 result;
    s16 file;
    u16 i;

    result = CARDMount(slot, sCardWork[slot], NULL);
    if (result < 0) {
        if (!CardThread_Recover(slot, result)) {
            return;
        }
    } else {
        result = CARDCheck(slot);
        if (result < 0) {
            goto fail;
        }
    }

    for (file = 0; file < CARD_MAX_FILES; file++) {
        enabled = OSDisableInterrupts();
        sCardFiles[slot][file].used = 0;
        sCardFiles[slot][file].blocks = 0;
        sCardFiles[slot][file].canCopy = TRUE;
        sCardFiles[slot][file].canMove = TRUE;
        sCardFiles[slot][file].opened = 0;
        sCardIcons[slot][file].hasIcon = FALSE;
        sCardIcons[slot][file].hasBanner = FALSE;
        OSRestoreInterrupts(enabled);
    }
    for (file = 0; file < CARD_MAX_FILES; file++) {
        result = __CARDGetStatusEx(slot, file, &dir);
        if (result < 0) {
            if (!CardThread_Recover(slot, result)) {
                return;
            }
            continue;
        }
        /* a file the IPL left behind while copying */
        *(u16*)company = *(u16*)noCompany;
        *(u32*)game = *(u32*)noGame;
        if (strncmp((char*)dir.fileName, "Broken File", 11) == 0 && memcmp(dir.gameName, game, 4) == 0 &&
            memcmp(dir.company, company, 2) == 0) {
            result = CARDFastDelete(slot, file);
            if (result < 0) {
                goto fail;
            }
        } else {
            result = CardThread_ReadEntry(slot, file, &dir);
            if (result < 0) {
                goto fail;
            }
        }
    }

    CARDGetMemSize(slot, &memSize);
    CARDGetSectorSize(slot, &sector);
    CARDFreeBlocks(slot, &freeBytes, &freeFiles);
    if (CARDGetResultCode(slot) >= 0) {
        enabled = OSDisableInterrupts();
        gCardSlots[slot].userBlocks = ((u32)memSize << 17) / sector - 5;
        gCardSlots[slot].sectorSize = sector;
        gCardSlots[slot].freeFiles = freeFiles;
        gCardSlots[slot].freeBlocks = (u32)freeBytes / sector;
        OSRestoreInterrupts(enabled);
    }
    sCardMounted[slot] = TRUE;
    CardThread_MarkConflicts();
    {
        u32 msg = CARD_MSG(CARD_CMD_MOUNT, CARD_RESULT_READY, slot, 0);
        OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
    }

    /* left over from a debug listing of the file names */
    for (i = 0; i < CARD_MAX_FILES; i++) {
        if (__CARDGetStatusEx(slot, i, &dir2) == 0) {
            memcpy(name, dir2.fileName, CARD_FILENAME_MAX);
            name[CARD_FILENAME_MAX] = '\0';
        }
    }
    return;

fail:
    if (result != CARD_RESULT_EXIST && result != CARD_RESULT_SIZE && result != CARD_RESULT_NOENT &&
        result != CARD_RESULT_INSSPACE) {
        for (file = 0; file < CARD_MAX_FILES; file++) {
            CardFile_Clear(slot, file);
        }
        if (result != CARD_RESULT_BROKEN && result != CARD_RESULT_ENCODING) {
            CARDUnmount(slot);
            sCardMounted[slot] = FALSE;
        }
    }
    CardThread_MarkConflicts();
    {
        u32 msg = CARD_MSG(CARD_CMD_MOUNT, result, slot, 0);
        OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
    }
}

/* 0x8131DAE8: CARDWriteAsync callback. */
void CardThread_WriteDone(s32 chan, s32 result) {
    sCardWriting = FALSE;
}

/* 0x8131DAF4: copy the data of a file into the file created on the other
   card, a chunk of sCardCopyBuf at a time. Cancelled if the card is pulled. */
s32 CardThread_CopyData(u8 slot, s16 file) {
    u32 sector;
    CARDDir dir;
    s32 result;
    s32 length;
    u32 offset;
    u32 blocks;
    s32 block;
    u32 chunk;
    BOOL cancelled = FALSE;

    result = __CARDGetStatusEx(slot, file, &dir);
    if (result >= 0) {
        result = CARDGetSectorSize(slot, &sector);
        if (result >= 0) {
            chunk = 0x40000 / sector;
            result = CARDFastOpen(slot, file, &sCardSrcInfo);
            if (result >= 0) {
                for (block = 0; block < dir.length; block += chunk) {
                    blocks = dir.length - block > chunk ? chunk : dir.length - block;
                    offset = block * sector;
                    length = blocks * sector;
                    result = CARDRead(&sCardSrcInfo, sCardCopyBuf, length, offset);
                    if (result < 0) {
                        goto fail;
                    }
                    sCardWriting = TRUE;
                    result = CARDWriteAsync(&sCardDstInfo, sCardCopyBuf, length, offset, CardThread_WriteDone);
                    if (result < 0) {
                        goto fail;
                    }
                    while (sCardWriting) {
                        if (!CARDProbe(slot) && !(u16)cancelled) {
                            CARDCancel(&sCardDstInfo);
                            cancelled = TRUE;
                        }
                    }
                }
                result = CARDClose(&sCardSrcInfo);
                if (result >= 0) {
                    result = CARDClose(&sCardDstInfo);
                    if (result >= 0) {
                        return 0;
                    }
                }
            }
        }
    }
fail:
    CARDFastDelete(slot ^ 1, sCardDstInfo.fileNo);
    return result;
}

/* 0x8131DC8C: give the copy the source's directory entry (one more copy
   counted, stamped with the current time). */
s32 CardThread_CopyFile(u8 slot, s16 file, s16 dst, BOOL* done) {
    CARDDir dir;
    u32 time;
    int other;
    s32 result;

    result = __CARDGetStatusEx(slot, file, &dir);
    if (result < 0) {
        return result;
    }
    dir.copyTimes++;
    result = __CARDSetStatusEx(slot, file, &dir);
    if (result < 0) {
        return result;
    }
    time = dir.time;
    dir.time = OSTicksToSeconds(OSGetTime());
    other = slot ^ 1;
    result = __CARDSetStatusEx(other, dst, &dir);
    if (result < 0) {
        if (result != CARD_RESULT_NOCARD) {
            dir.copyTimes--;
        }
        dir.time = time;
        __CARDSetStatusEx(slot, file, &dir);
        return result;
    }
    *done = TRUE;
    result = CardThread_ReadEntry(other, dst, &dir);
    if (result < 0) {
        return result;
    }
    return 0;
}

/* 0x8131DD9C: hide the source as a "Broken File" (no game code, so it can't
   be mistaken for the moved file), give its entry to the copy, then delete
   the source. */
s32 CardThread_MoveFile(u8 slot, s16 file, s16 dst, BOOL* done) {
    CARDDir dir;
    CARDDir hidden;
    BOOL enabled;
    u8 other;
    s16 n = 0;
    s32 result;

    result = __CARDGetStatusEx(slot, file, &dir);
    if (result < 0) {
        return result;
    }
    hidden = dir;
    memset(hidden.company, 0, sizeof(hidden.company));
    memset(hidden.gameName, 0, sizeof(hidden.gameName));
    for (;;) {
        memset(hidden.fileName, 0, CARD_FILENAME_MAX);
        sprintf((char*)hidden.fileName, "Broken File%03d", n);
        result = __CARDSetStatusEx(slot, file, &hidden);
        if (result < 0 && result != CARD_RESULT_EXIST) {
            return result;
        }
        n++;
        if (result == CARD_RESULT_READY || n >= 128) {
            break;
        }
    }

    other = slot ^ 1;
    result = __CARDSetStatusEx(other, dst, &dir);
    if (result < 0) {
        if (result == CARD_RESULT_NOCARD) {
            CARDFastDelete(slot, file);
            enabled = OSDisableInterrupts();
            sCardFiles[slot][file].used = 0;
            sCardFiles[slot][file].blocks = 0;
            sCardFiles[slot][file].canCopy = TRUE;
            sCardFiles[slot][file].canMove = TRUE;
            sCardFiles[slot][file].opened = 0;
            sCardIcons[slot][file].hasIcon = FALSE;
            sCardIcons[slot][file].hasBanner = FALSE;
            OSRestoreInterrupts(enabled);
        } else {
            __CARDSetStatusEx(slot, file, &dir);
        }
        return result;
    }
    *done = TRUE;
    result = CARDFastDelete(slot, file);
    if (result >= 0) {
        enabled = OSDisableInterrupts();
        sCardFiles[slot][file].used = 0;
        sCardFiles[slot][file].blocks = 0;
        sCardFiles[slot][file].canCopy = TRUE;
        sCardFiles[slot][file].canMove = TRUE;
        sCardFiles[slot][file].opened = 0;
        sCardIcons[slot][file].hasIcon = FALSE;
        sCardIcons[slot][file].hasBanner = FALSE;
        OSRestoreInterrupts(enabled);
        result = __CARDGetStatusEx(other, dst, &dir);
        if (result >= 0) {
            result = CardThread_ReadEntry(other, dst, &dir);
            if (result >= 0) {
                return 0;
            }
        }
    }
    return result;
}

/* 0x8131E044: CARD_RESULT_EXIST if the other card holds a file of the same
   game, company and name. */
s32 CardThread_FindInOther(u8 slot, s16 file) {
    CARDDir dir;
    CARDDir other;
    int i;
    s32 result;

    result = __CARDGetStatusEx(slot, file, &dir);
    if (result < 0) {
        return result;
    }
    i = 0;
    do {
        result = __CARDGetStatusEx(slot ^ 1, i, &other);
        if (result < 0 && result != CARD_RESULT_NOFILE) {
            return result;
        }
        if (result == CARD_RESULT_READY && memcmp(dir.gameName, other.gameName, 4) == 0 &&
            memcmp(dir.company, other.company, 2) == 0 && memcmp(dir.fileName, other.fileName, CARD_FILENAME_MAX) == 0) {
            return CARD_RESULT_EXIST;
        }
        i++;
    } while (i < CARD_MAX_FILES);
    return 0;
}

/* Reply with an error; forget the card unless it is only full. */
inline void CardThread_Error(u8 slot, s32 cmd, s32 result) {
    s16 file;

    if (result != CARD_RESULT_EXIST && result != CARD_RESULT_SIZE && result != CARD_RESULT_NOENT &&
        result != CARD_RESULT_INSSPACE) {
        for (file = 0; file < CARD_MAX_FILES; file++) {
            CardFile_Clear(slot, file);
        }
        if (result != CARD_RESULT_BROKEN && result != CARD_RESULT_ENCODING) {
            CARDUnmount(slot);
            sCardMounted[slot] = FALSE;
        }
    }
    CardThread_MarkConflicts();
    {
        u32 msg = ((slot << 16) & 0x10000) | ((result << 8) & 0xFF00) | (cmd & 0xFF);
        OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
    }
}

/* Refresh the block counts the UI shows for a card. */
inline void CardThread_UpdateSlot(u8 slot) {
    u32 sector;
    s32 freeBytes;
    s32 freeFiles;
    s32 pad;
    u16 memSize;
    BOOL enabled;

    (void)&pad;
    CARDGetMemSize(slot, &memSize);
    CARDGetSectorSize(slot, &sector);
    CARDFreeBlocks(slot, &freeBytes, &freeFiles);
    if (CARDGetResultCode(slot) >= 0) {
        enabled = OSDisableInterrupts();
        gCardSlots[slot].userBlocks = ((u32)memSize << 17) / sector - 5;
        gCardSlots[slot].sectorSize = sector;
        gCardSlots[slot].freeFiles = freeFiles;
        gCardSlots[slot].freeBlocks = (u32)freeBytes / sector;
        OSRestoreInterrupts(enabled);
    }
}

/* 0x8131E130 */
void CardThread_DoCopyMove(u8 slot, s16 file, s32 cmd) {
    CARDDir dir2;
    BOOL done = FALSE;
    u32 sector1;
    u32 sector0;
    CARDDir dir;
    u32 sector;
    BOOL created = FALSE;
    s16 dst = -1;
    s32 error;
    s16 n;
    u8 other;
    s32 result;

    result = CardThread_FindInOther(slot, file);
    if (result < 0) {
        CardThread_Error(slot, cmd, result);
        return;
    }

    error = 0;
    sector = 0;
    result = CARDGetSectorSize(0, &sector0);
    if (result < 0) {
        error = result;
    } else {
        result = CARDGetSectorSize(1, &sector1);
        if (result < 0) {
            error = result;
        } else if (sector0 != sector1) {
            error = CARD_RESULT_SIZE;
        } else {
            sector = sector0;
        }
    }
    if (error < 0) {
        CardThread_Error(slot, cmd, error);
        return;
    }

    result = 0;
    if (__CARDGetStatusEx(slot, file, &dir) >= 0) {
        switch (cmd) {
        case CARD_CMD_COPY:
            if (dir.permission & CARD_ATTR_NO_COPY) {
                result = CARD_RESULT_NOPERM;
            }
            break;
        case CARD_CMD_MOVE:
            if (dir.permission & CARD_ATTR_NO_MOVE) {
                result = CARD_RESULT_NOPERM;
            }
            break;
        }
    } else {
        result = CARDGetResultCode(slot);
    }
    if (result < 0) {
        CardThread_Error(slot, cmd, result);
        return;
    }

    if (__CARDGetStatusEx(slot, file, &dir2) >= 0) {
        other = slot ^ 1;
        n = 0;
        for (;;) {
            sprintf(sCardTmpName, "Broken File%03d", n);
            result = CARDCreate(other, sCardTmpName, dir2.length * sector, &sCardDstInfo);
            if (result < 0 && result != CARD_RESULT_EXIST) {
                break;
            }
            n++;
            if (result == CARD_RESULT_READY || n >= 128) {
                result = n < 128 ? sCardDstInfo.fileNo : -128;
                break;
            }
        }
        dst = result;
        if (dst < 0) {
            CardThread_Error(other, cmd, dst);
            return;
        }
        created = TRUE;
        if (CardThread_CopyData(slot, file) >= 0) {
            if (cmd == CARD_CMD_COPY) {
                if (CardThread_CopyFile(slot, file, dst, &done) < 0) {
                    goto cleanup;
                }
            } else if (cmd == CARD_CMD_MOVE && CardThread_MoveFile(slot, file, dst, &done) < 0) {
                goto cleanup;
            }
            CardThread_UpdateSlot(slot);
            CardThread_UpdateSlot(other);
            CardThread_MarkConflicts();
            OSSendMessage(&sCardReplyQueue, (OSMessage)((file << 24) | ((slot << 16) & 0x10000) | (CARD_CMD_MOUNT & 0xFF)), OS_MESSAGE_BLOCK);
            CardThread_MarkConflicts();
            OSSendMessage(&sCardReplyQueue, (OSMessage)CARD_MSG(cmd, 0, other, dst), OS_MESSAGE_BLOCK);
            return;
        }
    }

cleanup:
    other = slot ^ 1;
    result = CARDGetResultCode(other);
    if (result < 0) {
        CardThread_Error(other, cmd, result);
    } else {
        if (created && !done) {
            CARDFastDelete(other, dst);
        }
        if (done && __CARDGetStatusEx(other, dst, &dir2) == 0) {
            CardThread_ReadEntry(other, dst, &dir2);
        }
        CardThread_UpdateSlot(other);
        CardThread_MarkConflicts();
        OSSendMessage(&sCardReplyQueue, (OSMessage)CARD_MSG(CARD_CMD_MOUNT, 0, other, file), OS_MESSAGE_BLOCK);
    }
    result = CARDGetResultCode(slot);
    if (result < 0) {
        CardThread_Error(slot, cmd, result);
        return;
    }
    CardThread_UpdateSlot(slot);
    CardThread_MarkConflicts();
    OSSendMessage(&sCardReplyQueue, (OSMessage)CARD_MSG(CARD_CMD_MOUNT, 0, slot, file), OS_MESSAGE_BLOCK);
}

/* 0x8131E994 */
void CardThread_DoFormat(u8 slot) {
    s32 result;
    s32 unused;

    result = CARDFormat(slot);
    if (result < 0) {
        CardThread_Error(slot, CARD_CMD_FORMAT, result);
        return;
    }
    sCardMounted[slot] = TRUE;
    CardThread_UpdateSlot(slot);
    CardThread_MarkConflicts();
    {
        u32 msg = CARD_MSG(CARD_CMD_FORMAT, 0, slot, 0);
        OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
    }
}

/* 0x8131EB04 */
void CardThread_DoErase(u8 slot, s16 file) {
    BOOL enabled;
    s32 result;
    s32 unused;

    result = CARDFastDelete(slot, file);
    if (result < 0) {
        CardThread_Error(slot, CARD_CMD_ERASE, result);
        return;
    }
    enabled = OSDisableInterrupts();
    sCardFiles[slot][file].used = 0;
    sCardFiles[slot][file].blocks = 0;
    sCardFiles[slot][file].canCopy = TRUE;
    sCardFiles[slot][file].canMove = TRUE;
    sCardFiles[slot][file].opened = 0;
    sCardIcons[slot][file].hasIcon = FALSE;
    sCardIcons[slot][file].hasBanner = FALSE;
    OSRestoreInterrupts(enabled);
    CardThread_UpdateSlot(slot);
    CardThread_MarkConflicts();
    {
        u32 msg = ((u8)file << 24) | ((slot << 16) & 0x10000) | (CARD_CMD_ERASE & 0xFF);
        OSSendMessage(&sCardReplyQueue, (OSMessage)msg, OS_MESSAGE_BLOCK);
    }
}

/* 0x8131ECC4: count the blocks used by the running game's own files */
void CardThread_DoRefresh(void) {
    CARDDir dir;
    s32 slot;
    s32 file;
    s32 blocks;
    s32 result;
    u8* id = (u8*)0x80000000;

    for (slot = 0; slot < 2; slot++) {
        if (sCardMounted[slot]) {
            blocks = 0;
            for (file = 0; file < CARD_MAX_FILES; file++) {
                result = __CARDGetStatusEx(slot, file, &dir);
                if (result < 0 && result != CARD_RESULT_NOFILE) {
                    blocks = -1;
                    break;
                }
                if (result == CARD_RESULT_READY && memcmp(dir.gameName, id, 4) == 0 &&
                    memcmp(dir.company, id + 4, 2) == 0) {
                    blocks += dir.length;
                }
            }
            gCardSlots[slot].totalBlocks = (s16)blocks;
        }
    }
    CardThread_MarkConflicts();
    OSSendMessage(&sCardReplyQueue, (OSMessage)CARD_CMD_REFRESH, OS_MESSAGE_BLOCK);
}
