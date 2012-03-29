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



static void init_file_cass(struct FILE_VECTOR *vector, HWND hDlg)
{
   static int first = 1;
   const char *str_p;
   int i;

   if (first)
   {
      str_p = to8_GetK7Filename();
      if (*str_p)
         WGUI_push_back(vector, str_p, WGUI_get_filename(str_p));

      first = 0;
   }

   if (vector->size)
   {
      for (i=0; i<vector->size; i++)
         SendDlgItemMessage(hDlg, K7_COMBO, CB_ADDSTRING, 0, (LPARAM) vector->file[i].label);
   }
   else
      SendDlgItemMessage(hDlg, K7_COMBO, CB_ADDSTRING, 0, (LPARAM) (is_fr?"aucune cassette":"no tape"));

   SendDlgItemMessage(hDlg, K7_COMBO, CB_SETCURSEL, vector->selected, 0);
}


static void open_file_cass(struct FILE_VECTOR *vector, HWND hDlg)
{
   OPENFILENAME openfilename;
   char buffer[BUFFER_SIZE], dir[BUFFER_SIZE];
   int ret, index;

   /* initialisation du sélecteur de fichiers */
   memset(&openfilename, 0, sizeof(OPENFILENAME));

   openfilename.lStructSize = sizeof(OPENFILENAME);
   openfilename.hwndOwner = hDlg;
   openfilename.lpstrFilter = is_fr?"Fichiers K7\0*.k7\0":"K7 files\0*.k7\0";
   openfilename.nFilterIndex = 1;
   openfilename.lpstrFile = buffer;
   openfilename.nMaxFile = BUFFER_SIZE;

   if (vector->size)
   {
      WGUI_extract_dir(dir, vector->file[vector->selected].fullname);
      openfilename.lpstrInitialDir = dir;
   }
   else
      openfilename.lpstrInitialDir = ".\\k7";

   openfilename.lpstrTitle = is_fr?"Choisissez votre cassette:":"Choose your tape";
   openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
   openfilename.lpstrDefExt ="k7";

   buffer[0] = '\0';

   if (GetOpenFileName(&openfilename))
   {
      ret = to8_LoadK7(openfilename.lpstrFile);

      if (ret == TO8_ERROR)
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
      else
      {        
         index = WGUI_vector_index(vector, openfilename.lpstrFile);

         if (index == NOT_FOUND)
         {
            index = WGUI_push_back(vector, openfilename.lpstrFile, openfilename.lpstrFile + openfilename.nFileOffset);

            if (!index)
               SendDlgItemMessage(hDlg, K7_COMBO, CB_DELETESTRING, 0, 0);
                             
            SendDlgItemMessage(hDlg, K7_COMBO, CB_ADDSTRING, 0, (LPARAM) vector->file[index].label);
         }

         SendDlgItemMessage(hDlg, K7_COMBO, CB_SETCURSEL, index, 0);
         vector->selected = index;

         if ((ret == TO8_READ_ONLY) && !vector->protection)
         {
            MessageBox(hDlg, is_fr?"Attention: écriture impossible.":"Warning: writing unavailable.", PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
            CheckDlgButton(hDlg, K7_PROT_CHECK, BST_CHECKED); 
            vector->protection = TRUE;
         }
      }  
   }
}


static void select_file_cass(struct FILE_VECTOR *vector, HWND hDlg)
{
   int ret, index;

   if (vector->size)
   {
      index = SendDlgItemMessage(hDlg, K7_COMBO, CB_GETCURSEL, 0, 0);
      ret = to8_LoadK7(vector->file[index].fullname);

      if (ret == TO8_ERROR)
      {
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
         SendDlgItemMessage(hDlg, K7_COMBO, CB_SETCURSEL, vector->selected, 0);
      }
      else
      {
         vector->selected = index;
           
         if ((ret == TO8_READ_ONLY) && !vector->protection)
         {
            MessageBox(hDlg, is_fr?"Attention: écriture impossible.":"Warning: writing unavailable.", PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
            CheckDlgButton(hDlg, K7_PROT_CHECK, BST_CHECKED); 
            vector->protection = TRUE;
         }
      }
   }
}


static void init_check_cass(struct FILE_VECTOR *vector, HWND hDlg)
{
   if (!vector->size)
      vector->protection = TRUE;

   CheckDlgButton(hDlg, K7_PROT_CHECK, vector->protection ? BST_CHECKED : BST_UNCHECKED);
}


static void change_protection_cass(struct FILE_VECTOR *vector, HWND hDlg)
{
   if (IsDlgButtonChecked(hDlg, K7_PROT_CHECK))
   {
      to8_SetK7Mode(TO8_READ_ONLY);
      vector->protection = TRUE;
   }
   else
   {
      if (to8_SetK7Mode(TO8_READ_WRITE) == TO8_READ_ONLY)
      {
         MessageBox(hDlg, is_fr?"Ecriture impossible sur ce support.":"Warning: writing unavailable on this device.", PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
         CheckDlgButton(hDlg, K7_PROT_CHECK, BST_CHECKED); 
         vector->protection = TRUE;
      }
      else
         vector->protection = FALSE;
   }
}

#define COUNTER_MAX  999

static void update_counter_cass(HWND hDlg)
{
   int counter = to8_GetK7Counter();
   SetDlgItemInt(hDlg, K7_COUNTER_EDIT, counter, FALSE);
   SendDlgItemMessage(hDlg, K7_UPDOWN, UDM_SETPOS, 0, MAKELONG(counter, 0));
}


static void eject_file_cass(struct FILE_VECTOR *vector, HWND hDlg)
{
    SendDlgItemMessage(hDlg, K7_COMBO, CB_RESETCONTENT, 0, 0);
    WGUI_reset_vector (vector);
    vector->protection = TRUE;
    init_file_cass(vector, hDlg);
    init_check_cass(vector, hDlg);
    to8_EjectK7();
    update_counter_cass(hDlg);
}


/* CassetteTabProc:
 * Procédure pour l'onglet des cassettes
 */
int CALLBACK CassetteTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static struct FILE_VECTOR k7;
   static HWND counter_updown;
   LPNM_UPDOWN nmupdown;
   int counter;
   
   switch(uMsg)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANG
         SetWindowText(GetDlgItem(hDlg, K7_COUNTER_LTEXT), "Compteur:");
         SetWindowText(GetDlgItem(hDlg, K7_REWIND_BUTTON), "Rembobiner");
#else
         SetWindowText(GetDlgItem(hDlg, K7_COUNTER_LTEXT), " Counter:");
         SetWindowText(GetDlgItem(hDlg, K7_REWIND_BUTTON), "Rewind");
#endif
         /* initialisation du sélecteur de cassettes */
         init_file_cass(&k7, hDlg);
         init_check_cass(&k7, hDlg);

         /* initialisation du compteur de cassettes */
         counter_updown = GetDlgItem(hDlg, K7_UPDOWN);
         SendMessage(counter_updown, UDM_SETRANGE, 0, MAKELONG(COUNTER_MAX, 0));
         update_counter_cass(hDlg);

         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case K7_EJECT_BUTTON:
               eject_file_cass(&k7, hDlg);
               break; 

            case K7_MORE_BUTTON:
               open_file_cass(&k7, hDlg);
               break; 

            case K7_COMBO:
               select_file_cass(&k7, hDlg);
               update_counter_cass(hDlg);
               break;

            case K7_PROT_CHECK:
               change_protection_cass(&k7, hDlg);
               update_counter_cass(hDlg);
               break;

            case K7_COUNTER_EDIT:
               if (HIWORD(wParam) == EN_CHANGE)
               {
                  counter = GetDlgItemInt(hDlg, K7_COUNTER_EDIT, NULL, FALSE);
                  to8_SetK7Counter(counter);
                  SendMessage(counter_updown, UDM_SETPOS, 0, MAKELONG(counter, 0));
               }
               break;

            case K7_REWIND_BUTTON:
               to8_SetK7Counter(0);
               update_counter_cass(hDlg);
               break;
         }
         return TRUE;

      case WM_NOTIFY:
         nmupdown = (LPNM_UPDOWN)lParam;
         if ((HWND)nmupdown->hdr.hwndFrom == counter_updown)
         {
            counter = nmupdown->iPos;
            to8_SetK7Counter(counter);
            SetDlgItemInt(hDlg, K7_COUNTER_EDIT, counter, FALSE);
         }
         return TRUE;

      default:
         return FALSE;
   }
}
