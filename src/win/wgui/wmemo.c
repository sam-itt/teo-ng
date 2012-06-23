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
 *  Module     : win/wgui/wmemo.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *
 *  Gestion des cartouches.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <string.h>
   #include <windows.h>
   #include <shellapi.h>
   #include <commctrl.h>
#endif

#include "win/dialog.rh"
#include "win/gui.h"
#include "intern/gui.h"
#include "to8.h"

static int entry_max = 0;
static int combo_index = 0;
static struct STRING_LIST *path_list = NULL;
static struct STRING_LIST *name_list = NULL;



/* update_params:
 *  Sauve les paramètres d'une cartouche.
 */
static void update_params (HWND hWnd)
{
    combo_index = SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_GETCURSEL, 0, 0);
}



/* load_memo:
 *  Charge une cartouche.
 */
static int load_memo (HWND hWnd, char *filename)
{
    int ret = to8_LoadMemo7(filename);

    if (ret == TO8_ERROR)
        MessageBox(hWnd, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONINFORMATION);

    return ret;
}



/* add_combo_entry:
 *  Ajoute une entrée dans le combobox si inexistante
 *  et sélectionne l'entrée demandée.
 */
static void add_combo_entry (HWND hWnd, const char *name, const char *path)
{
    int path_index = gui_StringListIndex (path_list, (char *)path);
    int name_index = gui_StringListIndex (name_list, (char *)name);

    if ((path_index < 0) || (name_index != path_index))
    {
        path_list = gui_StringListAppend (path_list, (char *)path);
        name_list = gui_StringListAppend (name_list, (char *)name);
        SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) name);
        SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_SETCURSEL, entry_max, 0);
        entry_max++;
    }
    else
    {
        SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_SETCURSEL, path_index, 0);
        if (path_index != combo_index)
        {
            (void)load_memo (hWnd, gui_StringListText (path_list, path_index));
        }
    }
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (HWND hWnd)
{
    add_combo_entry (hWnd, is_fr?"(Aucun)":"(None)", "");
}



/* clear_combo:
 *  Vide un combo.
 */
static void clear_combo (HWND hWnd)
{
    free_memo_list ();
    to8_EjectMemo7();
    SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_RESETCONTENT, 0, 0);
    init_combo (hWnd);
    teo.command = COLD_RESET;
    update_params(hWnd);
}



/* combo_changed:
 *  Changement de sélection du combobox.
 */
static void combo_changed (HWND hWnd)
{
    int index = SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_GETCURSEL, 0, 0);

    if (index != combo_index)
    {
        if (index == 0)
        {
            to8_EjectMemo7();
        }
        else
        {
            (void)load_memo (hWnd, gui_StringListText (path_list, index));
        }
        teo.command = COLD_RESET;
        update_params(hWnd);
    }
}



/* open_file:
 *  Chargement d'un fichier cartouche.
 */
static void open_file (HWND hWnd)
{
   char current_file[BUFFER_SIZE]="";
   char def_folder[] = ".\\disks";
   OPENFILENAME ofn;

   memset(&ofn, 0, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hWnd;
   ofn.lpstrFilter = is_fr?"Fichiers M7\0*.m7\0":"M7 files\0*.m7\0";
   ofn.nFilterIndex = 1;
   ofn.lpstrFile = current_file;
   ofn.nMaxFile = BUFFER_SIZE;
   ofn.lpstrTitle = is_fr?"Choisissez votre cartouche:":"Choose your cartridge:";
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
   ofn.lpstrDefExt ="m7";

    if (strlen (gui->memo.file) != 0)
        ofn.lpstrInitialDir = gui->memo.file;
    else
    if (strlen (gui->default_folder) != 0)
        ofn.lpstrInitialDir = gui->default_folder;
    else
    if (access(def_folder, F_OK) == 0)
        ofn.lpstrInitialDir = def_folder;

   if (GetOpenFileName(&ofn))
   {
      if (load_memo (hWnd, ofn.lpstrFile) != TO8_ERROR)
      {
          add_combo_entry (hWnd, gui->memo.label, gui->memo.file);
          (void)snprintf (gui->default_folder, MAX_PATH, "%s", gui->memo.file);
          (void)snprintf (current_file, MAX_PATH, "%s", gui->memo.file);
          teo.command = COLD_RESET;
          update_params(hWnd);
      }
   }
}


/* --------------------------- Partie publique ----------------------------- */


/* free_memo_list
 *  Libère la mémoire utilisée par la liste des cartouches.
 */
void free_memo_list (void)
{
    gui_StringListFree (name_list);
    name_list=NULL;
    gui_StringListFree (path_list);
    path_list=NULL;
    entry_max=0;
    combo_index=0;
}



/* CartridgeTabProc:
 * Procédure pour l'onglet des cartouches
 */
int CALLBACK CartridgeTabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static int first=1;
   HANDLE himg;
   struct STRING_LIST *slist;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         /* initialisation du sélecteur de cartouches */
         if (first)
         {
             init_combo(hWnd);
             if (strlen(gui->memo.file) != 0)
                 add_combo_entry (hWnd, gui->memo.label, gui->memo.file);
             update_params (hWnd);
             first=0;
         }
         /* initialisation du combo */
         SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_RESETCONTENT, 0, 0);
         for (slist=name_list; slist!=NULL; slist=slist->next)
             SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) gui_BaseName(slist->str));
         SendDlgItemMessage(hWnd, MEMO7_COMBO, CB_SETCURSEL, combo_index, 0);

         /* initialisation des images pour les boutons */
         himg = LoadImage (prog_inst, "empty_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, MEMO7_EJECT_BUTTON), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg );
         himg = LoadImage (prog_inst, "open_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, MEMO7_MORE_BUTTON), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg );

         /* initialisation de l'info-bulle */
         create_tooltip (hWnd, MEMO7_EJECT_BUTTON, is_fr?"Vider la listedes fichiers":"Empty the file list");
         create_tooltip (hWnd, MEMO7_MORE_BUTTON, is_fr?"Ouvrir un fichier cartouche":"Open a cartridge file");

         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case MEMO7_EJECT_BUTTON:
               clear_combo(hWnd);
               break;

            case MEMO7_MORE_BUTTON:
               open_file(hWnd);
               break;

            case MEMO7_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
               {
                   combo_changed(hWnd);
               }
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}
