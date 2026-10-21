#include <dolphin/card.h>
#include <string.h>

/**
 * @TODO: Documentation
 */
s32 __CARDGetStatusEx(s32 chan, s32 fileNo, CARDDir* dirent)
{
	if (fileNo < 0 || CARD_MAX_FILE <= fileNo) {
		return CARD_RESULT_FATAL_ERROR;
	}

	{
		CARDControl* card;
		CARDDirectoryBlock* dir;
		CARDDir* ent;
		s32 result = __CARDGetControlBlock(chan, &card);

		if (result < CARD_RESULT_READY) {
			return result;
		}

		dir    = __CARDGetDirBlock(card);
		ent    = &dir->entries[fileNo];
		result = __CARDAccess(ent);
		if (result == CARD_RESULT_NOPERM) {
			result = __CARDIsPublic(ent);
		}

		if (result >= CARD_RESULT_READY) {
			memcpy(dirent, ent, sizeof(CARDDir));
		}
		return __CARDPutControlBlock(card, result);
	}
}

/**
 * @TODO: Documentation
 */
s32 __CARDSetStatusExAsync(s32 chan, s32 fileNo, CARDDir* dirent, CARDCallback callback)
{
	CARDControl* card;
	CARDDirectoryBlock* dir;
	CARDDir* ent;
	s32 result;
	u8* p;
	s32 i;

	if (fileNo < 0 || CARD_MAX_FILE <= fileNo || dirent->fileName[0] == 0xFF || dirent->fileName[0] == 0) {
		return CARD_RESULT_FATAL_ERROR;
	}

	result = __CARDGetControlBlock(chan, &card);
	if (result < CARD_RESULT_READY) {
		return result;
	}

	dir    = __CARDGetDirBlock(card);
	ent    = &dir->entries[fileNo];
	result = __CARDAccess(ent);
	if (result < CARD_RESULT_READY) {
		return __CARDPutControlBlock(card, result);
	}

	for (p = dirent->fileName; p < (u8*)&dirent->time; p++) {
		if (*p != 0) {
			continue;
		}
		while (++p < (u8*)&dirent->time) {
			*p = 0;
		}
		break;
	}

	if (memcmp(ent->fileName, dirent->fileName, CARD_FILENAME_MAX) != 0 || memcmp(ent->gameName, dirent->gameName, 4) != 0
	    || memcmp(ent->company, dirent->company, 2) != 0) {
		for (i = 0; i < CARD_MAX_FILE; i++) {
			if (i != fileNo) {
				CARDDir* ent = &dir->entries[i];
				if (ent->gameName[0] != 0xFF && memcmp(ent->gameName, dirent->gameName, 4) == 0
				    && memcmp(ent->company, dirent->company, 2) == 0
				    && memcmp(ent->fileName, dirent->fileName, CARD_FILENAME_MAX) == 0) {
					return __CARDPutControlBlock(card, CARD_RESULT_EXIST);
				}
			}
		}
		memcpy(ent->fileName, dirent->fileName, CARD_FILENAME_MAX);
		memcpy(ent->gameName, dirent->gameName, 4);
		memcpy(ent->company, dirent->company, 2);
	}

	ent->time         = dirent->time;
	ent->bannerFormat = dirent->bannerFormat;
	ent->iconAddr     = dirent->iconAddr;
	ent->iconFormat   = dirent->iconFormat;
	ent->iconSpeed    = dirent->iconSpeed;
	ent->commentAddr  = dirent->commentAddr;
	ent->permission   = dirent->permission;
	ent->copyTimes    = dirent->copyTimes;

	result = __CARDUpdateDir(chan, callback);
	if (result < CARD_RESULT_READY) {
		__CARDPutControlBlock(card, result);
	}
	return result;
}

/**
 * @TODO: Documentation
 */
s32 __CARDSetStatusEx(s32 chan, s32 fileNo, CARDDir* dirent)
{
	s32 result = __CARDSetStatusExAsync(chan, fileNo, dirent, __CARDSyncCallback);
	if (result < CARD_RESULT_READY) {
		return result;
	}

	return __CARDSync(chan);
}
