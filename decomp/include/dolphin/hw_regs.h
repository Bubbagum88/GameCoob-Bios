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

#endif
