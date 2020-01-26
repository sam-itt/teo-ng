#include "sdl2/teo-sdl-joystick.h"
#include "sdl2/teo-sdl-log.h"

#include "teo.h"
#include "defs.h" /*min/max, shoudl maybe add __typeof__ per https://stackoverflow.com/questions/3437404/min-and-max-in-c*/
#include "media/joystick.h"
#include "sdl2/sfront-bindings.h"

#define SDL_STRICT_JOY_IDX 0

//#define VJOY_VERTICAL_AXIS 1
//#define VJOY_HORIZONTAL_AXIS 0
//#define VJOY_A_BOUND_BTN 0
//#define VJOY_B_BOUND_BTN 1


#define TEO_SDL_JOYSTICK_STATE(state) ((state) == SDL_PRESSED ? TEO_JOYSTICK_FIRE_ON : TEO_JOYSTICK_FIRE_OFF) 

static SDL_Joystick **joys = NULL;
static int njoys = 0;

static int teoSDL_GetJoystickIdx(SDL_JoystickID j_id)
{
    for(int i = 0; i < njoys; i++){
        if(SDL_JoystickInstanceID(joys[i]) == j_id)
            return i;
    }
    return -1;
}


/**
 * Return the number of detected (real) joysticks
 *
 */
int teoSDL_JoystickInit(void)
{
    int usable;
   
    if(!SDL_WasInit(SDL_INIT_JOYSTICK)){
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0){
            Log_Printf(LOG_WARN, "Could not init joysticks: %s\n", SDL_GetError());
            return 0; 
        }
    }

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

void teoSDL_JoystickShutdown(void)
{
    if(!joys) return;

    for(int i = 0; i < njoys; i++){
        if(SDL_JoystickGetAttached(joys[i]))
            SDL_JoystickClose(joys[i]);
    }
    free(joys);

}

void teoSDL_JoystickMove(SDL_JoyAxisEvent *event)
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
    jdix = teoSDL_GetJoystickIdx(event->which);
#endif
    if(jdix < 0) return;

    /*Axis 1: Up(negative) / Down(positive)*/
    /*Axis 0: Left(negative) / Right (positive)*/

    pos = TEO_JOYSTICK_CENTER;
    if(event->axis == VJOY_VERTICAL_AXIS){
        if(event->value < 0)
            pos |= TEO_JOYSTICK_UP;
        if(event->value > 0)
            pos |= TEO_JOYSTICK_DOWN;
        
    }

    if(event->axis == VJOY_HORIZONTAL_AXIS){
        if(event->value < 0)
            pos |= TEO_JOYSTICK_LEFT;
        if(event->value > 0)
            pos |= TEO_JOYSTICK_RIGHT;
    }

    joystick_Move(jdix, pos);
}

void teoSDL_JoystickButton(SDL_JoyButtonEvent *event)
{
    int jdix;

#if !SDL_STRICT_JOY_IDX
    jdix = event->which;
#else
    jdix = teoSDL_GetJoystickIdx(event->which);
#endif

//    printf("Joystick %d button %d pressed\n", event->which, event->button);
    if(event->button == VJOY_A_BOUND_BTN)
        joystick_Button(jdix, 0, TEO_SDL_JOYSTICK_STATE(event->state));
    if(event->button == VJOY_B_BOUND_BTN)
        joystick_Button(jdix, 1, TEO_SDL_JOYSTICK_STATE(event->state));
}

