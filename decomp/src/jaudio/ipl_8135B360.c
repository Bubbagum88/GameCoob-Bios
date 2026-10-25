#include "jaudio/jaudio_types.h"
#include "jaudio/virload.h"

// Lookups into the sound archive linked into the IPL, so JAudio reads its
// bank, wave and sequence files from memory instead of the disc.

BEGIN_SCOPE_EXTERN_C

#include <string.h>

// The sound archive linked into the IPL (ipl.bnk, ipl.ws, ipl_0.aw, iplrom.com).
extern u8 lbl_813F7620[];

static u32 sVirtualHandle;

/* 0x8135B360: where a file sits in the linked-in archive */
u8* fn_8135B360(immut char* path)
{
	u8* base = lbl_813F7620;
	char name[0x20];
	int i;

	for (i = strlen(path); i >= 0; i--) {
		if (path[i] == '/') {
			break;
		}
	}
	strcpy(name, &path[i + 1]);
	if (strcmp(name, "ipl.bnk") == 0) {
		return base;
	}
	if (strcmp(name, "ipl.ws") == 0) {
		return base + 0x8E0;
	}
	if (strcmp(name, "ipl_0.aw") == 0) {
		return base + 0xF60;
	}
	if (strcmp(name, "iplrom.com") == 0) {
		return base + 0xB3060;
	}
}

/* 0x8135B460: size of a file in the linked-in archive */
u32 fn_8135B460(immut char* path)
{
	char name[0x20];
	int i;

	for (i = strlen(path); i >= 0; i--) {
		if (path[i] == '/') {
			break;
		}
	}
	strcpy(name, &path[i + 1]);
	if (strcmp(name, "ipl.bnk") == 0) {
		return 0x8E0;
	}
	if (strcmp(name, "ipl.ws") == 0) {
		return 0x680;
	}
	if (strcmp(name, "ipl_0.aw") == 0) {
		return 0xB2100;
	}
	if (strcmp(name, "iplrom.com") == 0) {
		return 0xC40;
	}
}

/* 0x8135B540 */
s32 fn_8135B540(u32 id)
{
	if (sVirtualHandle == 0xFFFFFFFF) {
		return 0;
	}
	return JV_CheckSize(sVirtualHandle | id);
}

END_SCOPE_EXTERN_C
