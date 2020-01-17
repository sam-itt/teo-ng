#ifndef TEO_SDL_H
#define TEO_SDL_H

#include <SDL.h>

void teoSDL_GfxInit(void);
void teo_sdl_RetraceScreen(int x, int y, int width, int height);
void teoSDL_GfxRefreshScreen(void);
SDL_Window *teoSDL_GfxWindow(void);
void teo_sdl_display_init(void);
void teoSDL_GfxRetraceWholeScreen(void);


int teoSDL_GfxGetPointer(void);

#endif


