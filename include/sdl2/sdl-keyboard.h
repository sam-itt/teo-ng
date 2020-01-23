#ifndef TEO_SDL_KEYBOARD_H
#define TEO_SDL_KEYBOARD_H

#include <SDL.h>


void teoSDL_KeyboardInit(void);

void teoSDL_KeyboardHandler(SDL_Scancode key, SDL_Keycode ksym, Uint8 released);

void teoSDL_KeyboardLoadKeybindings(char *filename);
#endif
