# Porting the decomp into the app

The app in `src/` was first written from behavior and from the specs in
`decomp/docs/`. Now that the decomp is nearly all matching, the app is being
reshaped to follow it unit by unit: one Rust module per decompiled C unit,
with the same function names in snake case and each function's doc comment
naming the C function and its PAL 1.0 address. Where BS2 calls the Dolphin
SDK, the app calls a PC stand-in under `src/sdk/` with the same shape.

Nothing changes for the user while this happens: every step keeps the intro
and menu rendering the same, checked by rendering frames with
`tools/intro-render` and `tools/menu-render` before and after and diffing
them. Differences are only accepted when they come from following the C code
more closely (float evaluation order, constants), and are noted in the PR.

## Layers

| layer | what it is | where |
|-------|------------|-------|
| `src/sdk/` | PC stand-ins for the Dolphin SDK: `mtx`, `gx` (GX registers kept across frames, immediate-mode vertices, XF lighting and texgen, TEV with konst colors, on the CPU rasterizer); later `vi` (GOP present and frame pacing), `os` (time, calendar, heap), `pad` (keyboard, later HID), `dvd` and `card` (dummies) as their callers are ported | new |
| `src/bs2/` | the BS2 units, one module per C file | being aligned |
| `src/ipl/` | reading the user's `ipl.bin`: descramble, archive, Yay0, J3D, GLH0/STH0/TXH0, textures | stays (BS2 reads the same data from RAM) |
| `src/vars.rs`, `src/firmware.rs` | UEFI variables and firmware actions behind the menu | stays; the PC side of Options, Calendar and Game Play |

## Unit map

Status: **ported** follows the C function for function; **close** follows it
in substance but is shaped differently; **behavioral** was rebuilt from
behavior and docs and still needs a line-by-line pass; **missing** has no
app counterpart yet.

| decomp unit | app module | status |
|-------------|------------|--------|
| `mtx.c` | `bs2/mtx.rs` | ported |
| `gBase.c` (Hermite) | `bs2/gbase.rs` | ported; heap is Rust's allocator |
| `bs2main.c` (DispSettings, projection) | `bs2/bs2main.rs` | ported; the loop itself is still `boot_intro.rs` |
| `camera.c` | `bs2/camera.rs` | ported |
| `gInit.c` `GX_Setup2D` | `bs2/ginit.rs` | ported |
| `gInit.c` (DEMO init, frame begin/end, Screen_Dim, disc state) | `boot_intro.rs`, `gfx.rs` | behavioral |
| `gMRbuild.c`, `gMRshape.c` (model load) | `ipl/j3d.rs` | close (parses the same J3D1 data; INF1 tree linked as `fn_81303838` does) |
| `gMRdraw.c`, `gMRtexture.c` (joint calc, material and shape draw) | `bs2/gmr.rs` on `sdk::gx` | ported for the intro and spin-out; shapes go out in immediate mode instead of display lists. The menu still draws through `bs2/render.rs` |
| `gMRanim.c` | `bs2/anim.rs` | close |
| `glh.c`, `font.c`, `fader.c` | `bs2/ui.rs`, `ipl/glh.rs` | behavioral |
| `lang.c`, `res_archive.c` | `ipl/mod.rs` | close |
| `cube_draw.c` (trail tiles) | `bs2/cube_draw.rs` | ported, on `sdk::gx`; the small cube model is drawn by the caller through `gmr.rs` |
| `cube.c` | `bs2/cube.rs` | close |
| `scene.c`, `scene_intro.c`, `scene_spinout.c` | `bs2/scene.rs`, `bs2/mod.rs` | close |
| `backdrop.c` | `bs2/menu/mod.rs`, `bs2/menu/card.rs` | behavioral |
| `pad.c` | `boot_intro.rs` (keyboard to pad) | behavioral |
| `rnd.c` | none | missing |
| `menu/menu.c` | `bs2/menu/mod.rs` | close |
| `menu/menu_fx*.c` | `bs2/fx.rs`, `ipl/fx.rs` | close |
| `menu/menu_icon_*.c` | `bs2/menu/icons.rs` | close |
| `menu/game.c`, `game_params.c` | `bs2/menu/game.rs` | close; START opens the firmware boot list |
| `menu/options.c`, `option_params.c` | `bs2/menu/mod.rs` | behavioral; edits the boot timeout |
| `menu/calendar.c`, `clock_params.c` | `bs2/menu/mod.rs` | behavioral; sets the firmware clock |
| `menu/memcard*.c` | `bs2/menu/card.rs`, `bs2/menu/backend.rs` | close; fake cards in RAM |
| `BS2Mach.c` (boot state machine, DVD) | none | dummy disc for now |
| JAudio, SDK (`dolphin/`), runtime, MSL | none | replaced by `src/sdk/` stand-ins; sound later |

## Order

1. `sdk::mtx`, `mtx.c`, `gBase.c`, `bs2main.c` settings and projection, `camera.c`.
2. `sdk::gx`: GX state and immediate-mode vertex calls feeding the existing
   rasterizer, with the projection as a real `Mtx44`; `GX_Setup2D` and
   `cube_draw.c` as its first users.
2b. `gMRdraw.c` and `gMRtexture.c` on `sdk::gx` (done for the intro; the
   menu moves over with step 5, then `render.rs` goes).
3. `glh.c`, `font.c`, `fader.c` on `sdk::gx`, replacing `ui.rs`.
4. The intro: `cube.c`, `cube_draw.c`, `backdrop.c`, `scene*.c`.
5. The menu core, `menu_fx*.c` and the icon units.
6. Options, Calendar, Game Play and Memory Card, keeping their UEFI and
   dummy backends at the edges.
7. `gInit.c` and the `BS2Main` loop over `sdk::vi` and `sdk::pad`, replacing
   `boot_intro.rs`.

## Not ported on purpose

- No Nintendo data: models, textures, layouts and paths are still read from
  the user's dump at boot. Tables of numbers that BS2 keeps in its own code
  or data (defaults, timings) are ported as constants, as before.
- Float math follows the C expression order, but not the PowerPC's fused
  multiply-adds, so results can differ in the last bit.
