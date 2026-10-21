# JAudio in BS2 (PAL 1.0)

BS2's sound driver is JAudio, the JSystem sound library Nintendo EAD also
shipped in Pikmin. It sits at `813376A0-8135A8E0`, between the app's own code
and `Sound_*`, every function aligned to 32 bytes (`-func_align 32`).

The units here were found by lining up BS2's function sizes, in address order,
against the Pikmin decomp's `jaudio/*.c` (github.com/projectPiki/pikmin, CC0,
config `DPIJ01_PIKIDEMO`). The sources in `src/jaudio/` start as Pikmin's,
built the same way (C++, `-O4,s -inline off -func_align 32`, see
`cflags_jaudio` in `configure.py`), so function names in `symbols.txt` are
Pikmin's mangled ones. Each function's comment gives its BS2 address and,
where it differs, Pikmin's size.

BS2's JAudio is an older build than Pikmin's: link order differs a little
(`memory.c` is at `8133EE40`, after `stackchecker.c`), several functions are
bigger, and some units have extra IPL-only functions (listed at the end of
each source file, still assembly).

| Unit | BS2 `.text` | Functions | Same size as Pikmin | Named, different size | IPL-only |
|---|---|---|---|---|---|
| `dummyprobe.c` | `813376A0-81337740` | 3 | 3 | 0 | 0 |
| `aictrl.c` | `81338400-81338CA0` | 12 | 8 | 3 | 1 |
| `sample.c` | `81338CA0-81338F40` | 5 | 5 | 0 | 0 |
| `dummyrom.c` | `81338F40-81339220` | 7 | 2 | 1 | 4 |
| `audiothread.c` | `81339220-813396C0` | 10 | 6 | 3 | 1 |
| `streamctrl.c` | `81339B60-81339F00` | 1 | 1 | 0 | 0 |
| `dspbuf.c` | `81339F00-8133A260` | 5 | 5 | 0 | 0 |
| `cpubuf.c` | `8133A260-8133A500` | 3 | 1 | 1 | 1 |
| `playercall.c` | `8133A500-8133A7C0` | 6 | 6 | 0 | 0 |
| `dvdthread.c` | `8133A7C0-8133B560` | 24 | 11 | 0 | 13 |
| `audiomesg.c` | `8133C240-8133CB60` | 16 | 2 | 0 | 14 |
| `stackchecker.c` | `8133ED60-8133EDA0` | 1 | 1 | 0 | 0 |
| `memory.c` | `8133EE40-8133EF20` | 2 | 2 | 0 | 0 |
| `dspboot.c` | `8134D2C0-8134D680` | 4 | 4 | 0 | 0 |
| `dspproc.c` | `8134D680-8134DAC0` | 9 | 8 | 0 | 1 |
| `ipldec.c` | `8134DAC0-8134DD00` | 4 | 3 | 1 | 0 |
| `dsp_cardunlock.c` | `8134DD00-8134E520` | 8 | 8 | 0 | 0 |
| `driverinterface.c` | `8134E520-81350080` | 33 | 20 | 0 | 13 |
| `dspdriver.c` | `81350080-813508E0` | 9 | 9 | 0 | 0 |
| `dspinterface.c` | `813508E0-81351040` | 24 | 20 | 4 | 0 |
| `fxinterface.c` | `81351040-81351180` | 1 | 0 | 1 | 0 |
| `bankread.c` | `81351180-81351580` | 6 | 6 | 0 | 0 |
| `waveread.c` | `81351580-81351AA0` | 7 | 6 | 1 | 0 |
| `connect.c` | `81351AA0-81352160` | 11 | 11 | 0 | 0 |
| `bankdrv.c` | `81352160-813529E0` | 8 | 3 | 5 | 0 |
| `random.c` | `813529E0-81352A80` | 2 | 2 | 0 | 0 |
| `aramcall.c` | `81352AA0-81352DA0` | 6 | 6 | 0 | 0 |
| `ja_calc.c` | `81352DA0-81352F20` | 4 | 4 | 0 | 0 |
| `fat.c` | `81352F20-81353320` | 5 | 5 | 0 | 0 |
| `cmdstack.c` | `81353320-813534C0` | 6 | 6 | 0 | 0 |
| `virload.c` | `813534C0-81353580` | 2 | 2 | 0 | 0 |
| `heapctrl.c` | `81353580-81353980` | 6 | 6 | 0 | 0 |
| `jammain_2.c` | `81353980-81357780` | 94 | 91 | 0 | 3 |
| `noteon.c` | `81357780-81357CE0` | 5 | 5 | 0 | 0 |
| `seqsetup.c` | `81357CE0-81358BC0` | 15 | 9 | 0 | 6 |
| `centcalc.c` | `81358BC0-81358CA0` | 1 | 1 | 0 | 0 |
| `jamosc.c` | `81358CA0-81359140` | 9 | 9 | 0 | 0 |
| `oneshot.c` | `81359140-8135A8E0` | 31 | 31 | 0 | 0 |
| total | | 405 | 328 | 20 | 57 |

Not split yet (no Pikmin counterpart, or edge unclear): `81337740-81338400`
(3 functions before `aictrl.c`), `813396C0-81339B60` after `StartAudioThread`
(another audio thread), `8133B560-8133C240` (likely the rest of
`dvdthread.c`), `8133CB60-8133ED60`, and the big IPL-only block
`8133EDA0-8134D2C0` (except `memory.c`), about 200 functions.

Data blobs: `jdsp` (the DSP microcode, `.data` `813F4BA0`, 0x1760 bytes) is
Nintendo data and is not committed. `configure.py` runs
`tools/extract_blobs.py`, which writes it from your own `bs2.dol` to
`build/PAL10/include/jaudio/jdsp.inc` for `dspboot.c` to include.
