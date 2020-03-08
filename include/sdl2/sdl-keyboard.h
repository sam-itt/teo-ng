#ifndef TEO_SDL_KEYBOARD_H
#define TEO_SDL_KEYBOARD_H

#include <SDL.h>


void teoSDL_KeyboardInit(void);

void teoSDL_KeyboardHandler(SDL_KeyboardEvent *event);

void teoSDL_KeyboardLoadKeybindings(char *filename);
#endif
