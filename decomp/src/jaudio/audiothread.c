#include "jaudio/audiothread.h"
#include "jaudio/sdk.h"
#include "jaudio/aictrl.h"
#include "jaudio/audiocommon.h"
#include "jaudio/cpubuf.h"
#include "jaudio/dspboot.h"
#include "jaudio/dspbuf.h"
#include "jaudio/dspinterface.h"
#include "jaudio/dspproc.h"
#include "jaudio/dsptask.h"
#include "jaudio/dummyprobe.h"
#include "jaudio/dummyrom.h"
#include "jaudio/dvdthread.h"
#include "jaudio/neosthread.h"
#include "jaudio/ja_calc.h"
#include "jaudio/playercall.h"
#include "jaudio/rate.h"
#include "jaudio/stackchecker.h"
#include <stddef.h>

// this is purely for comm bullshit to match. pretty sure it's not the comm bug, just wacky.
typedef struct Jac_AudioThread {
	OSThread thread; // _00
	u8 pad[0x10];    // _310
} Jac_AudioThread;

Jac_AudioThread jac_audioThread ATTRIBUTE_ALIGN(32);
OSThread jac_neosThread;
OSThread jac_dvdThread;

static u8 jac_audioStack[AUDIO_STACK_SIZE];
static u8 jac_neosStack[AUDIO_STACK_SIZE];
static u8 jac_dvdStack[AUDIO_STACK_SIZE];

static OSMessageQueue audioproc_mq;
static OSMessage msgbuf[AUDIOPROC_MQ_BUF_COUNT];

static u32 audioproc_mq_init;
static volatile int intcount;

/**
 * @TODO: Documentation
 * BS2: 81339220, size 0x8
 */
void DspSyncCountClear(int count)
{
	intcount = count;
}

/**
 * @TODO: Documentation
 * BS2: 81339240, size 0x8
 */
int DspSyncCountCheck()
{
	return intcount;
}

/**
 * @TODO: Documentation
 * BS2: 81339260, size 0x5c (Pikmin 0x44)
 */
static void DspSync()
{
	if (audioproc_mq_init) {
		if (OSSendMessage(&audioproc_mq, AUDIOPROC_MESSAGE_DSP_SYNC, OS_MESSAGE_NOBLOCK) == FALSE) {
			OSReport("DSPSync:: SendMiss\n");
		}
	} else {
		DSPReleaseHalt();
	}
}

#if defined(VERSION_GPIP01)
/**
 * @TODO: Documentation
 */
static void DspSync2(void*)
{
	u32 check;
	u32 mesg;

	u32* REF_check = &check;
	STACK_PAD_VAR(1);

	do {
		check = DSPCheckMailFromDSP();
	} while (check == 0);

	mesg = DSPReadMailFromDSP();
	if (mesg >> 0x10 == 0xf355) {
		switch (mesg & 0xff00) {
		case 0xff00:
			DspSync();
			break;
		default:
			DspFinishWork(mesg);
			break;
		}
	}
}
#endif

/**
 * @TODO: Documentation
 * BS2: 813392C0, size 0x50
 */
void StopAudioThread()
{
	if (audioproc_mq_init) {
		if (OSSendMessage(&audioproc_mq, AUDIOPROC_MESSAGE_3, OS_MESSAGE_NOBLOCK) == FALSE) {
			OSCancelThread(&jac_audioThread.thread);
		}
	}
}

/**
 * @TODO: Documentation
 * BS2: 81339320, size 0x98 (Pikmin 0x80)
 */
static void AudioSync()
{
	static BOOL first = TRUE;

	if (first == FALSE) {
		Probe_Finish(4);
	}

	first = FALSE;
	Probe_Start(4, "UPDATE-DAC");
	if (audioproc_mq_init) {
		if (OSSendMessage(&audioproc_mq, AUDIOPROC_MESSAGE_UPDATE_DAC, OS_MESSAGE_NOBLOCK) == FALSE) {
			OSReport("Miss Send AudioSync\n");
		}
	}
}

/**
 * @TODO: Documentation
 * BS2: 813393C0, size 0x3c
 */
