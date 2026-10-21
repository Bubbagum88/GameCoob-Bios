# SDK and runtime libraries in BS2 (PAL 1.0)

Map of the Dolphin SDK, MSL and runtime code linked into `bs2.dol`, from the
`dtk dol split` asm (`build/PAL10/asm/auto_01_813003A0_text.s`) and the Ghidra
project (`ghidra/bs2_PAL10.gpr`, imported from `build/PAL10/bs2.elf`).

The binary has no `<< Dolphin SDK - ... >>` version strings (the IPL predates
them), no `.comment` section and no extab, so libraries were placed by:

- hardware register blocks each function touches (`lis rX, 0xCC00` + offset):
  VI `0x2000`, PI `0x3000`, DSP/AR `0x5000`, DI `0x6000`, SI `0x6400`,
  EXI `0x6800`, AI `0x6C00`
- calls to and from already named functions, and the data symbols they use
- the usual SDK link order (base, os, db, mtx, dvd, vi, pad, ai, ar, arq, dsp,
  card, gx, then the runtime and MSL)

Ranges are start-inclusive, end-exclusive, and approximate at the edges
between neighbouring unnamed functions.

## Compiler

The Dolphin SDK in BS2 was built with **GC/1.2.5n** and `-fp_contract off`
(set in `configure.py` as `DolphinLib` / `cflags_dolphin`).

- Test compile of the SDK `OSMessage.c` (`OSInitMessageQueue`, `OSSendMessage`,
  `OSReceiveMessage`, `OSJamMessage`) with the base flags: GC/1.2.5n matches
  all four functions. GC/1.0, 1.1 and 1.2.5 match only the leaf-like
  `OSInitMessageQueue`; GC/1.1p1, 1.3, 1.3.2, 2.0 and 2.6 match none.
- `C_MTXLookAt` only matches with `-fp_contract off` (with it on, mwcc fuses
  the dot products into `fmadds`); everything else matches either way.
- Unreferenced functions are dead-stripped by the linker, but their string
  literals and float constants stay in the pools, so a matching file has to
  keep them (e.g. `C_MTXIdentity`, `C_MTXFrustum`, `OSCheckHeap`).

Other evidence: all 1321 non-leaf prologues use `mflr r0; stw r0, 0x4(r1);
stwu r1, ...` (pre-3.0 order), `mr` is emitted as `addi rD, rS, 0`, and the
code calls `_savefpr_NN` / `_restfpr_NN` but never `_savegpr_NN`.

## Matched units

| Unit | Functions |
|---|---|
| `dolphin/base/PPCArch.c` | 11 |
| `dolphin/os/OSAlarm.c` | 7 |
| `dolphin/os/OSAlloc.c` | 8 (+ stripped `OSAllocFixed`, `OSDestroyHeap`, `OSCheckHeap`) |
| `dolphin/os/OSArena.c` | 4 |
| `dolphin/os/OSCache.c` | 13 (built with `-inline noauto` so `L2GlobalInvalidate` stays a call) |
| `dolphin/os/OSError.c` | 4 |
| `dolphin/os/OSInterrupt.c` | 11 |
| `dolphin/os/OSMessage.c` | 4 |
| `dolphin/mtx/mtx.c` | 8 (+ stripped `C_MTXIdentity`) |
| `dolphin/mtx/mtxvec.c` | 1 |
| `dolphin/mtx/mtx44.c` | 2 (+ stripped `C_MTXFrustum`) |
| `dolphin/mtx/vec.c` | 3 |

Partial (NonMatching, source in `src/`):

- `dolphin/os/OSAudioSystem.c`: `__OSStopAudioSystem` matches; `__OSInitAudioSystem`
  is 106/111 instructions (the `li r0, 0x8AC` is scheduled one slot early).
- `dolphin/os/OSContext.c`: 11 of 12 functions match; `OSDumpContext` loads the
  current context into `r0` and copies it to `r30` in the original.

## Map

