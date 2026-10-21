// Based on the Pikmin decomp (CC0) OSFont.c
#include <dolphin/os.h>
#include <dolphin/hw_regs.h>

#define OS_FONT_ENCODE_ANSI 0
#define OS_FONT_ENCODE_SJIS 1

#define VI_NTSC 0
#define VI_PAL 1
#define VI_MPAL 2

#define VI_DTV_STAT 0x37

u16 OSGetFontEncode(void) {
    static u16 fontEncode = 0xFFFF;

    if (fontEncode <= 1) {
        return fontEncode;
    }
    switch (*(int*)0x800000CC) {
    case VI_NTSC:
        fontEncode = (__VIRegs[VI_DTV_STAT] & 2) ? OS_FONT_ENCODE_SJIS : OS_FONT_ENCODE_ANSI;
        break;
    case VI_PAL:
    case VI_MPAL:
    default:
        fontEncode = OS_FONT_ENCODE_ANSI;
    }
    return fontEncode;
}
