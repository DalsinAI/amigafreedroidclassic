/* Freedroid Classic on AmigaOS / acsdl12: the 8-bit screen. See amiga_fd.h.
 *
 * The AmiCradle Team. GPL 2 or later, like Freedroid. The zoom and
 * rotation follow Freedroid's own nearest-neighbour helpers (upstream
 * commits 7f38e18e and 80fdcba1), here for 8-bit surfaces.
 */
#define AMIGA_FD_NO_MACROS
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "amiga_fd.h"

#define PALETTE_FILE "PROGDIR:graphics/palette.png"

static SDL_Color g_pal[256];
static int g_have_pal;

/* ---------------------------------------------------------- palette */

static void load_palette(void)
{
    SDL_Surface *p;
    int i;

    if (g_have_pal)
        return;
    p = IMG_Load(PALETTE_FILE);
    if (p && p->format->palette) {
        memcpy(g_pal, p->format->palette->colors, sizeof(g_pal));
    } else {
        /* No palette.png: 3-3-2, as acsdl12 gives its richer PNGs. */
        fprintf(stderr, "Freedroid: no %s, using a 3-3-2 palette\n", PALETTE_FILE);
        for (i = 0; i < 256; i++) {
            int r = (i >> 5) & 7, g = (i >> 2) & 7, b = i & 3;
            g_pal[i].r = (Uint8)(r << 5 | r << 2 | r >> 1);
            g_pal[i].g = (Uint8)(g << 5 | g << 2 | g >> 1);
            g_pal[i].b = (Uint8)(b << 6 | b << 4 | b << 2 | b);
            g_pal[i].unused = 0;
        }
    }
    if (p)
        SDL_FreeSurface(p);
    g_have_pal = 1;
}

static void give_palette(SDL_Surface *s)
{
    if (!s)
        return;
    load_palette();
    SDL_SetColors(s, g_pal, 0, 256);
}

SDL_Surface *amiga_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
    SDL_Surface *s;

    (void)bpp;
    s = SDL_SetVideoMode(width, height, 8, flags);
    give_palette(s);
    return s;
}

/* A new surface is in the screen's palette, as SDL_DisplayFormat would
 * make it, whatever depth Freedroid asks for (it asks for the screen's). */
SDL_Surface *amiga_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
                                    Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    SDL_Surface *s;

    (void)depth;
    s = SDL_CreateRGBSurface(flags, width, height, 8, Rmask, Gmask, Bmask, Amask);
    give_palette(s);
    return s;
}

/* An alpha surface: colour key 0, the transparent pen. */
SDL_Surface *amiga_DisplayFormatAlpha(SDL_Surface *surface)
{
    SDL_Surface *s = SDL_DisplayFormat(surface);

    if (s)
        SDL_SetColorKey(s, SDL_SRCCOLORKEY, FD_KEY);
    return s;
}

/* acsdl12's SDL_ConvertSurface drops the colour key that SDL 1.2 keeps
 * when SDL_SRCCOLORKEY is in the flags (Duplicate_Font relies on it). */
SDL_Surface *amiga_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags)
{
    SDL_Surface *s = SDL_ConvertSurface(src, fmt, flags);

    if (s && src && (flags & SDL_SRCCOLORKEY))
        SDL_SetColorKey(s, SDL_SRCCOLORKEY, src->format->colorkey);
    return s;
}

/* Per-surface alpha off, which Freedroid sets for its internal blits that
 * copy the alpha channel as it is, is the colour key off: the transparent
 * pen is copied as it is. On, it is the key on. */
int amiga_SetAlpha(SDL_Surface *surface, Uint32 flag, Uint8 alpha)
{
    if (!surface)
        return -1;
    if (flag & SDL_SRCALPHA)
        SDL_SetColorKey(surface, SDL_SRCCOLORKEY, FD_KEY);
    else
        SDL_SetColorKey(surface, 0, 0);
    return SDL_SetAlpha(surface, flag, alpha);
}

/* ---------------------------------------------------------- cursors */

static SDL_Cursor g_cursor;

SDL_Cursor *amiga_CreateCursor(Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
    (void)data; (void)mask; (void)w; (void)h; (void)hot_x; (void)hot_y;
    return &g_cursor;
}

void amiga_SetCursor(SDL_Cursor *cursor) { (void)cursor; }
void amiga_FreeCursor(SDL_Cursor *cursor) { (void)cursor; }

/* ------------------------------------------------------ zoom, rotate */

static SDL_Surface *like(SDL_Surface *src, int w, int h)
{
    SDL_Surface *d = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);

    if (!d)
        return NULL;
    SDL_SetColors(d, src->format->palette->colors, 0, 256);
    return d;
}

static void keep_key(SDL_Surface *src, SDL_Surface *dst)
{
    if (src->flags & SDL_SRCCOLORKEY)
        SDL_SetColorKey(dst, SDL_SRCCOLORKEY, src->format->colorkey);
}

SDL_Surface *zoomSurface(SDL_Surface *src, double zoomx, double zoomy, int smooth)
{
    SDL_Surface *dst;
    int dw, dh, x, y;
    int *xs;

    (void)smooth;
    if (!src || zoomx <= 0.0 || zoomy <= 0.0)
        return NULL;
    dw = (int)lround(src->w * zoomx);
    dh = (int)lround(src->h * zoomy);
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    dst = like(src, dw, dh);
    if (!dst)
        return NULL;
    xs = (int *)malloc(sizeof(int) * (size_t)dw);
    if (!xs) {
        SDL_FreeSurface(dst);
        return NULL;
    }
    for (x = 0; x < dw; x++) {
        int sx = (int)floor((x + 0.5) / zoomx);
        xs[x] = sx < 0 ? 0 : (sx >= src->w ? src->w - 1 : sx);
    }
    for (y = 0; y < dh; y++) {
        int sy = (int)floor((y + 0.5) / zoomy);
        const Uint8 *s;
        Uint8 *d = (Uint8 *)dst->pixels + y * dst->pitch;

        if (sy < 0) sy = 0;
        if (sy >= src->h) sy = src->h - 1;
        s = (const Uint8 *)src->pixels + sy * src->pitch;
        for (x = 0; x < dw; x++)
            d[x] = s[xs[x]];
    }
    free(xs);
    keep_key(src, dst);
    return dst;
}

