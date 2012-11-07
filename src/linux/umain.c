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

#include "defs.h"
#include "to8.h"
#include "option.h"
#include "image.h"
#include "main.h"
#include "std.h"
#include "ini.h"
#include "media/disk.h"
#include "media/cass.h"
#include "media/memo.h"
#include "media/printer.h"
#include "linux/floppy.h"
#include "linux/display.h"
#include "linux/graphic.h"
#include "linux/sound.h"
#include "linux/gui.h"


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
        teo.command=TEO_COMMAND_BREAKPOINT;

    if ((teo.command == TEO_COMMAND_BREAKPOINT)
     || (teo.command == TEO_COMMAND_DEBUGGER))
        debug = udebug_Panel();

    if (teo.command == TEO_COMMAND_PANEL) {
        ugui_Panel();
        debug = 0;
    }

    if (teo.command == TEO_COMMAND_RESET) {
        to8_Reset();
        debug = 0;
    }

    if (teo.command == TEO_COMMAND_COLD_RESET) {
        to8_ColdReset();
        debug = 0;
    }
    if (teo.command == TEO_COMMAND_QUIT) {
        mc6809_FlushExec();
        gtk_main_quit ();
        return FALSE;
    }

    teo.command = TEO_COMMAND_NONE;

    ugraphic_Refresh ();
    if ((teo.setting.exact_speed)
     && (teo.setting.sound_enabled))
        usound_Play ();

    return TRUE;
    (void)user_data;
}



/* ReadCommandLine:
 *  Lit la ligne de commande
 */
