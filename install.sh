#!/bin/sh
# Build and install the shim, icons, launcher, and desktop/autostart overrides.
set -e
cd "$(dirname "$0")"
./build.sh

L="$HOME/.local/lib/viber-tray"
mkdir -p "$L/share/icons/hicolor/24x24/status" \
         "$HOME/.local/share/icons/hicolor/scalable/status" \
         "$HOME/.local/bin" "$HOME/.local/share/applications"

# Shim
install -m 755 build/viber-tray-shim.so "$L/"

# Themed SVGs: what Plasma actually draws (recolored via ColorScheme-Text)
install -m 644 icons/viber-tray-normal.svg icons/viber-tray-unread.svg \
    "$HOME/.local/share/icons/hicolor/scalable/status/"

# PNG placeholders so Viber's Qt (no SVG plugin) can resolve the names.
# Only Viber sees this dir (via XDG_DATA_DIRS in the launcher).
for n in normal unread; do
    cp /opt/viber/icons/24x24.png "$L/share/icons/hicolor/24x24/status/viber-tray-$n.png"
done

# Launcher
install -m 755 viber-themed "$HOME/.local/bin/viber-themed"

# Menu entries
for f in viber.desktop com.viber.Viber.desktop; do
    [ -f "/usr/share/applications/$f" ] || continue
    sed "s|^Exec=.*|Exec=$HOME/.local/bin/viber-themed %u|" \
        "/usr/share/applications/$f" > "$HOME/.local/share/applications/$f"
done

# Autostart (backup the original once)
A="$HOME/.config/autostart/Viber.desktop"
if [ -f "$A" ]; then
    [ -f "$L/autostart-Viber.desktop.bak" ] || cp "$A" "$L/autostart-Viber.desktop.bak"
    sed -i "s|^Exec=.*|Exec=$HOME/.local/bin/viber-themed|" "$A"
fi

command -v kbuildsycoca6 >/dev/null && kbuildsycoca6 >/dev/null 2>&1 || true
echo "Installed. Quit Viber completely (tray > Exit) and start it from the menu."
echo "If the tray icon stays purple, restart Plasma so it rescans icons:"
echo "  systemctl --user restart plasma-plasmashell.service"
