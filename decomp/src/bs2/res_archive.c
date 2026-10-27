#include <dolphin/types.h>

/* Resource archive linked into BS2's .data (textures, models, layouts and
   strings): a count, then one offset per resource; read through lang.c's
   ResArchive. The bytes are extracted from your bs2.dol at build time
   (config.yml "extract"), never stored in the repository. */
u8 lbl_81381820[0x6F680] ATTRIBUTE_ALIGN(32) = {
#include "bs2/res_archive.inc"
};
