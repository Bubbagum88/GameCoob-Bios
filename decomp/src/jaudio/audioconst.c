// NEOS constants. Based on ac-decomp src/static/jaudio_NES/internal/audioconst.c
// (CC0); the IPL has five spec configs (only the first set), a 42240 Hz output
// rate and a second, unreferenced effect config.

#include "jaudio_NES/audioconst.h"

fxconfig NA_HALL_DELAY = { 1, 0x40, 0x3FFF, 0x8, 0xFFF, 0x5FFF, 0xF001, 0xFFF, 1, 0x5FFF, 6, 6 };
fxconfig lbl_813F4620 = { 1, 0x80, 0x5FFF, 0x20, 0xFC01, 0x2FFF, 0x2FFF, 0xD001, -1, 0, 2, 2 };

// clang-format off
na_spec_config NA_SPEC_CONFIG[5] = {
    {
        0xA500,
        1,
        20,
        2,
        1,
        0,
        0,
        0,
        0,
        &NA_HALL_DELAY,
        0x200,
        0x200,
        0x7FFF,
        0,
        0,
        0,
        0x180000,
        0x4000,
        0x3000,
        0,
        0,
        0,
    },
};
// clang-format on

ALGlobalsConst AGC = {
    20, 48, 0x44000, 0x10000, 0x8000,
};
