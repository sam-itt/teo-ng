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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               24/10/2012
 *
 *  Gestion de l'imprimante.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "std.h"
#include "teo.h"
#include "alleg/gui.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "media/printer.h"


/* Procédure pour la text_list */
static char *listbox_getter(int index, int *list_size)
{
    if (index >= 0)
        return printer_code_list[index].name;
    *list_size = PRINTER_NUMBER;
    return NULL;
}

/* Boîte de dialogue. */
static DIALOG printerdial[]={
/*  dialog proc       x    y    w    h  fg bg  key flags   d1 d2  dp */
{ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,  0,     0, 0, NULL },
#ifdef FRENCH_LANGUAGE
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "Imprimantes matricielles" },
{ d_text_proc,        30,  44,   0,   0, 0, 0,   0,  0,     0, 0, "Sauver dans:" },
{ d_textbox_proc,    132,  40, 116,  16, 0, 0,   0,  0,     0, 0, NULL },
{ d_button_proc,     258,  40,  30,  16, 0, 0, '0', D_EXIT, 0, 0, "..." },
{ d_text_proc,        60,  74,   0,   0, 0, 0,   0,  0,     0, 0, "Imprimante:" },
{ d_list_proc,       150,  64, 100,  30, 0, 0,   0,  0,     0, 0, (void *)listbox_getter},
{ d_button_proc,      40, 100, 240,  16, 0, 0, 'i',  0,     0, 0, "Double &interligne" },
{ d_button_proc,      40, 118, 240,  16, 0, 0, 'h',  0,     0, 0, "Imprime en de &haute qualité" },
{ d_text_proc,        30, 145,   0,   0, 0, 0,   0,  0,     0, 0, "Sortie:" },
{ d_check_proc,       90, 142,  60,  14, 0, 0, 'b',  0,     0, 0, "&brute" },
{ d_check_proc,      160, 142,  60,  14, 0, 0, 't',  0,     0, 0, "&texte" },
{ d_check_proc,      230, 142,  50,  14, 0, 0, 'g',  0,     0, 0, "&graph" },
#else
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "Dot-matrix printers" },
{ d_text_proc,        30,  44,   0,   0, 0, 0,   0,  0,     0, 0, "Save in:" },
{ d_textbox_proc,    100,  40, 148,  16, 0, 0,   0,  0,     0, 0, NULL },
{ d_button_proc,     258,  40,  30,  16, 0, 0, '0', D_EXIT, 0, 0, "..." },
{ d_text_proc,        70,  74,   0,   0, 0, 0,   0,  0,     0, 0, "Printer:" },
{ d_list_proc,       140,  64, 100,  30, 0, 0,   0,  0,     0, 0, (void *)listbox_getter},
{ d_button_proc,      40, 100, 240,  16, 0, 0, 's',  0,     0, 0, "Double &spacing" },
{ d_button_proc,      40, 118, 240,  16, 0, 0, 'h',  0,     0, 0, "&High quality print" },
{ d_text_proc,        30, 145,   0,   0, 0, 0,   0,  0,     0, 0, "Output:" },
{ d_check_proc,       98, 142,  42,  14, 0, 0, 'r',  0,     0, 0, "&raw" },
{ d_check_proc,      152, 142,  50,  14, 0, 0, 't',  0,     0, 0, "&text" },
{ d_check_proc,      214, 142,  66,  14, 0, 0, 'g',  0,     0, 0, "&graphic" },
#endif
{ d_button_proc,     210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" },
{ d_yield_proc,       20,  10,   0,   0, 0, 0,   0,  0,     0, 0, NULL },
{ NULL,                0,   0,   0,   0, 0, 0,   0,  0,     0, 0, NULL }
};

#define PRINTERDIAL_FOLDER      3
#define PRINTERDIAL_MORE        4
#define PRINTERDIAL_LIST        6
#define PRINTERDIAL_DIP         7
#define PRINTERDIAL_NLQ         8
#define PRINTERDIAL_RAW_OUTPUT  10
#define PRINTERDIAL_TXT_OUTPUT  11
#define PRINTERDIAL_GFX_OUTPUT  12
#define PRINTERDIAL_OK          13


/* ------------------------------------------------------------------------- */


