#include "jaudio/aictrl.h"
#include "jaudio/sdk.h"
#include "jaudio/audiocommon.h"
#include "jaudio/audiostruct.h"
#include "jaudio/driverinterface.h"
#include "jaudio/dspbuf.h"
#include "jaudio/dspproc.h"
#include "jaudio/dummyprobe.h"
#include "jaudio/memory.h"
#include "jaudio/rate.h"
#include "jaudio/sample.h"
#include "jaudio/streamctrl.h"
#include <stddef.h>

u32 UNIVERSAL_DACCOUNTER = 0;

static s16* dac[3];
static ALHeap audio_hp;

static BOOL audio_hp_exist           = FALSE;
static s16* last_rsp_madep           = NULL;
static s16* use_rsp_madep            = NULL;
static BOOL vframe_work_running      = FALSE;
static DACCallback DAC_CALLBACK_FUNC = NULL;
u32 JAC_VFRAME_COUNTER               = 0;
static MixCallback ext_mixcallback   = NULL;
static u8 ext_mixmode                = MixMode_Mono;

/**
 * @TODO: Documentation
 * BS2: 81338400, size 0xa8 (Pikmin 0x4c)
 */
void Jac_HeapSetup(void* heap, s32 size)
{
	if (heap) {
		u8* end = (u8*)heap + size;
		// "memory is assigned from the main side, initialising the audio heap"
		OSReport("\n\201\364Jac/AI/Heap:: \203\201\203C\203\223\202\251\202\347\203\201\203\202\203\212\202\360\216w\222\350\202\263\202\352\202\334\202\265\202\275\202\314\202\305\n");
		OSReport("                  \203I\201[\203f\203B\203I\203q\201[\203v\202\360\217\211\212\372\211\273\202\265\202\334\202\267\n");
		Nas_HeapInit(&audio_hp, (u8*)heap, size);
		// "addresses %x to %x are usable"
		OSReport("\201\364Jac/AI/Heap:: \203A\203h\203\214\203X\201@%x \202\251\202\347 %x \202\334\202\305\216g\227p\211\302\224\\\202\305\202\267\n", heap, end - 1);
		audio_hp_exist = TRUE;
	} else {
		// "no memory assigned from the main side, using OSAlloc without an audio heap"
		OSReport("\n\201\364Jac/AI/Heap:: \203\201\203C\203\223\202\251\202\347\202\314\203\201\203\202\203\212\216w\222\350\202\252\202\240\202\350\202\334\202\271\202\361\202\314\202\305\n");
		OSReport("                  \203I\201[\203f\203B\203I\203q\201[\203v\202\360\212m\225\333\202\271\202\270\202\311OSAlloc\202\360\216g\227p\202\265\202\334\202\267\n");
		audio_hp_exist = FALSE;
	}
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000034
 */
void GetAudioHeapRemain(void)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * BS2: 813384C0, size 0xb0 (Pikmin 0x80)
 */
void* OSAlloc2(u32 size)
{
	void* alloc;
	BOOL enabled;

	Console_printf("\201\364Jac/Alloc2:: Request SIZE %d\n", size);
	if (audio_hp_exist == FALSE) {
		// "the audio heap has not been set up"
		OSReport("\201\364Jac/Alloc2:: \203I\201[\203f\203B\203I\203q\201[\203v\202\252\212m\225\333\202\263\202\352\202\304\202\242\202\334\202\271\202\361\n");
	}
	enabled = OSDisableInterrupts();
	switch (audio_hp_exist) {
	case FALSE:
	{
		alloc = OSAllocFromHeap(__OSCurrHeap, size);
		break;
	}
	case TRUE:
	{
		alloc = Nas_HeapAlloc(&audio_hp, size);
		break;
	}
	}

	OSRestoreInterrupts(enabled);
	return alloc;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000054
 */
void OSFree2(void)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * BS2: 81338580, size 0xcc (Pikmin 0xac)
 */
void Jac_Init()
{
	s16** thisDac;
	int i;
	STACK_PAD_VAR(2);
	// "initialising the Jac system"
	OSReport("\n\201\364Jac/AI/Init:: Jac\203V\203X\203e\203\200\202\360\217\211\212\372\211\273\202\265\202\334\202\267\n");
	for (i = 0; i < 3; i++) {
		void* alloc = OSAlloc2(DAC_SIZE * 2);
		thisDac     = &dac[i];
		*thisDac    = (s16*)alloc;
		// "internal DAC buffer %d= %x"
		OSReport("\201\364Jac/AI/Init:: \223\340\225\224DAC\203o\203b\203t\203@%d= %x\n", i, *thisDac);
		Jac_bzero(*thisDac, DAC_SIZE * 2);
		DCStoreRange(*thisDac, DAC_SIZE * 2);
	}

	AIInit(NULL);
	AIInitDMA((u32)dac[2], DAC_SIZE * 2);
	// "JAC system initialisation finished"
	OSReport("\201\364Jac/AI/Init:: JAC\203V\203X\203e\203\200\202\314\217\211\212\372\211\273\202\252\217I\227\271\202\265\202\334\202\265\202\275\n\n");
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000008
 */
u32 Jac_GetCurrentVCounter(void)
{
	return JAC_VFRAME_COUNTER;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00000C
 */
void HaltDSPSignal(void)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000010
 */
void HaltDSP(void)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000010
 */
void RunDSP(void)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000008
 */
void CheckHaltDSP(void)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * BS2: 81338660, size 0xdc
 */
static void MixMonoTrack(s16* track, s32 nSamples, MixCallback callback)
{
	STACK_PAD_VAR(2);
	Probe_Start(5, "MONO-MIX");

	s16* monoTrack = (*callback)(nSamples);
	int mix;

	if (monoTrack != NULL) {
		Probe_Finish(5);

		s16* dst_p = track;
		s16* src_p = monoTrack;

		for (s32 i = nSamples; i > 0; i--) {
			mix = *dst_p + *src_p;
			if (mix < S16_MIN) {
				mix = S16_MIN + 1;
			}

			if (mix > S16_MAX) {
				mix = S16_MAX;
			}

			*dst_p++ = (s16)mix;

			mix = *dst_p + *src_p;
			if (mix < S16_MIN) {
				mix = S16_MIN + 1;
			}

			if (mix > S16_MAX) {
				mix = S16_MAX;
			}

			*dst_p++ = (s16)mix;

			src_p++;
		}
	}
}

/**
 * @TODO: Documentation
 * BS2: 81338740, size 0xdc
 */
static void MixMonoTrackWide(s16* track, s32 nSamples, MixCallback callback)
{
	STACK_PAD_VAR(2);
	Probe_Start(5, "MONO(W)-MIX");

	s16* monoTrack = (*callback)(nSamples);
	int mix;

	if (monoTrack != NULL) {
		Probe_Finish(5);

		s16* dst_p = track;
		s16* src_p = monoTrack;

		for (s32 i = nSamples; i > 0; i--) {
			mix = *dst_p + *src_p;
			if (mix < S16_MIN) {
				mix = S16_MIN + 1;
			}

			if (mix > S16_MAX) {
				mix = S16_MAX;
			}

			*dst_p++ = (s16)mix;

			mix = *dst_p - *src_p;
			if (mix < S16_MIN) {
				mix = S16_MIN + 1;
			}

			if (mix > S16_MAX) {
				mix = S16_MAX;
			}

			*dst_p++ = (s16)mix;
			src_p++;
		}
	}
}

/**
 * @TODO: Documentation
 * BS2: 81338820, size 0xf8
 */
static void MixExtraTrack(s16* track, s32 nSamples, MixCallback callback)
{
	STACK_PAD_VAR(2);
	Probe_Start(5, "DSPMIX");

	s16* extraTrack = (*callback)(nSamples);
	int mix;

	if (extraTrack != NULL) {
		Probe_Finish(5);
		Probe_Start(6, "MIXING");

		s16* dst_p  = track;
		s16* src0_p = extraTrack + JAC_FRAMESAMPLES;
		s16* src1_p = extraTrack;

		for (s32 i = nSamples; i > 0; i--) {
			mix = *dst_p + src0_p[0];
			if (mix < S16_MIN) {
				mix = S16_MIN + 1;
			}

			if (mix > S16_MAX) {
				mix = S16_MAX;
			}

			*dst_p++ = (s16)mix;

			mix = *dst_p + src1_p[0];
			if (mix < S16_MIN) {
				mix = S16_MIN + 1;
			}

			if (mix > S16_MAX) {
				mix = S16_MAX;
			}

			*dst_p++ = (s16)mix;
			src0_p++;
			src1_p++;
		}

		Probe_Finish(6);
	}
}

/**
 * @TODO: Documentation
 * BS2: 81338920, size 0x8c
 */
static void MixInterleaveTrack(s16* track, s32 nSamples, MixCallback callback)
{
	STACK_PAD_VAR(2);
	s16* interleaveTrack = (*callback)(nSamples);
	int mix;

	if (interleaveTrack != NULL) {
		s16* track_p = track;
		s32 max      = nSamples * 2;

		for (s32 i = 0; i < max; i++) {
			mix = *track_p + *interleaveTrack;
			if (mix < S16_MIN) {
				mix = S16_MIN + 1;
			}

			if (mix > S16_MAX) {
				mix = S16_MAX;
			}

			*track_p++ = (s16)mix;
			interleaveTrack++;
		}
	}
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000010
 */
MixCallback Jac_GetMixcallback(u8* mixmode)
{
	*mixmode = ext_mixmode;
	return ext_mixcallback;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00000C
 */
void Jac_RegisterMixcallback(MixCallback callback, u8 mixmode)
{
	ext_mixcallback = callback;
	ext_mixmode     = mixmode;
}

/**
 * @TODO: Documentation
 * BS2: 813389E0, size 0x188
 */
void Jac_VframeWork()
{
	static u32 dacp = 0;

	JAC_VFRAME_COUNTER++;

	s16* mixedTrack = MixDsp(DAC_SIZE / 2);
	s16* x          = &mixedTrack[JAC_FRAMESAMPLES];
	Jac_imixcopy(x, &mixedTrack[0], dac[dacp], DAC_SIZE / 2);

	if (ext_mixcallback != NULL) {
		switch (ext_mixmode) {
		case MixMode_Mono:
		{
			MixMonoTrack(dac[dacp], DAC_SIZE / 2, ext_mixcallback);
			break;
		}
		case MixMode_MonoWide:
		{
			MixMonoTrackWide(dac[dacp], DAC_SIZE / 2, ext_mixcallback);
			break;
		}
		case MixMode_Extra:
		{
			MixExtraTrack(dac[dacp], DAC_SIZE / 2, ext_mixcallback);
			break;
		}
		case MixMode_Interleave:
		{
			MixInterleaveTrack(dac[dacp], DAC_SIZE / 2, ext_mixcallback);
			break;
		}
		}
	}

	BOOL enable = OSDisableInterrupts();
	DCStoreRange(dac[dacp], DAC_SIZE * 2);
	OSRestoreInterrupts(enable);

	last_rsp_madep = dac[dacp];
	dacp++;
	if (dacp == 3) {
		dacp = 0;
	}
	vframe_work_running = FALSE;
}

/**
 * @TODO: Documentation
 * BS2: 81338B80, size 0xa8
 */
void Jac_UpdateDAC()
{
	if (use_rsp_madep == NULL) {
		use_rsp_madep  = last_rsp_madep;
		last_rsp_madep = NULL;
	}

	if (use_rsp_madep != NULL) {
		AIInitDMA((u32)use_rsp_madep, DAC_SIZE * 2);
		use_rsp_madep = NULL;
	} else {
		UNIVERSAL_DACCOUNTER++;
	}

	if (last_rsp_madep == NULL && vframe_work_running == FALSE) {
		Jac_VframeWork();
	}

	StreamMain();

	if (DAC_CALLBACK_FUNC != NULL) {
		(*DAC_CALLBACK_FUNC)(last_rsp_madep, DAC_SIZE / 2);
	}
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000008
 */
void Jac_RegisterDacCallback(DACCallback callback)
{
	DAC_CALLBACK_FUNC = callback;
}

/**
 * @TODO: Documentation
 * BS2: 81338C40, size 0x8
 */
void Jac_SetOutputMode(int mode)
{
	JAC_SYSTEM_OUTPUT_MODE = mode;
}

/**
 * @TODO: Documentation
 */
int Jac_GetOutputMode()
{
	return JAC_SYSTEM_OUTPUT_MODE;
}

/**
 * @TODO: Documentation
 * BS2: 81338C60, size 0x34
 */
void Jac_SetMixerLevel(f32 channelLevel, f32 dspLevel)
{
	Channel_SetMixerLevel(channelLevel);
	DsetMixerLevel(dspLevel);
}

// IPL-only functions in this unit with no Pikmin counterpart, still assembly:
//   fn_813389C0 (size 0xc)
