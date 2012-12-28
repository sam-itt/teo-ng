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
 *  Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret
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
 *  Module     : dos/main.c
 *  Version    : 1.8.2
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Eric Botcazou 04/11/2003
 *               Samuel Devulder 08/2011
 *               François Mouret 08/2011 25/04/2012 01/11/2012
 *
 *  Boucle principale de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "teo.h"
#include "option.h"
#include "ini.h"
#include "image.h"
#include "main.h"
#include "error.h"
#include "media/disk.h"
#include "media/cass.h"
#include "media/memo.h"
#include "media/printer.h"
#include "mc68xx/mc6809.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "alleg/joyint.h"
#include "alleg/mouse.h"
#include "alleg/sound.h"
#include "dos/keybint.h"
#include "dos/floppy.h"
#include "dos/debug.h"

/* pour limiter la taille de l'éxécutable */
BEGIN_COLOR_DEPTH_LIST
    COLOR_DEPTH_8
    COLOR_DEPTH_15
    COLOR_DEPTH_16
END_COLOR_DEPTH_LIST

BEGIN_GFX_DRIVER_LIST
    GFX_DRIVER_VGA
    GFX_DRIVER_VBEAF
    GFX_DRIVER_VESA3
    GFX_DRIVER_VESA2L
    GFX_DRIVER_VESA2B
    GFX_DRIVER_VESA1
END_GFX_DRIVER_LIST

BEGIN_DIGI_DRIVER_LIST
    DIGI_DRIVER_SB
    DIGI_DRIVER_SOUNDSCAPE
    DIGI_DRIVER_AUDIODRIVE
    DIGI_DRIVER_WINSOUNDSYS
END_DIGI_DRIVER_LIST 

BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST

struct EMUTEO teo;

static int reset = FALSE;
static int gfx_mode = NO_GFX;
struct STRING_LIST *remain_name = NULL;

int frame;                 /* compteur de frame vidéo */
int direct_write_support = FALSE;
static volatile int tick;  /* compteur du timer */


static void Timer(void)
{
    tick++;
}
END_OF_FUNCTION(Timer)



/* RunTO8:
 *  Boucle principale de l'émulateur.
 */
static void RunTO8(void)
{
    amouse_Install (TO8_MOUSE); /* la souris est le périphérique de pointage par défaut */
    RetraceScreen(0, 0, SCREEN_W, SCREEN_H);

    do  /* boucle principale de l'émulateur */
    {
        teo.command=TEO_COMMAND_NONE;

        /* installation des handlers clavier, souris et son */ 
        dkeybint_Install();
        amouse_Install(LAST_POINTER);

        if (teo.setting.exact_speed)
        {
            if (teo.setting.sound_enabled)
                asound_Start();
            else
            {
                install_int_ex(Timer, BPS_TO_TIMER(TO8_FRAME_FREQ));
                frame=1;
                tick=frame;
            }
        }

        do  /* boucle d'émulation */
        {
            to8_DoFrame(FALSE);

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
                    asound_Play ();
                else
                    while (frame==tick)
                        ;
            }

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
        dkeybint_ShutDown();

        /* éxécution des commandes */
        if (teo.command==TEO_COMMAND_PANEL)
            agui_Panel();

        if (teo.command==TEO_COMMAND_SCREENSHOT)
            agfxdrv_Screenshot();

        if (teo.command==TEO_COMMAND_DEBUGGER)
        {
            remove_keyboard();
            SetGraphicMode(SHUTDOWN);
            ddebug_Run();
            SetGraphicMode(RESTORE);
            install_keyboard();
        }

        if (teo.command==TEO_COMMAND_RESET)
            to8_Reset();

        if (teo.command==TEO_COMMAND_COLD_RESET)
        {
            to8_ColdReset();
            amouse_Install(TO8_MOUSE);
        }
    }
    while (teo.command != TEO_COMMAND_QUIT);  /* fin de la boucle principale */

    /* Finit d'exécuter l'instruction et/ou l'interruption courante */
    mc6809_FlushExec();
}



/* ReadCommandLine:
 *  Lit la ligne de commande
 */
