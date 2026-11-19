#!/usr/bin/env bash
# Build a USB stick image that boots GameCoob on a legacy-BIOS-only PC.
#
# Clover's legacy boot sectors load CloverEFI (a UEFI firmware that runs on
# top of the old BIOS); Clover's menu then starts GameCoob after a short
# countdown. Nothing on the PC's own disk is touched.
#
# Needs: a Clover release zip (CloverV2-*.zip from
# https://github.com/CloverHackyColor/CloverBootloader/releases), mtools,
# dosfstools, util-linux (sfdisk), unzip.
#
# Usage (from the repo root; no root needed until the last step):
#   CLOVER=CloverV2-5161.zip scripts/make-clover-usb.sh             build target/clover-usb.img
#   scripts/make-clover-usb.sh --qemu                               boot that image in QEMU's legacy BIOS (SeaBIOS)
#   sudo scripts/make-clover-usb.sh --write /dev/sdX                write the image to a USB stick (wipes it)
#
# Env: CLOVER=<zip or unpacked folder> (default: newest CloverV2*.zip in local/ or here),
#      BOOT=<GameCoob-boot-files zip or folder> (default: newest local/GameCoob-boot-files*,
#      or a fresh cargo build), IPL=<dump> (default local/ipl.bin),
#      BOOTFILE=boot7|boot6 (CloverEFI flavour; boot7 reads disks through the BIOS, which
#      is what you want for USB sticks), TIMEOUT=<seconds in Clover's menu> (default 3),
#      SIZE=<image size in MiB> (default 256).
# The image holds your IPL dump: keep it to yourself.
set -euo pipefail
cd "$(dirname "$0")/.."
IMG=target/clover-usb.img
WORK=target/clover-work
LABEL=GAMECOOB

if [ "${1:-}" = --qemu ]; then
  [ -f "$IMG" ] || { echo "Build $IMG first" >&2; exit 1; }
  accel=(-accel tcg); [ -w /dev/kvm ] && accel=(-accel kvm)
  mkdir -p target/qemu
  # No pflash = QEMU's own legacy BIOS (SeaBIOS), like the laptop.
  # snapshot=on: Clover's writes (nvram.plist, logs) are thrown away on exit.
  exec qemu-system-x86_64 -machine pc -m 1G "${accel[@]}" \
    -drive file="$IMG",format=raw,snapshot=on \
    -net none -serial file:target/qemu/serial-legacy.log \
    -audiodev "${AUDIO:-pa},id=snd0" -device intel-hda -device hda-output,audiodev=snd0 \
    -device qemu-xhci,id=xhci "${@:2}"
fi

if [ "${1:-}" = --write ]; then
  DEV=${2:-}
  [ "$(id -u)" = 0 ] || { echo "Run with sudo" >&2; exit 1; }
  [ -f "$IMG" ] || { echo "Build $IMG first (without sudo)" >&2; exit 1; }
  [ -b "$DEV" ] || { echo "Usage: sudo $0 --write /dev/sdX  (lsblk shows the stick)" >&2; exit 1; }
  [ "$(lsblk -dno TYPE "$DEV")" = disk ] || { echo "$DEV is not a whole disk (use /dev/sdX, not /dev/sdX1)" >&2; exit 1; }
  rm=$(lsblk -dno RM "$DEV" | tr -d ' '); tran=$(lsblk -dno TRAN "$DEV" | tr -d ' ')
  [ "$rm" = 1 ] || [ "$tran" = usb ] || { echo "$DEV is not removable/USB; refusing" >&2; exit 1; }
  if lsblk -no MOUNTPOINTS "$DEV" 2>/dev/null | grep -q .; then
    echo "$DEV has mounted partitions; unmount them first:" >&2; lsblk "$DEV" >&2; exit 1
  fi
  lsblk -o NAME,SIZE,MODEL,TRAN,LABEL "$DEV"
  read -r -p "Erase everything on $DEV and write GameCoob? Type yes: " ans
  [ "$ans" = yes ] || { echo "Stopped."; exit 1; }
  dd if="$IMG" of="$DEV" bs=4M conv=fsync status=progress
  sync
  echo "Done. Boot the PC with the stick in and pick it in the boot menu (F12 on Dell)."
  exit 0
fi

for t in unzip sfdisk mkfs.fat mcopy mmd; do
  command -v "$t" >/dev/null || { echo "Missing $t (Arch: sudo pacman -S --needed unzip util-linux dosfstools mtools)" >&2; exit 1; }
done

# --- Clover ---
if [ -z "${CLOVER:-}" ]; then
  CLOVER=$(ls -t local/CloverV2*.zip CloverV2*.zip 2>/dev/null | head -n1 || true)
  [ -n "$CLOVER" ] || { echo "Set CLOVER=<CloverV2-*.zip> (download it into local/)" >&2; exit 1; }
fi
rm -rf "$WORK" && mkdir -p "$WORK"
if [ -f "$CLOVER" ]; then unzip -q "$CLOVER" -d "$WORK/clover"; CL=$WORK/clover; else CL=$CLOVER; fi
one() { find "$CL" "$@" -type f | head -n1; }
BOOT0=$(one -path '*BootSectors/boot0af')
BOOT1=$(one -path '*BootSectors/boot1f32')
STAGE2=$(one -path "*Bootloaders/x64/${BOOTFILE:-boot7}")
CLOVEREFI=$(one -path '*EFI/CLOVER/CLOVERX64.efi')
for v in BOOT0 BOOT1 STAGE2 CLOVEREFI; do
  [ -n "${!v}" ] || { echo "Not a Clover release (no $v in $CLOVER)" >&2; exit 1; }
