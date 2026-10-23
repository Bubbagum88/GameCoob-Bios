// Mosaic cube text cursor flights (PAL 1.0 BS2, 0x81334C08..0x81336968):
// when the Options / Calendar cursor moves, the cubes of the old item fly
// out and the new item's cubes fly in.

#include "bs2/menu_fx_cube.h"

/* Cubes that fly out of the old item and into the new one when the
   Options / Calendar cursor moves: 4 blocks of n cubes (old rows, new rows,
   old columns, new columns). */
static FxCube sFxGridCube[0x70];
static u8 sFxGridAlpha[0x70];   /* per glyph cube of the text */
static s16 sFxGridDist[0x70];   /* random flight distance */
static Vec sFxGridDir;

static FxCell sFxGridTo;
static FxCell sFxGridFrom;
static FxCell sFxGridFrame;
static s16 sFxGridCount;

/* the four flights: old text out (rows, then columns), new text in */
#define FXGRID_MOVE(src, l1, l2, n)                                                                                         \
    idx = fn_81332D7C(sFxGridFrom.y);                                                                                       \
    for (i = 0; i < n; i++) {                                                                                               \
        sFxGridCube[i].pos.x = src[l1[i]].pos.x + -sFxGridDir.x * (sFxGridDist[i] * Menu_Get()->fxD0) * fn_81336594();                  \
        sFxGridCube[i].pos.y = src[l1[i]].pos.y + -sFxGridDir.y * (sFxGridDist[i] * Menu_Get()->fxD0) * fn_81336594();                  \
        sFxGridCube[i].pos.z = src[l1[i]].pos.z + -sFxGridDir.z * (sFxGridDist[i] * Menu_Get()->fxD0) * fn_81336594();                  \
        sFxGridCube[i].rot[0] = src[l1[i]].rot[0];                                                                              \
        sFxGridCube[i].rot[1] = src[l1[i]].rot[1];                                                                              \
        sFxGridCube[i].rot[2] = src[l1[i]].rot[2];                                                                              \
        sFxGridCube[i].scale.x = src[l1[i]].scale.x;                                                                            \
        sFxGridCube[i].scale.y = src[l1[i]].scale.y;                                                                            \
        sFxGridCube[i].scale.z = src[l1[i]].scale.z;                                                                            \
        sFxGridAlpha[idx[i]] = 255.0f * (1.0f - fn_81336594());                                                                 \
    }                                                                                                                       \
    idx = fn_81332D7C(sFxGridTo.y);                                                                                         \
    for (i = 0; i < n; i++) {                                                                                               \
        sFxGridCube[n + i].pos.x = src[l1[i]].pos.x + sFxGridDir.x * (sFxGridDist[n + i] * Menu_Get()->fxD0) * (1.0f - fn_81336594());  \
        sFxGridCube[n + i].pos.y = src[l1[i]].pos.y + sFxGridDir.y * (sFxGridDist[n + i] * Menu_Get()->fxD0) * (1.0f - fn_81336594());  \
        sFxGridCube[n + i].pos.z = src[l1[i]].pos.z + sFxGridDir.z * (sFxGridDist[n + i] * Menu_Get()->fxD0) * (1.0f - fn_81336594());  \
        sFxGridCube[n + i].rot[0] = src[l1[i]].rot[0];                                                                          \
        sFxGridCube[n + i].rot[1] = src[l1[i]].rot[1];                                                                          \
        sFxGridCube[n + i].rot[2] = src[l1[i]].rot[2];                                                                          \
        sFxGridCube[n + i].scale.x = src[l1[i]].scale.x;                                                                        \
        sFxGridCube[n + i].scale.y = src[l1[i]].scale.y;                                                                        \
        sFxGridCube[n + i].scale.z = src[l1[i]].scale.z;                                                                        \
        sFxGridAlpha[n + idx[i]] = 255.0f * fn_81336594();                                                                      \
    }                                                                                                                       \
    idx = fn_81332D7C(sFxGridFrom.x);                                                                                       \
    for (i = 0; i < n; i++) {                                                                                               \
        sFxGridCube[n * 2 + i].pos.x = src[l2[i]].pos.x + -sFxGridDir.x * sFxGridDist[n * 2 + i] * fn_813365F4();                       \
        sFxGridCube[n * 2 + i].pos.y = src[l2[i]].pos.y + -sFxGridDir.y * sFxGridDist[n * 2 + i] * fn_813365F4();                       \
        sFxGridCube[n * 2 + i].pos.z = src[l2[i]].pos.z + -sFxGridDir.z * sFxGridDist[n * 2 + i] * fn_813365F4();                       \
        sFxGridCube[n * 2 + i].rot[0] = src[l2[i]].rot[0];                                                                      \
        sFxGridCube[n * 2 + i].rot[1] = src[l2[i]].rot[1];                                                                      \
        sFxGridCube[n * 2 + i].rot[2] = src[l2[i]].rot[2];                                                                      \
        sFxGridCube[n * 2 + i].scale.x = src[l2[i]].scale.x;                                                                    \
        sFxGridCube[n * 2 + i].scale.y = src[l2[i]].scale.y;                                                                    \
        sFxGridCube[n * 2 + i].scale.z = src[l2[i]].scale.z;                                                                    \
        sFxGridAlpha[n * 2 + idx[i]] = 255.0f * (1.0f - fn_813365F4());                                                         \
    }                                                                                                                       \
    idx = fn_81332D7C(sFxGridTo.x);                                                                                         \
    for (i = 0; i < n; i++) {                                                                                               \
        sFxGridCube[n * 3 + i].pos.x = src[l2[i]].pos.x + sFxGridDir.x * sFxGridDist[n * 3 + i] * (1.0f - fn_813365F4());               \
        sFxGridCube[n * 3 + i].pos.y = src[l2[i]].pos.y + sFxGridDir.y * sFxGridDist[n * 3 + i] * (1.0f - fn_813365F4());               \
        sFxGridCube[n * 3 + i].pos.z = src[l2[i]].pos.z + sFxGridDir.z * sFxGridDist[n * 3 + i] * (1.0f - fn_813365F4());               \
        sFxGridCube[n * 3 + i].rot[0] = src[l2[i]].rot[0];                                                                      \
        sFxGridCube[n * 3 + i].rot[1] = src[l2[i]].rot[1];                                                                      \
        sFxGridCube[n * 3 + i].rot[2] = src[l2[i]].rot[2];                                                                      \
        sFxGridCube[n * 3 + i].scale.x = src[l2[i]].scale.x;                                                                    \
        sFxGridCube[n * 3 + i].scale.y = src[l2[i]].scale.y;                                                                    \
        sFxGridCube[n * 3 + i].scale.z = src[l2[i]].scale.z;                                                                    \
        sFxGridAlpha[n * 3 + idx[i]] = 255.0f * fn_813365F4();                                                                  \
    }

