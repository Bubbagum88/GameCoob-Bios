// Non-matching: functionally equivalent decompilation
//
// Main menu drawing (PAL 1.0 BS2, 0x81312F88..0x81315724): the five GLH0
// face layouts on the cube, the submenus, the button guide and the
// fatal error overlay.
//
// Faces sit on a cube of half-size 144 (the layouts are 288 x 288 with
// y pointing down, hence the scale (1,-1,1) in every SRT). A side face is
// drawn once the cube is turned at least 0x2E3A (65 degrees) toward it and
// fades in over the last 5000 units (27.5 degrees) of the turn:
//     alpha = 255 - (0x4000 - |rot|) * 255 / 5000
// The front face fades out the same way as soon as the cube turns.
//
// "z" panes (zitl, zodi, ...) are drop shadows drawn in 0x282828 before
// the matching white pane.

#include "bs2/menu.h"
#include "bs2/scene.h"
#include "bs2/res.h"
#include <dolphin/os.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <string.h>

#define FACE_HALF       144.0f
#define FACE_SHOW_ROT   0x2E3A
#define FACE_FADE_RANGE 5000

/* 0x8137EC34: SRT of each face layout relative to the cube. The missing
   translation component of each entry is filled in by the code. */
extern const SRT sFaceSRT[6];
enum { FACE_TOP = 0, FACE_WORK = 1, FACE_CARD = 2, FACE_GAME = 3, FACE_CLOCK = 4, FACE_OPTION = 5 };

/* constant colours (0x814ADE84..0x814ADED0) */
static const GXColor sFaceWhite  = { 255, 255, 255, 255 };
static const GXColor sFaceShadow = {  40,  40,  40, 255 };
static const GXColor sStickBack  = { 150, 150, 150, 255 };

extern MenuWork gMenuWork;
extern MenuWork *gMenu;
extern void *gTopFaceGlh, *gGameFaceGlh, *gCardFaceGlh, *gOptionFaceGlh, *gClockFaceGlh;
extern void *gGuideGlh, *gDialogGlh, *gDialogSth;
extern s16 gSlotGlow[2];
extern s32 gSlotFill[2];
extern void *gSoundModeTex[2];
extern s16 gGuideAlpha, gGuideBlink;

