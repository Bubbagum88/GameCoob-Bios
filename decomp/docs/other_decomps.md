# Reusing other GameCube decomps

Survey of public GameCube decompilations and related projects, checked on
2026-10-21, for code, headers and tools this decomp can reuse. The BS2 SDK
was built on Oct 2 2001 (revision 47) with GC/1.2.5n, so the closest match is
any decomp that links that same SDK build.

**License rule:** this repo is CC0. Code from CC0 repos can be copied in
(credit the source in the file header or commit). Repos with no license file
are reference only: read them, then write our own code. GPL repos are
reference only for facts (addresses, struct offsets), never code.

**Never use leaked Nintendo source** (the 2020 "gigaleak" `bb2.zip` holds IPL
source). Do not copy names, comments or code from it, or from any project
that took them from it.

### Provenance

The CC0 decomps below (Pikmin, AC, SMS, MP4, TWW, TP and others) rebuild
their code from the retail binaries. Function and file names come from
linker map files that Nintendo left on retail or demo discs (for example
SMS's `mario.MAP`, AC's maps, TWW's kiosk demo debug maps), which is
public, legitimately obtained data. None of them say they used leaked
source, and the decomp.dev / doldecomp community does not accept leaked
material. We have not audited every line, so if a file in one of them
carries comments or names that no binary or map could explain, skip it.

| Project | Use |
|---|---|
| projectPiki/pikmin, ACreTeam/ac-decomp, doldecomp/sms, mariopartyrd/marioparty4, zeldaret/tww, zeldaret/tp, projectPiki/pikmin2, doldecomp/mkdd, bfbbdecomp/bfbb, ogamespec/dolwin | CC0, clean RE: copy with credit |
| doldecomp/melee, doldecomp/dolsdk2001, doldecomp/dolsdk2004 | clean RE, no license: read only |
| ogamespec/gc-ipl | clean RE (says it ignores the leak), no license: read only |
| OffBroadway/cubeboot, redolution/iplboot, emukidid/swiss-gc | GPL: facts only, no code |
| anything built from the 2020 leak | never |

## Function size check

Every named function in `config/PAL10/symbols.txt` (1201, excluding `fn_`)
was looked up by name in each repo's dtk `symbols.txt` and the sizes
compared. Same size is a strong hint the same source compiles to it.

| Repo (config) | License | Names in common | Same size |
|---|---|---|---|
| projectPiki/pikmin (`DPIJ01_PIKIDEMO`) | CC0 | 773 | **745** |
| projectPiki/pikmin (`GPIE01_01`) | CC0 | 754 | 701 |
| ACreTeam/ac-decomp (`GAFE01_00`) | CC0 | 750 | 506 |
| doldecomp/sms (`GMSE01`) | CC0 | 442 | 400 |
| doldecomp/melee (`GALE01`) | none | 423 | 396 |
| mariopartyrd/marioparty4 (`GMPE01_00`) | CC0 | 425 | 353 |
| zeldaret/tww (`GZLE01`) | CC0 | 435 | 281 |
| projectPiki/pikmin2, doldecomp/mkdd, bfbbdecomp/bfbb, zeldaret/tp | CC0 | ~430 | 129-236 |

Per library (same size / names in common):

| Library | Ours | Pikmin JP demo | Pikmin US 1.1 | Melee | SMS | AC |
|---|---|---|---|---|---|---|
| os | 101 | 88/89 | 83/86 | 85/89 | 89/94 | 85/92 |
| exi | 17 | 16/16 | 16/16 | 12/16 | 11/16 | 11/16 |
| pad | 14 | 12/13 | 3/11 | 7/13 | 8/12 | 8/12 |
| vi | 10 | 6/8 | 7/8 | 4/9 | 4/9 | 4/8 |
| gx | 142 | 132/132 | 117/125 | 129/129 | 125/133 | 124/132 |
| dvd | 22 | 21/21 | 20/21 | 20/20 | 20/21 | 20/21 |
| mtx | 14 | 8/8 | 3/4 | 12/12 | 13/13 | 11/11 |
| jaudio (813376A0..8135A8E0) | 347 | 327/347 | 324/346 | 0/1 | 4/7 | 153/330 |
| runtime + MSL | 68 | 66/67 | 66/67 | 60/60 | 64/64 | 29/57 |

The script that made these tables is a 40-line name/size join; rerun it after
naming more functions to find new hits.

## Pikmin's Japanese demo has our exact SDK build

`projectPiki/pikmin` builds several versions from one tree and switches SDK
code with `OS_BUILD_VERSION` (`include/Dolphin/OS/OSVersion.h`). The
Japanese demo (`DPIJ01_PIKIDEMO`) is `OS_BUILD_VERSION 20011002L`, built
"Oct 2 2001 11:02:22", revision 47: the same build as BS2. Our
`include/dolphin/os/OSVersion.h` already uses the same macro and value, so
Pikmin's `#if OS_BUILD_VERSION >= ...` blocks can be copied as they are.

- Use `config/DPIJ01_PIKIDEMO/` (symbols and splits), not `GPIE01_*`, when
  comparing. The US/JP retail builds use the Jul 19 2001 SDK and differ in
  pad, vi and parts of os.
- Every Pikmin SDK unit is marked Matching: os (24 files incl. OSRtc, OSReset,
  OSFont, OSThread), exi, si, pad, vi, gx (14 files), dvd, card, ai, ar, arq,
  dsp, mtx, db, base, plus Runtime and MSL_C and `src/jaudio`.
- Its header notes that `vi.c` has differences the build date does not
  explain. Our `VIInit` (0x438) and `VIConfigure` (0x690) differ from every
  game checked, so they are probably IPL-specific edits.

## Functions that only other repos have

Names where Pikmin has no same-size hit but another repo does:

- **Heap** (`OSFreeToHeap`, `OSSetCurrentHeap`, `OSCreateHeap`,
  `OSDumpHeap`): same size in Melee, SMS, Mario Party 4, TWW. SMS/MP4/TWW
  are CC0. `OSAddToHeap` (0x60) also exists in dolsdk2001, Melee and MP4.
- **mtx paired-single** (`PSMTXCopy`, `PSMTXMultVec`, `PSVECSubtract`,
  `PSVECNormalize`, `PSVECCrossProduct`) and `C_MTXLookAt`: same size in
  SMS, MP4, TWW, AC.
- **gx**: `GXInitLightSpot` (Melee, MP4), `GXInitLightDistAttn`,
  `GXInitTexObjCI`, `GXInitTlutObj`, `GXLoadTlut` (SMS, MP4, TWW, AC),
  `GXGetVtxDesc` (SMS).
- **os**: `__OSReadROM` (SMS, same size), `OSGetResetSwitchState`
  (SMS, TWW, AC), `OSJamMessage` and `OSCalendarTimeToTicks` (AC).
- **misc**: `LCEnable`, `DLInsert` (SMS, MP4, TWW), `DCTouchRange` (AC),
  MSL `parse_format` (SMS).
- **jaudio**: `DSP_AllocInit__FUc` (AC). AC links the same old C-style
  JAudio as Pikmin (153 same-size), so it is the second place to look for
  JAudio units.

IPL-heavy functions games rarely use (`__OSGetRTC`, `__OSCheckSram`,
`__OSSetTime`, `__VISetAdjustingValues`, `PADSync`, `EXIProbeReset`) have
source in doldecomp/dolsdk2001 (May 2001 SDK), Melee (`libs/dolphin`), TP
(`libs/dolphin`, CC0) and MP4. None had a same-size hit, so expect edits.

The DEMO library BS2 uses (`DEMOInit`, `__DEMOInitGX`, `DEMOBeforeRender`,
`DEMODoneRender`, `DEMOPadInit`, `DEMOPadRead`) has source in
doldecomp/dolsdk2001 `src/demo/` and TP `libs/dolphin/src/demo/` (CC0).

## JSystem

BS2's own code (scene, cube, menu) is C, not JSystem C++. The JSystem in
TWW, TP, SMS and Pikmin 2 is a later C++ library and will not match. Only
the JAudio part applies (lane G, Pikmin first, then AC). Their J3D file
format code is still a good reference for reading model data.

## IPL-specific projects

- **ogamespec/gc-ipl** (no license, reference only): NTSC IPL reverse
  engineering from before 2023. Ghidra-style dumps of BS2Main, the main loop
  and JAudio, a BS1 disassembly that reassembles, the descrambler and font
  notes. Function addresses are NTSC, so map them by code shape. States it
  does not use the leak.
- **OffBroadway/cubeboot** (GPL-2.0, facts only): patches the real BS2 at
  runtime to recolour the cube and swap logos. `patches/linker/link_*.ld`
  lists the same handful of addresses (`main`, `run`, `cube_init`,
  `cube_state`, logo and background models, `pad_status`, `rmode`) for NTSC
  1.0/1.1/1.2 and PAL 1.0/1.1/1.2, and `patches/include/structs.h` gives
  cube state and model struct layouts. Useful to cross-check lane F names and
  later to port the decomp to other IPL versions. Do not copy code.
- **redolution/iplboot** (GPL-2.0): small loader that replaces the IPL on
  PicoBoot-style modchips. Not decomp material, but relevant to the app's
  "boot like a GameCube" goal later.
- **ogamespec/dolwin** (CC0): emulator with hardware notes (`wiki/`), handy
  for register meanings.

No other matching decomp of the IPL was found.

## Tools

- **decomp-permuter** (simonlindholm/decomp-permuter, MIT): for a function
  that is close but not matching (register swaps, instruction order), it
  randomly rewrites the C in ways that keep its meaning (reorder statements,
  add temporaries, swap `if` branches, change types) and recompiles each
  variant, scoring it against the target asm. It runs unattended and prints
  the best variants. It supports PowerPC with mwcc; see
  `mwcceppc_compile_example.sh` in that repo. Use flags from `configure.py`
  (`-O4,p`, GC/1.2.5n via wibo/wine). Needs the compilers, so it runs where
  `build/compilers` exists.
- **Melee `tools/`** (no license, reference only for ideas): `decomp.py`
  (m2c wrapper that writes into the source file), `m2ctx/` (context
  builder), `diff_symbols.py`.
- **SMS `tools/decomp-diff.py`** (CC0): objdiff-cli wrapper that lists a
  unit's symbols by match status and prints one function's diff in text.
  Good for agents working without the objdiff GUI.
- All the dtk-based repos share our `tools/project.py`; their newer copies
  can be diffed against ours for fixes.

## decomp.me from the cloud

Tested 2026-10-21 22:02Z: `decomp.me` and `files.decomp.dev` both get 403
from the cloud sandbox proxy (policy denial). So decomp.me cannot stand in
for the missing compilers. Options: add both hosts to the environment's
allowed domains, or put the compilers zip in the project files
(`toolchain/`) so cloud sessions can unpack it. Locally, decomp.me's
"GameCube / GC/1.2.5n" preset works as a scratchpad.
