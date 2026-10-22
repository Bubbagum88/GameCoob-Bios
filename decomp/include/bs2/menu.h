#ifndef BS2_MENU_H
#define BS2_MENU_H

#include "types.h"
#include "bs2/mtx.h"
#include "bs2/j3d.h"
#include <dolphin/gx.h>
#include <dolphin/os.h>

/* Pieces of structs owned by other modules that the menu touches.     */
/* Display settings returned by Disp_GetSettings (0x813010F8).          */
typedef struct DispSettings {
    u8 pad0[8];
    f32 fovy;              /* 0x08 vertical field of view in degrees */
    u8 padC[0x1E - 0xC];
    u8 menuFlag;           /* 0x1E cleared every menu frame */
    u8 pad1F[0x2B - 0x1F];
    u8 textPad;            /* 0x2B frame padding around dialog text, pixels */
    s16 punctHang;         /* 0x2C how far a line-end SJIS comma/period hangs out */
    u8 pad2E[0x32 - 0x2E];
    s16 glowMax;           /* 0x32 text glow alpha range */
    s16 glowMin;           /* 0x34 */
    s16 pulse;             /* 0x36 glow pulse, 0..pulseLen */
    s16 pulseLen;          /* 0x38 */
    s16 pulseStep;         /* 0x3A +1 or -1 */
} DispSettings;
DispSettings *Disp_GetSettings(void);

/* Scene (0x814AF878, scene.h) offset 0xB0: s16 set when the disc state
   reaches DISC_FATAL; the menu then only shows the error text. */
#define SCENE_FATAL_ERROR(s) (*(s16 *)((u8 *)(s) + 0xB0))

/*
 * IPL main menu (PAL 1.0 BS2, code 0x81310F58..0x813376A0).
 *
 * The menu is a cube with a 2D layout (GLH0) on five of its faces. The
 * front face is the menu title; pushing a direction rotates the cube 90
 * degrees so another face comes to the front, and A zooms into the
 * submenu that face stands for:
 *
 *   up    -> Game Play   (top face)       MENU_GAME_FACE / MENU_GAME
 *   down  -> Memory Card (bottom face)    MENU_CARD_FACE / MENU_CARD
 *   left  -> Options     (left face)      MENU_OPTION_FACE / MENU_OPTION
 *   right -> Calendar    (right face)     MENU_CLOCK_FACE / MENU_CLOCK
 *
 * Inside a submenu, text and icons are drawn as clouds of small cubes
 * ("mosaic" particles, menu_fx*.c) and a group of icon cubes animates
 * behind them (menu_icon_*.c).
 *
 * Field names marked "?" are guesses; offsets are exact.
 */

/* ------------------------------------------------------------------ */
/* Menu states (Scene state 5 runs Menu_Update every frame)            */
/* ------------------------------------------------------------------ */
typedef enum MenuState {
    MENU_TOP = 0,          /* front face, nothing selected */
    MENU_GAME_FACE = 1,    /* rotated to the Game Play face */
    MENU_GAME = 2,         /* inside Game Play */
    MENU_CARD_FACE = 3,
    MENU_CARD = 4,
    MENU_OPTION_FACE = 5,
    MENU_OPTION = 6,
    MENU_CLOCK_FACE = 7,
    MENU_CLOCK = 8
} MenuState;

/* Sound effect ids passed to Sound_Play (0x8135AA80). */
enum {
    SE_JINGLE = 0,         /* 0..2 boot jingles (normal / Z easter eggs) */
    SE_FACE_SELECT = 3,    /* cube turns to a face */
    SE_FACE_BACK = 4,      /* cube turns back to the front */
    SE_ENTER = 5,          /* A on a face: zoom into the submenu */
    SE_EXIT = 6,           /* B in a submenu: zoom back out */
    SE_CANCEL = 7,
    SE_DECIDE = 8,
    SE_CHANGE = 9,         /* value changed with left/right or up/down */
    SE_CURSOR_H = 10,      /* field cursor (calendar) / menu cursor (card) */
    SE_CURSOR = 11,        /* list cursor */
    SE_CONFIRM = 12,       /* value stored */
    SE_BUZZER = 13,        /* not allowed / end of list */
    SE_CARD_14 = 14,
    SE_CARD_15 = 15,
    SE_CARD_16 = 16,
    SE_CARD_17 = 17,
    SE_CARD_18 = 18,
    SE_CARD_19 = 19,
    SE_CARD_20 = 20,
    SE_CARD_21 = 21,
    SE_GAME_START = 22,    /* START pressed in Game Play */
    SE_SPIN_CYCLE = 23     /* special: cycling pitch, used by the spin-out */
};

