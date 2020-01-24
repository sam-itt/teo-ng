#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "std.h"
#include "teo.h"
#include "defs.h"
#include "media/disk.h"
#include "sdl2/teo-sdl-log.h"
#include "sdl2/teo-sdl-jmouse.h"
#include "sdl2/sfront.h"
#include "sdl2/teo-sdl-vkbd.h"

bool sfront_show_vkbd = false;

/*These two globals are used by the GUI adapted from Hatari*/
bool bQuitProgram = false;
bool bInFullScreen = false;

static void sfront_RunTO8(void);
static void sfront_ExecutePendingCommand(void);
static int sfront_EventHandler(void);

unsigned short int sfront_features = FRONT_NONE;
Uint8 sfront_windowed_mode = TRUE;

int sfront_Init(int *j_support, unsigned char mode)
{
    int rv;
    char *cfg_file;

    sfront_features = mode;
    
    /*TODO: Split me up, see how hatari inits sound*/
    //rv = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK|SDL_INIT_AUDIO);
    rv = SDL_Init(0);
    if(rv != 0){
        return rv;
    }
    printf("SDL init ok\n");
    teoSDL_KeyboardInit();
    cfg_file = std_GetFirstExistingConfigFile("sdl-keymap.ini");
    if(cfg_file){
        teoSDL_KeyboardLoadKeybindings(cfg_file);
        std_free(cfg_file);
    }else{
        printf("Keymap %s not found !\n","sdl-keymap.ini");
    }

    SDLGui_Init();
    teoSDL_VKbdInit();

    if (*j_support >= 0 && (sfront_features & FRONT_JOYSTICK))
        *j_support = teoSDL_JoystickInit();

    return rv;
}

int sfront_startGfx(int windowed_mode, char *w_title)
{
    SDL_Window *w;

    /* Init the SDL's video subsystem: */
    if(!SDL_WasInit(SDL_INIT_VIDEO)){
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0){
            Log_Printf(LOG_WARN, "Could not init video: %s\n", SDL_GetError());
            return -1;
        }
    }

    sfront_windowed_mode = windowed_mode;
    w = teoSDL_GfxWindow(sfront_windowed_mode, w_title); /* Création de la fenêtre principale */
    if(!w) return -1;
    teoSDL_GfxInit();    /* Binds teo_ graphic callbacks to teoSDL functions*/

    /* Initialise le son */
    if(sfront_features & FRONT_SOUND)
        teoSDL_SoundInit(51200);

    return 0;
}

/**
 * Run the TO8 and execute commands from the GUI.
 * Returns the user wants to quit.
 */
void sfront_Run(void)
{

    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);

    teoSDL_GfxRetraceWholeScreen();

    do{  /* emulator main loop */
        teo.command=TEO_COMMAND_NONE;

        /* sfront_RunTO8 only returns (and thus pause emulation) 
         * when a command is pending
         * */
        sfront_RunTO8();
        
        sfront_ExecutePendingCommand();
#ifdef ENABLE_GTK_PANEL
        gtk_main_iteration_do(FALSE);
#endif 
    }while (teo.command != TEO_COMMAND_QUIT);  /* fin de la boucle principale */
    printf("%s: GOT TEO_COMMAND_QUIT\n", __FUNCTION__);
    /* Finit d'exécuter l'instruction et/ou l'interruption courante */
    mc6809_FlushExec();
}

void sfront_Shutdown()
{


}


static void sfront_RunTO8()
{
    Uint32 last_frame;
    bool sdl_timer = true;

    if(!SDL_WasInit(SDL_INIT_TIMER)){
        if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0){
            Log_Printf(LOG_WARN, "Could not init timers: %s\n", SDL_GetError());
            sdl_timer = false;
//            return false;
        }
    }

    do{  /* Virtual TO8 running loop */
        last_frame = SDL_GetTicks(); 
        if(teo.setting.exact_speed && !teo.setting.sound_enabled)
            last_frame = SDL_GetTicks(); 

        if (teo_DoFrame() == 0)
            if(sfront_windowed_mode)
                teo.command=TEO_COMMAND_BREAKPOINT;

        /* rafraîchissement de l'écran */
        teoSDL_GfxRefreshScreen();
        sfront_EventHandler();
        if(sfront_enabled(FRONT_JMOUSE))
            teoSDL_JMouseMove();

        /* synchronisation sur fréquence réelle */
        if (teo.setting.exact_speed)
        {
            if (teo.setting.sound_enabled){
                if((sfront_features & FRONT_SOUND)){
                    teoSDL_SoundPlay();
                }
            }
            Uint32 dt,frame_duration; /*milliseconds*/

            /*TEO_MICROSECONDS_PER_FRAME = 20.000 */
            dt = SDL_GetTicks() - last_frame;
            frame_duration = USEC_TO_MSEC(TEO_MICROSECONDS_PER_FRAME);
            if((SDL_GetTicks() - last_frame) < frame_duration)
                SDL_Delay(frame_duration-dt); /*Seems to work but a better understanding of all of this timing stuff won't hurt*/
        }

        disk_WriteTimeout();
#ifdef ENABLE_GTK_PANEL
        gtk_main_iteration_do(FALSE);
#endif 
    }while (teo.command==TEO_COMMAND_NONE);  /* End of Virtual TO8 running loop*/
}

