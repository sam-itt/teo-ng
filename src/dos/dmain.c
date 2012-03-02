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
 *  Copyright (C) 1997-2011 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Eric Botcazou 04/11/2003
 *               Samuel Devulder 08/2011
 *               François Mouret 08/2011
 *
 *  Boucle principale de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "alleg/joyint.h"
#include "alleg/keybint.h"
#include "alleg/main.h"
#include "alleg/mouse.h"
#include "alleg/sound.h"
#include "alleg/state.h"
#include "dos/disk.h"
#include "dos/debug.h"
#include "to8.h"

extern int (*SetInterlaced)(int);

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


struct EmuTO teo={
    TRUE,
    TRUE,
    NONE
};

int frame;                 /* compteur de frame vidéo */
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
    if (!teo.sound_enabled)
        install_int_ex(Timer,BPS_TO_TIMER(TO8_FRAME_FREQ));

    frame=1;
    InstallPointer(TO8_MOUSE); /* la souris est le périphérique de pointage par défaut */
    RetraceScreen(0, 0, SCREEN_W, SCREEN_H);

    do  /* boucle principale de l'émulateur */
    {
        teo.command=NONE;
        tick=frame;

        /* installation des handlers clavier, souris et son */ 
        InstallKeybint();
        InstallPointer(LAST_POINTER);

        if (teo.sound_enabled && teo.exact_speed)
            StartSound();

        do  /* boucle d'émulation */
        {
            to8_DoFrame();

            /* rafraîchissement de la palette */ 
            if (need_palette_refresh)
                RefreshPalette();

            /* rafraîchissement de l'écran */
            RefreshScreen();

            /* mise à jour de la position des joysticks */
            UpdateJoystick();

            /* synchronisation sur fréquence réelle */
            if (teo.exact_speed)
            {
                if (teo.sound_enabled)
                    PlaySoundBuffer();
                else
                    while (frame==tick)
                        ;
            }

            frame++;
        }
        while (teo.command==NONE);  /* fin de la boucle d'émulation */

        /* désinstallation des handlers clavier, souris et son */
        if (teo.sound_enabled && teo.exact_speed)
            StopSound();

        ShutDownPointer();
        ShutDownKeybint();

        /* éxécution des commandes */
        if (teo.command==CONTROL_PANEL)
            ControlPanel();

        if (teo.command==SCREENSHOT)
            Screenshot();

        if (teo.command==DEBUGGER)
        {
            remove_keyboard();
            SetGraphicMode(SHUTDOWN);
            Debugger();
            SetGraphicMode(RESTORE);
            install_keyboard();
        }

        if (teo.command==RESET)
            to8_Reset();

        if (teo.command==COLD_RESET)
        {
            to8_ColdReset();
            InstallPointer(TO8_MOUSE);
        }
    }
    while (teo.command != QUIT);  /* fin de la boucle principale */

    if (!teo.sound_enabled)
        remove_int(Timer);
}



/* ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
static void ExitMessage(const char msg[])
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}


#define IS_3_INCHES(drive) ((drive_type[drive]>2) && (drive_type[drive]<7))

/* main:
 *  Point d'entrée du programme appelé par MS-DOS.
 */