/* 0x81334C08 */
s16 fn_81334C08(void) {
    return sFxGridCount;
}

/* 0x81334C10 */
u8 *fn_81334C10(void) {
    return sFxGridAlpha;
}

/* 0x81334C1C */
FxCube *fn_81334C1C(void) {
    return sFxGridCube;
}

/* 0x81334C28: finish both flights at once. */
void fn_81334C28(void) {
    sFxGridFrame.x = Menu_Get()->fxD4;
    sFxGridFrame.y = Menu_Get()->fxD4;
}

/* Start the Options flight along -x from the old item to the new one. */
static inline void FxGridFlyItem(void) {
    Vec *d;
    Vec dir = { -1.0f, 0.0f, 0.0f };

    d = &dir;
    fn_81336650(*d, fn_8132CFF4()->item, fn_8132CFE8()->item);
}

/* 0x81334C60: Options cursor moved, fly the item cubes over. */
void fn_81334C60(FxCube *src) {
    u16 *l1;
    u16 n;
    u16 *l2;
    u16 *idx;
    int i;

    if (fn_8132CFE8()->mode == 2) {
        if (fn_8132CFF4()->mode != 2) {
            fn_813368F0();
            sFxGridCount = 0;
            return;
        }
        sFxGridCount = 0x70;
    } else {
        sFxGridCount = 0;
        return;
    }
    l1 = fn_8133285C();
    n = fn_813328C4();
    l2 = fn_81332900();
    for (i = 0; i < 0x70; i++) {
        sFxGridAlpha[i] = 0;
    }
    FXGRID_MOVE(src, l1, l2, n)
    FxGridFlyItem();
    if (sFxGridFrame.x < Menu_Get()->fxD4) {
        sFxGridFrame.x++;
    }
    if (sFxGridFrame.y < Menu_Get()->fxD4) {
        sFxGridFrame.y++;
    }
}

