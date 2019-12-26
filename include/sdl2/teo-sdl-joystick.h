#ifndef TEO_SDL_JOYSTICK_H
#define TEO_SDL_JOYSTICK_H
#include <SDL2/SDL.h>

int teo_sdl_joystick_init(void);
void teo_sdl_joystick_terminate(void);

void teo_sdl_joytick_move(SDL_JoyAxisEvent *event);
void teo_sdl_joystick_button(SDL_JoyButtonEvent *event);


#endif //TEO_SDL_JOYSTICK_H
