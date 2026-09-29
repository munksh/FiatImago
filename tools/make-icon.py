#!/usr/bin/env python3
"""fiat imago's launcher icon: the family silhouette in grey, with an RGB
histogram on a thin baseline. The channels add like light, so where all three
overlap the histogram is white and the colours show only at the edges: the
same drawing as the histogram inside the app. The outer peaks are rounded
so the histogram can be large without reaching the corners. Writes the four
PNG sizes. Needs Pillow; run from the repository root."""

import math
import os

from PIL import Image, ImageChops, ImageDraw

GROUND = (0x5C, 0x5C, 0x5C, 255)
BASELINE = (0xED, 0xED, 0xED, 255)
COLOURS = {"R": (215, 60, 55), "G": (60, 185, 80), "B": (60, 105, 230)}
SIZES = [86, 108, 128, 172]
SS = 8

# Peaks as (height, position, width) along the histogram. Red leans to the
# highlights, blue to the shadows, green sits in the middle.
CHANNELS = {
    "R": [(0.40, 0.30, 0.11), (0.95, 0.58, 0.15), (0.38, 0.80, 0.10)],
    "G": [(0.55, 0.26, 0.11), (1.00, 0.52, 0.16), (0.30, 0.78, 0.09)],
    "B": [(0.62, 0.22, 0.11), (0.80, 0.46, 0.16), (0.20, 0.74, 0.09)],
}

SCALE = 1.25
HALF = 0.22 * SCALE
HEIGHT = 0.30 * SCALE
BASE = 0.64 + (SCALE - 1.0) * 0.10
LEFT, RIGHT = 0.5 - HALF, 0.5 + HALF
LINE = 0.015
OVERHANG = 0.005


def silhouette_mask(s):
    mask = Image.new("L", (s, s), 0)
    d = ImageDraw.Draw(mask)
    d.ellipse([0, 0, s - 1, s - 1], fill=255)
    d.rectangle([s / 2, 0, s - 1, s / 2], fill=255)
    d.rectangle([0, s / 2, s / 2, s - 1], fill=255)
    return mask


def channel_mask(s, peaks):
    mask = Image.new("L", (s, s), 0)
    points = [(LEFT * s, BASE * s)]
    for i in range(161):
        t = i / 160
        level = sum(a * math.exp(-((t - c) / w) ** 2) for a, c, w in peaks)
        points.append(((LEFT + (RIGHT - LEFT) * t) * s, (BASE - HEIGHT * min(1.0, level)) * s))
    points.append((RIGHT * s, BASE * s))
    ImageDraw.Draw(mask).polygon(points, fill=255)
    return mask


def icon(size):
    s = size * SS
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    img.paste(Image.new("RGBA", (s, s), GROUND), (0, 0), silhouette_mask(s))

    masks = {k: channel_mask(s, v) for k, v in CHANNELS.items()}
    light = Image.new("RGB", (s, s), (0, 0, 0))
    black = Image.new("RGB", (s, s), (0, 0, 0))
    for k in ("R", "G", "B"):
        colour = Image.new("RGB", (s, s), COLOURS[k])
        light = ImageChops.add(light, Image.composite(colour, black, masks[k]))
    cover = ImageChops.lighter(ImageChops.lighter(masks["R"], masks["G"]), masks["B"])
    layer = light.convert("RGBA")
    layer.putalpha(cover)
    img = Image.alpha_composite(img, layer)

    ImageDraw.Draw(img).rounded_rectangle(
        [(LEFT - OVERHANG) * s, BASE * s, (RIGHT + OVERHANG) * s, (BASE + LINE) * s],
        radius=LINE * s / 2, fill=BASELINE)
    return img.resize((size, size), Image.LANCZOS)


if __name__ == "__main__":
    for size in SIZES:
        folder = "icons/%dx%d" % (size, size)
        os.makedirs(folder, exist_ok=True)
        icon(size).save("%s/harbour-fiatimago.png" % folder)
    print("icons written")
