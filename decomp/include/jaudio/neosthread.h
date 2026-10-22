#ifndef _JAUDIO_NEOSTHREAD_H
#define _JAUDIO_NEOSTHREAD_H

#include "jaudio/jaudio_types.h"

/////////// JAUDIO NEOS THREAD DEFINITIONS ///////////
// NEOS runs N64-style audio tasks on the CPU. BS2 only, not in Pikmin.

// Global functions (all C++, so no extern C wrap).
u32 Neos_Update(s16* dst);
void ImageLoaded(u32 param);
void* neosproc(void* param);

////////////////////////////////////////////////////

#endif
