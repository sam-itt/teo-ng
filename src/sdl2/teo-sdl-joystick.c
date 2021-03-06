#include "sdl2/teo-sdl-joystick.h"
#include "logsys.h"

#include "teo.h"
#include "defs.h" /*min/max, shoudl maybe add __typeof__ per https://stackoverflow.com/questions/3437404/min-and-max-in-c*/
#include "logsys.h"
#include "media/joystick.h"
#include "sdl2/sfront-bindings.h"
#include "sdl2/sfront.h"

#define SDL_STRICT_JOY_IDX 0

//#define VJOY_VERTICAL_AXIS 1
//#define VJOY_HORIZONTAL_AXIS 0
//#define VJOY_A_BOUND_BTN 0
//#define VJOY_B_BOUND_BTN 1


#define TEO_SDL_JOYSTICK_STATE(state) ((state) == SDL_PRESSED ? TEO_JOYSTICK_FIRE_ON : TEO_JOYSTICK_FIRE_OFF) 
#define AXIS_THRESHOLD SDL_JOYSTICK_AXIS_MAX/2


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
            log_msgf(LOG_WARNING, "Could not init joysticks: %s\n", SDL_GetError());
            return 0; 
        }
#ifdef PLATFORM_OGXBOX
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
#endif
    }

    njoys = SDL_NumJoysticks();
    usable = MIN(TEO_NJOYSTICKS, njoys);
    log_msgf(LOG_TRACE,"%d joysticks detected, using first %d\n", njoys, usable);
    njoys = usable;
   
    joys = (SDL_Joystick **)malloc(sizeof(SDL_Joystick *)*njoys);

    for(int i = 0; i < njoys; i++){
        joys[i] = SDL_JoystickOpen(i);
        log_msgf(LOG_TRACE,"Opened joystick %d: %s\n", i, SDL_JoystickName(joys[i]));
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

void teoSDL_JoystickHandler(SDL_Event *generic_event)
{

    switch(generic_event->type){
        case SDL_JOYAXISMOTION:
            teoSDL_JoystickMove(&(generic_event->jaxis));
            break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            teoSDL_JoystickButton(&(generic_event->jbutton));
            break;
        case SDL_JOYHATMOTION:
            if(!sfront_show_vkbd)
                teoSDL_JoystickHatMove(&(generic_event->jhat));
            break;
    }
}


void teoSDL_JoystickMove(SDL_JoyAxisEvent *event)
{
    int jdix;
    int pos;

/*    log_msgf(LOG_TRACE,"Joystick %d axis %d value changed to: %d\n", 
           event->which, 
           event->axis,
           event->value);*/


#if !SDL_STRICT_JOY_IDX
    jdix = event->which;
#else
    jdix = teoSDL_GetJoystickIdx(event->which);
#endif
    if(jdix < 0) return;

    if(abs(event->value) < AXIS_THRESHOLD)
        return;

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

void teoSDL_JoystickHatMove(SDL_JoyHatEvent *event)
{
    int jdix;
    int pos;

#if !SDL_STRICT_JOY_IDX
    jdix = event->which;
#else
    jdix = teoSDL_GetJoystickIdx(event->which);
#endif
    if(jdix < 0) return;

    switch(event->value){
        case SDL_HAT_LEFTUP:
            pos = TEO_JOYSTICK_LEFT | TEO_JOYSTICK_UP;
            break;
        case SDL_HAT_LEFT:
            pos = TEO_JOYSTICK_LEFT;
            break;
        case SDL_HAT_LEFTDOWN:
            pos = TEO_JOYSTICK_LEFT | TEO_JOYSTICK_DOWN;
            break;
        /*Up and down are swapped on purpose to get proper
         * in-game movment.
         * TODO: Check against "hardware" behavior. The TO8
         * manual reads about NORTH/SOUTH/ etc directions.
         * Check how that's implemented in TEO
         * */
        case SDL_HAT_UP:
            pos = TEO_JOYSTICK_DOWN;
            break;
        case SDL_HAT_DOWN:
            pos = TEO_JOYSTICK_UP;
            break;
        case SDL_HAT_RIGHTUP:
            pos = TEO_JOYSTICK_RIGHT | TEO_JOYSTICK_UP;
            break;
        case SDL_HAT_RIGHT:
            pos = TEO_JOYSTICK_RIGHT;
            break;
        case SDL_HAT_RIGHTDOWN:
            pos = TEO_JOYSTICK_RIGHT | TEO_JOYSTICK_DOWN;
            break;
        case SDL_HAT_CENTERED:
        default:
            pos = TEO_JOYSTICK_CENTER;
            break;
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

//    log_msgf(LOG_TRACE,"Joystick %d button %d pressed\n", event->which, event->button);
    if(event->button == VJOY_A_BOUND_BTN)
        joystick_Button(jdix, 0, TEO_SDL_JOYSTICK_STATE(event->state));
    if(event->button == VJOY_B_BOUND_BTN)
        joystick_Button(jdix, 1, TEO_SDL_JOYSTICK_STATE(event->state));
}

