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
 *  Module     : alleg/gui->c
 *  Version    : 1.8.2
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *
 *  Panneau de contrôle de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "to8.h"

extern void SetPrinterGUIColors(int fg_color, int bg_color, int bg_entry_color);
extern void SetDiskGUIColors(int fg_color, int bg_color, int bg_entry_color);
extern void SetCassGUIColors(int fg_color, int bg_color, int bg_entry_color);
extern void SetMemoGUIColors(int fg_color, int bg_color, int bg_entry_color);
extern void SetCommGUIColors(int fg_color, int bg_color, int bg_entry_color);
extern void SetAboutGUIColors(int fg_color, int bg_color, int bg_entry_color);
extern void MenuComm(void);
extern void MenuDisk(void);
extern void MenuK7(void);
extern void MenuMemo7(void);
extern void MenuPrinter(void);
extern void MenuAbout(void);
extern void InitCommGUI(char version_name[], int gfx_mode, char *title);
extern void InitDiskGUI(int direct_disk_support, char *title);
extern void InitMemoGUI(char *title);
extern void InitCassGUI(char *title);
extern void InitPrinterGUI(char *title);
extern void InitAboutGUI(char *title);

/* Facteur de correction pour la taille des boutons. */
#define BUTTON_FIX 6

/* Titre commun des boîtes de dialogue. */
#define TEXT_LENGTH  200
static char title[TEXT_LENGTH+1] = "";

/* Message affiché dans la boîte de dialogue. */
static char mesg[TEXT_LENGTH+1] = "";

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

#if 0
/* Question posée dans la boîte de dialogue. */
static char quest[TEXT_LENGTH+1] = "";

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

#ifdef DJGPP
    (void)sprintf(quest, "%s", question);
#else
    (void)snprintf(quest, TEXT_LENGTH, "%s", question);
#endif

    centre_dialog(questdial);

    return (popup_dialog(questdial, focus) == QUESTDIAL_YES ? TRUE : FALSE);
}
#endif

/* Boîte de dialogue. */
static DIALOG controldial[]={
/* (dialog proc)     (x)   (y)  (w)  (h) (fg) (bg) (key) (flags) (d1) (d2) (dp) */
{ d_shadow_box_proc,  20,  10, 280, 180,  0,   0,   0,     0,     0,   0,   NULL },
{ d_ctext_proc,      160,  20,   0,   0,  0,   0,   0,     0,     0,   0,   title },
{ d_ctext_proc,      160,  30,   0,   0,  0,   0,   0,     0,     0,   0,   "Gilles Fétis - Eric Botcazou" },
{ d_ctext_proc,      160,  40,   0,   0,  0,   0,   0,     0,     0,   0,   "Alex Pukall - Jérémie Guillaume" },
{ d_ctext_proc,      160,  50,   0,   0,  0,   0,   0,     0,     0,   0,   "François Mouret - Samuel Devulder" },
#ifdef FRENCH_LANG
{ d_button_proc,      30,  66, 260,  16,  0,   0,  'c',  D_EXIT,  0,   0,   "&Commandes et Réglages..." },
{ d_button_proc,      30,  86, 260,  16,  0,   0,  't',  D_EXIT,  0,   0,   "Lecteur de car&touches..." },
{ d_button_proc,      30, 106, 260,  16,  0,   0,  's',  D_EXIT,  0,   0,   "Lecteur de ca&ssettes..." },
{ d_button_proc,      30, 126, 260,  16,  0,   0,  'd',  D_EXIT,  0,   0,   "Lecteurs de &disquettes..." },
{ d_button_proc,      30, 146, 260,  16,  0,   0,  'i',  D_EXIT,  0,   0,   "&Imprimante matricielle..." },
{ d_button_proc,      30, 170,  80,  16,  0,   0,  'a',  D_EXIT,  0,   0,   "&A Propos" },
{ d_button_proc,     120, 170,  80,  16,  0,   0,  'o',  D_EXIT,  0,   0,   "&OK" },
{ d_button_proc,     210, 170,  80,  16,  0,   0,  'q',  D_EXIT,  0,   0,   "&Quitter" },
#else
{ d_button_proc,      30,  66, 260,  16,  0,   0,  'c',  D_EXIT,  0,   0,   "&Commands and Settings..." },
{ d_button_proc,      30,  86, 260,  16,  0,   0,  'r',  D_EXIT,  0,   0,   "Ca&rtridge reader..." },
{ d_button_proc,      30, 106, 260,  16,  0,   0,  't',  D_EXIT,  0,   0,   "&Tape recorder..." },
{ d_button_proc,      30, 126, 260,  16,  0,   0,  'd',  D_EXIT,  0,   0,   "&Disk drives..." },
{ d_button_proc,      30, 146, 260,  16,  0,   0,  'p',  D_EXIT,  0,   0,   "Dot-matrix &printer..." },
{ d_button_proc,      30, 170,  80,  16,  0,   0,  'a',  D_EXIT,  0,   0,   "&About" },
{ d_button_proc,     120, 170,  80,  16,  0,   0,  'o',  D_EXIT,  0,   0,   "&OK" },
{ d_button_proc,     210, 170,  80,  16,  0,   0,  'q',  D_EXIT,  0,   0,   "&Quit" },
#endif
{ d_yield_proc,       20,  10,   0,   0,  0,   0,   0,     0,     0,   0,   NULL },
{ NULL,                0,   0,   0,   0,  0,   0,   0,     0,     0,   0,   NULL }
};

