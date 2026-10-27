// NEOS audio archive tables of the IPL: the sequence, bank, wave and data
// archive headers (offsets and sizes in the linked-in sound archive) and the
// sequence -> bank map. NEOS reads them through jaudio_NES/audioheaders.h as
// ArcHeader; they are spelled out here with fixed entry counts.

#include "types.h"
#include "jaudio_NES/audiostruct.h"

#define ARC_HEADER(n)            struct {                         s16 numEntries;              s16 medium;                  u8* pData;                   u8 copy;                     u8 pad[7];                   ArcEntry entries[n];     }

ARC_HEADER(11) AudioseqHeaderStart = {
    11, 0, NULL, 0, { 0 },
    {
        { 0x0, 0x7C0, 2, 0, 0, 0, 0 },
        { 0x7C0, 0xA0, 2, 3, 0, 0, 0 },
        { 0x1, 0x0, 2, 3, 0, 0, 0 },
        { 0x1, 0x0, 2, 3, 0, 0, 0 },
        { 0x860, 0x3D60, 2, 2, 0, 0, 0 },
        { 0x45C0, 0xCC0, 2, 2, 0, 0, 0 },
        { 0x5280, 0x1720, 2, 2, 0, 0, 0 },
        { 0x69A0, 0x300, 2, 2, 0, 0, 0 },
        { 0x6CA0, 0x16E0, 2, 2, 0, 0, 0 },
        { 0x8380, 0x160, 2, 2, 0, 0, 0 },
        { 0x84E0, 0xE0, 2, 2, 0, 0, 0 },
    },
};

ARC_HEADER(6) AudiobankHeaderStart = {
    6, 0, NULL, 0, { 0 },
    {
        { 0x0, 0x12C0, 2, 0, 255, 278, 0 },
        { 0x12C0, 0x9E0, 2, 3, 511, 1280, 0 },
        { 0x1CA0, 0x1660, 2, 0, 767, 6656, 0 },
        { 0x3300, 0x220, 2, 2, 1023, 512, 0 },
        { 0x3520, 0xD00, 2, 2, 1023, 2624, 0 },
        { 0x4220, 0x11A0, 2, 2, 1023, 2624, 0 },
    },
};

ARC_HEADER(4) AudiowaveHeaderStart = {
    4, 0, NULL, 0, { 0 },
    {
        { 0x0, 0x13D20, 2, 3, 0, 0, 0 },
        { 0x13D20, 0x1E760, 2, 4, 0, 0, 0 },
        { 0x32480, 0x69880, 2, 3, 0, 0, 0 },
        { 0x9BD00, 0x9EB00, 2, 3, 0, 0, 0 },
    },
};

ARC_HEADER(3) AudiodataHeaderStart = {
    3, 0, NULL, 0, { 0 },
    {
        { 0x0, 0x85C0, 2, 0, 0, 0, 0 },
        { 0x85C0, 0x53C0, 2, 0, 0, 0, 0 },
        { 0xD980, 0x13A800, 2, 0, 0, 0, 0 },
    },
};

/* sequence -> bank map: one offset per sequence, then a count and the bank ids */
u8 AudiomapHeaderStart[] = {
    0x00, 0x16, 0x00, 0x19, 0x00, 0x1B, 0x00, 0x1D, 0x00, 0x1E, 0x00, 0x20, 0x00, 0x22, 0x00, 0x24, 0x00, 0x26, 0x00, 0x28, 0x00, 0x2A,
    0x02, 0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x05, 0x01, 0x04, 0x01, 0x05, 0x01, 0x04, 0x01, 0x05, 0x01, 0x03, 0x01, 0x04,
};
