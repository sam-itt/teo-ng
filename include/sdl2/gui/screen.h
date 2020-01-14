/*
  Hatari - screen.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/

#ifndef HATARI_SCREEN_H
#define HATARI_SCREEN_H
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <SDL_video.h>    /* for SDL_Surface */

#if WITH_SDL2
static inline int SDL_SetColors(SDL_Surface *surface, SDL_Color *colors,
                                int firstcolor, int ncolors)
{
	return SDL_SetPaletteColors(surface->format->palette, colors,
	                            firstcolor, ncolors);
}
void SDL_UpdateRects(SDL_Surface *screen, int numrects, SDL_Rect *rects);
void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h);
#define SDL_GRAB_OFF false
#define SDL_GRAB_ON true
#define SDL_WM_GrabInput SDL_SetRelativeMouseMode
#endif


extern bool bGrabMouse;
extern bool bInFullScreen;
extern int nScreenZoomX, nScreenZoomY;
extern int nBorderPixelsLeft, nBorderPixelsRight;
extern int STScreenStartHorizLine;
extern int STScreenLeftSkipBytes;
extern Uint8 *pSTScreen;
extern Uint32 STRGBPalette[16];
extern Uint32 ST2RGB[4096];


#endif  /* ifndef HATARI_SCREEN_H */
