#include <stdio.h>
#include <stdbool.h>

#include "teo.h"
#include "media/mouse.h"
#include "sdl2/teo-sdl-mouse.h"
#include "sdl2/teo-sdl.h"

#define is_in_window(x,y) (((x) > (TEO_BORDER_W*2)) && ((y) > (TEO_BORDER_H*2)))
#define clip_x(x) ((x)/2-TEO_BORDER_W)
#define clip_y(y) ((y)/2-TEO_BORDER_H)

static Uint8 teo_sdl_button_bound(Uint8 button)
{
    if(button == SDL_BUTTON_LEFT)
        return 1;
    if(button == SDL_BUTTON_RIGHT)
        return 2;
    return 0;
}

void teo_sdl_mouse_move(SDL_MouseMotionEvent *event)
{
    if(is_in_window(event->x, event->y)){
        mouse_Motion ((int)clip_x(event->x),(int)clip_y(event->y));
    }
}

void teo_sdl_mouse_button(SDL_MouseButtonEvent *event)
{
    Uint8 teo_btn;

    teo_btn = teo_sdl_button_bound(event->button);
    if(!teo_btn)
        return;

    if(event->state == SDL_RELEASED){
        mouse_Click(teo_btn, true);
        return;
    }
    /*pressed*/
    if(teo_btn == 2 && teo_sdl_getPointer() != TEO_STATUS_MOUSE)
        return;

    mouse_Click(teo_btn, false);
}

