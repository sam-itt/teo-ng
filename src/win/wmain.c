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
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou septembre 2000
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Samuel Devulder 30/07/2011
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

#include "intern/printer.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "alleg/joyint.h"
#include "alleg/keybint.h"
#include "alleg/main.h"
#include "alleg/mouse.h"
#include "alleg/sound.h"
#include "alleg/state.h"
#include "win/gui.h"
#include "xargs.h"
#include "to8.h"


struct EmuTO teo={
    TRUE,
    TRUE,
    NONE
};

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
    if (!teo.sound_enabled)
        install_int_ex(Timer, BPS_TO_TIMER(TO8_FRAME_FREQ));

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
                   Sleep(0);
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
        {
            if (windowed_mode)
                DisplayControlPanelWin();
            else
                ControlPanel();
        }

        if (teo.command==SCREENSHOT)
            Screenshot();

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
static void ExitMessage(char *msg)
{
    allegro_exit(); /* pour éviter une fenêtre DirectX zombie */
    MessageBox(NULL, (const char*)msg, is_fr?"Teo - Erreur":"Teo - Error", MB_OK | MB_ICONERROR);
    exit(EXIT_FAILURE);
}



/* isDir:
 *  Retourne vrai si le fichier est un répertoire.
 */
static int isDir(char *path) {
    struct stat buf;
    int ret = 0;
    if(!stat(path, &buf)) ret = S_ISDIR(buf.st_mode);
    return ret;
}

/* isFile:
 *  Retourne vrai si le fichier est un fichier.
 */
static int isFile(char *path) {
    struct stat buf;
    int ret = 0;
    if(!stat(path, &buf)) ret = S_ISREG(buf.st_mode);
    return ret;
}

/* sysexec:
 *  Demande à l'OS d'executer une cmd dans un dossier précis.
 */
static void sysExec(char *cmd, const char *dir) {
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
        sprintf(buf, is_fr?"CreateProcess a échoué (%d): %s pour %s.\n":"CreateProcess failed (%d): %s in %s.\n", (int)GetLastError(), cmd, dir?dir:(is_fr?"répertoire courant":"current dir"));
        ExitMessage(buf);
        return;
    }
    
    /* Wait until child process exits. */
    WaitForSingleObject(pi.hProcess, INFINITE);

    /* Close process and thread handles. */
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

/* shortpath:
 *  Retourne une version courte du path passé en argument alloué dans le tas.
 */
static char *shortpath(char *path) {
    int len = GetShortPathName(path, NULL, 0);
    char *buf = malloc(len + 1);
    if(!buf) ExitMessage(is_fr?"Plus de mémoire":"Out of memory");
    if(GetShortPathName(path, buf, len+1)) return buf;
    ExitMessage(path);
    free(buf);
    return NULL;
}

/* rmFile:
 *   Efface un fichier.
 */
static void rmFile(char *path) {
    DeleteFile(path);
}

/* tmpFile:
 *   Cree un fichier temporaire.
 */
