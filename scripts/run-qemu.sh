#!/usr/bin/env bash
# Build the app and boot it in QEMU with OVMF firmware.
# Usage: [IPL=path/to/ipl.bin] scripts/run-qemu.sh [extra qemu args...]
# Your own dump (IPL, or local/ipl.bin by default) is copied to the virtual ESP
# so the real intro plays.
set -euo pipefail
cd "$(dirname "$0")/.."

OVMF_CODE=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
OVMF_VARS_TEMPLATE=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}

cargo build --release
mkdir -p target/esp/EFI/BOOT target/qemu
cp target/x86_64-unknown-uefi/release/gamecoob.efi target/esp/EFI/BOOT/BOOTX64.EFI
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
  "$@"
