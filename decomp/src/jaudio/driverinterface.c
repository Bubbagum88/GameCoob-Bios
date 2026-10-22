#include "jaudio/driverinterface.h"

#include "jaudio/aictrl.h"
#include "jaudio/audiostruct.h"
#include "jaudio/bankdrv.h"
#include "jaudio/dspdriver.h"
#include "jaudio/dspinterface.h"
#include "jaudio/ja_calc.h"
#include <stddef.h>

#include "jaudio/dummyprobe.h"
#include "jaudio/sdk.h"

#define CHANNEL_SIZE (0x100)

static jcs_ GLOBAL_CHANNEL;
static jc_ CHANNEL[CHANNEL_SIZE] ATTRIBUTE_ALIGN(32);

static u16 MAX_MIXERLEVEL  = 12000;
u32 JAC_SYSTEM_OUTPUT_MODE = 1;

static u32 cur_waits;
static u32 cur_top;
static u32 cur_tail;
static jc_* waitp[0x20] ATTRIBUTE_ALIGN(32);
static int waittime[0x20];

static u8 calc_sw_table[]
    = { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 2, 0, 1, 0, 0, 2, 1, 0, 2, 2, 1, 0, 0, 1, 0, 1, 1, 0, 2, 1, 1, 0, 1, 1,
	    1, 1, 1, 2, 1, 2, 0, 1, 2, 1, 1, 2, 2, 1, 0, 0, 2, 0, 1, 2, 0, 2, 2, 1, 0, 2, 1, 1, 2, 1, 2, 2, 2, 0, 2, 2, 1, 2, 2, 2 };

// forward declarations
static void Cancel_WaitDSPChannel(jc_* jc);

/**
 * @TODO: Documentation
 * BS2: 8134E520, size 0x24
 */
void Channel_SetMixerLevel(f32 mix)
{
	MAX_MIXERLEVEL = mix * 16384.0f;
}

/**
 * @TODO: Documentation
 * BS2: 8134E560, size 0xc
 */
jcs_* Get_GlobalHandle()
{
	return &GLOBAL_CHANNEL;
}

/**
 * @TODO: Documentation
 * BS2: 8134E580, size 0x20
 */
int List_CountChannel(jc_** jc)
{
	jc_* chan = *jc;
	int num   = 0;

	while (TRUE) {
		if (chan == NULL) {
			break;
		}
		chan = chan->nextChan;
		num++;
	}

	return num;
}

/**
 * @TODO: Documentation
 * BS2: 8134E5A0, size 0x44 (unused in Pikmin)
 */
