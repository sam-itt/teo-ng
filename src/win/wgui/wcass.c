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
 *  Gestion des cassettes.
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

#define COUNTER_MAX  999

static int entry_max = 0;
static int combo_index = 0;
static UINT prot = BST_CHECKED;
static struct STRING_LIST *path_list = NULL;
static char current_dir[BUFFER_SIZE] = "";



/* update_params:
 *  Sauve les paramètres d'une cassette.
 */
static void update_params (HWND hWnd)
{
    prot = IsDlgButtonChecked(hWnd, K7_PROT_CHECK);
    combo_index = SendDlgItemMessage(hWnd, K7_COMBO, CB_GETCURSEL, 0, 0);
}



/* set_counter_cass:
 *  Positionne le compteur de cassette
 */
static void set_counter_cass (HWND hWnd)
{
   int counter = to8_GetK7Counter();

   SetDlgItemInt(hWnd, K7_COUNTER_EDIT, counter, FALSE);
   SendDlgItemMessage(hWnd, K7_UPDOWN, UDM_SETPOS, 0, MAKELONG(counter, 0));
}



/* rewind_cass:
 *  Met le compteur de cassette à 0
 */
static void rewind_cass (HWND hWnd)
{
   to8_SetK7Counter(0);
   set_counter_cass (hWnd);
}



/* eject_cass:
 *  Ejecte la cassette
 */
static void eject_cass (HWND hWnd)
{
    rewind_cass (hWnd);
    to8_EjectK7();
}



/* load_cass:
 *  Charge une cassette.
 */
