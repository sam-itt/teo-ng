#ifndef TEO_SDL_GFX40_H
#define TEO_SDL_GFX40_H

#include <SDL.h>

void        teoSDL_Gfx40Init(void);
void        teoSDL_Gfx40RetraceScreen(int x, int y, int width, int height);
void        teoSDL_Gfx40RefreshScreen(void);
SDL_Window *teoSDL_Gfx40Window(int windowed_mode, const char *w_title);
void        teoSDL_Gfx40RetraceWholeScreen(void);

int         teoSDL_Gfx40GetPointer(void);
SDL_Window *teoSDL_Gfx40GetWindow(void);
void        teoSDL_Gfx40Reset(void);

#endif


