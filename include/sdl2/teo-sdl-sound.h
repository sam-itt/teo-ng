#ifndef TEO_SDL_SOUND_H
#define TEO_SDL_SOUND_H
#include <stdbool.h>

bool teoSDL_SoundInit(int freq);
void teoSDL_SoundPlay(void);
void teoSDL_SoundPause(bool flag);

void teoSDL_SoundClear(void);
void teoSDL_SoundShutdown(void);
#endif //TEO_SDL_SOUND_H