/* Pad status kept by the BS2 pad code at 0x814AF60C (DEMOPadStatus-like). */
typedef struct MenuPad {
    u16 button;            /* 0x00 PADStatus copy */
    s8 stickX, stickY;
    s8 substickX, substickY;
    u8 triggerL, triggerR;
    u8 analogA, analogB;
    s8 err;
    u8 pad0B;
    u16 buttonDown;        /* 0x0C newly pressed PAD_BUTTON_* */
    u16 buttonUp;          /* 0x0E newly released */
    u16 dirs;              /* 0x10 held directions (DIR_*) */
    u16 dirsNew;           /* 0x12 new directions, with key repeat */
    u16 dirsReleased;      /* 0x14 */
} MenuPad;

/* Directions synthesised from the D-pad / stick by the pad code. */
#define DIR_UP    0x1000
#define DIR_DOWN  0x2000
#define DIR_LEFT  0x4000
#define DIR_RIGHT 0x8000
#define PAD_A     0x0100
#define PAD_B     0x0200
#define PAD_START 0x1000

extern MenuPad gMenuPad;   /* 0x814AF60C */

/* ------------------------------------------------------------------ */
/* Fader (0x10 bytes): an alpha ramp used for every fading 2D element. */
/* Init 0x8130AAEC, step 0x8130A87C, alpha 0x8130A964, draw 0x8130A9E8 */
/* ------------------------------------------------------------------ */
typedef struct Fader {
    s16 frame;             /* 0x0 0 .. delay + length */
    s16 length;            /* 0x2 ramp length in frames (default 20) */
    s16 delay;             /* 0x4 frames before the ramp starts */
    u16 ticks;             /* 0x6 frames since reset */
    u8 pad8;
    u8 maxAlpha;           /* 0x9 (default 255) */
    s16 slide;             /* 0xA vertical slide distance while fading */
    u32 id;                /* 0xC string index or pane tag */
} Fader;

enum { FADE_IN = 0, FADE_OUT = 1, FADE_HALF = 2 };

void Fader_Reset(Fader *f);                                  /* 0x8130A86C */
void Fader_Step(Fader *f, u8 mode);                          /* 0x8130A87C */
void Fader_GetAlpha(Fader *f, s16 *alpha, s32 *slide);       /* 0x8130A964 */
u8 Fader_DrawString(Fader *f, void *sth, void *glh, GXColor *c); /* 0x8130A9E8 */
void Fader_Init(Fader *f);                                   /* 0x8130AAEC */

/* GLH0 layout file (the fields the menu reads directly). */
typedef struct GlhAnchor {     /* 0x10 bytes, table at header+0x4 */
    u32 tag;               /* 0x0 FourCC */
    u16 y;                 /* 0x4 12.4 fixed point */
    u16 x;                 /* 0x6 ? */
    u8 pad8[8];
} GlhAnchor;

typedef struct GlhPane {       /* 0x30 bytes, table at header+0xC */
    u32 tag;               /* 0x00 FourCC */
    u16 x, y;              /* 0x04 centre */
    u8 pad8[8];
    u16 w, h;              /* 0x10 size */
    u8 pad14[0x30 - 0x14];
} GlhPane;

typedef struct GlhHeader {
    u32 magic;             /* 0x00 'GLH0' */
    u32 anchorOfs;         /* 0x04 */
    u32 pad8;
    u32 paneOfs;           /* 0x0C */
    u16 anchorCount;       /* 0x10 */
    u16 pad12;
    u16 paneCount;         /* 0x14 */
} GlhHeader;

/* Per-face resources and fader indices */
enum { SUBFADE_OPTION = 0, SUBFADE_CARD = 1, SUBFADE_CLOCK = 2, SUBFADE_GAME = 3 };

