#!/bin/sh
# Revert everything install.sh did.
L="$HOME/.local/lib/viber-tray"
A="$HOME/.config/autostart/Viber.desktop"
[ -f "$L/autostart-Viber.desktop.bak" ] && cp "$L/autostart-Viber.desktop.bak" "$A"
rm -f "$HOME/.local/bin/viber-themed" \
      "$HOME/.local/share/applications/viber.desktop" \
      "$HOME/.local/share/applications/com.viber.Viber.desktop" \
      "$HOME/.local/share/icons/hicolor/scalable/status/viber-tray-normal.svg" \
      "$HOME/.local/share/icons/hicolor/scalable/status/viber-tray-unread.svg"
rm -rf "$L"
command -v kbuildsycoca6 >/dev/null && kbuildsycoca6 >/dev/null 2>&1 || true
echo "Uninstalled. Restart Viber."
