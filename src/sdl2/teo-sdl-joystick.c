#include "sdl2/teo-sdl-joystick.h"

#include "teo.h"
#include "defs.h" /*min/max, shoudl maybe add __typeof__ per https://stackoverflow.com/questions/3437404/min-and-max-in-c*/
#include "media/joystick.h"

#define SDL_STRICT_JOY_IDX 0

#define VERTICAL_AXIS 1
#define HORIZONTAL_AXIS 0
#define A_BOUND_BTN 0
#define B_BOUND_BTN 1


#define TEO_SDL_JOYSTICK_STATE(state) ((state) == SDL_PRESSED ? TEO_JOYSTICK_FIRE_ON : TEO_JOYSTICK_FIRE_OFF) 

static SDL_Joystick **joys = NULL;
static int njoys = 0;

static int teo_sdl_get_joystick_idx(SDL_JoystickID j_id)
{
    for(int i = 0; i < njoys; i++){
        if(SDL_JoystickInstanceID(joys[i]) == j_id)
            return i;
    }
    return -1;
}

int teo_sdl_joystick_init(void)
{
    int usable;

    njoys = SDL_NumJoysticks();
    usable = MIN(TEO_NJOYSTICKS, njoys);
    printf("%d joysticks detected, using first %d\n", njoys, usable);
    njoys = usable;
   
    joys = (SDL_Joystick **)malloc(sizeof(SDL_Joystick *)*njoys);

    for(int i = 0; i < njoys; i++){
        joys[i] = SDL_JoystickOpen(i);
        printf("Opened joystick %d: %s\n", i, SDL_JoystickName(joys[i]));
    }

    return njoys;
}

void teo_sdl_joystick_terminate(void)
{
    if(!joys) return;

    for(int i = 0; i < njoys; i++){
        if(SDL_JoystickGetAttached(joys[i]))
            SDL_JoystickClose(joys[i]);
    }
    free(joys);

}

void teo_sdl_joytick_move(SDL_JoyAxisEvent *event)
{
    int jdix;
    int pos;

/*    printf("Joystick %d axis %d value changed to: %d\n", 
           event->which, 
           event->axis,
           event->value);*/


#if !SDL_STRICT_JOY_IDX
    jdix = event->which;
#else
    jdix = teo_sdl_get_joystick_idx(event->which);
#endif
    if(jdix < 0) return;

    /*Axis 1: Up(negative) / Down(positive)*/
    /*Axis 0: Left(negative) / Right (positive)*/

    pos = TEO_JOYSTICK_CENTER;
    if(event->axis == VERTICAL_AXIS){
        if(event->value < 0)
            pos |= TEO_JOYSTICK_UP;
        if(event->value > 0)
            pos |= TEO_JOYSTICK_DOWN;
        
    }

    if(event->axis == HORIZONTAL_AXIS){
        if(event->value < 0)
            pos |= TEO_JOYSTICK_LEFT;
        if(event->value > 0)
            pos |= TEO_JOYSTICK_RIGHT;
    }

    joystick_Move(jdix, pos);
}

void teo_sdl_joystick_button(SDL_JoyButtonEvent *event)
{
    int jdix;

#if !SDL_STRICT_JOY_IDX
    jdix = event->which;
#else
    jdix = teo_sdl_get_joystick_idx(event->which);
#endif

//    printf("Joystick %d button %d pressed\n", event->which, event->button);
    if(event->button == A_BOUND_BTN)
        joystick_Button(jdix, 0, TEO_SDL_JOYSTICK_STATE(event->state));
    if(event->button == B_BOUND_BTN)
        joystick_Button(jdix, 1, TEO_SDL_JOYSTICK_STATE(event->state));
}

