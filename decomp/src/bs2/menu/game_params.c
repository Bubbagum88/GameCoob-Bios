// Game Play icon parameters (PAL 1.0 BS2, 0x81315724..0x813157B8). A unit of
// its own: it sits between menu.c and memcard.c, apart from game.c.

#include "bs2/menu.h"

GameParams gGameParamsBuf;   /* 0x814B6360 */
GameParams* gGameParams;     /* 0x814AD440 */

static void GameParams_SetDefaults(GameParams* p);

/* 0x81315724 */
void Game_InitParams(BOOL first) {
    if (first) {
        gGameParams = &gGameParamsBuf;
        GameParams_SetDefaults(gGameParams);
    }
    Game_Init(first);
}

/* 0x8131576C */
static void GameParams_SetDefaults(GameParams* p) {
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
GameParams* Game_GetParams(void) {
    return gGameParams;
}
