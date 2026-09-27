#!/usr/bin/env python3
"""fiat imago's launcher icon: the family silhouette in grey, with an RGB
histogram on a thin baseline. The channels add like light, so where all three
overlap the histogram is white and the colours show only at the edges: the
same drawing as the histogram inside the app. Writes the four PNG sizes.
Needs Pillow; run from the repository root."""

import math
import os

from PIL import Image, ImageChops, ImageDraw

GROUND = (0x5C, 0x5C, 0x5C, 255)
BASELINE = (0xED, 0xED, 0xED, 255)
COLOURS = {"R": (215, 60, 55), "G": (60, 185, 80), "B": (60, 105, 230)}
SIZES = [86, 108, 128, 172]
SS = 8

# Red leans to the highlights, blue to the shadows, green sits in the middle.
CHANNELS = {
    "R": [(0.40, 0.30, 0.10), (0.95, 0.58, 0.14), (0.45, 0.84, 0.06)],
    "G": [(0.55, 0.24, 0.10), (1.00, 0.52, 0.15), (0.30, 0.80, 0.07)],
    "B": [(0.70, 0.18, 0.09), (0.80, 0.46, 0.15), (0.20, 0.76, 0.08)],
}

LEFT, RIGHT, BASE, HEIGHT = 0.28, 0.72, 0.64, 0.30
LINE = 0.015


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
    for i in range(81):
        t = i / 80
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

    ImageDraw.Draw(img).rectangle(
        [(LEFT - 0.02) * s, BASE * s, (RIGHT + 0.02) * s, (BASE + LINE) * s], fill=BASELINE)
    return img.resize((size, size), Image.LANCZOS)


if __name__ == "__main__":
    for size in SIZES:
        folder = "icons/%dx%d" % (size, size)
        os.makedirs(folder, exist_ok=True)
        icon(size).save("%s/harbour-fiatimago.png" % folder)
    print("icons written")
