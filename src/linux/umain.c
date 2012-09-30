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
 *                          Samuel Devulder
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
 *  Module     : linux/main.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               François Mouret 26/01/2010 08/2011 23/03/2012
                                 09/06/2012 19/10/2012
 *               Samuel Devulder 07/2011
 *               Gilles Fétis 07/2011
 *
 *  Boucle principale de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <locale.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <signal.h>
   #include <unistd.h>
   #include <sys/time.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <gtk/gtk.h>
   #include <X11/Xlib.h>
   #include <X11/Xresource.h>
   #include <X11/Xutil.h>
#endif

#include "intern/defs.h"
#include "intern/printer.h"
#include "intern/xargs.h"
#include "intern/std.h"
#include "intern/ini.h"
#include "linux/disk.h"
#include "linux/display.h"
#include "linux/graphic.h"
#include "linux/sound.h"
#include "linux/gui.h"
#include "to8.h"

struct EMUTEO teo;

static int idle_data = 0;
static GTimer *timer;

static gboolean reset = FALSE;
static gchar *cass_name = NULL;
static gchar *memo_name = NULL;
static gchar *disk_name[4] = { NULL, NULL, NULL, NULL };
static gchar **remain_name = NULL;


/* RunTO8:
 *  Boucle principale de l'émulateur.
 */
static gboolean RunTO8 (gpointer user_data)
{
    static int debug = 0;
    static gulong microseconds;

    if ((teo.setting.exact_speed)
     && (teo.setting.sound_enabled == 0)
     && (g_timer_elapsed (timer, &microseconds) < 0.02))
        return TRUE;

    g_timer_stop (timer);
    g_timer_start (timer);

    if (to8_DoFrame(debug) == 0)
        teo.command=BREAKPOINT;

    if ((teo.command == BREAKPOINT)
     || (teo.command == DEBUGGER))
        debug = DebugPanel();

    if (teo.command == CONTROL_PANEL) {
        ControlPanel();
        debug = 0;
    }

    if (teo.command == RESET) {
        to8_Reset();
        debug = 0;
    }

    if (teo.command == COLD_RESET) {
        to8_ColdReset();
        debug = 0;
    }
    if (teo.command == QUIT) {
        mc6809_FlushExec();
        gtk_main_quit ();
        return FALSE;
    }

    teo.command = NONE;

    RefreshScreen();
    if ((teo.setting.exact_speed)
     && (teo.setting.sound_enabled))
        PlaySoundBuffer();

    return TRUE;
    (void)user_data;
}



/* DisplayMessage:
 *  Affiche un message.
 */
static void DisplayMessage(const char msg[])
{
    fprintf(stderr, "%s\n", msg);
    error_box (msg, NULL);
}



/* ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
static void ExitMessage(const char msg[])
{
    DisplayMessage(msg);
    exit(EXIT_FAILURE);
}



/* ReadCommandLine:
 *  Lit la ligne de commande
 */
static int ReadCommandLine(int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] = {
        { "reset", 'r', 0, G_OPTION_ARG_NONE, &reset,
           is_fr?"Reset Ã  froid de l'Ã©mulateur"
                :"Cold-reset emulator", NULL },
        { "disk0", '0', 0, G_OPTION_ARG_FILENAME, &disk_name[0],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)",
           is_fr?"CHEMIN":"PATH" },
        { "disk1", '1', 0, G_OPTION_ARG_FILENAME, &disk_name[1],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)",
           is_fr?"CHEMIN":"PATH" },
        { "disk2", '2', 0, G_OPTION_ARG_FILENAME, &disk_name[2],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)",
           is_fr?"CHEMIN":"PATH" },
        { "disk3", '3', 0, G_OPTION_ARG_FILENAME, &disk_name[3],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)",
           is_fr?"CHEMIN":"PATH" },
        { "cass", 0, 0, G_OPTION_ARG_FILENAME, &cass_name,
           is_fr?"Charge une cassette":"Load a tape",
           is_fr?"FICHIER":"FILE" },
        { "memo", 0, 0, G_OPTION_ARG_FILENAME, &memo_name,
           is_fr?"Charge une cartouche":"Load a cartridge",
           is_fr?"FICHIER":"FILE" },
        { G_OPTION_REMAINING, 0, G_OPTION_FLAG_IN_MAIN,
          G_OPTION_ARG_FILENAME_ARRAY, &remain_name, "", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    context = g_option_context_new ("- test tree model performance");
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, FALSE);
//    g_option_context_set_description (context, "description");
//    g_option_context_set_summary (context, "summary");
    if (g_option_context_parse (context, &argc, &argv, &error) == FALSE)
        ExitMessage (error->message);


    /* Lecture des arguments supplémentaires */
    xargs_init(&xargs);
    xargs.tmpFile    = tmpFile;
    xargs.sysExec    = sysExec;
    xargs.rmFile     = rmFile;
    xargs.sapfs      = "sapfs";
    xargs.exitMsg = (void*)ExitMessage;
//    xargs_parse(&xargs, argc-1, argv+1);
    if (


    
    return 0;
}



/* sysexec:
 *  Demande à l'OS d'executer une cmd dans un dossier précis.
 */
static void sysExec(char *cmd, const char *dir) {
    char cwd[MAX_PATH]="";
    char *tmp;
    int i;
    
    tmp = getcwd(cwd, MAX_PATH);
    i = chdir(dir);
    i = system(cmd);
    i = chdir(cwd);
    (void)i;
    (void)tmp;
}


/* rmFile:
 *   Efface un fichier.
 */
static void rmFile(char *path) {
    unlink(path);
}


/* tmpFile:
 *   Cree un fichier temporaire.
 */
