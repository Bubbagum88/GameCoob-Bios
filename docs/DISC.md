# The disc drive

Game Play follows the PC's optical drive the way the console's menu
follows its DVD drive: "Please insert a disc" with the tray empty,
"reading" while a disc spins up, then the disc's banner and "Press START",
or "The disc could not be read".

Code: `src/drive.rs` (the drive, over the firmware's BlockIO) and
`src/launch/disc.rs` (what is on a disc, shared with the helper and
tested on the host by `tools/gamecoob-launch`).

## What START does

| Disc | Banner | START |
|---|---|---|
| GameCube (magic at 0x1C) | its own `opening.bnr`: picture and English texts | launches the game with the same game code from the emulator game list (`library.toml`, [EMULATORS.md](EMULATORS.md)) |
| PC disc with a UEFI El Torito image (OS installers, live discs) | a drawn disc, the ISO 9660 volume label and publisher | boots it: BootNext to the firmware's own entry for the drive, or to a `GameCoob disc` entry made for it, then a reset |
| PC disc without a UEFI image, Wii disc, audio CD, DVD video... | none | nothing: "The disc could not be read" |

Most PC drives can't read GameCube discs at all (they are not standard
DVDs). Those discs end up as "could not be read" after three tries, as a
scratched disc would on the console. Burned copies and drives that can
read them get the banner.

## How the drive is watched

- At the menu's start the first BlockIO device that is removable, not a
  partition and uses 2048-byte sectors is the drive. Without one the
  stand-in sample disc stays (D opens and closes its tray), so a VM or a
  PC without a drive still shows the screen.
- Once a second a zero-length read asks the firmware whether a disc is in
  (SCSI TEST UNIT READY on EDK2-style firmware); it doesn't spin the disc.
- A new disc shows "reading" first, then is read once. Those reads block
  the menu for as long as the drive takes (spin-up included).
- No eject: UEFI has no standard way. Use the drive's button.

## Testing in QEMU

```
qemu ... -drive if=none,id=cd0,media=cdrom -device ide-cd,drive=cd0,bus=ide.1,id=cdrom0
```

then, over QMP, `blockdev-change-medium id=cdrom0 filename=disc.iso
format=raw` puts a disc in and `eject id=cdrom0` takes it out. A UEFI
bootable test ISO: `xorriso -as mkisofs -V LABEL -e efi.img -no-emul-boot
-o pc.iso dir/` with `efi.img` a FAT image holding
`EFI/BOOT/BOOTX64.EFI`.

## Disc images

Card Files (Memory ▸ Files) lists plain disc images (`.iso`, `.gcm`) on
any FAT volume the firmware can read, with what each one is on the info
line. "Put in disc drive" puts one in: Game Play shows it in place of
the drive's disc, the same way as a real disc. It comes out with "Take
out of disc drive" on the "Disc drive:" row of the volume list, or the
tray key (D). Compressed formats (`.rvz`, `.ciso`, `.gcz`) can't be read
in place and aren't listed.

- A GameCube image shows its banner; START launches the game with the
  same game code from the emulator game list, as for a disc.
- A PC image with a UEFI boot image is read whole into memory when
  START is pressed and handed to the firmware's RAM disk driver as a
  virtual CD (EFI_RAM_DISK_PROTOCOL, as HTTP boot does), then its
  `\EFI\BOOT\BOOTX64.EFI` starts at once: a reset would lose the RAM
  disk. The menu waits while the file is read. The PC needs RAM for the
  whole image, and firmware without a RAM disk driver says so. Whether
  the OS finds its files after its boot loader depends on the OS: the
  kernel has to find the RAM disk itself (the firmware lists it in
  ACPI's NFIT table).

Code: `src/bs2/menu/explorer.rs` (the rows and popups),
`backend::ImageDrive` (an image on top of the real drive) and
`src/ramdisk.rs`.

## Next

- The banner's language from the menu language (BNR2 has six).
- Compressed images (`.rvz`, `.ciso`) for the banner.
