# Freedroid Classic for AmigaOS 3.x

- Upstream: `https://github.com/ReinhardPrix/FreedroidClassic`
- Based on: `efd34bebb8024698742ab2685a8941e480c23ee0` ("AndroidAppSettings:
  update version, android-34 platform", 21 Feb 2026), the last commit of
  master on SDL 1.2. Master's head (`d4854d8e340e2976e02fa76bbbfd19e63bc502a4`)
  has since moved to SDL2 and then SDL3 (from `0b5ab7f6`), which acsdl12
  does not provide; the game itself is the same.
- Licence: GPL 2 or later (`COPYING`)

## Target

AmigaOS 3.x / AmiCradle, Motorola 68040 + FPU, on acsdl12 (our SDL 1.2 on
ACGame and AHI). 640x480 in 256 colours, RTG or AGA (`ENV:ACGame/Display`).

## Build

    make -f amiga/Makefile.amigaos \
        STOVE=/path/to/openamigagcc-16.2.0-open4-x86_64-linux \
        NDK=/path/to/gfopensdk/OpenNDK/Include_H \
        ACSDL=/path/to/amicradle-guest/gameports/acsdl12 \
        ACGAME=/path/to/amicradle-guest/gameports/acgame

acsdl12 is built with its own `Makefile.amigaos`. This gives
`build/amigaos/Freedroid/` with `freedroid`, `graphics/` (with the three
theme drawers `classic_theme`, `lanzz_theme`, `para90_theme`), `sound/`,
`map/`, `Config/`, this ReadMe, `COPYING`, an icon for each of them and
for every drawer, and `Freedroid.info` beside the drawer. The icons are
made from droid 001's picture with AmiCradle's `port_icons.py` from
`amiga/icons/icons.json`.

## Playing

Run it from its drawer (Workbench or Shell): everything is found through
`PROGDIR:`. The options and the highscores are written in `Config/`, so the
drawer must be on a writable disk for them to be kept.

Keys as upstream: cursor keys move, Space fires, Space held (or Backspace,
Tab) takes over, Return/`e` activates consoles and lifts, `p` pauses,
Esc is the menu. The Amiga has no F12: Help takes a screenshot
(`Screenshot_N.bmp` in the current directory). Mouse and joystick work.

## Graphics in 256 colours

Freedroid draws RGBA PNGs and full-colour JPEGs with alpha on 16 or
32-bit screens. acsdl12 has 8-bit surfaces only and reads PNG only.

- `amiga/tools/convert_gfx.py` (host; Pillow and numpy) makes one palette
  from every picture (all three themes, the backgrounds, the droid
  portraits, and the dimmed and red droids ApplyFilter makes) and writes
  each picture as a paletted PNG in it: index 0 black and the only
  transparent colour (alpha under half), 1 magenta for BFont's markers,
  2-255 the colours; backgrounds and portraits dithered. Fonts are solid
  from alpha 96, their soft edges darkened as over black. The JPEGs become
  PNGs (`title.png` and so on; a droid's rotation frames `NNN_rot.png`).
  Its output, `amiga/graphics`, is committed with `palette.png`; the Amiga
  build copies it.
- `amiga/amiga_fd.c`, `amiga/amiga_fd.h` (included by `src/system.h`):
  the screen and every new surface get the shared palette, so blits are
  copies. Alpha becomes colour key 0: `SDL_DisplayFormatAlpha` and
  `SDL_SetAlpha(SDL_SRCALPHA)` key pen 0, `SDL_SetAlpha(0)` (Freedroid's
  "copy the alpha as it is" for its internal blits) turns the key off.
  `zoomSurface` and `rotozoomSurface` (SDL_gfx) are 8-bit nearest-pixel,
  after upstream's own later helpers. `ApplyFilter` (the low-energy blink,
  transfer mode's red) maps pens through a cached table per filter.
  Cursors and gamma are stubs; `SDL_ConvertSurface` keeps the colour key
  when asked (acsdl12 drops it).
- Lost: soft (anti-aliased) edges, smooth scaling, gamma correction.

## Sound

Music: the MODs as they are (4-channel M.K. and starpaws' 6CHN); the
Android-only `.mod.ogg` copies and `Paradroid.ogg` are left out. Sounds: the
WAVs as they are (8 and 16-bit PCM).

## Changes to upstream

Under `#ifdef __amigaos__`:

- `src/system.h`: `amiga_fd.h` in place of `SDL/SDL_rotozoom.h`.
- `src/defs.h`: the JPEG backgrounds by their PNG names; `DROID_ROT_SUFFIX`.
- `src/graphics.c`: `ApplyFilter` calls `amiga_ApplyFilter`; `Load_Block`
  always takes the alpha path (colour key 0); droid rotation frames by
  `DROID_ROT_SUFFIX` (`.jpg` elsewhere, as before).
- `src/misc.c`: `find_file` asks for `NAME.png` when given `NAME.jpg`
  (`map/Paradroid.mission` names `title.jpg`), and builds `PROGDIR:graphics/...` (no `/` after the
  colon, no `//`); the config drawer is `PROGDIR:Config`.
- `src/init.c`: themes are looked for in `PROGDIR:graphics`.
- `src/main.c`: `amiga_init()` (Help as F12).

New: `amiga/` (Makefile, `config.h`, `amiga_fd.[ch]`, `stack.c` 160 KB,
the converter, the converted graphics, the icons) and this file.

## Big-endian

Freedroid's data are text (maps, ruleset, themes, config), PNG, WAV and
MOD; acsdl12 reads the last three in the right byte order. The one binary
file is `Config/highscores`, a raw dump of the highscore records written
and read by the same program, so it is in the Amiga's own order; one copied
from a PC build would read as nonsense scores (delete it). `GetRGBA`'s
32-bit pixel read is not used on the Amiga (`ApplyFilter` is replaced).
