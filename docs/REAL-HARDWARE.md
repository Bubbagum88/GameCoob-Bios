# Testing on a PC without UEFI

GameCoob is a UEFI app. Old laptops like the Dell Inspiron N5050 only have a legacy
BIOS, so they can't start it directly. Two ways around that, both run from the repo
on that PC:

1. **QEMU with your real pads** (`scripts/run-qemu-pads.sh`): GameCoob runs in a VM
   on the PC's screen, and the USB controllers plugged into the PC are handed to the VM.
2. **Clover USB stick** (`scripts/make-clover-usb.sh`): the stick carries CloverEFI, a
   UEFI firmware that starts on top of the old BIOS, and Clover then starts GameCoob.
   This is the closest to a real boot: real screen, real sound chip, real USB ports.
   Nothing on the internal disk changes.

The commands below are one per line (the box's terminal mangles multi-line pastes).
`ssh -t isaac@192.168.1.222` opens a shell on the box; run the rest there, from
`~/GameCoob-Bios`. The boot files zip and `local/ipl.bin` get there as in the
Linux test guide (`scp local\GameCoob-boot-files.zip local\ipl.bin isaac@192.168.1.222:GameCoob-Bios/local/`).

## Once: packages and update

    sudo pacman -S --needed git unzip qemu-desktop edk2-ovmf mtools dosfstools
    cd ~/GameCoob-Bios && git pull

Check that the CPU's virtualization is usable (QEMU is slow without it):

    ls /dev/kvm

If that says "No such file", reboot into the BIOS setup (F2 on Dell), turn on
Virtualization if it is listed, then `sudo modprobe kvm_intel`.

## 1. QEMU with real controllers

Plug the pads in (Wii U GameCube adapter, Xbox 360/One wired, or any USB HID pad), then:

    scripts/run-qemu-pads.sh --list

If it says "you can open it: no", allow your user to open them (once per pad model):

    sudo scripts/run-qemu-pads.sh --udev

Then unplug and replug the pads (or reboot) and start it on the box's own screen
(someone logged in to its desktop):

    DISPLAY=:0 scripts/run-qemu-pads.sh

Wayland desktop: `WAYLAND_DISPLAY=wayland-0 XDG_RUNTIME_DIR=/run/user/1000 scripts/run-qemu-pads.sh`.
Watching from Windows over VNC instead works like in the Linux guide:

    ssh -L 5900:localhost:5900 isaac@192.168.1.222 "cd ~/GameCoob-Bios && AUDIO=none scripts/run-qemu-pads.sh -display none -vnc localhost:0"

While QEMU runs, the pads belong to the VM and Linux doesn't see them. They come back
when QEMU quits. A pad the script doesn't detect can be named directly:
`PADS="057e:0337" scripts/run-qemu-pads.sh` (IDs from `lsusb`).
The keyboard stays with QEMU's window as usual. Logs: `target/qemu/serial.log`.

## 2. Clover USB stick

Download the newest Clover release into `local/`:

    curl -sL https://api.github.com/repos/CloverHackyColor/CloverBootloader/releases/latest | grep -o 'https://[^"]*CloverV2-[0-9]*\.zip' | head -n1 | xargs -r curl -L --output-dir local -O

Build the stick image (no sudo; uses the newest `local/GameCoob-boot-files*` and `local/ipl.bin`):

    scripts/make-clover-usb.sh

Optional: try the image first in QEMU's legacy BIOS (SeaBIOS), which works like the laptop's:

    DISPLAY=:0 scripts/make-clover-usb.sh --qemu

Write it to a stick. `lsblk` shows the stick's name; the script refuses anything that
isn't a removable/USB disk, and asks before erasing:

    lsblk
    sudo scripts/make-clover-usb.sh --write /dev/sdX

Boot: power on with the stick in, press F12 on the Dell logo, pick the USB stick.
Clover shows a text menu for 3 seconds and starts GameCoob (`TIMEOUT=10` when
building gives more time; the arrow keys and Enter pick another entry).

What to expect on CloverEFI:

- Picture comes from the graphics card's VESA BIOS, so the resolution is whatever the
  VBIOS offers (often 1024x768 on the N5050's panel).
- Sound uses the real HD Audio chip, as on a UEFI PC.
- Settings that change boot entries (BootNext, Timeout, firmware setup) land in
  Clover's emulated NVRAM, not the laptop's BIOS. Harmless, but they don't do anything.
- USB pads depend on CloverEFI's USB drivers; the keyboard works through the BIOS either way.
  If pads do nothing there, use the keyboard and note it.
- If the stick doesn't boot at all, try `BOOTFILE=boot6 scripts/make-clover-usb.sh` and write it again.

The image (`target/clover-usb.img`) holds your IPL dump. Don't share it.