/* ------------------------------------------------------------------ */
/* Menu work area (0x814B5EA0, 0x4BC bytes); pointer at 0x814AD3F0.    */
/* Defaults are set by Menu_SetDefaults (0x81311464).                  */
/* ------------------------------------------------------------------ */
typedef struct FxTextLine {
    f32 x, y;              /* glyph cube origin of a mosaic text line */
    f32 scale;
    f32 spacing;
} FxTextLine;

typedef struct MenuWork {
    f32 unk00;             /* 0x000 0.0 */
    f32 fxDepth;           /* 0x004 5.0  (mosaic particle depth step) */
    f32 zoomDepth;         /* 0x008 -70: cube moves zoomDepth*(1-cos) when zooming in */
    s32 state;             /* 0x00C copy of the current MenuState */
    s16 wobbleFrame;       /* 0x010 +7 per frame */
    s16 rotX;              /* 0x012 cube pitch, +-0x4000 at the top/bottom face */
    s16 rotY;              /* 0x014 cube yaw,   +-0x4000 at the left/right face */
    s16 rotStep;           /* 0x016 750 per frame (22 frames per quarter turn) */
    s16 zoom;              /* 0x018 0 (face) .. 0x7FFF (inside the submenu) */
    s16 wobbleSpeed[3];    /* 0x01A rx, ry, rz angular speeds (50, 25, 25) */
    s16 wobbleAmp[3];      /* 0x020 rx, ry, rz amplitudes (350, 1000, 1000) */
    s16 wobbleSpeedTX;     /* 0x026 25 */
    s16 wobbleSpeedTY;     /* 0x028 50 */
    s16 wobbleAmpTX;       /* 0x02A 15 */
    s16 wobbleAmpTY;       /* 0x02C 6 */
    s16 wobblePhaseRY;     /* 0x02E -15000 */
    s16 wobblePhaseTX;     /* 0x030 -0x4000 */
    u16 inputAlways;       /* 0x032 debug: accept input while the cube moves */
    FxTextLine fxTitle[3]; /* 0x034 mosaic title lines (x, y, scale, spacing) */
    s32 fxLineOfs[6];      /* 0x064 (-2,-2,-2,-2,-1,-1) */
    FxTextLine fxItem[2];  /* 0x07C mosaic item lines */
    f32 fxCursorX;         /* 0x09C -8 */
    f32 fxCursorY;         /* 0x0A0 -8 */
    f32 fxCursorScale;     /* 0x0A4 0.8 */
    s16 fxFadeIn;          /* 0x0A8 10 */
    s16 fxFadeOut;         /* 0x0AA 10 */
    f32 fxCubeScale;       /* 0x0AC 1.0 */
    s32 fxB0;              /* 0x0B0 100 */
    f32 fxB4, fxB8, fxBC, fxC0, fxC4; /* 0x0B4 2, 5, 30, 3, 0.45 (particle flight) */
    s32 fxC8;              /* 0x0C8 120 */
    f32 fxCC;              /* 0x0CC 0.4 */
    f32 fxD0;              /* 0x0D0 2.0 */
    s16 fxD4;              /* 0x0D4 10 */
    s16 fxReadyCount;      /* 0x0D6 58: frames until a mosaic screen accepts input */
    f32 fxReadyRatio;      /* 0x0D8 0.9 */
    s16 fxDC, fxDE;        /* 0x0DC 200, 200 */
    f32 fxE0, fxE4, fxE8;  /* 0x0E0 200, 200, 200 */
    s32 fxBlinkPeriod;     /* 0x0EC 25 */
    f32 fxF0;              /* 0x0F0 10 */
    f32 fxSel[8];          /* 0x0F4 selection bar ends (x0,y0,x1,y1) x2 */
    f32 fxSel2[4];         /* 0x114 (-50, 13, 50, 13) */
    s16 fx124;             /* 0x124 30 */
    s16 fx126;             /* 0x126 10 */
    s16 fxCountOverride;   /* 0x128 -1 */
    s16 gameColor[6];      /* 0x12A Game Play cube colours: light (130,50,160) dark (50,20,70) interleaved */
    s16 optionColor[18];   /* 0x136 Options cube colours (3 pairs) */
    s16 clockColor[18];    /* 0x15A Calendar cube colours (3 pairs) */
    u16 fx17E;             /* 0x17E 1 */
    s16 zoomInStep;        /* 0x180 0x730 (18 frames to zoom in) */
    s16 zoomOutStep;       /* 0x182 0x578 (24 frames to zoom out) */
    s16 bgMinAlpha;        /* 0x184 0: background alpha when fully zoomed in */
    f32 zoomSpin;          /* 0x188 0: roll while zooming */
    f32 lightAttn;         /* 0x18C 500  backdrop spot light distance attenuation */
    f32 lightAttnK;        /* 0x190 0.375 */
    Vec lightPos;          /* 0x194 (296, 224, 474) */
    f32 lightCutoff;       /* 0x1A0 36 degrees */
    s16 guideColorA[3];    /* 0x1A4 (0, 200, 140) A button glyph */
    s16 guideColorB[3];    /* 0x1AA (210, 0, 70)  B button glyph */
    s16 subFadeDelay[3];   /* 0x1B0 (0, 20, 70) delays of the 3 submenu faders */
    Mtx cubeMtx;           /* 0x1B8 menu cube model matrix */
    J3DModel iconModel;    /* 0x1E8 small cube for the submenu icons (res 178) */
    void *guideSth[6];     /* 0x220 button guide strings for each language (res 116..121) */
    void *digitTex[10];    /* 0x238 digit textures 0..9 (res 91..100) */
    void *weekdayTex[7];   /* 0x260 weekday name textures (current language) */
    Fader guideIcon[6];    /* 0x27C button glyph groups (see Menu_UpdateGuide) */
    Fader guideText[8];    /* 0x2DC button guide labels */
    Fader subFade[4][3];   /* 0x35C submenu fade-ins: [SUBFADE_*][0..2] */
    Fader discBanner;      /* 0x41C Game face: disc with banner */
    Fader discNone;        /* 0x42C Game face: "no disc" */
    Fader discUnknown;     /* 0x43C Game face: disc without banner */
    Fader errorMsg;        /* 0x44C fatal disc error text (string 4) */
    Fader langGuide[6];    /* 0x45C cross-fade of the guide text between languages */
} MenuWork;                /* 0x4BC bytes */

