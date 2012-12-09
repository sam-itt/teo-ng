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
 *          TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOOaucun
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
 *  Module     : win/wgui/wdisk.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               24/10/2012
 *
 *  Gestion des disquettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <string.h>
   #include <windows.h>
   #include <shellapi.h>
   #include <commctrl.h>
   #include <shlobj.h>
#endif

#include "win/dialog.rh"
#include "win/gui.h"
#include "media/disk.h"
#include "std.h"
#include "error.h"
#include "teo.h"

static int direct_disk_support=0;

struct FILE_VECTOR {
    int id;
    int direct;
    int entry_max;
    int combo_index;
    char *current_dir;
    struct STRING_LIST *path_list;
};

static struct FILE_VECTOR vector[NBDRIVE];



/* update_params:
 *  Sauve les paramètres d'un disque.
 */
static void update_params (HWND hWnd, struct FILE_VECTOR *vector)
{
    int state = IsDlgButtonChecked(hWnd, DISK0_PROT_CHECK+vector->id);

    teo.disk[vector->id].write_protect = (state == BST_CHECKED) ? TRUE : FALSE;
    vector->combo_index = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id,
                                             CB_GETCURSEL, 0, 0);
}



/* load_disk:
 *  Charge une disquette.
 */
static int load_disk (HWND hWnd, char *filename, struct FILE_VECTOR *vector)
{
    int ret = disk_Load (vector->id, filename);

    switch (ret)
    {
        case TEO_ERROR :
            MessageBox(hWnd, teo_error_msg, PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
            break;

        case TO8_READ_ONLY :
            CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id, BST_CHECKED);
            break;

        default : break;
    }
    return ret;
}



/* set_access_mode:
 *  Positionne le checkbox de protection de disquette.
 */
static void set_access_mode (HWND hWnd, struct FILE_VECTOR *vector)
{
    int ret = disk_SetVirtual(vector->id);
    int index = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);

    ret = (teo.disk[vector->id].write_protect == TRUE)
                      ? TO8_READ_ONLY : TO8_READ_WRITE;
    if ((vector->direct) && (index == 1))
        ret = disk_SetDirect(vector->id);
    CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id,
                   (ret==TO8_READ_ONLY) ? BST_CHECKED : BST_UNCHECKED);
    update_params (hWnd, vector);
}



/* combo_changed:
 *  Changement de sélection du combobox.
 */
static void combo_changed (HWND hWnd, struct FILE_VECTOR *vector)
{
    int index = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);

    if (index != vector->combo_index)
    {
        if (index==0)
        {
            disk_Eject(vector->id);
        }
        else
        {
            if ((index!=1) || (vector->direct!=1))
                (void)load_disk (hWnd, std_StringListText (vector->path_list, index), vector);
        }
        update_params (hWnd, vector);
        set_access_mode (hWnd, vector);
    }
}



/* toggle_check_disk:
 *  Change la protection disque.
 */
static void toggle_check_disk(HWND hWnd, struct FILE_VECTOR *vector)
{
    if (IsDlgButtonChecked(hWnd, DISK0_PROT_CHECK+vector->id) == BST_CHECKED)
    {
        disk_SetMode (vector->id, TO8_READ_ONLY);
    }
    else
    if (disk_SetMode (vector->id, TO8_READ_WRITE)==TO8_READ_ONLY)
    {
        MessageBox(hWnd, is_fr?"Ecriture impossible sur ce support."
                              :"Warning: writing unavailable on this device."
                       , PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
        CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id, BST_CHECKED);
    }
    update_params (hWnd, vector);
}



/* add_combo_entry:
 *  Ajoute une entrée dans le combobox si inexistante et
 *  charge le fichier SAP correspondant si existante
 */
static void add_combo_entry (HWND hWnd, const char *path,
                               struct FILE_VECTOR *vector)
{
    int index = std_StringListIndex (vector->path_list, (char *)path);

    if (index<0)
    {
        vector->path_list = std_StringListAppend (vector->path_list, (char *)path);
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_ADDSTRING, 0,
                           (LPARAM) std_BaseName((char *)path));
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_SETCURSEL,
                           vector->entry_max, 0);
        vector->entry_max++;
    }
    else
    {
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_SETCURSEL, index, 0);
        if (index != vector->combo_index)
        {
            (void)load_disk (hWnd, std_StringListText (vector->path_list, index), vector);
        }
    }
}



