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
 *  Module     : win/gui.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *
 *  Gestion des disquettes.
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

#define NDISKS 4

static int direct_disk_support=0;

struct FILE_VECTOR {
    int id;
    int direct;
    int entry_max;
    int combo_index;
    UINT prot;
    char current_dir[BUFFER_SIZE];
    struct STRING_LIST *path_list;
};

static struct FILE_VECTOR vector[NDISKS];



/* update_params:
 *  Sauve les paramètres d'un disque.
 */
static void update_params (HWND hWnd, struct FILE_VECTOR *vector)
{
    vector->prot = IsDlgButtonChecked(hWnd, DISK0_PROT_CHECK+vector->id);
    vector->combo_index = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);
}



/* load_disk:
 *  Charge une disquette.
 */
static int load_disk (HWND hWnd, char *filename, struct FILE_VECTOR *vector)
{
    int ret = to8_LoadDisk (vector->id, filename);

    switch (ret)
    {
        case TO8_ERROR :
            MessageBox(hWnd, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
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
    int ret = to8_VirtualSetDrive(vector->id);
    int index = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);

    if ((vector->direct) && (index == 1))
        ret = to8_DirectSetDrive(vector->id);
    CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id, (ret==TO8_READ_ONLY) ? BST_CHECKED : BST_UNCHECKED);
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
            to8_EjectDisk(vector->id);
        }
        else
        {
            if ((index!=1) || (vector->direct!=1))
                (void)load_disk (hWnd, stringlist_text (vector->path_list, index), vector);
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
        to8_SetDiskMode (vector->id, TO8_READ_ONLY);
    }
    else
    if (to8_SetDiskMode (vector->id, TO8_READ_WRITE)==TO8_READ_ONLY)
    {
        MessageBox(hWnd, is_fr?"Ecriture impossible sur ce support."
                              :"Warning: writing unavailable on this device."
                       , PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
        CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id, BST_CHECKED);
    }
    update_params (hWnd, vector);
}



/* add_combo_entry
 *  Ajoute une entrée dans le combobox si inexistante et
 *  charge le fichier SAP correspondant si existante
 */
static void add_combo_entry (HWND hWnd, const char *path, struct FILE_VECTOR *vector)
{
    int index = stringlist_index (vector->path_list, (char *)path);

    if (index<0)
    {
        vector->path_list = stringlist_append (vector->path_list, (char *)path);
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_ADDSTRING, 0, (LPARAM) basename_ptr(path));
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_SETCURSEL, vector->entry_max, 0);
        vector->entry_max++;
    }
    else
    {
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_SETCURSEL, index, 0);
        if (index != vector->combo_index)
        {
            (void)load_disk (hWnd, stringlist_text (vector->path_list, index), vector);
        }
    }
}



/* free_disk_entry:
 *  Libère la mémoire utilisée par la liste des disquettes.
 */
static void free_disk_entry (struct FILE_VECTOR *vector)
{
    stringlist_free (vector->path_list);
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
     to8_EjectDisk(vector->id);
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
    static char last_dir[BUFFER_SIZE]=".\\disks";
    char current_file[BUFFER_SIZE]="";
    OPENFILENAME openfilename;

    memset(&openfilename, 0, sizeof(OPENFILENAME));
    openfilename.lStructSize = sizeof(OPENFILENAME);
    openfilename.hwndOwner = hWnd;
    openfilename.lpstrFilter = is_fr?"Fichiers SAP\0*.sap\0":"SAP files\0*.sap\0";
    openfilename.nFilterIndex = 1;
    openfilename.lpstrFile = current_file;
    openfilename.lpstrInitialDir = (vector->current_dir[0]=='\0')?last_dir:vector->current_dir;
    openfilename.nMaxFile = BUFFER_SIZE;
    openfilename.lpstrTitle = is_fr?"Choisissez votre disquette:":"Choose your disk:";
    openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    openfilename.lpstrDefExt ="sap";

    if (GetOpenFileName(&openfilename))
    {
        if (load_disk (hWnd, openfilename.lpstrFile, vector) != TO8_ERROR)
        {
            add_combo_entry (hWnd, to8_GetDiskFilename(vector->id), vector);
            update_params (hWnd, vector);
        }
        strcpy(vector->current_dir, current_file);
        strcpy(last_dir, current_file);
    }
}


/* --------------------------- Partie publique ----------------------------- */


/* free_disk_list:
 *  Libère la mémoire utilisée par les listes de disquettes.
 */
void free_disk_list (void)
{
    int i;

    for (i=0; i<NDISKS; i++)
        free_disk_entry (&vector[i]);
}



/* DiskTabProc:
 * Procédure pour l'onglet des disquettes.
 */
int CALLBACK DiskTabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   int drive;
   static int first=1;
   HANDLE himg;
   struct STRING_LIST *slist;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         for (drive=0; drive<NDISKS; drive++)
         {
             if (first)
             {
                 /* initialisation du sélecteur de disquettes */
                 memset (&vector[drive], 0x00, sizeof (struct FILE_VECTOR));
                 vector[drive].id=drive;
                 vector[drive].direct=(direct_disk_support>>drive)&1;
                 init_combo (hWnd, &vector[drive]);
                 if (strlen(to8_GetDiskFilename(drive)) != 0)
                 {
                     add_combo_entry (hWnd, to8_GetDiskFilename(drive), &vector[drive]);
                     vector[drive].current_dir[0]='\0';
                     strcpy (vector[drive].current_dir, to8_GetDiskFilename(drive));
                 }
                 set_access_mode (hWnd, &vector[drive]);
                 update_params (hWnd, &vector[drive]);
             }
             /* initialisation du combo */
             SendDlgItemMessage(hWnd, DISK0_COMBO+drive, CB_RESETCONTENT, 0, 0);
             for (slist=vector[drive].path_list; slist!=NULL; slist=slist->next)
                 SendDlgItemMessage(hWnd, DISK0_COMBO+drive, CB_ADDSTRING, 0, (LPARAM) basename_ptr(slist->str));
             SendDlgItemMessage(hWnd, DISK0_COMBO+drive, CB_SETCURSEL, vector[drive].combo_index, 0);

             /* initialisation de la protection */
             CheckDlgButton(hWnd, DISK0_PROT_CHECK+drive, vector[drive].prot);

             /* initialisation des images */
             himg=LoadImage (prog_inst, "empty_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, DISK0_EJECT_BUTTON+drive), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);
             himg=LoadImage (prog_inst, "open_ico",IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, DISK0_MORE_BUTTON+drive), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);
             
             /* initialisation de l'info-bulle */
             create_tooltip (hWnd, DISK0_EJECT_BUTTON+drive, is_fr?"Vider la liste":"Empty the list");

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

            case DISK0_COMBO:
            case DISK1_COMBO:
            case DISK2_COMBO:
            case DISK3_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
               {
                   combo_changed(hWnd, &vector[LOWORD(wParam)-DISK0_COMBO]);
               }
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
