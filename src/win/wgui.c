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
 *  Module     : win/gui.c
 *  Version    : 1.8.5
 *  Cr�� par   : Eric Botcazou 28/11/2000
 *  Modifi� par: Eric Botcazou 28/10/2003
 *               Fran�ois Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               21/09/2012 18/09/2013 11/04/2014
 *                               04/06/2015
 *               Samuel Cuella 02/2020
 *
 *  Interface utilisateur Windows native.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
#endif

#include "teo.h"
#include "alleg/gfxdrv.h"
#include "win/gui.h"
#include "gettext.h"

/* ressources globales de l'application */
#define NBTABS_MASTER 5
HINSTANCE prog_inst;
HWND prog_win;
HICON prog_icon;
HWND hTab[NBTABS_MASTER];

static int nCurrentTab = 0;



/* ShowTab:
 * Masque l'onglet actif et affiche l'onglet demand�
 */
static void
ShowTab(HWND hDlg)
{
    ShowWindow(hTab[nCurrentTab], SW_HIDE);
    nCurrentTab = TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_CONTROL_TAB));
    ShowWindow(hTab[nCurrentTab], SW_SHOW);
}



/* CreateTab:
 * Cr�e un onglet
 */
static HWND CreateTab(HWND hDlg, WORD number, char *title, WORD id,
                      int (CALLBACK *prog)(HWND, UINT, WPARAM, LPARAM))
{
    RECT rect0;
    RECT rect1;
    HWND hTabItem;
    HWND hMyTab;
    TCITEM tcitem;

    tcitem.mask = TCIF_TEXT;
    hTabItem = GetDlgItem(hDlg, IDC_CONTROL_TAB);

    /* cr�ation du dialogue enfant */
    hMyTab = CreateDialog(prog_inst, MAKEINTRESOURCE(id), hDlg, prog);

    /* ajout de l'onglet */
    tcitem.pszText = (LPTSTR)title;
    TabCtrl_InsertItem(hTabItem, number, &tcitem);

    /* d�finit le rectangle en rapport � la bo�te de dialogue parente */
    GetWindowRect(hTabItem, &rect0);
    SendMessage(hTabItem, TCM_ADJUSTRECT, FALSE, (LPARAM)&rect0);
    GetWindowRect(hMyTab, &rect1);
    SetWindowPos(hMyTab, NULL, rect0.left-rect1.left,
                               rect0.top-rect1.top,
                               rect0.right-rect0.left,
                               rect0.bottom-rect0.top,
                               SWP_NOZORDER|SWP_NOREDRAW);
    /* masque l'onglet */
    ShowWindow(hMyTab, SW_HIDE);

    return hMyTab;
}



/* ControlDialogProc:
 *  Proc�dure du panneau de contr�le.
 */