/* free_disk_entry:
 *  Libère la mémoire utilisée par la liste des disquettes.
 */
static void free_disk_entry (struct FILE_VECTOR *vector)
{
    std_StringListFree (vector->path_list);
    vector->path_list=NULL;
    vector->entry_max=0;
    vector->combo_index=0;
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (HWND hWnd, struct FILE_VECTOR *vector)
{
    add_combo_entry (hWnd, is_fr?"(Aucun)":"(None)", vector);
    if (vector->direct)
        add_combo_entry (hWnd, is_fr?"(Accès Direct)":"(Direct Access)", vector);
    set_access_mode (hWnd, vector);
}



/* clear_combo:
 *  Vide un combo.
 */
static void clear_combo (HWND hWnd, struct FILE_VECTOR *vector)
{
     disk_Eject(vector->id);
     free_disk_entry (vector);
     SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_RESETCONTENT, 0, 0);
     init_combo (hWnd, vector);
     update_params (hWnd, vector);
}



/* open_file:
 *  Charge une nouvelle disquette.
 */
static void open_file (HWND hWnd, struct FILE_VECTOR *vector)
{
    static char current_file[MAX_PATH+1]="";
    char def_folder[] = ".\\disks";
    OPENFILENAME ofn;

    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = is_fr?"Fichiers SAP\0*.sap\0":"SAP files\0*.sap\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = current_file;
    ofn.nMaxFile = BUFFER_SIZE;
    ofn.lpstrTitle = is_fr?"Choisissez votre disquette:":"Choose your disk:";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt ="sap";

    if (teo.disk[vector->id].file != NULL)
        ofn.lpstrInitialDir = teo.disk[vector->id].file;
    else
    if (teo.default_folder != NULL)
        ofn.lpstrInitialDir = teo.default_folder;
    else
    if (access(def_folder, F_OK) == 0)
        ofn.lpstrInitialDir = def_folder;

    if (GetOpenFileName(&ofn))
    {
        if (load_disk (hWnd, ofn.lpstrFile, vector) >= 0)
        {
            add_combo_entry (hWnd, teo.disk[vector->id].file, vector);
            teo.default_folder = std_free (teo.default_folder);
            teo.default_folder = std_strdup_printf ("%s", teo.disk[vector->id].file);
            (void)snprintf (current_file, MAX_PATH, "%s", teo.disk[vector->id].file);
            update_params (hWnd, vector);
        }
    }
}


#if 0
/* open_folder_Callback:
 *  Callback pour la boîte de dialogue du répertoire.
 */
