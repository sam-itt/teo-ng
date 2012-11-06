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
 *  Module     : win/wgui/wprinter.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 22/04/2012
 *  Modifié par: François Mouret 24/10/2012
 *
 *  Gestion des imprimantes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <windows.h>
   #include <shellapi.h>
   #include <commctrl.h>
   #include <shlobj.h>
#endif

#include "win/dialog.rh"
#include "win/gui.h"
#include "std.h"
#include "to8.h"

#define PRINTER_NUMBER 5

struct PRINTER_CODE_LIST {
    char name[9];
    int  number;
};

static struct PRINTER_CODE_LIST prt_list[PRINTER_NUMBER] = {
    { "PR90-042",  42 },
    { "PR90-055",  55 },
    { "PR90-582", 582 },
    { "PR90-600", 600 },
    { "PR90-612", 612 }
};



/*
 *  Callback pour la boîte de dialogue du répertoire.
 */
static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg,
                                             LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
        SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
    return 0;

    (void) lParam;
}



/* open_folder:
 *  Sélection d'un répertoire.
 */
static void open_folder (HWND hWnd)
{
    BROWSEINFO bi;
    ITEMIDLIST *pidl;
    static char folder[MAX_PATH] = "";

    bi.hwndOwner = hWnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = (LPTSTR)folder;
    bi.lpszTitle = is_fr ? "Choisir un répertoire":"Choose a directory";
    bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)folder;
    pidl = SHBrowseForFolder(&bi);
    if (SHGetPathFromIDList(pidl, (LPTSTR)folder) == TRUE)
    {
        SetWindowText(GetDlgItem(hWnd, PRINTER_MORE_EDIT), std_LastDir(folder));
        teo.lprt.folder = std_free (teo.lprt.folder);
        teo.lprt.folder = std_strdup_printf ("%s", folder);
    }
}


/* ------------------------------------------------------------------------- */


/* wprinter_TabProc:
 *  Procédure pour l'onglet des imprimantes
 */
int CALLBACK wprinter_TabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   int i;
   int state;
   int combo_index = 0;
   int first = 1;
   HANDLE himg;
   char str[MAX_PATH+1] = "";

   switch(uMsg)
   {
      case WM_INITDIALOG:
         if (first != 0)
         {
             if (teo.lprt.folder == NULL)
             {
                 (void)getcwd (str, MAX_PATH);
                 teo.lprt.folder = std_free (teo.lprt.folder);
                 teo.lprt.folder = std_strdup_printf ("%s", str);
             }
             first = 0;
         }

         /* initialisation du combo */
         SendDlgItemMessage(hWnd, PRINTER_CHOOSE_COMBO, CB_RESETCONTENT, 0, 0);
         for (i=0; i<PRINTER_NUMBER; i++)
         {
             SendDlgItemMessage(hWnd, PRINTER_CHOOSE_COMBO, CB_ADDSTRING, 0,
                                (LPARAM) prt_list[i].name);
             if (teo.lprt.number == prt_list[i].number)
                 combo_index = i;
         }
         SendDlgItemMessage(hWnd, PRINTER_CHOOSE_COMBO, CB_SETCURSEL,
                            combo_index, 0);

         /* initialisation des cases à cocher */
         state = (teo.lprt.dip == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, PRINTER_DIP_CHECK, state);
         state = (teo.lprt.nlq == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, PRINTER_NLQ_CHECK, state);
         state = (teo.lprt.raw_output == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, PRINTER_RAW_CHECK, state);
         state = (teo.lprt.txt_output == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, PRINTER_TXT_CHECK, state);
         state = (teo.lprt.gfx_output == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, PRINTER_GFX_CHECK, state);
         
         /* initialisation des images pour les boutons */
         himg = LoadImage (prog_inst, "open_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, PRINTER_MORE_BUTTON), BM_SETIMAGE,
                                (WPARAM)IMAGE_ICON, (LPARAM)himg );

         /* initialisation des textes */
         SetWindowText(GetDlgItem(hWnd, PRINTER_MORE_RTEXT),
                            is_fr?"Sauver dans : ":" Save in : ");
         SetWindowText(GetDlgItem(hWnd, PRINTER_CHOOSE_RTEXT),
                            is_fr?"Imprimante : ":"Printer : ");
         SetWindowText(GetDlgItem(hWnd, PRINTER_OUTPUT_RTEXT),
                            is_fr?"Sortie : ":"Output : ");
         SetWindowText(GetDlgItem(hWnd, PRINTER_RAW_CHECK),
                            is_fr?"brute":"raw");
         SetWindowText(GetDlgItem(hWnd, PRINTER_TXT_CHECK),
                            is_fr?"texte":"text");
         SetWindowText(GetDlgItem(hWnd, PRINTER_GFX_CHECK),
                            is_fr?"graphique":"graphic");
         SetWindowText(GetDlgItem(hWnd, PRINTER_MORE_EDIT),
                            std_LastDir(teo.lprt.folder));

         /* initialisation des info-bulles */
         wgui_CreateTooltip (hWnd, PRINTER_MORE_BUTTON,
                                   is_fr?"Choisir un répertoire de sauvegarde"
                                        :"Choose a save folder");
         wgui_CreateTooltip (hWnd, PRINTER_DIP_CHECK,
                                   is_fr?"Change le comportement de CR"
                                        :"Change behavior of CR");
         wgui_CreateTooltip (hWnd, PRINTER_NLQ_CHECK,
                                   is_fr?"Haute qualité"
                                        :"High-quality");

         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case PRINTER_MORE_BUTTON:
               open_folder (hWnd);
               break;

            case PRINTER_DIP_CHECK:
               state = IsDlgButtonChecked(hWnd, PRINTER_DIP_CHECK);
               teo.lprt.dip = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case PRINTER_NLQ_CHECK:
               state = IsDlgButtonChecked(hWnd, PRINTER_NLQ_CHECK);
               teo.lprt.nlq = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case PRINTER_RAW_CHECK:
               state = IsDlgButtonChecked(hWnd, PRINTER_RAW_CHECK);
               teo.lprt.raw_output = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case PRINTER_TXT_CHECK:
               state = IsDlgButtonChecked(hWnd, PRINTER_TXT_CHECK);
               teo.lprt.txt_output = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case PRINTER_GFX_CHECK:
               state = IsDlgButtonChecked(hWnd, PRINTER_GFX_CHECK);
               teo.lprt.gfx_output = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case PRINTER_CHOOSE_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
               {
                   combo_index = SendDlgItemMessage(hWnd, PRINTER_CHOOSE_COMBO,
                                                    CB_GETCURSEL, 0, 0);
                   teo.lprt.number = prt_list[combo_index].number;
               }
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}