static inline f64 FxFabs(f64 x) {
    return __fabs(x);
}

/* 0x813355E0: Calendar cursor or value changed, fly the field cubes over. */
void fn_813355E0(FxCube *src) {
    u16 *l1;
    u16 n;
    u16 *l2;
    u16 *idx;
    int i;
    s16 cur[6];
    s16 prev[6];
    FxSlot p;

    if (fn_8132CFE8()->page != 0) {
        if (fn_8132CFF4()->page != 0) {
            sFxGridCount = 0x70;
        } else {
            fn_8133682C();
            sFxGridCount = 0;
            return;
        }
    } else {
        sFxGridCount = 0;
        return;
    }
    n = fn_81332114();
    fn_8132A690(cur, fn_8132CFE8()->page, fn_8132CFE8()->value);
    fn_8132A690(prev, fn_8132CFF4()->page, fn_8132CFF4()->value);
    if (fn_8132CFE8()->page == 1) {
        switch (fn_8132CFE8()->dateField) {
        case 0:
            if ((cur[0] == 2099 && prev[0] == 2000) || (cur[0] == 2000 && prev[0] == 2099)) {
                Vec dir = { 0.0f, 1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[0], cur[0]);
            } else {
                Vec dir = { 0.0f, -1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[0], cur[0]);
            }
            l1 = fn_813321F4();
            l2 = fn_81332298();
            break;
        case 1:
            if ((cur[1] == 12 && prev[1] == 1) || (cur[1] == 1 && prev[1] == 12)) {
                Vec dir = { 0.0f, 1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[1], cur[1]);
            } else {
                Vec dir = { 0.0f, -1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[1], cur[1]);
            }
            l1 = fn_81331CD4();
            l2 = fn_81331D78();
            break;
        case 2:
            if (((cur[2] == 31 || cur[2] == 30 || cur[2] == 29 || cur[2] == 28) && prev[2] == 1) || (cur[2] == 1 && (prev[2] == 31 || prev[2] == 30 || prev[2] == 29 || prev[2] == 28))) {
                Vec dir = { 0.0f, 1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[2], cur[2]);
            } else {
                Vec dir = { 0.0f, -1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[2], cur[2]);
            }
            l1 = fn_81331710();
            l2 = fn_813317B4();
            break;
        }
    } else if (fn_8132CFE8()->page == 2) {
        switch (fn_8132CFE8()->timeField) {
        case 0:
            if ((cur[3] == 23 && prev[3] == 0) || (cur[3] == 0 && prev[3] == 23)) {
                Vec dir = { 0.0f, 1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[3], cur[3]);
            } else {
                Vec dir = { 0.0f, -1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[3], cur[3]);
            }
            l1 = fn_813318FC();
            l2 = fn_813319A0();
            break;
        case 1:
            if ((cur[4] == 59 && prev[4] == 0) || (cur[4] == 0 && prev[4] == 59)) {
                Vec dir = { 0.0f, 1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[4], cur[4]);
            } else {
                Vec dir = { 0.0f, -1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[4], cur[4]);
            }
            l1 = fn_81331AE8();
            l2 = fn_81331B8C();
            break;
        case 2:
            if ((cur[5] == 59 && prev[5] == 0) || (cur[5] == 0 && prev[5] == 59)) {
                Vec dir = { 0.0f, 1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[5], cur[5]);
            } else {
                Vec dir = { 0.0f, -1.0f, 0.0f };

                fn_81336650(*(Vec *)&dir, prev[5], cur[5]);
            }
            l1 = fn_81331EC0();
            l2 = fn_81331F64();
            break;
        }
    } else {
        return;
    }
    for (i = 0; i < 0x70; i++) {
        sFxGridAlpha[i] = 0;
    }
    if (fn_8132CFE8()->dateField != fn_8132CFF4()->dateField) {
        sFxGridFrame.x = Menu_Get()->fxD4;
        sFxGridFrame.y = Menu_Get()->fxD4;
        switch (fn_8132CFE8()->dateField) {
        case 0:
            fn_8132C660(&p, cur[0]);
            break;
        case 1:
            fn_8132C660(&p, cur[1]);
            break;
        case 2:
            fn_8132C660(&p, cur[2]);
            break;
        }
        sFxGridTo.x = sFxGridFrom.x = p.x;
        sFxGridTo.y = sFxGridFrom.y = p.y;
    }
    if (fn_8132CFE8()->timeField != fn_8132CFF4()->timeField) {
        sFxGridFrame.x = Menu_Get()->fxD4;
        sFxGridFrame.y = Menu_Get()->fxD4;
        switch (fn_8132CFE8()->timeField) {
        case 0:
            fn_8132C660(&p, cur[3]);
            break;
        case 1:
            fn_8132C660(&p, cur[4]);
            break;
        case 2:
            fn_8132C660(&p, cur[5]);
            break;
        }
        sFxGridTo.x = sFxGridFrom.x = p.x;
        sFxGridTo.y = sFxGridFrom.y = p.y;
    }
    FXGRID_MOVE(src, l1, l2, n)
    if (sFxGridFrame.x < Menu_Get()->fxD4) {
        sFxGridFrame.x++;
    }
    if (sFxGridFrame.y < Menu_Get()->fxD4) {
        sFxGridFrame.y++;
    }
}

