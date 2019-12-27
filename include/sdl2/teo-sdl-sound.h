#ifndef TEO_SDL_SOUND_H
#define TEO_SDL_SOUND_H
#include <stdbool.h>

bool teo_sdl_sound_init(int freq);
void teo_sdl_sound_play(void);

void teo_sdl_sound_clear(void);
#endif //TEO_SDL_SOUND_H
