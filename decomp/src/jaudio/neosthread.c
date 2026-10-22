// Based on ac-decomp's jaudio_NES/internal/neosthread.c (CC0), adapted to the BS2 build.
#include "jaudio/neosthread.h"
#include "jaudio/sdk.h"
#include "jaudio/aictrl.h"
#include "jaudio/audiothread.h"
#include "jaudio/cpubuf.h"
#include "jaudio/dummyprobe.h"
#include "jaudio/dummyrom.h"
#include "jaudio/dvdthread.h"
#include "jaudio/rate.h"
#include "jaudio/sample.h"

#define NEOSTHREAD_IMAGE_LOADED_MSG (0x12345678)
#define NEOSTHREAD_ACMD_BUF_NUM     1600

// NEOS internals, IPL-only and not named yet.
typedef struct NeosGlobalConfig {
	u32 _00;
	s32 acmdBufSize; // _04
	u32 _08;
	u32 _0C;
} NeosGlobalConfig;
extern "C" NeosGlobalConfig lbl_813F4750;                                    // AGC
union Acmd;
void Nas_InitAudio(u64* acmdBuf, s32 size);
u32 CreateAudioTask(Acmd* cmds, s16* dst, u32 samples, s32 flags);
extern "C" void fn_81337740(u32* taskBuf, u32 task, s32 flags);             // RspStart2

void Jac_RegisterMixcallback(MixCallback callback, u8 mixmode);

static OSMessageQueue neosproc_mq;
static u32 neosproc_mq_init;
static s16* tmp_buf;
static BOOL neos_ready;

/**
 * @TODO: Documentation
 * BS2: 81339880, size 0x68
 */
u32 Neos_Update(s16* dst)
{
	if (neosproc_mq_init) {
		if (OSSendMessage(&neosproc_mq, (OSMessage)dst, OS_MESSAGE_NOBLOCK) == TRUE) {
			return TRUE;
		} else {
			OSReport("Message Full\n\n");
			return FALSE;
		}
	}

	return FALSE;
}

/**
 * @TODO: Documentation
 * BS2: 81339900, size 0x34
 */
void ImageLoaded(u32 param)
{
	OSMessage msg = (OSMessage)NEOSTHREAD_IMAGE_LOADED_MSG;
	OSSendMessage(&neosproc_mq, msg, OS_MESSAGE_BLOCK);
}

/**
 * @TODO: Documentation
 * BS2: 81339940, size 0x210
 */
void* neosproc(void* param)
{
	static OSMessage msgbuf[1];
	static u32 cur = 0;

	neos_ready = FALSE;
	OSInitMessageQueue(&neosproc_mq, msgbuf, 1);
	neosproc_mq_init = TRUE;
	// "the NEOS thread has started"
	OSReport("\n\201\364Jac/NEOS:: NEOS\203X\203\214\203b\203h\202\252\213N\223\256\202\265\202\334\202\265\202\275\n");
	u32 addr = GetNeosRomTop(); // reused for the acmd buffer and the DAC target
	// "audio ROM image start address %x"
	OSReport("\201\364Jac/NEOS:: \203I\201[\203f\203B\203I\227pROM\203C\203\201\201[\203W\220\346\223\252\203A\203h\203\214\203X %x\n", addr);

	if (DVDT_LoadtoARAM(0, "/audiorom.img", addr, 0, 0, NULL, ImageLoaded) == -1) {
		// "failed to load the CPU audio ROM image"
		OSReport("\201\364Jac/NEOS:: CPU\203I\201[\203f\203B\203I\227pROM\203C\203\201\201[\203W\202\314\223\307\202\335\202\261\202\335\202\311\216\270\224s\202\265\202\334\202\265\202\275\n");
	}

	OSMessage msg;
	do {
		OSReceiveMessage(&neosproc_mq, &msg, OS_MESSAGE_BLOCK);
	} while (msg != (OSMessage)NEOSTHREAD_IMAGE_LOADED_MSG);
	// "loaded audiorom.img"
	OSReport("\201\364Jac/NEOS:: audiorom.img \202\360\201@\223\307\202\335\202\261\202\335\202\334\202\265\202\275 \n\n");

	tmp_buf = (s16*)OSAlloc2(DAC_SIZE * 2);

	s32 tmp      = lbl_813F4750.acmdBufSize;
	addr = (u32)OSAlloc2(tmp);
	if (addr == NULL) {
		// "could not reserve memory for CPU audio"
		OSReport("\201\364Jac/NEOS::CPU\203I\201[\203f\203B\203I\227p\202\314\203\201\203\202\203\212\202\252\212m\225\333\202\305\202\253\202\334\202\271\202\361\n");
	}
	Nas_InitAudio((u64*)addr, tmp);
	NeosSync();
	neos_ready = TRUE;

	Jac_RegisterMixcallback(MixCpu, 3);

	do {
		static u64 task_buf[2][NEOSTHREAD_ACMD_BUF_NUM];
		static u32 tasks[2] = { 0, 0 };

		OSReceiveMessage(&neosproc_mq, &msg, OS_MESSAGE_BLOCK);
		Probe_Start(8, "NEOS THREAD");
		addr = (u32)msg;
		tasks[cur]       = CreateAudioTask((Acmd*)task_buf[cur], tmp_buf, JAC_FRAMESAMPLES, 0);

		tmp = (cur + 1) & 1;
		if (tasks[tmp]) {
			fn_81337740((u32*)task_buf[tmp], tasks[tmp], 0);
			tasks[tmp] = 0;
			Jac_bcopy(tmp_buf, (s16*)addr, DAC_SIZE * 2);
		} else {
			Jac_bzero((s16*)addr, DAC_SIZE * 2);
		}

		Probe_Finish(8);
		NeosSync();
		cur = tmp;
	} while (TRUE);
	STACK_PAD_VAR(2);
}
