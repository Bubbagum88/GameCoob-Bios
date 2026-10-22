#ifndef NEOS_PR_ABI_H
#define NEOS_PR_ABI_H

/*
 * Minimal audio command list interface used by the NEOS (N64 sound driver
 * on CPU) code. Written from scratch for this project: only the command
 * word layout the driver relies on is described here.
 */

#define _SHIFTL(v, s, w) ((unsigned int)(((unsigned int)(v) & ((1 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w) ((unsigned int)(((unsigned int)(v) >> (s)) & ((1 << (w)) - 1)))

#ifndef ALIGN_NEXT
#define ALIGN_PREV(X, N) ((X) & ~((N) - 1))
#define ALIGN_NEXT(X, N) ALIGN_PREV(((X) + ((N) - 1)), N)
#endif

/* command opcodes */
#define A_SPNOOP 0
#define A_ADPCM 1
#define A_CLEARBUFF 2
#define A_RESAMPLE 5
#define A_SETBUFF 8
#define A_DMEMMOVE 10
#define A_LOADADPCM 11
#define A_MIXER 12
#define A_INTERLEAVE 13
#define A_SETLOOP 15

/* decoder/resampler flags */
#define A_CONTINUE 0x00
#define A_INIT 0x01
#define A_LOOP 0x02

/* samples per ADPCM frame */
#define ADPCMFSIZE 16

typedef struct {
    unsigned int w0;
    unsigned int w1;
} AcmdWords;

typedef union {
    AcmdWords words;
    long long force_align;
} Acmd;

#define aCmd2(pkt, op, hi, lo)                                                                                         \
    {                                                                                                                  \
        Acmd* _a = (Acmd*)pkt;                                                                                         \
        _a->words.w0 = _SHIFTL(op, 24, 8) | hi;                                                                      \
        _a->words.w1 = (unsigned int)(lo);                                                                             \
    }

#define aADPCMdec(pkt, f, s) aCmd2(pkt, A_ADPCM, _SHIFTL(f, 16, 8), s)
#define aClearBuffer(pkt, d, c) aCmd2(pkt, A_CLEARBUFF, _SHIFTL(d, 0, 24), c)
#define aDMEMMove(pkt, i, o, c) aCmd2(pkt, A_DMEMMOVE, _SHIFTL(i, 0, 24), _SHIFTL(o, 16, 16) | _SHIFTL(c, 0, 16))
#define aLoadADPCM(pkt, c, d) aCmd2(pkt, A_LOADADPCM, _SHIFTL(c, 0, 24), d)
#define aMix(pkt, f, g, i, o)                                                                                          \
    aCmd2(pkt, A_MIXER, _SHIFTL(f, 16, 8) | _SHIFTL(g, 0, 16), _SHIFTL(i, 16, 16) | _SHIFTL(o, 0, 16))
#define aResample(pkt, f, p, s) aCmd2(pkt, A_RESAMPLE, _SHIFTL(f, 16, 8) | _SHIFTL(p, 0, 16), s)
#define aSetBuffer(pkt, f, i, o, c)                                                                                    \
    aCmd2(pkt, A_SETBUFF, _SHIFTL(f, 16, 8) | _SHIFTL(i, 0, 16), _SHIFTL(o, 16, 16) | _SHIFTL(c, 0, 16))
#define aSetLoop(pkt, a) aCmd2(pkt, A_SETLOOP, 0, a)

#endif
