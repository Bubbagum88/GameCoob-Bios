// Memory Card screen (PAL 1.0 BS2, 0x813157B8..0x8131BF6C): the file browser
// for both slots, the Move / Copy / Erase menu, the Yes/No prompts and the
// format prompt. The card work itself runs on the card thread
// (memcard_card.c); this file posts commands to it and draws the result.
//

#include <dolphin/card.h>
#include "bs2/menu.h"
#include "bs2/res.h"
#include "bs2/scene.h"
#include "bs2/glh.h"
#include "bs2/mtx.h"
#include <math.h>
void qsort(void* base, unsigned long n, unsigned long size, int (*cmp)(const void*, const void*));

s32 Res_CardLayout(void);    /* 0x8130B884 */
s32 Res_CardStrings(void);   /* 0x8130B8B4 */
s32 Res_CardBarTex(void);    /* 0x8130BA74 */

/* Card thread commands (memcard_card.c) */
enum {
    CARD_CMD_MOUNT = 0,
    CARD_CMD_FORMAT = 1,
    CARD_CMD_COPY = 2,
    CARD_CMD_MOVE = 3,
    CARD_CMD_ERASE = 4
};

/* copy, move or erase */
#define CARD_OP_IS_XFER(k) ((u32)((k) - CARD_CMD_COPY) <= CARD_CMD_ERASE - CARD_CMD_COPY)


/* text of a string table entry */
#define STH_STRING(e) ((u8*)(e) + (e)->ofs)

/* Fade ramp of a slot header (0x20 bytes) */
typedef struct CardSlotFade {
    s16 a;                 /* 0x00 */
    s16 aMax;              /* 0x02 20 */
    s16 b;                 /* 0x04 */
    s16 bMax;              /* 0x06 30 */
    s16 s8;                /* 0x08 10 */
    s16 sA;                /* 0x0A 20 */
    u16 phase;             /* 0x0C */
    u8 padE;
    u8 alpha;              /* 0x0F 255 */
    s16 s10;               /* 0x10 40 */
    s16 s12;               /* 0x12 20 */
    s32 s14;               /* 0x14 */
    u16 lineH;             /* 0x18 height of one menu line */
    u16 h1;                /* 0x1A op menu box height */
    u16 h2;                /* 0x1C Yes/No box height */
    u16 h3;                /* 0x1E op menu height when fully open */
} CardSlotFade;

/* Icon and banner of one file, filled in by the card thread (0x40 bytes).
   The format and speed fields are volatile: every read reloads them. */
typedef struct CardIconInfo {
    u8 hasBanner;          /* 0x00 */
    u8 hasIcon;            /* 0x01 */
    u8 frames;             /* 0x02 */
    volatile u8 bannerFormat; /* 0x03 9: CI8, 5: RGB5A3 */
    s8 step;               /* 0x04 +1 / -1 while the icon animates */
    u8 bounce;             /* 0x05 CARD_STAT_ANIM_BOUNCE */
    u8 firstSpeed;         /* 0x06 */
    u8 lastSpeed;          /* 0x07 */
    volatile u8 format[8]; /* 0x08 per frame, 9: CI8, 5: RGB5A3 */
    volatile u16 speeds;   /* 0x10 CARDDir.iconSpeed */
    s16 duration;          /* 0x12 */
    u8* banner;            /* 0x14 */
    u8* bannerTlut;        /* 0x18 */
    u8* image[8];          /* 0x1C */
    u8* tlut;              /* 0x3C shared palette of the CI8 frames */
} CardIconInfo;

/* One entry of the sorted file list (8 bytes) */
typedef struct CardSortEntry {
    u32 file;              /* 0x0 index into the slot's CardFile table */
    u32 time;              /* 0x4 */
} CardSortEntry;

CardParams* gCardParams;
static u8 sCardSlot;           /* slot shown, 0 = A */
static s8 sCardCol[2];         /* cursor column 0..3, per slot */
static s8 sCardRow[2];         /* cursor row on screen 0..3 */
static s8 sCardTop[2];         /* first row shown */
static s8 sCardMaxTop[2];      /* last value of sCardTop */
static s32 sCardOpSlot;
static s32 sCardOpKind;
static s32 sCardOpError;
static s16 sCardUiState;
static s16 sCardUiNext;
static s16 sCardOp;
static s16 sCardOpNext;
static s16 sCardFmtA[2];
static s16 sCardFmtSel[2];
static s16 sCardYesNo;
static void* sCardGlh;
static void* sCardSth;
static TexHeader* sCardBarTex;
static u16 sCardNameX[3];
static u16 sCardNameY[3];
static u32 sCardMsgWait;
static s16 sCardBlink;
static s16 sCardBlinkDir;
static s16 sCardBlinkMax;
static s16 sCardArrow;
static s16 sCardArrowDir;
static s16 sCardArrowMax;

CardParams gCardParamsBuf;
static s16 sCardAnchorX[2][16];
static s16 sCardAnchorY[2][16];
static s16 sCardIconFrame[2][CARD_MAX_FILES];
static CardSortEntry sCardSort[2][CARD_MAX_FILES];
static Fader sCardMsg[0x2F];
static Fader sCardFadeA[2];
static Fader sCardFadeB[2];
static Fader sCardFadeC[2];
static CardSlotFade sCardSlotFade[2];
static Fader sCardFadeD[2];
static Fader sCardFadeE[2];
static Fader sCardFadeF[3];

void CardParams_SetDefaults(CardParams* p);
void Card_Init(BOOL first);
void fn_8131EEC4(BOOL first, s16 (*x)[16], s16 (*y)[16]);
void fn_8131A644(CardSlotFade* f);
BOOL fn_81321714(void);
BOOL fn_81320ABC(void);
void fn_8132056C(CardSortEntry (*sort)[CARD_MAX_FILES], s8* top);
void fn_81320094(u8 slot, u16 pos, u16 file, s32 cmd);
void fn_8132030C(u8 file);
void fn_813204B0(void);
void fn_81320920(s32 slot);
void fn_8131F8F8(void);

void CardThread_Format(u8 slot);
s32 CardThread_GetResult(void);
s32 CardThread_GetCmd(void);
s32 CardThread_GetDestResult(void);
u8 CardThread_GetDestFile(void);
void* CardThread_GetIcons(void);
void CardThread_Copy(u8 slot, s16 file);
void CardThread_Move(u8 slot, s16 file);
void CardThread_Erase(u8 slot, s16 file);
u8* CardThread_GetComment(u8 slot, s16 file);
void fn_813216F4(u8 slot, u8 alpha);
void fn_8130AB14(void);
void J3D_LoadTexture(s32 map, void* tex);
void Glh_MeasureString(GlhText* box, u8* str, u16* w, u16* h);
void fn_81308DB8(GlhText* box, u8* str);
void fn_813090A4(GlhText* box, u8* str);
void Font_SetColorU32(u32 top, u32 bottom);
void fn_81309554(GXColor* top, GXColor* bottom);
void fn_813095A0(GXColor* top, GXColor* bottom);
void Backdrop_DrawSpots(Mtx mv, u8 alpha, Vec* pos1, Vec* pos2, u8 alpha1, u8 alpha2, u8 spotAlpha);

void CardUi_StepSlotFader(CardSlotFade* f, u8 mode);
u32 CardDraw_LoadFileIcon(void);
void CardDraw_FileInfo(u8 alpha);
void CardDraw_SetScissor(s32 x, s32 y, s32 w, s32 h, s32 t, GXColor* c);

static inline void Card_ClearFrames(u8 slot) {
    int j;
    for (j = 0; j < CARD_MAX_FILES; j++) {
        sCardIconFrame[slot][j] = 0;
    }
}

/* 0x813157B8 */
void Card_InitParams(BOOL first) {
    if (first) {
        gCardParams = &gCardParamsBuf;
        CardParams_SetDefaults(gCardParams);
    }
    Card_Init(first);
}

/* 0x81315800 */
void CardParams_SetDefaults(CardParams* p) {
    p->tint[0] = -45;
    p->tint[1] = -40;
    p->tint[2] = 50;
    p->tint[3] = 255;
    p->cubeScale = 0.7f;
    p->rows = 5;
    p->cols = 8;
    p->colorTbl[0] = 0;
    p->colorTbl[2] = 150;
    p->colorTbl[4] = 255;
    p->colorTbl[6] = 0;
    p->colorTbl[8] = 50;
    p->colorTbl[10] = 200;
    p->colorTbl[12] = 0;
    p->colorTbl[14] = 30;
    p->colorTbl[16] = 90;
    p->colorTbl[18] = 0;
    p->colorTbl[20] = 20;
    p->colorTbl[22] = 60;
    p->colorTbl[1] = 0;
    p->colorTbl[3] = 150;
    p->colorTbl[5] = 255;
    p->colorTbl[7] = 0;
    p->colorTbl[9] = 50;
    p->colorTbl[11] = 200;
    p->colorTbl[13] = 0;
    p->colorTbl[15] = 30;
    p->colorTbl[17] = 90;
    p->colorTbl[19] = 0;
    p->colorTbl[21] = 20;
    p->colorTbl[23] = 60;
    p->iconZoom = 2.0f;
    p->iconZoomSel = 1.3f;
    p->white[0] = 255;
    p->white[1] = 255;
    p->white[2] = 255;
    p->cream[0] = 240;
    p->cream[1] = 230;
    p->cream[2] = 0;
    p->collapseFade = 70;
    p->collapseSpeed = 5;
    p->fadeOutSpeed = 13;
    p->f64_ = 10.0f;
    p->f68_ = 5.0f;
    p->s6C = 1;
    p->s6E = 0x80;
    p->f70 = 320.0f;
}

/* 0x81315934 */
CardParams* Card_GetParams(void) {
    return gCardParams;
}

/* 0x8131593C: read the icon slot positions ("ia00".."ib15") and the file
   name positions from the layout's anchors. */
void CardParams_SetColors(GlhHeader* glh) {
    GlhAnchor* tbl = (GlhAnchor*)((u8*)glh + glh->anchorOfs);
    GlhAnchor* a;
    u32 j;
    u32 i;

    for (i = 0; i < 32; i++) {
        u8 s = i / 16;
        u8 n = i % 16;
        u32 tag = 'ia00' + (s << 16) + ((n / 10) << 8) + n % 10;
        for (a = tbl, j = 0; j < ((GlhHeader*)sCardGlh)->anchorCount; a++, j++) {
            if (a->tag == tag) {
                sCardAnchorX[s][n] = a->y;
                sCardAnchorY[s][n] = a->x;
            }
        }
    }
    for (i = 0; i < 2; i++) {
        for (a = tbl, j = 0; j < ((GlhHeader*)sCardGlh)->anchorCount; a++, j++) {
            if ('fnma' + i == a->tag) {
                sCardNameX[i] = a->y;
                sCardNameY[i] = a->x;
            }
        }
    }
    for (a = tbl, j = 0; j < ((GlhHeader*)sCardGlh)->anchorCount; a++, j++) {
        if (a->tag == 'nmc1') {
            sCardNameX[2] = a->y;
            sCardNameY[2] = a->x;
        }
    }
}

/* 0x81315BA4 */
void Card_LoadLangRes(void) {
    sCardSth = ResGet(Res_CardStrings());
}

