// RSP audio microcode simulator. Based on ac-decomp src/static/jaudio_NES/internal/rspsim.c (CC0),
// adapted to the BS2 build (older RspStart: no A_FILTER).
#include "jaudio_NES/rspsim.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "dolphin/os/OSCache.h"
#ifdef __cplusplus
}
#endif
#include "jaudio_NES/sample.h"
#include "jaudio_NES/rate.h"
#include "jaudio_NES/audiocommon.h"
#include "macros.h"

static u8 DMEM[0x1000] ATTRIBUTE_ALIGN(32);
static u32 ADPCM_BOOKBUF_SIZE = 0;
static s16 ADPCM_BOOKBUF[8 * 16] ATTRIBUTE_ALIGN(32);
static s16 FINALR_STATE_BUF[16] ATTRIBUTE_ALIGN(32);

static s16 RES_FILTER[64][4] ATTRIBUTE_ALIGN(32) = {
    0x0C39, 0x66AD, 0x0D46, 0xFFDF, 0x0B39, 0x6696, 0x0E5F, 0xFFD8, 0x0A44, 0x6669, 0x0F83, 0xFFD0, 0x095A, 0x6626,
    0x10B4, 0xFFC8, 0x087D, 0x65CD, 0x11F0, 0xFFBF, 0x07AB, 0x655E, 0x1338, 0xFFB6, 0x06E4, 0x64D9, 0x148C, 0xFFAC,
    0x0628, 0x643F, 0x15EB, 0xFFA1, 0x0577, 0x638F, 0x1756, 0xFF96, 0x04D1, 0x62CB, 0x18CB, 0xFF8A, 0x0435, 0x61F3,
    0x1A4C, 0xFF7E, 0x03A4, 0x6106, 0x1BD7, 0xFF71, 0x031C, 0x6007, 0x1D6C, 0xFF64, 0x029F, 0x5EF5, 0x1F0B, 0xFF56,
    0x022A, 0x5DD0, 0x20B3, 0xFF48, 0x01BE, 0x5C9A, 0x2264, 0xFF3A, 0x015B, 0x5B53, 0x241E, 0xFF2C, 0x0101, 0x59FC,
    0x25E0, 0xFF1E, 0x00AE, 0x5896, 0x27A9, 0xFF10, 0x0063, 0x5720, 0x297A, 0xFF02, 0x001F, 0x559D, 0x2B50, 0xFEF4,
    0xFFE2, 0x540D, 0x2D2C, 0xFEE8, 0xFFAC, 0x5270, 0x2F0D, 0xFEDB, 0xFF7C, 0x50C7, 0x30F3, 0xFED0, 0xFF53, 0x4F14,
    0x32DC, 0xFEC6, 0xFF2E, 0x4D57, 0x34C8, 0xFEBD, 0xFF0F, 0x4B91, 0x36B6, 0xFEB6, 0xFEF5, 0x49C2, 0x38A5, 0xFEB0,
    0xFEDF, 0x47ED, 0x3A95, 0xFEAC, 0xFECE, 0x4611, 0x3C85, 0xFEAB, 0xFEC0, 0x4430, 0x3E74, 0xFEAC, 0xFEB6, 0x424A,
    0x4060, 0xFEAF, 0xFEAF, 0x4060, 0x424A, 0xFEB6, 0xFEAC, 0x3E74, 0x4430, 0xFEC0, 0xFEAB, 0x3C85, 0x4611, 0xFECE,
    0xFEAC, 0x3A95, 0x47ED, 0xFEDF, 0xFEB0, 0x38A5, 0x49C2, 0xFEF5, 0xFEB6, 0x36B6, 0x4B91, 0xFF0F, 0xFEBD, 0x34C8,
    0x4D57, 0xFF2E, 0xFEC6, 0x32DC, 0x4F14, 0xFF53, 0xFED0, 0x30F3, 0x50C7, 0xFF7C, 0xFEDB, 0x2F0D, 0x5270, 0xFFAC,
    0xFEE8, 0x2D2C, 0x540D, 0xFFE2, 0xFEF4, 0x2B50, 0x559D, 0x001F, 0xFF02, 0x297A, 0x5720, 0x0063, 0xFF10, 0x27A9,
    0x5896, 0x00AE, 0xFF1E, 0x25E0, 0x59FC, 0x0101, 0xFF2C, 0x241E, 0x5B53, 0x015B, 0xFF3A, 0x2264, 0x5C9A, 0x01BE,
    0xFF48, 0x20B3, 0x5DD0, 0x022A, 0xFF56, 0x1F0B, 0x5EF5, 0x029F, 0xFF64, 0x1D6C, 0x6007, 0x031C, 0xFF71, 0x1BD7,
    0x6106, 0x03A4, 0xFF7E, 0x1A4C, 0x61F3, 0x0435, 0xFF8A, 0x18CB, 0x62CB, 0x04D1, 0xFF96, 0x1756, 0x638F, 0x0577,
    0xFFA1, 0x15EB, 0x643F, 0x0628, 0xFFAC, 0x148C, 0x64D9, 0x06E4, 0xFFB6, 0x1338, 0x655E, 0x07AB, 0xFFBF, 0x11F0,
    0x65CD, 0x087D, 0xFFC8, 0x10B4, 0x6626, 0x095A, 0xFFD0, 0x0F83, 0x6669, 0x0A44, 0xFFD8, 0x0E5F, 0x6696, 0x0B39,
    0xFFDF, 0x0D46, 0x66AD, 0x0C39,
};

