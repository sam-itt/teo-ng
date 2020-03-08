/*
  Original code from Hatari, adapted for Teo
*/
#if HAVE_CONFIG_H
# include <config.h>
#endif

#if WITH_SDL2
#include <SDL.h>
#include "sdl2/gui/sdlgui.h"
#include "logsys.h"
/*static SDL_Renderer *sdlRenderer;
static SDL_Texture *sdlTexture;
static bool bUseSdlRenderer = false; */           /* true when using SDL2 renderer */

void SDL_UpdateRects(SDL_Surface *screen, int numrects, SDL_Rect *rects)
{
/*	if (bUseSdlRenderer)
	{
		SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
		SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
		SDL_RenderPresent(sdlRenderer);
	}
	else*/
	{
        if(screen != SDLGui_GetScreen()){
            log_msgf(LOG_ERROR,"%s:%d window/screen mismatch. THIS SHOULDN'T HAPPEN",__FUNCTION__,__LINE__);
        }
		SDL_UpdateWindowSurfaceRects(SDLGui_GetWindow(), rects, numrects);
        
	}
}

void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
	SDL_Rect rect;

	if (w == 0 && h == 0) {
		x = y = 0;
		w = screen->w;
		h = screen->h;
	}

	rect.x = x; rect.y = y;
	rect.w = w; rect.h = h;
	SDL_UpdateRects(screen, 1, &rect);
}

#endif /* WITH_SDL2 */

