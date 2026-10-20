#!/usr/bin/env bash
# Build the app and boot it in QEMU with OVMF firmware.
# Usage: scripts/run-qemu.sh [extra qemu args...]
set -euo pipefail
cd "$(dirname "$0")/.."

OVMF_CODE=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
OVMF_VARS_TEMPLATE=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}

cargo build --release
mkdir -p target/esp/EFI/BOOT target/qemu
cp target/x86_64-unknown-uefi/release/gamecoob.efi target/esp/EFI/BOOT/BOOTX64.EFI
# Writable copy so the app's variable changes (Timeout, BootNext...) persist between runs.
[ -f target/qemu/OVMF_VARS.fd ] || cp "$OVMF_VARS_TEMPLATE" target/qemu/OVMF_VARS.fd

exec qemu-system-x86_64 \
  -machine q35 -m 512M \
  -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
  -drive if=pflash,format=raw,file=target/qemu/OVMF_VARS.fd \
  -drive format=raw,file=fat:rw:target/esp \
  -net none \
  "$@"
