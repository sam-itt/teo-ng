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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
   #include <windowsx.h>
   #include <shellapi.h>
   #include <commctrl.h>
   #include <shlobj.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "main.h"
#include "errors.h"
#include "media/disk/controlr.h"
#include "media/disk.h"
#include "media/disk/daccess.h"
#include "win/gui.h"
#include "win/dialog.rh"

#define NDISKS 4

static int direct_disk_support=0;

struct FILE_VECTOR {
    int first_file;
    int first_dir;
    int id;
    int direct;
    int entry_max;
    int combo_index;
    char *current_dir;
    struct DISK_VECTOR *path_list;
};

static struct FILE_VECTOR vector[NDISKS];



/* update_params:
 *  Sauve les paramètres d'un disque.
 */
static void update_params (HWND hWnd, struct FILE_VECTOR *vector)
{
    int state = IsDlgButtonChecked(hWnd, DISK0_PROT_CHECK+vector->id);

    teo.disk[vector->id].write_protect = (state == BST_CHECKED) ? TRUE : FALSE;
    vector->combo_index = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id,
                                             CB_GETCURSEL, 0, 0);

    if ((vector->combo_index == 0)
     || ((vector->combo_index == 1) && (vector->direct)))
    {
        Button_Enable(GetDlgItem (hWnd, DISK0_EJECT_BUTTON+vector->id), FALSE);
        ComboBox_Enable(GetDlgItem (hWnd, DISK0_SIDE_COMBO+vector->id), FALSE);
        Button_Enable(GetDlgItem (hWnd, DISK0_PROT_CHECK+vector->id), FALSE);
    }
    else
    {
        Button_Enable(GetDlgItem (hWnd, DISK0_EJECT_BUTTON+vector->id), TRUE);
        ComboBox_Enable(GetDlgItem (hWnd, DISK0_SIDE_COMBO+vector->id), TRUE);
        Button_Enable(GetDlgItem (hWnd, DISK0_PROT_CHECK+vector->id), TRUE);
    }
}



/* toggle_check_disk:
 *  Change la protection disque.
 */
static void toggle_check_disk(HWND hWnd, struct FILE_VECTOR *vector)
{
    if (IsDlgButtonChecked(hWnd, DISK0_PROT_CHECK+vector->id) == BST_CHECKED)
    {
        disk_SetProtection (vector->id, TRUE);
    }
    else
    if (disk_SetProtection (vector->id, FALSE)==TRUE)
    {
        MessageBox(hWnd, is_fr?"Ecriture impossible sur ce support."
                              :"Warning: writing unavailable on this device."
                       , PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
        CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id, BST_CHECKED);
        teo.disk[vector->id].write_protect = TRUE;
    }
    update_params (hWnd, vector);
}



#if 0
/* 
 *  Positionne le checkbox de protection de disquette.
 */
static void set_access_mode (HWND hWnd, struct FILE_VECTOR *vector)
{
    int ret = disk_SetVirtual(vector->id);
    int index = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);

    ret = (teo.disk[vector->id].write_protect == TRUE)
                      ? TEO_STATUS_READ_ONLY : TEO_STATUS_READ_WRITE;
    if ((vector->direct) && (index == 1))
        ret = disk_SetDirect(vector->id);
    CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id,
                   (ret==TEO_STATUS_READ_ONLY) ? BST_CHECKED : BST_UNCHECKED);
    update_params (hWnd, vector);
    (void)hWnd;
    (void)vector;
}
#endif



/* add_combo_entry:
 *  Ajoute une entrée dans le combobox si inexistante et
 *  charge le fichier SAP correspondant si existante
 */
static void add_combo_entry (HWND hWnd, const char *path, struct FILE_VECTOR *vector)
{
    struct DISK_VECTOR *disk_vector = NULL;

    int index = disk_DiskVectorIndex (vector->path_list, path);

    if (index >= 0)
    {
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_SETCURSEL, index, 0);
        disk_vector = disk_DiskVectorPtr (vector->path_list, index);
        teo.disk[vector->id].side = disk_vector->side;
        disk[vector->id].side_count = disk_vector->side_count;
    }
    else
    {
        vector->path_list = disk_DiskVectorAppend (vector->path_list, path,
                                                   teo.disk[vector->id].side,
                                                   disk[vector->id].side_count);
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_ADDSTRING, 0,
                           (LPARAM) std_BaseName((char *)path));
        SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_SETCURSEL,
                           vector->entry_max, 0);
        vector->entry_max++;
    }