/* Button guide label indices (STH0 res 116..121) */
enum {
    GUIDE_MENU_SELECT = 0, GUIDE_CANCEL = 1, GUIDE_CONFIRM = 2,
    GUIDE_SELECT = 4, GUIDE_CONFIRM2 = 6, GUIDE_CANCEL2 = 8,
    GUIDE_FINISH = 11, GUIDE_CHANGE = 13
};

/* ------------------------------------------------------------------ */
/* Disc / boot state reported by the DVD code (0x813020EC).            */
/* ------------------------------------------------------------------ */
enum {
    DISC_READY = 0x10,     /* "Press START to begin!" */
    DISC_READING = 0x11,   /* "Reading disc..." */
    DISC_NONE = 0x12,      /* "Please insert a NINTENDO GAMECUBE DISC." */
    DISC_COVER_OPEN = 0x13,
    DISC_WRONG = 0x16,     /* "The disc could not be read." */
    DISC_FATAL = 0x17,     /* "An error has occurred." */
    DISC_RETRY = 0x18      /* "The disc could not be read." */
};

/* ------------------------------------------------------------------ */
/* Per-submenu icon parameters                                          */
/* ------------------------------------------------------------------ */

/* Game Play icon (0x814B6360, Game_GetParams 0x813157B0) */
typedef struct GameParams {
    s32 tint[4];           /* 0x00 (0, -90, 20, 255): TEV colour added to the icon cubes */
    s32 ringRadius;        /* 0x10 75 */
    f32 cubeScale;         /* 0x14 0.65 */
    s16 pad18;
    s16 shrinkSpeed;       /* 0x1A 5 */
    s16 fadeSpeed;         /* 0x1C 10 */
} GameParams;

