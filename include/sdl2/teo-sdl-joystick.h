#ifndef TEO_SDL_JOYSTICK_H
#define TEO_SDL_JOYSTICK_H
#include <SDL.h>

int teoSDL_JoystickInit(void);
void teoSDL_JoystickShutdown(void);
void teoSDL_JoystickHandler(SDL_Event *generic_event);

void teoSDL_JoystickMove(SDL_JoyAxisEvent *event);
void teoSDL_JoystickButton(SDL_JoyButtonEvent *event);
void teoSDL_JoystickHatMove(SDL_JoyHatEvent *event);


#endif //TEO_SDL_JOYSTICK_H
