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
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Eric Botcazou 04/11/2003
 *               Samuel Devulder 08/2011
 *               François Mouret 08/2011 25/04/2012 01/11/2012
 *                               19/09/2013 13/04/2014 31/07/2016
 *                               25/10/2018
 *               Samuel Cuella   01/2020
 *
 *  Boucle principale de l'émulateur.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <allegro.h>
#endif
#include <assert.h>

#include "defs.h"
#include "teo.h"
#include "option.h"
#include "ini.h"
#include "image.h"
#include "main.h"
#include "errors.h"
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
#include "alleg/afront.h"
#include "dos/floppy.h"
#include "dos/debug.h"
#include "gettext.h"

#if defined ENABLE_NLS && ENABLE_NLS
#define __(a) main_Latin1ToCP850(_(a))
#else
#define __(a) _(a)
#endif


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

int direct_write_support = TRUE;

/*
 * Despite claims by DJGPP gettext(), no transcoding
 * is done between iso-8859 to CP850, which results 
 * in strange chars in the output instead of accented
 * letters. 
 *
 * Having the mo file in CP850 while possible will make
 * Allegro very unhappy and display strange chars in the
 * "in-game" menu which is worst the the problem itself.
 *
 * This function does a rough transliteration for console
 * output.
 *
 * It will be replaced by a more sophisticated output
 * system when messages have to go through console to reach
 * the user.
 *
 */
static const char *main_Latin1ToCP850(const char *str)
{
    static unsigned char *buffer = NULL;
    static int buffer_len = 0;
    int len;

    len = strlen(str);
    if(!buffer || len+1 > buffer_len){
        if(buffer)
            free(buffer);
        buffer = malloc((len+1)*sizeof(char));
        buffer_len = len;
    }
    memset(buffer, 0, buffer_len+1);
    strcpy(buffer, str);
    for(int i = 0; i < len; i++){
        switch(buffer[i]){
            case 0xE7: //&ccdeil
                buffer[i] = 0x87;
                break;
            case 0xE8: //&eacute
                buffer[i] = 0x8A;
                break;
            case 0xE9: //&egrave
                buffer[i] = 0x82;
                break;
            case 0xF4: //ocirc
                buffer[i] = 0x83;
                break;
        }
    }
    return buffer;
}


/* main_ErrorMessage:
 *  Affiche un message d'erreur et sort du programme.
 */
static void main_ErrorMessage(const char msg[])
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
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
           __("Cold-reset emulator"), NULL },
        { "disk0", '0', OPTION_ARG_FILENAME, &teo.disk[0].file,
           __("Load virtual disk (drive 0)"),
           __("FILE") },
        { "disk1", '1', OPTION_ARG_FILENAME, &teo.disk[1].file,
           __("Load virtual disk (drive 1)"),
           __("FILE") },
        { "disk2", '2', OPTION_ARG_FILENAME, &teo.disk[2].file,
           __("Load virtual disk (drive 2)"),
           __("FILE") },
        { "disk3", '3', OPTION_ARG_FILENAME, &teo.disk[3].file,
           __("Load virtual disk (drive 3)"),
           __("FILE") },
        { "cass", '\0', OPTION_ARG_FILENAME, &teo.cass.file,
           __("Load a tape"),
           __("FILE") },
        { "memo", '\0', OPTION_ARG_FILENAME, &teo.memo.file,
           __("Load a cartridge"),
           __("FILE") },
        { "mode40", '\0', OPTION_ARG_BOOL, &mode40,
           __("40 columns display"), NULL},
        { "mode80", '\0', OPTION_ARG_BOOL, &mode80,
           __("80 columns display"), NULL},
        { "truecolor", '\0', OPTION_ARG_BOOL, &truecolor,
           __("Truecolor display"), NULL},
        { NULL, 0, 0, NULL, NULL, NULL }
    };
    message = option_Parse (argc, argv, "teo", entries, &remain_name);
    if (message != NULL)
        main_ErrorMessage(message);
        
    if (mode40)    gfx_mode = GFX_MODE40   ; else
    if (mode80)    gfx_mode = GFX_MODE80   ; else
    if (truecolor) gfx_mode = GFX_TRUECOLOR;
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



/* main_DisplayMessage:
 *  Affiche un message de sortie et sort du programme.
 */
