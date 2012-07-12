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
 *               François Mouret 26/01/2010 08/2011 23/03/2012 09/06/2012
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
#include "intern/gui.h"
#include "linux/disk.h"
#include "linux/display.h"
#include "linux/graphic.h"
#include "linux/gui.h"
#include "linux/sound.h"
#include "to8.h"
#include "xargs.h"

struct EmuTO teo={
    True,
    NONE
};

static int idle_data = 0;

#define OP_TABLE_SIZE  10
static XrmOptionDescRec op_table[OP_TABLE_SIZE]={
{"-help"                , ".help"     , XrmoptionIsArg , NULL  },
{"-m"                   , ".memo7"    , XrmoptionSepArg, NULL  },
{"-fast"                , ".speed"    , XrmoptionNoArg , "fast"},
{"-nosound"             , ".sound"    , XrmoptionNoArg , "off" },
{"-geometry"            , ".geometry" , XrmoptionSepArg, NULL  },
{"-noshm"               , ".shm"      , XrmoptionNoArg , "off" },
{"-display"             , ".display"  , XrmoptionSepArg, NULL  },
{"--enable-direct-write", ".direct"   , XrmoptionNoArg , "on"  },
{"-loadstate"           , ".loadstate", XrmoptionIsArg , NULL  },
{"-xrm"                 , NULL        , XrmoptionResArg, NULL  }
};

static GTimer *timer;


/* RunTO8:
 *  Boucle principale de l'émulateur.
 */
static gboolean RunTO8 (gpointer user_data)
{
    static int debug = 0;
    static gulong microseconds;

    if ((gui->setting.exact_speed)
     && (teo.sound_enabled == 0)
     && (g_timer_elapsed (timer, &microseconds) < 0.02))
        return TRUE;

    g_timer_stop (timer);
    g_timer_start (timer);

    if (to8_DoFrame(debug) == 0)
        teo.command=BREAKPOINT;
    debug=debug;

    if ((teo.command == BREAKPOINT)
     || (teo.command == DEBUGGER))
        debug = DebugPanel();

    if (teo.command == CONTROL_PANEL)
    {
        ControlPanel();
        debug = 0;
    }

    if (teo.command == RESET)
    {
        to8_Reset();
        debug = 0;
    }

    if (teo.command == COLD_RESET)
    {
        to8_ColdReset();
        debug = 0;
    }
    if (teo.command == QUIT)
    {
        mc6809_FlushExec();
        gtk_main_quit ();
        return FALSE;
    }

    teo.command = NONE;

    RefreshScreen();
    if ((gui->setting.exact_speed) && (teo.sound_enabled))
        PlaySoundBuffer();

    return TRUE;
    (void)user_data;
}


/* GetUserInput:
 *  Lit toutes les options spécifiées par l'utilisateur dans
 *  l'ordre de priorité décroissante suivant:
 *    - ligne de commande,
 *    - ressource de l'écran mise en place par xrdb,
 *    - fichier .Xdefaults dans le répertoire utilisateur,
 *  et les fusionne dans la base de données user_db.
 */
static XrmDatabase GetUserInput(int *argc, char *argv[])
{
    char file_name[MAX_PATH+1];
    char *str_type[20];
    XrmDatabase commandline_db=NULL, user_db;
    XrmValue value;

    XrmParseCommand(&commandline_db,op_table,OP_TABLE_SIZE,PROG_NAME,argc,argv);

    /* Demande d'aide ? */
    if (XrmGetResource(commandline_db,PROG_NAME".help",PROG_CLASS".Help",str_type, &value))
    {
    if (is_fr) {
        printf("Usage:\n");
        printf("           %s [options...]\n",argv[0]);
        printf("oÃ¹ les options sont prises parmi les suivantes:\n");
        printf("    -help                   affiche cette aide\n");
        printf("    -m file.m7              place la cartouche file.m7 dans le lecteur\n");
        printf("    -fast                   active la pleine vitesse de l'Ã©mulateur\n");
        printf("    -nosound                supprime le son de l'Ã©mulateur\n");
        printf("    -geometry +x+y          spÃ©cifie la position de la fenÃªtre\n");
        printf("    -noshm                  dÃ©sactive l'utilisation de l'extension MIT-SHM\n");
        printf("    -display displayname    spÃ©cifie le serveur X Ã  utiliser\n");
        printf("    -loadstate              charge le dernier Ã©tat sauvegardÃ© de l'Ã©mulateur\n");
    } else {
        printf("Usage:\n");
        printf("           %s [options...]\n",argv[0]);
        printf("Where options are:\n");
        printf("    -help                   this help message\n");
        printf("    -m file.m7              loads a ROM cart from file.m7\n");
        printf("    -fast                   fastest speed of emulation\n");
        printf("    -nosound                disable sound support\n");
        printf("    -geometry +x+y          set absolute position of window\n");
        printf("    -noshm                  disable MIT-SHM extension\n");
        printf("    -display displayname    set another X server\n");
        printf("    -loadstate              loads last state of emulation\n");
    }    
        exit(EXIT_SUCCESS);
    }

    /* On a besoin de display pour accéder à la configuration utilisateur */
    InitDisplay();

    if (XScreenResourceString(DefaultScreenOfDisplay(display)) != NULL)
        user_db=XrmGetStringDatabase(XScreenResourceString(DefaultScreenOfDisplay(display)));
    else if (XResourceManagerString(display) != NULL)
        user_db=XrmGetStringDatabase(XResourceManagerString(display));
    else
    {
        strcat(strcpy(file_name,getenv("HOME")),"./Xdefaults");
        user_db=XrmGetFileDatabase(file_name);
    }

    XrmMergeDatabases(commandline_db, &user_db);

    return user_db;
}



