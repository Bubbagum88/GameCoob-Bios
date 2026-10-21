#include "jaudio/virload.h"

#include "jaudio/aictrl.h"
#include "jaudio/dvdthread.h"

#include <stddef.h>
#include "jaudio/sdk.h"

// BS2's virload.c is a cut-down one: of Pikmin's file it keeps only the archive
// table and the two handle lookups below (no directory or archive names, no
// loaders), so their data (JV_DIR_NAME, JV_ARC_NAME, finfo) is not here either.

static Barc* JV_ARC[16]; // Pointers to BARC metadata (*.hed)

static u32 JV_CURRENT_ARCS = 0; // TODO: type unknown, init unclear

/**
 * @TODO: Documentation
 * BS2: 813534C0, size 0x80
 */
BarcEntry* JV_GetRealHandle(u32 handle)
{
	u32 unusedIndex;
	Barc* archiveHeader;
	u16 arcIdx;
	u16 seqIdx;

	seqIdx = (handle & 0x0000ffff);
	arcIdx = (handle & 0xffff0000) >> 16;

	if (arcIdx >= JV_CURRENT_ARCS) {
		return 0;
	}
	archiveHeader = JV_ARC[arcIdx];
	if (!archiveHeader) {
		return NULL;
	}
	if (seqIdx >= archiveHeader->meta.seqCount) {
		return NULL;
	}

	BarcEntry* entry = (&archiveHeader[seqIdx].entry) + 1; // skip header i guess?
	while (entry->isDummy != 0xFFFF) {
		// skip through any dummy tracks until we hit a real one (isDummy == 0xFFFF for real tracks)
		entry = (&archiveHeader[entry->isDummy].entry) + 1;
	}

	return entry;
}

/**
 * @TODO: Documentation
 * BS2: 81353540, size 0x34
 */
u32 JV_CheckSize(u32 handle)
{
	BarcEntry* entry;

	entry = JV_GetRealHandle(handle);
	if (!entry)
		return 0;
	return entry->size;
}
