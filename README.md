# viber_run_shim

Makes the Viber (Linux, `/opt/viber`) tray icon monochrome and colored like your
Plasma theme, instead of the purple logo.

## Before and after

| Theme | Before | After | After (unread) |
|---|:---:|:---:|:---:|
| Breeze Dark | ![Original Viber icon on a dark panel](assets/before-dark.png) | ![Themed icon on a dark panel](assets/after-normal-dark.png) | ![Themed unread icon on a dark panel](assets/after-unread-dark.png) |
| Breeze | ![Original Viber icon on a light panel](assets/before-light.png) | ![Themed icon on a light panel](assets/after-normal-light.png) | ![Themed unread icon on a light panel](assets/after-unread-light.png) |

The themed icon uses the panel's text color. When there are unread messages,
the signal waves switch to the theme's highlight color.

## Why a shim is needed

Viber hands the tray an already-drawn picture (StatusNotifierItem `IconPixmap`)
with an empty `IconName`, and it has no setting for a monochrome icon. The
desktop can't recolor a picture or replace it through an icon theme.

## How it works

1. **`src/viber-tray-shim.cpp`** is loaded into Viber with `LD_PRELOAD` and
   intercepts `QSystemTrayIcon::setIcon(const QIcon&)`. It passes
   `QIcon::fromTheme("viber-tray-normal")` instead, so Qt publishes an
   **icon name** over D-Bus rather than a picture.
   - It calls Viber's bundled Qt (6.5.x) by mangled symbol name, so it needs
     no Qt development headers to build.
   - `src/symbols.map` gives the hook the `Qt_6` symbol version, so it
     overrides Viber's versioned reference to `setIcon`.
2. **Unread state:** the shim scans the original 32×32 icon for red badge
   pixels. If more than 8 are found, it uses `viber-tray-unread` instead.
3. **`icons/*.svg`** are installed to
   `~/.local/share/icons/hicolor/scalable/status/`. They use KDE's
   `ColorScheme-Text` stylesheet, so Plasma draws them in the theme's text color
   and recolors them when the theme changes.
4. Viber ships without Qt's SVG icon-engine plugin, so its Qt can't load these
   SVGs, and `fromTheme` would come back empty. To fix this, the launcher adds
   `~/.local/lib/viber-tray/share` to `XDG_DATA_DIRS`. That folder holds PNG
   placeholders with the same names, which only Viber uses to resolve the name.
   Plasma ignores them and draws the SVGs.
5. **Safe fallback:** if the theme icons can't be resolved, the shim passes the
   original icon through unchanged.

## Files

| File | Purpose |
|---|---|
| `src/viber-tray-shim.cpp` | The `LD_PRELOAD` hook |
| `src/symbols.map` | Linker version script (exports the hook as `Qt_6`) |
| `icons/viber-tray-normal.svg`, `icons/viber-tray-unread.svg` | Monochrome, theme-recolorable tray icons |
| `icons/*-plain.svg` | Older handset-only icons (no bubble outline); copy over the main ones to use them |
| `assets/*.png` | Before/after examples shown above |
| `tools/make-icons.py` | Regenerates the tray icons from Viber's own logo (`/usr/share/icons/hicolor/scalable/apps/Viber.svg`) |
| `viber-themed` | Launcher: sets `LD_PRELOAD` and `XDG_DATA_DIRS`, then execs Viber |
| `build.sh` | Builds `build/viber-tray-shim.so` |
| `install.sh` | Builds and installs everything (see below) |
| `uninstall.sh` | Reverts the install |

## Requirements

- `g++` (no Qt dev packages needed)
- Viber installed in `/opt/viber` (bundled Qt 6)
- KDE Plasma (or any StatusNotifierItem host that loads icons by name)

## Install

```sh
./install.sh
```

The script installs:
- `~/.local/lib/viber-tray/viber-tray-shim.so` and the PNG placeholders
- SVG icons in `~/.local/share/icons/hicolor/scalable/status/`
- the launcher at `~/.local/bin/viber-themed`
- overrides for `viber.desktop` and `com.viber.Viber.desktop` in
  `~/.local/share/applications/`, pointing at the launcher
- a change to `~/.config/autostart/Viber.desktop` so it runs the launcher (the
  original is backed up to `~/.local/lib/viber-tray/autostart-Viber.desktop.bak`)

Then **quit Viber completely** (tray → Exit; if it keeps running,
`pkill -9 -x Viber`) and start it from the menu.

## Verify

```sh
for s in $(qdbus6 org.kde.StatusNotifierWatcher /StatusNotifierWatcher \
           org.kde.StatusNotifierWatcher.RegisteredStatusNotifierItems); do
  busctl --user get-property ${s%%/*} /${s#*/} org.kde.StatusNotifierItem Id IconName 2>/dev/null
done
```

Viber's entry should show `IconName "viber-tray-normal"` (or `viber-tray-unread`).

## Icon still purple?

If `IconName` is correct but the tray still shows the purple logo, plasmashell
is still using an icon lookup cached before the SVGs were installed. It treats
`viber-tray-normal` as missing and falls back to Viber's picture. Restart Plasma
to clear it (needed only once, after the first install):

```sh
systemctl --user restart plasma-plasmashell.service
```

## Debug and customize

- `VIBER_TRAY_DEBUG=1 viber-themed` prints `[viber-tray] setIcon: red=N normal=1 unread=1`
  for each icon change. `normal=0` means the name couldn't be resolved (check
  the PNG placeholders). If the unread icon never appears, check the `red=` value
  and adjust the threshold or color test in `redPixels()`.
- `VIBER_TRAY_ICON=<name>` and `VIBER_TRAY_ICON_UNREAD=<name>` use other icon
  names. Viber's Qt must be able to resolve each name, so put a PNG with that
  name in `~/.local/lib/viber-tray/share/icons/hicolor/24x24/status/`.
- To change the look, rerun `tools/make-icons.py` (adjust `WAVE_STROKE` for
  thicker or thinner waves), or edit the SVGs and keep
  `class="ColorScheme-Text"` / `fill:currentColor` so theme recoloring still works.

## Caveats

- **Viber updates:** if an update moves to a Qt version with a different ABI,
  rebuild with `./install.sh`. Viber may then show the default icon again until
  the shim is fixed.
- **Unread detection:** this relies on Viber drawing a red badge, and it hasn't
  been tested against a real unread message yet.
- **Menu overrides:** a Viber package update doesn't touch the overrides in
  `~/.local/share/applications`, but they replace the system entries.

## Uninstall

```sh
./uninstall.sh
```
