// Boot entry (PAL 1.0 BS2, 0x813003A0..0x81300598): BS2Init clears the low
// globals and BATs, main brings up the SDK and enters BS2Main.

#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/dvd.h>
#include <dolphin/card.h>
#include <dolphin/vi.h>
#include <dolphin/pad.h>
#include <dolphin/hw_regs.h>
#include <string.h>

void AD16Init(void);
void AD16WriteReg(u32 word);
void BS2SetTime(void);          /* 0x81302034 */
void InitDVDLoader(void);       /* 0x813020C8 */
void BS2Main(void);             /* 0x81300FF4 */
void fn_81301F0C(void);         /* 0x81301F0C */

extern u32 __PADFixBits;
extern u32 __OSCoreClock : 0x800000FC;

static f64 ZeroF;               /* 0x814AD260 */

/* Inlined; its local is what gives BS2Init its 0x18-byte frame. */
static inline u32 GetConsoleRev(void) {
    u32 rev = __PIRegs[11];
    return (rev & 0xF0000000) >> 28;
}

/* 0x813003A0 */
static asm void ClearUnusedBatRegisters(void) {
    nofralloc
    isync
    li r4, 0
    mtdbatl 2, r4
    mtdbatu 2, r4
    mtdbatl 3, r4
    mtdbatu 3, r4
    mtibatl 1, r4
    mtibatu 1, r4
    mtibatl 2, r4
    mtibatu 2, r4
    mtibatl 3, r4
    mtibatu 3, r4
    isync
    blr
}

/* 0x813003D8 */
static asm void ClearFPURegisters(void) {
    nofralloc
    lfd fp0, ZeroF
    fmr fp1, fp0
    fmr fp2, fp0
    fmr fp3, fp0
    fmr fp4, fp0
    fmr fp5, fp0
    fmr fp6, fp0
    fmr fp7, fp0
    fmr fp8, fp0
    fmr fp9, fp0
    fmr fp10, fp0
    fmr fp11, fp0
    fmr fp12, fp0
    fmr fp13, fp0
    fmr fp14, fp0
    fmr fp15, fp0
    fmr fp16, fp0
    fmr fp17, fp0
    fmr fp18, fp0
    fmr fp19, fp0
    fmr fp20, fp0
    fmr fp21, fp0
    fmr fp22, fp0
    fmr fp23, fp0
    fmr fp24, fp0
    fmr fp25, fp0
    fmr fp26, fp0
    fmr fp27, fp0
    fmr fp28, fp0
    fmr fp29, fp0
    fmr fp30, fp0
    fmr fp31, fp0
    blr
}

/* 0x8130045C */
void BS2Init(void) {
    memset((void*)0x80000000, 0, 0x100);
    memset((void*)0x80003000, 0, 0x100);
    ClearUnusedBatRegisters();
    *(u32*)0x80000028 = 0x01800000;
    *(u32*)0x8000002C = 1;
    *(u32*)0x8000002C += GetConsoleRev();
    __PADFixBits = 0xFFFFFFFF;
    ClearFPURegisters();
}

/* 0x813004E4 */
int main(void) {
    BS2Init();
    OSInit();
    AD16Init();
    AD16WriteReg(0x08000000);
    DVDInit();
    AD16WriteReg(0x09000000);
    CARDInit();
    AD16WriteReg(0x0A000000);
    fn_81301F0C();
    __VIInit(VI_TVMODE_PAL_INT);
    VIInit();
    AD16WriteReg(0x0B000000);
    __OSBusClock = 162000000;
    __OSCoreClock = 486000000;
    BS2SetTime();
    InitDVDLoader();
    PADSetSpec(PAD_SPEC_5);
    PADInit();
    AD16WriteReg(0x0C000000);
    BS2Main();
    OSPanic("BS2.c", 163, "BS2 ERROR >>> SHOULD NEVER REACH HERE");
}
