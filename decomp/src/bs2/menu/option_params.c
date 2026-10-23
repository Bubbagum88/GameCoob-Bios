// Options icon parameters (PAL 1.0 BS2, 0x81321730..0x813217F0).

#include "bs2/menu.h"

OptionParams gOptionParamsBuf;   /* 0x814D9F00 */
OptionParams* gOptionParams;     /* 0x814AD510 */

static void OptionParams_SetDefaults(OptionParams* p);

/* 0x81321730 */
void Option_InitParams(BOOL first) {
    if (first) {
        gOptionParams = &gOptionParamsBuf;
        OptionParams_SetDefaults(gOptionParams);
    }
    Option_Init(first);
}

/* 0x81321778 */
static void OptionParams_SetDefaults(OptionParams* p) {
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
OptionParams* Option_GetParams(void) {
    return gOptionParams;
}
