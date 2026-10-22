// Mosaic cube text wipes (PAL 1.0 BS2, 0x81336968..0x813376A0): when the
// Options tab or the Game screen item changes, the old text's cubes fly out
// one way and the new text's cubes fly in from the other.

#include "bs2/menu_fx.h"

#define FX_WIPE_CUBES 1430

static FxCube sFxWipeCube[FX_WIPE_CUBES];
static u8 sFxWipeAlpha[FX_WIPE_CUBES];
static s16 sFxWipeDist[FX_WIPE_CUBES];  /* random flight distance */
static Vec sFxWipeDir;

static s16 sFxWipeFrame;
static s16 sFxWipeFrom;
static s16 sFxWipeTo;
static s16 sFxWipeCount;

/* 0x81336968 */
s16 fn_81336968(void) {
    return sFxWipeCount;
}

/* 0x81336970 */
u8 *fn_81336970(void) {
    return sFxWipeAlpha;
}

/* 0x8133697C */
FxCube *fn_8133697C(void) {
    return sFxWipeCube;
}

/* 0x81336988: finish the wipe at once. */
void fn_81336988(void) {
    sFxWipeFrame = Menu_Get()->fxD4;
}

/* Start the wipe along -x / +x from the old text to the new one. */
static inline void FxWipeTabLeft(void) {
    Vec *d;
    Vec dir = { -1.0f, 0.0f, 0.0f };

    d = &dir;
    fn_8133758C(*d, fn_8132CFF4()->tab, fn_8132CFE8()->tab);
}

static inline void FxWipeTabRight(void) {
    Vec *d;
    Vec dir = { 1.0f, 0.0f, 0.0f };

    d = &dir;
    fn_8133758C(*d, fn_8132CFF4()->tab, fn_8132CFE8()->tab);
}

static inline void FxWipeGameLeft(void) {
    Vec *d;
    Vec dir = { -1.0f, 0.0f, 0.0f };

    d = &dir;
    fn_8133758C(*d, fn_8132CFF4()->gameItem, fn_8132CFE8()->gameItem);
}

/* 0x813369B0: Options tab changed, wipe the old tab text out and the new one in. */
void fn_813369B0(FxCube *src) {
    u16 *l;
    u16 n1;
    u16 n2;
    int i;

    if (fn_8132CFE8()->mode == 1 && fn_8132CFF4()->mode == 1) {
        sFxWipeCount = FX_WIPE_CUBES;
    } else {
        sFxWipeTo = sFxWipeFrom = fn_8132CFE8()->tab;
        sFxWipeCount = 0;
        return;
    }
    for (i = 0; i < FX_WIPE_CUBES; i++) {
        sFxWipeAlpha[i] = 0;
    }
    switch (sFxWipeFrom) {
    case 0:
        l = fn_81332714();
        n1 = fn_8133277C();
        break;
    case 1:
        l = fn_81332CD8();
        n1 = fn_81332D40();
        break;
    }
    for (i = 0; i < n1; i++) {
        sFxWipeCube[i].pos.x = src[l[i]].pos.x + -sFxWipeDir.x * sFxWipeDist[i] * fn_81337530();
        sFxWipeCube[i].pos.y = src[l[i]].pos.y + -sFxWipeDir.y * sFxWipeDist[i] * fn_81337530();
        sFxWipeCube[i].pos.z = src[l[i]].pos.z + -sFxWipeDir.z * sFxWipeDist[i] * fn_81337530();
        sFxWipeCube[i].rot[0] = src[l[i]].rot[0];
        sFxWipeCube[i].rot[1] = src[l[i]].rot[1];
        sFxWipeCube[i].rot[2] = src[l[i]].rot[2];
        sFxWipeCube[i].scale.x = src[l[i]].scale.x;
        sFxWipeCube[i].scale.y = src[l[i]].scale.y;
        sFxWipeCube[i].scale.z = src[l[i]].scale.z;
        sFxWipeAlpha[i] = 255.0f * (1.0f - fn_81337530());
    }
    switch (sFxWipeTo) {
    case 0:
        l = fn_81332714();
        n2 = fn_8133277C();
        break;
    case 1:
        l = fn_81332CD8();
        n2 = fn_81332D40();
        break;
    }
    for (i = 0; i < n2; i++) {
        sFxWipeCube[n1 + i].pos.x = src[l[i]].pos.x + sFxWipeDir.x * sFxWipeDist[n1 + i] * (1.0f - fn_81337530());
        sFxWipeCube[n1 + i].pos.y = src[l[i]].pos.y + sFxWipeDir.y * sFxWipeDist[n1 + i] * (1.0f - fn_81337530());
        sFxWipeCube[n1 + i].pos.z = src[l[i]].pos.z + sFxWipeDir.z * sFxWipeDist[n1 + i] * (1.0f - fn_81337530());
        sFxWipeCube[n1 + i].rot[0] = src[l[i]].rot[0];
        sFxWipeCube[n1 + i].rot[1] = src[l[i]].rot[1];
        sFxWipeCube[n1 + i].rot[2] = src[l[i]].rot[2];
        sFxWipeCube[n1 + i].scale.x = src[l[i]].scale.x;
        sFxWipeCube[n1 + i].scale.y = src[l[i]].scale.y;
        sFxWipeCube[n1 + i].scale.z = src[l[i]].scale.z;
        sFxWipeAlpha[n1 + i] = 255.0f * fn_81337530();
    }
    if (fn_8132CFE8()->tab != 0) {
        FxWipeTabLeft();
    } else {
        FxWipeTabRight();
    }
    if (sFxWipeFrame < Menu_Get()->fxD4) {
        sFxWipeFrame++;
    }
}

