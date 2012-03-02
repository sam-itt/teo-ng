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
 *  Module     : alleg/gui.c
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011
 *
 *  Panneau de contrôle de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/sound.h"
#include "alleg/main.h"
#include "alleg/gfxdrv.h"
#include "to8.h"


/* Facteur de correction pour la taille des boutons. */
#define BUTTON_FIX 6

/* Titre commun des boîtes de dialogue. */
static char title[35];



/* Message affiché dans la boîte de dialogue. */
static char mesg[35];

/* Boîte de dialogue. */
static DIALOG mesgdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc, 10,   10,    0,   50,     0,     0,   0,    0,       0,    0,    NULL },
{ d_ctext_proc,      10,   20,    0,    0,     0,     0,   0,    0,       0,    0,    mesg },
{ d_button_proc,     30,   40,   32,   16,     0,     0, 'o',    D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,      10,   10,    0,    0,     0,     0,   0,    0,       0,    0,    NULL },
{ NULL,               0,    0,    0,    0,     0,     0,   0,    0,       0,    0,    NULL }
};

#define MESGDIAL_SHADOW   0
#define MESGDIAL_MESG     1
#define MESGDIAL_OK       2

extern int  (*SetInterlaced)(int);

/* PopupMessage:
 *  Affiche une boîte de dialogue contenant le message et un bouton OK.
 */
static void PopupMessage(const char message[])
{
    mesgdial[MESGDIAL_SHADOW].w=strlen(message)*8+16;
    mesgdial[MESGDIAL_MESG].x=mesgdial[MESGDIAL_SHADOW].x+mesgdial[MESGDIAL_SHADOW].w/2;
    mesgdial[MESGDIAL_OK].x=mesgdial[MESGDIAL_MESG].x-(mesgdial[MESGDIAL_OK].w+BUTTON_FIX)/2;

    strcpy(mesg, message);

    centre_dialog(mesgdial);

    popup_dialog(mesgdial, MESGDIAL_OK);
}



/* Question posée dans la boîte de dialogue. */
static char quest[35];

/* Boîte de dialogue. */
static DIALOG questdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc, 10,   10,    0,   50,     0,     0,   0,    0,       0,    0,    NULL },
{ d_ctext_proc,      10,   20,    0,    0,     0,     0,   0,    0,       0,    0,    quest },
#ifdef FRENCH_LANG
{ d_button_proc,     30,   40,   32,   16,     0,     0, 'o',    D_EXIT,  0,    0,    "&Oui" },
{ d_button_proc,    130,   40,   32,   16,     0,     0, 'n',    D_EXIT,  0,    0,    "&Non" },
#else
{ d_button_proc,     30,   40,   32,   16,     0,     0, 'y',    D_EXIT,  0,    0,    "&Yes" },
{ d_button_proc,    130,   40,   32,   16,     0,     0, 'n',    D_EXIT,  0,    0,    "&No" },
#endif
{ d_yield_proc,      10,   10,    0,    0,     0,     0,   0,    0,       0,    0,    NULL },
{ NULL,               0,    0,    0,    0,     0,     0,   0,    0,       0,    0,    NULL }
};

#define QUESTDIAL_SHADOW  0
#define QUESTDIAL_QUEST   1
#define QUESTDIAL_YES     2
#define QUESTDIAL_NO      3

#define FOCUS_YES  QUESTDIAL_YES
#define FOCUS_NO   QUESTDIAL_NO


/* PopupQuestion:
 *  Affiche une boîte de dialogue contenant une question et deux boutons Oui/Non.
 */
static int PopupQuestion(const char question[], int focus)
{
    int esp;

    questdial[QUESTDIAL_SHADOW].w=strlen(question)*8+16;
    questdial[QUESTDIAL_QUEST].x=questdial[QUESTDIAL_SHADOW].x+questdial[QUESTDIAL_SHADOW].w/2;

    esp=(questdial[QUESTDIAL_SHADOW].w-2*(questdial[QUESTDIAL_YES].w+BUTTON_FIX))/3;
    questdial[QUESTDIAL_YES].x=questdial[QUESTDIAL_SHADOW].x+esp;
    questdial[QUESTDIAL_NO].x=questdial[QUESTDIAL_YES].x+(questdial[QUESTDIAL_YES].w+BUTTON_FIX)+esp;

    strcpy(quest, question);

    centre_dialog(questdial);

    return (popup_dialog(questdial, focus) == QUESTDIAL_YES ? TRUE : FALSE);
}



/* Bitmaps des voyants de l'imprimante. */
static BITMAP *nlq_led, *on_led, *online_led;

