#!/usr/bin/env python3
"""convert_gfx: Freedroid Classic's pictures in one 256-colour palette.

acsdl12 draws on an 8-bit screen. Its PNG loader keeps a paletted PNG's
palette, has no JPEG reader, and makes a PNG's transparency a colour key.
Freedroid's pictures are RGBA PNGs and full-colour JPEGs, drawn with alpha.

This makes one palette from all the pictures (all three themes, the
backgrounds, the droid portraits, and the dimmed and red-filtered droids
that ApplyFilter makes in the game), and writes every picture as a paletted
PNG in it:

  index 0      black, and the only transparent colour (tRNS 0): every
               picture's pixels under half alpha, and nothing else
  index 1      magenta (255, 0, 255), BFont's character markers, exact
  index 2-255  the pictures' colours (libimagequant when Pillow has it);
               the large pictures dithered, sprites and fonts not

Fonts are made opaque where alpha >= 96, their soft edges darkened as over
black. The JPEGs become PNGs: title.jpg -> title.png, and a droid's
rotation frames NNN.jpg -> NNN_rot.png (NNN.png is its still picture).
palette.png (16x16 pixels of indices 0-255) is loaded by the game to set
the screen's palette (amiga/amiga_fd.c).

  python3 amiga/tools/convert_gfx.py graphics amiga/graphics

Needs Pillow and numpy; the output is committed, so the Amiga build
needs neither. The AmiCradle Team. GPL 2 or later, like Freedroid.
"""

import os
import sys

import numpy as np
from PIL import Image, features

KEY, MAGENTA, FIRST = 0, 1, 2
NCOLOURS = 256 - FIRST
BIG = 200 * 200
FONTS = {"font05.png", "font05_green.png", "font05_red.png", "parafont.png"}
SKIP = {"paraicon.bmp"}


def load(path):
    im = Image.open(path)
    im.load()
    return np.asarray(im.convert("RGBA"), dtype=np.uint8).copy()


def out_name(rel):
    d, n = os.path.split(rel)
    base, ext = os.path.splitext(n)
    if ext.lower() == ".jpg":
        n = (base + "_rot.png") if base.isdigit() else (base + ".png")
    return os.path.join(d, n)


def prepare(rel, rgba):
    """Alpha to on/off: returns (rgb, opaque mask)."""
    a = rgba[:, :, 3].astype(np.int32)
    rgb = rgba[:, :, :3].astype(np.int32)
    if os.path.basename(rel) in FONTS:
        opaque = a >= 96
        magenta = (rgb[:, :, 0] == 255) & (rgb[:, :, 1] == 0) & (rgb[:, :, 2] == 255)
        dark = (rgb * a[:, :, None]) // 255
        rgb = np.where(magenta[:, :, None], rgb, dark)
    else:
        opaque = a >= 128
    return rgb.astype(np.uint8), opaque


def sample_rows(pictures):
    rows = []
    for rel, (rgb, opaque) in pictures.items():
        px = rgb[opaque]
        px = px[~((px[:, 0] == 255) & (px[:, 1] == 0) & (px[:, 2] == 255))]
        if not len(px):
            continue
        repeat = max(1, min(16, 16384 // len(px)))
        rows.append(np.repeat(px, repeat, axis=0))
        if os.path.basename(rel) == "droids.png":
            # ApplyFilter: the low-energy blink (grey 0.4-1.0) and the
            # transfer mode's red.
            for f in (0.4, 0.55, 0.7, 0.85):
                rows.append(np.repeat((px * f).astype(np.uint8), repeat, axis=0))
            red = px.copy()
            red[:, 1:] = 0
            rows.append(np.repeat(red, repeat, axis=0))
    return np.concatenate(rows)


def make_palette(sample):
    width = 1024
    pad = (-len(sample)) % width
    sample = np.concatenate([sample, np.repeat(sample[:1], pad, axis=0)])
    im = Image.fromarray(sample.reshape(-1, width, 3), "RGB")
    method = (Image.Quantize.LIBIMAGEQUANT if features.check("libimagequant")
              else Image.Quantize.MEDIANCUT)
    q = im.quantize(colors=NCOLOURS, method=method, dither=Image.Dither.NONE)
    pal = np.array(q.getpalette()[:3 * NCOLOURS], dtype=np.uint8).reshape(-1, 3)
    used = sorted(set(np.unique(np.asarray(q)).tolist()))
    colours = sorted(set(tuple(int(v) for v in pal[i]) for i in used))
    colours = [c for c in colours if c != (255, 0, 255)]
    while len(colours) < NCOLOURS:
        colours.append(colours[-1])
    return colours[:NCOLOURS]


def to_indices(rgb, opaque, colours, dither, keep_magenta):
    palimg = Image.new("P", (1, 1))
    flat = []
    for c in colours:
        flat.extend(c)
    palimg.putpalette(flat)
    im = Image.fromarray(np.ascontiguousarray(rgb), "RGB")
    d = Image.Dither.FLOYDSTEINBERG if dither else Image.Dither.NONE
    q = np.asarray(im.quantize(palette=palimg, dither=d), dtype=np.int32)
    q = np.where(q < len(colours), q, 0) + FIRST
    out = q.astype(np.uint8)
    if keep_magenta:
        magenta = (rgb[:, :, 0] == 255) & (rgb[:, :, 1] == 0) & (rgb[:, :, 2] == 255)
        out[magenta & opaque] = MAGENTA
    out[~opaque] = KEY
    return out


def write_png(path, indices, palette, keyed):
    im = Image.fromarray(indices, "P")
    flat = []
    for c in palette:
        flat.extend(c)
    im.putpalette(flat)
    kw = {"optimize": True}
    if keyed:
        kw["transparency"] = bytes([0] + [255] * 255)
    im.save(path, **kw)


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: convert_gfx.py GRAPHICS_DIR OUT_DIR")
    src, dst = sys.argv[1], sys.argv[2]
    pictures, others = {}, []
    for root, _dirs, files in os.walk(src):
        for name in sorted(files):
            rel = os.path.relpath(os.path.join(root, name), src)
            low = name.lower()
            if name in SKIP:
                continue
            if low.endswith((".png", ".jpg")):
                pictures[rel] = prepare(rel, load(os.path.join(src, rel)))
            elif name == "config.theme":
                others.append(rel)
    colours = make_palette(sample_rows(pictures))
    palette = [(0, 0, 0), (255, 0, 255)] + colours
    for rel, (rgb, opaque) in sorted(pictures.items()):
        out = os.path.join(dst, out_name(rel))
        os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
        h, w = opaque.shape
        dither = rel.lower().endswith(".jpg") or (h * w >= BIG and opaque.all())
        idx = to_indices(rgb, opaque, colours, dither,
                         os.path.basename(rel) in FONTS)
        write_png(out, idx, palette, bool((~opaque).any()))
    for rel in others:
        out = os.path.join(dst, rel)
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(os.path.join(src, rel), "rb") as f, open(out, "wb") as g:
            g.write(f.read())
    write_png(os.path.join(dst, "palette.png"),
              np.arange(256, dtype=np.uint8).reshape(16, 16), palette, False)
    print(f"{len(pictures)} pictures, {len(set(colours))} colours")


if __name__ == "__main__":
    main()
