#ifndef TEO_VKBD_H
#define TEO_VKBD_H
#include <SDL.h>

void teoSDL_VKbdSetWindow(SDL_Window *win);
void teoSDL_VKbdSetScreen(SDL_Surface *screenSurface);
void teoSDL_VKbdInit(void);
void teoSDL_VKbdShutdown(void);

void teoSDL_VKbdProcessEvent(SDL_Event *event);
void teoSDL_VKbdUpdate(void);

//int teoSDL_VKbdGetPressedKey(void);
#endif
