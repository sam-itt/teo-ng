#include <stdio.h>
#include <stdbool.h>

#include "teo.h"
#include "media/mouse.h"
#include "sdl2/teo-sdl-mouse.h"
#include "sdl2/teo-sdl-gfx.h"

#define is_in_window(x,y) (((x) > (TEO_BORDER_W*2)) && ((y) > (TEO_BORDER_H*2)))
#define clip_x(x) ((x)/2-TEO_BORDER_W)
#define clip_y(y) ((y)/2-TEO_BORDER_H)

static Uint8 teoSDL_MouseButtonBound(Uint8 button)
{
    if(button == SDL_BUTTON_LEFT)
        return 1;
    if(button == SDL_BUTTON_RIGHT)
        return 2;
    return 0;
}

void teoSDL_MouseMove(SDL_MouseMotionEvent *event)
{
    if(is_in_window(event->x, event->y)){
        mouse_Motion ((int)clip_x(event->x),(int)clip_y(event->y));
    }
}

void teoSDL_MouseButton(SDL_MouseButtonEvent *event)
{
    Uint8 teo_btn;

    teo_btn = teoSDL_MouseButtonBound(event->button);
    if(!teo_btn)
        return;

    if(event->state == SDL_RELEASED){
        mouse_Click(teo_btn, true);
        return;
    }
    /*pressed*/
    if(teo_btn == 2 && teoSDL_GfxGetPointer() != TEO_STATUS_MOUSE)
        return;

    mouse_Click(teo_btn, false);
}

