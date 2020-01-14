#include <stdio.h>
#include <math.h>
#include <sys/time.h>


#include "allegro.h"

#include "teo.h"
#include "std.h"
#include "defs.h"
#include "media/disk.h"
#include "alleg/mouse.h"
#include "alleg/joyint.h"
#include "alleg/akeyboard.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "alleg/akeybint.h"


#ifdef ENABLE_GTK_PANEL
#include <gtk/gtk.h>
#include <linux/gui.h>
#endif

//#define USE_ALLEGRO_TICKER 1

static void afront_RetraceCallback(void);
static void afront_CloseProcedure(void);
static void afront_RunTO8(int windowed_mode);
static void afront_ExecutePendingCommand(int windowed_mode);

#if defined (USE_ALLEGRO_TICKER)
static void afront_Timer(void);
#endif

int frame;                  /* compteur de frame vidéo */
static volatile int tick;   /* compteur du timer       */


/* Does the basic initialization work for the Allegro4 frontend
 * @w_title: window title, NULL for none (i.e MS-DOS)
 * @j_support: non-zero to enable joystick support
 * @alconfig_file: filename of a allegro.cfg formatted config
 * file. filename only, not path. Standard dirctories (/etc, Application Data, etc)
 * will be searched for it.
 * @keymap_file: key-value file (ini) with the keyboard binding. Filename only, same
 * behavior than @alconfig_file
 */
int afront_Init(const char *w_title, unsigned char j_support, const char *alconfig_file, const char *keymap_file)
{
    char *cfg_file;

    /* Allegro lib int */
    set_uformat(U_ASCII);  /* Latin-1 accents */
    if(allegro_init() != 0){
        return -1;
    }

    cfg_file = std_GetFirstExistingConfigFile((char *)alconfig_file);
    if(cfg_file){
        set_config_file(cfg_file);
        std_free(cfg_file);
    }else{
        printf("Config file %s not found, using default values\n",alconfig_file);
    }

    cfg_file = std_GetFirstExistingConfigFile((char *)keymap_file);
    if(cfg_file){
        override_config_file(cfg_file);
        std_free(cfg_file);
    }else{
        printf("Keymap %s not found !\n",keymap_file);
    }

    akeybint_Init();

    install_keyboard();
    install_timer();
    if (j_support){
        install_joystick(JOY_TYPE_AUTODETECT);
        /* Cap the number of joysticks to TEO_NJOYSTICKS*/
        ajoyint_Init(MIN(TEO_NJOYSTICKS, num_joysticks));
    }
    if(w_title)
       set_window_title(w_title);

    /* Sound init */
    asound_Init(51200);  /* 51200 Hz car 25600 Hz provoque des irregularites du timer */

    return 0;
}

int afront_startGfx(int gfx_mode, int *windowed_mode, char *version_name)
{
    int alleg_depth;

    /* initialisation du mode graphique */
    switch(gfx_mode)
    {
        case GFX_MODE40:
#if PLATFORM_MSDOS
            if (agfxdrv_Init(GFX_MODE40, 8, GFX_VGA, FALSE))
#else
            if (!agfxdrv_Init(GFX_MODE40, 8, GFX_AUTODETECT_FULLSCREEN, FALSE))
#endif
                return -1;
            *windowed_mode = FALSE;
            break;

        case GFX_MODE80:
            if (!agfxdrv_Init(GFX_MODE80, 8, GFX_AUTODETECT_FULLSCREEN, FALSE))
                return -1;
            *windowed_mode = FALSE;
            break;

        case GFX_TRUECOLOR:
            if (!agfxdrv_Init(GFX_TRUECOLOR, 15, GFX_AUTODETECT_FULLSCREEN, FALSE))
                if (!agfxdrv_Init(GFX_TRUECOLOR, 16, GFX_AUTODETECT_FULLSCREEN, FALSE))
                    if (!agfxdrv_Init(GFX_TRUECOLOR, 24, GFX_AUTODETECT_FULLSCREEN, FALSE))
                        if (!agfxdrv_Init(GFX_TRUECOLOR, 32, GFX_AUTODETECT_FULLSCREEN, FALSE))
                            return -1;
            *windowed_mode = FALSE;
            break;

        case GFX_WINDOW:
            alleg_depth = desktop_color_depth();
            switch (alleg_depth)
            {
                case 8:  /* 8bpp */
                default:
                    return -1;
                    break;

                case 16: /* 15 ou 16bpp */
                    if ( !agfxdrv_Init(GFX_TRUECOLOR, 15, GFX_AUTODETECT_WINDOWED, TRUE) && 
                         !agfxdrv_Init(GFX_TRUECOLOR, 16, GFX_AUTODETECT_WINDOWED, TRUE) )
                        return -1;
                    gfx_mode = GFX_TRUECOLOR;
                    break;
 
                case 24: /* 24bpp */
                case 32: /* 32bpp */
                    if (!agfxdrv_Init(GFX_TRUECOLOR, alleg_depth, GFX_AUTODETECT_WINDOWED, TRUE))
                        return -1;
                    gfx_mode = GFX_TRUECOLOR;
                    break;
            }
            *windowed_mode = TRUE;
            break;
    }

    /* Callback needed to retrace when going in/out fullscreen*/
    set_display_switch_callback(SWITCH_IN, afront_RetraceCallback);

    /* on continue de tourner meme sans focus car sinon la gui se bloque,
     * et le buffer son tourne sur lui meme sans mise a jour et c'est moche. */
    set_display_switch_mode(SWITCH_BACKGROUND);

    agui_Init(version_name, gfx_mode, FALSE);
    if(*windowed_mode)
        set_close_button_callback(afront_CloseProcedure);
   
    return 0;
}