static int load_cass (HWND hWnd, char *filename)
{
    int ret = to8_LoadK7 (filename);

    switch (ret)
    {
        case TO8_ERROR :
            MessageBox(hWnd, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
            break;

        case TO8_READ_ONLY :
            CheckDlgButton(hWnd, K7_PROT_CHECK, BST_CHECKED);
            break;

        default : break;
    }
    rewind_cass(hWnd);
    return ret;
}



/* toggle_check_cass:
 *  Gestion de la protection.
 */
static void toggle_check_cass (HWND hWnd)
{
    if (IsDlgButtonChecked(hWnd, K7_PROT_CHECK) == BST_CHECKED)
    {
        to8_SetK7Mode(TO8_READ_ONLY);
    }
    else
    if (to8_SetK7Mode(TO8_READ_WRITE)==TO8_READ_ONLY)
    {
        MessageBox(hWnd, is_fr?"Ecriture impossible sur ce support."
                              :"Warning: writing unavailable on this device."
                       , PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
        CheckDlgButton(hWnd, K7_PROT_CHECK, BST_CHECKED); 
    }
    update_params(hWnd);
    set_counter_cass (hWnd);
}



/* add_combo_entry
 *  Ajoute une entrée dans le combobox si inexistante.
 */
static void add_combo_entry (HWND hWnd, const char *path)
{
    int index = stringlist_index (path_list, (char *)path);

    if (index<0)
    {
        path_list = stringlist_append (path_list, (char *)path);
        SendDlgItemMessage(hWnd, K7_COMBO, CB_ADDSTRING, entry_max, (LPARAM) basename_ptr(path));
        SendDlgItemMessage(hWnd, K7_COMBO, CB_SETCURSEL, entry_max, 0);
        entry_max++;
    }
    else
    {
        SendDlgItemMessage(hWnd, K7_COMBO, CB_SETCURSEL, index, 0);
        if (index != combo_index)
        {
            (void)load_cass (hWnd, stringlist_text (path_list, index));
        }
    }

}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (HWND hWnd)
{
    add_combo_entry (hWnd, is_fr?"(aucun)":"(none)");
}



/* clear_combo:
 *  Vide un combo.
 */
static void clear_combo (HWND hWnd)
{
    free_cass_list ();
    eject_cass (hWnd);
    SendDlgItemMessage(hWnd, K7_COMBO, CB_RESETCONTENT, 0, 0);
    init_combo (hWnd);
    update_params(hWnd);
}


/* combo_changed:
 *  Changement de sélection du combobox.
 */
static void combo_changed (HWND hWnd)
{
    int index = SendDlgItemMessage(hWnd, K7_COMBO, CB_GETCURSEL, 0, 0);

    if (index != combo_index)
    {
        if (index == 0)
        {
            eject_cass (hWnd);
        }
        else
        {
            (void)load_cass(hWnd, stringlist_text (path_list, index));
        }
        update_params(hWnd);
    }
}



/* open_file:
 *  Chargement d'un fichier cassette.
 */
static void open_file (HWND hWnd)
{
    char current_file[BUFFER_SIZE]="";
    OPENFILENAME openfilename;

    memset(&openfilename, 0, sizeof(OPENFILENAME));
    openfilename.lStructSize = sizeof(OPENFILENAME);
    openfilename.hwndOwner = hWnd;
    openfilename.lpstrFilter = is_fr?"Fichiers K7\0*.k7\0":"K7 files\0*.k7\0";
    openfilename.nFilterIndex = 1;
    openfilename.lpstrFile = current_file;
    openfilename.lpstrInitialDir = (current_dir[0]=='\0')?".\\k7":current_dir;
    openfilename.nMaxFile = BUFFER_SIZE;
    openfilename.lpstrTitle = is_fr?"Choisissez votre cassette:":"Choose your tape";
    openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    openfilename.lpstrDefExt ="k7";

    if (GetOpenFileName(&openfilename))
    {
         if (load_cass (hWnd, openfilename.lpstrFile) != TO8_ERROR)
         {
             add_combo_entry (hWnd, to8_GetK7Filename());
             update_params(hWnd);
         }
         strcpy(current_dir, current_file);
    }
}


/* --------------------------- Partie publique ----------------------------- */


/* free_cass_list
 *  Libère la mémoire utilisée par la liste des cassettes.
 */
void free_cass_list (void)
{
    stringlist_free (path_list);
    path_list=NULL;
    entry_max=0;
    combo_index=0;
}



/* CassetteTabProc:
 * Procédure pour l'onglet des cassettes.
 */
int CALLBACK CassetteTabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static int first=1;
   static HWND counter_updown;
   LPNM_UPDOWN nmupdown;
   HANDLE himg;
   int counter;
   struct STRING_LIST *slist;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         /* initialisation du sélecteur de cassettes */
         if (first)
         {
             init_combo (hWnd);
             if (strlen(to8_GetK7Filename()))
                 add_combo_entry (hWnd, to8_GetK7Filename());
             update_params (hWnd);
             first=0;
         }
         /* initialisation du combo */
         SendDlgItemMessage(hWnd, K7_COMBO, CB_RESETCONTENT, 0, 0);
         for (slist=path_list; slist!=NULL; slist=slist->next)
             SendDlgItemMessage(hWnd, K7_COMBO, CB_ADDSTRING, 0, (LPARAM) basename_ptr(slist->str));
         SendDlgItemMessage(hWnd, K7_COMBO, CB_SETCURSEL, combo_index, 0);

         /* initialisation de la protection */
         CheckDlgButton(hWnd, K7_PROT_CHECK, prot);

         /* initialisation du compteur de cassettes */
         counter_updown = GetDlgItem(hWnd, K7_UPDOWN);
         SendMessage(counter_updown, UDM_SETRANGE, 0, MAKELONG(COUNTER_MAX, 0));
         set_counter_cass(hWnd);

         /* initialisation des images */
         himg=LoadImage (prog_inst, "empty_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, K7_EJECT_BUTTON), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);
         himg=LoadImage (prog_inst, "open_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, K7_MORE_BUTTON), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);

         /* initialisation des textes */
         SetWindowText(GetDlgItem(hWnd, K7_COUNTER_LTEXT), is_fr?"Compteur:":" Counter:");
         SetWindowText(GetDlgItem(hWnd, K7_REWIND_BUTTON), is_fr?"Rembobiner":"Rewind");

         /* initialisation del'info-bulle */
         create_tooltip (hWnd, K7_EJECT_BUTTON, is_fr?"Vider la liste":"Empty the list");
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case K7_EJECT_BUTTON:
                clear_combo(hWnd);
                break;

            case K7_MORE_BUTTON:
                open_file(hWnd);
                break;

            case K7_COMBO:
                if (HIWORD(wParam)==CBN_SELCHANGE)
                {
                   combo_changed (hWnd);
                }
                break;

            case K7_PROT_CHECK:
                toggle_check_cass(hWnd);
                break;

            case K7_REWIND_BUTTON:
                rewind_cass (hWnd);
                break;

            case K7_COUNTER_EDIT:
                if (HIWORD(wParam) == EN_CHANGE)
                {
                   counter = GetDlgItemInt(hWnd, K7_COUNTER_EDIT, NULL, FALSE);
                   to8_SetK7Counter(counter);
                   SendMessage(counter_updown, UDM_SETPOS, 0, MAKELONG(counter, 0));
                }
                break;
         }
         return TRUE;

     case WM_NOTIFY:
         nmupdown = (LPNM_UPDOWN)lParam;
         if ((HWND)nmupdown->hdr.hwndFrom == counter_updown)
         {
             counter = nmupdown->iPos;
             to8_SetK7Counter(counter);
             SetDlgItemInt(hWnd, K7_COUNTER_EDIT, counter, FALSE);
         }
         return TRUE;

     default:
         return FALSE;
   }
}
