#ifndef TEO_JMOUSE_H
#define TEO_JMOUSE_H
#include <SDL.h>

#define jmouse_use_axis(a) ((a) == jmouse_vertical_axis || (a) == jmouse_horizontal_axis)
#define jmouse_use_button(b) ((b) == jmouse_button_left || (b) == jmouse_button_right)

extern int jmouse_vertical_axis;
extern int jmouse_horizontal_axis;
extern int jmouse_button_left;
extern int jmouse_button_right;

void teoSDL_JMouseAccelerate(SDL_JoyAxisEvent *event);
void teoSDL_JMouseButton(SDL_JoyButtonEvent *event);
void teoSDL_JMouseMove(void);
#endif