static void ReadCommandLine(int argc, char *argv[])
{
    char *message;
    int mode40=0, mode80=0, truecolor=0;

    struct OPTION_ENTRY entries[] = {
        { "reset", 'r', OPTION_ARG_BOOL, &reset,
           is_fr?"Reset … froid de l'‚mulateur"
                :"Cold-reset emulator", NULL },
        { "disk0", '0', OPTION_ARG_FILENAME, &teo.disk[0].file,
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)",
           is_fr?"CHEMIN":"PATH" },
        { "disk1", '1', OPTION_ARG_FILENAME, &teo.disk[1].file,
           is_fr?"Charge un disque virtuel (lecteur 1)"
                :"Load virtual disk (drive 1)",
           is_fr?"CHEMIN":"PATH" },
        { "disk2", '2', OPTION_ARG_FILENAME, &teo.disk[2].file,
           is_fr?"Charge un disque virtuel (lecteur 2)"
                :"Load virtual disk (drive 2)",
           is_fr?"CHEMIN":"PATH" },
        { "disk3", '3', OPTION_ARG_FILENAME, &teo.disk[3].file,
           is_fr?"Charge un disque virtuel (lecteur 3)"
                :"Load virtual disk (drive 3)",
           is_fr?"CHEMIN":"PATH" },
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
        { "enable-direct-write", '\0', OPTION_ARG_BOOL, &direct_write_support,
           is_fr?"Autorise l'‚criture en mode direct":"Enable writing in direct mode", NULL},
        { NULL, 0, 0, NULL, NULL, NULL }
    };
    message = option_Parse (argc, argv, "teo", entries, &remain_name);
    if (message != NULL)
        main_ExitMessage(message);
        
    if (mode40)    gfx_mode = GFX_MODE40   ; else
    if (mode80)    gfx_mode = GFX_MODE80   ; else
    if (truecolor) gfx_mode = GFX_TRUECOLOR;
}



/* sysexec:
 *  Demande à l'OS d'executer une cmd dans un dossier précis.
 */
int main_SysExec(char *cmd, const char *dir) {
    char cwd[MAX_PATH]="";
    char *tmp;
    int i;
    
    tmp = getcwd(cwd, MAX_PATH);
    i = chdir(dir);
    i = system(cmd);
    i = chdir(cwd);
    return 0;
    (void)i;
    (void)tmp;
}


/* rmFile:
 *   Efface un fichier.
 */
int main_RmFile(char *path) {
    unlink(path);
    return 0;
}


/* tmpFile:
 *   Cree un fichier temporaire.
 */
char *main_TmpFile(char *buf, int maxlen) {
    char *tmp;
    tmp = tmpnam(buf);
    strcat(buf, ".sap");
    return buf;
}


/* main_ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
void main_DisplayMessage(const char msg[])
{
    fprintf(stderr, "%s\n", msg);
}


/* main_ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
void main_ExitMessage(const char msg[])
{
    main_DisplayMessage(msg);
    exit(EXIT_FAILURE);
}



#define IS_3_INCHES(drive) ((drive_type[drive]>2) && (drive_type[drive]<7))

/* main:
 *  Point d'entrée du programme appelé par MS-DOS.
 */
