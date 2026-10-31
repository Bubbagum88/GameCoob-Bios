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
| `gInit.c` disc state (`Disc_GetState`, the disc fade) | `bs2/ginit.rs` `Disc` | ported; no drive behind it, so always "no disc" |
| `gInit.c` (DEMO init, frame begin/end, Screen_Dim) | `boot_intro.rs`, `gfx.rs` | behavioral |
| `gMRbuild.c`, `gMRshape.c` (model load) | `ipl/j3d.rs` | close (parses the same J3D1 data; INF1 tree linked as `fn_81303838` does) |
| `gMRdraw.c`, `gMRtexture.c` (joint calc, material and shape draw) | `bs2/gmr.rs` on `sdk::gx` | ported; every model (intro, glass cube, icons, mosaic, memory card blocks) draws through it, with the callers' material swaps (`tevColor`, cleared `texNo`) as instance overrides. Shapes go out in immediate mode instead of display lists |
| `gMRanim.c` | `bs2/anim.rs` | close |
| `glh.c`, `font.c`, `fader.c` | `bs2/glh.rs`, `bs2/font.rs`, `bs2/fader.rs` on `sdk::gx` | ported; `bs2/ui.rs` is now a thin wrapper the menu calls until step 5 moves it onto `glh.rs` directly |
| `lang.c`, `res_archive.c` | `ipl/mod.rs` | close |
| `cube_draw.c` (trail tiles) | `bs2/cube_draw.rs` | ported, on `sdk::gx`; the small cube model is drawn by the caller through `gmr.rs` |
| `cube.c` | `bs2/cube.rs` | ported |
| `scene.c`, `scene_intro.c`, `scene_spinout.c` | `bs2/scene.rs` (`Scene`), `bs2/mod.rs` (`Intro`: the state machine and draws) | ported for the no-disc boot and the spin-out; the disc, SRAM-lost and cover-open states and the dialog texts wait for a drive. GameCoob lingers 60 frames on the logo (`HOLD_FRAMES`) |
| `backdrop.c` | `bs2/backdrop.rs` on `sdk::gx` | ported (GX spot lights on the tile grid, `Backdrop_DrawSpots` for the Memory Card slots); `fn_8130AB14` has no caller yet |
| `pad.c` | `boot_intro.rs` (keyboard to pad) | behavioral |
| `rnd.c` | `bs2/rnd.rs` over `sdk::os::get_tick` | ported; the host tools' tick is a fixed counter so renders repeat, UEFI uses the TSC |
| `menu/menu.c` | `bs2/menu/mod.rs` | update ported: `Work` is `MenuWork` (less the mosaic's fields), made at boot by the scene and wobbled by the spin-out; `Menu::update` runs `Menu_Update`'s steps in its order (rotation, icons, slot glow, guide, sub faders, one-shots, latch, cube or zoom-in). No SRAM-lost flag or fatal-error text yet (dummy disc). Draw side close |
| `menu/menu_fx*.c` | `bs2/fx/` (one module per unit: `mod.rs` = `menu_fx.c`, `shape`, `anim`, `draw`, `spark`, `grid`, `wipe`, `morph`), `ipl/fx.rs` | ported: the cube pool and its five eased values, the three `Fx*_Update` in the C order, the sway, digit flights, word wipes and side columns. Shape and glyph data are read raw, as BS2 reads them. `Fx_Draw` draws both AA passes in one frame. GameCoob drives Options with its two rows (boot timeout in the screen-position row, no sound row) |
| `menu/menu_icon_*.c` | `bs2/menu/icons.rs` | ported: one gMR instance per cube, `*Icon_CalcMtx` run from `Menu_UpdateIcons` (so on last frame's cube matrix), the C's draw order and settled flags. Cubes at alpha 0 are skipped (they draw nothing) |
| `menu/game.c`, `game_params.c` | `bs2/menu/game.rs` | close; START opens the firmware boot list |
| `menu/options.c`, `option_params.c` | `bs2/menu/options.rs` | ported: the three rows and four sub-states, the language faders and `Option_DrawText`. The screen-position row edits the firmware boot timeout (0..30 s, its title says so); the sound mode is kept in RAM until there is audio |
| `menu/calendar.c`, `clock_params.c` | `bs2/menu/mod.rs` | behavioral; sets the firmware clock |
| `menu/memcard*.c` | `bs2/menu/card.rs`, `bs2/menu/backend.rs` | close; fake cards in RAM. `CardBlock_DrawFile`/`DrawEmpty` ported |
| `BS2Mach.c` (boot state machine, DVD) | none | dummy disc for now |
| JAudio, SDK (`dolphin/`), runtime, MSL | none | replaced by `src/sdk/` stand-ins; sound later |

## Order

1. `sdk::mtx`, `mtx.c`, `gBase.c`, `bs2main.c` settings and projection, `camera.c`.
2. `sdk::gx`: GX state and immediate-mode vertex calls feeding the existing
   rasterizer, with the projection as a real `Mtx44`; `GX_Setup2D` and
   `cube_draw.c` as its first users.
2b. `gMRdraw.c` and `gMRtexture.c` on `sdk::gx` (done; the menu's models
   moved over too and `render.rs` keeps only `View` and `Prepared`).
3. `glh.c`, `font.c`, `fader.c` on `sdk::gx`, under `ui.rs` (done).
4. The intro: `cube.c`, `cube_draw.c`, `scene*.c` (done; `backdrop.c` goes with the menu).
5. The menu core, `menu_fx*.c` and the icon units (done).
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
