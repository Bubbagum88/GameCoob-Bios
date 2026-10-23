// Calendar icon parameters (PAL 1.0 BS2, 0x813217F0..0x813218C8).

#include "bs2/menu.h"

ClockParams gClockParamsBuf;   /* 0x814D9F38 */
ClockParams* gClockParams;     /* 0x814AD518 */

static void ClockParams_SetDefaults(ClockParams* p);

/* 0x813217F0 */
void Clock_InitParams(BOOL first) {
    if (first) {
        gClockParams = &gClockParamsBuf;
        ClockParams_SetDefaults(gClockParams);
    }
    Clock_Init(first);
}

/* 0x81321838 */
static void ClockParams_SetDefaults(ClockParams* p) {
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
ClockParams* Clock_GetParams(void) {
    return gClockParams;
}
