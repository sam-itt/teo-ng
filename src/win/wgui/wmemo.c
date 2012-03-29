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

#include "win/dialog.rh"
#include "win/gui.h"
#include "to8.h"
#include "alleg/main.h"



static void init_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
   static int first = 1;
   const char *str_p;
   int i;

   if (first)
   {
      str_p = to8_GetMemo7Filename();
      if (*str_p)
         WGUI_push_back(vector, str_p, to8_GetMemo7Label());

      first = 0;
   }

   if (vector->size)
   {
      for (i=0; i<vector->size; i++)
         SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) vector->file[i].label);
   }
   else
      SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) (is_fr?"aucune cartouche":"no cartridge"));

   SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_SETCURSEL, vector->selected, 0);
}



static void open_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
   OPENFILENAME openfilename;
   char buffer[BUFFER_SIZE], dir[BUFFER_SIZE];
   int index;

   /* initialisation du sélecteur de fichiers */
   memset(&openfilename, 0, sizeof(OPENFILENAME));

   openfilename.lStructSize = sizeof(OPENFILENAME);
   openfilename.hwndOwner = hDlg;
   openfilename.lpstrFilter = is_fr?"Fichiers M7\0*.m7\0":"M7 files\0*.m7\0";
   openfilename.nFilterIndex = 1;
   openfilename.lpstrFile = buffer;
   openfilename.nMaxFile = BUFFER_SIZE;

   if (vector->size)
   {
      WGUI_extract_dir(dir, vector->file[vector->selected].fullname);
      openfilename.lpstrInitialDir = dir;
   }
   else
      openfilename.lpstrInitialDir = ".\\memo7";

   openfilename.lpstrTitle = is_fr?"Choisissez votre cartouche:":"Choose your cartridge:";
   openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
   openfilename.lpstrDefExt ="m7";

   buffer[0] = '\0';

   if (GetOpenFileName(&openfilename))
   {
      if (to8_LoadMemo7(openfilename.lpstrFile))
      {        
         index = WGUI_vector_index(vector, openfilename.lpstrFile);
                         
         if (index == NOT_FOUND)
         {
            index = WGUI_push_back(vector, openfilename.lpstrFile, to8_GetMemo7Label());

            if (!index)
               SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_DELETESTRING, 0, 0);
                             
            SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) vector->file[index].label);
         }

         SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_SETCURSEL, index, 0);
         vector->selected = index;
         teo.command = COLD_RESET;
      }
      else
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
   }
}



static void select_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
   int index;

   if (vector->size)
   {
      index = SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_GETCURSEL, 0, 0);

      if (to8_LoadMemo7(vector->file[index].fullname))
      {
         vector->selected= index;
         teo.command = COLD_RESET;
      }
      else
      {
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
         SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_SETCURSEL, vector->selected, 0);
      }
   }
}



static void eject_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
    SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_RESETCONTENT, 0, 0);
    WGUI_reset_vector (vector);
    init_file_memo(vector, hDlg);
    to8_EjectMemo7();
    teo.command = COLD_RESET;
}         



/* CartridgeTabProc:
 * Procédure pour l'onglet des cartouches
 */
int CALLBACK CartridgeTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static struct FILE_VECTOR memo7;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         /* initialisation du sélecteur de cartouches */
         init_file_memo(&memo7, hDlg);

         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case MEMO7_EJECT_BUTTON:
               eject_file_memo(&memo7, hDlg);
               break;

            case MEMO7_MORE_BUTTON:
               open_file_memo(&memo7, hDlg);
               break;

            case MEMO7_COMBO:
               select_file_memo(&memo7, hDlg);
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}