int main(int argc, char *argv[])
{
    char version_name[]="Teo "TO8_VERSION_STR" (MSDOS/DPMI)";
#ifdef FRENCH_LANG
    char *mode_desc[3]= {
        " 1. Mode 40 colonnes 16 couleurs\n    (affichage rapide, adapt‚ aux jeux et … la plupart des applications)",
        " 2. Mode 80 colonnes 16 couleurs\n    (pour les applications fonctionnant en 80 colonnes)",
        " 3. Mode 80 colonnes 4096 couleurs\n    (affichage lent mais support des changements dynamiques de palette)" };
#else
    char *mode_desc[3]= {
        " 1. 40 columns mode 16 colors\n    (fast diplay, adapted to games and most applications)",
        " 2. 80 columns mode 16 colors\n    (for applications which needs 80 columns)",
        " 3. 80 columns mode 4096 colors\n    (slow display but allow dynamic changes of palette)" };
#endif
    int direct_support = 0;
    int drive_type[4];
    int njoy = 0;  /* njoy=-1 si joystick non supportés */
    int scancode, i;
    struct STRING_LIST *str_list = NULL;

#ifdef FRENCH_LANG
    is_fr = 1;
#else
    is_fr = 0;
#endif

    /* traitement des paramètres */
    ini_Load();                   /* Charge les paramètres par défaut */
    ReadCommandLine (argc, argv); /* Récupération des options */

    /* initialisation de la librairie Allegro */
    set_uformat(U_ASCII);  /* pour les accents français */
    allegro_init();
    set_config_file(ALLEGRO_CONFIG_FILE);
    install_keyboard();
    install_timer();
    if (njoy >= 0)
        install_joystick(JOY_TYPE_AUTODETECT);
    LOCK_VARIABLE(teo);
    LOCK_VARIABLE(tick);
    LOCK_FUNCTION(Timer);

    /* message d'entête */
    if (is_fr) {
    printf("Voici %s l'‚mulateur Thomson TO8.\n", version_name);
    printf("Copyright 1997-2012 Gilles F‚tis, Eric Botcazou, Alex Pukall,J‚r‚mie Guillaume, Fran‡ois Mouret, Samuel Devulder\n\n");
    printf("Touches: [ESC] Panneau de contr“le\n");
    printf("         [F11] Capture d'‚cran\n");
    printf("         [F12] D‚bogueur\n\n");
    } else {
    printf("Here is %s the Thomson TO8 emulator.\n", version_name);
    printf("Copyright 1997-2012 Gilles F‚tis, Eric Botcazou, Alex Pukall,J‚r‚mie Guillaume, Fran‡ois Mouret, Samuel Devulder\n\n");
    printf("Keys: [ESC] Control panel\n");
    printf("      [F11] Screen capture\n");
    printf("      [F12] Debugger\n\n");
    }

    /* détection de la présence de joystick(s) */
    njoy = MIN(TO8_NJOYSTICKS, num_joysticks);

    /* initialisation de l'émulateur */
    printf(is_fr?"Initialisation de l'‚mulateur...":"Emulator initialization...");

    if (to8_Init(TO8_NJOYSTICKS-njoy) < 0)
        main_ExitMessage(teo_error_msg);

    printf("ok\n");

    /* initialisation de l'interface clavier */
    dkeybint_Init();

    /* initialisation de l'interface d'accès direct */
    dfloppy_Init (drive_type, direct_write_support);

    /* Détection des lecteurs supportés (3"5 seulement) */
    for (i=0; i<4; i++)
    {
        if (IS_3_INCHES(i))
            direct_support |= (1<<i);
    }

    /* initialisation du son */
    asound_Init(25600);  /* 25600 Hz */

    /* initialisation des joysticks */
    ajoyint_Init(njoy);

    /* sélection du mode graphique */ 
    printf(is_fr?"\nS‚lection du mode graphique:\n\n":"\nSelect graphic mode:\n\n");

    if (gfx_mode == NO_GFX)
    {
        for (i=0; i<3; i++)
            printf("%s\n\n", mode_desc[i]);
            
        printf(is_fr?"Votre choix: [1 par d‚faut] ":"Your choice: [1 by default] ");

        do
        {
            scancode = readkey()>>8;

            if (key_shifts&KB_CTRL_FLAG)
               reset = 0;

            switch (scancode_to_ascii(scancode))
            {
                case '1':
                case 13:
                    gfx_mode=GFX_MODE40;
                    break;

                case '2':
                    gfx_mode=GFX_MODE80;
                    break;

                case '3':
                    gfx_mode=GFX_TRUECOLOR;
                    break;
            }
        }
        while (gfx_mode == NO_GFX);
    }
    else
    {
        printf("%s\n\n", mode_desc[gfx_mode-1]);
    }

    /* initialisation du mode graphique */
    switch (gfx_mode)
    {
        case GFX_MODE40:
            if (agfxdrv_Init(GFX_MODE40, 8, GFX_VGA, FALSE))
                goto driver_found;
            break;

        case GFX_MODE80:
            for (i=0; i<3; i++)
                if (agfxdrv_Init(GFX_MODE80, 8, GFX_AUTODETECT, FALSE))
                    goto driver_found;
            break;
                
        case GFX_TRUECOLOR:
            for (i=0; i<3; i++)
                if (agfxdrv_Init(GFX_TRUECOLOR, 15, GFX_AUTODETECT, FALSE) || 
                           agfxdrv_Init(GFX_TRUECOLOR, 16, GFX_AUTODETECT, FALSE))
                    goto driver_found;
           break;
    }

    main_ExitMessage(is_fr?"\nErreur: mode graphique non support‚.":"\nError: unsupported graphic mode.");

  driver_found:

    /* initialisation de l'interface utilisateur */
    agui_Init(version_name, gfx_mode, direct_support);
    
    disk_FirstLoad ();  /* chargement des disquettes éventuelles */
    cass_FirstLoad ();  /* chargement de la cassette éventuelle */
    memo_FirstLoad ();  /* chargement de la cartouche éventuelle */

    /* chargement des options non définies */
    for (str_list=remain_name; str_list!=NULL; str_list=str_list->next)
        option_Undefined (str_list->str);
    std_StringListFree (remain_name);

    /* reset éventuel de l'émulateur */
    to8_ColdReset();
    if (reset == 0)
        if (access("autosave.img", F_OK) >= 0)
            image_Load ("autosave.img");

    /* et c'est parti !!! */
    RunTO8();

    /* libère la mémoire de la GUI */
    agui_Free();

    /* sortie du mode graphique */
    SetGraphicMode(SHUTDOWN);

    /* mise au repos de l'interface d'accès direct */
    dfloppy_Exit();

    /* sortie de l'émulateur */
    printf(is_fr?"A bient“t !\n":"Goodbye !\n");

    /* sortie de l'émulateur */
    exit(EXIT_SUCCESS);
}

