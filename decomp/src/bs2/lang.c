#include "bs2/res.h"
#include "bs2/gbase.h"
#include <dolphin/os.h>
#include <dolphin/os/OSRtc.h>
#include <dolphin/os/OSUtil.h>

/* Resource archive linked into BS2's .data: a count, then one offset per
   resource from the start of the archive. Yay0 entries are unpacked to
   the heap by ResInit and their offset rewritten. */
typedef struct ResArchive {
    s32 count;
    s32 offset[1];
} ResArchive;

extern ResArchive lbl_81381820[];

static ResArchive *sRes;
static s32 sCardLayout;
static s32 sGameLayout;
static s32 sClockLayout;
static s32 sDialogLayout;
static s32 sGuideLayout;
static s32 sOptionLayout;
static s32 sMenuTexBank;
static s32 sDigitTexBase;
static s32 sCardBarTex;
static u16 sLangCode; /* SRAM language + 4 */

/* Per-language resource ids, indexed by SRAM language
   (0 English, 1 German, 2 French, 3 Spanish, 4 Italian, 5 Dutch). */
static s32 sCardStrings[6];
static s32 sGameStrings[6];
static s32 sClockStrings[6];
static s32 sDialogStrings[6];
static s32 sGuideStrings[6];
static s32 sOptionStrings[6];
static s32 sCardFaceLayout[6];
static s32 sOptionFaceLayout[6];
static s32 sGameFaceLayout[6];
static s32 sClockFaceLayout[6];
static s32 sTopFaceLayout[6];
static s32 sWeekdayTex[6];
static s32 sMonoTex[6];
static s32 sStereoTex[6];
static s32 sFxGlyphs[6];

void Card_LoadLangRes(void);
void Menu_LoadLangRes(void);
void Scene_LoadLangRes(void);
void Game_LoadLangRes(void);
void Clock_LoadLangRes(void);
void Option_LoadLangRes(void);
void Menu_SetGuideLang(s32 lang);

/* 0x8130B474 */
u16 Lang_GetCode(void) {
    return sLangCode;
}

/* 0x8130B47C */
s32 Lang_Get(void) {
    return sLangCode - 4;
}

/* 0x8130B488: store the language in SRAM and reload every language resource */
void Lang_Set(s32 lang) {
    BOOL changed = FALSE;
    OSSram *sram = __OSLockSram();

    if (sram->language != lang) {
        sram->language = lang;
        changed = TRUE;
        sLangCode = lang + 4;
    }
    __OSUnlockSram(changed);
    if (changed) {
        while (!__OSSyncSram()) {
        }
        Card_LoadLangRes();
        Menu_LoadLangRes();
        Scene_LoadLangRes();
        Game_LoadLangRes();
        Clock_LoadLangRes();
        Option_LoadLangRes();
        Menu_SetGuideLang(lang);
    }
}

#define LANG_RES(tbl, en, de, fr, es, it, nl) \
    tbl[0] = en;                              \
    tbl[2] = fr;                              \
    tbl[1] = de;                              \
    tbl[5] = nl;                              \
    tbl[3] = es;                              \
    tbl[4] = it

/* 0x8130B518 */
void Lang_InitResTable(void) {
    sCardLayout = 28;
    sGameLayout = 21;
    sClockLayout = 14;
    sDialogLayout = 108;
    sGuideLayout = 115;
    sOptionLayout = 122;
    sMenuTexBank = 167;
    sDigitTexBase = 91;
    sCardBarTex = 90;
    LANG_RES(sCardStrings, 30, 32, 31, 34, 33, 29);
    LANG_RES(sGuideStrings, 117, 119, 118, 121, 120, 116);
    LANG_RES(sOptionStrings, 124, 126, 125, 128, 127, 123);
    LANG_RES(sGameStrings, 23, 25, 24, 27, 26, 22);
    LANG_RES(sClockStrings, 16, 18, 17, 20, 19, 15);
    LANG_RES(sDialogStrings, 110, 112, 111, 114, 113, 109);
    LANG_RES(sCardFaceLayout, 59, 151, 144, 165, 158, 134);
    LANG_RES(sGameFaceLayout, 56, 148, 141, 162, 155, 131);
    LANG_RES(sClockFaceLayout, 136, 146, 139, 160, 153, 129);
    LANG_RES(sTopFaceLayout, 57, 149, 142, 163, 156, 132);
    LANG_RES(sOptionFaceLayout, 138, 152, 145, 166, 159, 135);
    LANG_RES(sWeekdayTex, 49, 71, 62, 171, 80, 40);
    LANG_RES(sMonoTex, 47, 69, 60, 169, 78, 38);
    LANG_RES(sStereoTex, 48, 70, 61, 170, 79, 39);
    LANG_RES(sFxGlyphs, 180, 182, 181, 184, 183, 179);
}

/* 0x8130B84C */
void Lang_Init(void) {
    sLangCode = __OSLockSram()->language + 4;
    __OSUnlockSram(FALSE);
    Lang_InitResTable();
}

#define CUR_LANG ((s16)(sLangCode - 4))