/* angle in degrees, counter-clockwise as SDL_gfx; zoom is always 1 here */
SDL_Surface *rotozoomSurface(SDL_Surface *src, double angle, double zoom, int smooth)
{
    SDL_Surface *dst;
    double rad, s, c, scx, scy, dcx, dcy;
    int dw, dh, x, y;
    Uint8 fill;

    (void)smooth;
    if (!src)
        return NULL;
    if (zoom <= 0.0)
        zoom = 1.0;
    rad = -angle * M_PI / 180.0;
    s = sin(rad) / zoom;
    c = cos(rad) / zoom;
    dw = (int)ceil((src->w * fabs(c) + src->h * fabs(s)) * zoom * zoom);
    dh = (int)ceil((src->w * fabs(s) + src->h * fabs(c)) * zoom * zoom);
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    dst = like(src, dw, dh);
    if (!dst)
        return NULL;
    fill = (src->flags & SDL_SRCCOLORKEY) ? (Uint8)src->format->colorkey : 0;
    SDL_FillRect(dst, NULL, fill);
    scx = (src->w - 1) * 0.5;
    scy = (src->h - 1) * 0.5;
    dcx = (dw - 1) * 0.5;
    dcy = (dh - 1) * 0.5;
    for (y = 0; y < dh; y++) {
        double dy = y - dcy;
        Uint8 *d = (Uint8 *)dst->pixels + y * dst->pitch;

        for (x = 0; x < dw; x++) {
            double dx = x - dcx;
            int sx = (int)lround(c * dx + s * dy + scx);
            int sy = (int)lround(-s * dx + c * dy + scy);

            if (sx >= 0 && sx < src->w && sy >= 0 && sy < src->h)
                d[x] = ((const Uint8 *)src->pixels)[sy * src->pitch + sx];
        }
    }
    keep_key(src, dst);
    return dst;
}

/* ------------------------------------------------------- ApplyFilter */

/* Multiplying a pen's colour by (fred, fgreen, fblue) and finding the
 * nearest pen, as a table per filter: the low-energy blink steps through
 * greys, transfer mode is red. Factors are taken in 1/32 steps. */
#define FILTERS 24

typedef struct {
    int used;
    Uint8 fr, fg, fb;
    Uint8 map[256];
} Filter;

static Filter g_filters[FILTERS];
static int g_next_filter;

static Uint8 nearest_pen(int r, int g, int b)
{
    int i, best = 1;
    long best_d = 0x7fffffffL;

    for (i = 1; i < 256; i++) {     /* never the transparent pen 0 */
        long dr = r - g_pal[i].r, dg = g - g_pal[i].g, db = b - g_pal[i].b;
        long d = dr * dr * 3 + dg * dg * 4 + db * db * 2;

        if (i == 1)                 /* nor BFont's magenta marker */
            continue;
        if (d < best_d) {
            best_d = d;
            best = i;
            if (!d)
                break;
        }
    }
    return (Uint8)best;
}

static Uint8 step(float f)
{
    int v = (int)(f * 32.0f + 0.5f);

    return (Uint8)(v < 0 ? 0 : (v > 32 ? 32 : v));
}

static const Uint8 *filter_map(float fred, float fgreen, float fblue)
{
    Uint8 fr = step(fred), fg = step(fgreen), fb = step(fblue);
    Filter *f;
    int i;

    for (i = 0; i < FILTERS; i++)
        if (g_filters[i].used && g_filters[i].fr == fr && g_filters[i].fg == fg &&
            g_filters[i].fb == fb)
            return g_filters[i].map;
    load_palette();
    f = &g_filters[g_next_filter];
    g_next_filter = (g_next_filter + 1) % FILTERS;
    f->used = 1;
    f->fr = fr;
    f->fg = fg;
    f->fb = fb;
    f->map[0] = 0;
    for (i = 1; i < 256; i++)
        f->map[i] = nearest_pen(g_pal[i].r * fr / 32, g_pal[i].g * fg / 32,
                                g_pal[i].b * fb / 32);
    return f->map;
}

int amiga_ApplyFilter(SDL_Surface *surf, float fred, float fgreen, float fblue)
{
    const Uint8 *map;
    Uint8 key;
    int keyed, x, y;

    if (!surf)
        return -1;
    map = filter_map(fred, fgreen, fblue);
    keyed = (surf->flags & SDL_SRCCOLORKEY) != 0;
    key = (Uint8)surf->format->colorkey;
    for (y = 0; y < surf->h; y++) {
        Uint8 *p = (Uint8 *)surf->pixels + y * surf->pitch;

        for (x = 0; x < surf->w; x++)
            if (!keyed || p[x] != key)
                p[x] = map[p[x]];
    }
    return 0;
}

/* -------------------------------------------------------------- init */

void amiga_init(void)
{
    /* No F12 on the Amiga: Help takes Freedroid's screenshot key. */
    ACSDL_SetKeyMap(0x5f, SDLK_F12);
}