/* Memory Card icon / UI (0x814B6380, Card_GetParams 0x81315934) */
typedef struct CardParams {
    s32 tint[4];           /* 0x00 (-45, -40, 50, 255) */
    f32 cubeScale;         /* 0x10 0.7 */
    s16 rows;              /* 0x14 5 */
    s16 cols;              /* 0x16 8 */
    s16 colorA[2][3];      /* 0x18 interleaved colour table (see Card_SetParams) */
    s16 colorTbl[0x18];    /* 0x18..0x47 */
    f32 iconZoom;          /* 0x48 2.0 */
    f32 iconZoomSel;       /* 0x4C 1.3 */
    s16 white[3];          /* 0x50 (255, 255, 255) */
    s16 cream[3];          /* 0x56 (240, 230, 0) */
    s16 collapseFade;      /* 0x5C 70 */
    s16 collapseSpeed;     /* 0x5E 5 */
    s16 fadeOutSpeed;      /* 0x60 13 */
    f32 f64_;              /* 0x64 10 */
    f32 f68_;              /* 0x68 5 */
    s16 s6C;               /* 0x6C 1 */
    s16 s6E;               /* 0x6E 0x80 */
    f32 f70;               /* 0x70 320 */
} CardParams;

/* Options icon / text (0x814D9F00, Option_GetParams 0x813217E8) */
typedef struct OptionParams {
    s32 tint[4];           /* 0x00 (-30, 20, -50, 255) */
    s16 textColor[3];      /* 0x10 (0, 255, 120) */
    s16 pad16;
    f32 f18;               /* 0x18 2.0 */
    s16 pad1C;
    s16 s1E;               /* 0x1E 5 */
    s16 s20;               /* 0x20 12 */
    s16 pad22;
    f32 itemOfs[3];        /* 0x24 (-15, -4, 6): vertical offsets of item texts */
    f32 langOfs;           /* 0x30 8 */
} OptionParams;

/* Calendar icon / text (0x814D9F38, Clock_GetParams 0x813218C0) */
typedef struct ClockParams {
    s16 tint[4];           /* 0x00 (40, 20, -50, 255) */
    s16 tint2[4];          /* 0x08 (80, 0, -60, 255) */
    s16 textColor[3];      /* 0x10 (255, 255, 0) */
    s16 handLen[6];        /* 0x16 (0xFB0,0x1240,0x1740, 0xFF0,0x1240,0x15D0) */
    s16 s22;               /* 0x22 6 */
    s16 s24;               /* 0x24 14 */
} ClockParams;

/* Calendar edit buffer (0x814DE600) */
typedef struct ClockEdit {
    s32 value[3];          /* 0x00 date: year, month, day   time: hour, minute, second */
    OSCalendarTime now;    /* 0x0C (sec, min, hour, mday, mon, year, wday, ...) */
} ClockEdit;

/* ------------------------------------------------------------------ */
/* Memory card                                                          */
/* ------------------------------------------------------------------ */
#define CARD_MAX_FILES 127

/* Slot status kept by the card thread (0x814B7280, 2 x 0x14) */
typedef struct CardSlot {
    u8 busy;               /* 0x00 */
    u8 dirty;              /* 0x01 */
    s16 state;             /* 0x02 CARD_SLOT_* */
    s32 result;            /* 0x04 last CARD result */
    u32 sectorSize;        /* 0x08 */
    u16 pad0C;
    u16 freeFiles;         /* 0x0E */
    u16 freeBlocks;        /* 0x10 */
    u16 totalBlocks;       /* 0x12 */
} CardSlot;

enum {
    CARD_SLOT_EMPTY = 0,
    CARD_SLOT_READY = 4,
    CARD_SLOT_WRONG = 5,       /* unusable object / wrong device */
    CARD_SLOT_NEED_FORMAT = 6, /* 6/7: broken, 8/9: other encoding (7/9 = asked) */
    CARD_SLOT_NEED_FORMAT_ASKED = 7,
    CARD_SLOT_ENCODING = 8,
    CARD_SLOT_ENCODING_ASKED = 9
};

/* One directory entry as seen by the UI (0xC bytes, 127 per slot) */
typedef struct CardFile {
    u16 used;              /* 0x0 nonzero if the entry holds a file */
    u16 blocks;            /* 0x2 */
    u8 canCopy;            /* 0x4 */
    u8 canMove;            /* 0x5 */
    u16 opened;            /* 0x6 file is in use / permission bits */
    u32 pad8;
} CardFile;

