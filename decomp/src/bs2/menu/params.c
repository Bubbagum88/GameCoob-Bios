// Non-matching: functionally equivalent decompilation
//
// Game/Card parameter blocks (PAL 1.0 BS2, 0x81315724..). Moved out of game.c:
// these live in a different translation unit than the Game Play submenu.

#include "bs2/menu.h"

/* 0x81315724 */
void Game_InitParams(BOOL first) {
    if (first) {
        gGameParams = &gGameParamsBuf;
        GameParams_SetDefaults(gGameParams);
    }
    Game_Init(first);
}

/* 0x8131576C */
static void GameParams_SetDefaults(GameParams *p) {
    p->tint[0] = 0;
    p->tint[1] = -90;
    p->tint[2] = 20;
    p->tint[3] = 255;
    p->ringRadius = 75;
    p->cubeScale = 0.65f;
    p->fadeSpeed = 10;
    p->shrinkSpeed = 5;
}

/* 0x813157B0 */
GameParams *Game_GetParams(void) {
    return gGameParams;
}



OptionParams gOptionParamsBuf;      /* 0x814D9F00 */
OptionParams *gOptionParams;        /* 0x814AD510 */

static void OptionParams_SetDefaults(OptionParams *p);

/* 0x81321730 */
void Option_InitParams(BOOL first) {
    if (first) {
        gOptionParams = &gOptionParamsBuf;
        OptionParams_SetDefaults(gOptionParams);
    }
    Option_Init(first);
}

/* 0x81321778 */
static void OptionParams_SetDefaults(OptionParams *p) {
    p->tint[0] = -30;
    p->tint[1] = 20;
    p->tint[2] = -50;
    p->tint[3] = 255;
    p->textColor[0] = 0;
    p->textColor[1] = 255;
    p->textColor[2] = 120;
    p->f18 = 2.0f;
    p->s20 = 12;
    p->s1E = 5;
    p->itemOfs[0] = -15.0f;
    p->itemOfs[1] = -4.0f;
    p->itemOfs[2] = 6.0f;
    p->langOfs = 8.0f;
}

/* 0x813217E8 */
OptionParams *Option_GetParams(void) {
    return gOptionParams;
}



ClockParams gClockParamsBuf;        /* 0x814D9F38 */
ClockParams *gClockParams;          /* 0x814AD518 */

static void ClockParams_SetDefaults(ClockParams *p);

/* 0x813217F0 */
void Clock_InitParams(BOOL first) {
    if (first) {
        gClockParams = &gClockParamsBuf;
        ClockParams_SetDefaults(gClockParams);
    }
    Clock_Init(first);
}

/* 0x81321838 */
static void ClockParams_SetDefaults(ClockParams *p) {
    p->tint[0] = 40;
    p->tint[1] = 20;
    p->tint[2] = -50;
    p->tint[3] = 255;
    p->tint2[0] = 80;
    p->tint2[1] = 0;
    p->tint2[2] = -60;
    p->tint2[3] = 255;
    p->handLen[0] = 0xFB0;
    p->handLen[1] = 0x1240;
    p->handLen[2] = 0x1740;
    p->handLen[3] = 0xFF0;
    p->handLen[4] = 0x1240;
    p->handLen[5] = 0x15D0;
    p->textColor[0] = 255;
    p->textColor[1] = 255;
    p->textColor[2] = 0;
    p->s24 = 14;
    p->s22 = 6;
}

/* 0x813218C0 */
ClockParams *Clock_GetParams(void) {
    return gClockParams;
}


