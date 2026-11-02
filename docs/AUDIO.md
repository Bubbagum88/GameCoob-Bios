# IPL audio map (for the Rust port)

How BS2 (PAL 1.0) makes sound, from the JAudio decomp in `decomp/src/jaudio/`.
JAudio is fully decompiled: every function has C. 669 of 678 compile
byte-exact. The other 9 differ only in register choice and instruction order,
not in logic (`fdiff -r` shows no logic change). 8 of those 9 are NEOS code,
which the IPL never runs (see below).

## In the app (status)

All of BS2's sound is in: the boot jingles (normal, pad-1 Z egg, all-pads Z
egg), the hold-A spin and its whooshes, the spin-out whoosh, the menu's
ambient loop and every `Sound_Play` the menus make (cube faces, Options,
Calendar, Memory Card, Game Play). It is the IPL's own sequence running, not
recordings of it.

- `src/audio/data.rs` finds the archive in the user's dump (`IBNK`, then
  `WSYS`, then the waves, each 32-byte aligned), decodes all 12 waves to PCM
  at load and reads each wave's sound id (WSYS scene `C-DF`).
- `src/audio/bank.rs` reads `IBNK` into instruments, key/velocity regions and
  envelopes.
- `src/audio/jam.rs` is the Jam interpreter (`jammain_2.c`, `jamosc.c`,
  `seqsetup.c`) running `iplrom.com`: the track tree, ports, interrupts,
  timed parameters, registers, every command 0xC0..0xFF. It ticks once per
  DSP subframe (80 samples at 32028.5 Hz) with the root tempo of
  `Jam_InitRoot`.
- `src/audio/voice.rs` is the channel side (`jcs`, `oneshot.c`,
  `bankdrv.c`): `Bank_OscToOfs` envelopes, `UpdateEffecterParam`, pan
  (`PanCalc`), 64 channels with priority stealing.
- `src/audio/mixer.rs` stands in for the DSP. Each voice is resampled once,
  straight to 48 kHz (Catmull-Rom), level ramps across the subframe, f32
  buses, soft limiter. Two echo lines (50-subframe delay, FIR
  {0,0,0,500,1000,4000,7000,8000}, gain 0x6000) feed back into themselves
  and play into left and right.
- `src/audio/mod.rs` is the glue (`Sound_Play`, `Sound_UpdateSpin`,
  `Sound_StartAmbient`/`StopAmbient`, `Sound_SetOutputMode`, reset).
  `src/bs2/sound.rs` queues those calls from the menus; the main loop hands
  them to the engine each frame.
- `src/hda.rs` drives an Intel HD Audio controller (QEMU `intel-hda`, most
  PCs): codec walk, every analog output pin routed to a DAC, one cyclic
  48 kHz stereo stream fed from the main loop.

