#ifndef BS2_RES_H
#define BS2_RES_H

#include "types.h"

/* Resource ids of the current language (scene resource tables). */
s32 Res_OptionLayout(void);          /* 0x8130B894 */
s32 Res_GameLayout(void);            /* 0x8130B89C */
s32 Res_GameStrings(void);           /* 0x8130B8D8 */
s32 Res_OptionStrings(s32 lang);     /* 0x8130BAB8 */
s32 Res_ClockLayout(void);
s32 Res_ClockStrings(void);

#endif
