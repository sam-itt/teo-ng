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
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou septembre 2000
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Samuel Devulder 30/07/2011
 *               François Mouret 19/10/2012 24/10/2012
 *
 *  Boucle principale de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <allegro.h>
   #include <winalleg.h>
   #include <sys/stat.h>
   #include <ctype.h>
#endif

#include "defs.h"
#include "teo.h"
#include "option.h"
#include "std.h"
#include "ini.h"
#include "image.h"
#include "main.h"
#include "error.h"
#include "media/libsap.h"
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
#include "win/keybint.h"
#include "win/gui.h"


struct EMUTEO teo;

static int reset = FALSE;
static int gfx_mode = GFX_WINDOW;
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
static void RunTO8(int windowed_mode)
{
    amouse_Install(TEO_MOUSE); /* la souris est le périphérique de pointage par défaut */
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
            teo_DoFrame(FALSE);

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

        if (teo.command==TEO_COMMAND_SCREENSHOT)
            agfxdrv_Screenshot();

        if (teo.command==TEO_COMMAND_RESET)
            teo_Reset();

        if (teo.command==TEO_COMMAND_COLD_RESET)
        {
            teo_ColdReset();
            amouse_Install(TEO_MOUSE);
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
    int mode40=0, mode80=0, truecolor=0, windowd=0;

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



/* shortpath:
 *  Retourne une version courte du path passé en argument alloué dans le tas.
 */
static char *shortpath(char *path) {
    int len = GetShortPathName(path, NULL, 0);
    char *buf = malloc(len + 1);
    if(!buf) main_ExitMessage(is_fr?"Plus de mémoire":"Out of memory");
    if(GetShortPathName(path, buf, len+1)) return buf;
    main_ExitMessage(path);
    free(buf);
    return NULL;
}


/* ------------------------------------------------------------------------- */


/* DisplayMessage:
 *  Affiche un message.
 */
void main_DisplayMessage(const char msg[])
{
    MessageBox(NULL, (const char*)msg, is_fr?"Teo - Erreur":"Teo - Error",
                MB_OK | MB_ICONERROR);
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



/* sysexec:
 *  Demande à l'OS d'executer une cmd dans un dossier précis.
 */
int main_SysExec(char *cmd, const char *dir) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    
    si.cb = sizeof(si);

    /*
    fprintf(stderr, "sysexec(%s, \"%s\")\n", cmd, dir?dir:"");
    fflush(stderr);
    */
    
    if(!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, dir, &si, &pi)) {
        char buf[256];
        sprintf(buf, is_fr?"CreateProcess a échoué (%d): %s pour %s.\n"
                          :"CreateProcess failed (%d): %s in %s.\n",
                          (int)GetLastError(),
                          cmd,
                          dir?dir:(is_fr?"répertoire courant":"current dir"));
        return TEO_ERROR;
    }
    
    /* Wait until child process exits. */
    WaitForSingleObject(pi.hProcess, INFINITE);

    /* Close process and thread handles. */
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}


/* rmFile:
 *   Efface un fichier.
 */
int main_RmFile(char *path) {
    DeleteFile(path);
    return 0;
}

/* tmpFile:
 *   Cree un fichier temporaire.
 */
char *main_TmpFile(char *buf, int maxlen) {
    char tmpdir[MAX_PATH], *s;
    
    if(MAX_PATH < GetTempPath(MAX_PATH, tmpdir)) return NULL;
    if(0 == GetTempFileName(tmpdir, "sap", 0, buf)) return NULL;
    if(!buf[0]) return NULL;
    
    /* il faut forcer la création du sap ou shortpath échoue */
    fclose(fopen(buf, "wb"));
    
    /* nom courts forcément */
    s = shortpath(buf);
    if(!s) return NULL;
    strncpy(buf, s, maxlen); 
    free(s);
    
    return buf;
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
    int windowed_mode=FALSE;
    int njoy = 0;
    TCHAR sapfs_exe[MAX_PATH];
    struct STRING_LIST *str_list;

#ifdef FRENCH_LANG
    is_fr = 1;
#else
    is_fr = 0;
#endif

    /* On s'assure que "fichier.rom" est dispo dans le repertoire courant sinon TEO échoue. */
    do {
        FILE *f = fopen("fichier.rom", "rb");
        if(f==NULL) {
            /* On a pas trouvé le fichier. On regarde dans le path et on se positionne dans le
               bon repertoire. */
            char *buf = sapfs_exe;  /* optim: on utilise le buffer sapfs_exe */
            int len = SearchPath(NULL, "fichier", ".rom", sizeof(sapfs_exe)/sizeof(TCHAR), buf, NULL);
            if(len) {
                char *s = buf;
                while(*s) ++s; 
                while(s>buf && *s!='/' && *s!='\\') --s;
                if(*s=='/' || *s=='\\') *s = '\0';
                SetCurrentDirectory(buf);
            }
        } else fclose(f);
    } while(0);
    
    /* Retrouve sapfs.exe dans le PATH */
    do {
        int len = SearchPath(NULL, "sapfs", ".exe", sizeof(sapfs_exe)/sizeof(TCHAR), sapfs_exe, NULL);
        if(len==0) {
            fprintf(stderr, is_fr?"sapfs.exe introuvable":"Cannot find sapfs.exe\n"); fflush(stderr);
            sapfs_exe[0] = '\0'; /* c'est pas grave, on peut faire sans */
        }
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
    ReadCommandLine (argc, argv); /* Récupération des options */

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
    set_window_title(is_fr?"Teo - l'émulateur TO8 (menu:ESC)":"Teo - the TO8 emulator (menu:ESC)");
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
            break;

        case GFX_MODE80:
            if (!agfxdrv_Init(GFX_MODE80, 8, GFX_AUTODETECT_FULLSCREEN, FALSE))
                main_ExitMessage(is_fr?"Mode graphique non supporté."
                                      :"Unsupported graphic mode");
            break;

        case GFX_TRUECOLOR:
            if (!agfxdrv_Init(GFX_TRUECOLOR, 15, GFX_AUTODETECT_FULLSCREEN, FALSE))
                if (!agfxdrv_Init(GFX_TRUECOLOR, 16, GFX_AUTODETECT_FULLSCREEN, FALSE))
                    if (!agfxdrv_Init(GFX_TRUECOLOR, 24, GFX_AUTODETECT_FULLSCREEN, FALSE))
                        if (!agfxdrv_Init(GFX_TRUECOLOR, 32, GFX_AUTODETECT_FULLSCREEN, FALSE))
                            main_ExitMessage(is_fr?"Mode graphique non supporté."
                                                  :"Unsupported graphic mode");
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

    /* initialisation de l'interface utilisateur Allegro */
    if (!windowed_mode)
       agui_Init(version_name, gfx_mode, FALSE);
    else
       set_window_close_hook(close_procedure);

    /* installation de la fonction callback de retraçage de l'écran nécessaire
       pour les modes fullscreen */
    set_display_switch_callback(SWITCH_IN, RetraceCallback);

    /* on continue de tourner même sans focus car sinon la gui se bloque,
     * et le buffer son tourne sur lui même sans mise à jour et c'est moche. */
    set_display_switch_mode(SWITCH_BACKGROUND); 

    disk_FirstLoad ();  /* Chargement des disquettes éventuelles */
    cass_FirstLoad ();  /* Chargement de la cassette éventuelle */
    memo_FirstLoad ();  /* Chargement de la cartouche éventuelle */

    /* Chargement des options non définies */
    for (str_list=remain_name; str_list!=NULL; str_list=str_list->next)
        option_Undefined (str_list->str);
    std_StringListFree (remain_name);

    /* reset éventuel de l'émulateur */
    teo_ColdReset();
    if (reset == 0)  
        if (access("autosave.img", F_OK) >= 0)
            image_Load("autosave.img");

    /* et c'est parti !!! */
    RunTO8(windowed_mode);

    /* désinstallation du callback *avant* la sortie du mode graphique */
    remove_display_switch_callback(RetraceCallback);

    /* libération de la mémoire si mode fenêtré */
    if (windowed_mode)
       wgui_Free();
    else
       agui_Free();

    /* sortie du mode graphique */
    SetGraphicMode(SHUTDOWN);

    /* sortie de l'émulateur */
    printf(is_fr?"A bient“t !\n":"Goodbye !\n");

    /* sortie de l'émulateur */
    exit(EXIT_SUCCESS);
}
