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
 *  Module     : alleg/agui/aprinter.c
 *  Version    : 1.8.2
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *
 *  Gestion de l'imprimante.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "intern/gui.h"
#include "to8.h"



/* Répertoire de chemin de fichier. */
#define FOLDER_NAME_LENGTH 40
static char foldername[FOLDER_NAME_LENGTH+1] = "";
/* Liste des imprimantes */
#define PRINTER_NUMBER 3
struct PRINTER_CODE_LIST {
    char name[9];
    int  number;
};
static struct PRINTER_CODE_LIST prt_list[PRINTER_NUMBER] = {
    { "PR90-055",  55 },
    { "PR90-600", 600 },
    { "PR90-612", 612 }
};
/* Procédure pour la text_list */
static char *listbox_getter(int index, int *list_size)
{
    if (index >= 0)
        return prt_list[index].name;
    *list_size = 3;
    return NULL;
}

/* Boîte de dialogue. */
static DIALOG printerdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,    0,    0,     0,     0,    0,    0,       0,    0,    NULL },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Imprimantes matricielles" },
{ d_text_proc,        30,  54,    0,    0,     0,     0,    0,    0,       0,    0,    "Sauver dans:" },
{ d_textbox_proc,    132,  50,  116,   16,     0,     0,    0,    0,       0,    0,    foldername },
{ d_button_proc,     258,  50,   30,   16,     0,     0,  '0',    D_EXIT,  0,    0,    "..." },
{ d_text_proc,        50,  84,    0,    0,     0,     0,    0,    0,       0,    0,    "Imprimante:" },
{ d_list_proc,        50, 100,  100,   30,     0,     0,    0,    0,       0,    0,    (void *)listbox_getter},
{ d_button_proc,      50, 140,   44,   16,     0,     0,  'd',    0,       0,    0,    "&Dip" },
{ d_button_proc,     106, 140,   44,   16,     0,     0,  'n',    0,       0,    0,    "&Nlq" },
{ d_text_proc,       200,  84,    0,    0,     0,     0,    0,    0,       0,    0,    "Sortie:" },
{ d_check_proc,      210, 100,   60,   14,     0,     0,  'b',    0,       0,    0,    "&brute" },
{ d_check_proc,      210, 120,   60,   14,     0,     0,  't',    0,       0,    0,    "&texte" },
{ d_check_proc,      178, 140,   94,   14,     0,     0,  'g',    0,       0,    0,    "&graphique" },
#else
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Dot-matrix printers" },
{ d_text_proc,        30,  54,    0,    0,     0,     0,    0,    0,       0,    0,    "Save in:" },
{ d_textbox_proc,    100,  50,  148,   16,     0,     0,    0,    0,       0,    0,    foldername },
{ d_button_proc,     258,  50,   30,   16,     0,     0,  '0',    D_EXIT,  0,    0,    "..." },
{ d_text_proc,        50,  84,    0,    0,     0,     0,    0,    0,       0,    0,    "Printer:" },
{ d_list_proc,        50, 100,  100,   30,     0,     0,    0,    0,       0,    0,    (void *)listbox_getter},
{ d_button_proc,      50, 140,   44,   16,     0,     0,  'd',    0,       0,    0,    "&Dip" },
{ d_button_proc,     106, 140,   44,   16,     0,     0,  'n',    0,       0,    0,    "&Nlq" },
{ d_text_proc,       206,  84,    0,    0,     0,     0,    0,    0,       0,    0,    "Output:" },
{ d_check_proc,      226, 100,   42,   14,     0,     0,  'r',    0,       0,    0,    "&raw" },
{ d_check_proc,      218, 120,   50,   14,     0,     0,  't',    0,       0,    0,    "&text" },
{ d_check_proc,      194, 140,   76,   14,     0,     0,  'g',    0,       0,    0,    "&graphic" },
#endif
{ d_button_proc,     30,  170,  126,   16,     0,     0,   'o',   D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,      20,   10,    0,    0,     0,     0,    0,    0,       0,    0,    NULL },
{ NULL,               0,    0,    0,    0,     0,     0,    0,    0,       0,    0,    NULL }
};