| Range | Library | Notes |
|---|---|---|
| `81300000-813001C8` | Runtime `__start.c` | `__start`, `__init_registers`, `__init_data`, `__init_hardware`, `__flush_cache` (.init) |
| `813001C8-81300390` | Runtime `__mem.c` | `memset`, `__fill_mem`, `memcpy` (.init) |
| `813003A0-8135B57C` | BS2 application | see below |
| `8135B57C-8135B5E0` | base `PPCArch` | `PPCMtdec`, `PPCMfhid2`, `PPCMthid2`, `PPCMtwpar` and unnamed mf/mt helpers |
| `8135B5E0-8135BE68` | os `OS.c` | `OSGetConsoleType`, `OSInit`, exception install (`fn_8135BA54`), `__OSSetExceptionHandler`, `__OSGetExceptionHandler` |
| `8135BE68-8135C5BC` | os `OSAlarm` | `OSInitAlarm`, `OSCreateAlarm`, `InsertAlarm`, `OSSetAlarm`, `fn_8135C178` (OSCancelAlarm), `DecrementerExceptionCallback/Handler` |
| `8135C5BC-8135C984` | os `OSAlloc` | `OSAllocFromHeap`, `OSFreeToHeap`, `OSSetCurrentHeap`, `OSInitAlloc`, `OSCreateHeap`, `OSAddToHeap`, `OSDumpHeap` |
| `8135C984-8135C9A4` | os `OSArena` | `OSGet/SetArenaHi/Lo` |
| `8135C9A4-8135CC4C` | os `OSAudioSystem` | `fn_8135C9A4` (__OSInitAudioSystem, DSP regs), `__OSStopAudioSystem` |
| `8135CC4C-8135D0BC` | os `OSCache` | `DCInvalidateRange`, `DCFlushRange`, `DCStoreRange`, `DCFlushRangeNoSync`, `ICFlashInvalidate`, `fn_8135CE68` (DMA error handler), `fn_8135CFC8` (__OSCacheInit) |
| `8135D0BC-8135D924` | os `OSContext` | `OSSetCurrentContext`, `OSLoadContext`, `OSClearContext`, `fn_8135D5B0` (OSDumpContext), `fn_8135D8DC` (__OSContextInit) |
| `8135D924-8135DD54` | os `OSError` | `OSReport`, `OSPanic`, `OSSetErrorHandler`, `fn_8135DAEC` (__OSUnhandledException) |
| `8135DD54-8135EF94` | EXI BIOS (`EXIBios.c`, linked inside os here) | EXI regs; used by `__OSGetRTC`, `__OSReadROM`, `AD16*`, `CARDProbe`; `fn_8135ECA8` is EXIInit |
| `8135EF94-8135F250` | os `OSExi`/AD16 | `AD16Init`, `AD16WriteReg` (+2 unnamed) |
| `8135F250-8135FAB0` | os `OSInterrupt` | `OSDisableInterrupts`, `OSRestoreInterrupts`, `__OSSetInterruptHandler`, `__OSUnmaskInterrupts`, `fn_8135F340` (SetInterruptMask), `fn_8135F728` (__OSDispatchInterrupt) |
| `8135FAB0-8135FE7C` | os `OSMessage`, `OSMemory`/`OSMutex` | `OSInitMessageQueue`, `OSSendMessage`, `OSReceiveMessage` (+7 unnamed) |
| `8135FE7C-81360244` | os `OSReset`/`OSResetSW` | PI reset regs |
| `81360244-81361038` | os `OSRtc` | `__OSGetRTC`, `__OSLockSram(Ex)`, `__OSUnlockSram(Ex)`, `__OSSyncSram`, `__OSCheckSram`, `__OSReadROM`, `OSGet/SetSoundMode`; `fn_81360F30`/`fn_81360FFC` used by `PADRead` (wireless ID) |
| `81361038-813626D8` | SI BIOS (`SIBios.c`, linked inside os here) | SI regs; `fn_81361814` is SIInit |
| `813626D8-8136275C` | os `OSSync` | |
| `8136275C-81363764` | os `OSThread` | `OSInitThreadQueue`, `OSDisable/EnableScheduler`, `SelectThread`, `__OSReschedule`, `OSCreateThread`, `OSCancelThread`, `OSResumeThread`, sleep/wakeup (`fn_8136356C`, `fn_81363658`) |
| `81363764-81363F00` | os `OSTime` | `OSGetTime`, `OSGetTick`, `__OSSetTime`, `__OSGetSystemTime`, `OSTicksToCalendarTime`, `OSCalendarTimeToTicks` |
| `81363F00-813641C4` | os `__ppc_eabi_init` / `__start` helpers | `fn_81364170` references `_ctors` (__init_cpp) |
| `813641C4-813642D0` | db | `DBPrintf` and DB init helpers |
| `813642D0-8136498C` | mtx | `PSMTXIdentity`, `PSMTXCopy`, `PSMTXConcat`, `PSMTXTranspose`, `PSMTXTrans`, `PSMTXScale`, `C_MTXLookAt`, `PSMTXMultVec`, `C_MTXPerspective`, `C_MTXOrtho`, vec helpers |
| `8136498C-81368D54` | dvd | dvdlow (`__DVDInitWA`, `__DVDInterruptHandler`, `DVDLow*`), dvdfs (`__DVDFSInit`, `DVDConvertPathToEntrynum`, `DVDOpen`, `DVDClose`), dvd (`DVDInit`, state machine, `DVDReadAbsAsyncForBS`, `DVDReadDiskID`, `DVDReset`, `DVDGetDriveStatus`, `DVDCancelAsync`), dvdqueue, dvderror (`__DVDStoreErrorCode`), fstload (`__fstLoad`) |
| `81368D54-8136A7CC` | vi | `fn_81368D54` (__VIRetraceHandler), `__VIInit`, `VIInit`, `VIWaitForRetrace`, `VIConfigure`, `VIFlush`, `VISetNextFrameBuffer`, `VISetBlack`, `VIGetNextField`, `VIGetTvFormat`, `__VISetAdjustingValues` |
| `8136A7CC-8136C408` | pad | `PADClamp`, `PADInit`, `PADRead`, `PADSetSpec`, SPEC*_MakeStatus helpers |
| `8136C408-8136CDB4` | ai | AI regs (+ DSP block for AID DMA); `fn_8136C908` is AIInit |
| `8136CDB4-8136E0F8` | ar | `fn_8136CDB4` (ARRegisterDMACallback), `fn_8136CDF8` (ARStartDMA), `fn_8136CFE4` (__ARHandler), `fn_8136D05C` (__ARChecksize). The symbol named `DSPInit` at `8136CEE8` sits here and calls __ARChecksize, so it is probably **ARInit** |
| `8136E0F8-8136E2BC` | arq | `ARQInit` and queue helpers |
| `8136E2BC-8136E308` | dsp | four mailbox accessors on DSP regs (DSPCheckMailToDSP etc.) |
| `8136E308-81373428` | card | `fn_8136E308`/`fn_8136E30C` (default/sync API callbacks), `CARDInit`, `CARDProbe`; mount, format, open, read, write, delete, rename, stat (called from `CardThread_*`) |
| `81373428-8137AB60` | gx | GXInit, GXFifo, GXAttr, GXMisc, GXGeometry, GXFrameBuf, GXLight, GXTexture, GXBump, GXTev, GXPixel, GXDisplayList, GXTransform (all named `GX*` symbols) |
| `8137AB60-8137B854` | Runtime (PowerPC_EABI_Support) | `__va_arg`, `__destroy_global_chain`, `__cvt_fp2unsigned`, `__save_fpr`/`__restore_fpr`, `__div2u`, `__div2i`, `__mod2u`, `fn_8137B098` (__mod2i) |
| `8137B854-8137E660` | MSL_C | buffer_io, file_io, ansi_files (`__files`), `fwrite`, `wcstombs`, `memchr`, `vprintf`, `__pformatter`, `float2str`, `longlong2str`, `long2str`, `parse_format`, `qsort`, `strchr`, `strcmp`, `strcpy`, `strlen`, `fwide` |
| `8137E660-8137EAA0` | MSL_C math | `tanf`, `sinf` (+ kernels); `fn_8137EA1C` (FORCEACTIVE) |

## BS2 application (`813003A0-8135B57C`)

Not SDK, listed for scale: `BS2Init`, `main`, `BS2Main`, the IPL's own DEMO
layer (`DEMOInit`, `DEMOPadRead`, `__DEMOInitGX`), the J3D-style model code
(`J3D_*`), math helpers (`Mtx*`, `SinS`, `CosS`, `Hermite`), the heap, `Yay0Decode`,
the scene and cube intro, the menus and memory card UI, and `Sound_*`
(`8135A8E0-8135B180`). `813376A0-8135A8E0` is JAudio, the JSystem sound
library: see `jaudio.md`. `8135ACC0-8135B57C` (`LCEnable`, game boot helpers
called by `Disc_StartGame`) is low-level boot code that may belong to the app
or to an IPL-only part of os; it needs a closer look.

## Not present

No AX/AXFX/MIX (sound talks to the DSP directly), no separate EXI or SI
libraries (their BIOS files are linked inside os), no THP, no C++ runtime
beyond the empty `__init_cpp_exceptions` stub.
