

#ifndef TEO_SDL_H
#define TEO_SDL_H

void teo_sdl_graphic_init(void);
void teo_sdl_RetraceScreen(int x, int y, int width, int height);
void teo_sdl_RefreshScreen(void);
void teo_sdl_window(void);
void teo_sdl_display_init(void);
void teo_sdl_RetraceWholeScreen(void);


int teo_sdl_getPointer(void);

#endif


