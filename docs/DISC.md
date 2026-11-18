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

## Next

- Disc images: "Insert in drive" for `.iso`/`.gcm` files in the Card
  Files explorer, like card images in the slots. A GameCube image would
  show its banner and launch; a PC ISO would boot through the firmware's
  RAM disk.
- The banner's language from the menu language (BNR2 has six).