static s16 AD2[4] = { 0x0000, 0x0001, 0xFFFE, 0xFFFF };

static s16 AD4[16] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF,
};

static void Jac_Resample16(s16* input_L_channel, s16* input_R_channel, s16* output_interleaved, s32 input_sample_count,
                           s32 output_sample_count, s16* history_buffer, u16* position_p, s32 is_first_block);

extern void RspStart2(u32* task, s32 tasks, s32 mode) {
    static u32* taskp;
    static s32 alltasks;
    static s32 consumes;

    if (mode == RSPSIM_MODE_INIT) {
        taskp = task;
        alltasks = tasks;
    }

    if (alltasks > 0) {
        consumes = RspStart(taskp, alltasks);
        alltasks -= consumes;
        taskp += consumes * 2;
    }
}

#define DMEM_OFS(ofs) ((s16*)&((u8*)DMEM)[(ofs)])

#pragma scheduling 7400
extern s32 RspStart(u32* pTaskCmds, s32 allTasks) {
    static BOOL init = TRUE;
    s32 i;
    u32 cmdLo;
    u32 cmdHi;
    u16 DMEMCount;
    u16 DMEMIn;
    u32 DMEMOut;
    void* loop_point;
    s16 envParam1_1;
    s16 envParam1_2;
    s16 envParam1_3;
    u16 envParam1_0;
    u16 envParam2_0;
    u16 envParam2_1;
    u16 sp12C;
    u8* sp128;

    if (init) {
        init = FALSE;
    }

    for (i = 0; i < allTasks; i++) {
        cmdLo = pTaskCmds[1];
        cmdHi = pTaskCmds[0];
        pTaskCmds += 2;

        switch (cmdHi >> 24) {
            case A_CMD_LOADCACHE: // A_LOADCACHE (special to GC?)
                sp128 = (u8*)cmdLo;
                sp12C = cmdHi & 0xFFFF;
                DCTouchRange((void*)cmdLo, ((cmdHi >> 16) & 0xFF) * 16);
                break;

            case A_CMD_SPNOOP: // A_SPNOOP
                break;

            case A_CMD_ADPCM: { // A_ADPCM
                u8 flags = cmdHi >> 16;
                void* state = (void*)cmdLo;
                if (flags & 1) {
                    // clear history
                    Jac_bzero(&DMEM[(u16)DMEMOut], 16 * sizeof(s16));
                } else if (flags & 2) {
                    // copy from loop_point
                    Jac_bcopy(loop_point, &DMEM[(u16)DMEMOut], 16 * sizeof(s16));
                } else {
                    // copy from address in command
                    Jac_bcopy(state, &DMEM[(u16)DMEMOut], 16 * sizeof(s16));
                }

                // The IPL build decodes with the two-tap predictor only.
                s32 j;
                s32 k;
                s16* dst = (s16*)&DMEM[(u16)(DMEMOut + 32)];
                s16 h1 = dst[-1];
                s16 h2 = dst[-2];
                u8* src = sp128 + DMEMIn - sp12C;
                u16 frames = (DMEMCount + 31) / 32;
                s32 is2bit = flags & 4;
                for (j = 0; j < frames; j++) {
                    u8 hdr = *src++;
                    s32 scale = hdr >> 4;
                    s16* book = &ADPCM_BOOKBUF[(hdr & 0xF) * 16];
                    s16 c0 = book[0];
                    s16 c1 = book[8];
                    if (is2bit) {
                        for (k = 0; k < 4; k++) {
                            u8 b = *src++;
                            s16 s;
                            s = (AD2[(b >> 6) & 3] << scale) + ((h1 * c1 + h2 * c0) >> 11);
                            h2 = h1; h1 = s; *dst++ = s;
                            s = (AD2[(b >> 4) & 3] << scale) + ((h1 * c1 + h2 * c0) >> 11);
                            h2 = h1; h1 = s; *dst++ = s;
                            s = (AD2[(b >> 2) & 3] << scale) + ((h1 * c1 + h2 * c0) >> 11);
                            h2 = h1; h1 = s; *dst++ = s;
                            s = (AD2[(b >> 0) & 3] << scale) + ((h1 * c1 + h2 * c0) >> 11);
                            h2 = h1; h1 = s; *dst++ = s;
                        }
                    } else {
                        for (k = 0; k < 8; k++) {
                            u8 b = *src++;
                            s16 s;
                            s = (AD4[(b >> 4) & 0xF] << scale) + ((h1 * c1 + h2 * c0) >> 11);
                            h2 = h1; h1 = s; *dst++ = s;
                            s = (AD4[(b >> 0) & 0xF] << scale) + ((h1 * c1 + h2 * c0) >> 11);
                            h2 = h1; h1 = s; *dst++ = s;
                        }
                    }
                }
                Jac_bcopy(dst - 16, state, 16 * sizeof(s16));
                break;
            }

            case A_CMD_CLEARBUFF: // A_CLEARBUFF
                u16 addr = cmdHi & 0xFFFF;
                u16 size = cmdLo & 0xFFFF;
                Jac_bzero(&DMEM[addr], size);
                break;

            case A_CMD_RESAMPLE: { // A_RESAMPLE
                s16 spC[8];
                s16* var_r4;
                s32 j;
                s16* var_r6;
                u32 var_r7;
                s32 var_r8;
                s16* temp_r9_2;
                s32 var_r9;
                s32 var_r14;
                s32 var_r15;
                u16 count;
                void* state = (void*)cmdLo;

                count = DMEMCount / 2;
                var_r14 = cmdHi & 0xFFFF;
                if ((cmdHi >> 16) & 1) {
                    Jac_bzero(spC, 8 * sizeof(s16));
                    var_r7 = 0;
                } else {
                    Jac_bcopy(state, spC, 8 * sizeof(s16));
                    var_r7 = spC[4] & 0x7FFF;
                }
                var_r4 = (s16*)&DMEM[DMEMIn];
                var_r6 = (s16*)&DMEM[(u16)DMEMOut];
                var_r8 = 4;

                for (j = 0; j < (u16)(count >> 1); j++) {
                    temp_r9_2 = RES_FILTER[var_r7 >> 9];
                    var_r9 = temp_r9_2[3] * spC[var_r8 - 1] + temp_r9_2[2] * spC[var_r8 - 2] + temp_r9_2[1] * spC[var_r8 - 3] + temp_r9_2[0] * spC[var_r8 - 4];
                    var_r7 += var_r14;
                    while (var_r7 >= 0x8000) {
                        spC[var_r8] = *var_r4++;
                        spC[var_r8 - 4] = spC[var_r8];
                        var_r8++;
                        var_r7 -= 0x8000;
                        if (var_r8 == 8) {
                            var_r8 = 4;
                        }
                    }
                    temp_r9_2 = RES_FILTER[var_r7 >> 9];
                    var_r15 = temp_r9_2[3] * spC[var_r8 - 1] + temp_r9_2[2] * spC[var_r8 - 2] + temp_r9_2[1] * spC[var_r8 - 3] + temp_r9_2[0] * spC[var_r8 - 4];
                    var_r7 += var_r14;
                    while (var_r7 >= 0x8000) {
                        spC[var_r8] = *var_r4++;
                        spC[var_r8 - 4] = spC[var_r8];
                        var_r8++;
                        var_r7 -= 0x8000;
                        if (var_r8 == 8) {
                            var_r8 = 4;
                        }
                    }
                    var_r9 >>= 15;
                    if (var_r9 > 0x7FFF) {
                        var_r9 = 0x7FFF;
                    }
                    if (var_r9 < -0x8000) {
                        var_r9 = -0x8000;
                    }
                    *var_r6++ = var_r9;
                    var_r15 >>= 15;
                    if (var_r15 > 0x7FFF) {
                        var_r15 = 0x7FFF;
                    }
                    if (var_r15 < -0x8000) {
                        var_r15 = -0x8000;
                    }
                    *var_r6++ = var_r15;
                }
                spC[var_r8] = var_r7 & 0x7FFF;
                Jac_bcopy(&spC[var_r8 - 4], state, 8 * sizeof(s16));
                break;
            }

            case A_CMD_SETBUFF: // A_SETBUFF
                DMEMIn = cmdHi & 0xFFFF;
                DMEMOut = (cmdLo >> 16) & 0xFFFF;
                DMEMCount = cmdLo & 0xFFFF;
                break;

            case A_CMD_DUPLICATE: { // A_DUPLICATE
                s32 j;
                u16 src = cmdHi & 0xFFFF;
                u16 dst = cmdLo >> 16;
                s32 count = (cmdHi >> 16) & 0xFF;

                for (j = 0; j < count; j++) {
                    Jac_bcopy(&DMEM[src], &DMEM[dst], 0x80);
                    src += 0x80;
                    dst += 0x80;
                }
                break;
            }

            case A_CMD_DMEMMOVE: { // A_DMEMMOVE
                u16 src = cmdHi & 0xFFFF;
                u16 dst = cmdLo >> 16;
                u16 size = cmdLo & 0xFFFF;
                Jac_bcopy(&DMEM[src], &DMEM[dst], size);
                break;
            }

            case A_CMD_LOADADPCM: { // A_LOADADPCM
                // The IPL keeps only the first coefficient of each 8-tap row.
                u16 size = cmdHi & 0xFFFF;
                s16* src = (s16*)cmdLo;
                s32 j;
                for (j = 0; j < size; j += 16) {
                    ADPCM_BOOKBUF[j >> 1] = *src;
                    src += 8;
                }
                ADPCM_BOOKBUF_SIZE = size;
                break;
            }

            case A_CMD_ADDMIXER: // A_ADDMIXER
            case A_CMD_MIXER: { // A_MIXER
                u16 size = ((cmdHi >> 16) & 0xFF) * 16;
                u16 in = cmdLo >> 16;
                u16 out = cmdLo & 0xFFFF;
                u16 gain = cmdHi & 0xFFFF;
                u16 count = size / 2;
                s16* src = DMEM_OFS(in);
                s16* dst = DMEM_OFS(out);
                s16 m = gain;
                s32 j;

                for (j = 0; j < count; j++) {
                    s32 mixed = *dst;
                    mixed += (*src++ * m) >> 15;
                    if (mixed > 0x7FFF) {
                        mixed = 0x7FFF;
                    }
                    if (mixed < -0x8000) {
                        mixed = -0x8000;
                    }

                    *dst++ = mixed;
                }
                break;
            }

            case A_CMD_INTERLEAVE: { // A_INTERLEAVE
                u16 inL = cmdLo >> 16;
                u16 inR = cmdLo & 0xFFFF;
                u16 out = cmdHi & 0xFFFF;
                u16 size = ((cmdHi >> 16) & 0xFF) * 16;
                u16 count = size / 2;
                s16* pInL;
                s16* pInR;
                s16* pOut;
                static s32 flag = TRUE;
                static u16 finalr_phase;
                static s16* finalr_state;

                if (flag == TRUE) {
                    finalr_state = FINALR_STATE_BUF;
                }

                pInL = DMEM_OFS(inL);
                pInR = DMEM_OFS(inR);
                pOut = DMEM_OFS(out);
                Jac_Resample16(pInR, pInL, pOut, count, JAC_FRAMESAMPLES >> 2, finalr_state, &finalr_phase, flag);
                flag = FALSE;
                break;
            }

            case A_CMD_SETLOOP: // A_SETLOOP
                loop_point = (void*)cmdLo;
                break;

            case A_CMD_UNK16: { // ???
                s32 stride;
                u32 dst;
                u32 src;
                s32 j;
                s32 count;
                dst = cmdHi & 0xFFFF;
                src = cmdLo >> 16;
                stride = cmdLo & 0xFFFF;
                count = (cmdHi >> 16) & 0xFF;
                for (j = 0; j < count; j++) {
                    Jac_bcopy(&DMEM[(u16)dst], &DMEM[(u16)src], stride);
                    dst += stride;
                    src += stride;
                }
                break;
            }

            case A_CMD_HALFCUT: { // A_INTERL
                s16* src = (s16*)&DMEM[cmdLo >> 16];
                s16* dst = (s16*)&DMEM[cmdLo & 0xFFFF];
                s32 count = cmdHi & 0xFFFF;
                s32 j;

                for (j = 0; j < count; j++) {
                    *dst = *src;
                    src += 2;
                    dst += 1;
                }
                break;
            }

            case A_CMD_LOADBUFFER2: { // A_LOADBUFF2
                u16 size = ((cmdHi >> 16) & 0xFF) * 16;
                Jac_bcopy((void*)cmdLo, (s16*)&DMEM[cmdHi & 0xFFFF], size);
                break;
            }

            case A_CMD_SAVEBUFFER2: { // A_SAVEBUFF2
                u16 size = ((cmdHi >> 16) & 0xFF) * 16;
                Jac_bcopy(DMEM_OFS(cmdHi & 0xFFFF), (void*)cmdLo, size);
                break;
            }

            case A_CMD_SETENVPARAM: { // A_ENVSETUP1
                u8 temp = cmdHi >> 16;
                envParam1_1 = cmdHi & 0xFFFF;
                envParam1_2 = cmdLo >> 16;
                envParam1_3 = cmdLo & 0xFFFF;
                envParam1_0 = temp << 8;
                break;
            }

            case A_CMD_SETENVPARAM2: // A_ENVSETUP2
                envParam2_0 = cmdLo >> 16;
                envParam2_1 = cmdLo & 0xFFFF;
                break;

            case A_CMD_ENVMIXER: { // A_ENVMIXER
                // The IPL build mixes the dry pair only (no wet/Haas path).
                s32 temp0;
                u8 temp1;
                u8 temp2;
                s32 size;
                s16* in;
                s16* outL;
                s16* outR;
                s32 j;
                u16 count;

                temp1 = cmdHi & 2;
                temp2 = cmdHi & 1;
                temp0 = (u8)(cmdHi >> 8);

                size = temp0 << 1;
                in = (s16*)&DMEM[((cmdHi >> 16) & 0xFF) * 16];
                outL = (s16*)&DMEM[((cmdLo >> 24) & 0xFF) * 16];
                outR = (s16*)&DMEM[((cmdLo >> 16) & 0xFF) * 16];
                DCTouchRange(outL, size);
                DCTouchRange(outR, size);

                count = temp0 >> 1;
                for (j = 0; j < count; j++) {
                    s32 l = *in++;
                    s32 r = *in++;
                    s32 lL = (l * envParam2_0) >> 16;
                    s32 lR = (l * envParam2_1) >> 16;
                    s32 rL = (r * envParam2_0) >> 16;
                    s32 rR = (r * envParam2_1) >> 16;
                    s32 o0, o1, o2, o3;

                    if (temp1) {
                        lL = -lL;
                        rL = -rL;
                    }
                    if (temp2) {
                        lR = -lR;
                        rR = -rR;
                    }
                    o0 = outL[0] + lL;
                    o1 = outL[1] + rL;
                    o2 = outR[0] + lR;
                    o3 = outR[1] + rR;
                    if (o0 > 0x7FFF) {
                        o0 = 0x7FFF;
                    }
                    if (o0 < -0x8000) {
                        o0 = -0x8000;
                    }
                    if (o1 > 0x7FFF) {
                        o1 = 0x7FFF;
                    }
                    if (o1 < -0x8000) {
                        o1 = -0x8000;
                    }
                    if (o2 > 0x7FFF) {
                        o2 = 0x7FFF;
                    }
                    if (o2 < -0x8000) {
                        o2 = -0x8000;
                    }
                    if (o3 > 0x7FFF) {
                        o3 = 0x7FFF;
                    }
                    if (o3 < -0x8000) {
                        o3 = -0x8000;
                    }
                    *outL++ = o0;
                    *outL++ = o1;
                    *outR++ = o2;
                    *outR++ = o3;
                    if ((j & 3) == 3) {
                        envParam2_0 += envParam1_2;
                        envParam2_1 += envParam1_3;
                        envParam1_0 += envParam1_1;
                    }
                }
                break;
            }

            case A_CMD_PCM8DEC: { // A_S8DEC
                u8 flags = cmdHi >> 16;
                void* state = (void*)cmdLo;

                if (flags & 1) {
                    // clear history
                    Jac_bzero(DMEM_OFS(DMEMOut), 16 * sizeof(s16));
                } else if (flags & 2) {
                    // copy from loop_point
                    Jac_bcopy(loop_point, DMEM_OFS(DMEMOut), 16 * sizeof(s16));
                } else {
                    // copy from address in command
                    Jac_bcopy(state, DMEM_OFS(DMEMOut), 16 * sizeof(s16));
                }

                u16 temp1 = DMEMOut + 32;
                s16* dst = (s16*)&DMEM[temp1];
                u8* src = sp128 + DMEMIn - sp12C;
                u16 count = DMEMCount / 2;
                s32 j;

                for (j = 0; j < count; j++) {
                    *dst++ = (*src++) << 8;
                }

                u16 temp2 = DMEMOut;
                temp2 += 32;
                Jac_bcopy(&DMEM[temp2] - 32, state, 32);
                break;
            }

            case A_CMD_EXIT: // ??? (special to GC?)
                return i + 1;
        }
    }
    return allTasks;
}