int Check_GlobalActiveChannel()
{
	int active    = List_CountChannel(&GLOBAL_CHANNEL.activeChannels);
	int releasing = List_CountChannel(&GLOBAL_CHANNEL.releasingChannels);
	STACK_PAD_VAR(4);
	return active + releasing;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00002C
 */
void Check_GlobalRelease(jc_** jc)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00006C
 * Not in BS2's text (dead-stripped), but its two strings head BS2's rodata, so this
 * debug dump is a guess that only has to emit them first.
 */
void List_GlobalChannel()
{
	jc_* jc;
	for (jc = GLOBAL_CHANNEL.releasingChannels; jc; jc = jc->nextChan) {
		OSReport("Rel.Jc (%x) Have DSPch  %x\n", jc, jc->dspChannel);
		if (jc->dspChannel) {
			OSReport(" DSPch Sign/Stat/CB is %x, %x, %x\n", jc->dspChannel->logicalChan, jc->dspChannel->allocState,
			         jc->dspChannel->logicalChanCb);
		}
	}
}

/**
 * @TODO: Documentation
 */
int List_CutChannel(jc_* jc)
{
	jc_* chan = *jc->chanListHead;
	int num   = 0;

	if (chan == jc) {
		*jc->chanListHead = jc->nextChan;
		jc->chanListHead  = NULL;
		return 0;
	}

	while (TRUE) {
		num++;
		if (chan == NULL) {
			Console_printf("Error: No Member\n");
			return -1;
		}

		if (chan->nextChan == jc) {
			break;
		}

		chan = chan->nextChan;
	}

	chan->nextChan   = jc->nextChan;
	jc->chanListHead = NULL;

	return num;
}

/**
 * @TODO: Documentation
 * BS2: 8134E6A0, size 0x30
 */
jc_* List_GetChannel(jc_** jc)
{
	jc_* chan = *jc;

	if (chan == NULL) {
		return NULL;
	}

	*jc                = chan->nextChan;
	chan->chanListHead = NULL;
	return chan;
}

/**
 * @TODO: Documentation
 */
void List_AddChannelTail(jc_** jc, jc_* in)
{
	jc_* chan = *jc;
	if (in->chanListHead) {
		OSReport("ROOTJC Error\n");
	}
	in->chanListHead = jc;

	if (chan == NULL) {
		*jc          = in;
		in->nextChan = NULL;
		return;
	}

	jc_* next;
	while (TRUE) {
		next = chan->nextChan;
		if (next == NULL) {
			chan->nextChan = in;
			in->nextChan   = NULL;
			return;
		}
		chan = next;
	}
}

/**
 * @TODO: Documentation
 */
void List_AddChannel(jc_** jc, jc_* in)
{
	jc_* chan = *jc;
	if (in->chanListHead) {
		OSReport("ROOTJC Error2\n");
	}
	in->chanListHead = jc;
	*jc              = in;
	in->nextChan     = chan;
}

/**
 * @TODO: Documentation
 */
int FixAllocChannel(jcs_* sys, u32 size)
{
	int num;
	Console_printf("Jcs %x : Allocate %d Channels \n", sys, size);
	num = 0;
	while (num < size) {
		jc_* chan = List_GetChannel(&GLOBAL_CHANNEL.freeChannels);
		if (chan == NULL) {
			break;
		}

		List_AddChannel(&sys->freeChannels, chan);
		chan->chanMgr = sys;
		Channel_Init(chan);
		num++;
	}

	sys->chanCount += num;
	GLOBAL_CHANNEL.chanCount -= num;
	return num;
}

/**
 * @TODO: Documentation
 * BS2: 8134E8A0, size 0x60
 */
int FixReleaseChannel(jc_* chan)
{
	List_AddChannel(&GLOBAL_CHANNEL.freeChannels, chan);
	chan->chanMgr->chanCount--;
	GLOBAL_CHANNEL.chanCount++;
	chan->chanMgr = &GLOBAL_CHANNEL;
	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8134E900, size 0xf0
 */
int FixReleaseChannelAll(jcs_* sys)
{
	jc_* chan;

	while (TRUE) {
		chan = List_GetChannel(&sys->freeChannels);
		if (chan == NULL) {
			break;
		}
		List_AddChannel(&GLOBAL_CHANNEL.freeChannels, chan);
		chan->chanMgr = &GLOBAL_CHANNEL;
	}

	while (TRUE) {
		chan = List_GetChannel(&sys->activeChannels);
		if (chan == NULL) {
			break;
		}
		List_AddChannel(&GLOBAL_CHANNEL.activeChannels, chan);
		chan->chanMgr = &GLOBAL_CHANNEL;
	}

	while (TRUE) {
		chan = List_GetChannel(&sys->releasingChannels);
		if (chan == NULL) {
			break;
		}
		List_AddChannel(&GLOBAL_CHANNEL.releasingChannels, chan);
		chan->chanMgr = &GLOBAL_CHANNEL;
	}

	while (TRUE) {
		chan = List_GetChannel(&sys->waitingChannels);
		if (chan == NULL) {
			break;
		}
		Cancel_WaitDSPChannel(chan);
		List_AddChannel(&GLOBAL_CHANNEL.freeChannels, chan);
		chan->chanMgr = &GLOBAL_CHANNEL;
	}

	GLOBAL_CHANNEL.chanCount += sys->chanCount;
	sys->chanCount = 0;
	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8134EA00, size 0xec
 */
int FixMoveChannelAll(jcs_* sys, jcs_* sys2)
{
	jc_* chan;

	while (TRUE) {
		chan = List_GetChannel(&sys->freeChannels);
		if (chan == NULL) {
			break;
		}
		List_AddChannel(&sys2->freeChannels, chan);
		chan->chanMgr = sys2;
	}

	while (TRUE) {
		chan = List_GetChannel(&sys->activeChannels);
		if (chan == NULL) {
			break;
		}
		List_AddChannel(&sys2->activeChannels, chan);
		chan->chanMgr = sys2;
	}

	while (TRUE) {
		chan = List_GetChannel(&sys->releasingChannels);
		if (chan == NULL) {
			break;
		}
		List_AddChannel(&sys2->releasingChannels, chan);
		chan->chanMgr = sys2;
	}

	while (TRUE) {
		chan = List_GetChannel(&sys->waitingChannels);
		if (chan == NULL) {
			break;
		}
		List_AddChannel(&sys2->waitingChannels, chan);
		chan->chanMgr = sys2;
	}

	sys2->chanCount += sys->chanCount;
	sys->chanCount = 0;
	sys2->chanAllocCount += sys->chanAllocCount;
	sys->chanAllocCount = 0;
	return 0;

	STACK_PAD_VAR(2);
}

/**
 * @TODO: Documentation
 * BS2: 8134EB00, size 0x70
 */
static f32 PanCalc(const PanMatrix_* mtx1, const PanMatrix_* mtx2, u8 a)
{
	f32 calc = 0.0f;

	u8* vals = calc_sw_table + (a * 3);

	for (int i = 0; i < 3; i++) {
		switch (vals[i]) {
		case 0:
		{
			break;
		}
		case 1:
		{
			calc += mtx1->values[i];
			break;
		}
		case 2:
		{
			calc += (mtx1->values[i] * mtx2->values[i]);
			break;
		}
		}
	}
	return calc;
}

/**
 * @TODO: Documentation
 * BS2: 8134EB80, size 0x104
 */
void InitJcs(jcs_* sys)
{
	sys->freeChannels      = NULL;
	sys->activeChannels    = NULL;
	sys->releasingChannels = NULL;
	sys->waitingChannels   = NULL;
	sys->chanAllocCount    = 0;
	sys->chanCount         = 0;
	sys->voiceStealingMode = 1;
	sys->volume            = 1.0f;
	sys->pitch             = 1.0f;
	sys->pan               = 0.5f;
	sys->fxmix             = 0.0f;
	sys->dolby             = 0.0f;

	int i;

	for (i = 0; i < 8; i++) {
		sys->firCoefficients[i] = 0;
	}

	sys->firCoefficients[0] = 0x7fff;
	sys->distFilter         = 0;

	for (i = 0; i < 4; i++) {
		sys->iirCoefficients[i] = 0;
		sys->masterLevels[i]    = 0;
	}

	sys->maxDelay           = 0;
	sys->iirCoefficients[0] = 0x7fff;
	sys->filterMode         = 0;
	sys->busConnect[0]      = 0x150;
	sys->busConnect[1]      = 0x210;
	sys->busConnect[2]      = 0x352;
	sys->busConnect[3]      = 0x412;
	sys->busConnect[4]      = 0;
	sys->busConnect[5]      = 0;
	sys->channelPriority    = 0x20103;
	sys->releaseTime        = 600;
	sys->panCalcTypes[0]    = 26;
	sys->panCalcTypes[1]    = 1;
	sys->panCalcTypes[2]    = 1;
}

/**
 * @TODO: Documentation
 * BS2: 8134ECA0, size 0x138
 */
void Channel_Init(jc_* jc)
{
	jc->updateCallback    = NULL;
	jc->pitchSweepUpdater = NULL;
	jc->noteId            = 0;
	jc->lastNotePlayed    = 0;
	jc->waveData          = NULL;
	jc->logicalChanType   = 0;
	jc->chanData          = 0;
	jc->_18               = 0;
	jc->_1C               = 0;

	if (jc->chanMgr == NULL) {
		jc->busRouting[0].whole = 0x150;
		jc->busRouting[1].whole = 0x210;
		jc->busRouting[2].whole = 0x352;
		jc->busRouting[3].whole = 0x412;
		jc->busRouting[4].whole = 0;
		jc->busRouting[5].whole = 0;
		jc->channelPriority     = 0x10101;
		jc->releaseTime         = 0x258;
		jc->panCalcTypes[0]     = 0x1A;
		jc->panCalcTypes[1]     = 1;
		jc->panCalcTypes[2]     = 1;
	} else {
		for (int i = 0; i < 6; i++) {
			jc->busRouting[i].whole = jc->chanMgr->busConnect[i];
		}

		jc->channelPriority = jc->chanMgr->channelPriority;
		jc->releaseTime     = jc->chanMgr->releaseTime;

		for (int i = 0; i < 3; i++) {
			jc->panCalcTypes[i] = jc->chanMgr->panCalcTypes[i];
		}
	}

	for (int i = 0; i < 4; i++) {
		jc->mOscillators[i] = NULL;
	}

	jc->pauseFlag = 0;
	jc->channelId++;
	if ((int)jc->channelId == 0) {
		jc->channelId = 1;
	}
}

/**
 * @TODO: Documentation
 * BS2: 8134EDE0, size 0x30
 */
static void Channel_FirstInit(jc_* jc)
{
	jc->dspChannel = NULL;
	jc->nextChan   = NULL;
	jc->chanMgr    = NULL;
	Channel_Init(jc);
}

/**
 * @TODO: Documentation
 * BS2: 8134EE20, size 0x7c
 */
void InitGlobalChannel()
{
	jcs_* global_channel;
	int i;

	global_channel = &GLOBAL_CHANNEL;
	InitJcs(global_channel);

	for (i = 0; i < CHANNEL_SIZE; i++) {
		Channel_FirstInit(&CHANNEL[i]);
		List_AddChannel(&global_channel->freeChannels, &CHANNEL[i]);
		CHANNEL[i].chanMgr = global_channel;
	}

	global_channel->chanCount = CHANNEL_SIZE;
}

/**
 * @TODO: Documentation
 * BS2: 8134EEA0, size 0xd8
 */
static void __UpdateJcToDSPInit(jc_* jc)
{
	int buf = jc->dspChannel->buffer_idx;
	DSP_SetMixerInitDelayMax(buf, jc->chanMgr->maxDelay);

	for (u32 i = 0; i < 6; i++) {
		DSP_SetMixerInitVolume(buf, i, jc->mixerLevels[i], jc->chanMgr->masterLevels[i]);
	}

	DSP_SetPitch(buf, jc->finalPitch);

	if (jc->chanMgr->filterMode & 0x20) {
		DSP_SetIIRFilterParam(buf, jc->chanMgr->iirCoefficients);
	}

	if (jc->chanMgr->filterMode & 0x1f) {
		DSP_SetFIR8FilterParam(buf, jc->chanMgr->firCoefficients);
	}

	DSP_SetFilterMode(buf, jc->chanMgr->filterMode);
	DSP_SetPauseFlag(buf, jc->pauseFlag);
}

/**
 * @TODO: Documentation
 * BS2: 8134EF80, size 0xd8
 */
static void __UpdateJcToDSP(jc_* jc)
{
	u8 dspBufferIndex;

	dspBufferIndex = jc->dspChannel->buffer_idx;
	for (u32 i = 0; i < 6; ++i) {
		DSP_SetMixerVolume(dspBufferIndex, i, jc->mixerLevels[i], jc->chanMgr->masterLevels[i]);
	}

	DSP_SetPitch(dspBufferIndex, jc->finalPitch);

	if ((jc->chanMgr->filterMode & 0x20) != 0) {
		DSP_SetIIRFilterParam(dspBufferIndex, jc->chanMgr->iirCoefficients);
	}

	if ((jc->chanMgr->filterMode & 0x1f) != 0) {
		DSP_SetFIR8FilterParam(dspBufferIndex, jc->chanMgr->firCoefficients);
	}

	DSP_SetFilterMode(dspBufferIndex, jc->chanMgr->filterMode);
	DSP_SetDistFilter(dspBufferIndex, jc->chanMgr->distFilter);
	DSP_SetPauseFlag(dspBufferIndex, jc->pauseFlag);
}

/**
 * @TODO: Documentation
 * BS2: 8134F060, size 0x38
 */
void UpdateJcToDSP(jc_* jc)
{
	__UpdateJcToDSP(jc);
	DSP_FlushChannel(jc->dspChannel->buffer_idx);
}

/**
 * @TODO: Documentation
 * BS2: 8134F0A0, size 0x348
 */
void UpdateEffecterParam(jc_* jc)
{
	f32 pan;
	f32 volume;
	f32 fxmix;
	f32 dolby = 0.0f;
	f32 angle;

	f32 tmp;
	if (jc->lastManager == jc->chanMgr) {
		jc->managerPitch             = jc->chanMgr->pitch;
		jc->managerVolume            = jc->chanMgr->volume;
		jc->panMatrices[1].values[2] = jc->chanMgr->pan;
		jc->panMatrices[2].values[2] = jc->chanMgr->fxmix;
		jc->panMatrices[3].values[2] = jc->chanMgr->dolby;

		for (u32 i = 0; i < 3; i++) {
			jc->panCalcTypes[i] = jc->chanMgr->panCalcTypes[i];
		}
	}

	switch (JAC_SYSTEM_OUTPUT_MODE) {
	case 0:
	{
		pan   = 0.5f;
		dolby = 0.0f;
		fxmix = PanCalc(&jc->panMatrices[2], &jc->panMatrices[0], jc->panCalcTypes[1]);
		break;
	}
	case 1:
	{
		if (jc->panCalcTypes[0] == 0) {
			pan = 0.5f;
		} else {
			pan = PanCalc(&jc->panMatrices[1], &jc->panMatrices[0], jc->panCalcTypes[0]);
		}
		fxmix = PanCalc(&jc->panMatrices[2], &jc->panMatrices[0], jc->panCalcTypes[1]);
		dolby = PanCalc(&jc->panMatrices[3], &jc->panMatrices[0], jc->panCalcTypes[2]);
		break;
	}
	}

	volume = jc->currentVolume * jc->volumeModifier * jc->managerVolume;

	if (pan < 0.0f) {
		pan = 0.0f;
	} else if (pan > 1.0f) {
		pan = 1.0f;
	}

	if (fxmix < 0.0f) {
		fxmix = 0.0f;
	} else if (fxmix > 1.0f) {
		fxmix = 1.0f;
	}

	if (dolby < 0.0f) {
		dolby = 0.0f;
	} else if (dolby > 1.0f) {
		dolby = 1.0f;
	}

	jc->finalPitch = jc->currentPitch * jc->pitchModifier * jc->managerPitch * 4096.0f;

	for (u32 i = 0; i < 6; i++) {
		f32 tmp          = volume;
		MixConfig config = jc->busRouting[i];

		if (config.parts.upper == 0) {
			jc->mixerLevels[i] = 0;
			continue;
		}

		f32 angle;
		if (config.parts.lower0) {
			switch (config.parts.lower0) {
			case 1:
			{
				angle = pan;
				break;
			}
			case 2:
			{
				angle = fxmix;
				break;
			}
			case 3:
			{
				angle = dolby;
				break;
			}
			case 5:
			{
				angle = 1.0f - pan;
				break;
			}
			case 6:
			{
				angle = 1.0f - fxmix;
				break;
			}
			case 7:
			{
				angle = 1.0f - dolby;
				break;
			}
			}
			tmp *= sinf3(angle);
		}

		if (config.parts.lower1) {
			switch (config.parts.lower1) {
			case 1:
			{
				angle = pan;
				break;
			}
			case 2:
			{
				angle = fxmix;
				break;
			}
			case 3:
			{
				angle = dolby;
				break;
			}
			case 5:
			{
				angle = 1.0f - pan;
				break;
			}
			case 6:
			{
				angle = 1.0f - fxmix;
				break;
			}
			case 7:
			{
				angle = 1.0f - dolby;
				break;
			}
			}
			tmp *= sinf3(angle);
		}

		if (tmp < 0.0f) {
			tmp = 0.0f;
		} else if (tmp > 1.0f) {
			tmp = 1.0f;
		}
		jc->mixerLevels[i] = tmp * (f32)MAX_MIXERLEVEL;
	}
}

/**
 * @TODO: Documentation
 * BS2: 8134F400, size 0x68
 */
void DoEffectOsc(jc_* jc, u8 id, f32 val)
{
	switch (id) {
	case 1:
	{
		jc->pitchModifier *= val;
		break;
	}
	case 0:
	{
		jc->volumeModifier *= val;
		break;
	}
	case 2:
	{
		jc->panMatrices[1].values[1] = val;
		break;
	}
	case 3:
	{
		jc->panMatrices[2].values[1] = val;
		break;
	}
	case 4:
	{
		jc->panMatrices[3].values[1] = val;
		break;
	}
	}
}

/**
 * @TODO: Documentation
 */
static void KillBrokenLogicalChannels(dspch_* ch)
{
	size_t i;
	jc_* chan;

	for (i = 0; i < CHANNEL_SIZE; i++) {
		chan = &CHANNEL[i];
		if (chan->dspChannel == ch) {
			StopLogicalChannel(chan);
			if (List_CutChannel(chan) != -1) {
				List_AddChannel(&chan->chanMgr->freeChannels, chan);
			}
		}
	}
}

/**
 * @TODO: Documentation
 */
static int CommonCallbackLogicalChannel(dspch_* ch, u32 eventType)
{
	jc_* jc;
	u32 oscIndex;
	u32 activeOscCount;
	STACK_PAD_VAR(2);

	activeOscCount = 0;
	jc             = ch->logicalChan;

	// Verify channel relationship integrity
	if (jc->dspChannel != ch) {
		// "logical channel and physical channel link is broken"
		OSReport("\230_\227\235\203`\203\203\203\223\203l\203\213\202\306\225\250\227\235\203`\203\203\203\223\203l\203\213\202\314\214\213\215\207\224j\222]\n");
		OSReport("PHY. %x Log->phy. %x\n", ch, jc->dspChannel);
		OSReport("Log. %x phy->log. %x\n", jc, ch->logicalChan);
		if (jc->dspChannel) {
			OSReport("Log->phy->log  %x\n", jc->dspChannel->logicalChan);
		}
		ForceStopDSPchannel(ch);
		StopLogicalChannel(jc);
		if (List_CutChannel(jc) == -1) {
			OSReport("Cut\202\305\202\253\202\310\202\242\n"); // "can't cut"
		} else {
			List_AddChannel(&jc->chanMgr->freeChannels, jc);
		}

		ch->logicalChan = 0;
		return DSPCHAN_CALLBACK_STOP;
	}

	// Handle completion event
	if (eventType == DSPCHCB_Completion) {
		Console_printf("DSPSTOP (JC= %x)\n", jc);
		Console_printf("-->call %8x\n", jc->updateCallback);
		if (jc->updateCallback) {
			jc->updateCallback(jc, JCSTAT_Unk1);
		} else {
			StopLogicalChannel(jc);
			if (List_CutChannel(jc) == -1) {
				OSReport("Cut\202\305\202\253\202\310\202\242\n");
			} else {
				List_AddChannel(&jc->chanMgr->freeChannels, jc);
			}
		}

		return DSPCHAN_CALLBACK_STOP;
	}

	// Check if wave data is loaded, force stop if not
	if (jc->waveData && !*jc->waveData->fileLoadStatus) {
		ForceStopDSPchannel(jc->dspChannel);
		return DSPCHAN_CALLBACK_FORCE_STOP;
	}

	// Handle priority update event
	if (eventType == DSPCHCB_PriorityUpdate) {
		u8 prio = jc->channelPriority >> 16;
		if (jc->dspChannel && prio < jc->dspChannel->prio) {
			jc->dspChannel->prio = prio;
		}

		return DSPCHAN_CALLBACK_STOP;
	}

	// Handle state change event - move to waiting list
	if (eventType == DSPCHCB_StateChange) {
		jc->mOscBuffers[0].state = 6;
		if (List_CutChannel(jc) == -1) {
			Console_printf("\201\364Jesus/DrvIF/CommonCB:: Error: Cut\202\305\202\253\202\334\202\271\202\361\n");
			return DSPCHAN_CALLBACK_CONTINUE;
		}

		List_AddChannel(&jc->chanMgr->waitingChannels, jc);
		eventType = 0;
		// "starting FORCESTOP"
		Console_printf("--------FORCESTOP \212J\216n\202\265\202\334\202\267 (PHY.%x ) (JC =%x) \n", ch, jc);
	}

	// Handle normal update processing
	if (eventType == DSPCHCB_NormalUpdate) {
		// Reset modifiers to default values
		jc->pitchModifier            = 1.0f;
		jc->volumeModifier           = 1.0f;
		jc->panMatrices[1].values[1] = 0.5f;
		jc->panMatrices[2].values[1] = 0.0f;
		jc->panMatrices[3].values[1] = 0.0f;

		// Process all oscillators
		for (oscIndex = 0; oscIndex < 4; oscIndex++) {
			if (jc->mOscillators[oscIndex]) {
				DoEffectOsc(jc, jc->mOscillators[oscIndex]->mode, Bank_OscToOfs(jc->mOscillators[oscIndex], &jc->mOscBuffers[oscIndex]));

				// Check if oscillator finished
				if (jc->mOscBuffers[oscIndex].state == 0) {
					// "envelope %d stopped"
					Console_printf("\201\364Jesus/DrvIF/Callback::\203G\203\223\203x\203\215\201[\203v %d \222\342\216~\202\265\202\334\202\265\202\275\n", oscIndex);
					// "FINISH request from the oscillator"
					Console_printf("\201\364Jesus/DRVINT:\203I\203V\203\214\201[\203^ \202\251\202\347\202\314FINISH\203\212\203N\203G\203X\203g\224\255\220\266\n");
					if (jc->updateCallback == NULL) {
						// No callback - stop channel immediately
						OSReport("Error: JC\222\342\216~\202\314\202\275\202\337\202\314UPDATECALL\202\252NULL\202\305\202\305\202\253\202\310\202\242(JC %x)\n", jc);
						if (StopLogicalChannel(jc) == FALSE) {
							DSP_PlayStop(ch->buffer_idx);
							DSP_FlushChannel(ch->buffer_idx);
						}

						if (List_CutChannel(jc) == -1) {
							OSReport("Cut\202\305\202\253\202\310\202\242\n");
						} else {
							List_AddChannel(&jc->chanMgr->freeChannels, jc);
						}
						return DSPCHAN_CALLBACK_STOP;
					} else {
						// Has callback - notify completion
						jc->updateCallback(jc, JCSTAT_Unk2);
						return DSPCHAN_CALLBACK_STOP;
					}
				}
				activeOscCount++;
			}
		}

		// Update effects if any oscillators are active
		if (activeOscCount) {
			UpdateEffecterParam(jc);
			jc->toFlush = 1;
		}

		// Handle pitch sweep updates
		if (jc->pitchSweepUpdater && (u32)jc->pitchSweepUpdater(jc, JCSTAT_Unk0) == TRUE) {
			jc->toFlush++;
		}

		// Early return if no update callback
		if (jc->updateCallback == NULL) {
			return DSPCHAN_CALLBACK_CONTINUE;
		}

		// Decrement note timer
		if (jc->lastNotePlayed > 0) {
			jc->lastNotePlayed--;
		}
	}

	// Handle callback timing and notification
	if (jc->lastNotePlayed == 0) {
		// "UPDATE request from the timer"
		Console_printf("\201\364Jesus/DRVINT:\203^\203C\203}\201[ \202\251\202\347\202\314UPDATE\203\212\203N\203G\203X\203g\224\255\220\266\n");
		jc->updateCallback(jc, JCSTAT_Unk0);
		jc->lastNotePlayed = jc->noteId;
	}

	// Flush changes to DSP if needed
	if (jc->toFlush) {
		UpdateJcToDSP(jc);
		jc->toFlush = 0;
	}

	return DSPCHAN_CALLBACK_CONTINUE;
}

/**
 * @TODO: Documentation
 */
BOOL StopLogicalChannel(jc_* jc)
{
	dspch_* ch = jc->dspChannel;
	if (ch == NULL) {
		// "no physical DSP channel is assigned"
		Console_printf("\201\364Jesus/DrvIF/Stop:: \225\250\227\235DSP\203`\203\203\203\223\203l\203\213\202\252\203A\203T\203C\203\223\202\263\202\352\202\304\202\242\202\334\202\271\202\361\n");
		Console_printf("---------------- \225\250\227\235DSP\203`\203\203\203\223\203l\203\213\202\252\203A\203T\203C\203\223\202\263\202\352\202\304\202\242\202\334\202\271\202\361\n");
		return FALSE;
	}

	ch->logicalChanCb             = NULL;
	jc->dspChannel->callbackTimer = 0;
	DSP_PlayStop(jc->dspChannel->buffer_idx);
	DSP_FlushChannel(jc->dspChannel->buffer_idx);
	if ((u32)DeAllocDSPchannel(jc->dspChannel, (u32)jc) != 0) {
		// "failed to release the physical voice"
		Console_printf("-----------------: \225\250\227\235\203{\203C\203X\202\314\225\324\213p\202\311\216\270\224s\202\265\202\334\202\265\202\275\n");
	}
	jc->dspChannel = NULL;
	return TRUE;
}

/**
 * @TODO: Documentation
 */
BOOL CheckLogicalChannel(jc_* jc)
{
	if (jc->waveData == NULL && jc->logicalChanType == 0) {
		// "no waveform is assigned"
		Console_printf("\201\364Jesus/DrvIF/Play:: \224g\214`\202\252\203A\203T\203C\203\223\202\263\202\352\202\304\202\242\202\334\202\271\202\361\n");
		return FALSE;
	}
	return TRUE;
}

/**
 * @TODO: Documentation
 */
BOOL PlayLogicalChannel(jc_* jc)
{
	if (jc->dspChannel == NULL) {
		// "no physical DSP channel is assigned"
		Console_printf("\201\364Jesus/DrvIF/Play:: \225\250\227\235DSP\203`\203\203\203\223\203l\203\213\202\252\203A\203T\203C\203\223\202\263\202\352\202\304\202\242\202\334\202\271\202\361\n");
		return FALSE;
	}
	if (CheckLogicalChannel(jc) == FALSE) {
		return FALSE;
	}

	jc->dspChannel->logicalChanCb = CommonCallbackLogicalChannel;
	jc->dspChannel->callbackTimer = 1;

	switch (jc->logicalChanType) {
	case 0:
	{
		DSP_SetWaveInfo(jc->dspChannel->buffer_idx, jc->waveData, jc->chanData);
		break;
	}
	case 1:
	{
		break;
	}
	case 2:
	{
		DSP_SetOscInfo(jc->dspChannel->buffer_idx, jc->chanData);
		break;
	}
	}

	for (u32 i = 0; i < 6; i++) {
		MixConfig bus = jc->busRouting[i];
		DSP_SetBusConnect(jc->dspChannel->buffer_idx, i, bus.parts.upper);
	}

	jc->lastManager = jc->chanMgr;
	UpdateEffecterParam(jc);
	__UpdateJcToDSPInit(jc);
	jc->dspChannel->prio        = jc->channelPriority;
	jc->dspChannel->releaseTime = jc->releaseTime;
	DSP_PlayStart(jc->dspChannel->buffer_idx);
	DSP_FlushChannel(jc->dspChannel->buffer_idx);
	return TRUE;
}

/**
 * @TODO: Documentation
 */
BOOL ResetInitialVolume(jc_* jc)
{
	if (jc->dspChannel == NULL) {
		Console_printf("\201\364Jesus/DrvIF/Play:: \225\250\227\235DSP\203`\203\203\203\223\203l\203\213\202\252\203A\203T\203C\203\223\202\263\202\352\202\304\202\242\202\334\202\271\202\361\n");
		return FALSE;
	}

	if (CheckLogicalChannel(jc) == FALSE) {
		return FALSE;
	}

	UpdateEffecterParam(jc);
	__UpdateJcToDSPInit(jc);
	DSP_FlushChannel(jc->dspChannel->buffer_idx);
	return TRUE;
}

/**
 * @TODO: Documentation
 * BS2: 8134F8C0, size 0xb0
 */
BOOL Add_WaitDSPChannel(jc_* jc)
{
	if (cur_waits == 32) {
		// "the wait entry list is full"
		Console_printf("\203E\203G\203C\203g\202\314\203G\203\223\203g\203\212\201[\202\360\203~\203X\202\350\202\334\202\265\202\275\n");
		return FALSE;
	}

	// "force-stop: DSP channel"
	Console_printf("\213\255\220\247\222\342\216~:DSP\203`\203\203\203\223\203l\203\213\n");
	if (BreakLowerDSPchannel(jc->channelPriority) == FALSE) {
		Console_printf("\203u\203\214\201[\203N\202\311\216\270\224s\201F\203v\203\211\203C\203I\203\212\203e\203B\202\252\222\341\202\242(%d)\202\251\201A\221S\202\304\203u\203\214\201[\203N\222\206\202\335\202\275\202\242\202\305\202\267\n", jc->channelPriority);
		return FALSE;
	}

	waittime[cur_tail] = 0;
	waitp[cur_tail]    = jc;
	cur_tail++;
	cur_waits++;

	if (cur_tail == 32) {
		cur_tail = 0;
	}
	// "registered in the wait entry list"
	Console_printf("\203E\203G\203C\203g\202\314\203G\203\223\203g\203\212\201[\202\360\215s\202\242\202\334\202\265\202\275\n");
	return TRUE;
}

/**
 * @TODO: Documentation
 * BS2: 8134FCE0, size 0x64
 */
BOOL Del_WaitDSPChannel(jc_* jc)
{
	for (u32 i = 0; i < cur_waits; i++) {
		u32 a = (cur_top + i) & 0x1F;
		if (waitp[a] == jc) {
			waitp[a] = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @TODO: Documentation
 */
void __Entry_WaitChannel(u8 a)
{
	jc_* jc;
	dspch_* ch;

	while (cur_waits != 0) {
		jc = waitp[cur_top];
		if (jc) {
			if (jc->dspChannel) {
				OSReport("------------\202\267\202\305\202\311\202\261\202\242\202\302\202\315DSP\203`\203\203\203\223\203l\203\213\202\360\216\235\202\301\202\304\202\250\202\351\202\314\202\305\201A\217\234\212O\202\271\202\346\n");
			}
			ch = AllocDSPchannel(0, (u32)jc);
			if (ch == NULL) {
				Console_printf("\202\250\202\251\202\265\202\242\202\310\201A\213\363\202\242\202\304\202\310\202\242\202\274\n");
				break;
			}
			jc->dspChannel = ch;
			if (PlayLogicalChannel(jc) == FALSE) {
				OSReport("\202\240\202\350\202\246\202\310\202\242\203G\203\211\201[\201F\203E\203G\203C\203g\221\322\202\277\214\343\202\314\203m\201[\203g\203I\203\223\202\305\201A\202\310\202\272\202\251\216\270\224s\n");
			}
			if (List_CutChannel(jc) == -1) {
				Console_printf("\201\364Jesus/DrvIF:: \203E\203G\203C\203g\202\251\202\347\202\314\203G\203\223\203g\203\212\201[\202\305Cut\216\270\224s\n");
			} else {
				List_AddChannelTail(&jc->chanMgr->activeChannels, jc);
			}
			cur_top++;
			if (cur_top == 0x20) {
				cur_top = 0;
			}
			cur_waits--;
			if (a == 1) {
				break;
			}
		} else {
			cur_top++;
			if (cur_top == 0x20) {
				cur_top = 0;
			}
			cur_waits--;
		}
	}
}

/**
 * @TODO: Documentation
 */
void EntryCheck_WaitDSPChannel()
{
	__Entry_WaitChannel(0);

	for (u32 i = 0; i < cur_waits; i++) {
		jc_* jc = waitp[(cur_top + i) & 0x1f];
		if (jc) {
			waittime[(cur_top + i) & 0x1f]++;
			if (jc->noteId > 0) {
				jc->noteId--;
			}
			if (jc->noteId == 0) {
				OSReport("\221\322\202\301\202\304\202\242\202\351\212\324\202\311\217\301\202\246\202\304\202\265\202\340\202\244\202\275(%d)\n", jc->lastNotePlayed);
				jc->updateCallback(jc, JCSTAT_Unk6);
				waitp[(cur_top + i) & 0x1f] = NULL;
			}
		}
	}

	STACK_PAD_VAR(2);
}

/**
 * @TODO: Documentation
 */
static void Cancel_WaitDSPChannel(jc_* jc)
{
	STACK_PAD_VAR(2);
	for (u32 i = 0; i < cur_waits; i++) {
		if (waitp[(cur_top + i) & 0x1F] == jc) {
			// "found in the JCS release list, relwait cancelled"
			OSReport("JCS\203\212\203\212\201[\203X\202\311\224\272\202\244relwait\203L\203\203\203\223\203Z\203\213\224\255\220\266\n");
			waitp[(cur_top + i) & 0x1F] = NULL;
		}
	}
}

/**
 * @TODO: Documentation
 * BS2: 81350040, size 0x38
 */
BOOL ForceStopLogicalChannel(jc_* jc)
{
	if (jc->dspChannel == NULL) {
		return FALSE;
	}

	ForceStopDSPchannel(jc->dspChannel);
	return TRUE;
}

// IPL-only functions in this unit with no Pikmin counterpart, still assembly:
//   fn_8134E5A0 (size 0x44)
//   fn_8134E600 (size 0x98)
//   fn_8134E6E0 (size 0x8c)
//   fn_8134E780 (size 0x58)
//   fn_8134E7E0 (size 0xa8)
//   fn_8134F480 (size 0x424)
//   fn_8134F980 (size 0x50)
//   fn_8134F9E0 (size 0x154)
//   fn_8134FB40 (size 0x84)
//   fn_8134FBE0 (size 0xec)
//   fn_8134FD60 (size 0x150)
//   fn_8134FEC0 (size 0xe0)
//   fn_8134FFA0 (size 0x90)
