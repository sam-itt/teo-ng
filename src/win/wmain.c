/*
 *    TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *    TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EEEEEEEEEE      OO          OO
 *          TT        EEEEEEEEEE      OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *          TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *
 *                  L'émulateur Thomson TO8
 *
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, Samuel Devulder
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  Module     : win/main.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou septembre 2000
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Samuel Devulder 30/07/2011
 *               François Mouret 19/10/2012 24/10/2012 19/09/2013 10/05/2014
 *                               31/07/2016 25/10/2018
 *
 *  Boucle principale de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <sys/stat.h>
   #include <ctype.h>
   #include <allegro.h>
   #include <winalleg.h>
#endif

#include "defs.h"
#include "teo.h"
#include "option.h"
#include "std.h"
#include "ini.h"
#include "image.h"
#include "main.h"
#include "errors.h"
#include "media/disk.h"
#include "media/cass.h"
#include "media/memo.h"
#include "media/printer.h"
#include "mc68xx/mc6809.h"
/* Windows includes (see win/gui.h for supported version) */
#include "win/gui.h"
#include "win/keybint.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "alleg/joyint.h"
#include "alleg/mouse.h"
#include "alleg/sound.h"


struct EMUTEO teo;

static int reset = FALSE;
static int gfx_mode = GFX_WINDOW;
static int windowed_mode = TRUE;
struct STRING_LIST *remain_name = NULL;

int frame;                  /* compteur de frame vidéo */
static volatile int tick;   /* compteur du timer       */

static void Timer(void)
{
    tick++;
}



/* RetraceCallback:
 *  Fonction callback de retraçage de l'écran après
 *  restauration de l'application.
 */
static void RetraceCallback(void)
{
    acquire_screen();
    RetraceScreen(0, 0, SCREEN_W, SCREEN_H);
    release_screen();
}



/* RunTO8:
 *  Boucle principale de l'émulateur.
 */
static void RunTO8(void)
{
    amouse_Install(TEO_STATUS_MOUSE); /* la souris est le périphérique de pointage par défaut */
    RetraceScreen(0, 0, SCREEN_W, SCREEN_H);

    do  /* boucle principale de l'émulateur */
    {
        teo.command=TEO_COMMAND_NONE;

        /* installation des handlers clavier, souris et son */ 
        wkeybint_Install();
        amouse_Install(LAST_POINTER);

        if (teo.setting.exact_speed)
        {
            if (teo.setting.sound_enabled)
                asound_Start();
            else
            {
                install_int_ex(Timer, BPS_TO_TIMER(TEO_FRAME_FREQ));
                frame=1;
                tick=frame;
            }
        }

        do  /* boucle d'émulation */
        {
            if (teo_DoFrame() == 0)
                if (windowed_mode)
                    teo.command=TEO_COMMAND_BREAKPOINT;

            /* rafraîchissement de la palette */
            if (need_palette_refresh)
                RefreshPalette();

            /* rafraîchissement de l'écran */
            RefreshScreen();

            /* mise à jour de la position des joysticks */
            ajoyint_Update();

            /* synchronisation sur fréquence réelle */
            if (teo.setting.exact_speed)
            {
                if (teo.setting.sound_enabled)
                    asound_Play();
                else
                    while (frame==tick)
                   Sleep(0);
            }

            disk_WriteTimeout();
            frame++;
        }
        while (teo.command==TEO_COMMAND_NONE);  /* fin de la boucle d'émulation */

        /* désinstallation des handlers clavier, souris et son */
        if (teo.setting.exact_speed)
        {
            if (teo.setting.sound_enabled)
                asound_Stop();
            else
                remove_int(Timer);
        }
        amouse_ShutDown();
        wkeybint_ShutDown();

        /* éxécution des commandes */
        if (teo.command==TEO_COMMAND_PANEL)
        {
            if (windowed_mode)
                wgui_Panel();
            else
                agui_Panel();
        }

        if ((teo.command == TEO_COMMAND_BREAKPOINT)
         || (teo.command == TEO_COMMAND_DEBUGGER)) {
            if (windowed_mode) {
                wdebug_Panel ();
                if (teo_DebugBreakPoint == NULL)
                    teo_FlushFrame();
            }
        }

        if (teo.command==TEO_COMMAND_SCREENSHOT)
            agfxdrv_Screenshot();

        if (teo.command==TEO_COMMAND_RESET)
            teo_Reset();

        if (teo.command==TEO_COMMAND_COLD_RESET)
        {
            teo_ColdReset();
            amouse_Install(TEO_STATUS_MOUSE);
        }

        if (teo.command==TEO_COMMAND_FULL_RESET)
        {
            teo_FullReset();
            amouse_Install(TEO_STATUS_MOUSE);
        }
    }
    while (teo.command != TEO_COMMAND_QUIT);  /* fin de la boucle principale */

    /* Finit d'exécuter l'instruction et/ou l'interruption courante */
    mc6809_FlushExec();
}



