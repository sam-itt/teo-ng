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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2018 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Fran�ois Mouret
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
 *  Version    : 1.8.5
 *  Cr�� par   : Eric Botcazou octobre 1999
 *  Modifi� par: Eric Botcazou 19/11/2006
 *               Fran�ois Mouret 26/01/2010 08/2011 23/03/2012
 *                               09/06/2012 19/10/2012 19/09/2013
 *                               13/04/2014 31/07/2016 25/10/2018
 *               Samuel Devulder 07/2011
 *               Gilles F�tis 07/2011
 *
 *  Boucle principale de l'�mulateur.
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
 *  Boucle principale de l'�mulateur.
 */
static gboolean RunTO8 (gpointer user_data)
{
    static gulong microseconds;

    if ((teo.setting.exact_speed)
     && (teo.setting.sound_enabled == 0)
     && (g_timer_elapsed (timer, &microseconds) < 0.02))
        return TRUE;

    g_timer_start (timer);

    if (teo_DoFrame() == 0)
        teo.command=TEO_COMMAND_BREAKPOINT;

    if ((teo.command == TEO_COMMAND_BREAKPOINT)
     || (teo.command == TEO_COMMAND_DEBUGGER)) {
        udebug_Panel();
        if (teo_DebugBreakPoint == NULL)
            teo_FlushFrame();
    }

    if (teo.command == TEO_COMMAND_PANEL)
        ugui_Panel();

    if (teo.command == TEO_COMMAND_RESET)
        teo_Reset();

    if (teo.command == TEO_COMMAND_COLD_RESET)
        teo_ColdReset();

    if (teo.command == TEO_COMMAND_FULL_RESET)
        teo_FullReset();

    if (teo.command == TEO_COMMAND_QUIT)
    {
        mc6809_FlushExec();
        gtk_main_quit ();
        return FALSE;
    }

    teo.command = TEO_COMMAND_NONE;

    ugraphic_Refresh ();
    if ((teo.setting.exact_speed)
     && (teo.setting.sound_enabled))
        usound_Play ();

    disk_WriteTimeout();

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
           is_fr?"Reset à froid de l'émulateur"
                :"Cold-reset emulator", NULL },
        { "disk0", '0', 0, G_OPTION_ARG_FILENAME, &disk_name[0],
           is_fr?"Charge un disque virtuel (lecteur 0)"
                :"Load virtual disk (drive 0)", is_fr?"FICHIER":"FILE" },
        { "disk1", '1', 0, G_OPTION_ARG_FILENAME, &disk_name[1],
           is_fr?"Charge un disque virtuel (lecteur 1)"
                :"Load virtual disk (drive 1)", is_fr?"FICHIER":"FILE" },
        { "disk2", '2', 0, G_OPTION_ARG_FILENAME, &disk_name[2],
           is_fr?"Charge un disque virtuel (lecteur 2)"
                :"Load virtual disk (drive 2)", is_fr?"FICHIER":"FILE" },
        { "disk3", '3', 0, G_OPTION_ARG_FILENAME, &disk_name[3],
           is_fr?"Charge un disque virtuel (lecteur 3)"
                :"Load virtual disk (drive 3)", is_fr?"FICHIER":"FILE" },
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
         ?"Options non définies :\n  Charge cassette, disquette et cartouche\n"
         :"Undefined options :\n  load tape, disk and cartridge\n");
    if (g_option_context_parse (context, &argc, &argv, &error) == FALSE)
        main_ExitMessage (error->message);
   
    /* Transfert des cha�nes dans la structure g�n�rale */
    /* Oblig� de faire comme �a : les cha�nes de la structure
       g�n�rale peuvent avoir �t� initialis�es par la lecture
       du fichier de configuration, et malheureusement
       g_option_context_parse() ne lib�re pas la m�moire avant
       de l'allouer pour une nouvelle cha�ne */
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


