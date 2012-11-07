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
 *  Module     : win/wcass.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               24/10/2012
 *
 *  Gestion des cassettes.
 */


#include "teo_win.h"

#define COUNTER_MAX  999

static int entry_max = 0;
static int combo_index = 0;
static struct STRING_LIST *path_list = NULL;



/* update_params:
 *  Sauve les paramètres d'une cassette.
 */
static void update_params (HWND hWnd)
{
    teo.cass.write_protect = (IsDlgButtonChecked(hWnd, K7_PROT_CHECK)
                                 == BST_CHECKED) ? TRUE : FALSE;
    combo_index = SendDlgItemMessage(hWnd, K7_COMBO, CB_GETCURSEL, 0, 0);
}



/* set_counter_cass:
 *  Positionne le compteur de cassette
 */
static void set_counter_cass (HWND hWnd)
{
   int counter = cass_GetCounter();

   SetDlgItemInt(hWnd, K7_COUNTER_EDIT, counter, FALSE);
   SendDlgItemMessage(hWnd, K7_UPDOWN, UDM_SETPOS, 0, MAKELONG(counter, 0));
}



/* rewind_cass:
 *  Met le compteur de cassette à 0
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
        case ERR_ERROR :
            MessageBox(hWnd, to8_error_msg, PROGNAME_STR,
                               MB_OK | MB_ICONINFORMATION);
            break;

        case TO8_READ_ONLY :
            CheckDlgButton(hWnd, K7_PROT_CHECK, BST_CHECKED);
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
    if (IsDlgButtonChecked(hWnd, K7_PROT_CHECK) == BST_CHECKED)
    {
        cass_SetMode(TO8_READ_ONLY);
    }
    else
    if (cass_SetMode(TO8_READ_WRITE)==TO8_READ_ONLY)
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
    int index = std_StringListIndex (path_list, (char *)path);

    if (index<0)
    {
        path_list = std_StringListAppend (path_list, (char *)path);
        SendDlgItemMessage(hWnd, K7_COMBO, CB_ADDSTRING, 0,
                           (LPARAM) std_BaseName((char *)path));
        SendDlgItemMessage(hWnd, K7_COMBO, CB_SETCURSEL, entry_max, 0);
        entry_max++;
    }
    else
    {
        SendDlgItemMessage(hWnd, K7_COMBO, CB_SETCURSEL, index, 0);
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
    add_combo_entry (hWnd, is_fr?"(Aucun)":"(None)");
}



/* clear_combo:
 *  Vide un combo.
 */
static void clear_combo (HWND hWnd)
{
    wcass_Free ();
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
    char def_folder[] = ".\\k7";
    OPENFILENAME ofn;

    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = is_fr?"Fichiers K7\0*.k7\0":"K7 files\0*.k7\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = current_file;
    ofn.nMaxFile = BUFFER_SIZE;
    ofn.lpstrTitle = is_fr?"Choisissez votre cassette:":"Choose your tape";
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
         if (load_cass (hWnd, ofn.lpstrFile) < 0)
         {
             add_combo_entry (hWnd, teo.cass.file);
             teo.default_folder = std_free (teo.default_folder);
             teo.default_folder = std_strdup_printf ("%s", teo.cass.file);
            (void)snprintf (current_file, MAX_PATH, "%s", teo.cass.file);
             update_params(hWnd);
         }
    }
}


/* ------------------------------------------------------------------------- */


/* wcass_Free:
 *  Libère la mémoire utilisée par la liste des cassettes.
 */
void wcass_Free (void)
{
    std_StringListFree (path_list);
    path_list=NULL;
    entry_max=0;
    combo_index=0;
}



/* wcass_TabProc:
 *  Procédure pour l'onglet des cassettes.
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
         /* initialisation du sélecteur de cassettes */
         if (first)
         {
             init_combo (hWnd);
             if (teo.cass.file != NULL)
                 add_combo_entry (hWnd, teo.cass.file);
             
             combo_index = SendDlgItemMessage(hWnd, K7_COMBO, CB_GETCURSEL, 0, 0);
             first=0;
         }
         /* initialisation du combo */
         SendDlgItemMessage(hWnd, K7_COMBO, CB_RESETCONTENT, 0, 0);
         for (slist=path_list; slist!=NULL; slist=slist->next)
             SendDlgItemMessage(hWnd, K7_COMBO, CB_ADDSTRING, 0,
                                (LPARAM) std_BaseName(slist->str));
         SendDlgItemMessage(hWnd, K7_COMBO, CB_SETCURSEL, combo_index, 0);

         /* initialisation de la protection */
         state = (teo.cass.write_protect == TRUE) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hWnd, K7_PROT_CHECK, state);

         /* initialisation du compteur de cassettes */
         counter_updown = GetDlgItem(hWnd, K7_UPDOWN);
         SendMessage(counter_updown, UDM_SETRANGE, 0, MAKELONG(COUNTER_MAX, 0));
         set_counter_cass(hWnd);

         /* initialisation des images */
         himg=LoadImage (prog_inst, "empty_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, K7_EJECT_BUTTON), BM_SETIMAGE,
                                (WPARAM)IMAGE_ICON, (LPARAM)himg);
         himg=LoadImage (prog_inst, "cass_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
         SendMessage(GetDlgItem(hWnd, K7_MORE_BUTTON), BM_SETIMAGE,
                                (WPARAM)IMAGE_ICON, (LPARAM)himg);

         /* initialisation des textes */
         SetWindowText(GetDlgItem(hWnd, K7_COUNTER_LTEXT),
                         is_fr?"Compteur:":" Counter:");
         SetWindowText(GetDlgItem(hWnd, K7_REWIND_BUTTON),
                         is_fr?"Rembobiner":"Rewind");

         /* initialisation des info-bulles */
         wgui_CreateTooltip (hWnd, K7_EJECT_BUTTON,
                             is_fr?"Vider la liste des fichiers"
                                  :"Empty the file list");
         wgui_CreateTooltip (hWnd, K7_MORE_BUTTON,
                             is_fr?"Ouvrir un fichier cassette"
                                  :"Open a tape file");

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
             SetDlgItemInt(hWnd, K7_COUNTER_EDIT, counter, FALSE);
         }
         return TRUE;

     default:
         return FALSE;
   }
}
