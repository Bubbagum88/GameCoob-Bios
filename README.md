# GameCoob-Bios

A UEFI boot app that makes any x86-64 PC start up like a GameCube: a real-time
cube intro, then a console-style menu where you pick what to boot and change
firmware boot settings.

It runs after your motherboard's firmware and before any operating system, so
nothing gets flashed and nothing can be bricked. If something goes wrong, your
normal boot menu is still there.

| Intro | Menu |
| --- | --- |
| ![Cube rolling](docs/screenshots/intro-1-roll.png) | ![Main menu](docs/screenshots/menu-boot.png) |
| ![Logo](docs/screenshots/intro-3-logo.png) | ![Settings](docs/screenshots/settings-timeout-changed.png) |

## What works today

- **The real GameCube intro, from your own console.** Put the `ipl.bin` you
  dumped from your GameCube at `\EFI\GameCoob\ipl.bin` on the boot
  partition and GameCoob plays the console's own boot animation: the cube
  drop, the rolling "G" with its fading trail, the flip and slam, the shake,
  the logo and the animated NINTENDO GAMECUBE wordmark, then the spin into
  the glass cube. The choreography was reimplemented from a decompilation of
  the IPL's BS2 program; the models and textures are read from your dump at
  boot. Hold **Enter** (the A button) while the cube rolls to spin it, as on
  the console. Esc skips.
- **Original intro** (no dump). A small cube tumbles around a grid, leaving tiles that trace a
  "C", hops into the middle, and the tiles rise into the logo. Drawn by our
  own software rasterizer, since there is no GPU driver before the OS loads.
  Press any key to skip. Press **Z** for the hidden alternate palette.
- **Boot.** Lists your firmware's boot entries and boots the one you pick (via
  the standard `BootNext` variable).
- **Settings.** Change the firmware boot menu timeout (the `Timeout` variable).
- **Firmware Setup.** Reboots straight into your vendor's BIOS setup screen
  (`OsIndications`), for settings this app can't reach yet.
- **Restart** and **Power Off**.

The menu shows a big cube that turns to a new colored face for each option.

## Assets and Nintendo content

This repository contains only original code and art. Nintendo's code,
models, textures, fonts and sounds are **never** committed here. GameCoob
reads them at boot from an `ipl.bin` dump of your own console, if you
provide one; see [docs/ROADMAP.md](docs/ROADMAP.md).

## Build and run

You need Rust (stable) with the UEFI target, plus QEMU and OVMF to test.

```sh
rustup target add x86_64-unknown-uefi
sudo apt install qemu-system-x86 ovmf     # Debian/Ubuntu

cargo build --release                     # -> target/x86_64-unknown-uefi/release/gamecoob.efi
scripts/run-qemu.sh                       # boots it in a QEMU window
IPL=~/ipl.bin scripts/run-qemu.sh         # same, with your dump for the real intro
scripts/screenshots.py out/               # headless run that saves PNGs (needs Pillow)
```

In QEMU: arrow keys move, Enter selects, Esc goes back.

### On real hardware

1. Copy `gamecoob.efi` to a FAT32 USB stick as `EFI/BOOT/BOOTX64.EFI`.
2. Boot from the stick (Secure Boot must be off for now; the app is unsigned).
3. To make it your everyday boot screen, add it as the first boot entry, and
   turn on your firmware's quiet/fast boot to hide the vendor logo.

## Layout

```
src/
  main.rs    entry point: graphics setup, intro, menu
  boot_intro.rs  loads ipl.bin from the ESP and runs the real intro on all CPU cores
  bs2/       the intro rebuilt from BS2: cube and scene state machines, GX-style
             TEV renderer and banded rasterizer, J3D animations
  ipl/       ipl.bin parsing: descrambling, resource archive, Yay0, J3D, textures
  intro.rs   the original cube intro choreography and palettes
  menu.rs    main menu, boot list, settings, confirm dialogs
  gfx.rs     software renderer: canvas, triangle rasterizer, camera, font
  math.rs    vectors, rotation matrices, easing
  vars.rs    UEFI variables: Boot####, BootOrder, BootNext, Timeout, OsIndications
  clock.rs   TSC-based timer for frame-rate-independent animation
scripts/     QEMU runner and screenshot automation
tools/       host-side checks: ipl-preview (models to PNG), intro-render (intro frames to PNG)
docs/        roadmap and screenshots
```

## Roadmap

See [docs/ROADMAP.md](docs/ROADMAP.md) for the full plan: real BIOS options
through HII, controller input, intro audio, optional assets from your own IPL
dump, and the long-term firmware-level version.
