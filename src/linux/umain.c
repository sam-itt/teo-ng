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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               François Mouret 26/01/2010 08/2011 23/03/2012
                                 09/06/2012 19/10/2012 19/09/2013
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
#include "teo.h"
#include "option.h"
#include "image.h"
#include "errors.h"
#include "main.h"
#include "std.h"
#include "ini.h"
#include "media/disk/controlr.h"
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

    g_timer_start (timer);

    if (teo_DoFrame(debug) == 0)
        teo.command=TEO_COMMAND_BREAKPOINT;

    if ((teo.command == TEO_COMMAND_BREAKPOINT)
     || (teo.command == TEO_COMMAND_DEBUGGER))
        debug = udebug_Panel();

    if (teo.command == TEO_COMMAND_PANEL)
        ugui_Panel();

    if (teo.command == TEO_COMMAND_RESET)
        teo_Reset();

    if (teo.command == TEO_COMMAND_COLD_RESET)
        teo_ColdReset();

    if (teo.command == TEO_COMMAND_QUIT)
    {
        disk_ControllerWriteUpdateTrack();
        mc6809_FlushExec();
        gtk_main_quit ();
        return FALSE;
    }

    disk_ControllerClearWriteFlag();
    teo.command = TEO_COMMAND_NONE;

    ugraphic_Refresh ();
    if ((teo.setting.exact_speed)
     && (teo.setting.sound_enabled))
        usound_Play ();

    disk_ControllerWriteUpdateTimeout();

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
                :"Load virtual disk (drive 0)", is_fr?"FICHIER":"FILE" },
        { "disk1", '1', 0, G_OPTION_ARG_FILENAME, &disk_name[1],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"FICHIER":"FILE" },
        { "disk2", '2', 0, G_OPTION_ARG_FILENAME, &disk_name[2],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"FICHIER":"FILE" },
        { "disk3", '3', 0, G_OPTION_ARG_FILENAME, &disk_name[3],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"FICHIER":"FILE" },
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
    context = g_option_context_new (is_fr?"[FICHIER...]"
                                         :"[FILE...]");
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



#ifdef DEBIAN_BUILD
static void copy_debian_file (const char filename[])
{
    char *src_name = NULL;
    char *dst_name = NULL;
    FILE *src_file = NULL;
    FILE *dst_file = NULL;
    int c;

    src_name = std_strdup_printf ("/usr/share/teo/%s", filename);
    dst_name = std_ApplicationPath (APPLICATION_DIR, filename);
    if ((src_name != NULL) && (*src_name != '\0')
     && (dst_name != NULL) && (*dst_name != '\0')
     && (access (dst_name, F_OK) < 0))
    {
        src_file = fopen (src_name, "rb");
        dst_file = fopen (dst_name, "wb");

        while ((src_file != NULL)
            && (dst_file != NULL)
            && ((c = fgetc(src_file)) != EOF))
        {
            fputc (c, dst_file);
        }

        src_file = std_fclose (src_file);
        dst_file = std_fclose (dst_file);
    }
    src_name = std_free (src_name);
    dst_name = std_free (dst_name);
}        
#endif
            

/* ------------------------------------------------------------------------- */


/* DisplayMessage:
 *  Affiche un message.
 */
void main_DisplayMessage(const char msg[])
{
    fprintf(stderr, "%s\n", msg);
    ugui_Error (msg, NULL);
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
    int direct_write_support = TRUE;
    int drive_type[4];
    char *lang;

    /* Repérage du language utilisé */
    lang=getenv("LANG");
    if (lang==NULL) lang="fr_FR";        
    setlocale(LC_ALL, "");
    is_fr = (strncmp(lang,"fr",2)==0) ? -1 : 0;

    gtk_init (&argc, &argv);     /* Initialisation gtk */
    ini_Load();                  /* Charge les paramètres par défaut */
    ReadCommandLine(argc, argv); /* Récupération des options */

#ifdef DEBIAN_BUILD
    copy_debian_file ("empty.hfe");
#endif    

    /* Affichage du message de bienvenue du programme */
    printf((is_fr?"Voici %s l'Ã©mulateur Thomson TO8.\n"
                 :"Here's %s the thomson TO8 emulator.\n"),
                 "Teo "TEO_VERSION_STR" (Linux/X11)");
    printf("Copyright (C) 1997-2013 Gilles FÃ©tis, Eric Botcazou, "
           "Alexandre Pukall, FranÃ§ois Mouret, Samuel Devulder.\n\n");
    printf((is_fr?"Touches: [ESC] Panneau de contrÃ´le\n"
                 :"Keys : [ESC] Control pannel\n"));
    printf((is_fr?"         [F12] DÃ©bogueur\n\n"
                 :"       [F12] Debugger\n\n"));

    /* Initialisation du TO8 */
    printf((is_fr?"Initialisation de l'Ã©mulateur..."
                 :"Initialization of the emulator...")); fflush(stderr);

    if ( teo_Init(TEO_NJOYSTICKS) < 0 )
        main_ExitMessage(teo_error_msg);

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
    if (memo_FirstLoad () < 0) /* Chargement de la cartouche éventuelle */
        reset = 1;

    /* Chargement des options non définies */
    for (i=0;(remain_name!=NULL)&&(remain_name[i]!=NULL);i++)
        if (option_Undefined (remain_name[i]) == 1)
            reset = 1;
    g_strfreev(remain_name); /* Libère la mémoire des options indéfinies */

    /* Initialise le son */
    if (usound_Init() < 0)
        main_DisplayMessage(teo_error_msg);

    /* Restitue l'état sauvegardé de l'émulateur */
    teo_ColdReset();
    if (reset == 0)
        if (image_Load ("autosave.img") != 0)
            teo_ColdReset();

    /* Initialise l'interface graphique */
    ugui_Init();

    /* Et c'est parti !!! */
    printf((is_fr?"Lancement de l'Ã©mulation...\n":"Launching emulation...\n"));
    teo.command=TEO_COMMAND_NONE;
    timer = g_timer_new ();
    g_timeout_add_full (G_PRIORITY_DEFAULT, 2, RunTO8, &idle_data, NULL);
    gtk_main ();
    g_timer_destroy (timer);

    /* Sauvegarde de l'état de l'émulateur */
    ini_Save();
    image_Save ("autosave.img");

    ufloppy_Exit(); /* Mise au repos de l'interface d'accès direct */
    ugui_Free ();   /* Libère la mémoire utilisée par la GUI */
    usound_Close(); /* Referme le périphérique audio*/

    /* Sortie de l'émulateur */
    printf((is_fr?"\nA bientÃ´t !\n":"\nGoodbye !\n"));
    exit(EXIT_SUCCESS);
}

