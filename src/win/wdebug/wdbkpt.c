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
 *  Module     : win/wdebug/wdbkpt.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 15/07/2016
 *
 *  Débogueur 6809 - Gestion des breakpoints.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "debug/debug.h"
#include "win/gui.h"


/* write_breakpoint:
 *  Overwrite the breakpoint value.
 */
static void write_breakpoint (HWND hDlg, int number, int value)
{
    char str[6] = "";
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_EDIT_BKPT1+number);

    teo.debug.breakpoint[number] = value;
    snprintf (str, 5, "%04X", value);
    Edit_SetText (hwnd, str);
}



/* update_display:
 *  Update the display.
 */
static void update_display (HWND hDlg)
{
    int i;
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_TOOL_BAR);

    /* disable the RUN button if no breakpoint */
    for (i=0; (teo.debug.breakpoint[i] == -1) && (i<MAX_BREAKPOINTS); i++);
    SendMessage(
        hwnd,
        TB_SETSTATE,
        (WPARAM)IDM_DEBUG_BUTTON_RUN,
        (LPARAM)(i == MAX_BREAKPOINTS)?TBSTATE_INDETERMINATE:TBSTATE_ENABLED
    );
}    
    



/* ------------------------------------------------------------------------- */


/* wdbkpt_Init:
 *  Init breakpoints.
 */
void wdbkpt_Init(HWND hDlg)
{
    HWND hwnd;
    int i;

    for (i=0; i<MAX_BREAKPOINTS; i++)
    {
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_EDIT_BKPT1+i);
        Edit_LimitText(hwnd, 4);
        if (teo.debug.breakpoint[i] != -1)
            write_breakpoint (hDlg, i, teo.debug.breakpoint[i]);
    }
    update_display (hDlg);
}



/* wdbkpt_Update:
 *  Update the breakpoint display.
 */
void wdbkpt_Update (HWND hDlg, int number)
{
    int addr = 0;
    char str[5] = "";
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_EDIT_BKPT1+number);

    if (Edit_GetTextLength (hwnd) == 0)
    {
        teo.debug.breakpoint[number] = -1;
    }
    else
    {
        Edit_GetText(hwnd, str, 5);
        sscanf (str, "%X", &addr);
        teo.debug.breakpoint[number] = addr;
        write_breakpoint (hwnd, number, addr);
    }
    update_display (hDlg);
}



/* wdbkpt_Exit:
 *  Exit the breakpoints.
 */
void wdbkpt_Exit(HWND hDlg)
{
    (void)hDlg;
}
