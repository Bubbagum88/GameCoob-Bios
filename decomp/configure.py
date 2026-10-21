#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "PAL10",  # 0: PAL IPL revision 1.0 (md5 0cdda509e2da83c85bfe423dd87346cc)
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

# BS2 is not shipped as a DOL: build one from the user's own IPL dump first.
_ipl = Path("orig") / str(args.version) / "ipl.bin"
_dol = Path("orig") / str(args.version) / "bs2.dol"
if args.mode == "configure" and not _dol.exists():
    if not _ipl.exists():
        sys.exit(f"Put your IPL dump at {_ipl} (see README.md)")
    import subprocess
    subprocess.check_call([sys.executable, "tools/extract_bs2.py", str(_ipl), str(_dol)])
if args.mode == "configure" and _dol.exists():
    # Data blobs some sources #include (never committed), see tools/extract_blobs.py
    import subprocess
    subprocess.check_call([sys.executable, "tools/extract_blobs.py", str(_dol),
                           str(args.build_dir / str(args.version) / "include"), str(args.version)])

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-2"
config.compilers_tag = "20251118"
config.dtk_tag = "v1.8.3"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.2"
config.wibo_tag = "1.0.3"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# BS1, the boot stub in front of BS2, is hand-written assembly. It is built on
# its own with GNU as and checked against config/<version>/bs1.sha1.
_bs1_sha = Path("config") / config.version / "bs1.sha1"
if _bs1_sha.exists():
    _bs1_dir = Path(config.build_dir) / config.version / "bs1"
    _binutils = Path(config.binutils_path or Path(config.build_dir) / "binutils")
    _dtk = config.dtk_path or Path(config.build_dir) / "tools" / "dtk"
    config.custom_build_rules = [
        {
            "name": "bs1_as",
            "command": f"{_binutils / 'powerpc-eabi-as'} -mgekko -mregnames -o $out $in",
            "description": "AS $in",
        },
        {
            "name": "bs1_bin",
            "command": f"{_binutils / 'powerpc-eabi-objcopy'} -O binary -j .text $in $out",
            "description": "OBJCOPY $out",
        },
        {
            "name": "bs1_check",
            "command": f"{_dtk} shasum -q -c $in -o $out",
            "description": "CHECK $in",
        },
    ]
    config.custom_build_steps = {
        "post-build": [
            {
                "outputs": _bs1_dir / "bs1.o",
                "rule": "bs1_as",
                "inputs": Path("src") / "bs1" / "bs1.s",
                "implicit": [_binutils],
            },
            {
                "outputs": _bs1_dir / "bs1.bin",
                "rule": "bs1_bin",
                "inputs": _bs1_dir / "bs1.o",
            },
            {
                "outputs": _bs1_dir / "bs1.ok",
                "rule": "bs1_check",
                "inputs": _bs1_sha,
                "implicit": [_bs1_dir / "bs1.bin", _dtk],
            },
        ]
    }

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
config.scratch_preset_id = None

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i include",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Warning flags
if args.warn == "all":
    cflags_base.append("-W all")
elif args.warn == "off":
    cflags_base.append("-W off")
elif args.warn == "error":
    cflags_base.append("-W error")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
]

# Runtime/MSL_C/TRK base flags (as in the Pikmin decomp): no fused
# multiply-add by default, and include/stl searched before include/
cflags_msl_base = [
    "-i include/stl",
    *[f for f in cflags_base if f != "-fp_contract on"],
    "-fp_contract off",
]

# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
]

config.linker_version = "GC/1.1"

# Dolphin SDK flags: BS2's SDK was built without fused multiply-add
cflags_dolphin = [
    *cflags_base,
    "-fp_contract off",
]


# JAudio (JSystem sound library) flags, from the Pikmin decomp's: JAudio there
# is the same library, built as C++ with 32-byte function alignment.
cflags_jaudio = [
    "-nodefaults",
    "-proc gekko",
    '-pragma "scheduling 7400"',
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,s",
    "-inline off",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse,readonly",
    "-multibyte",
    "-i include",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
    "-common on",
    "-func_align 32",
    "-lang c++",
    "-DNDEBUG=1",
    "-w off",
    "-use_lmw_stmw on",
]


# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_dolphin,
        "progress_category": "sdk",
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_rel,
        "progress_category": "game",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    # Runtime, MSL_C and TRK glue: sources follow the Pikmin decomp
    # (projectPiki/pikmin, CC0), which links the same library build.
    # include/stl comes first so their <stdio.h> etc. win over include/.
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_msl_base, "-use_lmw_stmw on", "-str reuse,readonly", "-common off", "-inline auto", "-inline deferred"],
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "Runtime/PPCEABI/H/__va_arg.c"),
            Object(NonMatching, "Runtime/PPCEABI/H/global_destructor_chain.c"),
            Object(NonMatching, "Runtime/PPCEABI/H/runtime.c"),
        ],
    },
    {
        "lib": "MSL_C.PPCEABI.bare.H",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_msl_base, "-fp_contract on", "-inline auto,deferred", "-str pool,readonly"],
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/ansi_files.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/abort_exit.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/ansi_fp.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/buffer_io.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/critical_regions.ppc_eabi.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/ctype.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/direct_io.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/mbstring.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/mem.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/mem_funcs.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/misc_io.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/printf.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/qsort.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/rand.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/string.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/uart_console_io.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/wchar_io.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/e_asin.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/e_atan2.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/s_atan.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/s_frexp.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/w_atan2.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/hyperbolicsf.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/trigf.c"),
            Object(NonMatching, "MSL_C/PPCEABI/bare/H/common_float_tables.c"),
        ],
    },
    {
        "lib": "TRK_MINNOW_DOLPHIN",
        "mw_version": "GC/1.1p1",
        "cflags": [*cflags_msl_base, "-use_lmw_stmw on", "-str reuse,readonly", "-common off", "-rostr", "-sdata 0", "-sdata2 0", "-pool off", "-inline on,noauto"],
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "TRK_MINNOW_DOLPHIN/dolphin_trk_glue.c"),
        ],
    },
    DolphinLib(
        "base",
        [
            Object(Matching, "dolphin/base/PPCArch.c"),
        ],
    ),
    DolphinLib(
        "os",
        [
            Object(NonMatching, "dolphin/os/OS.c"),
            Object(Matching, "dolphin/os/OSAlarm.c"),
            Object(Matching, "dolphin/os/OSAlloc.c"),
            Object(Matching, "dolphin/os/OSArena.c"),
            Object(NonMatching, "dolphin/os/OSAudioSystem.c"),
            # Built without auto-inlining: L2GlobalInvalidate stays a call
            Object(Matching, "dolphin/os/OSCache.c", extra_cflags=["-inline noauto"]),
            Object(NonMatching, "dolphin/os/OSContext.c"),
            Object(Matching, "dolphin/os/OSError.c"),
            Object(NonMatching, "dolphin/os/OSExi.c"),
            Object(NonMatching, "dolphin/os/OSExiAd16.c"),
            Object(NonMatching, "dolphin/os/OSFont.c"),
            Object(Matching, "dolphin/os/OSInterrupt.c"),
            Object(NonMatching, "dolphin/os/OSLink.c"),
            Object(Matching, "dolphin/os/OSMessage.c"),
            Object(Matching, "dolphin/os/OSMutex.c"),
            Object(NonMatching, "dolphin/os/OSReset.c"),
            Object(Matching, "dolphin/os/OSResetSW.c"),
            Object(NonMatching, "dolphin/os/OSRtc.c"),
            Object(Matching, "dolphin/os/OSSync.c"),
            Object(Matching, "dolphin/os/OSThread.c"),
            Object(Matching, "dolphin/os/OSTime.c", extra_cflags=["-inline noauto"]),
        ],
    ),
    DolphinLib(
        "gx",
        [
            Object(Matching, "dolphin/gx/GXInit.c"),
            Object(Matching, "dolphin/gx/GXFifo.c"),
            Object(Matching, "dolphin/gx/GXAttr.c"),
            Object(Matching, "dolphin/gx/GXMisc.c"),
            Object(Matching, "dolphin/gx/GXGeometry.c"),
            Object(Matching, "dolphin/gx/GXFrameBuf.c"),
            Object(Matching, "dolphin/gx/GXLight.c"),
            Object(Matching, "dolphin/gx/GXTexture.c"),
            Object(Matching, "dolphin/gx/GXBump.c"),
            Object(Matching, "dolphin/gx/GXTev.c"),
            Object(Matching, "dolphin/gx/GXPixel.c"),
            Object(Matching, "dolphin/gx/GXStubs.c"),
            Object(Matching, "dolphin/gx/GXDisplayList.c"),
            Object(Matching, "dolphin/gx/GXTransform.c"),
            Object(Matching, "dolphin/gx/GXPerf.c"),
        ],
    ),
    DolphinLib(
        "db",
        [
            Object(NonMatching, "dolphin/db/db.c"),
        ],
    ),
    DolphinLib(
        "pad",
        [
            Object(NonMatching, "dolphin/pad/Padclamp.c"),
            Object(NonMatching, "dolphin/pad/Pad.c"),
        ],
    ),
    DolphinLib(
        "mtx",
        [
            Object(Matching, "dolphin/mtx/mtx.c"),
            Object(Matching, "dolphin/mtx/mtxvec.c"),
            Object(Matching, "dolphin/mtx/mtx44.c"),
            Object(Matching, "dolphin/mtx/vec.c"),
        ],
    ),
    DolphinLib(
        "dvd",
        [
            Object(NonMatching, "dolphin/dvd/dvdlow.c"),
            Object(NonMatching, "dolphin/dvd/dvdfs.c"),
            Object(NonMatching, "dolphin/dvd/dvd.c"),
            Object(NonMatching, "dolphin/dvd/dvdqueue.c"),
            Object(NonMatching, "dolphin/dvd/dvderror.c"),
            Object(NonMatching, "dolphin/dvd/fstload.c"),
        ],
    ),
    DolphinLib(
        "ai",
        [
            Object(NonMatching, "dolphin/ai/ai.c"),
        ],
    ),
    DolphinLib(
        "ar",
        [
            Object(NonMatching, "dolphin/ar/ar.c"),
            Object(NonMatching, "dolphin/ar/arq.c"),
        ],
    ),
    DolphinLib(
        "dsp",
        [
            Object(NonMatching, "dolphin/dsp/dsp.c"),
        ],
    ),
    DolphinLib(
        "card",
        [
            Object(NonMatching, "dolphin/card/CARDBios.c"),
            Object(NonMatching, "dolphin/card/CARDRdwr.c"),
            Object(NonMatching, "dolphin/card/CARDBlock.c"),
            Object(NonMatching, "dolphin/card/CARDDir.c"),
            Object(NonMatching, "dolphin/card/CARDCheck.c"),
            Object(NonMatching, "dolphin/card/CARDMount.c"),
            Object(NonMatching, "dolphin/card/CARDFormat.c"),
            Object(NonMatching, "dolphin/card/CARDOpen.c"),
            Object(NonMatching, "dolphin/card/CARDCreate.c"),
            Object(NonMatching, "dolphin/card/CARDRead.c"),
            Object(NonMatching, "dolphin/card/CARDWrite.c"),
            Object(NonMatching, "dolphin/card/CARDDelete.c"),
            Object(NonMatching, "dolphin/card/CARDStatEx.c"),
        ],
    ),
    {
        "lib": "bs2",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_base,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "bs2/scene.c"),
            Object(NonMatching, "bs2/cube.c"),
            Object(Matching, "bs2/menu/game.c"),
            Object(NonMatching, "bs2/menu/calendar.c"),
            Object(NonMatching, "bs2/menu/menu_icon_game.c"),
            Object(NonMatching, "bs2/menu/options.c"),
        ],
    },
    {
        "lib": "jaudio",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_jaudio,
        "progress_category": "jaudio",
        "objects": [
            Object(Matching, "jaudio/dummyprobe.c"),
            Object(NonMatching, "jaudio/ipl_81337740.c"),  # IPL-only, no source yet
            Object(NonMatching, "jaudio/aictrl.c"),
            Object(Matching, "jaudio/sample.c"),
            Object(NonMatching, "jaudio/dummyrom.c"),
            Object(NonMatching, "jaudio/audiothread.c"),
            Object(NonMatching, "jaudio/ipl_813396C0.c"),  # IPL-only, no source yet
            Object(Matching, "jaudio/streamctrl.c"),
            Object(Matching, "jaudio/dspbuf.c"),
            Object(Matching, "jaudio/cpubuf.c"),
            Object(Matching, "jaudio/playercall.c"),
            Object(NonMatching, "jaudio/dvdthread.c"),
            Object(NonMatching, "jaudio/ipl_8133B560.c"),  # IPL-only, no source yet
            Object(NonMatching, "jaudio/audiomesg.c"),
            Object(NonMatching, "jaudio/ipl_8133CB60.c"),  # IPL-only, no source yet
            Object(NonMatching, "jaudio/stackchecker.c"),
            Object(NonMatching, "jaudio/ipl_8133EDA0.c"),  # IPL-only, no source yet
            Object(Matching, "jaudio/memory.c"),
            Object(NonMatching, "jaudio/ipl_8133EF20.c"),  # IPL-only, no source yet
            Object(Matching, "jaudio/dspboot.c"),
            Object(NonMatching, "jaudio/dspproc.c"),
            Object(NonMatching, "jaudio/ipldec.c"),
            Object(Matching, "jaudio/dsp_cardunlock.c"),
            Object(NonMatching, "jaudio/driverinterface.c"),
            Object(Matching, "jaudio/dspdriver.c"),
            Object(Matching, "jaudio/dspinterface.c"),
            Object(Matching, "jaudio/fxinterface.c"),
            Object(Matching, "jaudio/bankread.c"),
            Object(Matching, "jaudio/waveread.c"),
            Object(Matching, "jaudio/connect.c"),
            Object(Matching, "jaudio/bankdrv.c"),
            Object(Matching, "jaudio/random.c"),
            Object(NonMatching, "jaudio/ipl_81352A80.c"),  # IPL-only, no source yet
            Object(Matching, "jaudio/aramcall.c"),
            Object(Matching, "jaudio/ja_calc.c"),
            Object(Matching, "jaudio/fat.c"),
            Object(Matching, "jaudio/cmdstack.c"),
            Object(Matching, "jaudio/virload.c"),
            Object(Matching, "jaudio/heapctrl.c"),
            Object(NonMatching, "jaudio/jammain_2.c"),
            Object(Matching, "jaudio/noteon.c"),
            Object(NonMatching, "jaudio/seqsetup.c"),
            Object(Matching, "jaudio/centcalc.c"),
            Object(Matching, "jaudio/jamosc.c"),
            Object(Matching, "jaudio/oneshot.c"),
            Object(NonMatching, "jaudio/ipl_8135A8E0.c"),  # Sound_* and boot helpers, built like JAudio (32-byte aligned)
        ],
    },
]


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects


# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
    ProgressCategory("jaudio", "JAudio Code"),
]
config.progress_each_module = args.verbose
# Optional extra arguments to `objdiff-cli report generate`
config.progress_report_args = [
    # Marks relocations as mismatching if the target value is different
    # Default is "functionRelocDiffs=none", which is most lenient
    # "--config functionRelocDiffs=data_value",
]

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
    # Treemap, badges and history for the README (progress/)
    from tools import progress_art

    progress_art.main(str(config.build_dir / config.version / "report.json"), "progress")
else:
    sys.exit("Unknown mode: " + args.mode)