static BOOL CALLBACK ControlDialogProc(HWND hDlg, UINT uMsg,
                                        WPARAM wParam, LPARAM lParam)
{
   LPNMHDR lpnmhdr;
   int i;
   int response;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         SetWindowText(hDlg, _("Control panel"));
         SetWindowText(GetDlgItem(hDlg, IDC_RESET_BUTTON), _("Warm reset"));
         SetWindowText(GetDlgItem(hDlg, IDC_COLDRESET_BUTTON), _("Cold reset"));
         SetWindowText(GetDlgItem(hDlg, IDC_FULLRESET_BUTTON), _("Full reset"));
         SetWindowText(GetDlgItem(hDlg, IDC_ABOUT_BUTTON), _("About"));
         SetWindowText(GetDlgItem(hDlg, IDC_QUIT_BUTTON), _("Quit"));

         /* Cree les onglets */
         hTab[0] = CreateTab(hDlg, 0, _("Settings"),
                             IDC_SETTING_TAB, wsetting_TabProc);
         hTab[1] = CreateTab(hDlg, 1, _("Disks"),
                             IDC_DISK_TAB, wdisk_TabProc);
         hTab[2] = CreateTab(hDlg, 2, _("Tape"),
                             IDC_K7_TAB, wcass_TabProc);
         hTab[3] = CreateTab(hDlg, 3, _("Cartridge"),
                             IDC_MEMO7_TAB, wmemo_TabProc);
         hTab[4] = CreateTab(hDlg, 4, _("Printer"),
                             IDC_PRINTER_TAB, wprinter_TabProc);
         TabCtrl_SetCurSel(GetDlgItem(hDlg, IDC_CONTROL_TAB), nCurrentTab);
         ShowTab(hDlg);

         /* mise en place de l'icone */
         SetClassLong(hDlg, GCL_HICON,   (LONG) prog_icon);
         SetClassLong(hDlg, GCL_HICONSM, (LONG) prog_icon);

         /* cree les tooltips */
         wgui_CreateTooltip (hDlg, IDC_RESET_BUTTON,
								 _("Warm reset without\nclearing RAM memory"));
         wgui_CreateTooltip (hDlg, IDC_COLDRESET_BUTTON,
                                 _("Cold reset without\nclearing RAM memory"));
         wgui_CreateTooltip (hDlg, IDC_FULLRESET_BUTTON,
                                 _("Cold reset and\nclear the RAM memory"));
         return TRUE;

      case WM_DESTROY :
         for(i=0;i<NBTABS_MASTER;i++)
            if (hTab[i] != NULL)
                DestroyWindow(hTab[i]);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDOK:
               EndDialog(hDlg, IDOK);
               break;

            case IDC_QUIT_BUTTON:
               EndDialog(hDlg, IDCANCEL);
               break; 

            case IDC_ABOUT_BUTTON:
               (void)DialogBox (prog_inst, MAKEINTRESOURCE(IDC_ABOUT_DIALOG),
                                hDlg, (DLGPROC)wabout_Proc);
               break;

            case IDC_RESET_BUTTON:
               teo.command = TEO_COMMAND_RESET;
               EndDialog(hDlg, IDOK);
               break;

            case IDC_COLDRESET_BUTTON:
               teo.command = TEO_COMMAND_COLD_RESET;
               EndDialog(hDlg, IDOK);
               break;

            case IDC_FULLRESET_BUTTON:
               response = MessageBox(
                                NULL,
                                _("All the RAM memory will be cleared."),
                                _("Teo - Question"),
                                MB_OKCANCEL | MB_ICONEXCLAMATION);
		       if (response == IDOK)
		       {
                    teo.command = TEO_COMMAND_FULL_RESET;
                    EndDialog(hDlg, IDOK);
               }
               break;

            case WM_DESTROY:
               EndDialog(hDlg, IDOK);
               break;
         }
         return TRUE;

      case WM_NOTIFY :
         /* Change d'onglet */
         lpnmhdr = (LPNMHDR)lParam;
         if(lpnmhdr->code == TCN_SELCHANGE)
             ShowTab(hDlg);
         return FALSE;

      default:
         return FALSE;
   }
}


/* ------------------------------------------------------------------------- */


/* wgui_CreateTooltip:
 *  Cr�e une info-bulle.
 */
void wgui_CreateTooltip (HWND hWnd, WORD id, char *text)
{
    RECT rect;
    TOOLINFO ti;
    HWND hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
                            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            hWnd, NULL, prog_inst,
                            NULL);

    GetClientRect (hWnd, &rect);

    memset (&ti, 0x00, sizeof(TOOLINFO));
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = GetDlgItem(hWnd, id);
    ti.hinst = prog_inst;
    ti.lpszText = text;
    ti.rect.left = rect.left;
    ti.rect.top = rect.top;
    ti.rect.right = rect.right;
    ti.rect.bottom = rect.bottom;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
}



/* wgui_Error:
 *  Affiche une bo�te d'erreur
 */
void wgui_Error (HWND hwnd, const char *message)
{
    MessageBox(hwnd, (const char*)message,
               _("Teo - Error"),
               MB_OK | MB_ICONERROR);
}



/* wgui_Warning:
 *  Affiche une bo�te de pr�vention
 */
void wgui_Warning (HWND hwnd, const char *message)
{
    MessageBox(hwnd, (const char*)message,
               _("Teo - Warning"),
               MB_OK | MB_ICONWARNING);
}



/* wgui_Free:
 *  Lib�re la m�moire utilis�e par l'interface
 */
void wgui_Free (void)
{
    wmemo_Free ();
    wcass_Free ();
    wdisk_Free ();
}



/* wgui_Panel:
 *  Affiche le panneau de contr�le natif Windows.
 */
void wgui_Panel(void)
{
   int ret = DialogBox(prog_inst,
                        MAKEINTRESOURCE(IDC_CONTROL_DIALOG),
                        prog_win,
                        (DLGPROC)ControlDialogProc);

   if (ret == IDCANCEL)
   {
      if (teo.command == TEO_COMMAND_COLD_RESET)
          teo_ColdReset();
      teo.command = TEO_COMMAND_QUIT;
   }
}