/* aprinter_Panel:
 *  Affiche le panneau de commandes de l'imprimante.
 */
void aprinter_Panel(void)
{
    int i;
    static int first = 1;
    static char printer_folder[MAX_PATH+1] = "";
    int printer_number = 0;

    centre_dialog(printerdial);
    if (first)
    {
        if (teo.lprt.folder == NULL)
        {
            (void)getcwd (printer_folder, MAX_PATH);
            teo.lprt.folder = std_strdup_printf ("%s", printer_folder);
        }
        printerdial[PRINTERDIAL_FOLDER].dp = std_strdup_printf ("%s", get_filename(teo.lprt.folder));
        if (teo.lprt.dip)
            printerdial[PRINTERDIAL_DIP].flags|=D_SELECTED;
        if (teo.lprt.nlq)
            printerdial[PRINTERDIAL_NLQ].flags|=D_SELECTED;
        if (teo.lprt.raw_output)
            printerdial[PRINTERDIAL_RAW_OUTPUT].flags|=D_SELECTED;
        if (teo.lprt.txt_output)
            printerdial[PRINTERDIAL_TXT_OUTPUT].flags|=D_SELECTED;
        if (teo.lprt.gfx_output)
            printerdial[PRINTERDIAL_GFX_OUTPUT].flags|=D_SELECTED;
        for (i=0; i<PRINTER_NUMBER; i++)
        {
            if (teo.lprt.number == printer_code_list[i].number)
                printer_number = i;
        }
        printerdial[PRINTERDIAL_LIST].d1 = printer_number;

        first = 0;
    }

    clear_keybuf();

    while (TRUE)
    {
        switch (popup_dialog(printerdial, PRINTERDIAL_OK))
        {
            case PRINTERDIAL_LIST:
                agui_PopupMessage ("12");
                break;

            case PRINTERDIAL_MORE:
                (void)file_select_ex(is_fr?"Choisissez un répertoire:":"Choose a folder:",
                               printer_folder, "/+s+d", MAX_PATH,
                               OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT);
                i = strlen (printer_folder) - 1;
                if (i > 0)
                    while ((i >= 0) && (printer_folder[i] == '\\'))
                        printer_folder[i--] = '\0';
                printerdial[PRINTERDIAL_FOLDER].dp = std_free (printerdial[PRINTERDIAL_FOLDER].dp);
                printerdial[PRINTERDIAL_FOLDER].dp = std_strdup_printf ("%s", get_filename(printer_folder));
                teo.lprt.folder = std_free (teo.lprt.folder);
                teo.lprt.folder = std_strdup_printf ("%s", printer_folder);
                break;

            case -1:  /* ESC */
            case PRINTERDIAL_OK:
                teo.lprt.number = printer_code_list[printerdial[PRINTERDIAL_LIST].d1].number;
                teo.lprt.dip = (printerdial[PRINTERDIAL_DIP].flags & D_SELECTED) ? TRUE : FALSE;
                teo.lprt.nlq = (printerdial[PRINTERDIAL_NLQ].flags & D_SELECTED) ? TRUE : FALSE;
                teo.lprt.raw_output = (printerdial[PRINTERDIAL_RAW_OUTPUT].flags & D_SELECTED) ? TRUE : FALSE;
                teo.lprt.txt_output = (printerdial[PRINTERDIAL_TXT_OUTPUT].flags & D_SELECTED) ? TRUE : FALSE;
                teo.lprt.gfx_output = (printerdial[PRINTERDIAL_GFX_OUTPUT].flags & D_SELECTED) ? TRUE : FALSE;
                return;
        }
    }
}


/* aprinter_SetColors:
 *  Fixe les couleurs de l'interface utilisateur.
 */
void aprinter_SetColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(printerdial, fg_color, bg_color);
    printerdial[PRINTERDIAL_FOLDER].bg = bg_entry_color;
}



/* aprinter_Init:
 *  Initialise le module interface utilisateur.
 */
void aprinter_Init(void)
{
}



/* aprinter_Free:
 *  Libère le module interface utilisateur.
 */
void aprinter_Free(void)
{
     printerdial[PRINTERDIAL_FOLDER].dp = std_free (printerdial[PRINTERDIAL_FOLDER].dp);
}
