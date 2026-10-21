#include "jaudio/cpubuf.h"
#include "jaudio/sdk.h"
#include "jaudio/aictrl.h"
#include "jaudio/rate.h"
#include <stddef.h>

static u8 write_buffer = 0;
static u8 read_buffer  = 0;
static u8 dspstatus    = 0;

static s16* dsp_buf[DSPBUF_NUM];

// IPL-only, in the unsplit audiothread code at 813396C0: posts a mixed buffer to the audio thread
extern "C" u32 fn_81339880(s16* buffer);

/**
 * @TODO: Documentation
 * BS2: 8133A260, size 0x210 (Pikmin 0x1f8)
 */
s16* CpubufProcess(DSPBUF_EVENTS event)
{
	u32 i, j;

	switch (event) {
	case DSPBUF_EVENT_INIT:
	{
		write_buffer = 2;
		read_buffer  = 0;
		for (i = 0; i < DSPBUF_NUM; i++) {
			dsp_buf[i] = (s16*)OSAlloc2(JAC_FRAMESAMPLES << 2);
			for (j = 0; j < (JAC_FRAMESAMPLES << 1); j++) {
				dsp_buf[i][j] = 0;
			}

			DCStoreRange(dsp_buf[i], JAC_FRAMESAMPLES << 2);
		}

		dspstatus = 0;
		break;
	}
	case DSPBUF_EVENT_FRAME_END:
	{
		u8 write = write_buffer + 1;

		if (write == DSPBUF_NUM) {
			write = 0;
		}

		if (write == read_buffer) {
			dspstatus = 0;
		} else {
			write_buffer = write;
			if (fn_81339880(dsp_buf[write_buffer]) == 1) {
				dspstatus = 1;
			}
		}
		break;
	}
	case DSPBUF_EVENT_MIX:
	{
		u8 read = read_buffer + 1;
		if (read == DSPBUF_NUM) {
			read = 0;
		}

		if (read == write_buffer) {
			s16 left  = dsp_buf[read_buffer][(JAC_FRAMESAMPLES / 2) - 1];
			s16 right = dsp_buf[read_buffer][JAC_FRAMESAMPLES - 1];

			for (i = 0; i < JAC_FRAMESAMPLES; i++) {
				dsp_buf[read_buffer][i] = right;
			}

			for (i = JAC_FRAMESAMPLES; i < (JAC_FRAMESAMPLES << 1); i++) {
				dsp_buf[read_buffer][i] = left;
			}

			if (dspstatus == 0) {
				CpubufProcess(DSPBUF_EVENT_FRAME_END);
			}
		} else {
			read_buffer = read;
			if (dspstatus == 0) {
				CpubufProcess(DSPBUF_EVENT_FRAME_END);
			}
		}

		return dsp_buf[read_buffer];
	}
	}

	return NULL;
}

/**
 * @TODO: Documentation
 * BS2: 8133A480, size 0x40 (Pikmin 0x40, unused there)
 */
s16* MixCpu(s32)
{
	static u32 mixcpu_unk;
	static s8 mixcpu_init;

	if (!mixcpu_init) {
		mixcpu_unk  = 0;
		mixcpu_init = 1;
	}
	return CpubufProcess(DSPBUF_EVENT_MIX);
}

/**
 * @TODO: Documentation
 * BS2: 8133A4C0, size 0x24
 */
void CpuFrameEnd()
{
	CpubufProcess(DSPBUF_EVENT_FRAME_END);
}