#define PRINTERDIAL_FOLDER      4
#define PRINTERDIAL_MORE        5
#define PRINTERDIAL_LIST        7
#define PRINTERDIAL_DIP         8
#define PRINTERDIAL_NLQ         9
#define PRINTERDIAL_RAW_OUTPUT  11
#define PRINTERDIAL_TXT_OUTPUT  12
#define PRINTERDIAL_GFX_OUTPUT  13
#define PRINTERDIAL_OK          14


/* MenuPrinter:
 *  Affiche le panneau de commandes de l'imprimante.
 */
void MenuPrinter(void)
{
    int i;
    static int first = 1;
    static char printer_folder[MAX_PATH+1] = "";

    centre_dialog(printerdial);
    if (first)
    {
        if (strlen(gui->lprt.folder) == 0)
        {
            (void)getcwd (printer_folder, MAX_PATH);
            (void)sprintf (gui->lprt.folder, "%s", printer_folder);
        }
        (void)sprintf (foldername, "%s", get_filename(gui->lprt.folder));
        if (gui->lprt.dip)
            printerdial[PRINTERDIAL_DIP].flags|=D_SELECTED;
        if (gui->lprt.nlq)
            printerdial[PRINTERDIAL_NLQ].flags|=D_SELECTED;
        if (gui->lprt.raw_output)
            printerdial[PRINTERDIAL_RAW_OUTPUT].flags|=D_SELECTED;
        if (gui->lprt.txt_output)
            printerdial[PRINTERDIAL_TXT_OUTPUT].flags|=D_SELECTED;
        if (gui->lprt.gfx_output)
            printerdial[PRINTERDIAL_GFX_OUTPUT].flags|=D_SELECTED;
        for (i=0; i<PRINTER_NUMBER; i++)
        {
             if (gui->lprt.number == prt_list[i].number)
                 printerdial[PRINTERDIAL_LIST].d1 = i;
        }
        first = 0;
    }

    clear_keybuf();

    while (TRUE)
    {
        switch (popup_dialog(printerdial, PRINTERDIAL_OK))
        {
            case PRINTERDIAL_MORE:
                (void)file_select_ex(is_fr?"Choisissez un répertoire:":"Choose a folder:", printer_folder, "/+s+d",
                               MAX_PATH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT);
                i = strlen (printer_folder) - 1;
                if (i > 0)
                    while ((i >= 0) && (printer_folder[i] == '\\'))
                        printer_folder[i--] = '\0';
#ifdef DJGPP
                (void)sprintf (foldername, "%s", get_filename(printer_folder));
                (void)sprintf (gui->lprt.folder, "%s", printer_folder);
#else
                (void)snprintf (foldername, FOLDER_NAME_LENGTH, "%s", get_filename(printer_folder));
                (void)snprintf (gui->lprt.folder, MAX_PATH, "%s", printer_folder);
#endif
                break;

            case -1:  /* ESC */
            case PRINTERDIAL_OK:
                gui->lprt.number = prt_list[printerdial[PRINTERDIAL_LIST].d1].number;
                gui->lprt.dip = (printerdial[PRINTERDIAL_DIP].flags & D_SELECTED) ? TRUE : FALSE;
                gui->lprt.nlq = (printerdial[PRINTERDIAL_NLQ].flags & D_SELECTED) ? TRUE : FALSE;
                gui->lprt.raw_output = (printerdial[PRINTERDIAL_RAW_OUTPUT].flags & D_SELECTED) ? TRUE : FALSE;
                gui->lprt.txt_output = (printerdial[PRINTERDIAL_TXT_OUTPUT].flags & D_SELECTED) ? TRUE : FALSE;
                gui->lprt.gfx_output = (printerdial[PRINTERDIAL_GFX_OUTPUT].flags & D_SELECTED) ? TRUE : FALSE;
                return;
        }
    }
}


void SetPrinterGUIColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(printerdial, fg_color, bg_color);
    printerdial[PRINTERDIAL_FOLDER].bg = bg_entry_color;
}



/* InitPrinterGUI:
 *  Initialise le module interface utilisateur.
 */
void InitPrinterGUI(char *title)
{
    /* Définit le titre de la fenêtre */
    printerdial[1].dp = title;
}