/* SetParameters:
 *  Fixe les paramètres de l'émulation, à partir des options de
 *  l'utilisateur ou par défaut.
 */
static int SetParameters(char memo_name[], int *x, int *y, int *user_flags, int *direct_write_support, XrmDatabase user_db)
{
    char *str_type[20], buffer[20];
    int flags;
    XrmValue value;
    unsigned int w,h;

    /* Fichier qui décrit la cartouche placée dans le lecteur */
    if (XrmGetResource(user_db,PROG_NAME".memo7",PROG_CLASS".Memo7", str_type, &value))
        strncpy(memo_name, value.addr, value.size);

    /* Vitesse de l'émulation: réelle (celle du TO8) ou rapide (celle du PC) */
    if (XrmGetResource(user_db,PROG_NAME".speed",PROG_CLASS".Speed", str_type, &value))
    {
        if (!strncmp(value.addr,"fast",value.size))
            gui->setting.exact_speed=FALSE;
        else if (strncmp(value.addr,"true",value.size))
        {
            fprintf(stderr,is_fr?"%s: spÃ©cification de vitesse invalide (voir %s -help)\n"
                                :"%s: invalid speed parameter (see %s -help)\n", PROG_NAME, PROG_NAME);
            exit(EXIT_FAILURE);
        }
    }

    /* Activation de l'émulation sonore */
    if (XrmGetResource(user_db,PROG_NAME".sound",PROG_CLASS".Sound", str_type, &value))
    {
        if (!strncmp(value.addr,"off",value.size))
            teo.sound_enabled=False;
        else if (strncmp(value.addr,"on",value.size))
        {
            fprintf(stderr,is_fr?"%s: spÃ©cification de l'émulation sonore invalide (voir %s -help)\n"
                                :"%s: invalid sound parameter (see %s -help)\n", PROG_NAME, PROG_NAME);
            exit(EXIT_FAILURE);
        }
    }

    /* Géométrie de la fenêtre */
    if (XrmGetResource(user_db,PROG_NAME".geometry",PROG_CLASS".Geometry",str_type, &value))
    {
        strncpy(buffer, value.addr, value.size);
        flags=XParseGeometry(buffer, x, y, &w, &h);

    /* position x */
        if (flags&XValue)
        {
            *user_flags|=USPosition;

            if (flags&XNegative)
                *x=DisplayWidth(display,screen)+*x-TO8_SCREEN_W*2;
        }

    /* position y */
        if (flags&YValue)
        {
            *user_flags|=USPosition;

            if (flags&YNegative)
                *y=DisplayHeight(display,screen)+*y-TO8_SCREEN_H*2;
        }
    }

    /* Activation de l'extension MIT-SHM */
    if (XrmGetResource(user_db,PROG_NAME".shm",PROG_CLASS".Shm",str_type, &value))
    {
        if (!strncmp(value.addr,"off",value.size))
            mit_shm_enabled=False;
        else if (strncmp(value.addr,"on",value.size))
        {
            fprintf(stderr,is_fr?"%s: spÃ©cification de l'extension MIT-SHM invalide\n"
                                :"%s: invalid MIT-SHM extension\n",PROG_NAME);
            exit(EXIT_FAILURE);
        }
    }

    /* Autorisation d'écriture directe */
    if (XrmGetResource(user_db,PROG_NAME".direct",PROG_CLASS".direct",str_type, &value))
    {
        if (!strncmp(value.addr,"on",value.size))
            *direct_write_support = True;
    }

    /* Chargement du dernier état sauvegardé de l'émulateur */
    if (XrmGetResource(user_db,PROG_NAME".loadstate",PROG_CLASS".loadstate",str_type, &value))
        return TRUE;

    return FALSE;
}



/* ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
static void DisplayMessage(const char msg[])
{
    GtkWidget *dialog;

    fprintf(stderr, "%s\n", msg);
    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg);
    gtk_window_set_title (GTK_WINDOW(dialog), is_fr?"Teo - Erreur":"Teo - Error");
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}



/* ExitMessage:
 *  Affiche un message de sortie et sort du programme.
 */
