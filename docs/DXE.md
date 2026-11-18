# Putting GameCoob inside the firmware (DXE): feasibility

Roadmap phase 6 asks whether GameCoob could live in the motherboard's
firmware instead of on a disk, so the intro starts the moment the PC powers
on and the firmware's own setup screen draws through GameCoob's menu. This
page is the research behind that question: how firmware loads code, what
stops a modified firmware from running, and which routes are worth taking.

Short answer: **possible on some boards, never as a one-click install, and
most of the benefit is reachable without touching the flash chip.** The
recommended order is in "Recommendation" at the end.

## How a UEFI PC starts

1. **SEC / PEI**: the CPU starts from the SPI flash chip, sets up memory.
   Nothing on screen yet.
2. **DXE**: the DXE dispatcher loads every driver in the firmware volumes
   (FFS files in the flash image) whose dependencies are met. Graphics
   (GOP), USB, storage, the HII database and the setup forms all come
   from DXE drivers.
3. **BDS** (boot device selection): connects consoles (this is when the
   graphics card and USB keyboards actually start), shows the vendor logo,
   runs the setup screen if F2/Del was pressed, then starts `Boot####`
   entries. GameCoob today is one of those entries (`BOOTX64.EFI`).

A DXE driver of ours would run in step 2. The screen and the keyboard are
not ready until step 3, so even a DXE driver can't draw much earlier than
the vendor logo does. **The real gains of being in the firmware are
replacing the vendor logo and owning the setup screen, not a faster
start.** On a fast-boot board, BDS reaches `Boot####` within a second or two
of the logo anyway.

## What stops a modified firmware

| Protection | What it does | Effect on injection |
| --- | --- | --- |
| Intel Boot Guard (Haswell, 2013, and later; most OEM laptops and many desktops) | The CPU checks the first firmware stage against a key fused into the chipset by the OEM; that stage then checks the rest | Verified mode: a changed DXE volume fails the check and the board won't boot (or recovers). Can't be turned off: the fuses are one-time |
| AMD Platform Secure Boot (Zen, most OEM systems) | The PSP checks the firmware against the OEM's key and fuses the CPU to that vendor | Same as Boot Guard where it's on. DIY desktop boards usually ship with it off |
| BIOS write protection (BIOSWE/BLE, SMM_BWP, Protected Range Registers, Intel BIOS Guard) | Stops software from writing the flash chip | Software flashing fails; an external SPI programmer (CH341A clip) still works |
| Signed capsule updates | Vendor updaters only take signed images | The normal update path can't carry a modified image |
| UEFI Secure Boot | Checks every driver and app loaded from outside the flash | Applies to GameCoob today as much as to any driver: sign it or turn Secure Boot off |

Tools for the image itself are mature: UEFITool (old engine, 0.28) can
insert an FFS file into a DXE volume and rebuild the image; EDK II builds
the FFS; flashrom with a CH341A writes and, first, backs up the chip.

## The routes

### 1. Stay a boot app (today)

No risk, works on any UEFI PC. The vendor logo shows first, then GameCoob.
HII settings (this phase's work) already reach the firmware's own options
from here, through the same routing protocol the setup screen uses.

### 2. `Driver####`: a driver loaded from disk, before the boot menu

The UEFI spec lets the firmware load drivers from disk early in BDS:
`Driver####` variables listed in `DriverOrder`, read before
`Boot####` entries run. EDK II loads them right after its platform
"before console" step, before consoles connect and before the logo.

- No flashing; undone by deleting two variables. GameCoob can write them
  itself, like it writes `BootNext` now.
- Support varies: EDK II (OVMF, most coreboot UEFI payloads) honours them;
  AMI and Insyde boards often do, some ignore them. Testable in QEMU today.
- Secure Boot rules are the same as for the app.
- What it can do: install a platform logo protocol before the vendor's is
  used (where the firmware looks one up rather than linking it in), or
  register for the console's GOP and start the intro as soon as the screen
  exists, then hand over to the normal boot. It cannot change what ran
  before it.

### 3. Ship GameCoob as an FFS file with a firmware boot entry

The least intrusive firmware change: put the existing app (unchanged) into
the flash image as an application FFS file, the way EDK II ships the UEFI
Shell and its setup app (`UiApp`), and point a boot entry at it with a
firmware-volume device path (`Fv(..)/FvFile(<GameCoob GUID>)`). GameCoob
then boots with no disk at all and survives a wiped drive.

- Needs the same flash access and runs into the same Boot Guard / PSB wall
  as any modification.
- No new code model: the app stays an app.

### 4. A real DXE driver, replacing the logo and the setup screen

The full phase 6: a boot-service driver that

- installs before the vendor's logo protocol is used, so the intro replaces
  the logo, and
- takes the place of the firmware's `EFI_FORM_BROWSER2_PROTOCOL`, so the
  vendor setup screen's forms render in GameCoob's UI.

This needs a different program structure: a DXE driver's entry point must
return, and its work then runs from events (protocol notifications,
ReadyToBoot). Long-running drawing from an event callback blocks the
firmware's own timers. The browser swap means re-implementing what the
vendor browser does beyond drawing: driver callbacks (`ConfigAccess.Callback`),
`inconsistentif` / `nosubmitif` checks, defaults, and password forms. The
HII reader in `src/hii` is the start of that, but today it deliberately
skips callback questions.

Rust: the same crate builds as a driver by linking with
`/SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER`; the `uefi` crate works in drivers.

### 5. Boards with open firmware (coreboot)

On boards coreboot supports (many Chromebooks with MrChromebox firmware,
some ThinkPads, some desktops), Boot Guard is off or absent and the UEFI
part is an EDK II payload built from source. GameCoob can be added to that
build as route 3 or 4 without hand-editing an image, and flashed with
flashrom. This is the cleanest place to try phase 6 on real hardware.

## Notes for the boards in this project

- **Dell Inspiron N5050** (the Arch test box): legacy BIOS only, no UEFI
  boot, so neither the app nor any DXE route applies. It also isn't a
  coreboot board. GameCoob can reach it only through a UEFI loader on USB
  (Clover-style) or a newer UEFI PC.
- **QEMU + OVMF**: everything above can be tried here first with no risk:
  `Driver####` (route 2) on stock OVMF, and routes 3 and 4 by building
  OVMF from EDK II with GameCoob's FFS in its FDF.

## Recommendation

1. Keep the boot app as the product.
2. Next, try **route 2** (`Driver####`) in OVMF: a small driver that
   starts the intro as soon as GOP is up and registers GameCoob's logo.
   It needs no flashing and GameCoob can switch it on and off itself.
3. For phase 6 proper, build **route 3** into an OVMF image first, then on
   a coreboot board (route 5). Only then look at route 4's browser swap.
4. Don't build a tool that patches retail OEM firmware: with Boot Guard or
   PSB on, it bricks the board; without, it still needs an external
   programmer and a verified backup, and each vendor image differs.

## Sources

- UEFI Specification 2.10: §3.1.3 (`Driver####`, `DriverOrder`), §33
  (HII), §35 (forms browser and config routing)
- PI Specification vol. 2 (DXE dispatcher), vol. 3 (firmware volumes, FFS)
- EDK II `MdeModulePkg/Universal/BdsDxe/BdsEntry.c` (load order of driver
  options), `MdeModulePkg/Application/UiApp`, `OvmfPkg/OvmfPkgX64.fdf`
- Intel Boot Guard and AMD PSB: vendor platform security whitepapers;
  coreboot documentation on Intel Boot Guard
- UEFITool (LongSoft), flashrom, MrChromebox coreboot firmware docs
