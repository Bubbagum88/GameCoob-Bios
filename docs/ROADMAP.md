# Roadmap

The goal: power on any x86-64 PC and have it feel like a GameCube booting, with
a menu that can boot an OS and change real BIOS settings.

## Approach

GameCoob is a **UEFI application**, not replacement firmware. The motherboard's
firmware initializes the hardware, then hands off to us before any OS loads.
From there we have the framebuffer (GOP), keyboard and controller input, disk
access, and the firmware's settings data. Nothing is flashed, so a bug can't
brick a board, and the whole thing is developed and tested in QEMU with OVMF.

Everything built this way (renderer, menus, settings parsing) carries over to
the optional firmware-level version in phase 6.

Language: Rust with the [`uefi`](https://crates.io/crates/uefi) crate, `no_std`.

## Phase 1: Prototype (done)

- [x] Rust UEFI app that builds to `BOOTX64.EFI` and boots in QEMU/OVMF
- [x] Software renderer: backbuffer, depth-buffered flat-shaded triangles,
      pinhole camera, 8x8 bitmap font, fades, one GOP blit per frame
- [x] Frame-rate-independent animation (TSC timer calibrated against `Stall`)
- [x] Original cube intro, skippable, with a "press Z" alternate palette
- [x] Menu: Boot, Settings, Firmware Setup, Restart, Power Off
- [x] Boot any firmware boot entry via `BootNext`
- [x] Edit the firmware boot `Timeout`
- [x] Reboot into vendor setup via `OsIndications` (when the firmware supports it)
- [x] Scripted headless QEMU run that captures screenshots

## Phase 2: A menu worth living in

- [ ] Reorder `BootOrder`, enable/disable entries, and pick a default entry
- [ ] Our own auto-boot countdown (boot the default entry after N seconds
      unless a key is pressed), configurable from the menu
- [ ] Settings page for GameCoob itself, stored in our own NVRAM variable
      under a GameCoob vendor GUID: intro on/off, palette, countdown, resolution
- [ ] Pick the best GOP mode (native resolution) and scale the UI with it
- [ ] Smoother text: a proportional font of our own design, anti-aliased
- [ ] System info page: CPU, memory, firmware version, Secure Boot state
- [ ] Calendar/clock page in the spirit of the console's calendar screen

## Phase 3: Look and feel closer to the console

- [ ] Gouraud shading and per-vertex color so cubes glow like the original
- [x] Optional assets from the user's own `ipl.bin` (see "Assets" below):
      models, textures and animations loaded at runtime from the ESP
- [x] Reimplement the intro by decompiling BS2: the cube path, timings,
      camera, shake, wordmark animation and spin-out follow the IPL's code
- [ ] Port the main menu (the glass cube with Game Play, Memory Card,
      Calendar and Options) from the decompiled BS2 menu code
- [ ] Boot jingle and spin whoosh from the IPL's own audio
- [ ] More easter eggs in spirit: held buttons that change the intro, a
      secret sound or palette on certain dates, a hidden "cube" menu toy

## Phase 4: Input and sound

- [ ] USB game controller input: UEFI exposes HID devices on some firmware;
      fall back to keyboard everywhere else
- [ ] Mouse/pointer support through the Simple Pointer protocol
- [ ] Intro audio through Intel HDA, following OpenCore's AudioDxe approach.
      Ship only our own jingle; play the original only from the user's dump

## Phase 5: Real BIOS settings (HII)

UEFI firmware describes its setup menus as HII forms: every option, its
allowed values, and the variable it lives in (usually `Setup`).

- [ ] Enumerate HII packages through `EFI_HII_DATABASE_PROTOCOL`
- [ ] Parse IFR form opcodes (forms, one-of, checkbox, numeric, string, refs)
- [ ] Read and write values through `EFI_HII_CONFIG_ROUTING_PROTOCOL`, the same
      path the vendor setup screen uses, never raw offsets into `Setup`
- [ ] Render forms in our UI, grouped and searchable
- [ ] Safety: read-only by default, show a diff before saving, keep a backup
      of every variable we change, and a "restore all" option
- [ ] Test on OVMF first, then on real boards (vendor forms vary a lot)

## Phase 6 (optional): Inside the firmware

For an intro that starts the instant you press power, GameCoob would become a
DXE driver inside the firmware image, possibly replacing the Form Browser so
every vendor setting renders through our UI natively.

- Needs a supported board, an SPI programmer (CH341A) and a backup of the chip
- Intel Boot Guard / AMD PSP enforcement blocks modified firmware on many boards
- Tooling: UEFITool to repack, EDK2 to build the driver

This is a stretch goal and stays out of scope until phases 2 to 5 are solid.

## Assets

This repository only ever contains original code and art. Nintendo's assets
are used only by the person who owns the console, from their own dump:

1. Dump `ipl.bin` from your own GameCube with Swiss.
2. Copy it to the boot partition as `\EFI\GameCoob\ipl.bin`. GameCoob
   descrambles it at startup and reads the models, textures, animations and
   the cube's path straight from it (`src/ipl`). Without it, the original
   GameCoob intro plays instead.
3. For inspecting a dump, the extractor (currently at `tools/ipl-extract` in
   the project's shared files) writes textures, models, fonts, audio and
   strings to a folder.

Extracted output must never be committed. The extractor will move into this
repo under `tools/` once it settles; its output paths are already gitignored
by convention (`ipl-extracted/`, `*.ipl.bin`).

## Testing

- Every change runs in QEMU + OVMF; `scripts/screenshots.py` drives the menu
  over QMP and saves PNGs, so visual changes can be reviewed in PRs
- Later: CI that builds the `.efi` and runs the screenshot script headless
- Real hardware checks before each release, starting with a USB stick boot