Mixing facts (the DSP side) come from Dolphin's notes on this microcode
family (the IPL uses the light protocol, four mixing destinations per voice,
no command 0x0D). Dolphin is GPL, so only facts were taken. Two guesses:
bus 9 (0x0FA0, used only by the ambient's lead voice) plays on the right,
and the third echo line's buffer is never played, so it is left out.

Check on the host: `tools/audio-render <ipl.bin> <out-dir> [id | ambient |
hold | spinout | bank ...]` writes 48 kHz WAVs (`sound_<name>.wav`). In
QEMU, add `-audiodev wav,id=snd0,path=out.wav -device intel-hda -device
hda-output,audiodev=snd0`.

Later: a 32 kHz "Original" output for Minimal mode, and the jingle as a menu
option (it is a plain 0/1/2 already).

PAL 1.0 waves: 10 at 32000 Hz and 2 at 44100 Hz, AFC 4-bit. The jingles are
prerecorded stereo (two waves, one per side) of 5.2 to 5.9 s.

## Short version

- BS2 calls only a small glue API: `Sound_Play`, `Sound_UpdateSpin`,
  `Sound_StartAmbient`/`Sound_StopAmbient`, `Sound_SetOutputMode`, plus init,
  restart and stop. All of it is in `jaudio/ipl_8135A8E0.c`.
- Every sound is one sequence file, `iplrom.com`, that runs all the time. BS2
  never starts a sound directly. It writes numbers into the sequence's *ports*,
  and the sequence's own bytecode reacts to them.
- The data lives inside bs2.dol, not on disc. It is a 0xB3CA0-byte archive at
  0x813F7620 with 4 files: bank, wave table, ADPCM wave data, sequence.
- The DSP microcode `jdsp` (0x1760 bytes at 0x813F4BA0) does the mixing:
  ADPCM decode, resampling, volume ramps, 6 mix buses. It belongs to the same
  family as the Pikmin/Zelda JAudio microcode. The Rust side has to rewrite this
  part as plain mixing code (HLE).
- Output is 32028.5 Hz stereo. Each frame is 560 samples in 7 subframes
  (`rate.c`).

## Data: the linked-in sound archive

`jaudio/ipl_8135B360.c`, `fn_8135B360` / `fn_8135B460`:

| file | offset in archive | size | what |
| --- | --- | --- | --- |
| `ipl.bnk` | 0x0 | 0x8E0 | instrument bank: instruments, percussion, key/velocity regions, envelopes (`bankread.c`, `bankdrv.c`) |
| `ipl.ws` | 0x8E0 | 0x680 | wave system table: per-wave format, loop, rate, offset in the .aw (`waveread.c`) |
| `ipl_0.aw` | 0xF60 | 0xB2100 | the wave data itself (AFC 4-bit ADPCM: 9-byte frames of 16 samples, one fixed 16-entry coefficient table), copied to ARAM at boot |
| `iplrom.com` | 0xB3060 | 0xC40 | the sequence (JAudio "Jam" bytecode) |

The archive starts at 0x813F7620 in bs2.dol. The decomp build pulls these bytes
from the user's own dump (`sound_archive.inc`). The app extractor must do the
same and never ship them.

## Boot and teardown

- `bs2main.c` calls `fn_8135B0C0(HeapAlloc(1 MiB), 1 MiB, 8 MiB ARAM)`.
  - `StartAudioThread(..., flags 3)` means DVD + AUDIO. Flag 4 (NEOS) is off.
    So all the NEOS code (`driver.c`, `system.c`, `track.c`, `channel.c`,
    `rspsim.c`, `memory.c`, ported from Animal Crossing) is linked but never
    runs. **Skip NEOS in the port.**
  - It registers the wave table (`ipl.ws`). `WaveScene_Set(0, 0)` then loads
    `ipl_0.aw` into ARAM through the ARAM callback (`fn_8135ACC0`). Next it
    registers the bank (`ipl.bnk`) and starts `iplrom.com` on one sequence
    track (`Jaq_SetSeqData` and `Jaq_StartSeq`).
  - It sets mixer levels 0.8 / 4.0 and builds 4 FX lines from `sFxConfig`.
    Lines 0..2 get delay buffers (`circularBufferSize * 0xA0` bytes). Line 3
    has none.
- `fn_8135B180` (gInit, on reset) stops the sequence, waits one sound frame,
  stops and flushes all one-shots, then starts `iplrom.com` again.
- `fn_8135B200`: stops everything.
- `fn_8135B2C0` returns TRUE once no voice has been active for 30 polls. gInit
  waits on this, then `LCEnable` stops the audio thread before booting a game.

## The glue API (what the Rust menus call)

Sound IDs are listed in `include/bs2/menu.h` (`SE_*`). A *track handle* such
as 0x21002 addresses a track in the running sequence's track tree.

| call | does |
| --- | --- |
| `Sound_Play(0..2)` | track 0x21002, port 0 = id. Boot jingle: 0 is normal, 1 is Z on all four pads, 2 is Z on pad 1. Also resets the spin counter |
| `Sound_Play(3..22)` | track 0x20002, port 0 = id (menu sounds: face turn, enter/exit, cursor, buzzer, card sounds, game start) |
| `Sound_Play(23)` | spin cycle: a counter from 0 to 20 drives `Sound_UpdateSpin` (mode 0 up to 10, then mode 1 back down) |
| `Sound_Play(>23)` | track 0x20001, port 0 = id - 5 |
| `Sound_UpdateSpin(mode, speed)` | `s = sqrt(min(speed,1))`. Track 0x21002 port 1 = 32767 * (1 - s), or 0 in mode 1. If s != 0, track 0x21003: port 3 = a, port 2 = b, and when s > 0.6, port 0 = (u8)min(10/s, 100). Mode 0: a = 2048 * 1.6 * (0.1+s), b = 2048 * (0.1+s). Mode 1: a = 2048 * 3 * (0.15+s), b = 2048 * 3.6 * (0.15+s) |
| `Sound_StartAmbient(0)` | track 0x21001, port 0 = 1 (menu ambient loop) |
| `Sound_StopAmbient()` | track 0x21001, port 1 = 0 |
| `Sound_SetOutputMode(m)` | `Jac_SetOutputMode`: stores the mode (the `OSGetSoundMode` value from SRAM, 0 = mono, 1 = stereo; Options can change it) |

Callers: `cube.c` (jingles), `scene_intro.c` and `scene_spinout.c` (spin),
`menu/*.c` (SE ids), `scene.c` and `menu/game.c` (ambient).

## Engine chain (what to port, in order)

1. **Sequence interpreter.** `jammain_2.c` (Jam_*) runs the `iplrom.com`
   bytecode every subframe: the track tree, ports, timing, note on/off,
   parameter moves. `seqsetup.c` sets tracks up. The bytecode decides what
   each port value means, so read the effect out of `iplrom.com`, not from C
   tables.
2. **Voices.** `noteon.c`, `oneshot.c`, `jamosc.c`, `centcalc.c`: picking an
   instrument or percussion region from the bank, envelope and oscillator
   (vibrato/tremolo), and pitch from cents.
3. **Bank and waves.** `bankread.c`, `bankdrv.c`, `waveread.c`, `virload.c`,
   `aramcall.c`: offsets in the files become pointers, waves get ARAM
   addresses. In the port, ARAM is a plain byte buffer.
4. **DSP channel setup.** `dspdriver.c` (64 `dspch_`) and `dspinterface.c`
   (`DSPchannel_`): resampling ratio (pitch), 6 mixer volumes with
   target/current ramps, bus links, wave info, filters, start/stop.
5. **Mixer (HLE, new code).** For each active channel: decode AFC ADPCM
   from ARAM (9-byte frames, fixed coefficient table; loop history in the
   wave info), resample by `resamplingRatio`, ramp volumes, sum into the
   buses, then run the FX lines (`fxinterface.c`: delay/echo per `sFxConfig`)
   and the final L/R mix (`aictrl.c` `MixDsp`, `Jac_imixcopy`). Write this
   fresh from the C structs. Dolphin's Zelda HLE is GPL: facts only, no code.
6. **Output.** 560 samples per frame at 32028.5 Hz. Resample to the HDA rate
   (48 kHz) in the app's driver (ROADMAP phase 4).

## Units by role

- Glue: `ipl_8135A8E0.c`, `ipl_8135B360.c`.
- Sequence: `jammain_2.c`, `seqsetup.c`, `cmdstack.c`, `playercall.c`.
- Voices: `noteon.c`, `oneshot.c`, `jamosc.c`, `centcalc.c`, `random.c`, `ja_calc.c`.
- Bank/waves: `bankread.c`, `bankdrv.c`, `waveread.c`, `virload.c`, `fat.c`, `heapctrl.c`, `aramcall.c`, `connect.c`.
- DSP: `dspdriver.c`, `dspinterface.c`, `dspbuf.c`, `dspproc.c`, `dspboot.c`, `fxinterface.c`, `driverinterface.c`, `tables.c`.
- Output and threads: `aictrl.c`, `audiothread.c`, `rate.c`, `cpubuf.c`, `dvdthread.c`.
- Not used by the IPL (NEOS): `driver.c`, `system.c`, `track.c`, `channel.c`,
  `rspsim.c`, `memory.c`, `sub_sys.c`, `effect.c`, `os.c`, `astest.c`,
  `neosthread.c`, `audiotable.c`, `audioconst.c`, `audioheaders.c`.
- Other JAudio units not listed (`dsp_cardunlock.c`, `ipldec.c`, `streamctrl.c`,
  `sample.c`, `dummyrom.c`, `dummyprobe.c`) play no part in this chain.
