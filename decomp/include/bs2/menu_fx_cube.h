#ifndef BS2_MENU_FX_CUBE_H
#define BS2_MENU_FX_CUBE_H

/* Mosaic cube text ("fx"): what the drawing / effect units
   (0x81332FB8..0x813376A0) use from the fx core. The core's own units
   (menu_fx.c) type some of these differently; the ABI is the same. */

#include "bs2/menu_fx.h"

/* Cursor / mode record from the fx core (fields as this file reads them). */
typedef struct FxMode {
    s16 screen;            /* 0x00 3 on the Game screen */
    s16 pad2;
    s16 gameItem;          /* 0x04 */
    s16 mode;              /* 0x06 */
    s16 pad8;
    s16 item;              /* 0x0A */
    s16 tab;               /* 0x0C */
    s16 page;              /* 0x0E */
    s16 pad10;
    s16 dateField;         /* 0x12 */
    s16 timeField;         /* 0x14 */
    s16 value[1];          /* 0x16 */
} FxMode;

/* A 16-bit row/column pair */
typedef struct FxCell {
    s16 x;
    s16 y;
} FxCell;

/* Grid slot of a menu item (fn_8132C660): column, row and two more fields
   the cube flights do not read. */
typedef struct FxSlot {
    s16 x;
    s16 y;
    s16 unk4;
    s16 unk6;
} FxSlot;

#define FX_MAX_CUBES FX_MAX

/* fx core (menu_fx_option.c, menu_fx_anim.c) */
FxCube *Fx_GetCubes(void);                                  /* 0x81329B34 */
u8 *Fx_GetDrawBuf(void);                                    /* 0x81329B40 */
u32 Fx_GetCount(void);                                      /* 0x81329B4C */
GXColor *fn_81329B54(void);                                 /* 0x81329B54 light colours */
GXColor *fn_81329B60(void);                                 /* 0x81329B60 dark colours */
FxMode *fn_8132CFE8(void);                                  /* 0x8132CFE8 */
FxMode *fn_8132CFF4(void);                                  /* 0x8132CFF4 */
void fn_8132C750(s32 mode, f32 cx, f32 cy, f32 cz, u16 a, u16 b, FxCube *src, FxCube *dst); /* 0x8132C750 */
void fn_8132C830(Vec *centre, FxCube *cubes, u16 *list, u32 n); /* 0x8132C830 */
u16 fn_8132CB60(f32 x);                                     /* 0x8132CB60 */
void fn_8132C660(FxSlot *out, s32 idx);                     /* 0x8132C660 */
void fn_8132A690(s16 *out, s16 page, s16 *values);          /* 0x8132A690 */
u16 *fn_81332D7C(s16 idx);                                  /* 0x81332D7C */

/* glyph cube index lists of the mosaic text (menu_fx_build.c) */
u16 *fn_81331008(void); u16 fn_81331070(void);
u16 *fn_81331338(void); u16 fn_813313A0(void);
u16 *fn_813313DC(void); u16 fn_81331444(void);
u16 *fn_81331480(void); u16 fn_813314E8(void);
u16 *fn_81331524(void); u16 fn_8133158C(void);
u16 *fn_81331710(void); u16 fn_81331778(void);
u16 *fn_813317B4(void); u16 fn_8133181C(void);
u16 *fn_813318FC(void); u16 fn_81331964(void);
u16 *fn_813319A0(void); u16 fn_81331A08(void);
u16 *fn_81331AE8(void); u16 fn_81331B50(void);
u16 *fn_81331B8C(void); u16 fn_81331BF4(void);
u16 *fn_81331CD4(void); u16 fn_81331D3C(void);
u16 *fn_81331D78(void); u16 fn_81331DE0(void);
u16 *fn_81331EC0(void); u16 fn_81331F28(void);
u16 *fn_81331F64(void); u16 fn_81331FCC(void);
u16 *fn_813320AC(void); u16 fn_81332114(void);
u16 *fn_81332150(void); u16 fn_813321B8(void);
u16 *fn_813321F4(void); u16 fn_8133225C(void);
u16 *fn_81332298(void); u16 fn_81332300(void);
u16 *fn_8133233C(void); u16 fn_813323A4(void);
u16 *fn_813323E0(void); u16 fn_81332448(void);
u16 *fn_81332484(void); u16 fn_813324EC(void);
u16 *fn_81332528(void); u16 fn_81332590(void);
u16 *fn_81332714(void); u16 fn_8133277C(void);
u16 *fn_8133285C(void); u16 fn_813328C4(void);
u16 *fn_81332900(void); u16 fn_81332968(void);
u16 *fn_81332C34(void); u16 fn_81332C9C(void);
u16 *fn_81332CD8(void); u16 fn_81332D40(void);

MtxPtr fn_81302F94(void);                                   /* 0x81302F94 view matrix */
void J3D_LoadModelData(J3DModel *model);                    /* 0x813034E4 */
void J3D_DrawPacket(J3DModelInstance *inst, void *packet);  /* 0x81305248 */
void fn_813052E8(J3DModelInstance *inst, void *packet);     /* 0x813052E8 draw packet again */


/* menu_fx_draw.c */
void Fx_InitCubeModel(void);

/* menu_fx_morph.c */
void fn_81333344(void);
void fn_81333384(void);
FxCube *fn_813334AC(void);
u16 fn_813335AC(void);
s32 fn_81333614(void);

/* menu_fx_spark.c */
void fn_8133370C(void);
void fn_8133374C(FxCube *cubes, u32 n);
void Fx_EmitSparks(FxCube *cubes);
void fn_81333C7C(FxCube *cubes);
FxCube *fn_8133491C(void);
void fn_81334928(void);
void fn_8133498C(f32 *dx, f32 *dy, u16 *rx, u16 *ry, f32 t);
f32 fn_81334B40(f32 mid, f32 t, f32 lo, f32 hi);
f32 fn_81334BA8(f32 x, f32 y);

/* menu_fx_grid.c */
s16 fn_81334C08(void);
u8 *fn_81334C10(void);
FxCube *fn_81334C1C(void);
void fn_81334C28(void);
void fn_81334C60(FxCube *src);
void fn_813355E0(FxCube *src);
f32 fn_81336594(void);
f32 fn_813365F4(void);
void fn_81336650(Vec dir, s16 from, s16 to);
void fn_8133682C(void);
void fn_813368F0(void);

/* menu_fx_wipe.c */
s16 fn_81336968(void);
u8 *fn_81336970(void);
FxCube *fn_8133697C(void);
void fn_81336988(void);
void fn_813369B0(FxCube *src);
void fn_81336F9C(FxCube *src);
f32 fn_81337530(void);
void fn_8133758C(Vec dir, s16 from, s16 to);

#endif