void NeosSync()
{
	if (audioproc_mq_init) {
		OSSendMessage(&audioproc_mq, AUDIOPROC_MESSAGE_NEOS_SYNC, OS_MESSAGE_BLOCK);
	}
}

/**
 * @TODO: Documentation
 * BS2: 81339400, size 0x68
 */
static void __DspSync(__OSInterrupt interrupt, OSContext* context)
{
	u16 reg                       = __DSPRegs[DSP_CONTROL_STATUS];
	reg                           = (1 << 7) | reg & -0x29; /* clear AR + AI interrupt, set DSP interrupt */
	__DSPRegs[DSP_CONTROL_STATUS] = reg;

	OSContext tmp_context;
	OSClearContext(&tmp_context);
	OSSetCurrentContext(&tmp_context);
	DspSync();
	OSClearContext(&tmp_context);
	OSSetCurrentContext(context);
}

/**
 * @TODO: Documentation
 * BS2: 81339480, size 0x44
 */
static void __DspReg()
{
	BOOL enable = OSDisableInterrupts();
	__OSSetInterruptHandler(__OS_INTERRUPT_DSP_DSP, &__DspSync);
	OSRestoreInterrupts(enable);
}

/**
 * @TODO: Documentation
 * BS2: 813394E0, size 0x184 (Pikmin 0x130)
 */
static void* audioproc(void*)
{
	// "the audio thread has started"
	OSReport("\n\201\364Jac/Thread/Proc:: \203I\201[\203f\203B\203I\203X\203\214\203b\203h\202\252\212J\216n\202\265\202\334\202\265\202\275\n");
	OSInitFastCast();
	OSInitMessageQueue(&audioproc_mq, msgbuf, AUDIOPROC_MQ_BUF_COUNT);
	audioproc_mq_init = TRUE;
	Jac_Init();
	Jac_InitSinTable();
	ResetPlayerCallback();
	DspbufProcess(DSPBUF_EVENT_INIT);
	CpubufProcess(DSPBUF_EVENT_INIT);
	DspBoot();
	DSP_InitBuffer();
	__DspReg();
	AISetDSPSampleRate(JAC_AI_SETTING);
	AIRegisterDMACallback(&AudioSync);
	AIStartDMA();
	// "audio system initialisation finished"
	OSReport("\201\364Jac/Thread/Proc::\203I\201[\203f\203B\203I\203V\203X\203e\203\200\202\314\203C\203j\203V\203\203\203\213\202\252\217I\227\271\202\265\202\334\202\265\202\275\n");
	// "from here on it loops"
	OSReport("\t\t\t\t\t \202\261\202\261\202\251\202\347\202\315\203\213\201[\203v\202\311\202\310\202\350\202\334\202\267\n\n");

	while (TRUE) {
		OSMessage msg;

		OSReceiveMessage(&audioproc_mq, &msg, OS_MESSAGE_BLOCK);
		switch ((int)msg) {
		case (int)AUDIOPROC_MESSAGE_UPDATE_DAC:
		{
			Jac_UpdateDAC();
			break;
		}
		case (int)AUDIOPROC_MESSAGE_DSP_SYNC:
		{
			if (intcount == 0) {
				// "spurious interrupt occurred..."
				OSReport("\201\364Jac/thread/DSPSync:: \227]\225\252\202\310\212\204\202\350\215\236\202\335\202\252\224\255\220\266...\n");
				return;
			}

			intcount--;
			if (intcount == 0) {
				Probe_Finish(7);
				DspFrameEnd();
			} else {
				Probe_Start(2, "SFR_DSP");
				UpdateDSP();
				Probe_Finish(2);
			}

			break;
		}
		case (int)AUDIOPROC_MESSAGE_NEOS_SYNC:
		{
			CpuFrameEnd();
			break;
		}
		case (int)AUDIOPROC_MESSAGE_3:
		{
			// "ending the audio thread"
			OSReport("\203I\201[\203f\203B\203I\203X\203\214\203b\203h\202\360\217I\227\271\202\265\202\334\202\267\n");
			OSExitThread(NULL);
			// "it's strange if this is displayed"
			OSReport("\202\261\202\352\202\315\225\\\216\246\202\263\202\352\202\275\202\347\202\250\202\251\202\265\202\242\202\305\202\267\n");
			break;
		}
		}
	}

	STACK_PAD_VAR(3);
}

