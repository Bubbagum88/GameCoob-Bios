# GameCube IPL (BS2) decompilation, PAL 1.0

Our own reverse-engineered, non-matching C for the GameCube boot ROM's BS2
program: the intro (cube, scene director) and the main menu (cube menu,
Game Play, Memory Card, Options, Calendar). It is kept apart from the
GameCoob app in the rest of this repository; the app is original Rust written
from the specs in `docs/`.

- `src/bs2/`, `include/bs2/`: decompiled C, one comment per function with its address
- `docs/intro.md`, `docs/menu.md`: behavior specs (timings, state machines, layouts)
- `config/PAL10/`: decomp-toolkit config, symbols and splits
- `tools/extract_bs2.py`: descrambles your own `ipl.bin` into `bs2.dol`

No Nintendo binaries are included. To build, dump `ipl.bin` from your own
console, put it at `orig/PAL10/ipl.bin` (git ignores it), and follow
`docs/getting_started.md` (dtk-template layout).

Known issue: `dtk dol split` currently stops on an overlap between `__files`
and `lbl_814ACC9C` in `.data`.