static char *tmpFile(char *buf, int maxlen) {
#ifdef OS_LINUX
    int tmp;
    tmp = mkstemp(buf);
#else
    char *tmp;
    tmp = tmpnam(buf);
#endif
    strcat(buf, ".sap");
    return buf;
    (void)maxlen;
    (void)tmp;
}


#define IS_3_INCHES(drive) ((drive_type[drive]>2) && (drive_type[drive]<7))


/* main:
 *  Point d'entrée du programme appelé par Linux.
 */
int main(int argc, char *argv[])
{
    int i;
    int direct_write_support = FALSE;
    int drive_type[4];
    char *lang;
    int reset = FALSE;
    int ini_load_error = 0;
    xargs xargs;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";
#endif

    /* Repérage du language utilisée */
    lang=getenv("LANG");
    if (lang==NULL) lang="fr_FR";        
    setlocale(LC_ALL, "");
    is_fr = (strncmp(lang,"fr",2)==0) ? -1 : 0;

#ifdef DEBIAN_BUILD
    /* Création du répertoire pour Teo (tous les droits) */
    (void)snprintf (fname, MAX_PATH, "%s/.teo", getenv("HOME"));
    if (access (fname, F_OK) < 0)
        (void)mkdir (fname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
 
    /* Initialisation gtk */
    gtk_init(&argc, &argv);

    /* Charge les paramètres par défaut */
    ini_load_error = ini_Load();

    /* Lit la ligne de commande */
    ini_load_error += ReadCommandLine (argc, argv);

/*
        warning_box (is_fr?"Un fichier de configuration n'a pas pu Ãªtre " \
                           "chargÃ©. VÃ©rifiez qu'il n'a pas Ã©tÃ© dÃ©placÃ©, " \
                           "dÃ©truit et que le pÃ©riphÃ©rique a bien Ã©tÃ© montÃ©."
                          :"A configuration file was unable to be loaded. " \
                           "Check if this file has been moved, deleted and that " \
                           "the media has been successfully mounted.", NULL);
*/

    /* Affichage du message de bienvenue du programme */
    printf((is_fr?"Voici %s l'Ã©mulateur Thomson TO8.\n"
                 :"Here's %s the thomson TO8 emulator.\n"),
                 "Teo "TO8_VERSION_STR" (Linux/X11)");
    printf("Copyright (C) 1997-2012 Gilles FÃ©tis, Eric Botcazou, \
            Alexandre Pukall, FranÃ§ois Mouret, Samuel Devulder.\n\n");
    printf((is_fr?"Touches: [ESC] Panneau de contrÃ´le\n"
                 :"Keys : [ESC] Control pannel\n"));
    printf((is_fr?"         [F12] DÃ©bogueur\n\n"
                 :"       [F12] Debugger\n\n"));

    /* Initialisation du TO8 */
    printf((is_fr?"Initialisation de l'Ã©mulateur..."
                 :"Initialization of the emulator..."));
    fflush(stdout);

    if ( to8_Init(TO8_NJOYSTICKS) == TO8_ERROR )
        ExitMessage(to8_error_msg);

    /* Initialisation de l'interface d'accès direct */
    InitDirectDisk (drive_type, direct_write_support);

    /* Détection des lecteurs supportés (3"5 seulement) */
    for (i=0; i<4; i++)
        teo.disk[i].direct_access_allowed = (IS_3_INCHES(i)) ? 1 : 0;

    /* Création de la fenêtre principale de l'émulateur */
    InitWindow ();

    /* Initialisation du serveur X */
    InitDisplay();

    /* Initialisation des modules graphique, sonore et disquette */
    InitGraphic();

    if (InitSound() == TO8_ERROR)
        DisplayMessage(to8_error_msg);

    to8_ColdReset();

#if 0
    if (!reset)
    {
        /* Charge les disquettes */
        for (i=0;i<NBDRIVE;i++)
        {
            if (disk_name[i] !=NULL)
            {
                if (std_isdir (disk_name[i]))
                     xargs_parse (&xargs, disk_name[i]);
              
                to8_LoadDisk(i, disk_name[i]);
            }
        }
        /* Charge la cartouche */
        if (memo_name !=NULL)
            to8_LoadMemo(memo_name);

        /* Charge la cassette */
        if (cass_name !=NULL)
            to8_LoadCass(cass_name);

        /* Charge l'image de sauvegarde */
        if (access("autosave.img", F_OK) >= 0)
            to8_LoadImage("autosave.img");

        /* Charge les options non définies */
        for (i=0;remain_name[i]!=NULL;i++)
        {
            if (std_isdir (disk_name[i]))
                xargs_parse (&xargs, disk_name[i]);
                to8_LoadDisk(i, disk_name[i]);
            }
        }
    }
#endif

    /* Initialise l'interface graphique */
    InitGUI ();

    /* Et c'est parti !!! */
    printf((is_fr?"Lancement de l'Ã©mulation...\n":"Launching emulation...\n"));
    teo.command=NONE;
    timer = g_timer_new ();
    g_timeout_add (2, RunTO8, &idle_data);
    gtk_main ();
    g_idle_remove_by_data (&idle_data);
    g_timer_destroy (timer);

    /* Mise au repos de l'interface d'accès direct */
    ExitDirectDisk();

    /* nettoyage des arguments supplementaires */
    xargs_exit(&xargs);

    /* Libère la mémoire utilisée par la GUI */
    FreeGUI ();

    /* Referme le périphérique audio*/
    CloseSound();

    /* Sortie de l'émulateur */
    printf((is_fr?"\nA bientÃ´t !\n":"\nGoodbye !\n"));
    exit(EXIT_SUCCESS);
}

