#include <stdio.h>
#include <SDL2/SDL.h>

#include "std.h"
#include "teo.h"
#include "sdl2/sdl-keyboard.h"

int teo_sdl_gfx_init(void)
{
    int rv;
    char *cfg_file;
    
    /*TODO: Split me up, see how hatari inits sound*/
    rv = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK|SDL_INIT_AUDIO);
    if(rv == 0){
        printf("SDL init ok\n");
        cfg_file = std_GetFirstExistingConfigFile("sdl-keymap.ini");
        if(cfg_file){
            teo_sdl_keyboard_load_keybindings(cfg_file);
            std_free(cfg_file);
        }else{
            printf("Keymap %s not found !\n","sdl-keymap.ini");
        }

        SDLGui_Init();
//        SDL_AddEventWatch((SDL_EventFilter)teo_sdl_keyboard_handler,NULL);
    }else{
        printf("Couldn't init SDL, bailing out\n");
    }
    return rv;
}

