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
 *  Module     : win/wdebug/wdreg.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 15/07/2016
 *
 *  Débogueur 6809 - Affichage des registres.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/mc6821.h"
#include "mc68xx/dasm6809.h"
#include "media/disk.h"
#include "debug/debug.h"
#include "win/gui.h"

static HFONT hfont_normal = NULL;


void display (HWND hwnd)
{
    char *text = NULL;

    text = dreg_GetText ("\r\n");
    if (text != NULL)
    {
        Edit_SetText (hwnd, text);
        Edit_Scroll (hwnd, teo.debug.extra_first_line, 0);
        std_free (text);
    }
}


/* ------------------------------------------------------------------------- */


/* wdreg_Init:
 *  Init register area.
 */
void wdreg_Init(HWND hDlg)
{
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_REG_EDIT);

    hfont_normal = wdebug_GetNormalFixedWidthHfont();
    if (hfont_normal != NULL)
        SetWindowFont(hwnd, hfont_normal, TRUE);

    display (hwnd);
}



/* wdreg_Display:
 *  Display extra registers.
 */
void wdreg_Display(HWND hDlg)
{
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_REG_EDIT);

    teo.debug.extra_first_line = (int)Edit_GetFirstVisibleLine(hwnd);
    display (hwnd);
}



/* wdreg_Exit:
 *  Exit the register area.
 */
void wdreg_Exit(HWND hDlg)
{
    HWND hwnd;

    if (hfont_normal != NULL)
    {
        (void)DeleteObject((HGDIOBJ)hfont_normal);
        hfont_normal = NULL;
    }

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_REG_EDIT);
    teo.debug.extra_first_line = (int)Edit_GetFirstVisibleLine(hwnd);
}