#define CONTROLDIAL_COMMAND   5
#define CONTROLDIAL_CART      6
#define CONTROLDIAL_CASS      7
#define CONTROLDIAL_DISK      8
#define CONTROLDIAL_PRINTER   9
#define CONTROLDIAL_ABOUT     10
#define CONTROLDIAL_OK        11
#define CONTROLDIAL_QUIT      12


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

            case CONTROLDIAL_ABOUT:
                MenuAbout();
                break;

            case -1:  /* ESC */
            case CONTROLDIAL_OK:
                return;

            case CONTROLDIAL_QUIT:
                /*
                if (PopupQuestion(is_fr?"Voulez-vous vraiment quitter ?":"Do you really want to quit ?", FOCUS_NO))
                {
                */
                    teo.command=QUIT;
                    return;
                /*
                }
                */
        }
}



/* SetGUIColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void SetGUIColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(mesgdial, fg_color, bg_color);
/*    set_dialog_color(questdial, fg_color, bg_color); */
    
    SetCommGUIColors(fg_color, bg_color, bg_entry_color);
    SetDiskGUIColors(fg_color, bg_color, bg_entry_color);
    SetCassGUIColors(fg_color, bg_color, bg_entry_color);
    SetMemoGUIColors(fg_color, bg_color, bg_entry_color);
    SetPrinterGUIColors(fg_color, bg_color, bg_entry_color);
    SetAboutGUIColors(fg_color, bg_color, bg_entry_color);

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
#ifdef DJGPP
    (void)sprintf (title, "%s", version_name);
#else
    (void)snprintf (title, TEXT_LENGTH, "%s", version_name);
#endif
    InitCommGUI(version_name, gfx_mode, title);
    InitDiskGUI(direct_disk_support, title);
    InitMemoGUI(title);
    InitCassGUI(title);
    InitPrinterGUI(title);
    InitAboutGUI(title);
}



/* PopupMessage:
 *  Affiche une boîte de dialogue contenant le message et un bouton OK.
 */
void PopupMessage(const char message[])
{
    mesgdial[MESGDIAL_SHADOW].w=strlen(message)*8+16;
    mesgdial[MESGDIAL_MESG].x=mesgdial[MESGDIAL_SHADOW].x+mesgdial[MESGDIAL_SHADOW].w/2;
    mesgdial[MESGDIAL_OK].x=mesgdial[MESGDIAL_MESG].x-(mesgdial[MESGDIAL_OK].w+BUTTON_FIX)/2;

#ifdef DJGPP
    (void)sprintf (mesg, "%s", message);
#else
    (void)snprintf (mesg, TEXT_LENGTH, "%s", message);
#endif

    centre_dialog(mesgdial);

    popup_dialog(mesgdial, MESGDIAL_OK);
}