/* read_command_line:
 *  Lit la ligne de commande
 */
static void read_command_line(int argc, char *argv[])
{
    char *message;
    int mode40=0, mode80=0, truecolor=0, windowd=0;

    struct OPTION_ENTRY entries[] = {
        { "reset", 'r', OPTION_ARG_BOOL, &reset,
           is_fr?"Reset … froid de l'‚mulateur"
                :"Cold-reset emulator", NULL },
        { "disk0", '0', OPTION_ARG_FILENAME, &teo.disk[0].file,
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)",
           is_fr?"FICHIER":"FILE" },
        { "disk1", '1', OPTION_ARG_FILENAME, &teo.disk[1].file,
           is_fr?"Charge un disque virtuel (lecteur 1)"
                :"Load virtual disk (drive 1)",
           is_fr?"FICHIER":"FILE" },
        { "disk2", '2', OPTION_ARG_FILENAME, &teo.disk[2].file,
           is_fr?"Charge un disque virtuel (lecteur 2)"
                :"Load virtual disk (drive 2)",
           is_fr?"FICHIER":"FILE" },
        { "disk3", '3', OPTION_ARG_FILENAME, &teo.disk[3].file,
           is_fr?"Charge un disque virtuel (lecteur 3)"
                :"Load virtual disk (drive 3)",
           is_fr?"FICHIER":"FILE" },
        { "cass", '\0', OPTION_ARG_FILENAME, &teo.cass.file,
           is_fr?"Charge une cassette":"Load a tape",
           is_fr?"FICHIER":"FILE" },
        { "memo", '\0', OPTION_ARG_FILENAME, &teo.memo.file,
           is_fr?"Charge une cartouche":"Load a cartridge",
           is_fr?"FICHIER":"FILE" },
        { "mode40", '\0', OPTION_ARG_BOOL, &mode40,
           is_fr?"Affichage en 40 colonnes":"40 columns display", NULL},
        { "mode80", '\0', OPTION_ARG_BOOL, &mode80,
           is_fr?"Affichage en 80 colonnes":"80 columns display", NULL},
        { "truecolor", '\0', OPTION_ARG_BOOL, &truecolor,
           is_fr?"Affichage en vraies couleurs":"Truecolor display", NULL},
        { "window", '\0', OPTION_ARG_BOOL, &windowd,
           is_fr?"Mode fenˆtr‚":"Windowed display", NULL},
        { NULL, 0, 0, NULL, NULL, NULL }
    };
    message = option_Parse (argc, argv, "teow", entries, &remain_name);
    if (message != NULL)
        main_ExitMessage(message);
        
    if (mode40)    gfx_mode = GFX_MODE40   ; else
    if (mode80)    gfx_mode = GFX_MODE80   ; else
    if (truecolor) gfx_mode = GFX_TRUECOLOR; else
    if (windowd)   gfx_mode = GFX_WINDOW;
}



/* thomson_take char:
 *  Convert Thomson ASCII char into ISO-8859-1.
 */
static int thomson_take_char (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case '\x60':   /* middle line */
            strcat (pc_text, "\xad");
            i++;
            break;

        case '\x7e':   /* overline */
            strcat (pc_text, "\xaf");
            i++;
            break;

        case '\x7f':   /* full block */
            strcat (pc_text, " ");
            i++;
            break;

        default:       /* ASCII char */
            if (thomson_text[i] != '\0')
            {
                if (thomson_text[i] >= ' ')
                {
                    strncat (pc_text, thomson_text+i, 1);
                    i++;
                }
                else
                {
                    strcat (pc_text, "?");
                    i++;
                }
            }
            break;
    }
    return i;
}



