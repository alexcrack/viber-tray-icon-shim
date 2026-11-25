#!/usr/bin/env python3
"""Regenerate icons/viber-tray-{normal,unread}.svg from Viber's own logo.

Keeps the speech-bubble outline, handset and signal waves of the original
logo, drops the purple background, and makes everything theme-colored
(ColorScheme-Text / ColorScheme-Highlight with fill:currentColor).

Usage: tools/make-icons.py [SOURCE_SVG] [OUT_DIR]
"""
import os
import re
import sys

SRC = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/icons/hicolor/scalable/apps/Viber.svg"
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), "..", "icons")
WAVE_STROKE = 7  # thickens the thin wave arcs so they survive at 22px

src = open(SRC, encoding="utf-8").read()
d = [re.sub(r"\s+", " ", x).strip() for x in re.findall(r'\sd="([^"]+)"', src)]
if len(d) != 7:
    sys.exit(f"Unexpected Viber.svg layout: {len(d)} paths (expected 7)")
# 0 bubble fill, 1 purple background, 2 bubble outline, 3-5 waves, 6 handset
ring, waves, phone = d[2], d[3:6], d[6]

HEAD = """<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 256 256" version="1.1">
 <defs>
  <style id="current-color-scheme" type="text/css">
   .ColorScheme-Text { color:#d3dae3; } .ColorScheme-Highlight { color:#5294e2; }
  </style>
 </defs>
"""


def path(dd, cls, thick=False):
    st = f";stroke:currentColor;stroke-width:{WAVE_STROKE};stroke-linejoin:round" if thick else ""
    return f' <path style="fill:currentColor{st}" class="ColorScheme-{cls}" d="{dd}"/>\n'


def icon(wave_cls):
    s = HEAD + path(ring, "Text") + path(phone, "Text")
    s += "".join(path(w, wave_cls, True) for w in waves)
    return s + "</svg>\n"


for name, cls in (("normal", "Text"), ("unread", "Highlight")):
    p = os.path.join(OUT, f"viber-tray-{name}.svg")
    with open(p, "w", encoding="utf-8") as f:
        f.write(icon(cls))
    print("wrote", os.path.normpath(p))
