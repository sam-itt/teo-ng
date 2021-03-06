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
 *  Module     : win/wcass.c
 *  Version    : 1.8.5
 *  Cr�� par   : Eric Botcazou 28/11/2000
 *  Modifi� par: Eric Botcazou 28/10/2003
 *               Fran�ois Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               24/10/2012 10/05/2014
 *               Samuel Cuella 02/2020
 *
 *  Gestion des cassettes.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <string.h>
#endif

#include "teo.h"
#include "std.h"
#include "errors.h"
#include "media/cass.h"
#include "win/gui.h"
#include "gettext.h"

#define COUNTER_MAX  999

static int entry_max = 0;
static int combo_index = 0;
static struct STRING_LIST *path_list = NULL;



/* update_params:
 *  Sauve les param�tres d'une cassette.
 */
static void update_params (HWND hWnd)
{
    teo.cass.write_protect = (IsDlgButtonChecked(hWnd, IDC_K7_PROT_CHECK)
                                 == BST_CHECKED) ? TRUE : FALSE;
    combo_index = SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_GETCURSEL, 0, 0);

    if (combo_index == 0)
    {
        Button_Enable(GetDlgItem (hWnd, IDC_K7_UPDOWN), FALSE);
        Edit_Enable(GetDlgItem (hWnd, IDC_K7_COUNTER_EDIT), FALSE);
        Button_Enable(GetDlgItem (hWnd, IDC_K7_REWIND_BUTTON), FALSE);
        Static_Enable(GetDlgItem (hWnd, IDC_K7_COUNTER_LTEXT), FALSE);
        Button_Enable(GetDlgItem (hWnd, IDC_K7_EJECT_BUTTON), FALSE);
        Button_Enable(GetDlgItem (hWnd, IDC_K7_PROT_CHECK), FALSE);
    }
    else
    {
        Button_Enable(GetDlgItem (hWnd, IDC_K7_UPDOWN), TRUE);
        Edit_Enable(GetDlgItem (hWnd, IDC_K7_COUNTER_EDIT), TRUE);
        Button_Enable(GetDlgItem (hWnd, IDC_K7_REWIND_BUTTON), TRUE);
        Static_Enable(GetDlgItem (hWnd, IDC_K7_COUNTER_LTEXT), TRUE);
        Button_Enable(GetDlgItem (hWnd, IDC_K7_EJECT_BUTTON), TRUE);
        Button_Enable(GetDlgItem (hWnd, IDC_K7_PROT_CHECK), TRUE);
    }
}


/* set_counter_cass:
 *  Positionne le compteur de cassette
 */
static void set_counter_cass (HWND hWnd)
{
   int counter = cass_GetCounter();

   SetDlgItemInt(hWnd, IDC_K7_COUNTER_EDIT, counter, FALSE);
   SendDlgItemMessage(hWnd, IDC_K7_UPDOWN, UDM_SETPOS, 0, MAKELONG(counter, 0));
}



/* rewind_cass:
 *  Met le compteur de cassette � 0
 */
static void rewind_cass (HWND hWnd)
{
   cass_SetCounter(0);
   set_counter_cass (hWnd);
}



/* eject_cass:
 *  Ejecte la cassette
 */
static void eject_cass (HWND hWnd)
{
    rewind_cass (hWnd);
    cass_Eject();
}



/* load_cass:
 *  Charge une cassette.
 */
static int load_cass (HWND hWnd, char *filename)
{
    int ret = cass_Load (filename);

    switch (ret)
    {
        case TEO_ERROR :
            MessageBox(hWnd, teo_error_msg, PROGNAME_STR,
                               MB_OK | MB_ICONINFORMATION);
            break;

        case TRUE :
            CheckDlgButton(hWnd, IDC_K7_PROT_CHECK, BST_CHECKED);
            break;

        default : break;
    }
    rewind_cass(hWnd);
    update_params(hWnd);
    return ret;
}



/* toggle_check_cass:
 *  Gestion de la protection.
 */
static void toggle_check_cass (HWND hWnd)
{
    if (IsDlgButtonChecked(hWnd, IDC_K7_PROT_CHECK) == BST_CHECKED)
    {
        cass_SetProtection(TRUE);
    }
    else
    if (cass_SetProtection(FALSE)==TRUE)
    {
        MessageBox(hWnd, _("Warning: writing unavailable on this device.")
                       , PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
        CheckDlgButton(hWnd, IDC_K7_PROT_CHECK, BST_CHECKED); 
    }
    update_params(hWnd);
    set_counter_cass (hWnd);
}



/* add_combo_entry
 *  Ajoute une entr�e dans le combobox si inexistante.
 */
static void add_combo_entry (HWND hWnd, const char *path)
{
    int index = std_StringListIndex (path_list, (char *)path);

    if (index<0)
    {
        path_list = std_StringListAppend (path_list, (char *)path);
        SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_ADDSTRING, 0,
                           (LPARAM) std_BaseName((char *)path));
        SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_SETCURSEL, entry_max, 0);
        entry_max++;
    }
    else
    {
        SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_SETCURSEL, index, 0);
        if (index != combo_index)
        {
            (void)load_cass (hWnd, std_StringListText (path_list, index));
        }
    }
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (HWND hWnd)
{
    add_combo_entry (hWnd, _("(None)"));}



/* clear_combo:
 *  Vide un combo.
 */
static void clear_combo (HWND hWnd)
{
    wcass_Free ();
    eject_cass (hWnd);
    SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_RESETCONTENT, 0, 0);
    init_combo (hWnd);
    update_params(hWnd);
}


/* combo_changed:
 *  Changement de s�lection du combobox.
 */