/**
 * Run the pending command in teo.command.
 * as the command code is in the global "teo" struct
 * there is no params to pass
 */
static void sfront_ExecutePendingCommand()
{
    /* execute commands */
    if (teo.command==TEO_COMMAND_PANEL)
    {
#ifdef ENABLE_GTK_PANEL
        printf("windowed mode at panel code: ?d\n",sfront_windowed_mode);
        if (sfront_windowed_mode)
            ugui_Panel();
        else
            Dialog_MainDlg(false, 0); /*Volume adjustment diabled and direct access mask set to 0*/
#else
        /*Volume adjustment diabled and direct access mask set to 0*/
        Dialog_MainDlg(false, 0);
#endif
    }

    switch(teo.command){
        case TEO_COMMAND_BREAKPOINT:
        case TEO_COMMAND_DEBUGGER:
#ifdef ENABLE_GTK_DEBUGGER
            if (sfront_windowed_mode) {
                udebug_Panel();
                if (teo_DebugBreakPoint == NULL)
                    teo_FlushFrame();
            }
#endif
            break;
        case TEO_COMMAND_SCREENSHOT:
            printf("Screenshot: NOOP");
            /*TODO: Implement SDL2 screenshots*/
            break;
        case TEO_COMMAND_RESET:
            teo_Reset();
            break;
        case TEO_COMMAND_COLD_RESET:
            teo_ColdReset();
            /*TODO: Set mouse pointer ?*/
//            amouse_Install(TEO_STATUS_MOUSE);
            break;
        case TEO_COMMAND_FULL_RESET:
            teo_FullReset();
//            amouse_Install(TEO_STATUS_MOUSE);
            break;
        case TEO_COMMAND_NONE:
        case TEO_COMMAND_QUIT:
        default:
        break;
    }

}

static int sfront_EventHandler(void)
{
    SDL_Event event;

    while(SDL_PollEvent(&event) == 1){
        if(sfront_show_vkbd)
            teoSDL_VKbdProcessEvent(&event);
        switch(event.type){
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                teoSDL_KeyboardHandler(
                    event.key.keysym.scancode,
                    event.key.keysym.sym,
                    (event.type == SDL_KEYUP) ? 1 : 0
                );
                break;
            case SDL_MOUSEMOTION:
                teoSDL_MouseMove(&(event.motion)); 
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                teoSDL_MouseButton(&(event.button));
                break;
            case SDL_JOYAXISMOTION:
                if(sfront_enabled(FRONT_JMOUSE) && jmouse_use_axis(event.jaxis.axis)){
                    teoSDL_JMouseAccelerate(&(event.jaxis));
                }else{
                    teoSDL_JoystickMove(&(event.jaxis));
                }
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if(sfront_enabled(FRONT_JMOUSE) && jmouse_use_button(event.jbutton.button)){
                    teoSDL_JMouseButton(&(event.jbutton));
                }else{
                    teoSDL_JoystickButton(&(event.jbutton));
                }
                if(event.jbutton.button == 9)
                    teo.command = TEO_COMMAND_PANEL;
                if(event.jbutton.button == 11 && event.jbutton.state == SDL_PRESSED){
                    sfront_show_vkbd = !sfront_show_vkbd;
                    printf("sfront_show_vkbd: %d\n",sfront_show_vkbd);
                    if(!sfront_show_vkbd)
                        teoSDL_GfxRetraceWholeScreen();
                }
                break;
            case SDL_WINDOWEVENT:
                if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
/*                    printf("Window %d size changed to %dx%d\n",
                        event.window.windowID, event.window.data1,
                        event.window.data2);*/
                    teoSDL_GfxReset();
                }
                break;

        }
    }
    return 1;
}

