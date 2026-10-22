#include "jaudio/dummyrom.h"
#include "jaudio/sdk.h"
#include "jaudio/audiocommon.h"
#include "jaudio/memory.h"
#include "jaudio/dvdthread.h"

// IPL-only callees not named yet.
struct OSMesgQueue_s;
s32 Z_osSendMesg(OSMesgQueue_s* mq, void* msg, s32 flags); // NEOS os.c

ALHeap aram_hp;
u8* JAC_ARAM_DMA_BUFFER_TOP;

static u32 AUDIO_ARAM_TOP;
static u32 CARD_SECURITY_BUFFER;
static u32 SELECTED_ARAM_SIZE;

/**
 * @TODO: Documentation
 * BS2: 81338F40, size 0x8
 */
u32 GetNeosRomTop()
{
	return AUDIO_ARAM_TOP;
}

/**
 * @TODO: Documentation
 * BS2: 81338F60, size 0x28
 */
void mesg_finishcall(u32 mq)
{
	Z_osSendMesg((OSMesgQueue_s*)mq, NULL, 0);
}

/**
 * @TODO: Documentation
 * BS2: 81338FA0, size 0x5c
 */
BOOL ARAMStartDMAmesg(u32 dir, u32 dramAddr, u32 aramAddr, u32 size, s32, OSMessageQueue* mq)
{
	aramAddr += AUDIO_ARAM_TOP;
	if (dir == 1) {
		DVDT_ARAMtoDRAM((u32)mq, dramAddr, aramAddr, size, NULL, mesg_finishcall);
	} else {
		DVDT_DRAMtoARAM((u32)mq, dramAddr, aramAddr, size, NULL, mesg_finishcall);
	}
	return FALSE;
}

/**
 * @TODO: Documentation
 * BS2: 81339000, size 0x8
 */
void Jac_SetAudioARAMSize(u32 size)
{
	SELECTED_ARAM_SIZE = size;
}

/**
 * @TODO: Documentation
 * BS2: 81339020, size 0x5c
 */
void* ARAlloc(u32 size)
{
	void* alloc = Nas_HeapAlloc(&aram_hp, size);
	OSReport("ArAlloc:: %x %x %x\n", alloc, aram_hp.current, aram_hp.length);
	return alloc;
}

/**
 * @TODO: Documentation
 * BS2: 81339080, size 0x58
 */
void* ARAllocFull(u32* outSize)
{
	u32 freeSize = aram_hp.length - (aram_hp.current - aram_hp.base);

	void* alloc = Nas_HeapAlloc(&aram_hp, freeSize - 32);
	*outSize    = freeSize - 32;
	return alloc;
}

/**
 * @TODO: Documentation
 * BS2: 813390E0, size 0x124 (Pikmin 0x98)
 */
void Jac_InitARAM(u32 loadAudiorom)
{
	volatile u32 audiorom_size;
	u32 aram_size;
	STACK_PAD_VAR(2);

	if (SELECTED_ARAM_SIZE) {
		aram_size = SELECTED_ARAM_SIZE;
	} else {
		aram_size = AUDIO_ARAM_SIZE;
	}

	AUDIO_ARAM_TOP = ARGetBaseAddress();
	if (loadAudiorom) {
		audiorom_size = Jac_CheckFile("/audiorom.img");
		if (audiorom_size == 0) {
			// "audiorom.img is missing"
			OSReport("\201\364Jac/ARAM/DVD:: audiorom.img\202\252\202\240\202\350\202\334\202\271\202\361\n");
		} else {
			audiorom_size = ALIGN_NEXT(audiorom_size, 32);
			// "reserved audiorom.img (size %xh bytes)"
			OSReport("\201\364Jac/ARAM/DVD:: audiorom.img(\203T\203C\203Y %xh bytes) \202\360\201@\227\\\226\361\202\265\202\334\202\265\202\275 \n\n", audiorom_size);
		}
	} else {
		audiorom_size = 0;
	}

	CARD_SECURITY_BUFFER = 0x40;
	audiorom_size += AUDIO_ARAM_TOP;
	JAC_ARAM_DMA_BUFFER_TOP = (u8*)audiorom_size;
	audiorom_size += AUDIO_ARAM_HEAP_SIZE;
	// "ARAM DMA buffer is %xh to %xh bytes"
	OSReport("\n\201\364Jac/ARAM/Init:: ARAMDMA\203o\203b\203t\203@\202\315 %xh \202\251\202\347 %xh\203o\203C\203g\202\305\202\267\n", JAC_ARAM_DMA_BUFFER_TOP, AUDIO_ARAM_HEAP_SIZE);
	Nas_HeapInit(&aram_hp, (u8*)audiorom_size, aram_size - audiorom_size);
	// "reserving ARAM %xh to %xh for sound data"
	OSReport("\n\201\364Jac/ARAM/Init:: ARAM\202\314 %xh \202\251\202\347 %xh \203T\203E\203\223\203h\203f\201[\203^\227\314\210\346\202\360\212m\225\333\202\265\202\334\202\267\n", audiorom_size, aram_size - audiorom_size);
}