static void ExitMessage(const char msg[])
{
    DisplayMessage(msg);
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


/* unknownArg:
 *  traite les arguments inconnus.
 */
static int unknownArg(char *arg) {
    char buf[256];
        
    if(!strcmp(arg,"-help")      ||
       !strcmp(arg,"-m")         ||
       !strcmp(arg,"-fast")      ||
       !strcmp(arg,"-nosound")   ||
       !strcmp(arg,"-geometry")  ||
       !strcmp(arg,"-noshm")     ||
       !strcmp(arg,"-display")   ||
       !strcmp(arg,"-loadstate"))
            return 1;

    sprintf(buf, is_fr?"Argument inconnu: %s\n":"Unknown arg: %s\n", arg);
    ExitMessage(buf);
    
    return 0;
}

#define IS_3_INCHES(drive) ((drive_type[drive]>2) && (drive_type[drive]<7))


/* main:
 *  Point d'entrée du programme appelé par Linux.
 */
int main(int argc, char *argv[])
{
    int x=0, y=0, user_flags=0;

    int i;
    char version_name[]="Teo "TO8_VERSION_STR" (Linux/X11)";
    char memo_name[MAX_PATH+1]="\0";
    int direct_write_support = FALSE;
    int drive_type[4];
    char *lang;
    int load_state = FALSE;
    XrmDatabase user_db;
    xargs xargs;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";
#endif
    /* Initialisation du serveur X */ 
    lang=getenv("LANG");
    if (lang==NULL) lang="fr_FR";        
    setlocale(LC_ALL, "fr_FR.UTF8");    
    if (strncmp(lang,"fr",2)==0) 
        is_fr=-1;
    else
        is_fr=0;
         
    gtk_init(&argc, &argv);

    /* Initialisation du module de gestion des ressources */
    XrmInitialize();

    /* Mise en commun dans user_db toutes les commandes et options spécifiées
       par l'utilisateur */
    user_db = GetUserInput(&argc, argv);

    /* Lecture des arguments supplémentaires */
    xargs_init(&xargs);
    xargs.tmpFile    = tmpFile;
    xargs.sysExec    = sysExec;
    xargs.isFile     = isFile;
    xargs.isDir      = isDir;
    xargs.rmFile     = rmFile;
    xargs.unknownArg = unknownArg;
    xargs.sapfs      = "sapfs";
    xargs.exitMsg = (void*)ExitMessage;
    xargs_parse(&xargs, argc-1, argv+1);

    /* Mise en place des paramètres de l'émulation */
    load_state = SetParameters(memo_name, &x, &y, &user_flags, &direct_write_support, user_db);
    XrmDestroyDatabase(user_db);

    /* Affichage du message de bienvenue du programme */
    printf((is_fr?"Voici %s l'Ã©mulateur Thomson TO8.\n":"Here's %s the thomson TO8 emulator.\n"),version_name);
    printf("Copyright (C) 1997-2012 Gilles FÃ©tis, Eric Botcazou, Alexandre Pukall, FranÃ§ois Mouret, Samuel Devulder.\n\n");
    printf((is_fr?"Touches: [ESC] Panneau de contrÃ´le\n":"Keys : [ESC] Control pannel\n"));
    printf((is_fr?"         [F12] DÃ©bogueur\n\n":"     : [F12] Debugger\n\n"));

    /* Initialisation du TO8 */
    printf((is_fr?"Initialisation de l'Ã©mulateur...":"Initialization of the emulator..."));
    fflush(stdout);

    if ( to8_Init(TO8_NJOYSTICKS) == TO8_ERROR )
        ExitMessage(to8_error_msg);

    /* Chargement de la cartouche */
    if (memo_name[0])
        to8_LoadMemo7(memo_name);

    /* Initialisation de l'interface d'accès direct */
    InitDirectDisk(drive_type, direct_write_support);

    /* Détection des lecteurs supportés (3"5 seulement) */
    for (i=0; i<4; i++)
        gui->disk[i].direct_access_allowed = (IS_3_INCHES(i)) ? 1 : 0;

    /* Création de la fenêtre principale de l'émulateur */
    InitWindow(argc, argv, x, y, user_flags);

    /* Initialisation des modules graphique, sonore et disquette */
    InitGraphic();
    if (InitSound() == TO8_ERROR)
        DisplayMessage(to8_error_msg);

    /* Initialisation de l'imprimante */
    InitPrinter();

    to8_ColdReset();
#ifdef DEBIAN_BUILD
    (void)snprintf (fname, MAX_PATH, "%s/.teo", getenv("HOME"));
    if (access (fname, F_OK) < 0)
        (void)mkdir (fname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
    if (load_state == TRUE)
        if (to8_LoadState(TEO_CONFIG_FILE) != 0)
            warning_box (is_fr?"Un fichier de configuration n'a pas pu Ãªtre " \
                               "chargÃ©. VÃ©rifiez qu'il n'a pas Ã©tÃ© dÃ©placÃ©, " \
                               "dÃ©truit et que le pÃ©riphÃ©rique a bien Ã©tÃ© montÃ©."
                              :"A configuration file was unable to be loaded. " \
                               "Check if this file has been moved, deleted and that " \
                               "the media has been successfully mounted.", NULL);

    /* arguments supplementaires  */
    xargs_start(&xargs);

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

