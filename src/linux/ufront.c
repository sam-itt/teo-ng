#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>


#include "teo.h"
#include "defs.h"
#include "std.h"
#include "errors.h"
#include "media/disk.h"

#include "linux/display.h"
#include "linux/graphic.h"
#include "linux/sound.h"
#include "linux/gui.h"


static GTimer *timer;

static gboolean ufront_RunTO8 (gpointer user_data);

int ufront_Init(void)
{
    g_setenv ("GDK_BACKEND", "x11", TRUE);

    return TRUE;
}

int ufront_StartGfx(const char *keymap_file)
{
    int rv;
    char *cfg_file;

    rv = 0;
    /*Creates the GTK window*/
    udisplay_Window();


    /* Get pure X handles from GTK and initialize SHM if available
     * Also loads the keymap. Todo: Move filename up in the stack ?
     * */
    cfg_file = std_GetFirstExistingConfigFile((char *)keymap_file);
    if(cfg_file){
        udisplay_Init((const char *)cfg_file);
    }else{
        teo_error_msg = std_free(teo_error_msg);
        asprintf(&teo_error_msg, "Keymap %s not found !\n",keymap_file);
        return TEO_ERROR;
    }

    /*Create the palette, initialize graphic buffers and bind teo_ graphic
     * callbacks*/
    ugraphic_Init();
    /* Initialize Alsa structures and connects teo_ sound callbacks */
    rv = usound_Init();

//    /* Initialise l'interface graphique */
//    ugui_Init();
//    udebug_Init();

    std_free(cfg_file);
    return rv;
}


void ufront_Run(void)
{
    int idle_data = 0;

    teo.command=TEO_COMMAND_NONE;
    timer = g_timer_new ();
    g_timeout_add_full (G_PRIORITY_DEFAULT, 1, ufront_RunTO8, &idle_data, NULL);
    gtk_main();
    g_timer_destroy (timer);
}

void ufront_Shutdown(void)
{

    ugui_Free();   /* Libère la mémoire utilisée par la GUI */
    udebug_Free();  /* Free memory used by the debugger */
    usound_Close(); /* Referme le périphérique audio*/

}


/** 
 * Virtual TO8 running function
 */
static gboolean ufront_RunTO8(gpointer user_data)
{
    static gulong microseconds;
    double elapsed;

    if(teo.setting.exact_speed){
        if(!teo.setting.sound_enabled){
            elapsed = g_timer_elapsed(timer, &microseconds); 
            if( elapsed < USEC_TO_SEC(TEO_MICROSECONDS_PER_FRAME)){
                return TRUE;
            }
        }
    }

    g_timer_start (timer);

    if (teo_DoFrame() == 0)
        teo.command=TEO_COMMAND_BREAKPOINT;

    /*As a) the command must be handled during the same call
     * and b) the GUI can emit command
     * thus the panel must be ran before anything (or don't have a switch).
     * */
    if(teo.command == TEO_COMMAND_PANEL)
        ugui_Panel();

    switch(teo.command){
        case TEO_COMMAND_BREAKPOINT:
        case TEO_COMMAND_DEBUGGER:
            udebug_Panel();
            if (teo_DebugBreakPoint == NULL)
                teo_FlushFrame();
            break;
        case TEO_COMMAND_RESET:
            teo_Reset();
            break;
        case TEO_COMMAND_COLD_RESET:
            teo_ColdReset();
            break;
        case TEO_COMMAND_FULL_RESET:
            teo_FullReset();
            break;
        case TEO_COMMAND_QUIT:
            mc6809_FlushExec();
            gtk_main_quit ();
            return FALSE;
        case TEO_COMMAND_NONE:
        case TEO_COMMAND_SCREENSHOT:
        default:
            break;
    }

    teo.command = TEO_COMMAND_NONE;

    ugraphic_Refresh ();
    if ((teo.setting.exact_speed)
     && (teo.setting.sound_enabled))
        usound_Play ();

    disk_WriteTimeout();

    return TRUE;
}