done
CLEFI=$(dirname "$(dirname "$CLOVEREFI")")   # the zip's EFI folder
echo "Clover: $CLEFI, stage 2 $(basename "$STAGE2")"

# --- GameCoob ---
if [ -z "${BOOT:-}" ] && ! command -v cargo >/dev/null; then
  BOOT=$(ls -td local/GameCoob-boot-files* 2>/dev/null | head -n1 || true)
  [ -n "$BOOT" ] || { echo "No cargo and no local/GameCoob-boot-files*: set BOOT=<boot files>" >&2; exit 1; }
fi
if [ -n "${BOOT:-}" ]; then
  if [ -f "$BOOT" ]; then mkdir -p "$WORK/boot" && unzip -q "$BOOT" -d "$WORK/boot"; BOOT=$WORK/boot; fi
  APP=$(find "$BOOT" -path '*EFI/BOOT/BOOTX64.EFI' | head -n1)
  [ -n "$APP" ] || { echo "No EFI/BOOT/BOOTX64.EFI under $BOOT" >&2; exit 1; }
else
  cargo build --release
  APP=target/x86_64-unknown-uefi/release/gamecoob.efi
fi
if [ -z "${IPL:-}" ] && [ -f local/ipl.bin ]; then IPL=local/ipl.bin; fi
[ -n "${IPL:-}" ] || echo "Warning: no ipl.bin (set IPL=); GameCoob will go straight back to Clover" >&2

# --- Clover settings: start GameCoob after TIMEOUT seconds ---
mkdir -p "$WORK/cfg"
cat > "$WORK/cfg/config.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Boot</key>
	<dict>
		<key>DefaultVolume</key>
		<string>$LABEL</string>
		<key>DefaultLoader</key>
		<string>\\EFI\\GameCoob\\gamecoob.efi</string>
		<key>Timeout</key>
		<integer>${TIMEOUT:-3}</integer>
	</dict>
	<key>GUI</key>
	<dict>
		<key>TextOnly</key>
		<true/>
		<key>Scan</key>
		<dict>
			<key>Entries</key>
			<true/>
			<key>Legacy</key>
			<false/>
			<key>Tool</key>
			<true/>
		</dict>
		<key>Custom</key>
		<dict>
			<key>Entries</key>
			<array>
				<dict>
					<key>Disabled</key>
					<false/>
					<key>Hidden</key>
					<false/>
					<key>Path</key>
					<string>\\EFI\\GameCoob\\gamecoob.efi</string>
					<key>Title</key>
					<string>GameCoob</string>
					<key>Type</key>
					<string>Other</string>
					<key>Volume</key>
					<string>$LABEL</string>
				</dict>
			</array>
		</dict>
	</dict>
</dict>
</plist>
EOF

# --- FAT32 partition image ---
SIZE=${SIZE:-256}
PART=$WORK/part.img
truncate -s "$(( (SIZE - 1) * 1024 * 1024 ))" "$PART"
# -h 2048: the partition starts at sector 2048; boot1f32 reads that from the BPB.
mkfs.fat -F 32 -n "$LABEL" -h 2048 "$PART" >/dev/null
export MTOOLS_SKIP_CHECK=1
mcopy -s -i "$PART" "$CLEFI" ::/EFI
mcopy -o -i "$PART" "$WORK/cfg/config.plist" ::/EFI/CLOVER/config.plist
mmd -i "$PART" ::/EFI/GameCoob 2>/dev/null || true
mcopy -o -i "$PART" "$APP" ::/EFI/GameCoob/gamecoob.efi
[ -n "${IPL:-}" ] && mcopy -o -i "$PART" "$IPL" ::/EFI/GameCoob/ipl.bin
mcopy -o -i "$PART" "$STAGE2" ::/boot          # what boot1f32 loads
# boot1f32 goes in the partition's boot sector, keeping mkfs's BPB (bytes 3..89).
dd if="$PART" of="$WORK/origbs" bs=512 count=1 status=none
cp "$BOOT1" "$WORK/newbs"
dd if="$WORK/origbs" of="$WORK/newbs" bs=1 skip=3 seek=3 count=87 conv=notrunc status=none
dd if="$WORK/newbs" of="$PART" bs=512 count=1 conv=notrunc status=none

# --- Whole disk: MBR, one active FAT32 partition at 1 MiB, boot0af ---
rm -f "$IMG"
truncate -s "$(( SIZE * 1024 * 1024 ))" "$IMG"
printf 'label: dos\nstart=2048, type=c, bootable\n' | sfdisk -q "$IMG"
dd if="$BOOT0" of="$IMG" bs=440 count=1 conv=notrunc status=none   # keeps the partition table
dd if="$PART" of="$IMG" bs=1M seek=1 conv=notrunc status=none
rm -f "$PART"
echo "Built $IMG (${SIZE} MiB). Test: scripts/make-clover-usb.sh --qemu   Write: sudo scripts/make-clover-usb.sh --write /dev/sdX"