//    set_access_mode (hWnd, vector);
}



/* free_disk_entry:
 *  Libère la mémoire utilisée par la liste des disquettes.
 */
static void free_disk_entry (struct FILE_VECTOR *vector)
{
    disk_DiskVectorFree (vector->path_list);
    vector->path_list=NULL;
    vector->entry_max = 0;
    vector->combo_index=0;
}



/* reset_side_combo:
 *  Reinitialize the combo for sides.
 */
static void reset_side_combo (HWND hWnd, int selected_side, struct FILE_VECTOR *vector)
{
    int side = 0;
    char *str = NULL;

    SendDlgItemMessage(hWnd, DISK0_SIDE_COMBO+vector->id, CB_RESETCONTENT, 0, 0);
    do
    {
        str = std_strdup_printf ("%d", side);
        SendDlgItemMessage(hWnd, DISK0_SIDE_COMBO+vector->id, CB_ADDSTRING, 0,
                           (LPARAM)str);
        str = std_free (str);
        side++;
    } while (side < disk[vector->id].side_count);
    
    SendDlgItemMessage(hWnd, DISK0_SIDE_COMBO+vector->id, CB_SETCURSEL,
                           selected_side, 0);
    teo.disk[vector->id].side = selected_side;
}



/* side_combo_changed:
 *  Changement de sélection du side combobox.
 */
static void side_combo_changed (HWND hWnd, struct FILE_VECTOR *vector)
{
    int active_row;
    struct DISK_VECTOR *p;

    teo.disk[vector->id].side = SendDlgItemMessage(hWnd, DISK0_SIDE_COMBO+vector->id, CB_GETCURSEL, 0, 0);
    active_row = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);
    p = disk_DiskVectorPtr (vector->path_list, active_row);
    p->side = teo.disk[vector->id].side;
    dkc->WriteUpdateTrack();
    disk[vector->id].info->track = -1;
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (HWND hWnd, struct FILE_VECTOR *vector)
{
    add_combo_entry (hWnd, is_fr?"(Aucun)":"(None)", vector);
    if (vector->direct)
        add_combo_entry (hWnd, is_fr?"(Accès Direct)":"(Direct Access)", vector);
//    set_access_mode (hWnd, vector);
}



/* reset_combo:
 *  Vide un combo.
 */
static void reset_combo (HWND hWnd, struct FILE_VECTOR *vector)
{
     free_disk_entry (vector);
     SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_RESETCONTENT, 0, 0);
     disk_Eject(vector->id);
     init_combo (hWnd, vector);
     update_params (hWnd, vector);
}



/* load_virtual_disk:
 *  Charge une disquette.
 */
