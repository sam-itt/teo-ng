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
 *  Copyright (C) 1997-2016 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 22/04/2012
 *  Modifié par: François Mouret 24/10/2012 20/09/2013 10/05/2014
 *
 *  Gestion des imprimantes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
#endif

#include "std.h"
#include "teo.h"
#include "media/printer.h"
#include "win/gui.h"


static void update_options (HWND hWnd, int number)
{
    if (number >= PRINTER_NUMBER)
        number = 0;

    teo.lprt.number = printer_code_list[number].number;

    if (teo.lprt.number < 600)
    {
        Button_Enable(GetDlgItem (hWnd, IDC_PRINTER_DIP_CHECK), FALSE);
        Button_Enable(GetDlgItem (hWnd, IDC_PRINTER_NLQ_CHECK), FALSE);
    }
    else
    {
        Button_Enable(GetDlgItem (hWnd, IDC_PRINTER_DIP_CHECK), TRUE);
        Button_Enable(GetDlgItem (hWnd, IDC_PRINTER_NLQ_CHECK), TRUE);
    }
}



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
        SetWindowText(GetDlgItem(hWnd, IDC_PRINTER_MORE_EDIT),
                      std_LastDir(folder));
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
   HWND hw;

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
         SendDlgItemMessage(hWnd,
                            IDC_PRINTER_CHOOSE_COMBO,
                            CB_RESETCONTENT,
                            0,
                            0);
         for (i=0; i<PRINTER_NUMBER; i++)
         {
             SendDlgItemMessage(hWnd,
                                IDC_PRINTER_CHOOSE_COMBO,
                                CB_ADDSTRING,
                                0,
                                (LPARAM) printer_code_list[i].name);
             if (teo.lprt.number == printer_code_list[i].number)
                 combo_index = i;
         }
         SendDlgItemMessage(hWnd,
                            IDC_PRINTER_CHOOSE_COMBO,
                            CB_SETCURSEL,
                            combo_index,
                            0);

         /* initialisation des cases à cocher */
         state = (teo.lprt.dip == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, IDC_PRINTER_DIP_CHECK, state);
         state = (teo.lprt.nlq == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, IDC_PRINTER_NLQ_CHECK, state);
         state = (teo.lprt.raw_output == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, IDC_PRINTER_RAW_CHECK, state);
         state = (teo.lprt.txt_output == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, IDC_PRINTER_TXT_CHECK, state);
         state = (teo.lprt.gfx_output == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, IDC_PRINTER_GFX_CHECK, state);
         
         /* initialisation des images pour les boutons */
         himg = LoadImage (prog_inst,
                           "folder_ico",
                           IMAGE_ICON,
                           0,
                           0,
                           LR_DEFAULTCOLOR);
         hw = GetDlgItem(hWnd, IDC_PRINTER_MORE_BUTTON);
         SendMessage(hw, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg );

         /* initialisation des textes */
         hw = GetDlgItem(hWnd, IDC_PRINTER_OPTIONS_GROUP);
         SetWindowText(hw, is_fr?"Imprimante":"Printer");
         hw = GetDlgItem(hWnd, IDC_PRINTER_CHOOSE_RTEXT);
         SetWindowText(hw, "Type ");
         hw = GetDlgItem(hWnd, IDC_PRINTER_DIP_CHECK);
         SetWindowText(hw, is_fr?"Double interligne":"Double spacing");
         hw = GetDlgItem(hWnd, IDC_PRINTER_NLQ_CHECK);
         SetWindowText(hw, is_fr?"Imprime en haute qualité"
                                :"High quality print");
         hw = GetDlgItem(hWnd, IDC_PRINTER_OUTPUT_GROUP);
         SetWindowText(hw, is_fr?"Sortie":"Output");
         hw = GetDlgItem(hWnd, IDC_PRINTER_RAW_CHECK);
         SetWindowText(hw, is_fr?"brute":"raw");
         hw = GetDlgItem(hWnd, IDC_PRINTER_TXT_CHECK);
         SetWindowText(hw, is_fr?"texte":"text");
         hw = GetDlgItem(hWnd, IDC_PRINTER_GFX_CHECK);
         SetWindowText(hw, is_fr?"graphique":"graphic");
         hw = GetDlgItem(hWnd, IDC_PRINTER_MORE_EDIT);
         SetWindowText(hw, std_LastDir(teo.lprt.folder));
         hw = GetDlgItem(hWnd, IDC_PRINTER_MORE_RTEXT);
         SetWindowText(hw, is_fr?"Sauver les fichiers dans "
                                :"Saving output files in ");

         /* initialisation des info-bulles */
         wgui_CreateTooltip (hWnd,
                             IDC_PRINTER_MORE_BUTTON,
                             is_fr?"Choisir un répertoire de sauvegarde"
                                  :"Choose a save folder");
         update_options (hWnd, combo_index);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDC_PRINTER_MORE_BUTTON:
               open_folder (hWnd);
               break;

            case IDC_PRINTER_DIP_CHECK:
               state = IsDlgButtonChecked(hWnd, IDC_PRINTER_DIP_CHECK);
               teo.lprt.dip = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case IDC_PRINTER_NLQ_CHECK:
               state = IsDlgButtonChecked(hWnd, IDC_PRINTER_NLQ_CHECK);
               teo.lprt.nlq = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case IDC_PRINTER_RAW_CHECK:
               state = IsDlgButtonChecked(hWnd, IDC_PRINTER_RAW_CHECK);
               teo.lprt.raw_output = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case IDC_PRINTER_TXT_CHECK:
               state = IsDlgButtonChecked(hWnd, IDC_PRINTER_TXT_CHECK);
               teo.lprt.txt_output = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case IDC_PRINTER_GFX_CHECK:
               state = IsDlgButtonChecked(hWnd, IDC_PRINTER_GFX_CHECK);
               teo.lprt.gfx_output = (state == BST_CHECKED) ? TRUE : FALSE;
               break;

            case IDC_PRINTER_CHOOSE_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
               {
                   combo_index = SendDlgItemMessage(hWnd,
                                                    IDC_PRINTER_CHOOSE_COMBO,
                                                    CB_GETCURSEL,
                                                    0,
                                                    0);
                   update_options (hWnd, combo_index);
               }
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}