/* thomson_accent_grave:
 *  Convert Thomson grave accents into ISO-8859-1.
 */
static int thomson_accent_grave (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xe0"); i++; break;
        case 'e': strcat (pc_text, "\xe8"); i++; break;
        case 'i': strcat (pc_text, "\xec"); i++; break;
        case 'o': strcat (pc_text, "\xf2"); i++; break;
        case 'u': strcat (pc_text, "\xf9"); i++; break;
        case 'A': strcat (pc_text, "\xc0"); i++; break;
        case 'E': strcat (pc_text, "\xc8"); i++; break;
        case 'I': strcat (pc_text, "\xcc"); i++; break;
        case 'O': strcat (pc_text, "\xd2"); i++; break;
        case 'U': strcat (pc_text, "\xd9"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_acute:
 *  Convert Thomson acute accents into ISO-8859-1.
 */
static int thomson_accent_acute (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xe1"); i++; break;
        case 'e': strcat (pc_text, "\xe9"); i++; break;
        case 'i': strcat (pc_text, "\xed"); i++; break;
        case 'o': strcat (pc_text, "\xf3"); i++; break;
        case 'u': strcat (pc_text, "\xfa"); i++; break;
        case 'A': strcat (pc_text, "\xc1"); i++; break;
        case 'E': strcat (pc_text, "\xc9"); i++; break;
        case 'I': strcat (pc_text, "\xcd"); i++; break;
        case 'O': strcat (pc_text, "\xd3"); i++; break;
        case 'U': strcat (pc_text, "\xda"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_circ:
 *  Convert Thomson circumflex accents into ISO-8859-1.
 */
static int thomson_accent_circ (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xe2"); i++; break;
        case 'e': strcat (pc_text, "\xea"); i++; break;
        case 'i': strcat (pc_text, "\xee"); i++; break;
        case 'o': strcat (pc_text, "\xf4"); i++; break;
        case 'u': strcat (pc_text, "\xfb"); i++; break;
        case 'A': strcat (pc_text, "\xc2"); i++; break;
        case 'E': strcat (pc_text, "\xca"); i++; break;
        case 'I': strcat (pc_text, "\xce"); i++; break;
        case 'O': strcat (pc_text, "\xd4"); i++; break;
        case 'U': strcat (pc_text, "\xdb"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_uml:
 *  Convert Thomson diaeresis accents into ISO-8859-1.
 */
static int thomson_accent_uml (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xe4"); i++; break;
        case 'e': strcat (pc_text, "\xeb"); i++; break;
        case 'i': strcat (pc_text, "\xef"); i++; break;
        case 'o': strcat (pc_text, "\xf6"); i++; break;
        case 'u': strcat (pc_text, "\xfc"); i++; break;
        case 'A': strcat (pc_text, "\xc4"); i++; break;
        case 'E': strcat (pc_text, "\xcb"); i++; break;
        case 'I': strcat (pc_text, "\xcf"); i++; break;
        case 'O': strcat (pc_text, "\xd6"); i++; break;
        case 'U': strcat (pc_text, "\xdc"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_cedil:
 *  Convert Thomson cedilla into ISO-8859-1.
 */
static int thomson_accent_cedil (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'c': strcat (pc_text, "\xe7"); i++; break;
        case 'C': strcat (pc_text, "\xc7"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent:
 *  Convert Thomson accent into ISO-8859-1.
 */
static int thomson_accent (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case '\0': break;
        case 'A': i = thomson_accent_grave (thomson_text, i+1, pc_text); break;
        case 'B': i = thomson_accent_acute (thomson_text, i+1, pc_text); break;
        case 'C': i = thomson_accent_circ  (thomson_text, i+1, pc_text); break;
        case 'H': i = thomson_accent_uml   (thomson_text, i+1, pc_text); break;
        case 'K': i = thomson_accent_cedil (thomson_text, i+1, pc_text); break;
        case '#': strcat (pc_text, "\xa3"); i++; break;  /* pound */
        case '$': strcat (pc_text, "$"); i++; break;  /* dollar */
        case '&': strcat (pc_text, "#"); i++; break;  /* diesis */
        case ',': strcat (pc_text, "<-"); i++; break;  /* arrow left */
        case '-': strcat (pc_text, "^"); i++; break;  /* arrow up */
        case '.': strcat (pc_text, "->"); i++; break;  /* arrow right */
        case '/': strcat (pc_text, "V"); i++; break;  /* arrow down */
        case '0': strcat (pc_text, "\xb0"); i++; break; /* degree */
        case '1': strcat (pc_text, "\xb1"); i++; break; /*plus minus */
        case '8': strcat (pc_text, "\xf7"); i++; break; /* */
        case '<': strcat (pc_text, "\xbc"); i++; break; /* 1/4 */
        case '=': strcat (pc_text, "\xbd"); i++; break; /* 1/2 */
        case '>': strcat (pc_text, "\xbe"); i++; break; /* 3/4 */
        case 'j': strcat (pc_text, "OE"); i++; break;   /* */
        case 'z': strcat (pc_text, "oe"); i++; break;   /* */
        case '{': strcat (pc_text, "\xdf"); i++; break; /* sharp S */
        case '\'':strcat (pc_text, "\xa7"); i++; break; /* */
        default : i++; break;
    }
    return i;
}



/* thomson_escape:
 *  Skip Thomson escape sequence.
 */
static int thomson_escape (char *thomson_text, int i)
{
    /* skip full screen codes */
    while ((thomson_text[i] == '\x20')
        || (thomson_text[i] == '\x23'))
    {
        i++;
    }

    /* skip escape code */
    if (thomson_text[i] != '\0')
    {
        i++;
    }
    return i;
}



/* thomson_cursor:
 *  Skip Thomson cursor sequence.
 */
static int thomson_cursor (char *thomson_text, int i)
{
    /* skip first code */
    if (thomson_text[i] != '\0')
    {
        i++;
        /* skip second code */
        if (thomson_text[i] != '\0')
        {
            i++;
        }
    }
    return i;
}


/* ------------------------------------------------------------------------- */
 
 
/* main_ThomsonToPcText:
 *  Convert Thomson string into ISO-8859-1.
 */
char *main_ThomsonToPcText (char *thomson_text)
{
    int i = 0;
    static char pc_text[306];

    pc_text[0] = 0;

    while (thomson_text[i] != '\0')
    {
        switch (thomson_text[i])
        {
            case '\x16':   /* accent sequence */
                i = thomson_accent (thomson_text, i+1, pc_text);
                break;

            case '\x1b':   /* escape sequence */
                i = thomson_escape (thomson_text, i+1);
                break;

            case '\x1f':   /* cursor sequence */
                i = thomson_cursor (thomson_text, i+1);
                break;

            default:
                i = thomson_take_char (thomson_text, i, pc_text);
                break;
        }
     }

     return pc_text;
}



/* DisplayMessage:
 *  Affiche un message.
 */
void main_DisplayMessage(const char msg[])
{
    if (windowed_mode)
    {
        MessageBox(prog_win, (const char*)msg, is_fr?"Teo - Erreur":"Teo - Error",
                    MB_OK | MB_ICONERROR);
    }
    else
    {
        agui_PopupMessage (msg);
    }
}



/* ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
void main_ExitMessage(const char msg[])
{
    allegro_exit(); /* pour éviter une fenêtre DirectX zombie */
    main_DisplayMessage(msg);
    exit(EXIT_FAILURE);
}



/* close_procedure:
 *  Procédure de fermeture de la fenêtre par le bouton close.
 */
static void close_procedure (void)
{
    teo.command = TEO_COMMAND_QUIT;
}
 


/* WinMain:
 *  Point d'entrée du programme appelé par l'API Win32.
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    char version_name[]="Teo "TEO_VERSION_STR" (Windows/DirectX)";
    int alleg_depth, argc=0;
#ifndef __MINGW32__
    char *argv[16];
#else
    char **argv;
#endif
    int njoy = 0;
    TCHAR sapfs_exe[MAX_PATH];
    struct STRING_LIST *str_list;

#ifdef FRENCH_LANGUAGE
    is_fr = 1;
#else
    is_fr = 0;
#endif

    /* initialise les librairies */
    InitCommonControls();
    OleInitialize(0);

    /* On s'assure que "allegro.cfg" est dispo dans le repertoire courant sinon TEO échoue. */
    do {
        FILE *f = fopen("allegro.cfg", "rb");
        if(f==NULL) {
            /* On a pas trouvé le fichier. On regarde dans le path et on se positionne dans le
               bon repertoire. */
            char *buf = sapfs_exe;  /* optim: on utilise le buffer sapfs_exe */
            int len = SearchPath(NULL, "allegro", ".cfg", sizeof(sapfs_exe)/sizeof(TCHAR), buf, NULL);
            if(len) {
                char *s = buf;
                while(*s) ++s; 
                while(s>buf && *s!='/' && *s!='\\') --s;
                if(*s=='/' || *s=='\\') *s = '\0';
                SetCurrentDirectory(buf);
            }
        } else fclose(f);
    } while(0);

    /* conversion de la ligne de commande Windows */
    prog_inst = hInst;
    prog_icon = LoadIcon(hInst, "thomson_ico");

#ifndef __MINGW32__	
    if (*lpCmdLine)
    {
        argv[argc++]=lpCmdLine++;

        while (*lpCmdLine)
            if (*lpCmdLine == ' ')
            {
                *lpCmdLine++ = '\0';
                argv[argc++]=lpCmdLine++;
            }
            else
                lpCmdLine++;
    }
#else
	/* Windows fourni des argc/argv déjà parsés qui tient 
	   compte des guillemets et des blancs. */
	argc = __argc;
	argv = (void*)__argv;
#endif

    ini_Load();                   /* Charge les paramètres par défaut */
    read_command_line (argc, argv); /* Récupération des options */

    /* l'initialisation de l'interface clavier, qui utilise un appel GDI, doit avoir lieu
       avant celle du module clavier d'Allegro, basé sur DirectInput */
    wkeybint_Init();

    /* initialisation de la librairie Allegro */
    set_uformat(U_ASCII);  /* pour les accents Latin-1 */
    allegro_init();
    set_config_file(ALLEGRO_CONFIG_FILE);
    install_keyboard();
    install_timer();
    if (njoy >= 0)
        install_joystick(JOY_TYPE_AUTODETECT);
        
    /* décoration de la fenêtre */
    set_window_title(is_fr?"Teo - l'émulateur TO8 (menu:ESC/debogueur:F12)"
                          :"Teo - the TO8 emulator (menu:ESC/debugger:F12)");
    prog_win = win_get_window();
    SetClassLong(prog_win, GCL_HICON,   (LONG) prog_icon);
    SetClassLong(prog_win, GCL_HICONSM, (LONG) prog_icon);

    /* détection de la présence de joystick(s) */
    njoy = MIN(TEO_NJOYSTICKS, num_joysticks);

    /* initialisation de l'émulateur */
    printf(is_fr?"Initialisation de l'‚mulateur...":"Emulator initialization...");
    if (teo_Init(TEO_NJOYSTICKS-njoy) < 0)
        main_ExitMessage(teo_error_msg);
    printf("ok\n");

    /* initialisation du son */
    asound_Init(51200);  /* 51200 Hz car 25600 Hz provoque des irrégularités du timer */

    /* initialisation des joysticks */
    ajoyint_Init(njoy);

    /* initialisation du mode graphique */
    switch(gfx_mode)
    {
        case GFX_MODE40:
            if (!agfxdrv_Init(GFX_MODE40, 8, GFX_AUTODETECT_FULLSCREEN, FALSE))
                main_ExitMessage(is_fr?"Mode graphique non supporté."
                                      :"Unsupported graphic mode");
            windowed_mode = FALSE;
            break;

        case GFX_MODE80:
            if (!agfxdrv_Init(GFX_MODE80, 8, GFX_AUTODETECT_FULLSCREEN, FALSE))
                main_ExitMessage(is_fr?"Mode graphique non supporté."
                                      :"Unsupported graphic mode");
            windowed_mode = FALSE;
            break;

        case GFX_TRUECOLOR:
            if (!agfxdrv_Init(GFX_TRUECOLOR, 15, GFX_AUTODETECT_FULLSCREEN, FALSE))
                if (!agfxdrv_Init(GFX_TRUECOLOR, 16, GFX_AUTODETECT_FULLSCREEN, FALSE))
                    if (!agfxdrv_Init(GFX_TRUECOLOR, 24, GFX_AUTODETECT_FULLSCREEN, FALSE))
                        if (!agfxdrv_Init(GFX_TRUECOLOR, 32, GFX_AUTODETECT_FULLSCREEN, FALSE))
                            main_ExitMessage(is_fr?"Mode graphique non supporté."
                                                  :"Unsupported graphic mode");
            windowed_mode = FALSE;
            break;

        case GFX_WINDOW:
            alleg_depth = desktop_color_depth();

            switch (alleg_depth)
            {
                case 8:  /* 8bpp */
                default:
                    main_ExitMessage(is_fr?"Mode graphique non supporté."
                                          :"Unsupported graphic mode");
                    break;

                case 16: /* 15 ou 16bpp */
                    if ( !agfxdrv_Init(GFX_TRUECOLOR, 15, GFX_AUTODETECT_WINDOWED, TRUE) && 
                         !agfxdrv_Init(GFX_TRUECOLOR, 16, GFX_AUTODETECT_WINDOWED, TRUE) )
                            main_ExitMessage(is_fr?"Mode graphique non supporté."
                                                  :"Unsupported graphic mode");
                    gfx_mode = GFX_TRUECOLOR;
                    break;
 
                case 24: /* 24bpp */
                case 32: /* 32bpp */
                    if (!agfxdrv_Init(GFX_TRUECOLOR, alleg_depth, GFX_AUTODETECT_WINDOWED, TRUE))
                        main_ExitMessage(is_fr?"Mode graphique non supporté."
                                              :"Unsupported graphic mode");
                    gfx_mode = GFX_TRUECOLOR;
                    break;
            }
            windowed_mode = TRUE;
            break;
    }

    /* installation de la fonction callback de retraçage de l'écran nécessaire
       pour les modes fullscreen */
    set_display_switch_callback(SWITCH_IN, RetraceCallback);

    /* on continue de tourner même sans focus car sinon la gui se bloque,
     * et le buffer son tourne sur lui même sans mise à jour et c'est moche. */
    set_display_switch_mode(SWITCH_BACKGROUND); 

    disk_FirstLoad ();  /* Chargement des disquettes éventuelles */
    cass_FirstLoad ();  /* Chargement de la cassette éventuelle */
    if (memo_FirstLoad () < 0) /* Chargement de la cartouche éventuelle */
        reset = 1;

    /* Chargement des options non définies */
    for (str_list=remain_name; str_list!=NULL; str_list=str_list->next)
        if (option_Undefined (str_list->str) == 1)
            reset = 1;
    std_StringListFree (remain_name);

    /* Restitue l'état sauvegardé de l'émulateur */
    teo_FullReset();
    if (reset == 0)
        if (image_Load ("autosave.img") != 0)
            teo_FullReset();

    /* initialisation de l'interface utilisateur Allegro et du débogueur */
    teo_DebugBreakPoint = NULL;
    if (!windowed_mode)
    {
       agui_Init(version_name, gfx_mode, FALSE);
    }
    else
    {
       set_window_close_hook(close_procedure);
    }

    /* et c'est parti !!! */
    RunTO8();

    /* Sauvegarde de l'état de l'émulateur */
    ini_Save();
    image_Save ("autosave.img");

    /* désinstallation du callback *avant* la sortie du mode graphique */
    remove_display_switch_callback(RetraceCallback);

    /* libération de la mémoire si mode fenêtré */
    if (windowed_mode)
       wgui_Free();
    else
       agui_Free();

    /* sortie du mode graphique */
    SetGraphicMode(SHUTDOWN);

    /* désinstalle les librairies */
    OleUninitialize ();

    /* sortie de l'émulateur */
    printf(is_fr?"A bient“t !\n":"Goodbye !\n");

    /* sortie de l'émulateur */
    exit(EXIT_SUCCESS);
}
