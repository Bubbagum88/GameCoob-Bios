#include "jaudio/seqsetup.h"

#include "jaudio/dummyprobe.h"
#include "jaudio/driverinterface.h"
#include "jaudio/fat.h"
#include "jaudio/jammain_2.h"
#include "jaudio/jamosc.h"
#include "jaudio/noteon.h"
#include "jaudio/oneshot.h"
#include "jaudio/playercall.h"

#include "jaudio/sdk.h"
#include <stddef.h>

#include <stddef.h>

#define SEQ_SIZE             (256)
#define ROOT_OUTER_SIZE      (16)
#define ROOTSEQ_SIZE         (16)
#define FREE_SEQP_QUEUE_SIZE (256)

static seqp_ seq[SEQ_SIZE];
static OuterParam_ ROOT_OUTER[ROOT_OUTER_SIZE];
static seqp_* rootseq[ROOTSEQ_SIZE];
static seqp_* FREE_SEQP_QUEUE[FREE_SEQP_QUEUE_SIZE];

static u32 BACK_P;
static u32 GET_P;
static u32 SEQ_REMAIN;

/**
 * @TODO: Documentation
 * BS2: 81357CE0, size 0xa0
 */
void Jaq_Reset(void)
{
	int i;

	for (i = 0; i < SEQ_SIZE; ++i) {
		seq[i].trackState    = 0;
		seq[i].childMuteMask = 0;
		seq[i].isMuted       = 0;
		FREE_SEQP_QUEUE[i]   = &seq[i];
	}

	BACK_P     = 0;
	GET_P      = 0;
	SEQ_REMAIN = FREE_SEQP_QUEUE_SIZE;

	for (i = 0; i < ROOTSEQ_SIZE; ++i) {
		rootseq[i] = NULL;
	}

	Jam_InitRegistTrack();
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000008
 */
void Jaq_GetRemainFreeTracks(void)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 */
static BOOL BackTrack(seqp_* track)
{
	track->trackState = 0;
	if (track->isAllocated == 1) {
		if (SEQ_REMAIN == FREE_SEQP_QUEUE_SIZE) {
			OSReport("\201\364Jam/setup:: Error : Cannot back sequence structure %x\n", track);
			return FALSE;
		}

		FREE_SEQP_QUEUE[BACK_P] = track;
		++SEQ_REMAIN;
		++BACK_P;

		if (BACK_P == FREE_SEQP_QUEUE_SIZE) {
			BACK_P = 0;
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @TODO: Documentation
 * BS2: 81357E40, size 0x64
 */
static seqp_* GetNewTrack()
{
	seqp_* track;

	if (SEQ_REMAIN == 0) {
		return NULL;
	}

	track = FREE_SEQP_QUEUE[GET_P];
	++GET_P;
	--SEQ_REMAIN;

	if (GET_P == FREE_SEQP_QUEUE_SIZE) {
		GET_P = 0;
	}

	track->trackState  = 2;
	track->isAllocated = 1;
	return track;
}

/**
 * @TODO: Documentation
 * BS2: 81357EC0, size 0x48
 */
int AllocNewRoot(seqp_* track)
{
	int i;

	for (i = 0; i < ROOTSEQ_SIZE; ++i) {
		if (!rootseq[i]) {
			rootseq[i] = track;
			return i;
		}
	}
	return -1;
}

/**
 * @TODO: Documentation
 */
int DeAllocRoot(seqp_* track)
{
	int i;

	for (i = 0; i < ROOTSEQ_SIZE; ++i) {
		if (rootseq[i] == track) {
			rootseq[i] = NULL;
			// "returned the RootSeq"
			OSReport("\201\364Jam/Setup::RootSeq\202\360\225\324\213p\202\265\202\334\202\265\202\275\n");
			return i;
		}
	}
	// "could not return (DeAlloc) the RootSeq"
	OSReport("\201\364Jam/Setup:: Error : RootSeq\202\360\225\324\213p(DeAlloc)\202\305\202\253\202\334\202\271\202\361\n");
	return -1;
}

/**
 * @TODO: Documentation
 */
seqp_* Jaq_HandleToSeq(u32 handle)
{
	return rootseq[handle];
}

/**
 * @TODO: Documentation
 */
static void Init_Track(seqp_* track, u32 dataAddress, seqp_* parent)
{
	int i;

	if (!parent) {
		track->seqData          = (u8*)dataAddress;
		track->programCounter   = 0;
		track->tempo            = 120;
		track->timeBase         = 48;
		track->timeRelationMode = 1;
		track->isPaused         = FALSE;
		track->pauseStatus      = 10;
		track->childMuteMask    = 0;
		track->isMuted          = 0;
	} else {
		track->seqData          = parent->seqData;
		track->programCounter   = dataAddress;
		track->fileHandle       = parent->fileHandle;
		track->tempo            = parent->tempo;
		track->needsTempoSync   = FALSE;
		track->tempoFactor      = parent->tempoFactor;
		track->timeBase         = parent->timeBase;
		track->timeRelationMode = parent->timeRelationMode;
		track->isPaused         = parent->isPaused;
		track->pauseStatus      = parent->pauseStatus;
		track->childMuteMask    = 0;
	}
	track->callStackDepth  = 0;
	track->noteDuration    = 0;
	track->waitTimer       = 0;
	track->trackState      = 1;
	track->parent          = parent;
	track->interruptEnable = 0;
	track->interruptActive = 0;
	track->timer           = 0;

	// Initialize all MoveParams with default values.
	for (i = 0; i < 18; ++i) {
		track->timedParam.move[i].duration     = 0.0f;
		track->timedParam.move[i].currentValue = 1.0f;
		track->timedParam.move[i].targetValue  = 1.0f;
	}

	track->timedParam.inner.pitch.currentValue = 0.0f;
	track->timedParam.inner.pitch.targetValue  = 0.0f;
	track->timedParam.inner.pitch.currentValue = 0.0f; // Just to be sure.
	track->timedParam.inner.pitch.targetValue  = 0.0f; // Just to be sure.
	track->timedParam.inner.pan.currentValue   = 0.5f;
	track->timedParam.inner.pan.targetValue    = 0.5f;

	track->timedParam.inner.pauseVolumeFactor.currentValue = 0.5f;
	track->timedParam.inner.pauseVolumeFactor.targetValue  = 0.5f;
	track->timedParam.inner.panDelay.currentValue          = 0.0f;
	track->timedParam.inner.panDelay.targetValue           = 0.0f;

	track->timedParam.inner.fxmix.currentValue = 0.0f;
	track->timedParam.inner.fxmix.targetValue  = 0.0f;
	track->timedParam.inner.dolby.currentValue = 0.0f;
	track->timedParam.inner.dolby.targetValue  = 0.0f;

	// Initialize IIRs (skipping the first one)
	for (i = 1; i < 4; ++i) {
		track->timedParam.inner.IIRs[i].currentValue = 0.0f;
		track->timedParam.inner.IIRs[i].targetValue  = 0.0f;
	}

	track->timedParam.inner.distFilter.currentValue = 0.0f;
	track->timedParam.inner.distFilter.targetValue  = 0.0f;

	for (i = 0; i < 32; ++i) {
		track->regParam.reg[i] = 0;
	}

	Console_printf("Parent_Relation %d\n", track->flags);
	if ((track->flags & 2) || !track->parent) {
		track->regParam.param.arguments[0] = 0;
		track->regParam.param.arguments[1] = 1;
		track->regParam.param.arguments[2] = 1;
		track->regParam.param.arguments[3] = 0x7fff;
		track->regParam.param.arguments[4] = 0x4000;
		for (i = 0; i < 3; ++i) {
			track->panCalcTypes[i]                  = 2;
			track->parentPanCalcTypes[i]            = 2;
			track->parentController.panCalcTypes[i] = 26;
		}
		track->regParam.param.bankNumber   = 0xf0;
		track->regParam.param.pitchScale   = 0x0c;
		track->regParam.param.basePriority = 0x40;
	} else {
		for (i = 0; i < 5; ++i) {
			track->regParam.param.arguments[i] = track->parent->regParam.param.arguments[i];
		}
		track->regParam.param.bankNumber   = track->parent->regParam.param.bankNumber;
		track->regParam.param.pitchScale   = track->parent->regParam.param.pitchScale;
		track->regParam.param.basePriority = track->parent->regParam.param.basePriority;
		for (i = 0; i < 3; ++i) {
			track->panCalcTypes[i]                  = track->parent->panCalcTypes[i];
			track->parentPanCalcTypes[i]            = track->parent->parentPanCalcTypes[i];
			track->parentController.panCalcTypes[i] = track->parent->parentController.panCalcTypes[i];
		}
	}

	for (i = 0; i < 16; ++i) {
		track->childOuterParams[i] = NULL;
		track->children[i]         = 0;
	}

	for (i = 0; i < 8; ++i) {
		track->activeNotes[i]    = -1;
		track->channels[i]       = NULL;
		track->activeSoundIds[i] = 0;
	}
	track->noteFlags     = 0;
	track->lastNote      = 0;
	track->wasNotePlayed = 0;
	track->isGateMode    = FALSE;
	Osc_Init_Env(track);
	track->transpose      = 0;
	track->finalTranspose = 0;
	track->tickCounter    = -1;

	for (i = 0; i < 16; ++i) {
		track->trackPort[i].importFlag = 0;
		track->trackPort[i].exportFlag = 0;
	}

	track->isRegistered = 0;
}

/**
 * @TODO: Documentation
 * BS2: 81358340, size 0xa0
 */
BOOL Jaq_StopSeq(s32 index)
{
	seqp_* track;

	if (index == -1) {
		return FALSE;
	}
	track = rootseq[index];
	if (!track) {
		return FALSE;
	}

	switch (track->trackState) {
	case 0:
	{
		break;
	}
	case 2:
	{
		BackTrack(track);
		DeAllocRoot(track);
		break;
	}
	default:
	{
		track->trackState = 3;
		break;
	}
	}
	return TRUE;
}

/**
 * @TODO: Documentation
 * BS2: 813583E0, size 0x54
 */
static void __StopSeq(seqp_* track)
{
	SeqUpdate(track, 0);
	Jaq_CloseTrack(track);
	DeAllocRoot(track);
	if (track->dataSourceMode == 1) {
		FAT_FreeMemory(track->fileHandle);
	}
}

/**
 * @TODO: Documentation
 * BS2: 81358440, size 0x24
 */
s32 Jaq_SetSeqData(seqp_* track, u8* sequenceData, u32 sequenceSize, u32 sourceMode)
{
	return Jaq_SetSeqData_Limit(track, sequenceData, sequenceSize, sourceMode, 0);
}

/**
 * @TODO: Documentation
 */
s32 Jaq_SetSeqData_Limit(seqp_* track, u8* sequenceData, u32 sequenceSize, u32 sourceMode, u8 oneShotMode)
{
	u8* trackData;
	s32 root;
	BOOL enabled;

	if (!track) {
		enabled = OSDisableInterrupts();
		track   = GetNewTrack();
		OSRestoreInterrupts(enabled);
		if (!track) {
			// "failed to allocate the root sequence struct"
			OSReport("\203\213\201[\203g\203V\201[\203P\203\223\203X\215\\\221\242\221\314\202\314\212m\225\333\202\311\216\270\224s\202\265\202\334\202\265\202\275\n");
			return -1;
		}
		// "root NULL given => root sequence struct allocated"
		OSReport("\203\213\201[\203gNULL\216w\222\350\201\313\203\213\201[\203g\203V\201[\203P\203\223\203X\215\\\221\242\221\314\202\314\212m\225\333\202\311\220\254\214\367\202\265\202\334\202\265\202\275\n");
	} else {
		track->isAllocated = 0;
	}

	root = AllocNewRoot(track);
	if (root == -1) {
		// "the root sequence pointers are full"
		OSReport("\203\213\201[\203g\203V\201[\203P\203\223\203X\203|\203C\203\223\203^\202\252\202\242\202\301\202\317\202\242\202\305\202\267\n");
		return -1;
	}

	track->dataSourceMode = sourceMode;
	switch (sourceMode) {
	case 0:
	{
		trackData = sequenceData;
		break;
	}
	case 1:
	{
		track->fileHandle = FAT_AllocateMemory(sequenceSize);
		if (track->fileHandle == 0xffff) { // Isn't this literally impossible?
			OSReport("Error:: No FAT Memory\n");
			return -1;
		}
		FAT_StoreBlock(sequenceData, track->fileHandle, 0, sequenceSize);
		trackData = NULL;
		break;
	}
	case 2:
	{
		track->fileHandle = (u8)sequenceData;
		trackData         = NULL;
		break;
	}
	}
	track->trackId = root;
	track->flags   = 3;
	Init_Track(track, (u32)trackData, NULL);
	Jam_InitExtBuffer(&ROOT_OUTER[root]);
	Jam_AssignExtBuffer(track, &ROOT_OUTER[root]);
	Init_1shot(&track->parentController, oneShotMode);
	track->tempoAccumulator = 0.0f;
	track->tempoFactor      = 1.0f;
	Jam_UpdateTrackAll(track);
	track->trackState = 2;
	return root;
}

/**
 * @TODO: Documentation
 */
BOOL Jaq_SetBankNumber(seqp_* track, u8 bankNum)
{
	u8 lo;

	// Let's get ahead of ourselves here.
	lo = track->regParam.param.bankNumber & 0xFF;
	if (!track) {
		return FALSE;
	}

	track->regParam.param.bankNumber = (bankNum << 8) | lo;
	return TRUE;
}

static s32 Jaq_RootCallback(void* track);

/**
 * @TODO: Documentation
 */
BOOL Jaq_StartSeq(u32 trackId)
{
	seqp_* track;
	u8* pTrackState;

	if (trackId == -1) {
		OSReport("Jam/Seq::ROOT\202\252\212m\225\333\202\263\202\352\202\304\202\242\202\310\202\242\n"); // "ROOT is not allocated"
		return FALSE;
	}

	track = rootseq[trackId];
	if (!track) {
		OSReport("Jam/Seq::SEQP\202\252\223o\230^\202\263\202\352\202\304\202\242\202\310\202\242\n"); // "SEQP is not registered"
		return FALSE;
	};

	// This feels like a fakematch, but oh well.
	switch (*(pTrackState = &track->trackState)) {
	case 0:
	{
		OSReport("StartSeq:   \217\200\224\365\202\252\202\305\202\253\202\304\202\242\202\334\202\271\202\361\n"); // "not ready"
		return FALSE;
	}
	case 1:
	{
		OSReport("StartSeq:   \212\371\202\311\223\256\215\354\222\206\202\305\202\267\n"); // "already running"
		return FALSE;
	}
	case 3:
	{
		OSReport("StartSeq: \222\342\216~\222\206\202\305\202\267\n"); // "stopped"
		return FALSE;
	}
	case 2:
	{
		*pTrackState = 1;
	}
	}
	Jac_RegisterDspPlayerCallback(&Jaq_RootCallback, rootseq[trackId]);
	return TRUE;
}

/* Flags bit layout (u32):
 * -----------------------
 * Bits 0-3   : Index Selector (0 to 15)
 * Bit 5      : Direct Register Read Flag
 *               - If set, index is read from hardware register
 *               - Forces mode to 4
 * Bits 6-7   : Mode Flags (0 to 3)
 */

/**
 * @TODO: Documentation
 */
s32 Jaq_OpenTrack(seqp_* track, u32 flags, u32 source)
{

	seqp_* oldChildTrack;
	seqp_* newChildTrack;
	u8 childIndex;
	u8 trackFlags;
	u32 depth;

	childIndex = (flags & 0b00001111);
	trackFlags = (flags & 0b11000000) >> 6;
	if ((flags & 0b00100000)) {
		trackFlags = 4;
	}

	if ((u8)(flags & 0b00100000)) { // This u8 cast is a repeating pattern across JAudio... Mysterious.
		childIndex = Jam_ReadRegDirect(track, childIndex);
	}

	if (childIndex >= 16) {
		OSReport("\225s\220\263\202\310\203g\203\211\203b\203N\203i\203\223\203o %d\n", childIndex); // "invalid track number"
		return -1;
	}

	oldChildTrack = track->children[childIndex];
	if (oldChildTrack) {
		Jaq_CloseTrack(oldChildTrack);
	}

	newChildTrack = GetNewTrack();
	if (!newChildTrack) {
		OSReport("\203g\203\211\203b\203N\202\252\221\253\202\350\202\334\202\271\202\361\n"); // "not enough tracks"
		return -1;
	}

	track->children[childIndex] = newChildTrack;

	depth = track->trackId & 0xF0000000;
	if (depth >= 0x70000000) {
		// "nesting went past 8 levels, an invalid track ID was made"
		OSReport("\201\364Jam/Track/Init/Warn::\212K\221w\202\2528\222i\212K\202\360\222\264\202\246\202\304\202\265\202\334\202\242\201A\225s\220\263\202\310\203g\203\211\203b\203NID\202\252\220\266\220\254\202\263\202\352\202\334\202\265\202\275\n");
	}
	newChildTrack->trackId        = ((track->trackId << 4 | childIndex) & 0xFFFFFFF) | (depth + 0x10000000);
	newChildTrack->connectionId   = 0;
	newChildTrack->dataSourceMode = track->dataSourceMode;
	newChildTrack->flags          = trackFlags;

	Init_Track(newChildTrack, source, track);

	// Dev rolls "worst bit extraction method", asked to leave Nintendo EAD.
	newChildTrack->isMuted = newChildTrack->parent->isMuted | ((newChildTrack->parent->childMuteMask & (1 << childIndex)) >> childIndex);
	newChildTrack->outerParams = newChildTrack->parent->childOuterParams[childIndex];
	if (newChildTrack->outerParams) {
		++newChildTrack->outerParams->refCount;
	}

	Init_1shot(&newChildTrack->parentController, 0);
	Jam_UpdateTrackAll(newChildTrack);
	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 813588E0, size 0xb4
 */
void __AllNoteOff(seqp_* track)
{
	u32 i;

	if (!track->parent) {
		for (i = 0; i < 8; ++i) {
			NoteOFF_R(track, i, 10);
			track->activeNotes[i] = -1;
			track->channels[i]    = NULL;
		}
	} else {
		for (i = 0; i < 8; ++i) {
			NoteOFF(track, i);
			track->activeNotes[i] = -1;
			track->channels[i]    = NULL;
		}
	}
}

/**
 * @TODO: Documentation
 * BS2: 813589A0, size 0x120
 */
u32 Jaq_CloseTrack(seqp_* track)
{
	size_t i;

	// Specifically two separate conditional blocks, because why not.
	if (!track) {
		return 0;
	}
	if (track->trackState == 0) {
		return 0;
	}

	__AllNoteOff(track);
	BackTrack(track);

	for (i = 0; i < 16; ++i) {
		if (track->children[i]) {
			Jaq_CloseTrack(track->children[i]);
			track->children[i] = NULL;
		}
	}

	if (track->outerParams) {
		// This smells like refcounting.
		track->outerParams->refCount -= 1;
		track->outerParams = NULL;
	}

	for (i = 0; i < 16; ++i) {
		if (track->childOuterParams[i]) {
			track->childOuterParams[i]->isAssigned = FALSE;
			track->childOuterParams[i]             = NULL;
		}
	}
	track->isMuted       = 0;
	track->childMuteMask = 0;
	if (track->parent) {
		FixMoveChannelAll(&track->parentController, &track->parent->parentController);
	} else {
		FixReleaseChannelAll(&track->parentController);
	}
	Jam_UnRegistTrack(track);
	return 0;
}

/**
 * Note: While this function accepts `void*`, it really expects `seqp_*`.
 * BS2: 81358AC0, size 0xe8
 */
static s32 Jaq_RootCallback(void* VOID_track)
{
	seqp_* track;

	track = (seqp_*)VOID_track;
	if (track && track->trackState != 0) {
		if (track->trackState == 3) {
			__StopSeq(track);
			return -1;
		}

		track->tempoAccumulator += track->tempoFactor;
		if (track->tempoAccumulator < 1.0f) {
			SeqUpdate(track, 0);
		} else {
			while (track->tempoAccumulator >= 1.0f) {
				track->tempoAccumulator -= 1.0f;
				if (Jam_SeqmainNote(track, 0) == -1) {
					__StopSeq(track);
					return -1;
				}
			}

			SeqUpdate(track, 0);
		}
	} else {
		return -1;
	}

	return 0;
}

// IPL-only functions in this unit with no Pikmin counterpart, still assembly:
//   fn_81357D80 (size 0xa4)
//   fn_81357F20 (size 0x8c)
//   fn_81357FC0 (size 0x37c)
//   fn_81358480 (size 0x1ac)
//   fn_81358640 (size 0xf8)
//   fn_81358740 (size 0x188)
