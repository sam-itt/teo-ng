#ifndef TEO_SDL_KEYBOARD_H
#define TEO_SDL_KEYBOARD_H

#include <SDL2/SDL.h>



int teo_sdl_event_handler(void);
void teo_sdl_keyboard_handler(SDL_Scancode key, SDL_Keycode ksym, Uint8 released);

void teo_sdl_keyboard_load_keybindings(char *filename);
#endif
