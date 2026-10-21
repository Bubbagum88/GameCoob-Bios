#ifndef _JAUDIO_DSPROM_H
#define _JAUDIO_DSPROM_H

#include "jaudio/jaudio_types.h"

// JAudio DSP microcode (BS2 PAL 1.0: 0x1760 bytes at 813F4BA0). It is Nintendo
// data, so it is not in this repository: configure.py runs tools/extract_blobs.py,
// which writes it from your own bs2.dol to build/<version>/include/jaudio/jdsp.inc.
u16 jdsp[] ATTRIBUTE_ALIGN(32) = {
#include "jaudio/jdsp.inc"
};

#endif
