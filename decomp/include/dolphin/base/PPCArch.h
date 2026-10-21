#ifndef _DOLPHIN_PPCARCH_H_
#define _DOLPHIN_PPCARCH_H_

#include <dolphin/types.h>

u32 PPCMfmsr(void);
void PPCMtmsr(u32 newMSR);
u32 PPCMfhid0(void);
u32 PPCMfl2cr(void);
void PPCMtl2cr(u32 newL2cr);
void PPCMtdec(u32 newDec);
void PPCSync(void);
void PPCHalt(void);
u32 PPCMfhid2(void);
void PPCMthid2(u32 newhid2);
void PPCMtwpar(u32 newwpar);

#endif