int main(int argc, char *argv[])
{
    char version_name[]="Teo "TO8_VERSION_STR" (MSDOS/DPMI)";
    char memo_name[FILENAME_LENGTH]="\0";
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
    int gfx_mode=NO_GFX;
    int direct_support = 0, direct_write_support = FALSE;
    int drive_type[4];
    int load_state = FALSE;
    int njoy = 0, scancode, i;
    int interlaced = FALSE;

#ifdef FRENCH_LANG
    is_fr = 1;
#else
    is_fr = 0;
#endif

    /* traitement des paramètres */
    for (i=1;i<argc;i++)
    {
        if (!strcmp(argv[i],"-help") || !strcmp(argv[i],"-h"))
        {
            if (is_fr) {
            printf("Usage:\n");
            printf("           %s [options...]\n",argv[0]);
            printf("o— les options sont prises parmi les suivantes:\n");
            printf("    -help         affiche cette aide\n");
            printf("    -m file.m7    place la cartouche file.m7 dans le lecteur\n");
            printf("    -fast         active la pleine vitesse de l'‚mulateur\n");
            printf("    -nosound      supprime le son de l'‚mulateur\n");
            printf("    -nojoy        d‚sactive la prise en charge des joysticks PC\n");
            printf("    -mode40       lance l'‚mulateur en mode 40 colonnes 16 couleurs\n");
            printf("    -mode80       lance l'‚mulateur en mode 80 colonnes 16 couleurs\n");
            printf("    -truecolor    lance l'‚mulateur en mode 80 colonnes 4096 couleurs\n");
            printf("    -loadstate    charge le dernier ‚tat sauvegard‚ de l'‚mulateur\n");
            printf("    -interlaced   active l'affichage entrelac‚\n");
            printf("    xxxxxx        charge un (des) fichier(s) SAP, K7, M7\n");
            } else {
            printf("Usage:\n");
            printf("           %s [options...]\n",argv[0]);
            printf("where options are taken among the following:\n");
            printf("    -help         this help\n");
            printf("    -m file.m7    load the cartridge file.m7 in the drive\n");
            printf("    -fast         activate fastest speed of emulator\n");
            printf("    -nosound      disable the sound\n");
            printf("    -nojoy        disable PC joysticks\n");
            printf("    -mode40       run the emulator in 40 columns mode 16 colors\n");
            printf("    -mode80       run the emulator in 80 columns mode 16 colors\n");
            printf("    -truecolor    run the emulator in 80 columns mode 4096 colors\n");
            printf("    -loadstate    load last saved state of the emulator\n");
            printf("    -interlaced   activate interlaced video\n");
            printf("    xxxxxx        load SAP, K7, M7 file(s)\n");
            }
            exit(EXIT_SUCCESS);
        }
        else if (!strcmp(argv[i],"-m") && i<argc-1)
            strcpy(memo_name, argv[++i]);
        else if (!strcmp(argv[i],"-fast"))
            teo.exact_speed = FALSE;
        else if (!strcmp(argv[i],"-nosound"))
            teo.sound_enabled = FALSE;
        else if (!strcmp(argv[i],"-nojoy"))
            njoy = -1;
        else if (!strcmp(argv[i],"-mode40"))
            gfx_mode = GFX_MODE40;
        else if (!strcmp(argv[i],"-mode80"))
            gfx_mode = GFX_MODE80;
        else if (!strcmp(argv[i],"-truecolor"))
            gfx_mode = GFX_TRUECOLOR;
        else if (!strcmp(argv[i],"-loadstate"))
            load_state = TRUE;
        else if (!strcmp(argv[i],"-interlaced"))
            interlaced = TRUE;
        else if (!strcmp(argv[i],"--enable-direct-write"))
            direct_write_support = TRUE;
        else
        {
            char msg[128];

            sprintf(msg, is_fr?"ParamŠtre inconnu: %s":"Unknown parameter: %s", argv[i]);
            ExitMessage(msg);
        }
    }

    /* initialisation de la librairie Allegro */
    set_uformat(U_ASCII);  /* pour les accents français */
    allegro_init();
    set_config_file(TEO_CONFIG_FILE);
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
    printf("Copyright 1997-2011 Gilles F‚tis, Eric Botcazou, Alex Pukall,J‚r‚mie Guillaume, Fran‡ois Mouret, Samuel Devulder\n\n");
    printf("Touches: [ESC] Panneau de contr“le\n");
    printf("         [F11] Capture d'‚cran\n");
    printf("         [F12] D‚bogueur\n\n");
    } else {
    printf("Here is %s the Thomson TO8 emulator.\n", version_name);
    printf("Copyright 1997-2011 Gilles F‚tis, Eric Botcazou, Alex Pukall,J‚r‚mie Guillaume, Fran‡ois Mouret, Samuel Devulder\n\n");
    printf("Keys: [ESC] Control panel\n");
    printf("      [F11] Screen capture\n");
    printf("      [F12] Debugger\n\n");
    }

    /* détection de la présence de joystick(s) */
    njoy = MIN(TO8_NJOYSTICKS, num_joysticks);

    /* initialisation de l'émulateur */
    printf(is_fr?"Initialisation de l'‚mulateur...":"Emulator initialization...");

    if (to8_Init(TO8_NJOYSTICKS-njoy) == TO8_ERROR)
        ExitMessage(to8_error_msg);

    printf("ok\n");

    /* initialisation de l'interface clavier */
    InitKeybint();

    /* initialisation de l'interface d'accès direct */
    InitDirectDisk(drive_type, direct_write_support);

    /* Détection des lecteurs supportés (3"5 seulement) */
    for (i=0; i<4; i++)
    {
        if (IS_3_INCHES(i))
            direct_support |= (1<<i);
    }

    /* initialisation du son */
    InitSound(25600);  /* 25600 Hz */

    /* initialisation des joysticks */
    InitJoyint(njoy);

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
               load_state = TRUE;

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
            if (InitGraphic(GFX_MODE40, 8, GFX_VGA, FALSE))
                goto driver_found;
            break;

        case GFX_MODE80:
            for (i=0; i<3; i++)
                if (InitGraphic(GFX_MODE80, 8, GFX_AUTODETECT, FALSE))
                    goto driver_found;
            break;
                
        case GFX_TRUECOLOR:
            for (i=0; i<3; i++)
                if (InitGraphic(GFX_TRUECOLOR, 15, GFX_AUTODETECT, FALSE) || 
                           InitGraphic(GFX_TRUECOLOR, 16, GFX_AUTODETECT, FALSE))
                    goto driver_found;
           break;
    }

    ExitMessage(is_fr?"\nErreur: mode graphique non support‚.":"\nError: unsupported graphic mode.");

  driver_found:
    /* initialisation de l'interface utilisateur */
    InitGUI(version_name, gfx_mode, direct_support);

    to8_ColdReset();

    if (memo_name[0])
        to8_LoadMemo7(memo_name);

    if (interlaced)
        SetInterlaced(1);

    if (load_state)
        LoadState();

    /* et c'est parti !!! */
    RunTO8();

    SaveState();

    /* sortie du mode graphique */
    SetGraphicMode(SHUTDOWN);

    /* mise au repos de l'interface d'accès direct */
    ExitDirectDisk();

    /* sortie de l'émulateur */
    printf(is_fr?"A bient“t !\n":"Goodbye !\n");

    exit(EXIT_SUCCESS);
}

