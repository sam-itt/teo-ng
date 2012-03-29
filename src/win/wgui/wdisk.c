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



static void init_file_disk(struct FILE_VECTOR *vector, HWND hDlg)
{
   static int first = 4;
   const char *str_p;
   int i;

   if (first)
   {
      str_p = to8_GetDiskFilename(vector->id);
      if (*str_p)
         vector->selected = WGUI_push_back(vector, str_p, WGUI_get_filename(str_p));

      first--;
   }

   if (vector->size)
   {
      for (i=0; i<vector->size; i++)
         SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_ADDSTRING, 0, (LPARAM) vector->file[i].label);
   }
   else
      SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_ADDSTRING, 0, (LPARAM) (is_fr?"aucune disquette":"no disk"));

   SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_SETCURSEL, vector->selected, 0);
}



static void open_file_disk(struct FILE_VECTOR *vector, HWND hDlg)
{
   OPENFILENAME openfilename;
   char buffer[BUFFER_SIZE], dir[BUFFER_SIZE];
   int ret, index;

   /* initialisation du sélecteur de fichiers */
   memset(&openfilename, 0, sizeof(OPENFILENAME));

   openfilename.lStructSize = sizeof(OPENFILENAME);
   openfilename.hwndOwner = hDlg;
   openfilename.lpstrFilter = is_fr?"Fichiers SAP\0*.sap\0":"SAP files\0*.sap\0";
   openfilename.nFilterIndex = 1;
   openfilename.lpstrFile = buffer;
   openfilename.nMaxFile = BUFFER_SIZE;

   if (vector->size)
   {
      WGUI_extract_dir(dir, vector->file[vector->selected].fullname);
      openfilename.lpstrInitialDir = dir;
   }
   else
      openfilename.lpstrInitialDir = ".\\disks";

   openfilename.lpstrTitle = is_fr?"Choisissez votre disquette:":"Choose your disk:";
   openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
   openfilename.lpstrDefExt ="sap";

   buffer[0] = '\0';

   if (GetOpenFileName(&openfilename))
   {
      ret = to8_LoadDisk(vector->id, openfilename.lpstrFile);

      if (ret == TO8_ERROR)
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
      else
      {
         index = WGUI_vector_index(vector, openfilename.lpstrFile);

         if (index == NOT_FOUND)
         {
            index = WGUI_push_back(vector, openfilename.lpstrFile, openfilename.lpstrFile + openfilename.nFileOffset);

            if (!index)
               SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_DELETESTRING, 0, 0);

            SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_ADDSTRING, 0, (LPARAM) vector->file[index].label);
         }

         SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_SETCURSEL, index, 0);
         vector->selected = index;

         if ((ret == TO8_READ_ONLY) && !vector->protection)
         {
            MessageBox(hDlg, is_fr?"Attention: écriture impossible.":"Warning: writing unavailable.", PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
            CheckDlgButton(hDlg, DISK0_PROT_CHECK+vector->id, BST_CHECKED); 
            vector->protection = TRUE;
         }
      }
   }
}



static void select_file_disk(struct FILE_VECTOR *vector, HWND hDlg)
{
   int ret, index;

   if (vector->size)
   {
      index = SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);
      ret = to8_LoadDisk(vector->id, vector->file[index].fullname);

      if (ret == TO8_ERROR)
      {
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
         SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_SETCURSEL, vector->selected, 0);
      }
      else
      {
         vector->selected = index;
           
         if ((ret == TO8_READ_ONLY) && !vector->protection)
         {
            MessageBox(hDlg, is_fr?"Attention: écriture impossible.":"Warning: writing unavailable.", PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
            CheckDlgButton(hDlg, DISK0_PROT_CHECK+vector->id, BST_CHECKED); 
            vector->protection = TRUE;
         }
      }
   }
}



static void init_check_disk(struct FILE_VECTOR *vector, HWND hDlg)
{
   if (!vector->size)
      vector->protection = FALSE;

   CheckDlgButton(hDlg, DISK0_PROT_CHECK+vector->id, vector->protection ? BST_CHECKED : BST_UNCHECKED);
}


static void change_protection_disk(struct FILE_VECTOR *vector, HWND hDlg)
{
   if (IsDlgButtonChecked(hDlg, DISK0_PROT_CHECK+vector->id))
   {
      to8_SetDiskMode(vector->id, TO8_READ_ONLY);
      vector->protection = TRUE;
   }
   else
   {
      if (to8_SetDiskMode(vector->id, TO8_READ_WRITE) == TO8_READ_ONLY)
      {
         MessageBox(hDlg, is_fr?"Ecriture impossible sur ce support.":"Writing unavailable on this device.", PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
         CheckDlgButton(hDlg, DISK0_PROT_CHECK+vector->id, BST_CHECKED); 
         vector->protection = TRUE;
      }
      else
         vector->protection = FALSE;
   }
}



static void eject_file_disk(struct FILE_VECTOR *vector, HWND hDlg)
{
    SendDlgItemMessage(hDlg, DISK0_COMBO+vector->id, CB_RESETCONTENT, 0, 0);
    WGUI_reset_vector (vector);
    init_file_disk(vector, hDlg);
    to8_EjectDisk(vector->id);
}



/* DiskTabProc:
 * Procédure pour l'onglet des disquettes
 */
int CALLBACK DiskTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   int i;
   static struct FILE_VECTOR disk[4] = { {0}, {1}, {2}, {3} };

   switch(uMsg)
   {
      case WM_INITDIALOG:
         /* initialisation des quatre sélecteurs de disquettes */
         for (i=0; i<4; i++)
         {
             init_file_disk(&disk[i], hDlg);
             init_check_disk(&disk[i], hDlg);
         }
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case DISK0_EJECT_BUTTON:
            case DISK1_EJECT_BUTTON:
            case DISK2_EJECT_BUTTON:
            case DISK3_EJECT_BUTTON: 
               eject_file_disk(&disk[LOWORD(wParam) - DISK0_EJECT_BUTTON], hDlg);
               break;

            case DISK0_MORE_BUTTON:
            case DISK1_MORE_BUTTON:
            case DISK2_MORE_BUTTON:
            case DISK3_MORE_BUTTON: 
               open_file_disk(&disk[LOWORD(wParam) - DISK0_MORE_BUTTON], hDlg);
               break;

            case DISK0_COMBO:
            case DISK1_COMBO:
            case DISK2_COMBO:
            case DISK3_COMBO: 
               select_file_disk(&disk[LOWORD(wParam) - DISK0_COMBO], hDlg);
               break;

            case DISK0_PROT_CHECK:
            case DISK1_PROT_CHECK:
            case DISK2_PROT_CHECK:
            case DISK3_PROT_CHECK: 
               change_protection_disk(&disk[LOWORD(wParam) - DISK0_PROT_CHECK], hDlg);
               break;
         }
         return TRUE;
      default:
         return FALSE;
   }
}
