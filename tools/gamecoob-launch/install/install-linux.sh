#!/bin/sh
# Installs gamecoob-launch on Linux (tested target: Arch). Run with sudo from
# the folder holding the gamecoob-launch binary:
#
#   sudo ./install-linux.sh              install or update
#   sudo ./install-linux.sh --uninstall  remove
#
# What it sets up (docs/EMULATORS.md):
# - /usr/local/bin/gamecoob-launch
# - gamecoob-scan.service: at boot, as root, lists your games into
#   \EFI\GameCoob\library.toml on the ESP (Arch mounts the ESP root-only)
#   and keeps a readable copy of paths.toml in /var/lib/gamecoob.
# - /etc/xdg/autostart/gamecoob-launch.desktop: at desktop login, starts
#   the game picked in the boot menu.
set -eu

BIN=/usr/local/bin/gamecoob-launch
UNIT=/etc/systemd/system/gamecoob-scan.service
AUTOSTART=/etc/xdg/autostart/gamecoob-launch.desktop

if [ "$(id -u)" -ne 0 ]; then
    echo "run with sudo" >&2
    exit 1
fi

if [ "${1:-}" = "--uninstall" ]; then
    systemctl disable --now gamecoob-scan.service 2>/dev/null || true
    rm -f "$UNIT" "$AUTOSTART" "$BIN"
    rm -rf /var/lib/gamecoob
    systemctl daemon-reload
    echo "gamecoob-launch removed"
    exit 0
fi

here=$(dirname "$0")
src="$here/gamecoob-launch"
[ -f "$src" ] || src=gamecoob-launch
install -Dm755 "$src" "$BIN"

cat > "$UNIT" <<UNIT
[Unit]
Description=List games for the GameCoob boot menu
After=local-fs.target

[Service]
Type=oneshot
ExecStart=$BIN scan

[Install]
WantedBy=multi-user.target
UNIT

install -Dm644 /dev/stdin "$AUTOSTART" <<DESKTOP
[Desktop Entry]
Type=Application
Name=GameCoob launch
Comment=Start the game picked in the GameCoob boot menu
Exec=$BIN run
Terminal=false
NoDisplay=true
X-GNOME-Autostart-enabled=true
DESKTOP

systemctl daemon-reload
systemctl enable gamecoob-scan.service
"$BIN" check
"$BIN" scan
# A launch request left from before the install must not start a game at
# the next login.
if [ -n "${SUDO_USER:-}" ]; then
    sudo -u "$SUDO_USER" "$BIN" mark
fi
echo "gamecoob-launch installed"