static char *tmpFile(char *buf, int maxlen) {
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
 
/* unknownArg:
 *  traite les arguments inconnus.
 */
static int unknownArg(char *arg) {
    char buf[256];
    
    if(!strcmp(arg,"-m")         ||
       !strcmp(arg,"-fast")      ||
       !strcmp(arg,"-nosound")   ||
       !strcmp(arg,"-nojoy")     ||
       !strcmp(arg,"-mode40")    ||
       !strcmp(arg,"-mode80")    ||
       !strcmp(arg,"-truecolor") ||
       !strcmp(arg,"-window")    ||
       !strcmp(arg,"-loadstate"))
            return 1;

    sprintf(buf, is_fr?"Argument inconnu: %s\n":"Unknown arg: %s\n", arg);
    ExitMessage(buf);
    
    return 0;
}


/* WinMain:
 *  Point d'entrée du programme appelé par l'API Win32.
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    register int i;
    char version_name[]="Teo "TO8_VERSION_STR" (Windows/DirectX)";
    char memo_name[FILENAME_LENGTH+1]="\0";
    int alleg_depth, argc=0;
#ifndef __MINGW32__
    char *argv[16];
#else
    char **argv;
#endif
    int gfx_mode=NO_GFX;
    int windowed_mode=FALSE;
    int load_state = FALSE;
    int njoy = 0;
    xargs xargs;
    TCHAR sapfs_exe[MAX_PATH];

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
	++argv; --argc;	
#endif
    
    /* traitement des paramètres */
    for (i=0; i<argc; i++)
    {
	if (!strcmp(argv[i],"-m") && i<argc-1)
        strcpy(memo_name, argv[++i]);
        else if (!strcmp(argv[i],"-fast"))
            teo.exact_speed=FALSE;
        else if (!strcmp(argv[i],"-nosound"))
            teo.sound_enabled=FALSE;
        else if (!strcmp(argv[i],"-nojoy"))
            njoy = -1;
        else if (!strcmp(argv[i],"-mode40"))
            gfx_mode=GFX_MODE40;
        else if (!strcmp(argv[i],"-mode80"))
            gfx_mode=GFX_MODE80;
        else if (!strcmp(argv[i],"-truecolor"))
            gfx_mode=GFX_TRUECOLOR;
        else if (!strcmp(argv[i],"-window"))
            gfx_mode=GFX_WINDOW;
        else if (!strcmp(argv[i],"-loadstate"))
            load_state = TRUE;
    }
    
    /* arguments supplémentaires */
    xargs_init(&xargs);
    xargs.tmpFile    = tmpFile;
    xargs.sysExec    = sysExec;
    xargs.isFile     = isFile;
    xargs.isDir      = isDir;
    xargs.rmFile     = rmFile;
    xargs.unknownArg = unknownArg;
    if(*sapfs_exe)
    xargs.sapfs      = shortpath(sapfs_exe);
    xargs_parse(&xargs, argc, argv);

    /* sélection du mode graphique */
    if (gfx_mode == NO_GFX) {
        if (load_state)
            SelectGraphicMode(&gfx_mode, NULL);
        else
            SelectGraphicMode(&gfx_mode, &load_state);
    }

    /* l'initialisation de l'interface clavier, qui utilise un appel GDI, doit avoir lieu
       avant celle du module clavier d'Allegro, basé sur DirectInput */
    InitKeybint();

    /* initialisation de la librairie Allegro */
    set_uformat(U_ASCII);  /* pour les accents Latin-1 */
    allegro_init();
    set_config_file(TEO_CONFIG_FILE);
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
    njoy = MIN(TO8_NJOYSTICKS, num_joysticks);

    /* initialisation de l'émulateur */
    printf(is_fr?"Initialisation de l'‚mulateur...":"Emulator initialization...");

    if (to8_Init(TO8_NJOYSTICKS-njoy) == TO8_ERROR)
        ExitMessage(to8_error_msg);

    printf("ok\n");

    /* initialisation du son */
    InitSound(51200);  /* 51200 Hz car 25600 Hz provoque des irrégularités du timer */

    /* initialisation des joysticks */
    InitJoyint(njoy);
    
    /* initialisation du mode graphique */
    switch(gfx_mode)
    {
        case GFX_MODE40:
            if (!InitGraphic(GFX_MODE40, 8, GFX_AUTODETECT_FULLSCREEN, FALSE))
                ExitMessage(is_fr?"Mode graphique non supporté.":"Unsupported graphic mode");
            break;

        case GFX_MODE80:
            if (!InitGraphic(GFX_MODE80, 8, GFX_AUTODETECT_FULLSCREEN, FALSE))
                ExitMessage(is_fr?"Mode graphique non supporté.":"Unsupported graphic mode");
            break;

        case GFX_TRUECOLOR:
            if (!InitGraphic(GFX_TRUECOLOR, 15, GFX_AUTODETECT_FULLSCREEN, FALSE))
                if (!InitGraphic(GFX_TRUECOLOR, 16, GFX_AUTODETECT_FULLSCREEN, FALSE))
                    if (!InitGraphic(GFX_TRUECOLOR, 24, GFX_AUTODETECT_FULLSCREEN, FALSE))
                        if (!InitGraphic(GFX_TRUECOLOR, 32, GFX_AUTODETECT_FULLSCREEN, FALSE))
                            ExitMessage(is_fr?"Mode graphique non supporté.":"Unsupported graphic mode");
            break;

        case GFX_WINDOW:
            alleg_depth = desktop_color_depth();

            switch (alleg_depth)
            {
                case 8:  /* 8bpp */
                default:
                    ExitMessage(is_fr?"Mode graphique non supporté.":"Unsupported graphic mode");
                    break;

                case 16: /* 15 ou 16bpp */
                    if ( !InitGraphic(GFX_TRUECOLOR, 15, GFX_AUTODETECT_WINDOWED, TRUE) && 
                         !InitGraphic(GFX_TRUECOLOR, 16, GFX_AUTODETECT_WINDOWED, TRUE) )
                            ExitMessage(is_fr?"Mode graphique non supporté.":"Unsupported graphic mode");
                    gfx_mode = GFX_TRUECOLOR;
                    break;
 
                case 24: /* 24bpp */
                case 32: /* 32bpp */
                    if (!InitGraphic(GFX_TRUECOLOR, alleg_depth, GFX_AUTODETECT_WINDOWED, TRUE))
                        ExitMessage(is_fr?"Mode graphique non supporté.":"Unsupported graphic mode");
                    gfx_mode = GFX_TRUECOLOR;
                    break;
            }
            windowed_mode = TRUE;
            break;
    }
    
    /* initialisation de l'interface utilisateur Allegro */
    if (!windowed_mode)   
       InitGUI(version_name, gfx_mode, FALSE);

    /* Initialisation de l'imprimante */
    InitPrinter();

    /* installation de la fonction callback de retraçage de l'écran nécessaire
       pour les modes fullscreen */
    set_display_switch_callback(SWITCH_IN, RetraceCallback);

    /* on continue de tourner même sans focus car sinon la gui see bloque,
     * et le buffer son tourne sur lui même sans mise à jour et c'est moche. */
    set_display_switch_mode(SWITCH_BACKGROUND); 

    to8_ColdReset();

    if (memo_name[0])
        to8_LoadMemo7(memo_name);

    /* arguments supplémentaires  */
    xargs_start(&xargs);
	
    if (load_state)
        LoadState();

    /* et c'est parti !!! */
    RunTO8(windowed_mode);

    SaveState();

    /* nettoyage des arguments supplémentaires */
    xargs_exit(&xargs);

    /* désinstallation du callback *avant* la sortie du mode graphique */
    remove_display_switch_callback(RetraceCallback);

    /* libération de la mémoire si mode fenêtré */
    if (windowed_mode)
       FreeGUI();

    /* sortie du mode graphique */
    SetGraphicMode(SHUTDOWN);

    /* sortie de l'émulateur */
    exit(EXIT_SUCCESS);
}

