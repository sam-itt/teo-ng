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
#include "sdl2/sfront-bindings.h"

bool sfront_show_vkbd = false;

/*These two globals are used by the GUI adapted from Hatari*/
bool bQuitProgram = false;
bool bInFullScreen = false;

static void sfront_RunTO8(void);
static void sfront_ExecutePendingCommand(void);
static int sfront_EventHandler(void);
static void sfront_KeyboardEventHandler(SDL_KeyboardEvent *event);
static void sfront_JoystickEventHandler(SDL_Event *generic_event);

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

#ifdef PLATFORM_OGXBOX
void printSDLErrorAndReboot(void)
{
    debugPrint("SDL_Error: %s\n", SDL_GetError());
    debugPrint("Rebooting in 5 seconds.\n");
    Sleep(5000);
    XReboot();
}
#endif

int sfront_startGfx(int windowed_mode, char *w_title)
{
    SDL_Window *w;

    /* Init the SDL's video subsystem: */
    if(!SDL_WasInit(SDL_INIT_VIDEO)){
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0){
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL video.\n");
#ifdef PLATFORM_OGXBOX
            printSDLErrorAndReboot();
#endif
            return -1;
        }
    }

    sfront_windowed_mode = windowed_mode;
    w = teoSDL_GfxWindow(sfront_windowed_mode, w_title); /* Création de la fenêtre principale */
    if(!w) return -1;
    teoSDL_GfxInit();    /* Binds teo_ graphic callbacks to teoSDL functions*/

    /* Initialise le son */
    if(sfront_features & FRONT_SOUND){
        /*Dare to lower me and I'll start cracking*/
        teoSDL_SoundInit(48000); 
    }else{
        teo_SilenceSound = NULL;
        teo_PutSoundByte = NULL;
    }
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
    teoSDL_SoundShutdown();
}

void sfront_Shutdown()
{


}


static void sfront_RunTO8()
{
    bool sdl_timer = true;

#ifdef PLATFORM_OGXBOX /*TODO: have sdl timer + platform*/
    DWORD last_frame;
#else
    Uint32 last_frame;

    if(!SDL_WasInit(SDL_INIT_TIMER)){
        if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0){
            //debugPrint("Could not init timers: %s\n", SDL_GetError());
            sdl_timer = false;
        }
    }
#endif
    do{  /* Virtual TO8 running loop */
#ifdef PLATFORM_OGXBOX
        last_frame = GetTickCount();
#else
        last_frame = SDL_GetTicks(); 
#endif
        if(teo.setting.exact_speed && !teo.setting.sound_enabled){
#ifdef PLATFORM_OGXBOX
            last_frame = GetTickCount();
#else
            last_frame = SDL_GetTicks();
#endif
        }

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
                    teoSDL_SoundPlay(); /*Also does sync*/
                }
            }else{
#ifdef PLATFORM_OGXBOX
                DWORD dt,frame_duration; /*milliseconds*/
                dt = GetTickCount() - last_frame;
                frame_duration = USEC_TO_MSEC(TEO_MICROSECONDS_PER_FRAME);
                if(dt < frame_duration)
                    Sleep(frame_duration-dt); 
#else
                Uint32 dt,frame_duration; /*milliseconds*/
                dt = SDL_GetTicks() - last_frame;
                frame_duration = USEC_TO_MSEC(TEO_MICROSECONDS_PER_FRAME);
                if(dt < frame_duration)
                    SDL_Delay(frame_duration-dt);
#endif
            }
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
                teo.command = TEO_COMMAND_QUIT;
                break;
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                sfront_KeyboardEventHandler(&(event.key));
                break;
            case SDL_MOUSEMOTION:
                teoSDL_MouseMove(&(event.motion)); 
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                teoSDL_MouseButton(&(event.button));
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYHATMOTION:
                sfront_JoystickEventHandler(&event);
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

/**
 * Gets input from the host joystick and decide what to do with it:
 * - Do "emulators" commands (show panel on press button start,
 *   jmouse, etc).
 * - Translate parts of these events to movements of the TO8
 *   joystick
 */
#define sfront_is_jbutton_event(type) ((type) == SDL_JOYBUTTONDOWN || (type) == SDL_JOYBUTTONUP)
#define sfront_jbutton_pressed(btn, event) ((event)->button == btn && (event)->state == SDL_PRESSED)
static void sfront_JoystickEventHandler(SDL_Event *generic_event)
{

    /*Emulator bindings of joystick actions*/
    if(generic_event->type == SDL_JOYAXISMOTION){
        SDL_JoyAxisEvent *event;

        event = &(generic_event->jaxis);
        if(sfront_enabled(FRONT_JMOUSE) && jmouse_use_axis(event->axis)){
            teoSDL_JMouseAccelerate(event);
            return;
        }
    }else if(sfront_is_jbutton_event(generic_event->type)){
        SDL_JoyButtonEvent *event;

        event = &(generic_event->jbutton);
        if(sfront_enabled(FRONT_JMOUSE) && jmouse_use_button(event->button)){
            teoSDL_JMouseButton(event);
            return;
        }else if(sfront_jbutton_pressed(PANEL_TOGGLE_BUTTON, event)){
            teo.command = TEO_COMMAND_PANEL;
            return;
        }else if(sfront_jbutton_pressed(VKB_TOGGLE_BTN, event)){
            sfront_show_vkbd = !sfront_show_vkbd;
            if(!sfront_show_vkbd)
                teoSDL_GfxRetraceWholeScreen();
            teoSDL_VKbdNotifyVisibility(sfront_show_vkbd);
            return;
        }
    }
    /* If we reach this part the action hasn't been bound to something
     * in the emualator and can be safely forwarded to the virtual
     * TO8 translation layer
     * */
    teoSDL_JoystickHandler(generic_event);
}


/**
 * Gets all the input from the host keyboard. It's job is to handle/dispatch:
 * a) Emulators commands (open the config panel take a screenshot, etc.)
 * b) What must be sent to the translation layer which will decide which
 * TOKEY_ must be sent to the virutal TO8
 */
static void sfront_KeyboardEventHandler(SDL_KeyboardEvent *event)
{
    bool release;
    SDL_Keycode ksym;

    ksym = event->keysym.sym;
    release = (event->state == SDL_RELEASED) ? true : false;

    /*Handling keyboard-triggered emulator functions*/
    if(ksym == SDLK_ESCAPE && !release){
        teo.command=TEO_COMMAND_PANEL;
        return;
    }

    if(ksym == SDLK_F11 && !release){
        teo.command=TEO_COMMAND_SCREENSHOT;
        return;
    }

    if(ksym == SDLK_F12 && !release){
        teo.command=TEO_COMMAND_DEBUGGER;
        return;
    }

    if(ksym == SDLK_RETURN && !release){ //Toggle fullscreen
        SDL_Keymod mod;

        mod = SDL_GetModState();
        if(mod & KMOD_LALT){
            if(sfront_windowed_mode)
                SDL_SetWindowFullscreen(teoSDL_GfxGetWindow(), SDL_WINDOW_FULLSCREEN_DESKTOP);
            else
                SDL_SetWindowFullscreen(teoSDL_GfxGetWindow(), 0);
            sfront_windowed_mode = !sfront_windowed_mode;
            teoSDL_GfxReset();
        }
    }

    /* Only send keystrockes for translation into TOKEYS_ when:
     * -It's not restrictively bound to a emulator feature
     * -The virtual keyboard is not active. When the virtual
     *  keyboard is active, the host keyboard only controls emulator
     *  features.
     * */
    if(!sfront_show_vkbd)
        teoSDL_KeyboardHandler(event);
}