/* view * base * SRT -> GX_PNMTX0 (written out at every call site) */
static void Menu_LoadFaceMtx(MtxPtr base, const SRT *srt) {
    Mtx tmp, m;

    PSMTXConcat(Camera_GetView(), base, tmp);
    MtxFromSRT(srt, m);
    PSMTXConcat(tmp, m, m);
    GXLoadPosMtxImm(m, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

static u8 Face_Alpha(s16 rot) {
    if (rot < 0) rot = -rot;
    return (u8)(255 - (0x4000 - rot) * 255 / FACE_FADE_RANGE);
}

/* 0x81312F88: the front face on the spinning cube during the spin-out
   (Scene state 4). The alpha is applied twice (alpha^2 / 255). */
void Menu_DrawSpinOutFace(MtxPtr cube, u8 alpha) {
    SRT srt = sFaceSRT[FACE_TOP];
    GXColor c = sFaceWhite;

    Menu_LoadFaceMtx(cube, &srt);
    c.a = alpha;
    c.a = (u8)(c.a * alpha / 255);
    GX_Setup2DTex();
    Glh_DrawAll(gTopFaceGlh, &c);
}

static void Menu_DrawTopFace(u8 alpha) {
    SRT srt = sFaceSRT[FACE_TOP];
    GXColor c = sFaceWhite;

    Menu_LoadFaceMtx(gMenuWork.cubeMtx, &srt);
    c.a = alpha;
    c.a = (u8)(c.a * alpha / 255);
    GX_Setup2DTex();
    Glh_DrawAll(gTopFaceGlh, &c);
}

/* 0x813130CC: disc banner on the Game face: image, short name, maker. */
static void Menu_DrawBannerInfo(void *glh, u8 alpha) {
    GXColor white = sFaceWhite;
    GXColor shadow = sFaceShadow;
    TexHeader tex;
    s16 a;
    u8 *bnr;
    u32 ofs;

    Fader_GetAlpha(&gMenuWork.discBanner, &a, NULL);
    if (a == 0) return;

    bnr = Disc_GetBanner();
    memset(&tex, 0, sizeof(tex));
    tex.format = GX_TF_RGB5A3;
    tex.width = 96;
    tex.height = 32;
    tex.minFilter = GX_LINEAR;
    tex.magFilter = GX_LINEAR;
    tex.imageOffset = (bnr + 0x20) - (u8 *)&tex;
    white.a = shadow.a = (u8)(alpha * a / 255);

    GX_Setup2D(1, 0, 1, 0, white.a);
    Glh_DrawPaneTex('bana', glh, &white, &tex);
    GX_Setup2DText();
    if (Disc_GetBannerType() == 0) {
        ofs = 0;
    } else if (Disc_GetBannerType() == 1) {
        ofs = Lang_Get() * 0x140;
    } else {
        return;
    }
    Glh_DrawBannerLine('zame', glh, &shadow, bnr + ofs + 0x1820, 0x20);  /* short name */
    Glh_DrawBannerLine('game', glh, &white,  bnr + ofs + 0x1820, 0x20);
    Glh_DrawBannerLine('zakr', glh, &shadow, bnr + ofs + 0x1840, 0x20);  /* short maker */
    Glh_DrawBannerLine('makr', glh, &white,  bnr + ofs + 0x1840, 0x20);
}

/* 0x81313334: Game Play face ("nodi" no disc / "qust" unknown disc). */
static void Menu_DrawGameFace(void *glh, const SRT *srt, u8 alpha) {
    GXColor white = sFaceWhite;
    GXColor shadow = sFaceShadow;
    GXColor w2, s2;
    s16 a;

    Menu_LoadFaceMtx(gMenuWork.cubeMtx, srt);
    white.a = alpha;
    white.a = (u8)(white.a * alpha / 255);
    shadow.a = (u8)(shadow.a * alpha / 255);
    Glh_DrawFrame('fram', glh, &white);
    GX_Setup2DTex();
    Glh_DrawPane('titl', glh, &white);

    Fader_GetAlpha(&gMenuWork.discNone, &a, NULL);
    if (a > 0) {
        w2 = white; s2 = shadow;
        w2.a = (u8)(w2.a * a / 255);
        s2.a = (u8)(s2.a * a / 255);
        Glh_DrawPane('zodi', glh, &s2);
        Glh_DrawPane('nodi', glh, &w2);
    }
    Fader_GetAlpha(&gMenuWork.discUnknown, &a, NULL);
    if (a > 0) {
        w2 = white; s2 = shadow;
        w2.a = (u8)(w2.a * a / 255);
        s2.a = (u8)(s2.a * a / 255);
        Glh_DrawPane('zust', glh, &s2);
        Glh_DrawPane('qust', glh, &w2);
    }
    Menu_DrawBannerInfo(glh, alpha);
}

/* 0x8131358C: one memory card slot on the Card face: label, glowing
   frame and a fill bar that rises with the card's used space. */
static void Menu_DrawSlot(u8 slot, void *glh, u8 alpha) {
    GXColor c = sFaceWhite;
    GlhHeader *card = gCardFaceGlh;
    GlhPane *pane;
    u32 tag;
    u8 fill;
    u16 i;
    u32 h;

    c.a = alpha;
    GX_Setup2DTex();
    Glh_DrawPane('texa' + slot, glh, &c);

    c.a = (u8)(alpha * gSlotGlow[slot] / 255);
    tag = slot << 8;                                /* 'mca?' / 'mcb?' */
    Glh_DrawFrame('mca1' + tag, gCardFaceGlh, &c);

    fill = (u8)(gSlotFill[slot] * (gSlotGlow[slot] - 60) / 195);
    if (fill == 0) return;
    pane = (GlhPane *)((u8 *)card + card->paneOfs);
    for (i = 0; i < card->paneCount; i++, pane++) {
        if (pane->tag == 'mca2' + tag) {
            h = fill * pane->h / 255;               /* bottom-anchored bar */
            Glh_DrawPaneRect(i, card, &c,
                             pane->x - pane->w / 2,
                             pane->y - pane->h / 2 + pane->h - h,
                             pane->w, h);
        }
    }
}

static void *Digit(s32 n) {
    return gMenuWork.digitTex[n];
}

/* 0x81313708: Calendar face, today's date and the current time. */
static void Menu_DrawClockFace(void *glh, const SRT *srt, u8 alpha) {
    GXColor white = sFaceWhite;
    GXColor shadow = sFaceShadow;
    OSCalendarTime t;
    s32 pass;
    GXColor *c;
    u32 z;

    memset(&t, 0, sizeof(t));
    OSTicksToCalendarTime(OSGetTime(), &t);
    Menu_LoadFaceMtx(gMenuWork.cubeMtx, srt);
    white.a = alpha;
    white.a = (u8)(white.a * alpha / 255);
    shadow.a = (u8)(shadow.a * alpha / 255);
    Glh_DrawFrame('fram', glh, &white);
    GX_Setup2DTex();

    /* pass 0 draws the shadow panes ('z' + 3 letters), pass 1 the white
       ones. The first year digit is always "2". */
    for (pass = 0; pass < 2; pass++) {
        c = pass ? &white : &shadow;
        z = pass ? 0 : 1;
        Glh_DrawPane(z ? 'zolo' : 'colo', glh, c);
        Glh_DrawPane(z ? 'zlas' : 'slas', glh, c);
        Glh_DrawPane(z ? 'zare' : 'pare', glh, c);
        Glh_DrawPane(z ? 'zitl' : 'titl', glh, c);
        Glh_DrawPaneTex(z ? 'zal1' : 'cal1', glh, c, Digit(2));
        Glh_DrawPaneTex(z ? 'zal2' : 'cal2', glh, c, Digit(t.year / 100 % 10));
        Glh_DrawPaneTex(z ? 'zal3' : 'cal3', glh, c, Digit(t.year / 10 % 10));
        Glh_DrawPaneTex(z ? 'zal4' : 'cal4', glh, c, Digit(t.year % 10));
        Glh_DrawPaneTex(z ? 'zal5' : 'cal5', glh, c, Digit((t.mon + 1) / 10 % 10));
        Glh_DrawPaneTex(z ? 'zal6' : 'cal6', glh, c, Digit((t.mon + 1) % 10));
        Glh_DrawPaneTex(z ? 'zal7' : 'cal7', glh, c, Digit(t.mday / 10));
        Glh_DrawPaneTex(z ? 'zal8' : 'cal8', glh, c, Digit(t.mday % 10));
        Glh_DrawPaneTex(z ? 'zeek' : 'week', glh, c, gMenuWork.weekdayTex[t.wday]);
        Glh_DrawPaneTex(z ? 'zim1' : 'tim1', glh, c, Digit(t.hour / 10));
        Glh_DrawPaneTex(z ? 'zim2' : 'tim2', glh, c, Digit(t.hour % 10));
        Glh_DrawPaneTex(z ? 'zim3' : 'tim3', glh, c, Digit(t.min / 10));
        Glh_DrawPaneTex(z ? 'zim4' : 'tim4', glh, c, Digit(t.min % 10));
        Glh_DrawPaneTex(z ? 'zim5' : 'tim5', glh, c, Digit(t.sec / 10));
        Glh_DrawPaneTex(z ? 'zim6' : 'tim6', glh, c, Digit(t.sec % 10));
    }
    GX_Setup2DText();
}

/* 0x81313F20: Options face: sound mode, screen offset with an arrow on
   the side it is shifted to, and the language. */
static void Menu_DrawOptionFace(void *glh, const SRT *srt, u8 alpha) {
    GXColor white = sFaceWhite;
    GXColor shadow = sFaceShadow;
    u32 stereo;
    s16 ofs, mag;
    OSSram *sram;

    stereo = OSGetSoundMode();
    sram = __OSLockSram();
    ofs = (s8)sram->displayOffsetH;
    __OSUnlockSram(FALSE);

    Menu_LoadFaceMtx(gMenuWork.cubeMtx, srt);
    white.a = (u8)(white.a * alpha / 255);
    shadow.a = (u8)(shadow.a * alpha / 255);
    Glh_DrawFrame('fram', glh, &white);
    GX_Setup2DTex();
    Glh_DrawPane('zxt1', glh, &shadow);
    Glh_DrawPane('zxt2', glh, &shadow);
    Glh_DrawPane('zitl', glh, &shadow);
    Glh_DrawPane('txt1', glh, &white);
    Glh_DrawPane('txt2', glh, &white);
    Glh_DrawPane('titl', glh, &white);
    if (ofs > 0) {
        Glh_DrawPane('zrwr', glh, &shadow);
        Glh_DrawPane('arwr', glh, &white);
    } else if (ofs < 0) {
        Glh_DrawPane('zrwl', glh, &shadow);
        Glh_DrawPane('arwl', glh, &white);
    }
    if (ofs < -32) ofs = -32;
    if (ofs > 32) ofs = 32;
    mag = ofs >= 0 ? ofs : -ofs;
    Glh_DrawPaneTex('zum1', glh, &shadow, Digit(mag / 10));
    Glh_DrawPaneTex('zum2', glh, &shadow, Digit(mag % 10));
    Glh_DrawPaneTex('num1', glh, &white, Digit(mag / 10));
    Glh_DrawPaneTex('num2', glh, &white, Digit(mag % 10));

    if (stereo) stereo = 1;
    Glh_DrawPaneTex('zond', glh, &shadow, gSoundModeTex[stereo]);
    Glh_DrawPaneTex('sond', glh, &white, gSoundModeTex[stereo]);
    Glh_DrawPane('zxt3', glh, &shadow);
    Glh_DrawPane('zang', glh, &shadow);
    Glh_DrawPane('txt3', glh, &white);
    Glh_DrawPane('lang', glh, &white);       /* language name (per-language layout) */
    GX_Setup2DText();
}

static s16 AbsClamp(s16 rot) {
    if (rot >= FACE_FADE_RANGE || rot <= -FACE_FADE_RANGE) return FACE_FADE_RANGE;
    return rot >= 0 ? rot : -rot;
}

/* 0x81314304: all faces of the cube, alpha = overall menu alpha. */
void Menu_DrawFaces(u8 alpha) {
    SRT srt = sFaceSRT[FACE_WORK];
    s16 ax = AbsClamp(gMenu->rotX);
    s16 ay = AbsClamp(gMenu->rotY);
    GXColor c;
    void *glh;
    u8 a;

    srt.tz = FACE_HALF;

    /* front face: fades with the larger of the two turns */
    if (ax > ay) {
        Menu_DrawTopFace((u8)(255 - ax * 255 / FACE_FADE_RANGE));
    } else if (ax < ay) {
        Menu_DrawTopFace((u8)(255 - ay * 255 / FACE_FADE_RANGE));
    } else if (ax == 0) {
        Menu_DrawTopFace(255);
    }

    /* bottom: Memory Card */
    if (gMenu->rotX <= -FACE_SHOW_ROT) {
        srt = sFaceSRT[FACE_CARD];
        srt.ty = -FACE_HALF;
        glh = gCardFaceGlh;
        c = sFaceWhite;
        a = (u8)(alpha * Face_Alpha(gMenu->rotX) / 255);
        Menu_LoadFaceMtx(gMenuWork.cubeMtx, &srt);
        c.a = a;
        c.a = (u8)(c.a * a / 255);
        Glh_DrawFrame('fram', glh, &c);
        GX_Setup2DTex();
        Glh_DrawPane('titl', glh, &c);
        Menu_DrawSlot(0, glh, a);
        Menu_DrawSlot(1, glh, a);
    }
    /* top: Game Play */
    if (gMenu->rotX >= FACE_SHOW_ROT) {
        srt = sFaceSRT[FACE_GAME];
        srt.ty = FACE_HALF;
        Menu_DrawGameFace(gGameFaceGlh, &srt, (u8)(alpha * Face_Alpha(gMenu->rotX) / 255));
    }
    /* right: Calendar */
    if (gMenu->rotY <= -FACE_SHOW_ROT) {
        srt = sFaceSRT[FACE_CLOCK];
        srt.tx = FACE_HALF;
        Menu_DrawClockFace(gClockFaceGlh, &srt, (u8)(alpha * Face_Alpha(gMenu->rotY) / 255));
    }
    /* left: Options */
    if (gMenu->rotY >= FACE_SHOW_ROT) {
        srt = sFaceSRT[FACE_OPTION];
        srt.tx = -FACE_HALF;
        Menu_DrawOptionFace(gOptionFaceGlh, &srt, (u8)(alpha * Face_Alpha(gMenu->rotY) / 255));
    }
}

/* 0x81314CC8: the card submenu has (almost) faded in. */
BOOL Menu_CardFadeDone(void) {
    s16 a[3];
    s32 i;

    for (i = 0; i < 3; i++) {
        Fader_GetAlpha(&gMenuWork.subFade[SUBFADE_CARD][i], &a[i], NULL);
    }
    return a[0] > 172 && a[1] > 172 && a[2] > 172;
}

/* 0x81314D70: each submenu is drawn while its first fader is non zero;
   the three fader alphas are passed as (fx/icons, backdrop, text). */
void Menu_DrawSubmenus(void) {
    s16 a[3];
    s32 i;

    for (i = 0; i < 3; i++) Fader_GetAlpha(&gMenuWork.subFade[SUBFADE_CARD][i], &a[i], NULL);
    if (a[0] > 0) Card_Draw((u8)a[0], (u8)a[1], (u8)a[2]);

    for (i = 0; i < 3; i++) Fader_GetAlpha(&gMenuWork.subFade[SUBFADE_GAME][i], &a[i], NULL);
    if (a[0] > 0) Game_Draw((u8)a[0], (u8)a[1], (u8)a[2]);

    for (i = 0; i < 3; i++) Fader_GetAlpha(&gMenuWork.subFade[SUBFADE_OPTION][i], &a[i], NULL);
    if (a[0] > 0) Option_Draw((u8)a[0], (u8)a[1], (u8)a[2]);

    for (i = 0; i < 3; i++) Fader_GetAlpha(&gMenuWork.subFade[SUBFADE_CLOCK][i], &a[i], NULL);
    if (a[0] > 0) Clock_Draw((u8)a[0], (u8)a[1], (u8)a[2]);
}

/* 0x81314F44: guide labels, drawn once per language so that a language
   change cross-fades (langGuide[] faders). */
static void Menu_DrawGuideText(u8 alpha) {
    GXColor c = sFaceWhite;
    s16 a;
    s32 lang, i;

    for (lang = 0; lang < 6; lang++) {
        Fader_GetAlpha(&gMenuWork.langGuide[lang], &a, NULL);
        c.a = (u8)(alpha * a / 255);
        for (i = 0; i < 8; i++) {
            Fader_DrawString(&gMenuWork.guideText[i], gMenuWork.guideSth[lang], gGuideGlh, &c);
        }
    }
}

/* 0x8131508C: button guide at the bottom of the screen (res 115).
   Two groups of three glyphs (stick, B, A), one per guide line.
   guideIcon[0..2] = stick/B/A of line 1, [3..5] of line 2. */
void Menu_DrawGuide(u8 alpha) {
    GXColor white = sFaceWhite;
    GXColor back = sStickBack;
    GXColor colA, colB;
    GXColor c;
    s16 g[6];
    s16 blink = gGuideBlink;
    s16 pulse, base, f;
    u8 fa;
    s32 i, k;

    colA.r = (u8)gMenu->guideColorA[0];
    colA.g = (u8)gMenu->guideColorA[1];
    colA.b = (u8)gMenu->guideColorA[2];
    colA.a = 255;
    colB.r = (u8)gMenu->guideColorB[0];
    colB.g = (u8)gMenu->guideColorB[1];
    colB.b = (u8)gMenu->guideColorB[2];
    colB.a = 255;

    GX_Setup2DView();
    GX_Setup2DTex();
    for (i = 0; i < 6; i++) {
        Fader_GetAlpha(&gMenuWork.guideIcon[i], &g[i], NULL);
    }

    /* glyph backgrounds pulse 255 -> 100 -> 255 every 256 frames */
    if (blink % 256 > 127) {
        pulse = 100 + (blink % 128) * 155 / 127;
    } else {
        pulse = 255 - (blink % 128) * 155 / 127;
    }
    base = (s16)(pulse * alpha / 255);
    for (i = 0; i < 2; i++) {
        back.a = (u8)(base * g[i * 3 + 0] / 255);
        colB.a = (u8)(base * g[i * 3 + 1] / 255);
        colA.a = (u8)(base * g[i * 3 + 2] / 255);
        Glh_DrawPane('bac1' + i * 3, gGuideGlh, &back);
        Glh_DrawPane('bac2' + i * 3, gGuideGlh, &colB);
        Glh_DrawPane('bac3' + i * 3, gGuideGlh, &colA);
    }

    /* stick: every 128 frames cross-fade between the neutral sprite
       (stk0 / stk5) and one of the four tilted ones, cycling directions */
    k = (blink / 128) % 8;
    if (k % 2 == 0) {
        f = (blink % 128) * 255 / 127;
    } else {
        f = 255 - (blink % 128) * 255 / 127;
    }
    fa = (u8)(f * alpha / 255);
    for (i = 0; i < 2; i++) {
        c = white;
        c.a = (u8)((255 - fa) * g[i * 3] / 255);
        Glh_DrawPane('stk0' + i * 5 + ((k + 1) / 2 + 3) % 4 + 1, gGuideGlh, &c);
        c.a = (u8)(fa * g[i * 3] / 255);
        Glh_DrawPane('stk0' + i * 5, gGuideGlh, &c);
    }

    /* button glyphs and the dots after them, faded in by gGuideAlpha */
    fa = (u8)(white.a * alpha / 255);
    fa = (u8)(fa * gGuideAlpha / 255);
    c = white;
    c.a = (u8)(fa * g[1] / 255);
    Glh_DrawPane('butb', gGuideGlh, &c);
    Glh_DrawPane('dot2', gGuideGlh, &c);
    c.a = (u8)(fa * g[4] / 255);
    Glh_DrawPane('butc', gGuideGlh, &c);
    Glh_DrawPane('dot5', gGuideGlh, &c);
    c.a = (u8)(fa * g[2] / 255);
    Glh_DrawPane('buta', gGuideGlh, &c);
    Glh_DrawPane('dot3', gGuideGlh, &c);
    c.a = (u8)(fa * g[5] / 255);
    Glh_DrawPane('butd', gGuideGlh, &c);
    Glh_DrawPane('dot6', gGuideGlh, &c);
    c.a = (u8)(fa * g[0] / 255);
    Glh_DrawPane('dot1', gGuideGlh, &c);
    c.a = (u8)(fa * g[3] / 255);
    Glh_DrawPane('dot4', gGuideGlh, &c);

    GX_Setup2DText();
    Menu_DrawGuideText(alpha);

    gGuideAlpha += 4;
    gGuideBlink++;
    if (gGuideAlpha > 255) gGuideAlpha = 255;
}

/* 0x8131567C: Scene draw hook for state 5. */
void Menu_Draw2D(u8 alpha) {
    GXColor c = sFaceWhite;
    Scene *scene = Scene_Get();
    s16 a;

    Menu_DrawFaces(alpha);
    Menu_DrawSubmenus();
    Menu_DrawGuide(255);
    if (SCENE_FATAL_ERROR(scene)) {
        /* "An error has occurred..." over a darkened screen */
        Fader_GetAlpha(&gMenuWork.errorMsg, &a, NULL);
        Screen_Dim((u8)(a / 2));
        GX_Setup2DView();
        GX_Setup2DText();
        Fader_DrawString(&gMenuWork.errorMsg, gDialogSth, gDialogGlh, &c);
    }
}
