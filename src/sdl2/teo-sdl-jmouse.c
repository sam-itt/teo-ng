#include <SDL.h>
#include <stdbool.h>

#include "teo.h"
#include "defs.h"

#include "media/mouse.h"
#include "sdl2/teo-sdl-mouse.h"
#include "sdl2/sfront-bindings.h"

#define MOUSE_SPEED 1;

#ifdef PLATFORM_OGXBOX
#define AXIS_THRESHOLD SDL_JOYSTICK_AXIS_MAX/2
#else
#define AXIS_THRESHOLD 0
#endif


int jmouse_vertical_axis = JMOUSE_VERTICAL_AXIS;
bool jmouse_inverted_va = JMOUSE_INVERTED_VA;
int jmouse_horizontal_axis = JMOUSE_HORIZONTAL_AXIS;
bool jmouse_inverted_ha = JMOUSE_INVERTED_HA;
int jmouse_button_left = JMOUSE_BUTTON_LEFT;
int jmouse_button_right = JMOUSE_BUTTON_RIGHT;

static Sint8 jmouse_vx = 0;
static Sint8 jmouse_vy = 0;




void teoSDL_JMouseAccelerate(SDL_JoyAxisEvent *event)
{
/*    printf("Joystick %d axis %d value changed to: %d\n", 
           event->which, 
           event->axis,
           event->value);*/
    Sint16 factor;

    if(abs(event->value) < AXIS_THRESHOLD){
        jmouse_vx = 0;
        jmouse_vy = 0;
        return;
    }

    /* Analogue axis go from -32768 to 32767. 
     * i.e for a horizontal axis:
     * -32768 -> 0: left
     *  0: center
     *  0-32767: right
     * is the stick is pushed past halfway in a
     * direction we apply a speedup of twice
     * */
#if PLATFORM_OGXBOX
    factor = 1;
#else
    factor = (abs(event->value) < 16383) ? 1 : 2;
#endif

    if(event->axis == jmouse_vertical_axis){
        event->value *= (jmouse_inverted_va) ? -1 : 1;
        if(event->value < 0){
            jmouse_vy = -1 * factor*MOUSE_SPEED
        }else if(event->value > 0){
            jmouse_vy = factor*MOUSE_SPEED
        }else{
            jmouse_vy = 0;
        }
    }else if(event->axis == jmouse_horizontal_axis){
        event->value *= (jmouse_inverted_ha) ? -1 : 1;
        if(event->value < 0){
            jmouse_vx = -1 * factor*MOUSE_SPEED
        }else if(event->value > 0){
            jmouse_vx = factor*MOUSE_SPEED
        }else{
            jmouse_vx = 0;
        }
    }

}


void teoSDL_JMouseMove(void)
{
    int cx,cy;

    if(jmouse_vx == 0 && jmouse_vy == 0)
        return;
    
    mouse_GetPosition(&cx, &cy);
    cx += jmouse_vx;
    cy += jmouse_vy;

    mouse_Motion(cx, cy);
}

void teoSDL_JMouseButton(SDL_JoyButtonEvent *event)
{
    SDL_MouseButtonEvent mb_event;
    Uint32 type;

    type = (event->state == SDL_PRESSED) ? 
           SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;

    mb_event = (SDL_MouseButtonEvent) {
        type,
        event->timestamp,
        0, /*windowID*/
        0, /*which mouse*/
        0, /*button*/
        event->state, /*state*/
        1, /*clicks*/
        0, /*x*/
        0, /*y*/
    };

    if(event->button == jmouse_button_left){
        mb_event.button = SDL_BUTTON_LEFT;
    }else if(event->button == jmouse_button_right){
        mb_event.button = SDL_BUTTON_RIGHT;
    }

    teoSDL_MouseButton(&mb_event);
}