static void ReadCommandLine(int argc, char *argv[])
{
    int i;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] = {
        { "reset", 'r', 0, G_OPTION_ARG_NONE, &reset,
           is_fr?"Reset Ã  froid de l'Ã©mulateur"
                :"Cold-reset emulator", NULL },
        { "disk0", '0', 0, G_OPTION_ARG_FILENAME, &disk_name[0],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"CHEMIN":"PATH" },
        { "disk1", '1', 0, G_OPTION_ARG_FILENAME, &disk_name[1],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"CHEMIN":"PATH" },
        { "disk2", '2', 0, G_OPTION_ARG_FILENAME, &disk_name[2],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"CHEMIN":"PATH" },
        { "disk3", '3', 0, G_OPTION_ARG_FILENAME, &disk_name[3],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"CHEMIN":"PATH" },
        { "cass", 0, 0, G_OPTION_ARG_FILENAME, &cass_name,
           is_fr?"Charge une cassette":"Load a tape", is_fr?"FICHIER":"FILE" },
        { "memo", 0, 0, G_OPTION_ARG_FILENAME, &memo_name,
           is_fr?"Charge une cartouche":"Load a cartridge",
           is_fr?"FICHIER":"FILE" },
        { G_OPTION_REMAINING, 0, G_OPTION_FLAG_IN_MAIN, 
          G_OPTION_ARG_FILENAME_ARRAY, &remain_name, "", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    /* Lit la ligne de commande */
    context = g_option_context_new (is_fr?"[FICHIER...] [REPERTOIRE...]"
                                         :"[FILE...] [FOLDER...]");
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    g_option_context_set_description (context, is_fr
         ?"Options non dÃ©finies :\n  Charge cassette, disquette et cartouche\n"
         :"Undefined options :\n  load tape, disk and cartridge\n");
    if (g_option_context_parse (context, &argc, &argv, &error) == FALSE)
        main_ExitMessage (error->message);
   
    /* Transfert des chaînes dans la structure générale */
    /* Obligé de faire comme ça : les chaînes de la structure
       générale peuvent avoir été initialisées par la lecture
       du fichier de configuration, et malheureusement
       g_option_context_parse() ne libère pas la mémoire avant
       de l'allouer pour une nouvelle chaîne */
    for (i=0;i<NBDRIVE;i++) {
        if (disk_name[i] != NULL) {
            teo.disk[i].file = std_free (teo.disk[i].file);
            teo.disk[i].file = std_strdup_printf ("%s", disk_name[i]);
            g_free (disk_name[i]);
        }
    }
    if (cass_name != NULL) {
        teo.cass.file = std_free (teo.cass.file);
        teo.cass.file = std_strdup_printf ("%s", cass_name);
        g_free (cass_name);
    }
    if (memo_name != NULL) {
        teo.memo.file = std_free (teo.memo.file);
        teo.memo.file = std_strdup_printf ("%s", memo_name);
        g_free (memo_name);
    }
    g_option_context_free(context);
}



/* main_SysExec:
 *  Demande à l'OS d'executer une cmd dans un dossier précis.
 */
int main_SysExec(char *cmd, const char *dir)
{
    int i;
    char cwd[MAX_PATH+1]="";
    int ret = FALSE;
    
    if (getcwd(cwd, MAX_PATH) != NULL)
    {
        i = chdir(dir);
        if (system(cmd) == 0)
            ret = TRUE;
        i = chdir(cwd);
        printf ("cwd='%s' dir='%s' cmd='%s'\n", cwd, dir, cmd); 
    }
    return ret;
    (void)i;
}



/* main_RmFile:
 *   Efface un fichier.
 */
int main_RmFile(char *path)
{
    return (unlink(path) == 0) ? TRUE : FALSE;
}



/* main_TmpFile:
 *   Cree un fichier temporaire.
 */
char *main_TmpFile(char *buf, int maxlen)
{
#ifdef OS_LINUX
    char tmp_name[] = "/tmp/teo_XXXXXX";
    char *tmp = tmp_name;

    if (mkstemp(tmp_name) >= 0)
        (void)snprintf (buf, maxlen, "%s", tmp_name);
    else
        tmp = NULL;
#else
    char *tmp;
    tmp = tmpnam(buf);
#endif
    return tmp;
    (void)maxlen;
}



/* DisplayMessage:
 *  Affiche un message.
 */
void main_DisplayMessage(const char msg[])
{
    fprintf(stderr, "%s\n", msg);
    ugui_Error
     (msg, NULL);
}



/* ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
void main_ExitMessage(const char msg[])
{
    main_DisplayMessage(msg);
    exit(EXIT_FAILURE);
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

#ifdef DEBIAN_BUILD
    char *fname = NULL;
#endif

    /* Repérage du language utilisé */
    lang=getenv("LANG");
    if (lang==NULL) lang="fr_FR";        
    setlocale(LC_ALL, "");
    is_fr = (strncmp(lang,"fr",2)==0) ? -1 : 0;

#ifdef DEBIAN_BUILD
    /* Création du répertoire pour Teo (tous les droits) */
    fname = std_strdup_printf ("%s/.teo", getenv("HOME"));
    if (access (fname, F_OK) < 0)
        (void)mkdir (fname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    fname = std_free (fname);
#endif
 
    gtk_init (&argc, &argv);     /* Initialisation gtk */
    ini_Load();                  /* Charge les paramètres par défaut */
    ReadCommandLine(argc, argv); /* Récupération des options */

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
    printf("Copyright (C) 1997-2012 Gilles FÃ©tis, Eric Botcazou,\
 Alexandre Pukall, FranÃ§ois Mouret, Samuel Devulder.\n\n");
    printf((is_fr?"Touches: [ESC] Panneau de contrÃ´le\n"
                 :"Keys : [ESC] Control pannel\n"));
    printf((is_fr?"         [F12] DÃ©bogueur\n\n"
                 :"       [F12] Debugger\n\n"));

    /* Initialisation du TO8 */
    printf((is_fr?"Initialisation de l'Ã©mulateur..."
                 :"Initialization of the emulator...")); fflush(stdout);

    if ( to8_Init(TO8_NJOYSTICKS) < 0 )
        main_ExitMessage(to8_error_msg);

    /* Initialisation de l'interface d'accès direct */
    ufloppy_Init (drive_type, direct_write_support);

    /* Détection des lecteurs supportés (3"5 seulement) */
    for (i=0; i<4; i++)
        teo.disk[i].direct_access_allowed = (IS_3_INCHES(i)) ? 1 : 0;

    udisplay_Window (); /* Création de la fenêtre principale */
    udisplay_Init();    /* Initialisation du serveur X */
    ugraphic_Init();    /* Initialisation du module graphique */
    disk_FirstLoad ();  /* Chargement des disquettes éventuelles */
    cass_FirstLoad ();  /* Chargement de la cassette éventuelle */
    memo_FirstLoad ();  /* Chargement de la cartouche éventuelle */

    /* Chargement des options non définies */
    for (i=0;(remain_name!=NULL)&&(remain_name[i]!=NULL);i++)
        option_Undefined (remain_name[i]);
    g_strfreev(remain_name); /* Libère la mémoire des options indéfinies */

    /* Initialise le son */
    if (usound_Init() < 0)
        main_DisplayMessage(to8_error_msg);

    /* Restitue l'état sauvegardé de l'émulateur */
    to8_ColdReset();    /* Reset à froid de l'émulateur */
    if (!reset)
        if (access("autosave.img", F_OK) >= 0)
            image_Load ("autosave.img");

    ugui_Init();      /* Initialise l'interface graphique */

    /* Et c'est parti !!! */
    printf((is_fr?"Lancement de l'Ã©mulation...\n":"Launching emulation...\n"));
    teo.command=TEO_COMMAND_NONE;
    timer = g_timer_new ();
    g_timeout_add (2, RunTO8, &idle_data);
    gtk_main ();
    g_idle_remove_by_data (&idle_data);
    g_timer_destroy (timer);

    ufloppy_Exit(); /* Mise au repos de l'interface d'accès direct */
    ugui_Free ();   /* Libère la mémoire utilisée par la GUI */
    usound_Close(); /* Referme le périphérique audio*/

    /* Sortie de l'émulateur */
    printf((is_fr?"\nA bientÃ´t !\n":"\nGoodbye !\n"));
    exit(EXIT_SUCCESS);
}

