#ifndef TEO_SDL_MOUSE_H
#define TEO_SDL_MOUSE_H

#include <SDL.h>

void teoSDL_MouseMove(SDL_MouseMotionEvent *event);
void teoSDL_MouseButton(SDL_MouseButtonEvent *event);
#endif //TEO_SDL_MOUSE_H