/* Boîte de dialogue. */
static DIALOG printerdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,    0,    0,     0,     0,    0,    0,       0,    0,    title },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Imprimante matricielle PR 90-612" },
#else
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Dot-matrix printer PR 90-612" },
#endif
{ d_box_proc,         40,  50,  100,  110,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,       90,  55,    0,    0,     0,     0,    0,    0,       0,    0,    "NLQ/Failure" },
{ d_box_proc,         60,  65,   60,   16,     0,     0,    0,    0,       0,    0,    NULL },
{ d_bitmap_proc,      61,  66,   58,   14,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,       90,  90,    0,    0,     0,     0,    0,    0,       0,    0,    "On" },
{ d_box_proc,         60, 100,   60,   16,     0,     0,    0,    0,       0,    0,    NULL },
{ d_bitmap_proc,      61, 101,   58,   14,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,       90, 125,    0,    0,     0,     0,    0,    0,       0,    0,    "On-Line" },
{ d_box_proc,         60, 135,   60,   16,     0,     0,    0,    0,       0,    0,    NULL },
{ d_bitmap_proc,      61, 136,   58,   14,     0,     0,    0,    0,       0,    0,    NULL },
#ifdef FRENCH_LANG
{ d_text_proc,       160,  50,    0,    0,     0,     0,    0,    0,       0,    0,    "Commandes:" }, 
#else
{ d_text_proc,       160,  50,    0,    0,     0,     0,    0,    0,       0,    0,    "Commands:" }, 
#endif
{ d_button_proc,     160,  65,  120,   16,     0,     0,    0,    D_EXIT,  0,    0,    "On-Line" },
{ d_button_proc,     160,  85,  120,   16,     0,     0,    0,    D_EXIT,  0,    0,    "Line Feed" },
{ d_button_proc,     160, 105,  120,   16,     0,     0,    0,    D_EXIT,  0,    0,    "Form Feed/NLQ" },
#ifdef FRENCH_LANG
{ d_text_proc,       175, 135,    0,    0,     0,     0,    0,    0,       0,    0,    "Sortie" },
{ d_check_proc,      175, 145,   90,   14,     0,     0,    0,    D_SELECTED,0,  0,    "graphique" },
#else
{ d_text_proc,       175, 135,    0,    0,     0,     0,    0,    0,       0,    0,    "Graphical" },
{ d_check_proc,      175, 145,   90,   14,     0,     0,    0,    D_SELECTED,0,  0,    "output" },
#endif
{ d_button_proc,      30, 170,  126,   16,     0,     0,  'o',    D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,       20,  10,    0,    0,     0,     0,    0,    0,       0,    0,    NULL },
{ NULL,                0,   0,    0,    0,     0,     0,    0,    0,       0,    0,    NULL }
};

#define PRINTERDIAL_NLQ_LED     6
#define PRINTERDIAL_ON_LED      9
#define PRINTERDIAL_ONLINE_LED  12
#define PRINTERDIAL_ONLINE      14
#define PRINTERDIAL_LINEFEED    15
#define PRINTERDIAL_FORMFEED    16
#define PRINTERDIAL_GFX         18
#define PRINTERDIAL_OK          19


/* MenuPrinter:
 *  Affiche le panneau de commandes de l'imprimante.
 */
static void MenuPrinter(void)
{
    static int first = 1;
    static int leds[3];

    if (first)
    {
        nlq_led = create_bitmap(58, 14);
        printerdial[PRINTERDIAL_NLQ_LED].dp = nlq_led;

        on_led = create_bitmap(58, 14);
        printerdial[PRINTERDIAL_ON_LED].dp = on_led;

        online_led = create_bitmap(58, 14);
        printerdial[PRINTERDIAL_ONLINE_LED].dp = online_led;

        to8_PrinterEnableGraphicMode(TRUE);
        first = 0;
    }

    centre_dialog(printerdial);
    clear_keybuf();

    while (TRUE)
    {
        /* Lit l'état de l'imprimante. */
        to8_PrinterGetState(leds);
        clear_to_color(nlq_led, leds[0] ? makecol(255, 0, 0) : gui_bg_color);
        clear_to_color(on_led, leds[1] ? makecol(255, 255, 0) : gui_bg_color);
        clear_to_color(online_led, leds[2] ? makecol(0, 255, 0) : gui_bg_color);

        switch (popup_dialog(printerdial, PRINTERDIAL_OK))
        {
            case PRINTERDIAL_ONLINE:
                to8_PrinterSendCommand(TO8_PRINTER_ONLINE);
                break;

            case PRINTERDIAL_LINEFEED:
                to8_PrinterSendCommand(TO8_PRINTER_LINE_FEED);
                break;

            case PRINTERDIAL_FORMFEED:
                to8_PrinterSendCommand(TO8_PRINTER_FORM_FEED);
                break;

            case -1:  /* ESC */
            case PRINTERDIAL_OK:
                to8_PrinterEnableGraphicMode(printerdial[PRINTERDIAL_GFX].flags & D_SELECTED);
                return;
        }
    }
}



/* Noms des fichiers utilisés comme disquettes. */
#define LABEL_LENGTH     20
static char disk_label[4][LABEL_LENGTH+1];

/* Masque de bits décrivant les lecteurs supportant la lecture directe. */
static int direct_disk;

