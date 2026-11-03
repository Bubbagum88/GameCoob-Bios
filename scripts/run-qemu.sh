#!/usr/bin/env bash
# Build the app and boot it in QEMU with OVMF firmware.
# Usage: [IPL=path/to/ipl.bin] [BOOT=dir] scripts/run-qemu.sh [extra qemu args...]
# BOOT=<folder with EFI/BOOT/BOOTX64.EFI> (or a GameCoob-boot-files .zip) skips
# the build and boots those files, so no Rust is needed. Without BOOT and
# without cargo it uses the newest GameCoob-boot-files download under local/.
# Your own dump (IPL, or local/ipl.bin by default) is copied to the virtual ESP
# so the real intro plays. Sound goes through an emulated Intel HD Audio card
# to PulseAudio (AUDIO=<backend> picks another); AUDIO=none mutes it, AUDIO=wav,path=out.wav
# records it.
set -euo pipefail
cd "$(dirname "$0")/.."

# Debian/Ubuntu paths by default; Fedora (edk2-ovmf) and Arch (edk2-ovmf) are tried too.
pick() { for f in "$@"; do [ -f "$f" ] && { echo "$f"; return; }; done; echo "$1"; }
OVMF_CODE=${OVMF_CODE:-$(pick /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd \
  /usr/share/edk2/ovmf/OVMF_CODE.fd /usr/share/edk2/x64/OVMF_CODE.4m.fd)}
OVMF_VARS_TEMPLATE=${OVMF_VARS:-$(pick /usr/share/OVMF/OVMF_VARS_4M.fd /usr/share/OVMF/OVMF_VARS.fd \
  /usr/share/edk2/ovmf/OVMF_VARS.fd /usr/share/edk2/x64/OVMF_VARS.4m.fd)}

mkdir -p target/esp/EFI/BOOT target/qemu
if [ -z "${BOOT:-}" ] && ! command -v cargo >/dev/null; then
  BOOT=$(ls -td local/GameCoob-boot-files* 2>/dev/null | head -n1 || true)
  [ -n "$BOOT" ] || { echo "No cargo and no local/GameCoob-boot-files*: set BOOT=<boot files>" >&2; exit 1; }
fi
if [ -n "${BOOT:-}" ]; then
  if [ -f "$BOOT" ]; then  # a downloaded .zip
    rm -rf target/boot-zip && mkdir -p target/boot-zip
    unzip -q -o "$BOOT" -d target/boot-zip
    BOOT=target/boot-zip
  fi
  EFI=$(find "$BOOT" -path '*EFI/BOOT/BOOTX64.EFI' | head -n1)
  [ -n "$EFI" ] || { echo "No EFI/BOOT/BOOTX64.EFI under $BOOT" >&2; exit 1; }
  echo "Boot files: $EFI"
  cp "$EFI" target/esp/EFI/BOOT/BOOTX64.EFI
else
  cargo build --release
  cp target/x86_64-unknown-uefi/release/gamecoob.efi target/esp/EFI/BOOT/BOOTX64.EFI
fi
rm -f target/esp/EFI/GameCoob/ipl.bin
if [ -z "${IPL:-}" ] && [ -f local/ipl.bin ]; then IPL=local/ipl.bin; fi
if [ -n "${IPL:-}" ]; then
  mkdir -p target/esp/EFI/GameCoob
  cp "$IPL" target/esp/EFI/GameCoob/ipl.bin
fi
# Writable copy so the app's variable changes (Timeout, BootNext...) persist between runs.
[ -f target/qemu/OVMF_VARS.fd ] || cp "$OVMF_VARS_TEMPLATE" target/qemu/OVMF_VARS.fd

exec qemu-system-x86_64 \
  -machine q35 -m 512M -smp "$(nproc | awk '{n=$1; if (n>8) n=8; if (n<2) n=2; print n}')" \
  -accel kvm -accel tcg,thread=multi \
  -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
  -drive if=pflash,format=raw,file=target/qemu/OVMF_VARS.fd \
  -drive format=raw,file=fat:rw:target/esp \
  -net none \
  -serial file:target/qemu/serial.log \
  -audiodev "${AUDIO:-pa},id=snd0" -device intel-hda -device hda-output,audiodev=snd0 \
  "$@"