static BOOL priority_set        = FALSE;
static volatile OSPriority pri  = 0;
static volatile OSPriority pri2 = 0;
static volatile OSPriority pri3 = 0;

/**
 * @TODO: Documentation
 * BS2: 813396C0, size 0x1b8 (Pikmin 0x124)
 */
void StartAudioThread(void* heap, s32 heapSize, u32 aramSize, u32 flags)
{
	if (priority_set == FALSE) {
		OSPriority base_prio = OSGetThreadPriority(OSGetCurrentThread()) - 3;

		pri = base_prio;

		OSPriority p = pri;
		pri3         = p + 1;
		pri2         = p + 2;
		// "using the default thread priority"
		OSReport("\201\364Jac/Thread/Init:: \203f\203t\203H\203\213\203g\202\314\203X\203\214\203b\203h\203v\203\211\203C\203I\203\212\203e\203B\222l\202\360\216g\227p\202\265\202\334\202\267\n");
	} else {
		// "using the reserved thread priority"
		OSReport("\201\364Jac/Thread/Init:: \227\\\226\361\215\317\202\335\202\314\203X\203\214\203b\203h\203v\203\211\203C\203I\203\212\203e\203B\222l\202\360\216g\227p\202\265\202\334\202\267\n");
	}

	u32 neos_flag;

	// "starting the audio thread"
	OSReport("\n\201\364Jac/Thread/Init:: \203I\201[\203f\203B\203I\203X\203\214\203b\203h\202\360\213N\223\256\202\265\202\334\202\267\n");
	Jac_HeapSetup(heap, heapSize);
	Jac_SetAudioARAMSize(aramSize);

	neos_flag = flags & AUDIO_THREAD_FLAG_NEOS;
	Jac_InitARAM(neos_flag);
	// "thread priority is %d"
	OSReport("\201\364Jac/Thread/Init:: \201@\203X\203\214\203b\203h\202\314\227D\220\346\217\207\210\312\202\315 %d  \202\305\202\267\n", pri);

	if ((flags & AUDIO_THREAD_FLAG_AUDIO)) {
		OSCreateThread(&jac_audioThread.thread, &audioproc, NULL, jac_audioStack + AUDIO_STACK_SIZE, AUDIO_STACK_SIZE, pri, 0);
		OSResumeThread(&jac_audioThread.thread);
		// "audio thread started"
		OSReport("\201\364Jac/Thread/Init:: \201@\203I\201[\203f\203B\203I\203X\203\214\203b\203h\202\360\213N\223\256\217I\227\271\202\265\202\334\202\265\202\275\n\n");
	}

	if ((flags & AUDIO_THREAD_FLAG_DVD)) {
		jac_dvdproc_init();
		OSCreateThread(&jac_dvdThread, &jac_dvdproc, NULL, jac_dvdStack + AUDIO_STACK_SIZE, AUDIO_STACK_SIZE, pri3, 0);
		OSResumeThread(&jac_dvdThread);
		// "DVD thread started"
		OSReport("\201\364Jac/Thread/Init:: \201@DVD\203X\203\214\203b\203h\202\360\213N\223\256\217I\227\271\202\265\202\334\202\265\202\275\n\n");
	} else {
		// "DVD thread not started"
		OSReport("\201\364Jac/Thread/Init:: \201@DVD\203X\203\214\203b\203h\202\360\213N\223\256\202\265\202\334\202\271\202\361\202\305\202\265\202\275\n\n");
	}

	if (neos_flag) {
		OSCreateThread(&jac_neosThread, &neosproc, NULL, jac_neosStack + AUDIO_STACK_SIZE, AUDIO_STACK_SIZE, pri2, 0);
		OSResumeThread(&jac_neosThread);
		// "NEOS thread started"
		OSReport("\201\364Jac/Thread/Init:: \201@NEOS\203X\203\214\203b\203h\202\360\213N\223\256\217I\227\271\202\265\202\334\202\265\202\275\n\n");
	}
}
