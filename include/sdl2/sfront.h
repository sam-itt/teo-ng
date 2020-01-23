#ifndef SDL2_DRV_H
#define SDL2_DRV_H

#include <SDL2/SDL.h>

#include "sdl2/sdl-keyboard.h"
#include "sdl2/teo-sdl-mouse.h"
#include "sdl2/teo-sdl-joystick.h"
#include "sdl2/teo-sdl-sound.h"

#include "sdl2/gui/dialog.h"
#include "sdl2/gui/sdlgui.h"

#include "sdl2/teo-sdl-gfx.h"

#define sfront_enabled(feature) (sfront_features & feature)

extern unsigned short int sfront_features;

int sfront_Init(int *j_support, unsigned char mode);
int sfront_startGfx(int windowed_mode, char *w_title);
void sfront_Run(void);
void sfront_Shutdown();


#endif
