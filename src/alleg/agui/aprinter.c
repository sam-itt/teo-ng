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
 *  Version    : 1.8.5
 *  Cr�� par   : Gilles F�tis 1998
 *  Modifi� par: J�r�mie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               Fran�ois Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               24/10/2012
 *               Samuel Cuella 02/2020
 *
 *  Gestion de l'imprimante.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif
#include <assert.h>

#include "std.h"
#include "teo.h"
#include "gettext.h"
#include "alleg/gui.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "media/printer.h"


/* Proc�dure pour la text_list */
static char *listbox_getter(int index, int *list_size)
{
    if (index >= 0)
        return printer_code_list[index].name;
    *list_size = PRINTER_NUMBER;
    return NULL;
}


#define PRINTERDIAL_FOLDER      3
#define PRINTERDIAL_MORE        4
#define PRINTERDIAL_LIST        6
#define PRINTERDIAL_DIP         7
#define PRINTERDIAL_NLQ         8
#define PRINTERDIAL_RAW_OUTPUT  10
#define PRINTERDIAL_TXT_OUTPUT  11
#define PRINTERDIAL_GFX_OUTPUT  12
#define PRINTERDIAL_OK          13
#define PRINTERDIAL_LEN         17

static DIALOG *_printerdial = NULL;
#define aprinter_GetDialog() (_printerdial ? _printerdial : aprinter_AllocDialog())


/* ------------------------------------------------------------------------- */

static DIALOG *aprinter_AllocDialog(void)
{
    DIALOG *rv;
    int i;

    if(_printerdial)
        return _printerdial;

    rv = malloc(sizeof(DIALOG)*PRINTERDIAL_LEN);
    i = 0;

/*                         dialog proc       x    y    w    h  fg bg  key flags   d1 d2  dp */
    rv[i++] = (DIALOG){ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,  0,     0, 0, NULL };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, _("Dot-matrix printers") };
    rv[i++] = (DIALOG){ d_text_proc,        30,  44,   0,   0, 0, 0,   0,  0,     0, 0, _("Save into:") };
    rv[i++] = (DIALOG){ d_textbox_proc,    132,  40, 116,  16, 0, 0,   0,  0,     0, 0, NULL };
    rv[i++] = (DIALOG){ d_button_proc,     258,  40,  30,  16, 0, 0, '0', D_EXIT, 0, 0, "..." };
    rv[i++] = (DIALOG){ d_text_proc,        70,  74,   0,   0, 0, 0,   0,  0,     0, 0, _("Printer:") };
    rv[i++] = (DIALOG){ d_list_proc,       140,  64, 100,  30, 0, 0,   0,  0,     0, 0, (void *)listbox_getter};
    rv[i++] = (DIALOG){ d_button_proc,      40, 100, 240,  16, 0, 0, 's',  0,     0, 0, _("Double &spacing") };
    rv[i++] = (DIALOG){ d_button_proc,      40, 118, 240,  16, 0, 0, 'h',  0,     0, 0, _("&High quality print") };
    rv[i++] = (DIALOG){ d_text_proc,        30, 145,   0,   0, 0, 0,   0,  0,     0, 0, _("Output:") };
    rv[i++] = (DIALOG){ d_check_proc,       90, 142,  60,  14, 0, 0, 'r',  0,     0, 0, _("&raw") };
    rv[i++] = (DIALOG){ d_check_proc,      160, 142,  60,  14, 0, 0, 't',  0,     0, 0, _("&text") };
    rv[i++] = (DIALOG){ d_check_proc,      220, 142,  50,  14, 0, 0, 'g',  0,     0, 0, _("&graphic") };
    rv[i++] = (DIALOG){ d_button_proc,     210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" };
    rv[i++] = (DIALOG){ d_yield_proc,       20,  10,   0,   0, 0, 0,   0,  0,     0, 0, NULL };
    rv[i++] = (DIALOG){ NULL,                0,   0,   0,   0, 0, 0,   0,  0,     0, 0, NULL };

    assert(i <= PRINTERDIAL_LEN);

    _printerdial = rv;
    return rv;
}


/* aprinter_Panel:
 *  Affiche le panneau de commandes de l'imprimante.
 */
void aprinter_Panel(void)
{
    int i;
    static int first = 1;
    static char printer_folder[MAX_PATH+1] = "";
    int printer_number = 0;
    DIALOG *printerdial;

    printerdial = aprinter_GetDialog();

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
                (void)file_select_ex(_("Select a folder:"),
                               printer_folder, "/+d", MAX_PATH,
                               OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT);
                i = strlen (printer_folder) - 1;
                if (i > 0)
                    while ((i >= 0) && (printer_folder[i] == DIR_SEPARATOR))
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
    DIALOG *printerdial;

    printerdial = aprinter_GetDialog();
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
 *  Lib�re le module interface utilisateur.
 */
void aprinter_Free(void)
{
    DIALOG *printerdial;

    printerdial = aprinter_GetDialog();
    printerdial[PRINTERDIAL_FOLDER].dp = std_free (printerdial[PRINTERDIAL_FOLDER].dp);
}
