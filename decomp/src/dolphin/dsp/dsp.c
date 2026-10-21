#include <dolphin/dsp.h>
#include <dolphin/hw_regs.h>
#include <dolphin/os.h>

u32 DSPCheckMailToDSP(void) {
    return __DSPRegs[DSP_MAILBOX_IN_HI] >> 0xF & 1;
}

u32 DSPCheckMailFromDSP(void) {
    return __DSPRegs[DSP_MAILBOX_OUT_HI] >> 0xF & 1;
}

u32 DSPReadMailFromDSP(void) {
    return (__DSPRegs[DSP_MAILBOX_OUT_HI] << 0x10) | __DSPRegs[DSP_MAILBOX_OUT_LO];
}

void DSPSendMailToDSP(u32 mail) {
    __DSPRegs[DSP_MAILBOX_IN_HI] = mail >> 0x10;
    __DSPRegs[DSP_MAILBOX_IN_LO] = mail;
}