/* Card errors shown by the UI (index into the STH0 card table) */
enum {
    CARD_UI_OK = -0x15,
    CARD_UI_ENCODING = -0x16,
    CARD_UI_NO_CARD = -0x17,
    CARD_UI_SAME_FILE = -0x18,
    CARD_UI_NO_SPACE = -0x19,
    CARD_UI_NOT_ALLOWED = -0x1A,
    CARD_UI_DIFFERENT_SIZE = -0x1B,
    CARD_UI_BROKEN = -0x1C
};

/* ------------------------------------------------------------------ */
/* Icon cubes (0x78 byte J3DModelInstance + per-cube motion records)   */
/* ------------------------------------------------------------------ */
typedef struct CardIconCube {  /* 0x1A bytes, 0x814DB040 */
    s16 pos[3];            /* 0x00 current position */
    s16 from[3];           /* 0x06 */
    s16 start;             /* 0x0C frame the cube starts moving */
    s16 to[3];             /* 0x0E grid position (6 x 6, step 20) */
    s16 end;               /* 0x14 start + 20 */
    s16 spin;              /* 0x16 roll angle while flipping */
    u8 flag18;
    u8 flag19;             /* 0x80 */
} CardIconCube;

typedef struct GameIconCube {  /* 0x14 bytes, 0x814DBBA0 */
    s16 start;             /* 0x00 */
    s16 end;               /* 0x02 */
    s16 angle;             /* 0x04 position on the ring */
    s16 radius;            /* 0x06 */
    s16 pad8, padA;
    f32 x, y;              /* 0x0C */
} GameIconCube;

/* ------------------------------------------------------------------ */
/* Functions                                                            */
/* ------------------------------------------------------------------ */

/* menu.c */
void Menu_LoadLangRes(void);
void Menu_Init(BOOL first);
s32 Menu_Update(void);
MtxPtr Menu_GetCubeMtx(void);
MenuWork *Menu_Get(void);
void Menu_SetGuideLang(s16 lang);

/* menu_draw.c */
void Menu_DrawSpinOutFace(Mtx m, u8 alpha);
void Menu_DrawFaces(u8 alpha);
BOOL Menu_CardFadeDone(void);
void Menu_DrawSubmenus(void);
void Menu_DrawGuide(u8 alpha);
void Menu_Draw2D(u8 alpha);

/* game.c */
GameParams *Game_GetParams(void);
void Game_InitParams(BOOL first);
void Game_LoadLangRes(void);
void Game_Init(BOOL first);
s32 Game_Update(void);
void Game_Enter(void);
BOOL Game_IsFatal(void);
void Game_Draw(u8 fxAlpha, u8 bgAlpha, u8 textAlpha);

/* options.c */
void Option_LoadLangRes(void);
void Option_Init(BOOL first);
s32 Option_Update(void);
BOOL Option_IsTop(void);
s16 Option_GetLangRow(void);
s16 Option_GetLang(void);
void Option_Draw(u8 fxAlpha, u8 bgAlpha, u8 textAlpha);
void Option_InitParams(BOOL first);

/* calendar.c */
void Clock_Commit(void);
void Clock_LoadLangRes(void);
void Clock_Init(BOOL first);
s32 Clock_Update(void);
BOOL Clock_IsTop(void);
void Clock_Draw(u8 fxAlpha, u8 bgAlpha, u8 textAlpha);
void Clock_InitParams(BOOL first);

/* memcard*.c */
CardParams *Card_GetParams(void);
void Card_InitParams(BOOL first);
void Card_LoadLangRes(void);
void Card_Init(BOOL first);
s32 Card_Update(void);
void Card_Enter(void);
void Card_Draw(u8 a0, u8 a1, u8 a2);
BOOL Card_IsBrowsing(void);
void *CardThread_GetFiles(void);
CardSlot *CardThread_GetSlots(void);
void CardThread_Poll(void);
void CardThread_Init(void);

