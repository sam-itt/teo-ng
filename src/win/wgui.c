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
 *  Copyright (C) 1997-2014 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : win/gui.c
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               21/09/2012 18/09/2013 11/04/2014
 *
 *  Interface utilisateur Windows native.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
#endif

#include "teo.h"
#include "alleg/gfxdrv.h"
#include "win/gui.h"

/* ressources globales de l'application */
#define NBTABS_MASTER 5
HINSTANCE prog_inst;
HWND prog_win;
HICON prog_icon;
HWND hTab[NBTABS_MASTER];

static int nCurrentTab = 0;



/* ShowTab:
 * Masque l'onglet actif et affiche l'onglet demandé
 */
static void
ShowTab(HWND hDlg)
{
    ShowWindow(hTab[nCurrentTab], SW_HIDE);
    nCurrentTab = TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_CONTROL_TAB));
    ShowWindow(hTab[nCurrentTab], SW_SHOW);
}



/* CreateTab:
 * Crée un onglet
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

    /* création du dialogue enfant */
    hMyTab = CreateDialog(prog_inst, MAKEINTRESOURCE(id), hDlg, prog);

    /* ajout de l'onglet */
    tcitem.pszText = (LPTSTR)title;
    TabCtrl_InsertItem(hTabItem, number, &tcitem);

    /* définit le rectangle en rapport à la boîte de dialogue parente */
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
 *  Procédure du panneau de contrôle.
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
#ifdef FRENCH_LANGUAGE
         SetWindowText(hDlg, "Panneau de contrôle");
         SetWindowText(GetDlgItem(hDlg, IDC_RESET_BUTTON), "Reset à chaud");
         SetWindowText(GetDlgItem(hDlg, IDC_COLDRESET_BUTTON), "Reset à froid");
         SetWindowText(GetDlgItem(hDlg, IDC_FULLRESET_BUTTON), "Reset total");
         SetWindowText(GetDlgItem(hDlg, IDC_ABOUT_BUTTON), "A propos");
         SetWindowText(GetDlgItem(hDlg, IDC_QUIT_BUTTON), "Quitter");
#else
         SetWindowText(hDlg, "Control panel");
         SetWindowText(GetDlgItem(hDlg, IDC_RESET_BUTTON), "Warm reset");
         SetWindowText(GetDlgItem(hDlg, IDC_COLDRESET_BUTTON), "Cold reset");
         SetWindowText(GetDlgItem(hDlg, IDC_FULLRESET_BUTTON), "Full reset");
         SetWindowText(GetDlgItem(hDlg, IDC_ABOUT_BUTTON), "About");
         SetWindowText(GetDlgItem(hDlg, IDC_QUIT_BUTTON), "Quit");
#endif
         /* Crée les onglets */
         hTab[0] = CreateTab(hDlg, 0, is_fr?"Réglage":"Setting",
                             IDC_SETTING_TAB, wsetting_TabProc);
         hTab[1] = CreateTab(hDlg, 1, is_fr?"Disquette":"Disk",
                             IDC_DISK_TAB, wdisk_TabProc);
         hTab[2] = CreateTab(hDlg, 2, is_fr?"Cassette":"Tape",
                             IDC_K7_TAB, wcass_TabProc);
         hTab[3] = CreateTab(hDlg, 3, is_fr?"Cartouche":"Cartridge",
                             IDC_MEMO7_TAB, wmemo_TabProc);
         hTab[4] = CreateTab(hDlg, 4, is_fr?"Imprimante":"Printer",
                             IDC_PRINTER_TAB, wprinter_TabProc);
         TabCtrl_SetCurSel(GetDlgItem(hDlg, IDC_CONTROL_TAB), nCurrentTab);
         ShowTab(hDlg);

         /* mise en place de l'icône */
         SetClassLong(hDlg, GCL_HICON,   (LONG) prog_icon);
         SetClassLong(hDlg, GCL_HICONSM, (LONG) prog_icon);

         /* crée les tooltips */
         wgui_CreateTooltip (hDlg, IDC_RESET_BUTTON,
								 is_fr?"Redémarre à chaud sans " \
                                       "effacer la mémoire RAM"
                                      :"Warm reset without to\n"
                                       "clear the RAM memory");
         wgui_CreateTooltip (hDlg, IDC_COLDRESET_BUTTON,
                                 is_fr?"Redémarre à froid sans " \
                                       "effacer la mémoire RAM"
                                      :"Cold reset without to\n" \
                                       "clear the RAM memory");
         wgui_CreateTooltip (hDlg, IDC_FULLRESET_BUTTON,
                                 is_fr?"Redémarre à froid et " \
                                       "efface la mémoire RAM"
                                      :"Cold reset and\n" \
                                       "clear the RAM memory");
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
                                is_fr?"Toute la mémoire RAM sera effacée."
                                     :"All the RAM memory will be cleared.",
                                is_fr?"Teo - Question":"Teo - Question",
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
 *  Crée une info-bulle.
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
 *  Affiche une boîte d'erreur
 */
void wgui_Error (HWND hwnd, const char *message)
{
    MessageBox(hwnd, (const char*)message,
               is_fr?"Teo - Erreur":"Teo - Error",
               MB_OK | MB_ICONERROR);
}



/* wgui_Warning:
 *  Affiche une boîte de prévention
 */
void wgui_Warning (HWND hwnd, const char *message)
{
    MessageBox(hwnd, (const char*)message,
               is_fr?"Teo - Attention":"Teo - Warning",
               MB_OK | MB_ICONWARNING);
}



/* wgui_Free:
 *  Libère la mémoire utilisée par l'interface
 */
void wgui_Free (void)
{
    wmemo_Free ();
    wcass_Free ();
    wdisk_Free ();
}



/* wgui_Panel:
 *  Affiche le panneau de contrôle natif Windows.
 */
void wgui_Panel(void)
{
   int ret = DialogBox(prog_inst,
                        MAKEINTRESOURCE(IDC_CONTROL_DIALOG),
                        prog_win,
                        (DLGPROC)ControlDialogProc);

   printf ("Control Panel\n");
   if (ret == IDCANCEL)
   {
      if (teo.command == TEO_COMMAND_COLD_RESET)
          teo_ColdReset();
      teo.command = TEO_COMMAND_QUIT;
   }
}