/* 0x81315BCC */
void Card_Init(BOOL first) {
    u32 i;
    int j;

    if (first) {
        sCardGlh = ResGet(Res_CardLayout());
        sCardSth = ResGet(Res_CardStrings());
        CardParams_SetColors(sCardGlh);
        sCardBarTex = ResGet(Res_CardBarTex());
    }
    fn_8131EEC4(first, sCardAnchorX, sCardAnchorY);
    Card_ClearFrames(0);
    Card_ClearFrames(1);
    sCardSlot = 0;
    sCardCol[0] = 0;
    sCardCol[1] = 0;
    sCardRow[0] = 0;
    sCardRow[1] = 0;
    sCardTop[0] = 0;
    sCardTop[1] = 0;
    sCardUiState = 1;
    sCardOp = 2;
    sCardUiNext = 1;
    sCardOpNext = 2;
    sCardOpSlot = 0;
    sCardOpKind = 0;
    sCardOpError = 0;
    sCardYesNo = 1;
    for (i = 0; i < 0x2F; i++) {
        Fader_Init(&sCardMsg[i]);
        sCardMsg[i].id = i;
    }
    for (i = 0; i < 2; i++) {
        Fader_Init(&sCardFadeA[i]);
        Fader_Init(&sCardFadeB[i]);
        Fader_Init(&sCardFadeC[i]);
        Fader_Init(&sCardFadeD[i]);
        sCardFadeD[i].id = 'arau' + (i << 8);
        Fader_Init(&sCardFadeE[i]);
        sCardFadeE[i].id = 'arad' + (i << 8);
        fn_8131A644(&sCardSlotFade[i]);
        sCardFmtA[i] = 1;
        sCardFmtSel[i] = sCardFmtA[i];
    }
    sCardBlinkDir = 1;
    sCardMsgWait = 0;
    sCardBlink = 0;
    sCardBlinkMax = 15;
    sCardArrow = 0;
    sCardArrowDir = 1;
    sCardArrowMax = 30;
    Fader_Init(&sCardFadeF[0]);
    sCardFadeF[0].length = 10;
    Fader_Init(&sCardFadeF[1]);
    sCardFadeF[1].length = 10;
    Fader_Init(&sCardFadeF[2]);
    sCardFadeF[2].length = 10;
}