/* menu_icon_*.c */
OptionParams *Option_GetParams(void);
ClockParams *Clock_GetParams(void);
void CardIcon_Init(J3DModel *m, BOOL first);
void GameIcon_Init(J3DModel *m, BOOL first);
void OptionIcon_Init(J3DModel *m, BOOL first);
void ClockIcon_Init(J3DModel *m, BOOL first);
void CardIcon_Draw(void);
void GameIcon_Draw(void);
void OptionIcon_Draw(void);
void ClockIcon_Draw(void);
void CardIcon_UpdateFace(void);   /* 0x81322650 */
void CardIcon_UpdateEnter(void);  /* 0x81322EE8 */
void CardIcon_UpdateHide(void);   /* 0x81322D58 */
void CardIcon_CalcMtx(void);      /* 0x81322A18 */
void CardIcon_ResetFiles(void);   /* 0x813207C8 */
void CardIcon_InitGrid(void);     /* 0x813219F4 */
BOOL CardIcon_IsInner(s16 i);     /* 0x81321B98 */
s16 CardIcon_Diagonal(s16 i);     /* 0x81321BE4 */
void CardIcon_UpdateFly(void);    /* 0x81321C68 */
void CardIcon_FadeInner(BOOL in); /* 0x81321F60 */
void CardIcon_SpinY(s32 t);       /* 0x81322434 */
BOOL CardIcon_IsSettled(void);    /* 0x813230E8 */
void GameIcon_InitRing(void);     /* 0x813231F8 */
void GameIcon_UpdateFace(void);   /* 0x813233C8 */
void GameIcon_UpdateEnter(void);  /* 0x813239E4 */
void GameIcon_UpdateHide(void);   /* 0x81323868 */
void GameIcon_CalcMtx(void);      /* 0x813235D4 */
void OptionIcon_UpdateFace(void); /* 0x8132554C */
void OptionIcon_UpdateEnter(void);/* 0x81325DB0 */
void OptionIcon_UpdateHide(void); /* 0x81325C30 */
void OptionIcon_CalcMtx(void);    /* 0x81325960 */
void ClockIcon_UpdateFace(void);  /* 0x813262E4 */
void ClockIcon_UpdateEnter(void); /* 0x81326FDC */
void ClockIcon_UpdateHide(void);  /* 0x81326DE0 */
void ClockIcon_CalcMtx(void);     /* 0x81326808 */
void CardUi_Reset(void);          /* 0x8131783C */
void Fx_InitCubeModel(void);      /* 0x81332FB8 */
BOOL GameIcon_IsSettled(void);
BOOL OptionIcon_IsSettled(void);
BOOL ClockIcon_IsSettled(void);

/* menu_fx*.c */
void Fx_Init(void *keys, void *curves);
void Fx_Clear(void);
u32 Fx_IsReady(s32 margin);
void Fx_Update(void);
void Fx_Draw(u8 alpha);
void FxOption_Update(s32 sub, s16 row, s16 offset, s16 sound);
void FxClock_Update(s32 sub, s32 row, s32 dateField, s32 timeField, ClockEdit *e);
void FxGame_Update(s32 starting, s32 discMsg);


/* ------------------------------------------------------------------ */
/* Helpers outside the menu range (0x81301000..0x8130C2xx, SDK) as the  */
/* menu calls them. Names are working names from names_menu.txt.        */
/* ------------------------------------------------------------------ */
/* JUTTexture/BTI style texture header passed to Glh_DrawPaneTex */
typedef struct TexHeader {
    u8 format;             /* 0x00 GX_TF_* */
    u8 alpha;              /* 0x01 */
    u16 width;             /* 0x02 */
    u16 height;            /* 0x04 */
    u8 wrapS, wrapT;       /* 0x06 */
    u8 indexed;            /* 0x08 */
    u8 tlutFormat;         /* 0x09 */
    u16 tlutCount;         /* 0x0A */
    u32 tlutOffset;        /* 0x0C */
    u8 mipmap;             /* 0x10 */
    u8 edgeLod;            /* 0x11 */
    u8 biasClamp;          /* 0x12 */
    u8 maxAniso;           /* 0x13 */
    u8 minFilter;          /* 0x14 */
    u8 magFilter;          /* 0x15 */
    s8 minLod, maxLod;     /* 0x16 */
    u8 lodCount;           /* 0x18 */
    u8 pad19;
    s16 lodBias;           /* 0x1A */
    s32 imageOffset;       /* 0x1C relative to the header */
} TexHeader;

