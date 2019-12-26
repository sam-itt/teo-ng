#include <stdio.h>
#include <SDL2/SDL.h>

#include "std.h"
#include "teo.h"
#include "sdl2/sdl-keyboard.h"

int teo_sdl_gfx_init(void)
{
    int rv;
    char *cfg_file;
    
    rv = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK);
    if(rv == 0){
        printf("SDL init ok\n");
        cfg_file = std_GetFirstExistingConfigFile("sdl-keymap.ini");
        if(cfg_file){
            teo_sdl_keyboard_load_keybindings(cfg_file);
            std_free(cfg_file);
        }else{
            printf("Keymap %s not found !\n","sdl-keymap.ini");
        }

//        SDL_AddEventWatch((SDL_EventFilter)teo_sdl_keyboard_handler,NULL);
    }
    return rv;
}