static inline BOOL Card_OtherReady(s32* state) {
    u8 other = sCardSlot ^ 1;
    CardSlot* slots = CardThread_GetSlots();

    if (state != NULL) {
        *state = slots[other].state;
    }
    if (slots[other].state == CARD_SLOT_READY) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* 0x81315FDC: can file `file` of `slot` be copied to the other slot? */
BOOL Card_CheckCopy(u8 slot, s32 file, s32* errOut) {
    s32 err;
    BOOL ok = FALSE;
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    CardSlot* slots = CardThread_GetSlots();
    s32 state;

    if (Card_OtherReady(NULL) && files[slot][file].canCopy &&
        slots[slot].sectorSize == slots[slot ^ 1].sectorSize &&
        slots[slot ^ 1].freeBlocks >= files[slot][file].blocks && slots[slot ^ 1].freeFiles != 0 &&
        files[slot][file].opened == 0) {
        err = CARD_UI_OK;
        ok = TRUE;
    } else if (!Card_OtherReady(&state)) {
        if (state == CARD_SLOT_EMPTY) {
            err = CARD_UI_NO_CARD;
        } else if (state == CARD_SLOT_WRONG || state == CARD_SLOT_NEED_FORMAT_ASKED ||
                   state == CARD_SLOT_ENCODING_ASKED) {
            err = CARD_UI_ENCODING;
        } else {
            err = CARD_UI_BROKEN;
        }
    } else if (!files[slot][file].canCopy) {
        err = CARD_UI_NOT_ALLOWED;
    } else if (slots[slot].sectorSize != slots[slot ^ 1].sectorSize) {
        err = CARD_UI_DIFFERENT_SIZE;
    } else if (slots[slot ^ 1].freeBlocks < files[slot][file].blocks || slots[slot ^ 1].freeFiles == 0) {
        err = CARD_UI_NO_SPACE;
    } else if (files[slot][file].opened != 0) {
        err = CARD_UI_SAME_FILE;
    }
    if (errOut != NULL) {
        *errOut = err;
    }
    return ok;
}

/* 0x813161F8: same for moving the file. */
BOOL Card_CheckMove(u8 slot, s32 file, s32* errOut) {
    s32 err;
    BOOL ok = FALSE;
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    CardSlot* slots = CardThread_GetSlots();
    s32 state;

    if (Card_OtherReady(NULL) && files[slot][file].canMove &&
        slots[slot].sectorSize == slots[slot ^ 1].sectorSize &&
        slots[slot ^ 1].freeBlocks >= files[slot][file].blocks && slots[slot ^ 1].freeFiles != 0 &&
        files[slot][file].opened == 0) {
        err = CARD_UI_OK;
        ok = TRUE;
    } else if (!Card_OtherReady(&state)) {
        if (state == CARD_SLOT_EMPTY) {
            err = CARD_UI_NO_CARD;
        } else if (state == CARD_SLOT_WRONG || state == CARD_SLOT_NEED_FORMAT_ASKED ||
                   state == CARD_SLOT_ENCODING_ASKED) {
            err = CARD_UI_ENCODING;
        } else {
            err = CARD_UI_BROKEN;
        }
    } else if (!files[slot][file].canMove) {
        err = CARD_UI_NOT_ALLOWED;
    } else if (slots[slot].sectorSize != slots[slot ^ 1].sectorSize) {
        err = CARD_UI_DIFFERENT_SIZE;
    } else if (slots[slot ^ 1].freeBlocks < files[slot][file].blocks || slots[slot ^ 1].freeFiles == 0) {
        err = CARD_UI_NO_SPACE;
    } else if (files[slot][file].opened != 0) {
        err = CARD_UI_SAME_FILE;
    }
    if (errOut != NULL) {
        *errOut = err;
    }
    return ok;
}

/* 0x81316414: back to browsing. A slot that needs formatting asks first;
   if the current slot has no usable card, switch to the other one. */
void CardUi_Browse_Enter(BOOL force) {
    CardSlot* slots = CardThread_GetSlots();
    volatile s16* st;
    CardSlot* o;

    if (sCardUiState == 5 && !force) {
        return;
    }
    o = &slots[sCardSlot ^ 1];
    st = &o->state;
    if (o->state == CARD_SLOT_ENCODING || *st == CARD_SLOT_NEED_FORMAT) {
        sCardUiState = sCardUiNext = 4;
        sCardSlot ^= 1;
    } else {
        sCardUiState = sCardUiNext = 1;
        if (slots[sCardSlot].state != CARD_SLOT_READY && Card_OtherReady(NULL)) {
            sCardSlot ^= 1;
        }
    }
}

static inline BOOL Card_CalcMaxTop(u8 slot) {
    s8* maxTop = &sCardMaxTop[slot];
    CardSlot* s = &CardThread_GetSlots()[slot];
    volatile u16* n;
    s8 m;

    if (s->state != CARD_SLOT_READY) {
        return FALSE;
    }
    n = &s->freeFiles;
    m = (CARD_MAX_FILES - *n) / 4 - 3;
    if ((CARD_MAX_FILES - *n) % 4 == 0) {
        m--;
    }
    if (m < 0) {
        m = 0;
    }
    *maxTop = m;
    return TRUE;
}

/* 0x813164FC: UI state 1, moving over the file grid. */
s32 CardUi_Browse(void) {
    s32 next = MENU_CARD;
    CardSlot* slots = CardThread_GetSlots();
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    MenuWork* menu;
    volatile CardSlot* s;
    int i;

    if (!fn_81321714()) {
        return MENU_CARD;
    }
    if (slots[sCardSlot].state == CARD_SLOT_READY && Card_CalcMaxTop(sCardSlot)) {
        if (gMenuPad.dirsNew & DIR_UP) {
            sCardRow[sCardSlot]--;
            if (sCardRow[sCardSlot] < 0) {
                sCardRow[sCardSlot] = 0;
                if (sCardTop[sCardSlot] > 0) {
                    sCardTop[sCardSlot]--;
                    fn_8132056C(sCardSort, sCardTop);
                    Sound_Play(SE_CURSOR);
                } else if (!Pad_IsRepeat()) {
                    Sound_Play(SE_BUZZER);
                }
            } else {
                Sound_Play(SE_CURSOR);
            }
        }
        if (gMenuPad.dirsNew & DIR_DOWN) {
            sCardRow[sCardSlot]++;
            if (sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4 >= CARD_MAX_FILES) {
                if (!Pad_IsRepeat()) {
                    Sound_Play(SE_BUZZER);
                }
                sCardRow[sCardSlot]--;
            } else if (sCardRow[sCardSlot] > 3) {
                sCardRow[sCardSlot] = 3;
                if (sCardTop[sCardSlot] < sCardMaxTop[sCardSlot]) {
                    sCardTop[sCardSlot]++;
                    fn_8132056C(sCardSort, sCardTop);
                    Sound_Play(SE_CURSOR);
                } else if (!Pad_IsRepeat()) {
                    Sound_Play(SE_BUZZER);
                }
            } else {
                Sound_Play(SE_CURSOR);
            }
        }
        if (gMenuPad.dirsNew & DIR_LEFT) {
            if (sCardSlot == 0) {
                if (sCardCol[sCardSlot] > 0) {
                    Sound_Play(SE_CURSOR);
                    sCardCol[sCardSlot]--;
                } else if (!Pad_IsRepeat()) {
                    Sound_Play(SE_BUZZER);
                }
            } else if (sCardSlot == 1) {
                sCardCol[sCardSlot]--;
                if (sCardCol[sCardSlot] < 0) {
                    sCardCol[sCardSlot] = 0;
                    if (slots[sCardSlot ^ 1].state == CARD_SLOT_READY) {
                        if (sCardCol[sCardSlot ^ 1] + (sCardRow[sCardSlot] + sCardTop[sCardSlot ^ 1]) * 4 >= CARD_MAX_FILES) {
                            if (!Pad_IsRepeat()) {
                                Sound_Play(SE_BUZZER);
                            }
                        } else {
                            sCardSlot ^= 1;
                            sCardRow[sCardSlot] = sCardRow[sCardSlot ^ 1];
                            sCardCol[sCardSlot] = 3;
                            Sound_Play(SE_CURSOR);
                        }
                    } else if (!Pad_IsRepeat()) {
                        Sound_Play(SE_BUZZER);
                    }
                } else {
                    Sound_Play(SE_CURSOR);
                }
            }
        }
        if (gMenuPad.dirsNew & DIR_RIGHT) {
            if (sCardSlot == 1) {
                if (sCardCol[sCardSlot] < 3) {
                    sCardCol[sCardSlot]++;
                    if (sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4 >= CARD_MAX_FILES) {
                        sCardCol[sCardSlot]--;
                        if (!Pad_IsRepeat()) {
                            Sound_Play(SE_BUZZER);
                        }
                    } else {
                        Sound_Play(SE_CURSOR);
                    }
                } else if (!Pad_IsRepeat()) {
                    Sound_Play(SE_BUZZER);
                }
            } else if (sCardSlot == 0) {
                sCardCol[sCardSlot]++;
                if (sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4 >= CARD_MAX_FILES) {
                    sCardCol[sCardSlot]--;
                    if (!Pad_IsRepeat()) {
                        Sound_Play(SE_BUZZER);
                    }
                } else if (sCardCol[sCardSlot] > 3) {
                    sCardCol[sCardSlot] = 3;
                    if (slots[sCardSlot ^ 1].state == CARD_SLOT_READY) {
                        sCardSlot ^= 1;
                        sCardRow[sCardSlot] = sCardRow[sCardSlot ^ 1];
                        sCardCol[sCardSlot] = 0;
                        Sound_Play(SE_CURSOR);
                    } else if (!Pad_IsRepeat()) {
                        Sound_Play(SE_BUZZER);
                    }
                } else {
                    Sound_Play(SE_CURSOR);
                }
            }
        }
        if (fn_81320ABC() && (gMenuPad.buttonDown & PAD_A) &&
            files[sCardSlot][sCardSort[sCardSlot][sCardCol[sCardSlot] + sCardRow[sCardSlot] * 4 + sCardTop[sCardSlot] * 4].file].used) {
            sCardUiNext = 2;
            Sound_Play(SE_DECIDE);
        }
    }
    if ((gMenuPad.buttonDown & PAD_B) && ((menu = Menu_Get())->zoom == 0x7FFF || menu->inputAlways) && fn_81320ABC()) {
        Sound_Play(SE_EXIT);
        s = CardThread_GetSlots();
        for (i = 0; i < 2; i++, s++) {
            if (!s->dirty) {
                if (s->state == CARD_SLOT_NEED_FORMAT_ASKED) {
                    s->state = CARD_SLOT_NEED_FORMAT;
                    s->dirty = TRUE;
                } else if (s->state == CARD_SLOT_ENCODING_ASKED) {
                    s->state = CARD_SLOT_ENCODING;
                    s->dirty = TRUE;
                }
            }
        }
        next = MENU_CARD_FACE;
    }
    return next;
}

static inline void CardUi_TryCopy(s16 file) {
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    s32 err;

    if (Card_CheckCopy(sCardSlot, file, &err)) {
        Sound_Play(SE_DECIDE);
        sCardUiNext = 3;
    } else {
        Sound_Play(SE_BUZZER);
        sCardOpSlot = sCardSlot;
        sCardOpKind = 5;
        sCardOpError = err;
        sCardUiNext = 5;
    }
}

static inline void CardUi_TryMove(s16 file) {
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    s32 err;

    if (Card_CheckMove(sCardSlot, file, &err)) {
        Sound_Play(SE_DECIDE);
        sCardUiNext = 3;
    } else {
        Sound_Play(SE_BUZZER);
        sCardOpSlot = sCardSlot;
        sCardOpKind = 6;
        sCardOpError = err;
        sCardUiNext = 5;
    }
}

/* 0x81316BAC: UI state 2, the Move / Copy / Erase menu. */
s32 CardUi_OpMenu(void) {
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    s32 file;

    if (gMenuPad.dirsNew & DIR_UP) {
        Sound_Play(SE_CURSOR_H);
        sCardOpNext--;
        if (sCardOpNext < 0) {
            sCardOpNext = 2;
        }
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        Sound_Play(SE_CURSOR_H);
        sCardOpNext++;
        if (sCardOpNext >= 3) {
            sCardOpNext = 0;
        }
    }
    if ((gMenuPad.buttonDown & PAD_A) && sCardOpNext == sCardOp) {
        file = sCardSort[sCardSlot][sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4].file;
        sCardOpSlot = sCardSlot;
        sCardYesNo = 1;
        switch (sCardOp) {
        case 1:
            CardUi_TryCopy(file);
            break;
        case 0:
            CardUi_TryMove(file);
            break;
        case 2:
            Sound_Play(SE_DECIDE);
            sCardUiNext = 3;
            break;
        }
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        CardUi_Browse_Enter(FALSE);
    }
    return MENU_CARD;
}

/* 0x81316DDC: UI state 3, "Are you sure?" for the chosen operation. */
s32 CardUi_Confirm(void) {
    CardSlot* slots = CardThread_GetSlots();
    s32 file;

    if (gMenuPad.dirsNew & DIR_UP) {
        if (sCardYesNo != 0) {
            Sound_Play(SE_CHANGE);
        }
        sCardYesNo = 0;
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        if (sCardYesNo != 1) {
            Sound_Play(SE_CHANGE);
        }
        sCardYesNo = 1;
    }
    if (gMenuPad.buttonDown & PAD_A) {
        if (sCardYesNo == 0) {
            file = sCardSort[sCardSlot][sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4].file;
            sCardOpSlot = sCardSlot;
            switch (sCardOp) {
            case 1:
                sCardOpSlot = sCardSlot;
                sCardOpKind = CARD_CMD_COPY;
                if (Card_OtherReady(NULL)) {
                    Sound_Play(SE_DECIDE);
                    fn_81320094(sCardSlot, sCardCol[sCardSlot] + sCardRow[sCardSlot] * 4, file, CARD_CMD_COPY);
                    CardThread_Copy(sCardSlot, file);
                    sCardOpError = CARD_UI_OK;
                    sCardMsgWait = 0;
                } else {
                    sCardOpError = CARD_UI_NO_CARD;
                }
                sCardUiNext = 5;
                break;
            case 0:
                sCardOpSlot = sCardSlot;
                sCardOpKind = CARD_CMD_MOVE;
                if (Card_OtherReady(NULL)) {
                    Sound_Play(SE_DECIDE);
                    fn_81320094(sCardSlot, sCardCol[sCardSlot] + sCardRow[sCardSlot] * 4, file, CARD_CMD_MOVE);
                    CardThread_Move(sCardSlot, file);
                    sCardOpError = CARD_UI_OK;
                    sCardMsgWait = 0;
                } else {
                    sCardOpError = CARD_UI_NO_CARD;
                }
                sCardUiNext = 5;
                break;
            case 2:
                sCardOpSlot = sCardSlot;
                sCardOpKind = CARD_CMD_ERASE;
                Sound_Play(SE_DECIDE);
                fn_81320094(sCardSlot, sCardCol[sCardSlot] + sCardRow[sCardSlot] * 4, file, CARD_CMD_ERASE);
                CardThread_Erase(sCardSlot, file);
                sCardUiNext = 5;
                sCardMsgWait = 0;
                sCardOpError = CARD_UI_OK;
                break;
            }
        } else {
            Sound_Play(SE_CANCEL);
            sCardUiNext = 2;
        }
    }
    if (gMenuPad.buttonDown & PAD_B) {
        Sound_Play(SE_CANCEL);
        sCardUiNext = 2;
    }
    if (!Card_OtherReady(NULL)) {
        switch (sCardOp) {
        case 0:
        case 1:
            sCardUiNext = 2;
            break;
        }
    }
    return MENU_CARD;
}

static inline BOOL CardUi_FormatShown(void) {
    s16 a;
    s16 b;

    if (fn_81320ABC()) {
        Fader_GetAlpha(&sCardMsg[39 + sCardSlot], &a, NULL);
        Fader_GetAlpha(&sCardMsg[41 + sCardSlot], &b, NULL);
        if (a > 0xAC || b > 0xAC) {
            return TRUE;
        }
    }
    return FALSE;
}

static inline void CardUi_FormatCancel(void) {
    CardSlot* p;
    s16* st;

    Sound_Play(SE_CANCEL);
    p = &CardThread_GetSlots()[sCardSlot];
    st = &p->state;
    if (*st == CARD_SLOT_NEED_FORMAT) {
        *st = CARD_SLOT_NEED_FORMAT_ASKED;
    } else {
        *st = CARD_SLOT_ENCODING_ASKED;
    }
    sCardFmtSel[sCardSlot] = 1;
    CardUi_Browse_Enter(FALSE);
}

/* 0x81317150: UI state 4, "format this card?" asked twice. */
s32 CardUi_Format(void) {
    CardSlot* slots = CardThread_GetSlots();
    s16 v;

    if (!CardUi_FormatShown()) {
        return MENU_CARD;
    }
    if (gMenuPad.dirsNew & DIR_UP) {
        v = sCardFmtSel[sCardSlot] & 0xFE;
        if (v != sCardFmtSel[sCardSlot]) {
            Sound_Play(SE_CURSOR);
        }
        sCardFmtSel[sCardSlot] = v;
    }
    if (gMenuPad.dirsNew & DIR_DOWN) {
        v = sCardFmtSel[sCardSlot] | 1;
        if (v != sCardFmtSel[sCardSlot]) {
            Sound_Play(SE_CURSOR);
        }
        sCardFmtSel[sCardSlot] = v;
    }
    if (gMenuPad.buttonDown & PAD_A) {
        switch (sCardFmtA[sCardSlot] & 3) {
        case 0:
            sCardFmtSel[sCardSlot] = 3;
            Sound_Play(SE_DECIDE);
            break;
        case 2:
            Sound_Play(SE_DECIDE);
            CardThread_Format(sCardSlot);
            sCardUiNext = 5;
            sCardOpSlot = sCardSlot;
            sCardOpKind = CARD_CMD_FORMAT;
            sCardOpError = CARD_UI_OK;
            break;
        case 1:
        case 3:
            CardUi_FormatCancel();
            break;
        }
    }
    if (gMenuPad.buttonDown & PAD_B) {
        CardUi_FormatCancel();
    }
    return MENU_CARD;
}

/* 0x813173EC: UI state 5, a message; plays the result sound once. */
s32 CardUi_Message(void) {
    CardSlot* slots = CardThread_GetSlots();

    if (!fn_81320ABC()) {
        return MENU_CARD;
    }
    if (CardThread_GetResult() != CARD_UI_OK) {
        if (sCardMsgWait != 0) {
            switch (sCardOpKind) {
            case 2:
            case 5:
                if (sCardOpError == 0) {
                    Sound_Play(SE_CARD_16);
                }
                break;
            case 4:
            case 7:
                if (sCardOpError == 0) {
                    Sound_Play(SE_CARD_15);
                }
                break;
            }
        }
        sCardMsgWait = 0;
        if (gMenuPad.buttonDown & PAD_A) {
            sCardOpKind = 0;
            sCardYesNo = 1;
            sCardOpError = 0;
            CardUi_Browse_Enter(TRUE);
            Sound_Play(SE_CANCEL);
        }
        if (gMenuPad.buttonDown & PAD_B) {
            sCardYesNo = 1;
            sCardOpKind = 0;
            sCardOpError = 0;
            CardUi_Browse_Enter(TRUE);
            Sound_Play(SE_CANCEL);
        }
    }
    return MENU_CARD;
}

/* 0x81317514: qsort order: oldest first, empty entries last. */
int CardFile_Compare(const void* a, const void* b) {
    const CardSortEntry* x = a;
    const CardSortEntry* y = b;

    if (x->time > y->time) {
        return 1;
    } else if (x->time < y->time) {
        return -1;
    }
    if (x->file > y->file) {
        return 1;
    } else if (x->file < y->file) {
        return -1;
    }
    return 0;
}

static inline void Card_SetMaxTop(u8 slot) {
    CardSlot* s = &CardThread_GetSlots()[(u8)slot];
    volatile u16* n;
    s8 m;

    if (s->state == CARD_SLOT_READY) {
        n = &s->freeFiles;
        m = (CARD_MAX_FILES - *n) / 4 - 3;
        if ((CARD_MAX_FILES - *n) % 4 == 0) {
            m--;
        }
        if (m < 0) {
            m = 0;
        }
        sCardMaxTop[slot] = m;
    }
}

static inline void Card_BuildSort(CardFile (*files)[CARD_MAX_FILES], u8 slot) {
    u32 i;

    for (i = 0; i < CARD_MAX_FILES; i++) {
        if (files[slot][i].used) {
            sCardSort[slot][i].file = i;
            sCardSort[slot][i].time = files[slot][i].time;
        } else {
            sCardSort[slot][i].file = i;
            sCardSort[slot][i].time = (s32)(0xFF - i) - 0x100;
        }
    }
    qsort(sCardSort[slot], CARD_MAX_FILES, sizeof(CardSortEntry), CardFile_Compare);
}

/* 0x81317564: rebuild the display order of a slot's files. */
void Card_SortFiles(u8 slot) {
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    u32 i;

    Card_BuildSort(files, slot);
    Card_SetMaxTop(slot);
    if (sCardMaxTop[slot] < sCardTop[slot]) {
        sCardTop[slot] = sCardMaxTop[slot];
    }
}

static inline BOOL Card_UpdateMaxTop(u8 slot) {
    CardSlot* s = &CardThread_GetSlots()[slot];
    volatile u16* n;
    s8 m;

    if (s->state != CARD_SLOT_READY) {
        return FALSE;
    }
    n = &s->freeFiles;
    m = (CARD_MAX_FILES - *n) / 4 - 3;
    if ((CARD_MAX_FILES - *n) % 4 == 0) {
        m--;
    }
    if (m < 0) {
        m = 0;
    }
    sCardMaxTop[slot] = m;
    return TRUE;
}

/* 0x813176AC: the card thread re-read `slot`. */
void CardUi_OnSlotRefresh(u8 slot) {
    CardSlot* s = &CardThread_GetSlots()[slot];

    s->dirty = FALSE;
    if (Card_UpdateMaxTop(slot)) {
        Card_SortFiles(slot);
        if (CARD_OP_IS_XFER(sCardOpKind)) {
            if (sCardOpError == CARD_UI_OK) {
                sCardOpError = CardThread_GetDestResult();
            }
            if (sCardOpError == 0) {
                fn_8132030C(CardThread_GetDestFile());
                switch (sCardOpKind) {
                case 3:
                    if (sCardOpSlot == 0) {
                        Sound_Play(SE_CARD_20);
                    } else {
                        Sound_Play(SE_CARD_21);
                    }
                    sCardOpKind = 6;
                    break;
                case 4:
                    sCardOpKind = 7;
                    break;
                case 2:
                    Sound_Play(SE_CARD_19);
                    sCardOpKind = 5;
                    break;
                }
            } else {
                fn_813204B0();
            }
        }
        fn_8132056C(sCardSort, sCardTop);
    }
}

/* 0x8131783C */
void CardUi_Reset(void) {
    fn_8132056C(sCardSort, sCardTop);
}

/* 0x81317868: follow a running copy / move / erase / format. */
void CardUi_PollOpResult(void) {
    s32 kind = sCardOpKind;

    if (CARD_OP_IS_XFER(kind)) {
        if (sCardOpError == CARD_UI_OK) {
            if (sCardOpSlot == 0) {
                switch (sCardOpKind) {
                case 2:
                case 3:
                    if (sCardMsgWait == 10) {
                        Sound_Play(SE_CARD_17);
                    }
                    break;
                }
            } else {
                switch (sCardOpKind) {
                case 2:
                case 3:
                    if (sCardMsgWait == 10) {
                        Sound_Play(SE_CARD_18);
                    }
                    break;
                }
            }
            sCardMsgWait++;
            if (sCardMsgWait > 20) {
                sCardMsgWait = 20;
            }
            sCardOpError = CardThread_GetDestResult();
        }
        if (sCardOpError != CARD_UI_OK && sCardOpError != 0) {
            switch (sCardOpKind) {
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
            Sound_Play(SE_BUZZER);
        }
    } else if (kind == CARD_CMD_FORMAT) {
        if (sCardOpError == CARD_UI_OK) {
            sCardOpError = CardThread_GetDestResult();
        }
        if (sCardOpError != CARD_UI_OK) {
            if (sCardOpError == 0) {
                Sound_Play(SE_CARD_16);
            } else {
                Sound_Play(SE_BUZZER);
            }
            sCardOpKind = 8;
        }
    }
}

/* 0x813179E0: react to a slot's status change. */
void CardUi_PollSlot(u8 slot) {
    CardSlot* slots = CardThread_GetSlots();
    CardSlot* s = &slots[slot];
    volatile s16* st;

    if (s->dirty == 1) {
        st = &s->state;
        if (*st == CARD_SLOT_READY) {
            CardUi_OnSlotRefresh(slot);
        } else if (*st == CARD_SLOT_ENCODING || *st == CARD_SLOT_NEED_FORMAT) {
            CardSlot* s2 = CardThread_GetSlots();
            sCardSlot = slot;
            sCardUiState = sCardUiNext = 4;
            sCardFmtSel[slot] = 1;
            s2[slot].dirty = FALSE;
        }
        if (sCardOpError == 0 && CardThread_GetCmd() == 9 && sCardUiState == 5) {
            CardUi_Browse_Enter(TRUE);
        }
    }
    CardUi_PollOpResult();
    if (slots[slot].state != CARD_SLOT_READY) {
        fn_81320920(slot);
    }
    s = &slots[sCardSlot];
    st = &s->state;
    if (s->state == CARD_SLOT_EMPTY) {
        CardUi_Browse_Enter(FALSE);
        if (Card_OtherReady(NULL)) {
            sCardSlot ^= 1;
        }
    } else if (*st != CARD_SLOT_READY && *st != CARD_SLOT_NEED_FORMAT && *st != CARD_SLOT_ENCODING) {
        CardUi_Browse_Enter(FALSE);
    }
}

/* 0x81317B80: UI update, called by Card_Update. */
s32 CardUi_Update(void) {
    s32 next = MENU_CARD;
    Scene* scene = Scene_Get();

    sCardUiState = sCardUiNext;
    sCardOp = sCardOpNext;
    sCardFmtA[0] = sCardFmtSel[0];
    sCardFmtA[1] = sCardFmtSel[1];
    CardUi_PollSlot(0);
    CardUi_PollSlot(1);
    fn_8131F8F8();
    if (!SCENE_FATAL_ERROR(scene)) {
        switch (sCardUiState) {
        case 1:
            next = CardUi_Browse();
            break;
        case 2:
            next = CardUi_OpMenu();
            break;
        case 3:
            next = CardUi_Confirm();
            break;
        case 4:
            next = CardUi_Format();
            break;
        case 5:
            next = CardUi_Message();
            break;
        }
    }
    return next;
}

/* 0x81317C6C: grey out Copy / Move when they are not possible. */
void CardUi_UpdateOpFaders(u8 slot) {
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    s32 file = sCardSort[slot][sCardCol[slot] + (sCardRow[slot] + sCardTop[slot]) * 4].file;

    if (Card_CheckCopy(slot, file, NULL)) {
        Fader_Step(&sCardFadeB[slot], FADE_IN);
    } else {
        Fader_Step(&sCardFadeB[slot], FADE_OUT);
    }
    if (Card_CheckMove(slot, file, NULL)) {
        Fader_Step(&sCardFadeC[slot], FADE_IN);
    } else {
        Fader_Step(&sCardFadeC[slot], FADE_OUT);
    }
}

/* 0x81317D88: the two format prompts of a slot. */
void CardUi_UpdateFormatMsg(CardSlot* slots, u8 slot, BOOL show) {
    CardSlot* s = &slots[slot];
    volatile s16* st = &s->state;

    if (s->state == CARD_SLOT_NEED_FORMAT || *st == CARD_SLOT_ENCODING) {
        if (sCardUiState == 4 || show) {
            if (!(sCardFmtA[slot] & 2)) {
                if (slot == sCardSlot) {
                    Fader_Step(&sCardMsg[39 + slot], FADE_IN);
                } else {
                    Fader_Step(&sCardMsg[39 + slot], FADE_HALF);
                }
                Fader_Step(&sCardMsg[41 + slot], FADE_OUT);
            } else {
                Fader_Step(&sCardMsg[39 + slot], FADE_OUT);
                if (slot == sCardSlot) {
                    Fader_Step(&sCardMsg[41 + slot], FADE_IN);
                } else {
                    Fader_Step(&sCardMsg[41 + slot], FADE_HALF);
                }
            }
        } else {
            Fader_Step(&sCardMsg[39 + slot], FADE_OUT);
            Fader_Step(&sCardMsg[41 + slot], FADE_OUT);
        }
    } else {
        Fader_Step(&sCardMsg[39 + slot], FADE_OUT);
        Fader_Step(&sCardMsg[41 + slot], FADE_OUT);
    }
}

static inline void CardUi_UpdateSlotMsgs(CardSlot* slots, volatile CardSlot* s, u8 i, BOOL show) {
    if (s->state == CARD_SLOT_EMPTY) {
        Fader_Step(&sCardMsg[33 + i], FADE_IN);
        Card_ClearFrames(i);
    } else {
        Fader_Step(&sCardMsg[33 + i], FADE_OUT);
    }
    if (s->state == CARD_SLOT_WRONG || s->state == 10 || s->state == CARD_SLOT_NEED_FORMAT_ASKED ||
        s->state == CARD_SLOT_ENCODING_ASKED) {
        Fader_Step(&sCardMsg[35 + i], FADE_IN);
        Card_ClearFrames(i);
    } else {
        Fader_Step(&sCardMsg[35 + i], FADE_OUT);
    }
    CardUi_UpdateFormatMsg(slots, i, show);
}

/* 0x81317F40: per-frame fades of the slot messages and arrows. */
void CardUi_UpdateFaders(void) {
    CardSlot* s;
    CardSlot* slots = CardThread_GetSlots();
    u8 i;

    for (i = 0, s = slots; i < 2; i++, s++) {
        if (s->state == CARD_SLOT_READY) {
            Fader_Step(&sCardFadeA[i], FADE_IN);
            CardUi_UpdateOpFaders(0);
            CardUi_UpdateOpFaders(1);
        } else {
            Fader_Step(&sCardFadeA[i], FADE_OUT);
        }
        if (sCardUiState != 5) {
            CardUi_UpdateSlotMsgs(slots, s, i, FALSE);
        } else if ((sCardOpKind != CARD_CMD_FORMAT && sCardOpKind != 8) || sCardOpSlot == i) {
            Fader_Step(&sCardMsg[33 + i], FADE_OUT);
            Fader_Step(&sCardMsg[35 + i], FADE_OUT);
            Fader_Step(&sCardMsg[39 + i], FADE_OUT);
            Fader_Step(&sCardMsg[41 + i], FADE_OUT);
        } else if (sCardOpSlot != i) {
            CardUi_UpdateSlotMsgs(slots, s, i, TRUE);
        }
        if (sCardUiState == 1) {
            if (sCardTop[i] > 0) {
                Fader_Step(&sCardFadeD[i], FADE_IN);
            } else {
                Fader_Step(&sCardFadeD[i], FADE_OUT);
            }
            if (sCardMaxTop[i] > sCardTop[i]) {
                Fader_Step(&sCardFadeE[i], FADE_IN);
            } else {
                Fader_Step(&sCardFadeE[i], FADE_OUT);
            }
        } else {
            Fader_Step(&sCardFadeD[i], FADE_OUT);
            Fader_Step(&sCardFadeE[i], FADE_OUT);
        }
    }
}

/* 0x813186F4: advance the icon animation of the 16 files on screen. */
void CardUi_UpdateIconAnim(void) {
    CardIconInfo(*icons)[CARD_MAX_FILES] = CardThread_GetIcons();
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    u32 file;
    CardIconInfo* icon;
    s16* frame;
    s8* step;
    u8 slot;
    int j;
    s16* dur;

    for (slot = 0; slot < 2; slot++) {
        for (j = 0; j < 16; j++) {
            file = sCardSort[slot][j + sCardTop[slot] * 4].file;
            if (files[slot][file].used) {
                icon = &icons[slot][file];
                frame = &sCardIconFrame[slot][file];
                step = &icon->step;
                dur = &icon->duration;
                *frame += *step;
                if (*frame >= *dur) {
                    if (icon->bounce == CARD_STAT_ANIM_BOUNCE) {
                        *frame = *dur - icon->lastSpeed - 1;
                        *step = -1;
                    } else {
                        *frame = 0;
                        *step = 1;
                    }
                }
                if (*frame < 0) {
                    if (icon->bounce == CARD_STAT_ANIM_BOUNCE) {
                        *frame = icon->firstSpeed;
                    } else {
                        *frame = 0;
                    }
                    *step = 1;
                }
            }
        }
    }
}

/* 0x81318858: result text of a copy. */
void CardUi_CopyResultMsg(BOOL show) {
    if (show) {
        if (sCardOpError == 0) {
            Fader_Step(&sCardMsg[19], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[19], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_NOT_ALLOWED) {
            Fader_Step(&sCardMsg[20], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[20], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_NO_CARD) {
            Fader_Step(&sCardMsg[22 - sCardOpSlot], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[22 - sCardOpSlot], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_ENCODING) {
            Fader_Step(&sCardMsg[24 - sCardOpSlot], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[24 - sCardOpSlot], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_DIFFERENT_SIZE) {
            Fader_Step(&sCardMsg[27], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[27], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_SAME_FILE) {
            Fader_Step(&sCardMsg[28], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[28], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_NO_SPACE) {
            Fader_Step(&sCardMsg[29], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[29], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_BROKEN) {
            Fader_Step(&sCardMsg[30], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[30], FADE_OUT);
        }
    } else {
        Fader_Step(&sCardMsg[19], FADE_OUT);
        Fader_Step(&sCardMsg[20], FADE_OUT);
        Fader_Step(&sCardMsg[21], FADE_OUT);
        Fader_Step(&sCardMsg[22], FADE_OUT);
        Fader_Step(&sCardMsg[23], FADE_OUT);
        Fader_Step(&sCardMsg[24], FADE_OUT);
        Fader_Step(&sCardMsg[27], FADE_OUT);
        Fader_Step(&sCardMsg[28], FADE_OUT);
        Fader_Step(&sCardMsg[29], FADE_OUT);
        Fader_Step(&sCardMsg[30], FADE_OUT);
    }
}

/* 0x81318A88: result text of a move. */
void CardUi_MoveResultMsg(BOOL show) {
    if (show) {
        if (sCardOpError == 0) {
            Fader_Step(&sCardMsg[7], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[7], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_NOT_ALLOWED) {
            Fader_Step(&sCardMsg[8], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[8], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_NO_CARD) {
            Fader_Step(&sCardMsg[10 - sCardOpSlot], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[10 - sCardOpSlot], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_ENCODING) {
            Fader_Step(&sCardMsg[12 - sCardOpSlot], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[12 - sCardOpSlot], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_DIFFERENT_SIZE) {
            Fader_Step(&sCardMsg[15], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[15], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_SAME_FILE) {
            Fader_Step(&sCardMsg[16], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[16], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_NO_SPACE) {
            Fader_Step(&sCardMsg[17], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[17], FADE_OUT);
        }
        if (sCardOpError == CARD_UI_BROKEN) {
            Fader_Step(&sCardMsg[18], FADE_IN);
        } else {
            Fader_Step(&sCardMsg[18], FADE_OUT);
        }
    } else {
        Fader_Step(&sCardMsg[7], FADE_OUT);
        Fader_Step(&sCardMsg[8], FADE_OUT);
        Fader_Step(&sCardMsg[9], FADE_OUT);
        Fader_Step(&sCardMsg[10], FADE_OUT);
        Fader_Step(&sCardMsg[11], FADE_OUT);
        Fader_Step(&sCardMsg[12], FADE_OUT);
        Fader_Step(&sCardMsg[15], FADE_OUT);
        Fader_Step(&sCardMsg[16], FADE_OUT);
        Fader_Step(&sCardMsg[17], FADE_OUT);
        Fader_Step(&sCardMsg[18], FADE_OUT);
    }
}

/* 0x81318CB8 */
void CardUi_UpdateResultMsg(void) {
    if (sCardUiState == 5) {
        if (sCardOpError != CARD_UI_OK) {
            if (sCardOpKind == 2 || sCardOpKind == 5) {
                CardUi_CopyResultMsg(TRUE);
            }
            if (sCardOpKind == 3 || sCardOpKind == 6) {
                CardUi_MoveResultMsg(TRUE);
            }
            if (sCardOpKind == 4 || sCardOpKind == 7) {
                if (sCardOpError == 0) {
                    Fader_Step(&sCardMsg[31], FADE_IN);
                    Fader_Step(&sCardMsg[32], FADE_OUT);
                } else {
                    Fader_Step(&sCardMsg[31], FADE_OUT);
                    Fader_Step(&sCardMsg[32], FADE_IN);
                }
            }
            if (sCardOpKind == 1 || sCardOpKind == 8) {
                if (sCardOpError == 0) {
                    Fader_Step(&sCardMsg[43 + sCardOpSlot], FADE_IN);
                    Fader_Step(&sCardMsg[45 + sCardOpSlot], FADE_OUT);
                } else {
                    Fader_Step(&sCardMsg[43 + sCardOpSlot], FADE_OUT);
                    Fader_Step(&sCardMsg[45 + sCardOpSlot], FADE_IN);
                }
            }
        }
    } else {
        CardUi_CopyResultMsg(FALSE);
        CardUi_MoveResultMsg(FALSE);
        Fader_Step(&sCardMsg[31], FADE_OUT);
        Fader_Step(&sCardMsg[32], FADE_OUT);
        Fader_Step(&sCardMsg[43 + sCardOpSlot], FADE_OUT);
        Fader_Step(&sCardMsg[45 + sCardOpSlot], FADE_OUT);
    }
}

/* 0x81318E80: the blinking slot cursor. */
void CardUi_UpdateCursor(void) {
    CardSlot* slots = CardThread_GetSlots();
    int i;
    int n;

    sCardArrow += sCardArrowDir;
    if (sCardArrow < 0) {
        sCardArrow = 0;
        sCardArrowDir = 1;
    }
    if (sCardArrow > sCardArrowMax) {
        sCardArrow = sCardArrowMax;
        sCardArrowDir = -1;
    }
    n = 0;
    for (i = 0; i < 2; i++) {
        if (i == sCardSlot && slots[i].state == CARD_SLOT_READY) {
            Fader_Step(&sCardFadeF[i], FADE_IN);
            n++;
        } else {
            Fader_Step(&sCardFadeF[i], FADE_OUT);
        }
    }
    if (n == 0) {
        Fader_Step(&sCardFadeF[2], FADE_IN);
    } else {
        Fader_Step(&sCardFadeF[2], FADE_OUT);
    }
}

/* 0x81318F70 */
s32 Card_Update(void) {
    CardSlot* p;
    s16* st;

    CardUi_UpdateFaders();
    CardUi_UpdateIconAnim();
    p = &CardThread_GetSlots()[sCardSlot];
    st = &p->state;
    if (*st == 4 && sCardUiState == 2) {
        CardUi_StepSlotFader(&sCardSlotFade[sCardSlot], 0);
        CardUi_StepSlotFader(&sCardSlotFade[sCardSlot ^ 1], 1);
    } else if (*st == 4 && sCardUiState == 3) {
        CardUi_StepSlotFader(&sCardSlotFade[sCardSlot], 2);
        CardUi_StepSlotFader(&sCardSlotFade[sCardSlot ^ 1], 1);
    } else {
        CardUi_StepSlotFader(&sCardSlotFade[0], 1);
        CardUi_StepSlotFader(&sCardSlotFade[1], 1);
    }
    CardUi_UpdateResultMsg();
    sCardBlink += sCardBlinkDir;
    if (sCardBlink < 0) {
        sCardBlink = 0;
        sCardBlinkDir = 1;
    }
    if (sCardBlink > sCardBlinkMax) {
        sCardBlink = sCardBlinkMax;
        sCardBlinkDir = -1;
    }
    CardUi_UpdateCursor();
    return CardUi_Update();
}

/* 0x813190C8 */
u32 Card_GetSelectedFile(void) {
    return sCardSort[sCardSlot][sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4].file;
}

/* 0x8131911C */
u8 Card_GetSlot(void) {
    return sCardSlot;
}

/* 0x81319124 */
void Card_Enter(void) {
    Fader_Reset(&sCardMsg[19]);
    Fader_Reset(&sCardMsg[20]);
    Fader_Reset(&sCardMsg[21]);
    Fader_Reset(&sCardMsg[22]);
    Fader_Reset(&sCardMsg[23]);
    Fader_Reset(&sCardMsg[24]);
    Fader_Reset(&sCardMsg[27]);
    Fader_Reset(&sCardMsg[28]);
    Fader_Reset(&sCardMsg[29]);
    Fader_Reset(&sCardMsg[30]);
    Fader_Reset(&sCardMsg[7]);
    Fader_Reset(&sCardMsg[8]);
    Fader_Reset(&sCardMsg[9]);
    Fader_Reset(&sCardMsg[10]);
    Fader_Reset(&sCardMsg[11]);
    Fader_Reset(&sCardMsg[12]);
    Fader_Reset(&sCardMsg[15]);
    Fader_Reset(&sCardMsg[16]);
    Fader_Reset(&sCardMsg[17]);
    Fader_Reset(&sCardMsg[18]);
    Fader_Reset(&sCardMsg[31]);
    Fader_Reset(&sCardMsg[32]);
    Fader_Reset(&sCardMsg[43]);
    Fader_Reset(&sCardMsg[44]);
    Fader_Reset(&sCardMsg[45]);
    Fader_Reset(&sCardMsg[46]);
    Fader_Reset(&sCardMsg[33]);
    Fader_Reset(&sCardMsg[34]);
    Fader_Reset(&sCardMsg[35]);
    Fader_Reset(&sCardMsg[36]);
}

/* 0x813191F8 */
void CardDraw_SetupTev(void) {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 4);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3X4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXSetCullMode(GX_CULL_BACK);
}

/* 0x81319414: one digit cell of the 11-cell number strip. */
void CardDraw_LoadIconTex(s32 x, s32 y, s32 digit) {
    f32 u = digit / 11.0f;
    s16 w = sCardBarTex->width * 16 / 11;
    s16 h = sCardBarTex->height * 16;
    s16 unused[4];

    CardDraw_SetupTev();
    x -= w / 2;
    y -= h / 2;
    J3D_LoadTexture(0, sCardBarTex);
    GXBegin(GX_QUADS, GX_VTXFMT1, 4);
    GXPosition3s16(x, y, 0);
    GXTexCoord2f32(u, 0.0f);
    GXPosition3s16(x + w, y, 0);
    GXTexCoord2f32(u + 1.0f / 11.0f, 0.0f);
    GXPosition3s16(x + w, y + h, 0);
    GXTexCoord2f32(u + 1.0f / 11.0f, 1.0f);
    GXPosition3s16(x, y + h, 0);
    GXTexCoord2f32(u, 1.0f);
    GXEnd();
}

/* 0x81319568: a number of up to four digits, centred on x. */
void CardDraw_LoadTexture(s32 x, s32 y, s32 n) {
    s32 w = sCardBarTex->width * 16 / 11;

    if (n < 10) {
        CardDraw_LoadIconTex(x, y, n % 10);
    } else if (n < 100) {
        x -= (w + 32) / 2;
        CardDraw_LoadIconTex(x, y, n / 10 % 10);
        x += w + 32;
        CardDraw_LoadIconTex(x, y, n % 10);
    } else if (n < 1000) {
        x -= w + 32;
        CardDraw_LoadIconTex(x, y, n / 100 % 10);
        x += w + 32;
        CardDraw_LoadIconTex(x, y, n / 10 % 10);
        x += w + 32;
        CardDraw_LoadIconTex(x, y, n % 10);
    } else {
        x -= (w + 32) * 3 / 2;
        CardDraw_LoadIconTex(x, y, n / 1000 % 10);
        x += w + 32;
        CardDraw_LoadIconTex(x, y, n / 100 % 10);
        x += w + 32;
        CardDraw_LoadIconTex(x, y, n / 10 % 10);
        x += w + 32;
        CardDraw_LoadIconTex(x, y, n % 10);
    }
}

/* 0x81319858: loads frame `frame` of a file's icon into GX_TEXMAP0. */
BOOL CardDraw_SetIconFrame(s32 slot, s32 file, s32 frame) {
    CardIconInfo(*icons)[CARD_MAX_FILES] = CardThread_GetIcons();
    CardIconInfo* icon = &icons[slot][file];
    u8* image = (u8*)icon + (u32)icons[slot][file].image[frame];
    TexHeader tex;

    if (icons[slot][file].hasIcon == 0) {
        return FALSE;
    }
    tex.mipmap = 0;
    tex.edgeLod = 0;
    tex.biasClamp = 0;
    tex.maxAniso = 0;
    tex.width = 32;
    tex.height = 32;
    tex.wrapS = GX_CLAMP;
    tex.wrapT = GX_CLAMP;
    tex.format = icons[slot][file].format[frame];
    tex.minFilter = GX_LINEAR;
    tex.magFilter = GX_LINEAR;
    tex.minLod = 0;
    tex.maxLod = 0;
    tex.lodBias = 0;
    if (icons[slot][file].format[frame] == GX_TF_RGB5A3) {
        tex.indexed = 0;
        tex.imageOffset = image - (u8*)&tex;
        J3D_LoadTexture(0, &tex);
        return TRUE;
    } else if (icons[slot][file].format[frame] == GX_TF_C8) {
        u8* tlut = (u8*)icon + (u32)icons[slot][file].tlut;

        tex.indexed = 1;
        tex.tlutFormat = GX_TL_RGB5A3;
        tex.tlutCount = 256;
        tex.tlutOffset = tlut - (u8*)&tex;
        tex.imageOffset = image - (u8*)&tex;
        J3D_LoadTexture(0, &tex);
        return TRUE;
    }
    return FALSE;
}

/* 0x813199AC: picks the icon frame for the file's animation timer. */
BOOL CardDraw_IconFrameCount(s32 slot, s32 file) {
    CardIconInfo(*icons)[CARD_MAX_FILES] = CardThread_GetIcons();
    s16 n = 0;
    s32 t = 0;

    if (slot != 0 && slot != 1) {
        return FALSE;
    }
    if (file < 0 || file > CARD_MAX_FILES) {
        return FALSE;
    }
    do {
        t += ((icons[slot][file].speeds >> (n * 2)) & 3) * 4;
        n++;
    } while (sCardIconFrame[slot][file] > t && n <= 8);
    return CardDraw_SetIconFrame(slot, file, (u8)(n - 1));
}

/* white with alpha a */
static inline GXColor CardDraw_White(u8 a) {
    GXColor c = { 255, 255, 255, 0 };

    c.a = a;
    return c;
}

/* 0x81319A8C: icon of grid cell `pos`; the selected file without a */
/* banner also gets the 'ic00' pane. */
void CardDraw_Icon(u8 slot, s16 pos, u8 frame, u8 alpha) {
    CardIconInfo(*icons)[CARD_MAX_FILES] = CardThread_GetIcons();
    GXColor color = CardDraw_White(alpha);
    u32 file;
    u32 sel;

    sel = sCardSort[sCardSlot][sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4].file;
    file = sCardSort[slot][pos + sCardTop[slot] * 4].file;
    if (CardDraw_SetIconFrame(slot, file, frame)) {
        GX_Setup2D(1, 0, 1);
        if (sel == file && slot == sCardSlot && icons[slot][file].hasBanner == 0) {
            Glh_DrawPaneTex('ic00', sCardGlh, &color, NULL);
        }
    }
}

/* 0x81319BD4: slot header, its free-block count and title. */
void CardDraw_SlotHeader(u8 slot, CardSlot* slots, GXColor* c) {
    CardParams* p = Card_GetParams();
    GXColor mat;
    s16 a;
    s16 unused; /* only takes a stack slot */

    if (p->s6C & 1) {
        Fader_GetAlpha(&sCardFadeF[slot], &a, NULL);
        c->a = c->a * (u8)(0xFF - p->s6E + p->s6E * a / 255) / 255;
    }
    if ((p->s6C & 2) && slot == sCardSlot) {
        c->a = c->a * (u8)(0xFF - p->s6E + p->s6E * sCardArrow / sCardArrowMax) / 255;
    }
    Glh_DrawPane('texa' + slot, sCardGlh, c);
    mat = *c;
    GXSetChanMatColor(GX_COLOR0A0, mat);
    CardDraw_LoadTexture(sCardNameX[slot], sCardNameY[slot], slots[slot].freeBlocks);
    Glh_DrawFrame('ffra' + slot, sCardGlh, c);
    GX_Setup2DText();
    Glh_DrawText(slot, sCardSth, sCardGlh, c);
}

/* 0x81319D6C: one slot: header, scroll arrows and the selected file's banner. */
void CardDraw_FileBanner(u8 slot, CardSlot* slots, GXColor* c) {
    s16 a;
    GXColor hdr;
    s16 arrow[2];
    s16 unused;
    u8 alpha;
    u8 a2;

    GX_Setup2DTex();
    Fader_GetAlpha(&sCardFadeA[slot], &a, NULL);
    alpha = c->a;
    c->a = alpha * a / 255;
    /* the copy is scheduled after the alpha math, as a comma */
    CardDraw_SlotHeader(slot, slots, (hdr = *c, &hdr));
    GX_Setup2DTex();
    a2 = c->a;
    Fader_GetAlpha(&sCardFadeD[slot], &arrow[0], NULL);
    Fader_GetAlpha(&sCardFadeE[slot], &arrow[1], NULL);
    c->a = a2 * arrow[0] / 255;
    Glh_DrawPaneOfs(sCardFadeD[slot].id, sCardGlh, c, 0, Hermite(sCardBlink, 0.0f, 0.0f, 0.0f, sCardBlinkMax, 128.0f, 0.0f));
    c->a = a2 * arrow[1] / 255;
    Glh_DrawPaneOfs(sCardFadeE[slot].id, sCardGlh, c, 0, -(s32)Hermite(sCardBlink, 0.0f, 0.0f, 0.0f, sCardBlinkMax, 128.0f, 0.0f));
    c->a = a2;
    if (slot == sCardSlot) {
        GX_Setup2D(1, 0, 1);
        if (slots[sCardSlot].state == CARD_SLOT_READY) {
            if (CardDraw_LoadFileIcon()) {
                Glh_DrawPaneTex('bana', sCardGlh, c, NULL);
            }
            CardDraw_FileInfo(c->a);
        }
    }
    c->a = alpha;
}

/* animation frame of a file's icon */
static inline u8 Card_IconFrame(u8 slot, s16 pos) {
    CardIconInfo(*icons)[CARD_MAX_FILES] = CardThread_GetIcons();
    u32 file = sCardSort[slot][pos + sCardTop[slot] * 4].file;
    s16 n = 0;
    s32 t = 0;

    do {
        t += ((icons[slot][file].speeds >> (n * 2)) & 3) * 4;
        n++;
    } while (sCardIconFrame[slot][file] > t && n <= 8);
    return n - 1;
}

/* 0x81319FC8: the 4x4 icon grid of a slot. */
void CardDraw_FileGrid(u8 slot, u8 alpha) {
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    s16 unused[2];
    s16 a;
    s16 unused2[2];
    s16 i;

    Fader_GetAlpha(&sCardFadeA[slot], &a, NULL);
    alpha = alpha * a / 255;
    GX_Setup2DTex();
    for (i = 0; i < 16; i++) {
        if (files[slot][sCardSort[slot][i + sCardTop[slot] * 4].file].used) {
            CardDraw_Icon(slot, i, Card_IconFrame(slot, i), alpha);
        }
    }
}

/* widest of the two choice strings (5, 6) in text pane of string idx */
static inline u16 CardDraw_ChoiceWidth(u16 idx, u16* h, u16* lineH) {
    SthEntry* entries = ((Sth*)sCardSth)->entry;
    GlhText* texts = GLH_TEXTS(sCardGlh);
    u16 w = 0;
    u16 max = 0;
    u16 i;

    for (i = 0; i < 2; i++) {
        Glh_MeasureString(&texts[entries[idx].pane], (u8*)&entries[i + 5] + entries[i + 5].ofs, &w, h);
        if (max < w) {
            max = w;
        }
    }
    return max;
}

/* 0x8131A13C: a message with a two-choice menu under it (0x27 copy, 0x29 move). */
void CardDraw_OpMenu(u16 msg, u8 slot, u8 alpha) {
    DispSettings* disp = Disp_GetSettings();
    CardParams* p = Card_GetParams();
    u16 maxW = 0;
    u16 w = 0;
    u16 h = 0;
    u16 lh = 0;
    SthEntry* entries = ((Sth*)sCardSth)->entry;
    GlhText* texts = GLH_TEXTS(sCardGlh);
    s16 a;
    s32 slide;
    GlhText box;
    GXColor c;
    GXColor glow;
    s32 idx;
    SthEntry* e;
    SthEntry* entries2;
    GlhText* texts2;
    u16 boxH;
    u16 total;
    u16 x;
    u16 y;
    u16 i;
    BOOL sel;
    Fader* f;

    c.r = p->white[0];
    c.g = p->white[1];
    c.b = p->white[2];
    c.a = 0xFF;
    glow.r = p->cream[0];
    glow.g = p->cream[1];
    glow.b = p->cream[2];
    glow.a = 0xFF;
    GX_Setup2DView();
    idx = msg + slot;
    f = &sCardMsg[idx];
    Fader_GetAlpha(f, &a, &slide);
    glow.a = c.a = alpha * a / 255;
    GX_Setup2DText();
    e = &entries[idx];
    Glh_MeasureString(&texts[e->pane], (u8*)e + e->ofs, &w, &h);
    entries2 = ((Sth*)sCardSth)->entry;
    texts2 = GLH_TEXTS(sCardGlh);
    {
        u16 sw = 0;
        s16 unused[4];

        for (i = 0; i < 2; i++) {
            Glh_MeasureString(&texts2[entries2[(u16)idx].pane], (u8*)&entries2[i + 5] + entries2[i + 5].ofs, &sw, &lh);
            if (maxW < sw) {
                maxW = sw;
            }
        }
    }
    h += disp->textPad * 32;
    boxH = lh + texts2[entries2[(u16)idx].pane].lineSpace * 16 + disp->textPad * 32;
    total = h + (boxH + Card_GetParams()->cols * 16);
    Glh_DrawStringOfs(idx, sCardSth, sCardGlh, &c, 0, -total / 2 + h / 2);
    if (f->frame > sCardMsg[idx].length / 2) {
        glow.a = c.a = alpha * (f->frame - sCardMsg[idx].length / 2) * 2 / sCardMsg[idx].length;
    } else {
        glow.a = c.a = 0;
    }
    x = texts[e->pane].x - (maxW / 2 + disp->textPad * 16);
    y = (total / 2 + texts[e->pane].y) - boxH;
    Glh_DrawPaneRect(texts[e->pane].bgPane, sCardGlh, &c, x, y, maxW + disp->textPad * 32, boxH);
    CardDraw_SetScissor(x, y, maxW + disp->textPad * 32, boxH, (u8)(sCardMsg[idx].ticks * 5), &c);
    box = texts[e->pane];
    box.y = y + (disp->textPad * 16 + lh / 2);
    box.bgPane = 0xFFFF;
    GX_Setup2DText();
    for (i = 0; i < 2; i++) {
        fn_813095A0(&glow, &glow);
        fn_81309554(&c, &c);
        if (msg == 0x27) {
            sel = (((sCardFmtA[slot] & 2) == 2 && i == 0) || ((sCardFmtA[slot] & 2) != 2 && i == (sCardFmtA[slot] & 1))) && sCardSlot == slot;
        } else {
            sel = i == (sCardFmtA[slot] & 1) && sCardSlot == slot;
        }
        if (sel) {
            fn_813090A4(&box, (u8*)&entries[i + 5] + entries[i + 5].ofs);
        } else {
            fn_81308DB8(&box, (u8*)&entries[i + 5] + entries[i + 5].ofs);
        }
        box.y += box.lineSpace * 16;
    }
}

/* 0x8131A644 */
void fn_8131A644(CardSlotFade* f) {
    f->a = 0;
    f->aMax = 20;
    f->b = 0;
    f->bMax = 30;
    f->s8 = 10;
    f->sA = 20;
    f->alpha = 255;
    f->s10 = 40;
    f->s12 = 20;
    f->s14 = 0;
}

/* 0x8131A680: mode 0 open, 1 close, 2 open the second part. */
void CardUi_StepSlotFader(CardSlotFade* f, u8 mode) {
    switch (mode) {
    case 0:
        f->a++;
        if (f->a > f->aMax) {
            f->a = f->aMax;
        }
        f->b--;
        if (f->b < 0) {
            f->b = 0;
        }
        break;
    case 2:
        f->a++;
        if (f->a > f->aMax) {
            f->a = f->aMax;
        }
        f->b++;
        if (f->b > f->bMax) {
            f->b = f->bMax;
        }
        break;
    case 1:
        f->a--;
        if (f->a < 0) {
            f->a = 0;
        }
        f->b--;
        if (f->b < 0) {
            f->b = 0;
        }
        break;
    }
    f->phase += Card_GetParams()->rows;
}

/* 0x8131A7B4: pulsing outline around a box. */
void CardDraw_SetScissor(s32 x, s32 y, s32 w, s32 h, s32 t, GXColor* c) {
    u32 color = 0xC0C0C000;
    u32 a;

    fn_8130AB14();
    a = (0x100 - t) * 255 / 256;
    x -= t + 16;
    y -= t + 16;
    w += t * 2 + 32;
    h += t * 2 + 32;
    if (a > 255) {
        a = 255;
    }
    color += (u8)(a * c->a / 255);
    GXSetLineWidth(18, GX_TO_ZERO);
    GXBegin(GX_LINES, GX_VTXFMT1, 8);
    GXPosition3s16(x, y, 0);
    GXColor1u32(color);
    GXPosition3s16(x + w, y, 0);
    GXColor1u32(color);
    GXPosition3s16(x, y, 0);
    GXColor1u32(color);
    GXPosition3s16(x, y + h, 0);
    GXColor1u32(color);
    GXPosition3s16(x, y + h, 0);
    GXColor1u32(color);
    GXPosition3s16(x + w, y + h, 0);
    GXColor1u32(color);
    GXPosition3s16(x + w, y, 0);
    GXColor1u32(color);
    GXPosition3s16(x + w, y + h, 0);
    GXColor1u32(color);
    GXEnd();
}

/* 0x8131A914: sizes of the Copy / Move / Erase box. */
void CardDraw_MeasureLines(CardSlotFade* f, void* sth, void* glh, u16* outW) {
    SthEntry* entries = ((Sth*)sth)->entry;
    GlhText* texts = GLH_TEXTS(glh);
    u16 max = 0;
    u16 i;
    u16 w;
    u16 step;
    u16 str;
    SthEntry* e;

    for (i = 0, str = 2; i < 3; str++, i++) {
        e = &entries[i + 2];
        Glh_MeasureString(&texts[entries[str].pane], STH_STRING(e), &w, &f->lineH);
        if (max < w) {
            max = w;
        }
    }
    f->h3 = f->lineH + texts[entries[2].pane].lineSpace * 32;
    if (f->b < f->s8) {
        step = texts[entries[2].pane].lineSpace * 32;
    } else if (f->b >= f->s8 && f->b < f->sA) {
        step = (f->sA - f->b) * (texts[entries[2].pane].lineSpace * 32) / (f->sA - f->s8);
    } else if (f->b >= f->sA) {
        step = 0;
    }
    f->h1 = f->lineH + step;
    f->h2 = f->lineH + texts[entries[2].pane].lineSpace * 16;
    *outW = max;
}

/* 0x8131AA6C: the Copy / Move / Erase lines. */
void CardDraw_TextBox(u8 slot, CardSlotFade* f, void* sth, void* glh, GXColor* c, s32 x, s32 y, s32 w) {
    CardParams* p = Card_GetParams();
    SthEntry* entries = ((Sth*)sth)->entry;
    GlhText* texts = GLH_TEXTS(glh);
    GXColor col;
    GXColor glow;
    GXColor g2;
    GXColor c2;
    s16 a[3];
    GlhText box;
    u16 step;
    u16 i;
    u16 str;

    col.r = p->white[0];
    col.g = p->white[1];
    col.b = p->white[2];
    col.a = 0xFF;
    glow.r = p->cream[0];
    glow.g = p->cream[1];
    glow.b = p->cream[2];
    glow.a = 0xFF;
    box = texts[entries[2].pane];
    box.x = x;
    box.y = y + f->lineH / 2;
    box.w = w;
    box.h = f->lineH;
    col.a = f->b < f->s8 ? (s16)(c->a * (f->s8 - f->b) / f->s8) : 0;
    glow.a = c->a;
    GX_Setup2DText();
    if (f->b < f->s8) {
        step = box.lineSpace * 16;
    } else if (f->b >= f->s8 && f->b < f->sA) {
        step = box.lineSpace * 16 * (f->sA - f->b) / (f->sA - f->s8);
    } else if (f->b >= f->sA) {
        step = 0;
    }
    Fader_GetAlpha(&sCardFadeA[slot ^ 1], &a[2], NULL);
    Fader_GetAlpha(&sCardFadeB[slot], &a[1], NULL);
    Fader_GetAlpha(&sCardFadeC[slot], &a[0], NULL);
    a[2] /= 2;
    a[1] /= 2;
    a[0] /= 2;
    for (i = 0, str = 2; i < 3; str++, i++) {
        if (i <= 1) {
            u8 k;
            CardParams* p2;

            g2 = glow;
            c2 = col;
            k = a[i];
            p2 = Card_GetParams();

            c2.r = c2.r * (p2->collapseFade + k) / (p2->collapseFade + 127);
            c2.g = c2.g * (p2->collapseFade + k) / (p2->collapseFade + 127);
            c2.b = c2.b * (p2->collapseFade + k) / (p2->collapseFade + 127);
            if (sCardOp == i) {
                if (sCardUiState == 2) {
                    fn_813095A0(&g2, &g2);
                    c2.a = g2.a;
                    fn_81309554(&c2, &c2);
                    fn_813090A4(&box, STH_STRING(&entries[str]));
                } else {
                    Font_SetColorU32(c->a - 0x100, c->a - 0x100);
                    fn_81308DB8(&box, STH_STRING(&entries[str]));
                }
            } else {
                fn_81309554(&c2, &c2);
                fn_81308DB8(&box, STH_STRING(&entries[str]));
            }
        } else if (sCardOp == i) {
            if (sCardUiState == 2) {
                fn_813095A0(&glow, &glow);
                Font_SetColorU32(c->a - 0x100, c->a - 0x100);
                fn_813090A4(&box, STH_STRING(&entries[str]));
            } else {
                Font_SetColorU32(c->a - 0x100, c->a - 0x100);
                fn_81308DB8(&box, STH_STRING(&entries[str]));
            }
        } else {
            fn_81309554(&col, &col);
            fn_81308DB8(&box, STH_STRING(&entries[str]));
        }
        box.y += step;
    }
}

/* 0x8131AE80: the Yes / No lines. */
void CardDraw_TextLines(CardSlotFade* f, void* sth, void* glh, GXColor* c, s32 x, s32 y, s32 w) {
    CardParams* p = Card_GetParams();
    SthEntry* entries = ((Sth*)sth)->entry;
    GlhText* texts = GLH_TEXTS(glh);
    GXColor col;
    GXColor glow;
    GlhText box;
    u16 i;
    u16 str;

    col.r = p->white[0];
    col.g = p->white[1];
    col.b = p->white[2];
    col.a = c->a;
    glow.r = p->cream[0];
    glow.g = p->cream[1];
    glow.b = p->cream[2];
    glow.a = c->a;
    box = texts[entries[5].pane];
    box.x = x;
    box.y = y + f->lineH / 2;
    box.w = w;
    box.h = f->lineH;
    GX_Setup2DText();
    for (i = 0, str = 5; i < 2; str++, i++) {
        fn_81309554(&col, &col);
        fn_813095A0(&glow, &glow);
        if (sCardYesNo == i) {
            fn_813090A4(&box, STH_STRING(&entries[str]));
        } else {
            fn_81308DB8(&box, STH_STRING(&entries[str]));
        }
        box.y += box.lineSpace * 16;
    }
}

/* 0x8131B018: the op menu box next to the cursor and the Yes/No box under it. */
void CardDraw_Dialog(u8 slot, CardSlotFade* f, void* sth, void* glh, GXColor* c) {
    GlhFrame* frames = GLH_FRAMES(glh);
    DispSettings* disp = Disp_GetSettings();
    u16 pane;
    s32 x;
    s32 y;
    u16 w;
    s16 a;
    s32 dy;
    s32 y1;
    u8 alpha;
    s16 a2;
    s32 dy2;
    s16 unused[4];

    for (pane = 0; pane < ((Glh*)glh)->nFrames; pane++) {
        if (frames[pane].tag == 'mes0') {
            break;
        }
    }
    x = sCardAnchorX[slot][sCardCol[slot] + sCardRow[slot] * 4];
    y = sCardAnchorY[slot][sCardCol[slot] + sCardRow[slot] * 4];
    CardDraw_MeasureLines(f, sth, glh, &w);
    a = f->alpha * f->a / f->aMax;
    dy = f->s10 * 16 * (f->aMax - f->a) / f->aMax;
    if (a != 0) {
        if (sCardCol[slot] < 2) {
            x += 0x280;
        } else {
            x -= (w / 16 + disp->textPad * 2 + 40) * 16;
        }
        if (sCardRow[slot] < 2) {
            c->a = c->a; /* dead store; keeps the empty-then branch */
        } else {
            dy = -dy;
            y -= f->h3 + disp->textPad * 8;
        }
        alpha = c->a;
        y1 = y + dy;
        c->a = alpha * a / 255;
        Glh_DrawPaneRect(pane, glh, c, x, y1, w + disp->textPad * 32, f->h1 + disp->textPad * 32);
        CardDraw_TextBox(slot, f, sth, glh, c, x + (w / 2 + disp->textPad * 16), y1 + disp->textPad * 16, w);
        c->a = c->a * (f->bMax - f->b) / f->bMax;
        CardDraw_SetScissor(x, y1, w + disp->textPad * 32, f->h1 + disp->textPad * 32, (u8)f->phase, c);
        c->a = alpha;
        if (f->b >= f->sA) {
            a2 = f->alpha * (f->b - f->sA) / (f->bMax - f->sA);
            dy2 = f->s12 * 16 * (f->bMax - f->b) / (f->bMax - f->sA);
        } else {
            a2 = 0;
            dy2 = 0;
        }
        c->a = c->a * a2 / 255;
        Glh_DrawPaneRect(pane, glh, c, x, y + (f->h1 + disp->textPad * 32 + dy2 + 0x80), w + disp->textPad * 32, f->h2 + disp->textPad * 32);
        CardDraw_SetScissor(x, y + (f->h1 + disp->textPad * 32 + dy2 + 0x80), w + disp->textPad * 32, f->h2 + disp->textPad * 32, (u8)f->phase, c);
        CardDraw_TextLines(f, sth, glh, c, x + (w / 2 + disp->textPad * 16), y + (disp->textPad * 16 + f->lineH + disp->textPad * 32 + dy2 + 0x80), w);
        c->a = alpha;
    }
}

/* 0x8131B3CC: loads the selected file's banner into GX_TEXMAP0. */
u32 CardDraw_LoadFileIcon(void) {
    CardIconInfo(*icons)[CARD_MAX_FILES] = CardThread_GetIcons();
    CardFile(*files)[CARD_MAX_FILES] = CardThread_GetFiles();
    u32 file = sCardSort[sCardSlot][sCardCol[sCardSlot] + (sCardRow[sCardSlot] + sCardTop[sCardSlot]) * 4].file;
    CardIconInfo* icon;
    u8* image;
    s16 unused[4];
    TexHeader tex;

    if (files[sCardSlot][file].used == 0) {
        return FALSE;
    }
    icon = &icons[sCardSlot][file];
    image = (u8*)icon + (u32)icons[sCardSlot][file].banner;
    if (icon->hasBanner == 0) {
        return FALSE;
    }
    tex.mipmap = 0;
    tex.edgeLod = 0;
    tex.biasClamp = 0;
    tex.maxAniso = 0;
    tex.width = 96;
    tex.height = 32;
    tex.wrapS = GX_CLAMP;
    tex.wrapT = GX_CLAMP;
    tex.format = icons[sCardSlot][file].bannerFormat;
    tex.minFilter = GX_LINEAR;
    tex.magFilter = GX_LINEAR;
    tex.minLod = 0;
    tex.maxLod = 0;
    tex.lodBias = 0;
    if (icons[sCardSlot][file].bannerFormat == GX_TF_RGB5A3) {
        tex.indexed = 0;
        tex.imageOffset = image - (u8*)&tex;
        J3D_LoadTexture(0, &tex);
        return TRUE;
    } else if (icons[sCardSlot][file].bannerFormat == GX_TF_C8) {
        u8* tlut = (u8*)icon + (u32)icons[sCardSlot][file].bannerTlut;

        tex.indexed = 1;
        tex.tlutFormat = GX_TL_RGB5A3;
        tex.tlutCount = 256;
        tex.tlutOffset = tlut - (u8*)&tex;
        tex.imageOffset = image - (u8*)&tex;
        J3D_LoadTexture(0, &tex);
        return TRUE;
    }
    return FALSE;
}

/* 0x8131B56C: title, comment and block count of the selected file. */
void CardDraw_FileInfo(u8 alpha) {
    u8 slot = sCardSlot;
    s16 file = sCardSort[slot][sCardCol[slot] + (sCardRow[slot] + sCardTop[slot]) * 4].file;
    GXColor frm = CardDraw_White(alpha);
    CardFile(*files)[CARD_MAX_FILES];

    files = CardThread_GetFiles();
    GX_Setup2DTex();
    if (files[slot][file].used) {
        GXColor c = { 255, 255, 255, 255 };

        c.a = c.a * alpha / 255;
        Glh_DrawBannerLine('titl', sCardGlh, &c, CardThread_GetComment(slot, file), 31);
        Glh_DrawBannerLine('info', sCardGlh, &c, CardThread_GetComment(slot, file) + 32, 31);
        Glh_DrawFrame('frmc', sCardGlh, &frm);
        CardDraw_LoadTexture(sCardNameX[2], sCardNameY[2], files[slot][file].blocks);
    }
}

/* 0x8131B6E0: result and status messages. */
void CardDraw_Messages(u8 alpha) {
    GXColor c = { 255, 255, 255, 255 };

    c.a = c.a * alpha / 255;
    GX_Setup2DText();
    Fader_DrawString(&sCardMsg[19], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[20], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[21], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[22], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[23], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[24], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[27], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[28], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[29], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[30], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[7], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[8], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[9], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[10], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[11], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[12], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[15], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[16], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[17], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[18], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[31], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[32], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[43], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[44], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[45], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[46], sCardSth, sCardGlh, &c);
}

/* one slot: banner, grid, slot texts and the two op menus */
static inline void CardDraw_Slot(u8 slot, u8 alpha) {
    CardSlot* slots = CardThread_GetSlots();
    GXColor c = { 255, 255, 255, 255 };

    c.a = c.a * alpha / 255;
    CardDraw_FileBanner(slot, slots, &c);
    CardDraw_FileGrid(slot, alpha);
    GX_Setup2DText();
    Fader_DrawString(&sCardMsg[33 + slot], sCardSth, sCardGlh, &c);
    Fader_DrawString(&sCardMsg[35 + slot], sCardSth, sCardGlh, &c);
    CardDraw_OpMenu(0x27, slot, alpha);
    CardDraw_OpMenu(0x29, slot, alpha);
}

/* 0x8131B94C: drawn by the menu while zoomed into the Memory Card face. */
/* lamp spot over slot s's card */
static inline void Card_SpotPos(Vec* v, u8 s) {
    CardParams* p = Card_GetParams();

    v->x = (sCardAnchorX[s][5] + sCardAnchorX[s][6]) / 32.0f;
    v->y = (sCardAnchorY[s][5] + sCardAnchorY[s][9]) / 32.0f;
    v->z = p->f70;
}

static inline void CardDraw_Dialogs(u8 alpha) {
    GXColor c = { 255, 255, 255, 255 };

    c.a = c.a * alpha / 255;
    CardDraw_Dialog(0, &sCardSlotFade[0], sCardSth, sCardGlh, &c);
    CardDraw_Dialog(1, &sCardSlotFade[1], sCardSth, sCardGlh, &c);
}

void Card_Draw(u8 fxAlpha, u8 bgAlpha, u8 alpha) {
    DispSettings* disp = Disp_GetSettings();
    CardSlot* slots = CardThread_GetSlots();
    SRT srt = { 1.0f, -1.0f, 1.0f, 0, 0, 0, 0, -292.0f, 224.0f, 0.0f };
    Vec up = { 0.0f, 1.0f, 0.0f };
    Vec eye = { 0.0f, 0.0f, 0.0f };
    Vec at = { 0.0f, 0.0f, 0.0f };
    Vec spot1;
    Vec spot0;
    s16 a[2];
    s16 a2;
    Mtx mv;
    Mtx view;
    Mtx nrm;
    s16 unused[2];

    eye.z = 224.0f / tanf(3.141592f * disp->fovy / 360.0f);
    C_MTXLookAt(view, &eye, &up, &at);
    MtxFromSRT(&srt, mv);
    PSMTXConcat(view, mv, mv);
    MtxInverse33(mv, nrm);
    GXLoadPosMtxImm(mv, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    Card_SpotPos(&spot0, 0);
    Card_SpotPos(&spot1, 1);
    Fader_GetAlpha(&sCardFadeF[0], &a[0], NULL);
    Fader_GetAlpha(&sCardFadeF[1], &a[1], NULL);
    Fader_GetAlpha(&sCardFadeF[2], &a2, NULL);
    Backdrop_DrawSpots(mv, bgAlpha, &spot0, &spot1, a[0], a[1], a2);

    {
        GXColor fr = { 255, 255, 255, 255 };

        CardThread_GetSlots();
        fr.a = fr.a * alpha / 255;
        Glh_DrawFrame('mes6', sCardGlh, &fr);
    }
    if (sCardSlot == 0) {
        fn_813216F4(0, fxAlpha);
        fn_813216F4(1, fxAlpha);
    } else {
        fn_813216F4(1, fxAlpha);
        fn_813216F4(0, fxAlpha);
    }
    GXLoadPosMtxImm(mv, GX_PNMTX0);
    GXLoadNrmMtxImm(nrm, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    if (sCardSlot == 0) {
        CardDraw_Slot(1, alpha);
        CardDraw_Slot(0, alpha);
    } else {
        CardDraw_Slot(0, alpha);
        CardDraw_Slot(1, alpha);
    }
    CardDraw_Dialogs(alpha);
    CardDraw_Messages(alpha);
}

/* 0x8131BF54 */
BOOL Card_IsBrowsing(void) {
    if (sCardUiState == 1) {
        return TRUE;
    }
    return FALSE;
}