static void combo_changed (HWND hWnd)
{
    int index = SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_GETCURSEL, 0, 0);

    if (index != combo_index)
    {
        if (index == 0)
        {
            eject_cass (hWnd);
        }
        else
        {
            (void)load_cass(hWnd, std_StringListText (path_list, index));
        }
        update_params(hWnd);
    }
}



/* open_file:
 *  Chargement d'un fichier cassette.
 */
static void open_file (HWND hWnd)
{
    char current_file[MAX_PATH+1]="";
    char def_folder[] = ".\\cass";
    OPENFILENAME ofn;

    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;

    /*Gettext can't handle in-string \0. They are encoded
     * as \a(bell) and in-place converted to suit format specified
     * for OPENFILENAME lpstrFilter*/
    char *_filter = strdup(_("K7 files\a*.k7\a"));
    int filter_len = strlen(_filter);
    for(int i = 0; i < filter_len; i++){
        if(_filter[i] == '\a')
            _filter[i] = '\0';
    }
    ofn.lpstrFilter = _filter;

    ofn.nFilterIndex = 1;
    ofn.lpstrFile = current_file;
    ofn.nMaxFile = BUFFER_SIZE;
    ofn.lpstrTitle = _("Select your tape");
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt ="k7";

    if (teo.cass.file != NULL)
        ofn.lpstrInitialDir = teo.cass.file;
    else
    if (teo.default_folder != NULL)
        ofn.lpstrInitialDir = teo.default_folder;
    else
    if (access(def_folder, F_OK) == 0)
        ofn.lpstrInitialDir = def_folder;
      
    if (GetOpenFileName(&ofn))
    {
         if (load_cass (hWnd, ofn.lpstrFile) >= 0)
         {
             add_combo_entry (hWnd, teo.cass.file);
             teo.default_folder = std_free (teo.default_folder);
             teo.default_folder = std_strdup_printf ("%s", teo.cass.file);
            (void)snprintf (current_file, MAX_PATH, "%s", teo.cass.file);
             update_params(hWnd);
         }
    }
    free(_filter);
}


/* ------------------------------------------------------------------------- */


/* wcass_Free:
 *  Lib�re la m�moire utilis�e par la liste des cassettes.
 */
void wcass_Free (void)
{
    std_StringListFree (path_list);
    path_list=NULL;
    entry_max=0;
    combo_index=0;
}



/* wcass_TabProc:
 *  Proc�dure pour l'onglet des cassettes.
 */
int CALLBACK wcass_TabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static int first=1;
   int state;
   static HWND counter_updown;
   LPNM_UPDOWN nmupdown;
   HANDLE himg;
   int counter;
   struct STRING_LIST *slist;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         /* initialisation du s�lecteur de cassettes */
         if (first)
         {
             init_combo (hWnd);
             if (teo.cass.file != NULL)
                 add_combo_entry (hWnd, teo.cass.file);
             
             combo_index = SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_GETCURSEL, 0, 0);
             first=0;
         }
         /* initialisation du combo */
         SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_RESETCONTENT, 0, 0);
         for (slist=path_list; slist!=NULL; slist=slist->next)
             SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_ADDSTRING, 0,
                                (LPARAM) std_BaseName(slist->str));
         SendDlgItemMessage(hWnd, IDC_K7_COMBO, CB_SETCURSEL, combo_index, 0);

         /* initialisation de la protection */
         state = (teo.cass.write_protect == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, IDC_K7_PROT_CHECK, state);

         /* initialisation du compteur de cassettes */
         counter_updown = GetDlgItem(hWnd, IDC_K7_UPDOWN);
         SendMessage(counter_updown, UDM_SETRANGE, 0, MAKELONG(COUNTER_MAX, 0));
         set_counter_cass(hWnd);

         /* initialisation des images */
         himg=LoadImage (prog_inst, "clearlst_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, IDC_K7_EJECT_BUTTON), BM_SETIMAGE,
                                (WPARAM)IMAGE_ICON, (LPARAM)himg);
         himg=LoadImage (prog_inst, "folder_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, IDC_K7_MORE_BUTTON), BM_SETIMAGE,
                                (WPARAM)IMAGE_ICON, (LPARAM)himg);

         /* initialisation des textes */
         SetWindowText(GetDlgItem(hWnd, IDC_K7_COUNTER_LTEXT),
                         _("Counter:"));
         SetWindowText(GetDlgItem(hWnd, IDC_K7_REWIND_BUTTON),
                         _("Rewind"));

         /* initialisation des info-bulles */
         wgui_CreateTooltip (hWnd, IDC_K7_EJECT_BUTTON,
                             _("clear the file list"));
         wgui_CreateTooltip (hWnd, IDC_K7_MORE_BUTTON,
                             _("Open a tape file"));
         update_params(hWnd);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDC_K7_EJECT_BUTTON:
                clear_combo(hWnd);
                break;

            case IDC_K7_MORE_BUTTON:
                open_file(hWnd);
                break;

            case IDC_K7_COMBO:
                if (HIWORD(wParam)==CBN_SELCHANGE)
                {
                   combo_changed (hWnd);
                }
                break;

            case IDC_K7_PROT_CHECK:
                toggle_check_cass(hWnd);
                break;

            case IDC_K7_REWIND_BUTTON:
                rewind_cass (hWnd);
                break;

            case IDC_K7_COUNTER_EDIT:
                if (HIWORD(wParam) == EN_CHANGE)
                {
                   counter = GetDlgItemInt(hWnd, IDC_K7_COUNTER_EDIT, NULL, FALSE);
                   cass_SetCounter(counter);
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
             cass_SetCounter(counter);
             SetDlgItemInt(hWnd, IDC_K7_COUNTER_EDIT, counter, FALSE);
         }
         return TRUE;

     default:
         return FALSE;
   }
}
