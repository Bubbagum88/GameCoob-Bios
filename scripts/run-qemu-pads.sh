#!/usr/bin/env bash
# Boot GameCoob in QEMU with this PC's real USB controllers handed to the VM.
# For PCs that can't boot UEFI apps natively (legacy-BIOS-only laptops).
#
# Usage (from the repo root):
#   scripts/run-qemu-pads.sh --list        show the pads found and what QEMU would get
#   sudo scripts/run-qemu-pads.sh --udev   one time: let your user open those pads
#   scripts/run-qemu-pads.sh [qemu args]   boot (same env vars as run-qemu.sh)
#
# Pads are found by USB interface class: HID that isn't a boot keyboard/mouse,
# Xbox 360/One vendor interfaces, plus the Wii U GameCube adapter (057e:0337).
# PADS="057e:0337 045e:028e" picks devices by vendor:product instead.
# Each pad is detached from Linux while QEMU runs and comes back when it quits.
set -euo pipefail
cd "$(dirname "$0")/.."
SYSFS=${SYSFS:-/sys/bus/usb/devices}
RULES=/etc/udev/rules.d/70-gamecoob-pads.rules

# Prints "bus addr vid pid name" for each pad found.
find_pads() {
  local d vid pid ifc cls sub proto want kbd
  for d in "$SYSFS"/*; do
    [ -f "$d/idVendor" ] || continue
    vid=$(cat "$d/idVendor"); pid=$(cat "$d/idProduct")
    [ "$(cat "$d/bDeviceClass" 2>/dev/null)" = 09 ] && continue  # hub
    want=0
    if [ -n "${PADS:-}" ]; then
      case " $PADS " in *" $vid:$pid "*) want=1 ;; esac
    else
      [ "$vid:$pid" = 057e:0337 ] && want=1
      kbd=0
      for ifc in "$d"/"$(basename "$d")":*; do
        [ -f "$ifc/bInterfaceClass" ] || continue
        cls=$(cat "$ifc/bInterfaceClass"); sub=$(cat "$ifc/bInterfaceSubClass"); proto=$(cat "$ifc/bInterfaceProtocol")
        # HID with protocol 0 is a pad candidate; any boot keyboard (1) or
        # mouse (2) interface rules the whole device out (media-key interfaces).
        [ "$cls" = 03 ] && [ "$proto" = 00 ] && want=1
        [ "$cls" = 03 ] && { [ "$proto" = 01 ] || [ "$proto" = 02 ]; } && kbd=1
        # Xbox 360 (ff/5d/01) and Xbox One (ff/47/d0) pads.
        [ "$cls" = ff ] && { [ "$sub/$proto" = 5d/01 ] || [ "$sub/$proto" = 47/d0 ]; } && want=1
      done
      [ "$kbd" = 1 ] && want=0
    fi
    [ "$want" = 1 ] || continue
    echo "$(cat "$d/busnum") $(cat "$d/devnum") $vid $pid $(cat "$d/product" 2>/dev/null || echo '?')"
  done
}

mapfile -t pads < <(find_pads)

case "${1:-}" in
  --list)
    [ ${#pads[@]} -gt 0 ] || { echo "No pads found. Plug one in, or set PADS=vid:pid (see lsusb)."; exit 0; }
    for p in "${pads[@]}"; do
      read -r bus addr vid pid name <<<"$p"
      node=$(printf '/dev/bus/usb/%03d/%03d' "$bus" "$addr")
      ok=no; [ -r "$node" ] && [ -w "$node" ] && ok=yes
      echo "$vid:$pid  bus $bus addr $addr  $name  (you can open it: $ok)"
    done
    exit 0 ;;
  --udev)
    [ "$(id -u)" = 0 ] || { echo "Run with sudo" >&2; exit 1; }
    [ ${#pads[@]} -gt 0 ] || { echo "No pads found: plug them in first, or set PADS=vid:pid" >&2; exit 1; }
    touch "$RULES"
    for p in "${pads[@]}"; do
      read -r _ _ vid pid name <<<"$p"
      line="SUBSYSTEM==\"usb\", ATTR{idVendor}==\"$vid\", ATTR{idProduct}==\"$pid\", MODE=\"0660\", GROUP=\"kvm\"  # $name"
      grep -q "\"$vid\".*\"$pid\"" "$RULES" || echo "$line" >> "$RULES"
    done
    udevadm control --reload
    udevadm trigger --subsystem-match=usb
    echo "Wrote $RULES. Your user needs to be in group kvm (sudo usermod -aG kvm \$USER, then log in again)."
    exit 0 ;;
esac

[ -e /dev/kvm ] || echo "Note: no /dev/kvm, QEMU will be slow. Turn on Virtualization (VT) in the BIOS setup, then: sudo modprobe kvm_intel" >&2

args=(-device qemu-xhci,id=xhci)
for p in "${pads[@]}"; do
  read -r bus addr vid pid name <<<"$p"
  node=$(printf '/dev/bus/usb/%03d/%03d' "$bus" "$addr")
  if ! { [ -r "$node" ] && [ -w "$node" ]; }; then
    echo "Can't open $name ($vid:$pid, $node). Run once: sudo scripts/run-qemu-pads.sh --udev" >&2
    exit 1
  fi
  echo "Pad: $vid:$pid $name"
  args+=(-device "usb-host,bus=xhci.0,hostbus=$bus,hostaddr=$addr")
done
[ ${#pads[@]} -gt 0 ] || echo "No pads found; booting with keyboard only. (--list to check)" >&2

exec scripts/run-qemu.sh "${args[@]}" "$@"