void main_DisplayMessage(const char msg[])
{
    agui_PopupMessage (msg);
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
    char version_name[]=PACKAGE_STRING" (MSDOS/DPMI)";
    char *mode_desc[3];
    int direct_support = 0;
    int drive_type[4];
    int njoy = 0;  /* njoy=-1 si joystick non supportés */
    int scancode, i;
    struct STRING_LIST *str_list = NULL;
    int windowed_mode;
    int rv;

    mode_desc[0] = strdup(__(" 1. 40 columns mode 16 colors\n    (fast diplay, adapted to games and most applications)"));
    mode_desc[1] = strdup(__(" 2. 80 columns mode 16 colors\n    (for applications which needs 80 columns)"));
    mode_desc[2] = strdup(__(" 3. 80 columns mode 4096 colors\n    (slow display but allow dynamic changes of palette)"));

#if defined ENABLE_NLS && ENABLE_NLS
    /* Setting the i18n environment */
    setlocale (LC_ALL, "");
    char *localedir = std_GetLocaleBaseDir();
    bindtextdomain(PACKAGE, localedir);
    bind_textdomain_codeset(PACKAGE, "iso-8859-1");
    textdomain (PACKAGE);
    if(localedir)
        free(localedir);
#endif

    /* traitement des paramètres */
    ini_Load();                   /* Charge les paramètres par défaut */
    ReadCommandLine (argc, argv); /* Récupération des options */

    rv = afront_Init(NULL, (njoy >= 0), ALLEGRO_CONFIG_FILE, "akeymap.ini");
    if(rv != 0){
        printf("Couldn't initialize Allegro, bailing out !\n");
        exit(EXIT_FAILURE);
    }
   
    LOCK_VARIABLE(teo);

    /* message d'entete */
    printf(__("Here comes %s the Thomson TO8 emulator.\n"), version_name);
    printf(__("Copyright (C) 1997-%s Teo Authors: %s.\n\n"), TEO_YEAR_STRING, TEO_AUTHORS);                 
    printf(__("Command keys: [ESC] Control panel\n"));           
    printf(__("\t[F11] Screenshot\n"));           
    printf(__("\t[F12] Debugger\n"));           

    /* détection de la présence de joystick(s) */
    njoy = MIN(TEO_NJOYSTICKS, num_joysticks);

    /* initialisation de l'emulateur */
    printf(__("Emulator init..."));

    if (teo_Init(TEO_NJOYSTICKS-njoy) < 0)
        main_ErrorMessage(teo_error_msg);

    printf("ok\n");


    /* initialisation de l'interface d'accès direct */
    dfloppy_Init (drive_type, direct_write_support);

    /* Détection des lecteurs supportés (3"5 seulement) */
    for (i=0; i<4; i++)
    {
        if (IS_3_INCHES(i))
            direct_support |= (1<<i);
    }

    /* selection du mode graphique */ 
    printf(__("\nSelect graphic mode:\n\n"));

    if (gfx_mode == NO_GFX)
    {
        for (i=0; i<3; i++)
            printf("%s\n\n", mode_desc[i]);
            
        printf(__("Your choice: [default 1] "));

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

    rv = afront_startGfx(gfx_mode, &windowed_mode, version_name);
    if(rv != 0){
        main_ExitMessage(__("Unsupported graphic mode"));
    }

    disk_FirstLoad ();  /* chargement des disquettes eventuelles */
    cass_FirstLoad ();  /* chargement de la cassette eventuelle */
    if (memo_FirstLoad () < 0) /* Chargement de la cartouche eventuelle */
        reset = 1;

    /* chargement des options non definies */
    for (str_list=remain_name; str_list!=NULL; str_list=str_list->next)
        if (option_Undefined (str_list->str) == 1)
            reset = 1;
    std_StringListFree (remain_name);

    /* Restitue l'etat sauvegarde de l'émulateur */
    teo_FullReset();
    if (reset == 0)
        if (image_Load ("autosave.img") != 0)
            teo_FullReset();

    /* et c'est parti !!! */
    afront_Run(windowed_mode);

    /* Sauvegarde de l'etat de l'emulateur */
    ini_Save();
    image_Save ("autosave.img");

    afront_Shutdown();

    /* mise au repos de l'interface d'accès direct */
    dfloppy_Exit();

    /* sortie de l'emulateur */
    printf(__("Goodbye !\n"));

    /* sortie de l'emulateur */
    exit(EXIT_SUCCESS);
}