MtxPtr Camera_GetView(void);                                  /* 0x81302F88 */
void MtxFromSRT(const SRT *srt, Mtx out);                     /* 0x8130764C */
BOOL MtxInverse33(Mtx in, Mtx out);                           /* 0x81307820 */
void GX_Setup2D(s32 a, s32 b, s32 c);                         /* 0x8130180C */
void Screen_Dim(u8 alpha);                                    /* 0x81301BC8 black quad */
void GX_Setup2DView(void);                                    /* 0x813030B0 */
void GX_Setup2DTex(void);                                     /* 0x8130963C */
void GX_Setup2DText(void);                                    /* 0x81309668 */
void Txh_SetBank(void *txh);                                  /* 0x81309620 */
void Glh_DrawPaneRect(u16 pane, void *glh, GXColor *c, s32 x, s32 y, s32 w, s32 h); /* 0x81309948 */
void Glh_DrawFrame(u32 tag, void *glh, GXColor *c);           /* 0x81309C3C 9-slice frame */
void Glh_DrawAll(void *glh, GXColor *c);                      /* 0x8130A158 */
void Glh_DrawPane(u32 tag, void *glh, GXColor *c);            /* 0x8130A1F8 */
#ifndef BS2_GLH_C
/* glh.c takes s16; the menu's prototype passes s32 */
void Glh_DrawPaneOfs(u32 tag, void *glh, GXColor *c, s32 dx, s32 dy); /* 0x8130A284 (12.4) */
#endif
void Glh_DrawPaneTex(u32 tag, void *glh, GXColor *c, void *tex); /* 0x8130A318 */
void Glh_DrawString(u16 idx, void *sth, void *glh, GXColor *c, GXColor *glow, s32 dx, s32 dy); /* 0x8130A39C */
void Glh_DrawStringOfs(u16 idx, void *sth, void *glh, GXColor *c, s32 dx, s32 dy); /* 0x8130A5A4 */
void Glh_DrawText(u16 idx, void *sth, void *glh, GXColor *c);    /* 0x8130A5D0 */
void Glh_DrawBannerLine(u32 tag, void *glh, GXColor *c, const void *s, s32 max); /* 0x8130A75C */
void Glh_DrawBannerBox(u32 tag, void *glh, GXColor *c, const void *s, s32 max);  /* 0x8130A7E4 */
void Backdrop_Draw(Mtx mv, u8 alpha);                         /* 0x8130B244 */
u16 Lang_GetCode(void);                                       /* 0x8130B474 language + 4 */
s16 Lang_Get(void);                                           /* 0x8130B47C 0 En 1 De 2 Fr 3 Es 4 It 5 Nl */
void Lang_Set(s32 lang);                                      /* 0x8130B488 */
void *ResGet(s32 id);                                         /* 0x8130BDA0 */
BOOL Boot_SramWasLost(void);                                  /* 0x813020C0 */
s32 Disc_GetState(void);                                      /* 0x813020EC DISC_* */
void Disc_FadeOutStart(void);                                 /* 0x81302100 */
u8 Disc_GetFadeAlpha(void);                                   /* 0x81302130 */
void Disc_StartGame(void);                                    /* 0x81302200 */
BOOL Disc_HasBanner(void);                                    /* 0x813025E8 */
u8 *Disc_GetBanner(void);                                     /* 0x8130264C */
u32 Disc_GetBannerType(void);                                 /* 0x81302654 0 BNR1, 1 BNR2 */
BOOL Pad_IsRepeat(void);                                      /* 0x81302DE0 */
void Sound_Play(s32 se);                                      /* 0x8135AA80 */
void Sound_StopAmbient(void);                                 /* 0x8135ABE0 */
void Sound_StartAmbient(s32 n);                               /* 0x8135AC20 */
void Sound_SetOutputMode(s32 stereo);                         /* 0x8135B160 */
void __VISetAdjustingValues(s16 x, s16 y);                    /* 0x8136A504 */

#endif