static void init_empty_disk(char *filename)
{
    char *src_name = NULL;
    char *dst_name = NULL;
    FILE *src_file = NULL;
    FILE *dst_file = NULL;
    int c;
    char *data_dir;

    src_name = std_GetTeoSystemFile(filename);
    dst_name = std_GetUserDataFile(filename);

    if(!std_FileExists(src_name)){
        printf("%s: File %s not found, not copying empty disk to user folder %s\n", __FUNCTION__, src_name, dst_name);
        return;
    }

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

/* thomson_take char:
 *  Convert Thomson ASCII char into UTF-8.
 */
static int thomson_take_char (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case '\x60':   /* middle line */
            strcat (pc_text, "\xef\xbb\xbf\xe2\x80\x92");
            i++;
            break;

        case '\x7e':   /* overline */
            strcat (pc_text, "\xef\xbb\xbf\xe2\x80\xbe");
            i++;
            break;

        case '\x7f':   /* full block */
            strcat (pc_text, "\xef\xbb\xbf\xe2\x96\x88");
            i++;
            break;

        default:
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
 *  Convert Thomson grave accents into UTF-8.
 */
static int thomson_accent_grave (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xc3\xa0"); i++; break;
        case 'e': strcat (pc_text, "\xc3\xa8"); i++; break;
        case 'i': strcat (pc_text, "\xc3\xac"); i++; break;
        case 'o': strcat (pc_text, "\xc3\xb2"); i++; break;
        case 'u': strcat (pc_text, "\xc3\xb9"); i++; break;
        case 'A': strcat (pc_text, "\xc3\x80"); i++; break;
        case 'E': strcat (pc_text, "\xc3\x88"); i++; break;
        case 'I': strcat (pc_text, "\xc3\x8c"); i++; break;
        case 'O': strcat (pc_text, "\xc3\x92"); i++; break;
        case 'U': strcat (pc_text, "\xc3\x99"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_acute:
 *  Convert Thomson acute accents into UTF-8.
 */
static int thomson_accent_acute (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xc3\xa1"); i++; break;
        case 'e': strcat (pc_text, "\xc3\xa9"); i++; break;
        case 'i': strcat (pc_text, "\xc3\xad"); i++; break;
        case 'o': strcat (pc_text, "\xc3\xb3"); i++; break;
        case 'u': strcat (pc_text, "\xc3\xba"); i++; break;
        case 'A': strcat (pc_text, "\xc3\x81"); i++; break;
        case 'E': strcat (pc_text, "\xc3\x89"); i++; break;
        case 'I': strcat (pc_text, "\xc3\x8d"); i++; break;
        case 'O': strcat (pc_text, "\xc3\x93"); i++; break;
        case 'U': strcat (pc_text, "\xc3\x9a"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_circ:
 *  Convert Thomson circumflex accents into UTF-8.
 */
static int thomson_accent_circ (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xc3\xa2"); i++; break;
        case 'e': strcat (pc_text, "\xc3\xaa"); i++; break;
        case 'i': strcat (pc_text, "\xc3\xae"); i++; break;
        case 'o': strcat (pc_text, "\xc3\xb4"); i++; break;
        case 'u': strcat (pc_text, "\xc3\xbb"); i++; break;
        case 'A': strcat (pc_text, "\xc3\x82"); i++; break;
        case 'E': strcat (pc_text, "\xc3\x8a"); i++; break;
        case 'I': strcat (pc_text, "\xc3\x8e"); i++; break;
        case 'O': strcat (pc_text, "\xc3\x94"); i++; break;
        case 'U': strcat (pc_text, "\xc3\x9b"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_uml:
 *  Convert Thomson diaeresis accents into UTF-8.
 */
static int thomson_accent_uml (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'a': strcat (pc_text, "\xc3\xa4"); i++; break;
        case 'e': strcat (pc_text, "\xc3\xab"); i++; break;
        case 'i': strcat (pc_text, "\xc3\xaf"); i++; break;
        case 'o': strcat (pc_text, "\xc3\xb6"); i++; break;
        case 'u': strcat (pc_text, "\xc3\xbc"); i++; break;
        case 'A': strcat (pc_text, "\xc3\x84"); i++; break;
        case 'E': strcat (pc_text, "\xc3\x8b"); i++; break;
        case 'I': strcat (pc_text, "\xc3\x8f"); i++; break;
        case 'O': strcat (pc_text, "\xc3\x96"); i++; break;
        case 'U': strcat (pc_text, "\xc3\x9c"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent_cedil:
 *  Convert Thomson cedilla into UTF-8.
 */
static int thomson_accent_cedil (char *thomson_text, int i, char *pc_text)
{
    switch (thomson_text[i])
    {
        case 'c': strcat (pc_text, "\xc3\xa7"); i++; break;
        case 'C': strcat (pc_text, "\xc3\x87"); i++; break;
        default : i = thomson_take_char (thomson_text, i, pc_text); break;
    }
    return i;
}



/* thomson_accent:
 *  Convert Thomson accent into UTF-8.
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
        case '#': strcat (pc_text, "\xc2\xa3"); i++; break;  /* pound */
        case '$': strcat (pc_text, "$"); i++; break;  /* dollar */
        case '&': strcat (pc_text, "#"); i++; break;  /* diesis */
        case ',': strcat (pc_text, "\xe2\x86\x90"); i++; break;  /* arrow left */
        case '-': strcat (pc_text, "\xe2\x86\x91"); i++; break;  /* arrow up */
        case '.': strcat (pc_text, "\xe2\x86\x92"); i++; break;  /* arrow right */
        case '/': strcat (pc_text, "\xe2\x86\x93"); i++; break;  /* arrow down */
        case '0': strcat (pc_text, "\xc2\xb0"); i++; break;
        case '1': strcat (pc_text, "\xc2\xb1"); i++; break;
        case '8': strcat (pc_text, "\xc3\xb7"); i++; break;
        case '<': strcat (pc_text, "\xc2\xbc"); i++; break;
        case '=': strcat (pc_text, "\xc2\xbd"); i++; break;
        case '>': strcat (pc_text, "\xc2\xbe"); i++; break;
        case 'j': strcat (pc_text, "\xc5\x92"); i++; break;
        case 'z': strcat (pc_text, "\xc5\x93"); i++; break;
        case '{': strcat (pc_text, "\xc3\x9f"); i++; break;
        case '\'':strcat (pc_text, "\xc2\xa7"); i++; break;
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
 *  Convert Thomson string into UTF-8.
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

            default:       /* ASCII character */
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
    fprintf(stderr, "%s\n", msg);
    ugui_Error (msg, wMain);
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
 *  Point d'entr�e du programme appel� par Linux.
 */
int main(int argc, char *argv[])
{
    int i;
    int direct_write_support = TRUE;
    int drive_type[4];
    char *lang;

    /* Rep�rage du language utilis� */
    lang=getenv("LANG");
    if (lang==NULL) lang="fr_FR";        
    setlocale(LC_ALL, "");
    is_fr = (strncmp(lang,"fr",2)==0) ? -1 : 0;

    g_setenv ("GDK_BACKEND", "x11", TRUE);
    gtk_init (&argc, &argv);     /* Initialisation gtk */
    ini_Load();                  /* Charge les param�tres par d�faut */
    ReadCommandLine(argc, argv); /* R�cup�ration des options */

    init_empty_disk("empty.hfe");

    /* Affichage du message de bienvenue du programme */
    printf((is_fr?"Voici %s l'émulateur Thomson TO8.\n"
                 :"Here's %s the thomson TO8 emulator.\n"),
                 "Teo "TEO_VERSION_STR" (Linux/X11)");
    printf("Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, "
           "Alexandre Pukall, François Mouret, Samuel Devulder.\n\n");
    printf((is_fr?"Touches: [ESC] Panneau de contrôle\n"
                 :"Keys : [ESC] Control pannel\n"));
    printf((is_fr?"         [F12] Débogueur\n\n"
                 :"       [F12] Debugger\n\n"));

    /* Initialisation du TO8 */
    printf((is_fr?"Initialisation de l'émulateur..."
                 :"Initialization of the emulator...")); fflush(stderr);

    if ( teo_Init(TEO_NJOYSTICKS) < 0 )
        main_ExitMessage(teo_error_msg);

    /* Initialisation de l'interface d'acc�s direct */
    ufloppy_Init (drive_type, direct_write_support);

    /* D�tection des lecteurs support�s (3"5 seulement) */
    for (i=0; i<4; i++)
        teo.disk[i].direct_access_allowed = (IS_3_INCHES(i)) ? 1 : 0;

    udisplay_Window (); /* Cr�ation de la fen�tre principale */
    udisplay_Init();    /* Initialisation du serveur X */
    ugraphic_Init();    /* Initialisation du module graphique */
    disk_FirstLoad ();  /* Chargement des disquettes �ventuelles */
    cass_FirstLoad ();  /* Chargement de la cassette �ventuelle */
    if (memo_FirstLoad () < 0) /* Chargement de la cartouche �ventuelle */
        reset = 1;

    /* Chargement des options non d�finies */
    for (i=0;(remain_name!=NULL)&&(remain_name[i]!=NULL);i++)
        if (option_Undefined (remain_name[i]) == 1)
            reset = 1;
    g_strfreev(remain_name); /* Lib�re la m�moire des options ind�finies */

    /* Initialise le son */
    if (usound_Init() < 0)
        main_DisplayMessage(teo_error_msg);

    /* Restitue l'�tat sauvegard� de l'�mulateur */
    teo_FullReset();
    if (reset == 0)
        if (image_Load ("autosave.img") != 0)
            teo_FullReset();

    /* Initialise l'interface graphique */
    ugui_Init();
    udebug_Init();

    /* Et c'est parti !!! */
    printf((is_fr?"Lancement de l'émulation...\n":"Launching emulation...\n"));
    teo.command=TEO_COMMAND_NONE;
    timer = g_timer_new ();
    g_timeout_add_full (G_PRIORITY_DEFAULT, 1, RunTO8, &idle_data, NULL);
    gtk_main ();
    g_timer_destroy (timer);

    /* Sauvegarde de l'�tat de l'�mulateur */
    ini_Save();
    image_Save ("autosave.img");

    ufloppy_Exit(); /* Mise au repos de l'interface d'acc�s direct */
    ugui_Free ();   /* Lib�re la m�moire utilis�e par la GUI */
    udebug_Free();  /* Free memory used by the debugger */
    usound_Close(); /* Referme le p�riph�rique audio*/

    /* Sortie de l'�mulateur */
    printf((is_fr?"\nA bientôt !\n":"\nGoodbye !\n"));
    exit(EXIT_SUCCESS);
}