static void Jac_Resample16(
    s16* input_L_channel,
    s16* input_R_channel,
    s16* output_interleaved,
    s32 input_sample_count,
    s32 output_sample_count,
    s16* history_buffer,
    u16* position_p,
    s32 is_first_block
) {
    u32 current_pos_fract;
    s32 circular_buf_idx;
    s16* history_write_ptr;
    u32 next_pos_fract;
    s32 interpolated_sample_L, interpolated_sample_R;
    s32 step_increment;
    s32 i;
    STACK_PAD_VAR(2);

    // Calculate the resampling ratio as a 16.16 fixed-point number.
    // This determines how much to step through the input for each output sample.
    step_increment = (input_sample_count << 16) / output_sample_count;

    if ((is_first_block & 1) != 0) {
        // FIrst block, clear the history buffer and reset the position.
        Jac_bzero(history_buffer, 16 * sizeof(s16)); // Zero out 32 bytes (16 samples)
        current_pos_fract = 0;
    } else {
        // If this is not the first block of audio, load the last fractional position.
        current_pos_fract = (u16)*position_p;
    }

    // This loop appears to be a safeguard, possibly to handle edge cases where the
    // step increment is very large. It adjusts the step value to prevent over-reading.
    while (TRUE) {
        // Predict the position after processing all output samples.
        next_pos_fract = current_pos_fract + step_increment * output_sample_count;
        // The position is stored in 16.16 fixed point, so we shift to get the integer part.
        if ((next_pos_fract >> 16) < input_sample_count) {
            step_increment++;
        } else if ((next_pos_fract >> 16) > input_sample_count + 1) {
            // If we'd read past the end of the input, reduce the step slightly and re-check.
            step_increment--;
        } else {
            break;
        }
    }

    // Initialize the index for our circular history buffer.
    circular_buf_idx = 4;
    for (i = 0; i < output_sample_count; i++) {
        // --- Polyphase FIR Filter Calculation ---
        // Use the top bits of the fractional position to look up coefficients from the filter table.
        s16* filter_table = RES_FILTER[current_pos_fract >> 10];

        // LEFT CHANNEL: Apply 4-tap FIR filter (multiply-accumulate)
        // It multiplies 4 historical samples by 4 filter coefficients.
        interpolated_sample_L =
            filter_table[3] * history_buffer[circular_buf_idx - 1] +
            filter_table[2] * history_buffer[circular_buf_idx - 2] +
            filter_table[1] * history_buffer[circular_buf_idx - 3] +
            filter_table[0] * history_buffer[circular_buf_idx - 4];
        interpolated_sample_L >>= 15;

        // RIGHT CHANNEL: Apply 4-tap FIR filter
        // The history buffer stores L and R channels interleaved. We access the R samples at an offset of +4.
        interpolated_sample_R =
            filter_table[3] * history_buffer[circular_buf_idx + 7] +
            filter_table[2] * history_buffer[circular_buf_idx + 6] +
            filter_table[1] * history_buffer[circular_buf_idx + 5] +
            filter_table[0] * history_buffer[circular_buf_idx + 4];
        interpolated_sample_R >>= 15;

        // --- Clamping and Output ---
        // Clamp the Left sample to the valid 16-bit range [-32768, 32767].
        if (interpolated_sample_L > 0x7fff) interpolated_sample_L = 0x7fff;
        if (interpolated_sample_L < -0x8000) interpolated_sample_L = -0x8000;

        // Clamp the Right sample.
        if (interpolated_sample_R > 0x7fff) interpolated_sample_R = 0x7fff;
        if (interpolated_sample_R < -0x8000) interpolated_sample_R = -0x8000;

        // Write the interleaved stereo pair to the output buffer.
        *output_interleaved++ = (short)interpolated_sample_L;
        *output_interleaved++ = (short)interpolated_sample_R;

        // Advance the fractional position and the output pointer.
        current_pos_fract += step_increment;

        // --- History Buffer Management ---
        // If the integer part of our position has advanced, we need to pull new samples
        // from the input buffers into our circular history buffer.
        while (current_pos_fract >= 0x10000) {
            s16 read;
            
            history_write_ptr = &history_buffer[circular_buf_idx++];

            // Load next sample from Left channel input.
            read = *input_L_channel++;
            history_write_ptr[0] = read;
            history_write_ptr[-4] = history_write_ptr[0]; // Part of circular buffer logic

            // Load next sample from Right channel input.
            read = *input_R_channel++;
            history_write_ptr[8] = read;
            history_write_ptr[4] = history_write_ptr[8]; // Part of circular buffer logic

            // Decrement the fractional part of the position tracker.
            current_pos_fract -= 0x10000;

            // Wrap the circular buffer index. It holds 4 stereo pairs (8 samples total).
            if (circular_buf_idx == 8) {
                circular_buf_idx = 4;
            }
        }
    }

    // --- State Saving ---
    // Save the final fractional position for the next call.
    *position_p = (u16)current_pos_fract;

    // Re-arrange the history buffer so the last 4 samples are at the beginning,
    // ready for the next call to the function. This preserves the state.
    {
        s16* src_ptr;
        s16* dest_ptr;
        s32 i;

        for (i = 0; i < 4; i++) {
            src_ptr = &history_buffer[circular_buf_idx - 4 + i];
            dest_ptr = &history_buffer[i];
    
            dest_ptr[0] = src_ptr[0]; // Copy Left channel sample
            dest_ptr[8] = src_ptr[8]; // Copy Right channel sample
        }
    }
}
