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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : win/wdebug/wdstatus.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 15/07/2016
 *
 *  Débogueur 6809 - Gestion de la barre d'état.
 */

#ifndef SCAN_DEPEND
    #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "win/gui.h"

static mc6809_clock_t prev_clock = 0;


/* wdstatus_Init:
 *  Initialize the status bar.
 */
void wdstatus_Init (HWND hDlg)
{
    HWND hwnd;
    int status_width_list[] = {120, 210, 300, 390, -1};
    int count = sizeof(status_width_list)/sizeof(int);

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_STATUS_BAR);
    SendMessage(hwnd, SB_SETPARTS, count, (LPARAM)status_width_list);
}



/* wdstatus_Display:
 *  Update the display of the status bar.
 */
void wdstatus_Display (HWND hDlg)
{
    char str[40] = "";
    mc6809_clock_t clock = mc6809_clock();
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_STATUS_BAR);

    /* display the clock */
    str[0] = '\0';
    sprintf (str, "%s: %lld", is_fr?"Horloge":"Clock", clock-prev_clock);
    SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)str);
    prev_clock = clock;

    /* display the clock */
    clock %= TEO_CYCLES_PER_FRAME;
    str[0] = '\0';
    sprintf (str, "%s: %lld", is_fr?"Frame":"Frame", clock);
    SendMessage(hwnd, SB_SETTEXT, 1, (LPARAM)str);

    /* display the number of the line */
    str[0] = '\0';
    sprintf (str, "%s: %lld", is_fr?"Ligne":"Line", clock/FULL_LINE_CYCLES);
    SendMessage(hwnd, SB_SETTEXT, 2, (LPARAM)str);

    /* display the number of the column */
    str[0] = '\0';
    sprintf (str, "%s: %lld", is_fr?"Colonne":"Column", clock%FULL_LINE_CYCLES);
    SendMessage(hwnd, SB_SETTEXT, 3, (LPARAM)str);
}

