#ifndef TEO_SDL_MOUSE_H
#define TEO_SDL_MOUSE_H

#include <SDL2/SDL.h>

void teo_sdl_mouse_move(SDL_MouseMotionEvent *event);
void teo_sdl_mouse_button(SDL_MouseButtonEvent *event);
#endif //TEO_SDL_MOUSE_H