/* 0x81336F9C: Game screen item changed, same wipe. */
void fn_81336F9C(FxCube *src) {
    u16 *l;
    u16 n1;
    u16 n2;
    int i;

    if (fn_8132CFE8()->screen == 3 && fn_8132CFF4()->screen == 3) {
        sFxWipeCount = FX_WIPE_CUBES;
    } else {
        sFxWipeTo = sFxWipeFrom = fn_8132CFE8()->gameItem;
        sFxWipeCount = 0;
        return;
    }
    for (i = 0; i < FX_WIPE_CUBES; i++) {
        sFxWipeAlpha[i] = 0;
    }
    switch (sFxWipeFrom) {
    case 0:
    default:
        l = fn_813323E0();
        n1 = fn_81332448();
        break;
    case 1:
        l = fn_8133233C();
        n1 = fn_813323A4();
        break;
    case 2:
        l = fn_81332484();
        n1 = fn_813324EC();
        break;
    }
    for (i = 0; i < n1; i++) {
        sFxWipeCube[i].pos.x = src[l[i]].pos.x + -sFxWipeDir.x * sFxWipeDist[i] * fn_81337530();
        sFxWipeCube[i].pos.y = src[l[i]].pos.y + -sFxWipeDir.y * sFxWipeDist[i] * fn_81337530();
        sFxWipeCube[i].pos.z = src[l[i]].pos.z + -sFxWipeDir.z * sFxWipeDist[i] * fn_81337530();
        sFxWipeCube[i].rot[0] = src[l[i]].rot[0];
        sFxWipeCube[i].rot[1] = src[l[i]].rot[1];
        sFxWipeCube[i].rot[2] = src[l[i]].rot[2];
        sFxWipeCube[i].scale.x = src[l[i]].scale.x;
        sFxWipeCube[i].scale.y = src[l[i]].scale.y;
        sFxWipeCube[i].scale.z = src[l[i]].scale.z;
        sFxWipeAlpha[i] = 255.0f * (1.0f - fn_81337530());
    }
    switch (sFxWipeTo) {
    case 0:
    default:
        l = fn_813323E0();
        n2 = fn_81332448();
        break;
    case 1:
        l = fn_8133233C();
        n2 = fn_813323A4();
        break;
    case 2:
        l = fn_81332484();
        n2 = fn_813324EC();
        break;
    }
    for (i = 0; i < n2; i++) {
        sFxWipeCube[n1 + i].pos.x = src[l[i]].pos.x + sFxWipeDir.x * sFxWipeDist[n1 + i] * (1.0f - fn_81337530());
        sFxWipeCube[n1 + i].pos.y = src[l[i]].pos.y + sFxWipeDir.y * sFxWipeDist[n1 + i] * (1.0f - fn_81337530());
        sFxWipeCube[n1 + i].pos.z = src[l[i]].pos.z + sFxWipeDir.z * sFxWipeDist[n1 + i] * (1.0f - fn_81337530());
        sFxWipeCube[n1 + i].rot[0] = src[l[i]].rot[0];
        sFxWipeCube[n1 + i].rot[1] = src[l[i]].rot[1];
        sFxWipeCube[n1 + i].rot[2] = src[l[i]].rot[2];
        sFxWipeCube[n1 + i].scale.x = src[l[i]].scale.x;
        sFxWipeCube[n1 + i].scale.y = src[l[i]].scale.y;
        sFxWipeCube[n1 + i].scale.z = src[l[i]].scale.z;
        sFxWipeAlpha[n1 + i] = 255.0f * fn_81337530();
    }
    FxWipeGameLeft();
    if (sFxWipeFrame < Menu_Get()->fxD4) {
        sFxWipeFrame++;
    }
}

/* 0x81337530: wipe progress, 0..1. */
f32 fn_81337530(void) {
    return (f32)sFxWipeFrame / Menu_Get()->fxD4;
}

/* 0x8133758C: start a wipe from text `from` to text `to` along dir. */
void fn_8133758C(Vec dir, s16 from, s16 to) {
    int i;

    if (from != to) {
        sFxWipeDir.x = dir.x;
        sFxWipeDir.y = dir.y;
        sFxWipeDir.z = dir.z;
        for (i = 0; i < FX_WIPE_CUBES; i++) {
            sFxWipeDist[i] = Menu_Get()->fxCC * (f32)(Rand() % 600 + 200) / 10.0f;
        }
        sFxWipeFrom = from;
        sFxWipeFrame = 0;
        sFxWipeTo = to;
    }
}