/**
 * Run the TO8 and execute commands from the GUI.
 * Returns the user wants to quit.
 */
void afront_Run(int windowed_mode)
{
    /*Defaults to mouse (vs lightpen)*/
    amouse_Install(TEO_STATUS_MOUSE); 
    RetraceScreen(0, 0, SCREEN_W, SCREEN_H);

    do{  /* Emulator main loop*/
   
        teo.command=TEO_COMMAND_NONE;

        akeybint_Install(); 
        amouse_Install(LAST_POINTER);
       
#if defined (USE_ALLEGRO_TICKER)
        if (teo.setting.exact_speed){
            if (teo.setting.sound_enabled){
                asound_Start();
            }else{
                install_int_ex(afront_Timer, BPS_TO_TIMER(TEO_FRAME_FREQ));
                frame=1;
                tick=frame;
            }
        }
#else
       if(teo.setting.sound_enabled)
           asound_Start();
#endif       
        /* afront_RunTO8 only returns (and thus pause emulation) 
         * when a command is pending
         * */
        afront_RunTO8(windowed_mode);

        /*Remove handlers to avoid sending events to the (paused) Virtual TO8*/
#if defined (USE_ALLEGRO_TICKER)
        if (teo.setting.exact_speed){
            if (teo.setting.sound_enabled)
                asound_Stop();
            else
                remove_int(afront_Timer);
        }
#else
        if (teo.setting.sound_enabled)
            asound_Stop();
#endif
        amouse_ShutDown();
        akeybint_ShutDown();

        afront_ExecutePendingCommand(windowed_mode);
    
#if PLATFORM_UNIX && defined (ENABLE_GTK_PANEL)
        gtk_main_iteration_do(FALSE);
#endif
    }while (teo.command != TEO_COMMAND_QUIT);  /* Emulator main loop */
    /* If something was pending, do it */
    mc6809_FlushExec();
}

void afront_Shutdown(void)
{
    /*Remove the callback *before* shutting down graphics*/
    remove_display_switch_callback(afront_RetraceCallback);
    agui_Free();
    SetGraphicMode(SHUTDOWN);
}