/* 0x81336594: row flight progress, 0..1. */
f32 fn_81336594(void) {
    return (f32)sFxGridFrame.y / Menu_Get()->fxD4;
}

/* 0x813365F4: column flight progress, 0..1. */
f32 fn_813365F4(void) {
    return (f32)sFxGridFrame.x / Menu_Get()->fxD4;
}

/* 0x81336650: start a flight from item `from` to item `to` along dir. */
void fn_81336650(Vec dir, s16 from, s16 to) {
    FxSlot p0;
    FxSlot p1;
    s16 tbl[8]; /* unused; keeps the frame size */
    s16 i;
    s16 d;

    d = to - from;
    if (from != to) {
        if (d > 0) {
            sFxGridDir.x = dir.x;
            sFxGridDir.y = dir.y;
            sFxGridDir.z = dir.z;
        } else {
            sFxGridDir.x = -dir.x;
            sFxGridDir.y = -dir.y;
            sFxGridDir.z = -dir.z;
        }
        for (i = 0; i < 0x70; i++) {
            sFxGridDist[i] = Menu_Get()->fxCC * (f32)(Rand() % 600 + 200) / 10.0f;
        }
        fn_8132C660(&p0, (s32)FxFabs(from));
        fn_8132C660(&p1, (s32)FxFabs(to));
        if (p0.y != p1.y) {
            sFxGridFrame.y = 0;
            sFxGridFrom.y = p0.y;
            sFxGridTo.y = p1.y;
        }
        if (p0.x != p1.x) {
            sFxGridFrom.x = p0.x;
            sFxGridFrame.x = 0;
            sFxGridTo.x = p1.x;
        }
    }
}

/* 0x8133682C: snap the Calendar flight to the current field. */
void fn_8133682C(void) {
    s16 tbl[8];
    FxSlot p;

    fn_8132A690(tbl, fn_8132CFE8()->page, fn_8132CFE8()->value);
    if (fn_8132CFE8()->page == 1) {
        fn_8132C660(&p, tbl[fn_8132CFE8()->dateField]);
    } else if (fn_8132CFE8()->page == 2) {
        fn_8132C660(&p, tbl[3 + fn_8132CFE8()->timeField]);
    } else {
        return;
    }
    sFxGridTo.x = sFxGridFrom.x = p.x;
    sFxGridTo.y = sFxGridFrom.y = p.y;
}

/* 0x813368F0: snap the Options flight to the current item. */
void fn_813368F0(void) {
    FxSlot p;
    s16 tbl[8]; /* unused; keeps the frame size */

    fn_8132C660(&p, (s32)FxFabs(fn_8132CFE8()->item));
    sFxGridFrom.x = sFxGridTo.x = p.x;
    sFxGridFrom.y = sFxGridTo.y = p.y;
}