s32 Res_CardLayout(void) { return sCardLayout; }                    /* 0x8130B884 */
s32 Res_GuideLayout(void) { return sGuideLayout; }                  /* 0x8130B88C */
s32 Res_OptionLayout(void) { return sOptionLayout; }                /* 0x8130B894 */
s32 Res_GameLayout(void) { return sGameLayout; }                    /* 0x8130B89C */
s32 Res_ClockLayout(void) { return sClockLayout; }                  /* 0x8130B8A4 */
s32 Res_DialogLayout(void) { return sDialogLayout; }                /* 0x8130B8AC */
s32 Res_CardStrings(void) { return sCardStrings[CUR_LANG]; }        /* 0x8130B8B4 */
s32 Res_GameStrings(void) { return sGameStrings[CUR_LANG]; }        /* 0x8130B8D8 */
s32 Res_ClockStrings(void) { return sClockStrings[CUR_LANG]; }      /* 0x8130B8FC */
s32 Res_DialogStrings(void) { return sDialogStrings[CUR_LANG]; }    /* 0x8130B920 */
s32 Res_CardFaceLayout(void) { return sCardFaceLayout[CUR_LANG]; }  /* 0x8130B944 */
s32 Res_OptionFaceLayout(void) { return sOptionFaceLayout[CUR_LANG]; } /* 0x8130B968 */
s32 Res_GameFaceLayout(void) { return sGameFaceLayout[CUR_LANG]; }  /* 0x8130B98C */
s32 Res_ClockFaceLayout(void) { return sClockFaceLayout[CUR_LANG]; } /* 0x8130B9B0 */
s32 Res_TopFaceLayout(void) { return sTopFaceLayout[CUR_LANG]; }    /* 0x8130B9D4 */
s32 Res_MenuTexBank(void) { return sMenuTexBank; }                  /* 0x8130B9F8 */
s32 Res_DigitTexBase(void) { return sDigitTexBase; }                /* 0x8130BA00 */
s32 Res_WeekdayTexBase(void) { return sWeekdayTex[CUR_LANG]; }      /* 0x8130BA08 */
s32 Res_MonoTex(void) { return sMonoTex[CUR_LANG]; }                /* 0x8130BA2C */
s32 Res_StereoTex(void) { return sStereoTex[CUR_LANG]; }            /* 0x8130BA50 */
s32 Res_CardBarTex(void) { return sCardBarTex; }                    /* 0x8130BA74 */
s32 Res_FxGlyphs(void) { return sFxGlyphs[CUR_LANG]; }              /* 0x8130BA7C */
s32 Res_GuideStrings(s32 lang) { return sGuideStrings[lang]; }      /* 0x8130BAA0 */
s32 Res_OptionStrings(s32 lang) { return sOptionStrings[lang]; }    /* 0x8130BAB8 */
s32 Res_FxGlyphsLang(s32 lang) { return sFxGlyphs[lang]; }          /* 0x8130BAD0 */

#define READ_BE32(p) ((p)[0] << 24 | (p)[1] << 16 | (p)[2] << 8 | (p)[3])

/* 0x8130BAE8: unpack a Yay0 block */
void Yay0Decode(u8 *src, u8 *dst) {
    s32 size;
    s32 codeOfs;
    s32 dstOfs;
    s32 linkOfs;
    s32 chunkOfs;
    u32 bits;
    u32 code;

    dstOfs = 0;
    bits = 0;
    size = READ_BE32(src + 4);
    linkOfs = READ_BE32(src + 8);
    chunkOfs = READ_BE32(src + 12);
    codeOfs = 16;

    do {
        if (bits == 0) {
            code = READ_BE32(src + codeOfs);
            codeOfs += 4;
            bits = 32;
        }
        if (code & 0x80000000) {
            dst[dstOfs++] = src[chunkOfs++];
        } else {
            s32 link = src[linkOfs] << 8 | src[linkOfs + 1];
            s32 back;
            s32 hi;
            s32 n;
            s32 i;

            linkOfs += 2;
            back = dstOfs - (link & 0xFFF);
            hi = link >> 12;
            if (hi == 0) {
                n = src[chunkOfs++] + 0x12;
            } else {
                n = hi + 2;
            }
            for (i = n; i > 0; i--) {
                dst[dstOfs] = dst[back - 1];
                dstOfs++;
                back++;
            }
        }
        code <<= 1;
        bits--;
    } while (dstOfs < size);
}

/* 0x8130BCB4: unpack every Yay0 resource of the archive */
void ResInit(void) {
    s32 i;
    s32 count;
    s32 *ofs;
    u8 *buf;
    s32 size;
    u8 *p;
    s32 unused[2];

    i = 0;
    sRes = lbl_81381820;
    count = *(ofs = (s32 *)sRes);
    for (; i < count; i++) {
        p = (u8 *)sRes + ofs[1];
        if (p[0] == 'Y' && p[1] == 'a' && p[2] == 'y') {
            size = READ_BE32(p + 4);
        } else {
            size = 0;
        }
        if (size != 0) {
            buf = HeapAlloc(OSRoundUp32B(size));
            Yay0Decode((u8 *)sRes + ofs[1], buf);
            ofs[1] = buf - (u8 *)sRes;
            DCStoreRange(buf, size);
        }
        ofs++;
    }
}

/* 0x8130BDA0 */
void *ResGet(s32 id) {
    return (u8 *)sRes + sRes->offset[id];
}