/* Boîte de dialogue. */
static DIALOG diskdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc, 20,   10,  280,  180,     0,     0,   0,    0,       0,    0,    NULL },
{ d_ctext_proc,     160,   20,    0,    0,     0,     0,   0,    0,       0,    0,    title },
#ifdef FRENCH_LANG
{ d_ctext_proc,     160,   30,    0,    0,     0,     0,   0,    0,       0,    0,    "Lecteurs de disquettes" },
#else
{ d_ctext_proc,     160,   30,    0,    0,     0,     0,   0,    0,       0,    0,    "Disk drives" },
#endif
  /* disk 0 */
{ d_text_proc,       30,   54,    0,    0,     0,     0,   0,    0,       0,    0,    "0:" },
{ d_textbox_proc,    47,   50,  167,   16,     0,     0,   0,    0,       0,    0,    disk_label[0] },
{ d_button_proc,    220,   50,   30,   16,     0,     0, '0',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,   50,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* disk 1 */
{ d_text_proc,       30,   78,    0,    0,     0,     0,   0,    0,       0,    0,    "1:" },
{ d_textbox_proc,    47,   74,  167,   16,     0,     0,   0,    0,       0,    0,    disk_label[1] },
{ d_button_proc,    220,   74,   30,   16,     0,     0, '1',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,   74,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* disk 2 */
{ d_text_proc,       30,  102,    0,    0,     0,     0,   0,    0,       0,    0,    "2:" },
{ d_textbox_proc,    47,   98,  167,   16,     0,     0,   0,    0,       0,    0,    disk_label[2] },
{ d_button_proc,    220,   98,   30,   16,     0,     0, '2',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,   98,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* disk 3 */
{ d_text_proc,       30,  126,    0,    0,     0,     0,   0,    0,       0,    0,    "3:" },
{ d_textbox_proc,    47,  122,  167,   16,     0,     0,   0,    0,       0,    0,    disk_label[3] },
{ d_button_proc,    220,  122,   30,   16,     0,     0, '3',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,  122,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* direct disk */
#ifdef FRENCH_LANG
{ d_button_proc,     30,  146,  250,   16,     0,     0, 'd',    D_EXIT,  0,    0,    "Accès &direct" },
#else
{ d_button_proc,     30,  146,  250,   16,     0,     0, 'D',    D_EXIT,  0,    0,    "&Direct access" },
#endif
{ d_text_proc,      255,   40,    0,    0,     0,     0,   0,    0,       0,    0,    "prot." },
{ d_button_proc,     30,  170,  126,   16,     0,     0, 'o',    D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,      20,   10,    0,    0,     0,     0,   0,    0,       0,    0,    NULL },
{ NULL,               0,    0,    0,    0,     0,     0,   0,    0,       0,    0,    NULL }
};

#define DISKDIAL_LABEL0    4
#define DISKDIAL_BUTTON0   5
#define DISKDIAL_CHECK0    6
#define DISKDIAL_LABEL1    8
#define DISKDIAL_BUTTON1   9
#define DISKDIAL_CHECK1    10
#define DISKDIAL_LABEL2    12
#define DISKDIAL_BUTTON2   13
#define DISKDIAL_CHECK2    14
#define DISKDIAL_LABEL3    16
#define DISKDIAL_BUTTON3   17
#define DISKDIAL_CHECK3    18
#define DISKDIAL_DIRECT    19
#define DISKDIAL_OK        21


/* MenuDisk:
 *  Affiche le menu de gestion des lecteurs de disquettes.
 */
static void MenuDisk(void)
{
    static int first=1;
    static char filename[FILENAME_LENGTH];
    int drive, ret, ret2;
    
    if (first)
    {
        /* La première fois on tente d'ouvrir le répertoire par défaut. */
        if (file_exists("./disks", FA_DIREC, NULL))
	    strcpy(filename,"./disks/");
	
        centre_dialog(diskdial);

        for (drive=0; drive<4; drive++)
        {
            strncpy(disk_label[drive], get_filename(to8_GetDiskFilename(drive)), LABEL_LENGTH);
            if (disk_label[drive][0] == '\0')
                strncpy(disk_label[drive], is_fr?"aucune disquette":"no disk", LABEL_LENGTH);
        }

	first=0;
    }

    clear_keybuf();

    while (TRUE)
    {
        ret=popup_dialog(diskdial, DISKDIAL_OK);
        
        switch (ret)
        {
            case DISKDIAL_BUTTON0:
            case DISKDIAL_BUTTON1:
            case DISKDIAL_BUTTON2:
            case DISKDIAL_BUTTON3:
                drive=(ret-5)/4;
            
                if (file_select_ex(is_fr?"Choisissez votre disquette:":"Choose your disk:", filename, "sap",
                                   FILENAME_LENGTH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    ret2=to8_LoadDisk(drive, filename);

                    if (ret2 == TO8_ERROR)
                        PopupMessage(to8_error_msg);
                    else
                    {
                        strncpy(disk_label[drive], get_filename(filename), LABEL_LENGTH);

                        if ((ret2==TO8_READ_ONLY) && !(diskdial[ret+1].d2))
                        {
                            PopupMessage(is_fr?"Attention: écriture impossible.":"Warning: writing unavailable.");
                            diskdial[ret+1].flags|=D_SELECTED;
                            diskdial[ret+1].d2=1;
                        }
                    }
                }
                break;

            case DISKDIAL_CHECK0:
            case DISKDIAL_CHECK1:
            case DISKDIAL_CHECK2:
            case DISKDIAL_CHECK3:
                drive=(ret-6)/4;
            
                if (diskdial[ret].d2)
                {
                    if (to8_SetDiskMode(drive, TO8_READ_WRITE)==TO8_READ_ONLY)
                    {
                        PopupMessage(is_fr?"Ecriture impossible sur ce support.":"Writing unavailable on this device.");
                        diskdial[ret].flags|=D_SELECTED;
                        diskdial[ret].d2=1;
                    }
                    else
                    {
                        diskdial[ret].flags&=~D_SELECTED;
                        diskdial[ret].d2=0;
                    }
                }
                else
                {
                    to8_SetDiskMode(drive, TO8_READ_ONLY);
                    diskdial[ret].flags|=D_SELECTED;
                    diskdial[ret].d2=1;
                }
                break;

            case DISKDIAL_DIRECT:
                for (drive=0; drive<4; drive++)
                {
                    if (direct_disk & (1<<drive))
                    {
                        strncpy(disk_label[drive], is_fr?"accès direct":"direct access", LABEL_LENGTH);

                        if (to8_DirectSetDrive(drive) == TO8_READ_ONLY)
                        {
                            diskdial[DISKDIAL_CHECK0+4*drive].flags|=D_SELECTED;
                            diskdial[DISKDIAL_CHECK0+4*drive].d2=1;
                        }
                        else
                        {
                            diskdial[DISKDIAL_CHECK0+4*drive].flags&=~D_SELECTED;
                            diskdial[DISKDIAL_CHECK0+4*drive].d2=0;
                        }
                    }
                }
                break;

            case -1:  /* ESC */
            case DISKDIAL_OK:
                return;
        }
    }
}



/* Nom du fichier utilisé comme cassette. */
static char k7_label[LABEL_LENGTH+1];

/* Affichage du compteur. */
#define COUNTER_MAX  999
static char k7_counter_str[4];

/* Horloge de la répétition automatique. */
static int k7_tick;

/* Références avant. */
static int updown_edit_proc(int msg, DIALOG *d, int c);
static int up_button_proc(int msg, DIALOG *d, int c);
static int down_button_proc(int msg, DIALOG *d, int c);

/* Boîte de dialogue. */
static DIALOG k7dial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,    0,    0,     0,     0,    0,    0,       0,    0,    title },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Lecteur de cassettes" },
#else
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Tape recorder" },
#endif
{ d_text_proc,        30,  54,    0,    0,     0,     0,    0,    0,       0,    0,    "k7" },
{ d_textbox_proc,     47,  50,  167,   16,     0,     0,    0,    0,       0,    0,    k7_label },
{ d_button_proc,     220,  50,   30,   16,     0,     0,  'k',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,      260,  50,   15,   15,     0,     0,    0,    D_EXIT|D_SELECTED,0,1,"" },
{ d_text_proc,       255,  40,    0,    0,     0,     0,    0,    0,       0,    0,    "prot." },
#ifdef FRENCH_LANG
{ d_text_proc,        30,  79,    0,    0,     0,     0,    0,    0,       0,    0,    "Compteur:" },
#else
{ d_text_proc,        30,  79,    0,    0,     0,     0,    0,    0,       0,    0,    "Counter:" },
#endif
{ d_box_proc,        106,  77,   36,   12,     0,     0,    0,    0,       0,    0,    NULL },
{ updown_edit_proc,  108,  79,   32,   10,     0,     0,    0,    0,       3,    3,    k7_counter_str },
{ up_button_proc,    145,  73,   19,   10,     0,     0,    0,    D_EXIT,  0,    0,    "+" },
{ down_button_proc,  145,  84,   19,   10,     0,     0,    0,    0,       0,    0,    "-" },
#ifdef FRENCH_LANG
{ d_button_proc,     174,  75,  106,   16,     0,     0,  'r',    D_EXIT,  0,    0,    "&Rembobiner" },
#else
{ d_button_proc,     174,  75,  106,   16,     0,     0,  'r',    D_EXIT,  0,    0,    "&Rewind" },
#endif
{ d_button_proc,      30, 170,  126,   16,     0,     0,  'o',    D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,       20,  10,    0,    0,     0,     0,    0,    0,       0,    0,    NULL },
{ NULL,                0,   0,    0,    0,     0,     0,    0,    0,       0,    0,    NULL }
};

#define K7DIAL_LABEL    4
#define K7DIAL_BUTTON   5
#define K7DIAL_CHECK    6
#define K7DIAL_BOXEDIT  9
#define K7DIAL_EDIT     10
#define K7DIAL_REWIND   13
#define K7DIAL_OK       14


/* updown_edit_proc:
 *  Procédure du champ d'entrée du compteur.
 */
static int updown_edit_proc(int msg, DIALOG *d, int c)
{
    char *endp;
    int ret, counter;

    /* Seuls les chiffres nous intéressent. */
    if ((msg == MSG_UCHAR) && !uisdigit(c))
        return D_O_K;

    ret = d_edit_proc(msg, d, c);

    /* d_edit_proc peut ne rien faire à la réception de MSG_CHAR et attendre
       le MSG_UCHAR suivant. Par conséquent D_USED_CHAR est le bon signal. */
    if (ret == D_USED_CHAR)
    {
        counter = ustrtol(k7_counter_str, &endp, 10);
        if (endp == k7_counter_str)
            counter = 0;
        else
            counter = MID(0, counter, COUNTER_MAX);

        to8_SetK7Counter(counter);
    }

    return ret;
}


/* update_counter:
 *  Met à jour le compteur et son affichage.
 */
static void update_counter(int *counter)
{
    *counter = MID(0, *counter, COUNTER_MAX);
    to8_SetK7Counter(*counter);

    k7dial[K7DIAL_EDIT].d2 = uszprintf(k7_counter_str, sizeof(k7_counter_str), "%d", *counter);
    scare_mouse();
    object_message(k7dial+K7DIAL_EDIT, MSG_DRAW, 0);
    unscare_mouse();
}


/* updown_button_proc:
 *  Corps de procédure des boutons UP et DOWN.
 */
static int updown_button_proc(int msg, DIALOG *d, int c, int down)
{
    int previous_tick, step, counter;

    if (msg == MSG_CLICK)
    {
        /* Enfonce le bouton. */
        d->flags |= D_SELECTED;
        scare_mouse();
        object_message(d, MSG_DRAW, 0);
        unscare_mouse();

        counter = to8_GetK7Counter();

        if (down)
           counter--;
        else
           counter++;

        update_counter(&counter);

        /* Gestion de la répétition automatique. */
        k7_tick = 0;
        previous_tick = 1;
        while (gui_mouse_b())
        {
            if (k7_tick > previous_tick)
            {
                if (k7_tick < 5)
                    step = 1;
                else if (k7_tick < 15)
                    step = 10;
                else
                    step = 50;
 
                if (down)
                   counter -= step;
                else
                   counter += step;

                update_counter(&counter);

                previous_tick = k7_tick;
            }

   	    /* Poursuit l'animation des autres objets. */
	    broadcast_dialog_message(MSG_IDLE, 0);
        }

        /* Relâche le bouton. */
        d->flags &= ~D_SELECTED;
        scare_mouse();
        object_message(d, MSG_DRAW, 0);
        unscare_mouse();

        return D_O_K;
    }
  
    return d_button_proc(msg, d, c);
}


/* up_button_proc:
 *  Procédure du boutons UP.
 */
static int up_button_proc(int msg, DIALOG *d, int c)
{
    return updown_button_proc(msg, d, c, FALSE);
}


/* down_button_proc:
 *  Procédure des boutons DOWN.
 */
static int down_button_proc(int msg, DIALOG *d, int c)
{
    return updown_button_proc(msg, d, c, TRUE);
}


/* k7_ticker:
 *  Mécanisme de l'horloge de la répétition automatique.
 */
static void k7_ticker(void)
{
    k7_tick++;
}


/* MenuK7:
 *  Affiche le menu de gestion du lecteur de cassettes.
 */
static void MenuK7(void)
{
    static int first=1;
    static char filename[FILENAME_LENGTH];
    int ret;

    if (first)
    {
        /* La première fois on tente d'ouvrir le répertoire par défaut. */
    	if (file_exists("./k7", FA_DIREC, NULL))
	    strcpy(filename,"./k7/");

        centre_dialog(k7dial);

        strncpy(k7_label, get_filename(to8_GetK7Filename()), LABEL_LENGTH);
        if (k7_label[0] == '\0')
            strncpy(k7_label, is_fr?"aucune cassette":"no tape", LABEL_LENGTH);

	first=0;
    }

    clear_keybuf();
    install_int_ex(k7_ticker, BPS_TO_TIMER(2));

    while (TRUE)
    {
        k7dial[K7DIAL_EDIT].d2 = uszprintf(k7_counter_str, sizeof(k7_counter_str), "%d", to8_GetK7Counter());

        ret=popup_dialog(k7dial, K7DIAL_OK);
        
        switch (ret)
        {
            case K7DIAL_BUTTON:
                if (file_select_ex(is_fr?"Choisissez votre cassette:":"Choose your tape:", filename, "k7", 
                                   FILENAME_LENGTH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    ret=to8_LoadK7(filename);

                    if (ret == TO8_ERROR)
                        PopupMessage(to8_error_msg);
                    else
                    {
                        strncpy(k7_label, get_filename(filename), LABEL_LENGTH);

                        if ((ret==TO8_READ_ONLY) && !(k7dial[6].d2))
                        {
                            PopupMessage(is_fr?"Attention: écriture impossible.":"Warning: writing unavailable.");
                            k7dial[K7DIAL_CHECK].flags|=D_SELECTED;
                            k7dial[K7DIAL_CHECK].d2=1;
                        }
                    }
                }
                break;

            case K7DIAL_CHECK:
                if (k7dial[K7DIAL_CHECK].d2)
                {
                    if (to8_SetK7Mode(TO8_READ_WRITE)==TO8_READ_ONLY)
                    {
                        PopupMessage(is_fr?"Ecriture impossible sur ce support.":"Writing unavailable on this device.");
                        k7dial[K7DIAL_CHECK].flags|=D_SELECTED;
                        k7dial[K7DIAL_CHECK].d2=1;
                    }
                    else
                    {
                        k7dial[K7DIAL_CHECK].flags&=~D_SELECTED;
                        k7dial[K7DIAL_CHECK].d2=0;
                    }
                }
                else
                {
                    to8_SetK7Mode(TO8_READ_ONLY);
                    k7dial[K7DIAL_CHECK].flags|=D_SELECTED;
                    k7dial[K7DIAL_CHECK].d2=1;
                }
                break;

            case K7DIAL_REWIND:
                to8_SetK7Counter(0);
                break;

            case -1:  /* ESC */
            case K7DIAL_OK:
                remove_int(k7_ticker);
                return;
        }
    }
}



/* Nom du fichier utilisé comme cartouche. */
static char m7_label[TO8_MEMO7_LABEL_LENGTH+1];

/* Boîte de dialogue. */
static DIALOG m7dial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,    0,    0,     0,     0,    0,    0,       0,    0,    title },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Lecteur de cartouches" },
#else
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Cartridge reader" },
#endif
{ d_text_proc,        30,  54,    0,    0,     0,     0,    0,    0,       0,    0,    "m7" },
{ d_textbox_proc,     47,  50,  208,   16,     0,     0,    0,    0,       0,    0,    m7_label },
{ d_button_proc,     260,  50,   30,   16,     0,     0,  'm',    D_EXIT,  0,    0,    "..." },
{ d_button_proc,      30, 170,  126,   16,     0,     0,  'o',    D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,       20,  10,    0,    0,     0,     0,    0,    0,       0,    0,    NULL },
{ NULL,                0,   0,    0,    0,     0,     0,    0,    0,       0,    0,    NULL }
};

#define M7DIAL_LABEL   4
#define M7DIAL_BUTTON  5
#define M7DIAL_OK      6


/* MenuMemo7:
 *  Affiche le menu de gestion du lecteur de cartouches.
 */
static void MenuMemo7(void)
{
    static int first=1;
    static char filename[FILENAME_LENGTH];
    int ret;

    if (first)
    {
        /* La première fois on tente d'ouvrir le répertoire par défaut. */
	if (file_exists("./memo7", FA_DIREC, NULL))
	    strcpy(filename,"./memo7/");
	
        centre_dialog(m7dial);

        strncpy(m7_label, to8_GetMemo7Label(), TO8_MEMO7_LABEL_LENGTH);
        if (m7_label[0] == '\0')
            strncpy(m7_label, is_fr?"aucune cartouche":"no cartridge", TO8_MEMO7_LABEL_LENGTH);

	first=0;
    }

    clear_keybuf();

    while (TRUE)
    {
        ret=popup_dialog(m7dial, M7DIAL_OK);

        switch (ret)
        {
            case M7DIAL_BUTTON:
                if (file_select_ex(is_fr?"Choisissez votre cartouche:":"Choose your cartridge", filename, "m7",
                                   FILENAME_LENGTH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    if (to8_LoadMemo7(filename) == TO8_ERROR)
                        PopupMessage(to8_error_msg);
                    else
                    {
                        strncpy(m7_label, to8_GetMemo7Label(), TO8_MEMO7_LABEL_LENGTH);
                        teo.command=COLD_RESET;
                    }   
                }
                break;

            case -1:  /* ESC */
            case M7DIAL_OK:
                return;
        }
    }
}



/* Boîte de dialogue. */
static DIALOG commdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)    (fg)   (bg)   (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,   280,  180,     0,     0,      0,   0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,     0,    0,     0,     0,      0,   0,       0,    0,    title },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  30,     0,    0,     0,     0,      0,   0,       0,    0,    "Commandes et Réglages" },
{ d_button_proc,      30,  45,   260,   16,     0,     0,    'r',  D_EXIT,   0,    0,    "&Réinitialiser le TO8" },
{ d_button_proc,      30,  68,   260,   16,     0,     0,    'f',  D_EXIT,   0,    0,    "&Redémarrer à &froid le TO8" },
{ d_text_proc,        32,  94,     0,    0,     0,     0,      0,   0,       0,    0,    "Vitesse:" },
{ d_radio_proc,      107,  94,   126,    8,     0,     0,    'e',   0,       1,    0,    "&exacte" },
{ d_radio_proc,      107, 108,   126,    8,     0,     0,    'm',   0,       1,    0,    "&maximale" },
{ d_text_proc,        32, 124,     0,    0,     0,     0,      0,   0,       0,    0,    "&Volume:" },
{ d_slider_proc,      92, 122,   100,   15,     0,     0,    'v',   0,     254,    0,    NULL },
{ d_check_proc,       30, 142,   147,   14,     5,     0,    't',   0,       0,    0,    "Vidéo en&trelacée" },
{ d_box_proc,        209,  94,    80,   74,     0,     0,      0,   0,       0,    0,    NULL },
{ d_ctext_proc,      248, 100,     0,    0,     0,     0,      0,   0,       0,    0,    "Images:" },
{ d_button_proc,     217, 117,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Charger" },
{ d_button_proc,     217, 142,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Sauver" },
#else
{ d_ctext_proc,      160,  30,     0,    0,     0,     0,      0,   0,       0,    0,    "Commands and Settings" },
{ d_button_proc,      30,  45,   260,   16,     0,     0,    'w',  D_EXIT,   0,    0,    "TO8 &warm reset" },
{ d_button_proc,      30,  68,   260,   16,     0,     0,    'c',  D_EXIT,   0,    0,    "TO8 &cold reset" },
{ d_text_proc,        32,  94,     0,    0,     0,     0,      0,   0,       0,    0,    "  Speed:" },
{ d_radio_proc,      107,  94,   126,    8,     0,     0,    'e',   0,       1,    0,    "&exact" },
{ d_radio_proc,      107, 108,   126,    8,     0,     0,    'f',   0,       1,    0,    "&fast" },
{ d_text_proc,        32, 124,     0,    0,     0,     0,      0,   0,       0,    0,    "&Volume:" },
{ d_slider_proc,      92, 122,   100,   15,     0,     0,    'v',   0,     254,    0,    NULL },
{ d_check_proc,       30, 142,   147,   14,     5,     0,    't',   0,       0,    0,    "In&terlaced video" },
{ d_box_proc,        209,  94,    80,   74,     0,     0,      0,   0,       0,    0,    NULL },
{ d_ctext_proc,      248, 100,     0,    0,     0,     0,      0,   0,       0,    0,    "Images:" },
{ d_button_proc,     217, 117,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Load" },
{ d_button_proc,     217, 142,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Save" },
#endif
{ d_button_proc,      30, 170,   126,   16,     0,     0,    'o',  D_EXIT,   0,    0,    "&OK" },
{ d_yield_proc,       20,  10,     0,    0,     0,     0,      0,   0,       0,    0,    NULL },
{ NULL,                0,   0,     0,    0,     0,     0,      0,   0,       0,    0,    NULL }
};

#define COMMDIAL_INIT        3
#define COMMDIAL_COLDINIT    4
#define COMMDIAL_EXACTSPEED  6
#define COMMDIAL_MAXSPEED    7
#define COMMDIAL_VOLUME      8
#define COMMDIAL_SLIDER      9
#define COMMDIAL_INTERLACE   10
#define COMMDIAL_SPAN        11
#define COMMDIAL_LOAD        12
#define COMMDIAL_SAVE        13
#define COMMDIAL_OK          14


/* MenuComm:
 *  Affiche le menu des commandes et réglages.
 */
static void MenuComm(void)
{
    static int interlaced;
    static char filename[FILENAME_LENGTH];

    interlaced = commdial[COMMDIAL_INTERLACE].flags & D_SELECTED;
    
    if (teo.exact_speed)
    {
        commdial[COMMDIAL_EXACTSPEED].flags=D_SELECTED;
        commdial[COMMDIAL_MAXSPEED].flags=0;
    }
    else
    {
        commdial[COMMDIAL_EXACTSPEED].flags=0;
        commdial[COMMDIAL_MAXSPEED].flags=D_SELECTED;
    }

    if (teo.sound_enabled && teo.exact_speed)
        commdial[COMMDIAL_SLIDER].d2=GetVolume()-1;

    clear_keybuf();
    centre_dialog(commdial);

    switch (popup_dialog(commdial, COMMDIAL_OK))
    {
        case COMMDIAL_INIT:
            teo.command=RESET;
            break;

        case COMMDIAL_COLDINIT:
            teo.command=COLD_RESET;
            break;

        case COMMDIAL_LOAD:
            if (file_select_ex(is_fr?"Choisissez votre image:":"Choose your image:", filename, "img",
                               FILENAME_LENGTH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
            {
                if (to8_LoadImage(filename) == TO8_ERROR)
                    PopupMessage(to8_error_msg);
            }
            break;
        
        case COMMDIAL_SAVE:
            if (file_select_ex(is_fr?"Spécifiez un nom pour votre image:":"Specify a name for your image:", filename, "img",
                               FILENAME_LENGTH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
            {
                if (to8_SaveImage(filename) == TO8_ERROR)
                    PopupMessage(to8_error_msg);
            }
            break;
    }

    if ((commdial[COMMDIAL_INTERLACE].flags & D_SELECTED) != interlaced)
        SetInterlaced(((commdial[COMMDIAL_INTERLACE].flags & D_SELECTED) == 0) ? 0 : 1);

    if (teo.sound_enabled && teo.exact_speed)
        SetVolume(commdial[COMMDIAL_SLIDER].d2+1);

    teo.exact_speed=(commdial[COMMDIAL_EXACTSPEED].flags&D_SELECTED ? TRUE : FALSE);
}



/* Boîte de dialogue. */
static DIALOG controldial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg) (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,     0,     0,   0,    0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,    0,    0,     0,     0,   0,    0,       0,    0,    title },
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,   0,    0,       0,    0,    "Gilles Fétis - Eric Botcazou" },
{ d_ctext_proc,      160,  40,    0,    0,     0,     0,   0,    0,       0,    0,    "Alex Pukall - Jérémie Guillaume" },
{ d_ctext_proc,      160,  50,    0,    0,     0,     0,   0,    0,       0,    0,    "François Mouret - Samuel Devulder" },
#ifdef FRENCH_LANG
{ d_button_proc,      30,  66,  260,   16,     0,     0,   'c',  D_EXIT,  0,    0,    "&Commandes et Réglages..." },
{ d_button_proc,      30,  86,  260,   16,     0,     0,   't',  D_EXIT,  0,    0,    "Lecteur de car&touches..." },
{ d_button_proc,      30, 106,  260,   16,     0,     0,   's',  D_EXIT,  0,    0,    "Lecteur de ca&ssettes..." },
{ d_button_proc,      30, 126,  260,   16,     0,     0,   'd',  D_EXIT,  0,    0,    "Lecteurs de &disquettes..." },
{ d_button_proc,      30, 146,  260,   16,     0,     0,   'i',  D_EXIT,  0,    0,    "&Imprimante matricielle..." },
{ d_button_proc,      30, 170,  126,   16,     0,     0,   'o',  D_EXIT,  0,    0,    "&OK" },
{ d_button_proc,     164, 170,  126,   16,     0,     0,   'q',  D_EXIT,  0,    0,    "&Quitter" },
#else
{ d_button_proc,      30,  66,  260,   16,     0,     0,   'c',  D_EXIT,  0,    0,    "&Commands and Settings..." },
{ d_button_proc,      30,  86,  260,   16,     0,     0,   'r',  D_EXIT,  0,    0,    "&Cartridge reader..." },
{ d_button_proc,      30, 106,  260,   16,     0,     0,   't',  D_EXIT,  0,    0,    "&Tape recorder..." },
{ d_button_proc,      30, 126,  260,   16,     0,     0,   'd',  D_EXIT,  0,    0,    "&Disk drives..." },
{ d_button_proc,      30, 146,  260,   16,     0,     0,   'p',  D_EXIT,  0,    0,    "Dot-matrix &printer..." },
{ d_button_proc,      30, 170,  126,   16,     0,     0,   'o',  D_EXIT,  0,    0,    "&OK" },
{ d_button_proc,     164, 170,  126,   16,     0,     0,   'q',  D_EXIT,  0,    0,    "&Quit" },
#endif
{ d_yield_proc,       20,  10,    0,    0,     0,     0,   0,    0,       0,    0,    NULL },
{ NULL,                0,   0,    0,    0,     0,     0,   0,    0,       0,    0,    NULL }
};

#define CONTROLDIAL_COMMAND   5
#define CONTROLDIAL_CART      6
#define CONTROLDIAL_CASS      7
#define CONTROLDIAL_DISK      8
#define CONTROLDIAL_PRINTER   9
#define CONTROLDIAL_OK        10
#define CONTROLDIAL_QUIT      11


/* ControlPanel:
 *  Affiche le panneau de contrôle.
 */
void ControlPanel(void)
{
    clear_keybuf();
    centre_dialog(controldial);

    while (TRUE)
        switch (popup_dialog(controldial, CONTROLDIAL_OK))
        {
            case CONTROLDIAL_COMMAND:
                MenuComm();
                break;

            case CONTROLDIAL_CART:
                MenuMemo7();
                break;

            case CONTROLDIAL_CASS:
                MenuK7();
                break;

            case CONTROLDIAL_DISK:
                MenuDisk();
                break;

            case CONTROLDIAL_PRINTER:
                MenuPrinter();
                break;

            case -1:  /* ESC */
            case CONTROLDIAL_OK:
                return;

            case CONTROLDIAL_QUIT:
                if (PopupQuestion(is_fr?"Voulez-vous vraiment quitter ?":"Do you really want to quit ?", FOCUS_NO))
                {
                    teo.command=QUIT;
                    return;
                }
        }
}



/* SetGUIColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void SetGUIColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color( mesgdial, fg_color, bg_color);
    set_dialog_color(questdial, fg_color, bg_color);
    
    set_dialog_color(printerdial, fg_color, bg_color);

    set_dialog_color(diskdial, fg_color, bg_color);
    diskdial[DISKDIAL_LABEL0].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL1].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL2].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL3].bg = bg_entry_color;
    
    set_dialog_color(k7dial, fg_color, bg_color);
    k7dial[K7DIAL_LABEL].bg = bg_entry_color;
    k7dial[K7DIAL_BOXEDIT].bg = bg_entry_color;
    k7dial[K7DIAL_EDIT].bg = bg_entry_color;

    set_dialog_color(m7dial, fg_color, bg_color);
    m7dial[M7DIAL_LABEL].bg = bg_entry_color;
    
    set_dialog_color(commdial, fg_color, bg_color);
    set_dialog_color(controldial, fg_color, bg_color);

    /* Pour tous les objets. */
    gui_fg_color = fg_color;
    gui_bg_color = bg_color;
    gui_mg_color = bg_color;
}



/* InitGUI:
 *  Initialise le module interface utilisateur.
 */
void InitGUI(char version_name[], int gfx_mode, int direct_disk_support)
{
    strcpy(title, version_name);

    if ((strstr (version_name, "MSDOS") != NULL) && (gfx_mode == GFX_MODE40))
        commdial[COMMDIAL_INTERLACE].flags |= D_DISABLED;

    if (!teo.sound_enabled)
        commdial[COMMDIAL_VOLUME].flags=commdial[COMMDIAL_SPAN].flags=commdial[COMMDIAL_SLIDER].flags=D_DISABLED;

    if (!direct_disk_support)
        diskdial[DISKDIAL_DIRECT].flags=D_HIDDEN;

    direct_disk = direct_disk_support;

    if (strlen(to8_GetMemo7Label()))
        strncpy(m7_label, to8_GetMemo7Label(), TO8_MEMO7_LABEL_LENGTH);
}

