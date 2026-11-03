#!/usr/bin/env bash
# Install GameCoob on this Linux PC's EFI system partition and boot it once.
# Usage (from the repo root):
#   sudo scripts/install-esp.sh [boot-files.zip|folder]   install/update, then boot GameCoob once on reboot
#   sudo scripts/install-esp.sh --remove                  delete the entry and the files
# Default boot files: newest local/GameCoob-boot-files*. Your dump comes from local/ipl.bin
# (or IPL=path). Your OS boot entry stays first in BootOrder; GameCoob only runs when
# BootNext points at it, so a failed boot just falls back to the normal one.
# Secure Boot must be off: the app is unsigned.
set -euo pipefail
cd "$(dirname "$0")/.."
[ "$(id -u)" = 0 ] || { echo "Run with sudo" >&2; exit 1; }
[ -d /sys/firmware/efi ] || { echo "This PC did not boot in UEFI mode" >&2; exit 1; }
command -v efibootmgr >/dev/null || { echo "Install efibootmgr first (pacman -S efibootmgr / apt install efibootmgr)" >&2; exit 1; }

LABEL=GameCoob
ESP=${ESP:-$(bootctl -p 2>/dev/null || true)}
for d in /boot/efi /efi /boot; do
  [ -n "$ESP" ] && break
  [ -d "$d/EFI" ] && [ "$(findmnt -no FSTYPE "$d" 2>/dev/null)" = vfat ] && ESP=$d
done
[ -n "$ESP" ] || { echo "No EFI system partition found; set ESP=/path" >&2; exit 1; }
DIR="$ESP/EFI/GameCoob"
entry() { efibootmgr | sed -n "s/^Boot\([0-9A-Fa-f]\{4\}\)\*\{0,1\} $LABEL\b.*/\1/p" | head -n1; }

if [ "${1:-}" = --remove ]; then
  n=$(entry); [ -n "$n" ] && efibootmgr -q -b "$n" -B && echo "Removed Boot$n"
  rm -rf "$DIR" && echo "Removed $DIR"
  exit 0
fi

SB=$(od -An -tu1 -j4 -N1 /sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c 2>/dev/null | tr -d ' ' || true)
[ "$SB" = 1 ] && { echo "Secure Boot is on: turn it off in firmware setup first" >&2; exit 1; }

SRC=${1:-$(ls -td local/GameCoob-boot-files* 2>/dev/null | head -n1 || true)}
[ -n "$SRC" ] || { echo "No boot files: pass the zip or put it in local/" >&2; exit 1; }
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
if [ -f "$SRC" ]; then unzip -q "$SRC" -d "$TMP"; SRC=$TMP; fi
EFI=$(find "$SRC" -path '*EFI/BOOT/BOOTX64.EFI' | head -n1)
[ -n "$EFI" ] || { echo "No EFI/BOOT/BOOTX64.EFI in $SRC" >&2; exit 1; }
IPL=${IPL:-local/ipl.bin}
[ -f "$IPL" ] || { echo "No dump at $IPL (GameCoob needs it)" >&2; exit 1; }

mkdir -p "$DIR"
cp "$EFI" "$DIR/gamecoob.efi"
cp "$IPL" "$DIR/ipl.bin"
sync
echo "Copied app + dump to $DIR"

n=$(entry)
if [ -z "$n" ]; then
  DEV=$(findmnt -no SOURCE "$ESP")
  DISK=/dev/$(lsblk -no PKNAME "$DEV")
  PART=$(cat "/sys/class/block/$(basename "$DEV")/partition")
  OLD=$(efibootmgr | sed -n 's/^BootOrder: //p')
  efibootmgr -q -c -d "$DISK" -p "$PART" -L "$LABEL" -l '\EFI\GameCoob\gamecoob.efi'
  [ -n "$OLD" ] && efibootmgr -q -o "$OLD"   # keep the OS first; GameCoob runs via BootNext
  n=$(entry)
  echo "Created Boot$n on $DISK partition $PART"
fi
efibootmgr -q -n "$n"
echo "GameCoob (Boot$n) boots once on the next restart. Reboot now with: sudo reboot"
