/* Freedroid Classic on AmigaOS / acsdl12: what the 8-bit screen needs.
 *
 * Included by src/system.h after the SDL headers. Freedroid was written
 * for 16 and 32-bit screens with alpha; acsdl12 has 8-bit surfaces only.
 * Every picture is converted on the host into one shared palette
 * (amiga/tools/convert_gfx.py) whose index 0 is the only transparent
 * colour, so "alpha on" becomes "colour key 0 on", and the screen is
 * given that palette when it opens. The wrappers below make the SDL
 * calls Freedroid relies on behave that way; amiga_fd.c has them, the
 * 8-bit zoom and rotation that stand in for SDL_gfx, and ApplyFilter's
 * colour tables.
 *
 * The AmiCradle Team. GPL 2 or later, like Freedroid.
 */
#ifndef AMIGA_FD_H
#define AMIGA_FD_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#define FD_KEY 0                /* the transparent pen of every picture */

/* acsdl12 has no cursors: Freedroid's two are made and set, never seen. */
typedef struct FD_Cursor { int dummy; } SDL_Cursor;
SDL_Cursor *amiga_CreateCursor(Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
void amiga_SetCursor(SDL_Cursor *cursor);
void amiga_FreeCursor(SDL_Cursor *cursor);

/* SDL_gfx, nearest neighbour, 8-bit, keeping the colour key. */
SDL_Surface *zoomSurface(SDL_Surface *src, double zoomx, double zoomy, int smooth);
SDL_Surface *rotozoomSurface(SDL_Surface *src, double angle, double zoom, int smooth);

SDL_Surface *amiga_SetVideoMode(int width, int height, int bpp, Uint32 flags);
SDL_Surface *amiga_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
                                    Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
SDL_Surface *amiga_DisplayFormatAlpha(SDL_Surface *surface);
SDL_Surface *amiga_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags);
int amiga_SetAlpha(SDL_Surface *surface, Uint32 flag, Uint8 alpha);
int amiga_ApplyFilter(SDL_Surface *surf, float fred, float fgreen, float fblue);
void amiga_init(void);

/* No gamma ramp on a palette screen; Freedroid only sets it to 1. */
static inline int amiga_SetGamma(float r, float g, float b)
{
    (void)r; (void)g; (void)b;
    return 0;
}

#ifndef AMIGA_FD_NO_MACROS
#define SDL_CreateCursor amiga_CreateCursor
#define SDL_SetCursor amiga_SetCursor
#define SDL_FreeCursor amiga_FreeCursor
#define SDL_SetGamma amiga_SetGamma
#define IMG_isJPG(rw) 0
#define SDL_SetVideoMode amiga_SetVideoMode
#define SDL_CreateRGBSurface amiga_CreateRGBSurface
#define SDL_DisplayFormatAlpha amiga_DisplayFormatAlpha
#define SDL_ConvertSurface amiga_ConvertSurface
#define SDL_SetAlpha amiga_SetAlpha
#endif

#endif