static void afront_RunTO8(int windowed_mode)
{
#if !defined (USE_ALLEGRO_TICKER)
    struct timeval base;
    struct timeval begin;
    struct timeval end;


    gettimeofday(&(base), NULL);
#endif

    do{  /* Virtual TO8 running loop */
#if !defined (USE_ALLEGRO_TICKER)
        gettimeofday(&(begin), NULL);
#endif //USE_ALLEGRO_TICKER

#if PLATFORM_MSDOS
        (void)teo_DoFrame();
#else
        if (teo_DoFrame() == 0)
            if (windowed_mode)
                teo.command=TEO_COMMAND_BREAKPOINT;
#endif
        if (need_palette_refresh)
            RefreshPalette();

        RefreshScreen();

        ajoyint_Update();

#if !defined (USE_ALLEGRO_TICKER)
        if (teo.setting.sound_enabled)
            asound_Play();
#endif

        /* Sync to real hardware freq */
        if (teo.setting.exact_speed){
#if defined (USE_ALLEGRO_TICKER)
            if (teo.setting.sound_enabled){
                asound_Play();
            }else{
#if PLATFORM_MSDOS
                while (frame==tick);
#elif PLATFORM_WIN32
                while (frame==tick) 
                    Sleep(0);
#elif PLATFORM_UNIX
                while (frame==tick)
                    usleep(0);
#endif
            }
#else //USE_ALLEGRO_TICKER
            struct timeval delta;
            suseconds_t udt;
        
            gettimeofday(&end, NULL);

            begin.tv_sec -= base.tv_sec;
            begin.tv_usec -= base.tv_usec;

            end.tv_sec -= base.tv_sec;
            end.tv_usec -= base.tv_usec;

            delta.tv_sec = end.tv_sec - begin.tv_sec;
            delta.tv_usec = end.tv_usec - begin.tv_usec;

            if(delta.tv_sec > 0){
                udt = delta.tv_usec + delta.tv_sec*1000000;
            }else{
                udt = delta.tv_usec;
            }

            if(udt < TEO_MICROSECONDS_PER_FRAME){
                rest(round((TEO_MICROSECONDS_PER_FRAME-udt)/1000.0));
            }
#endif //USE_ALLEGRO_TICKER
        }

        disk_WriteTimeout();
        frame++;
#if PLATFORM_UNIX && defined (ENABLE_GTK_PANEL)
        gtk_main_iteration_do(FALSE);
#endif 

    }while (teo.command==TEO_COMMAND_NONE); //Virtual TO8 run loop
}

/**
 * Run the pending command in teo.command.
 * as the command code is in the global "teo" struct
 * there is no params to pass
 */
static void afront_ExecutePendingCommand(int windowed_mode)
{
    if(teo.command == TEO_COMMAND_PANEL){
#if (PLATFORM_UNIX && defined (ENABLE_GTK_PANEL)) || PLATFORM_WIN32
        if (windowed_mode)
            ugui_Panel();
        else
            agui_Panel();
#else //MSDOS or Unix *without* gtk
        agui_Panel();
#endif

    }

    switch(teo.command){
        case TEO_COMMAND_BREAKPOINT:
#if PLATFORM_MSDOS
            break;
#endif
        case TEO_COMMAND_DEBUGGER:
#if PLATFORM_MSDOS
            remove_keyboard();
            SetGraphicMode(SHUTDOWN);
            ddebug_Run();
            SetGraphicMode(RESTORE);
            install_keyboard();
#elif PLATFORM_WIN32 || (PLATFORM_UNIX && defined (ENABLE_GTK_DEBUGGER))
            if (windowed_mode){
#if PLATFORM_WIN32
                wdebug_Panel();
#else 
                udebug_Panel();
#endif //PLATFORM_WIN32
                    if(teo_DebugBreakPoint == NULL)
                        teo_FlushFrame();
            }
#endif
            break;
#if !PLATFORM_MSDOS
        case TEO_COMMAND_SCREENSHOT:
            agfxdrv_Screenshot();
            break;
#endif
        case TEO_COMMAND_RESET:
            teo_Reset();
            break;
        case TEO_COMMAND_COLD_RESET:
            teo_ColdReset();
            amouse_Install(TEO_STATUS_MOUSE);
            break;
        case TEO_COMMAND_FULL_RESET:
            teo_FullReset();
            amouse_Install(TEO_STATUS_MOUSE);
            break;
        case TEO_COMMAND_NONE:
        case TEO_COMMAND_QUIT:
        default:
        break;
    }
}


/**
 * Registered in Allegro to be called when the window
 * get a redraw event from the window manager(move, minimized, ...)
 *
 */
static void afront_RetraceCallback(void)
{
    acquire_screen();
    /*SCREEN_W and SCREEN_H are defined by Allegro 
     * and might change over time (i.e. going fullscreen)
     * */
    RetraceScreen(0, 0, SCREEN_W, SCREEN_H);
    release_screen();
}

/**
 * Registered in Allegro to be called when the window
 * close button is clicked
 */
static void afront_CloseProcedure(void)
{
    teo.command = TEO_COMMAND_QUIT;
}

#if defined (USE_ALLEGRO_TICKER)
static void afront_Timer(void)
{
    tick++;
}
END_OF_FUNCTION(afront_Timer)
#endif