static int CALLBACK open_folder_Callback(HWND hWnd, UINT uMsg,
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
static void open_folder (HWND hWnd, struct FILE_VECTOR *vector)
{
    BROWSEINFO bi;
    ITEMIDLIST *pidl;
    static char folder_name[MAX_PATH] = "";

    bi.hwndOwner = hWnd;
    bi.pidlRoot = NULL;
//    bi.pszDisplayName = (LPTSTR)folder_name;
    bi.lpszTitle = is_fr ? "Choisir un répertoire":"Choose a directory";
    bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = open_folder_Callback;
    bi.lParam = (LPARAM)folder_name;

    if (teo.disk[vector->id].file != NULL)
        bi.pszDisplayName = (LPTSTR)teo.disk[vector->id].file;
    else
    if (teo.default_folder != NULL)
        bi.pszDisplayName = (LPTSTR)teo.default_folder;
    else
    if (access(folder_name, F_OK) == 0)
        bi.pszDisplayName = (LPTSTR)folder_name;

    pidl = SHBrowseForFolder(&bi);

    if (SHGetPathFromIDList(pidl, (LPTSTR)folder_name) == TRUE)
    {
        if (load_disk (hWnd, folder_name, vector) >= 0)
        {
            add_combo_entry (hWnd, teo.disk[vector->id].file, vector);
            teo.default_folder = std_free(teo.default_folder);
            if (*folder_name != '\0')
                teo.default_folder = std_strdup_printf ("%s", folder_name);
        }
    }
}
#endif

/* ------------------------------------------------------------------------- */


/* wdisk_Free:
 *  Libère la mémoire utilisée par les listes de disquettes.
 */
void wdisk_Free (void)
{
    int i;

    for (i=0; i<NBDRIVE; i++)
        free_disk_entry (&vector[i]);
}



/* wdisk_TabProc:
 *  Procédure pour l'onglet des disquettes.
 */
int CALLBACK wdisk_TabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static int first=1;
   int drive;
   int state;
   HANDLE himg;
   struct STRING_LIST *slist;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         for (drive=0; drive<NBDRIVE; drive++)
         {
             if (first)
             {
                 /* initialisation du sélecteur de disquettes */
                 memset (&vector[drive], 0x00, sizeof (struct FILE_VECTOR));
                 vector[drive].id=drive;
                 vector[drive].direct=(direct_disk_support>>drive)&1;
                 init_combo (hWnd, &vector[drive]);
                 if (teo.disk[drive].file != NULL)
                 {
                     add_combo_entry (hWnd, teo.disk[drive].file, &vector[drive]);
                     vector[drive].current_dir = std_strdup_printf ("%s",
                                                         teo.disk[drive].file);
                 }
                 set_access_mode (hWnd, &vector[drive]);
                 update_params (hWnd, &vector[drive]);
             }
             /* initialisation du combo */
             SendDlgItemMessage(hWnd, DISK0_COMBO+drive, CB_RESETCONTENT, 0, 0);
             for (slist=vector[drive].path_list; slist!=NULL; slist=slist->next)
                 SendDlgItemMessage(hWnd, DISK0_COMBO+drive, CB_ADDSTRING, 0,
                                    (LPARAM) std_BaseName(slist->str));
             SendDlgItemMessage(hWnd, DISK0_COMBO+drive, CB_SETCURSEL,
                                vector[drive].combo_index, 0);

             /* initialisation de la protection */
             state = (teo.disk[drive].write_protect == TRUE) ? BST_CHECKED : BST_UNCHECKED;
             CheckDlgButton(hWnd, DISK0_PROT_CHECK+drive, state);

             /* initialisation des images */
             himg=LoadImage (prog_inst, "empty_ico",IMAGE_ICON, 0, 0,
                             LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, DISK0_EJECT_BUTTON+drive),
                           BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);
             himg=LoadImage (prog_inst, "disk_ico",IMAGE_ICON, 0, 0,
                             LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, DISK0_MORE_BUTTON+drive),
                           BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);
             himg=LoadImage (prog_inst, "open_ico",IMAGE_ICON, 0, 0,
                             LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, DISK0_FOLDER_BUTTON+drive),
                           BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);
             
             /* initialisation des info-bulles */
             wgui_CreateTooltip (hWnd, DISK0_EJECT_BUTTON+drive,
                                 is_fr?"Vider la liste des fichiers"
                                      :"Empty the file list");
             wgui_CreateTooltip (hWnd, DISK0_MORE_BUTTON+drive,
                                 is_fr?"Ouvrir un fichier disquette"
                                      :"Open a disk file");
         }
         first=0;
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case DISK0_EJECT_BUTTON:
            case DISK1_EJECT_BUTTON:
            case DISK2_EJECT_BUTTON:
            case DISK3_EJECT_BUTTON: 
               clear_combo(hWnd, &vector[LOWORD(wParam)-DISK0_EJECT_BUTTON]);
               break;

            case DISK0_MORE_BUTTON:
            case DISK1_MORE_BUTTON:
            case DISK2_MORE_BUTTON:
            case DISK3_MORE_BUTTON: 
               open_file(hWnd, &vector[LOWORD(wParam)-DISK0_MORE_BUTTON]);
               break;
#if 0
            case DISK0_FOLDER_BUTTON:
            case DISK1_FOLDER_BUTTON:
            case DISK2_FOLDER_BUTTON:
            case DISK3_FOLDER_BUTTON: 
               open_folder(hWnd, &vector[LOWORD(wParam)-DISK0_FOLDER_BUTTON]);
               break;
#endif
            case DISK0_COMBO:
            case DISK1_COMBO:
            case DISK2_COMBO:
            case DISK3_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
                   combo_changed(hWnd, &vector[LOWORD(wParam)-DISK0_COMBO]);
               break;

            case DISK0_PROT_CHECK:
            case DISK1_PROT_CHECK:
            case DISK2_PROT_CHECK:
            case DISK3_PROT_CHECK: 
               toggle_check_disk(hWnd, &vector[LOWORD(wParam)-DISK0_PROT_CHECK]);
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}
