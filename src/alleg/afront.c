#include <stdio.h>

#include "allegro.h"

#include "teo.h"
#include "std.h"
#include "alleg/joyint.h"
#include "alleg/akeyboard.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"


static void afront_RetraceCallback(void);
static void afront_CloseProcedure(void);

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

    akeyboard_init();

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
#ifdef PLATFORM_MSDOS
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
            printf("Autodetected color depth was: %d\n",alleg_depth);
//            alleg_depth = 32;
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
   

    printf("window mode: %d\n", *windowed_mode);
    return 0;
}


/* RetraceCallback:
 *  Fonction callback de retraçage de l'écran après
 *  restauration de l'application.
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

/* close_procedure:
 *  Procédure de fermeture de la fenêtre par le bouton close.
 */
static void afront_CloseProcedure(void)
{
    printf("%s: Sending TEO_COMMAND_QUIT\n", __FUNCTION__);
    teo.command = TEO_COMMAND_QUIT;
}

