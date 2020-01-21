#ifndef SDL2_DRV_H
#define SDL2_DRV_H

#include <SDL.h>

#include "sdl2/sdl-keyboard.h"
#include "sdl2/teo-sdl-mouse.h"
#include "sdl2/teo-sdl-joystick.h"
#include "sdl2/teo-sdl-sound.h"

#include "sdl2/gui/dialog.h"
#include "sdl2/gui/sdlgui.h"

#include "sdl2/teo-sdl-gfx.h"

int sfront_Init(int *j_support);
int sfront_startGfx(int *windowed_mode, char *w_title);
void sfront_Run(int windowed_mode);
void sfront_Shutdown();


#endif
