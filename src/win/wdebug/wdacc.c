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
 *  Module     : win/wdebug/wdacc.c
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 04/06/2015 15/07/2016
 *
 *  Débogueur 6809 - Affichage des accumulateurs.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "mc68xx/mc6809.h"
#include "win/gui.h"
#include "debug/debug.h"

static HFONT hfont_normal = NULL;
static HFONT hfont_bold = NULL;
static struct MC6809_REGS regs;


/* display_accumulators:
 *  Display the 6809 registers.
 */
static void display (HWND hDlg)
{
    HWND hwnd;
    char string[30] = "";

    mc6809_GetRegs(&regs);

    sprintf (string, "%02X=%c%c%c%c%c%c%c%c"
                 , regs.cc
                 , regs.cc&0x80 ? 'E' : '.'
                 , regs.cc&0x40 ? 'F' : '.'
                 , regs.cc&0x20 ? 'H' : '.'
                 , regs.cc&0x10 ? 'I' : '.'
                 , regs.cc&0x08 ? 'N' : '.'
                 , regs.cc&0x04 ? 'Z' : '.'
                 , regs.cc&0x02 ? 'V' : '.'
                 , regs.cc&0x01 ? 'C' : '.');
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_CC_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    sprintf (string, "%02X", regs.ar);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_AR_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    sprintf (string, "%02X", regs.br);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_BR_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    sprintf (string, "%02X", regs.dp);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_DP_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    dacc_GetDumpFor16Bits (string, regs.xr);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_XR_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    dacc_GetDumpFor16Bits (string, regs.yr);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_YR_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    dacc_GetDumpFor16Bits (string, regs.ur);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_UR_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    dacc_GetDumpFor16Bits (string, regs.sr);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_SR_VALUE_LTEXT);
    SetWindowText(hwnd, string);

    dacc_GetDumpFor16Bits (string, regs.pc);
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_PC_VALUE_LTEXT);
    SetWindowText(hwnd, string);
}

 
/* ------------------------------------------------------------------------- */


/* wdacc_Init:
 *  Init accumulators area.
 */
void wdacc_Init(HWND hDlg)
{
    HWND hwnd;

    hfont_normal = wdebug_GetNormalFixedWidthHfont();
    if (hfont_normal != NULL)
    {
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_CC_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_AR_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_BR_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_DP_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_XR_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_YR_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_UR_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_SR_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_PC_VALUE_LTEXT);
        SetWindowFont(hwnd, hfont_normal, TRUE);
    }

    hfont_bold = wdebug_GetBoldFixedWidthHfont();
    if (hfont_bold != NULL)
    {
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_CC_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_AR_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_BR_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_DP_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_XR_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_YR_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_UR_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_SR_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_PC_NAME_LTEXT);
        SetWindowFont(hwnd, hfont_bold, TRUE);
    }
}



/* wdacc_Display:
 *  Display accumulators.
 */
void wdacc_Display(HWND hDlg)
{
    display (hDlg);
}



/* wdacc_Exit:
 *  Exit the accumulators area.
 */
void wdacc_Exit(HWND hDlg)
{
    if (hfont_normal != NULL)
    {
        (void)DeleteObject((HGDIOBJ)hfont_normal);
        hfont_normal = NULL;
    }

    if (hfont_bold != NULL)
    {
        (void)DeleteObject((HGDIOBJ)hfont_bold);
        hfont_bold = NULL;
    }

    (void)hDlg;
}
