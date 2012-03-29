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
 *  Module     : win/gui.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *
 *  Interface utilisateur Windows native.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <windows.h>
   #include <shellapi.h>
   #include <commctrl.h>
#endif

#include "alleg/gfxdrv.h"
#include "win/dialog.rh"
#include "win/gui.h"
#include "to8.h"



/* StartDialogProc:
 *  Procédure de la boîte de dialogue de démarrage.
 */
BOOL CALLBACK StartDialogProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
   int ret;

   switch(Message)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANG
         SetWindowText(hDlg, "Teo - l'émulateur TO8");
         SetWindowText(GetDlgItem(hDlg, START_LTEXT), 
                    "Teo (Windows/DirectX)\r\n"\
                    "Version "\
                    TO8_VERSION_STR\
                    "\r\n"\
                    "Copyright © 1997-2012  Gilles Fétis, Eric Botcazou,\r\n"\
                    "Alexandre Pukall, Jérémie Guillaume, François Mouret,\r\n"\
                    "Samuel Devulder\r\n\n"\
                    "Ce programme est un logiciel libre; vous pouvez le redistri-\r\nbuer "\
                    "et/ou le modifier selon les termes de la GNU General Public License; "\
                    "version 2 de la licence, ou (à votre conve-\r\nnance) toute version ultérieure.");
         SetWindowText(GetDlgItem(hDlg, GFXMODE_GROUP), "Mode graphique");
         SetWindowText(GetDlgItem(hDlg, MODE40_BUTTON), "Plein écran 40 colonnes 16 couleurs (rapide)");
         SetWindowText(GetDlgItem(hDlg, MODE80_BUTTON), "Plein écran 80 colonnes 16 couleurs (médium)");
         SetWindowText(GetDlgItem(hDlg, TRUECOLOR_BUTTON), "Plein écran 80 colonnes 4096 couleurs (lent)");
         SetWindowText(GetDlgItem(hDlg, WINDOWED_BUTTON), "Fenêtré 80 colonnes 4096 couleurs");
#else
         SetWindowText(hDlg, "Teo - the TO8 emulator");
         SetWindowText(GetDlgItem(hDlg, START_LTEXT), 
                   "Teo (Windows/DirectX)\r\n"\
                    "Version "\
                    TO8_VERSION_STR\
                    "\r\n"\
                    "Copyright © 1997-2012  Gilles Fétis, Eric Botcazou,\r\n"\
                    "Alexandre Pukall, Jérémie Guillaume, François Mouret,\r\n"\
                    "Samuel Devulder\r\n\n"\
                    "This program is free software; you can redistribute it and/or modify "\
                    "it under the terms of the GNU General Public License; either version "\
                    "2 of the License, or (at your option) any later version.");
         SetWindowText(GetDlgItem(hDlg, GFXMODE_GROUP), "Graphic mode");
         SetWindowText(GetDlgItem(hDlg, MODE40_BUTTON), "Full screen 40 columns 16 colors (fast)");
         SetWindowText(GetDlgItem(hDlg, MODE80_BUTTON), "Full screen 80 columns 16 colors (medium)");
         SetWindowText(GetDlgItem(hDlg, TRUECOLOR_BUTTON), "Full screen 80 columns 4096 colors (slow)");
         SetWindowText(GetDlgItem(hDlg, WINDOWED_BUTTON), "Windowed 80 columns 4096 colors");
#endif
         /* mise en place de l'icône */
         SetClassLong(hDlg, GCL_HICON,   (LONG) prog_icon);
         SetClassLong(hDlg, GCL_HICONSM, (LONG) prog_icon);

         CheckRadioButton(hDlg, MODE40_BUTTON, WINDOWED_BUTTON, WINDOWED_BUTTON);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDOK:
               if (IsDlgButtonChecked(hDlg, MODE40_BUTTON))
                  ret = GFX_MODE40;
               else if (IsDlgButtonChecked(hDlg, MODE80_BUTTON))
                  ret = GFX_MODE80;
               else if (IsDlgButtonChecked(hDlg, TRUECOLOR_BUTTON))
                  ret = GFX_TRUECOLOR;
               else
                  ret = GFX_WINDOW;

               EndDialog(hDlg, ret);
               break;

            case WM_DESTROY:
               EndDialog(hDlg, NO_GFX);
               exit(EXIT_SUCCESS);
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}



/* SelectGraphicMode:
 *  Affiche la boîte de dialogue de démarrage et permet 
 *  à l'utilisateur de choisir le mode graphique.
 */
void SelectGraphicMode(int *graphics_mode, int *ctrl_pressed)
{
   *graphics_mode = DialogBox(prog_inst, "START_DIALOG", NULL, StartDialogProc);

   if (ctrl_pressed)
      *ctrl_pressed = GetKeyState(VK_CONTROL)&0x8000;
}