static int load_virtual_disk (HWND hWnd, char *filename, struct FILE_VECTOR *vector)
{
    int ret = disk_Load (vector->id, filename);

    switch (ret)
    {
        case TEO_ERROR :
            MessageBox(hWnd, teo_error_msg, PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
            break;

        case TRUE :
            CheckDlgButton(hWnd, DISK0_PROT_CHECK+vector->id, BST_CHECKED);
            teo.disk[vector->id].write_protect = TRUE;
            break;

        default : break;
    }
    return ret;
}



/* combo_changed:
 *  Changement de sélection du combobox.
 */
static void combo_changed (HWND hWnd, struct FILE_VECTOR *vector)
{
    struct DISK_VECTOR *p=NULL;
    int active_row = SendDlgItemMessage(hWnd, DISK0_COMBO+vector->id, CB_GETCURSEL, 0, 0);

    if (active_row != vector->combo_index)
    {
        if (active_row == 0)
            disk_Eject(vector->id);
        else
        if ((active_row == 1) && (vector->direct))
            (void)daccess_LoadDisk (vector->id, "");
        else
        {
            p = disk_DiskVectorPtr (vector->path_list, active_row);
            (void)load_virtual_disk (hWnd, p->str, vector);
            reset_side_combo (hWnd, p->side, vector);
        }
        update_params (hWnd, vector);
//        set_access_mode (hWnd, vector);
    }
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
    ofn.lpstrFilter = is_fr?"Fichiers HFE\0*.hfe\0" \
                            "Fichiers SAP\0*.sap\0" \
                            "Fichiers disque bruts\0*.fd\0" \
                            "Fichiers QDD bruts\0*.qd\0" \
                            "Tous les fichiers\0*.*\0"
                           :"HFE files\0*.hfe\0" \
                            "SAP files\0*.sap\0" \
                            "Raw floppy files\0*.fd\0" \
                            "Raw QDD files\0*.qd\0" \
                            "All files\0*.*\0";
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
        if (load_virtual_disk (hWnd, ofn.lpstrFile, vector) >= 0)
        {
            add_combo_entry (hWnd, teo.disk[vector->id].file, vector);
            reset_side_combo (hWnd, teo.disk[vector->id].side, vector);
            teo.default_folder = std_free (teo.default_folder);
            teo.default_folder = std_strdup_printf ("%s", teo.disk[vector->id].file);
            (void)snprintf (current_file, MAX_PATH, "%s", teo.disk[vector->id].file);
            update_params (hWnd, vector);
        }
    }
}


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
   int i;
   int state;
   HANDLE himg;
   struct DISK_VECTOR *slist;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         for (i=0; i<NBDRIVE; i++)
         {
             if (first)
             {
                 /* initialisation du sélecteur de disquettes */
                 memset (&vector[i], 0x00, sizeof (struct FILE_VECTOR));
                 vector[i].id=i;
                 vector[i].direct=(direct_disk_support>>i)&1;
                 init_combo (hWnd, &vector[i]);
                 if (teo.disk[i].file != NULL)
                 {
                     add_combo_entry (hWnd, teo.disk[i].file, &vector[i]);
                     vector[i].current_dir = std_strdup_printf ("%s",
                                                         teo.disk[i].file);
                 }
//                 set_access_mode (hWnd, &vector[drive]);
                 update_params (hWnd, &vector[i]);
             }
             /* initialisation du combo des noms de fichiers */
             SendDlgItemMessage(hWnd, DISK0_COMBO+i, CB_RESETCONTENT, 0, 0);
             for (slist=vector[i].path_list; slist!=NULL; slist=slist->next)
                 SendDlgItemMessage(hWnd, DISK0_COMBO+i, CB_ADDSTRING, 0,
                                    (LPARAM) std_BaseName(slist->str));
             SendDlgItemMessage(hWnd, DISK0_COMBO+i, CB_SETCURSEL,
                                vector[i].combo_index, 0);

             /* initialisation du combo des faces de disquettes */
             reset_side_combo (hWnd, teo.disk[i].side, &vector[i]);

             /* initialisation de la protection */
             state = (teo.disk[i].write_protect == TRUE) ? BST_CHECKED : BST_UNCHECKED;
             CheckDlgButton(hWnd, DISK0_PROT_CHECK+i, state);

             /* initialisation des images */
             himg=LoadImage (prog_inst, "empty_ico",IMAGE_ICON, 0, 0,
                             LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, DISK0_EJECT_BUTTON+i),
                           BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);
             himg=LoadImage (prog_inst, "open_ico",IMAGE_ICON, 0, 0,
                             LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, DISK0_MORE_BUTTON+i),
                           BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);

             /* initialisation des textes */
             SetWindowText(GetDlgItem(hWnd, DISK0_SIDE_RTEXT), is_fr?"face":"side");
             SetWindowText(GetDlgItem(hWnd, DISK1_SIDE_RTEXT), is_fr?"face":"side");
             SetWindowText(GetDlgItem(hWnd, DISK2_SIDE_RTEXT), is_fr?"face":"side");
             SetWindowText(GetDlgItem(hWnd, DISK3_SIDE_RTEXT), is_fr?"face":"side");

             /* initialisation des info-bulles */
             wgui_CreateTooltip (hWnd, DISK0_EJECT_BUTTON+i,
                                 is_fr?"Vider la liste des fichiers"
                                      :"Empty the file list");
             wgui_CreateTooltip (hWnd, DISK0_MORE_BUTTON+i,
                                 is_fr?"Ouvrir un fichier disquette"
                                      :"Open a disk file");
             update_params (hWnd, &vector[i]);
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
               reset_combo(hWnd, &vector[LOWORD(wParam)-DISK0_EJECT_BUTTON]);
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
                   combo_changed(hWnd, &vector[LOWORD(wParam)-DISK0_COMBO]);
               break;

            case DISK0_SIDE_COMBO:
            case DISK1_SIDE_COMBO:
            case DISK2_SIDE_COMBO:
            case DISK3_SIDE_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
                   side_combo_changed(hWnd, &vector[LOWORD(wParam)-DISK0_SIDE_COMBO]);
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
