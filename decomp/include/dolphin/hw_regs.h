#ifndef _DOLPHIN_HW_REGS_H_
#define _DOLPHIN_HW_REGS_H_

#include <dolphin/types.h>

extern volatile u16 __VIRegs[] : 0xCC002000;
extern volatile u32 __PIRegs[] : 0xCC003000;
extern volatile u16 __MEMRegs[] : 0xCC004000;
extern volatile u16 __DSPRegs[] : 0xCC005000;
extern volatile u32 __DIRegs[] : 0xCC006000;
extern volatile u32 __SIRegs[] : 0xCC006400;
extern volatile u32 __EXIRegs[] : 0xCC006800;
extern volatile u32 __AIRegs[] : 0xCC006C00;

// Register indices used by dvd, ai, ar and dsp
#define DI_STATUS 0
#define DI_COVER_STATUS 1
#define DI_CMD_BUF_0 2
#define DI_CMD_BUF_1 3
#define DI_CMD_BUF_2 4
#define DI_DMA_MEM_ADDR 5
#define DI_DMA_LENGTH 6
#define DI_CONTROL 7
#define DI_MM_BUF 8

#define DSP_MAILBOX_IN_HI 0
#define DSP_MAILBOX_IN_LO 1
#define DSP_MAILBOX_OUT_HI 2
#define DSP_MAILBOX_OUT_LO 3
#define DSP_CONTROL_STATUS 5
#define DSP_ARAM_SIZE 9
#define DSP_ARAM_MODE 11
#define DSP_ARAM_REFRESH 13
#define DSP_ARAM_DMA_MM_HI 16
#define DSP_ARAM_DMA_MM_LO 17
#define DSP_ARAM_DMA_ARAM_HI 18
#define DSP_ARAM_DMA_ARAM_LO 19
#define DSP_ARAM_DMA_SIZE_HI 20
#define DSP_ARAM_DMA_SIZE_LO 21
#define DSP_DMA_START_HI 24
#define DSP_DMA_START_LO 25
#define DSP_DMA_CONTROL_LEN 27
#define DSP_DMA_START_FLAG 0x8000

#define AI_CONTROL 0
#define AI_VOLUME 1
#define AI_SAMPLE_COUNTER 2
#define AI_INTRPT_TIMING 3
#define AI_CONTROL_PLAY_STATE 0x1
#define AI_CONTROL_STREAM_SAMPLE_RATE 0x2
#define AI_CONTROL_STREAM_SAMPLE_COUNT 0x20
#define AI_CONTROL_DSP_SAMPLE_RATE 0x40

#define PI_RESETCODE 9

#define VI_DTV_STAT 55

#endif
