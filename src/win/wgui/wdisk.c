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
 *  Module     : win/wgui/wdisk.c
 *  Version    : 1.8.5
 *  Cr�� par   : Eric Botcazou 28/11/2000
 *  Modifi� par: Eric Botcazou 28/10/2003
 *               Fran�ois Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               24/10/2012 10/05/2014
 *               Samuel Cuella 02/2020
 *
 *  Gestion des disquettes.
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

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "main.h"
#include "errors.h"
#include "media/disk.h"
#include "media/disk/daccess.h"
#include "win/gui.h"
#include "gettext.h"

#define NDISKS 4

#define MIN(a,b)   (((a)<(b))?(a):(b))


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



/* load_virtual_disk:
 *  Charge une disquette.
 */
static int load_virtual_disk (HWND hWnd, char *filename, struct FILE_VECTOR *vector)
{
    int ret = disk_Load (vector->id, filename);

    switch (ret)
    {
        case TEO_ERROR :
            MessageBox(hWnd,
                       teo_error_msg,
                       PROGNAME_STR,
                       MB_OK | MB_ICONINFORMATION);
            break;

        case TRUE :
            CheckDlgButton(hWnd, IDC_DISK0_PROT_CHECK+vector->id, BST_CHECKED);
            teo.disk[vector->id].write_protect = TRUE;
            break;

        default : break;
    }
    return ret;
}



/* set_protection_check:
 *  Set the disk protection.
 */
static void set_protection_check (HWND hWnd, struct FILE_VECTOR *vector)
{
    struct DISK_VECTOR *disk_vector = NULL;
    int row;
    int protection;
    int state;

    row = ComboBox_GetCurSel(GetDlgItem (hWnd, IDC_DISK0_COMBO+vector->id));
    state = IsDlgButtonChecked(hWnd, IDC_DISK0_PROT_CHECK+vector->id);
    protection = (state == BST_CHECKED) ? TRUE : FALSE;

    if ((protection == FALSE)
     && (disk_Protection (vector->id, FALSE) == TRUE))
    {
        MessageBox(hWnd, _("Warning: writing unavailable on this device.")
                       , PROGNAME_STR, MB_OK | MB_ICONINFORMATION);
        CheckDlgButton(hWnd, IDC_DISK0_PROT_CHECK+vector->id, BST_CHECKED);
    }

    disk_vector = disk_DiskVectorPtr (vector->path_list, row);
    disk_vector->write_protect = protection;
    teo.disk[vector->id].write_protect = protection;
}



/* set_side_combo:
 *  Set the disk sides.
 */
static void set_side_combo (HWND hWnd, struct FILE_VECTOR *vector)
{
    struct DISK_VECTOR *disk_vector = NULL;
    int row;
    int side;
    HWND hwindow;

    hwindow = GetDlgItem (hWnd, IDC_DISK0_COMBO+vector->id);
    row = ComboBox_GetCurSel (hwindow);
    hwindow = GetDlgItem (hWnd,IDC_DISK0_SIDE_COMBO+vector->id);
    side = ComboBox_GetCurSel (hwindow);

    disk_vector = disk_DiskVectorPtr (vector->path_list, row);
    disk_vector->side = side;
    teo.disk[vector->id].side = side;
}



/* reset_side_combo:
 *  Reinitialize the combo for sides.
 */
static void reset_side_combo (HWND hWnd, int side_count, struct FILE_VECTOR *vector)
{
    int side = 0;
    char *str = NULL;

    SendDlgItemMessage (hWnd,
                        IDC_DISK0_SIDE_COMBO+vector->id,
                        CB_RESETCONTENT,
                        0,
                        0);
    for (side=0; side<side_count; side++)
    {
        str = std_strdup_printf ("%d", side);
        SendDlgItemMessage(hWnd,
                           IDC_DISK0_SIDE_COMBO+vector->id,
                           CB_ADDSTRING, 0,
                           (LPARAM)str);
        str = std_free (str);
    }
}


    
/* add_combo_entry:
 *  Ajoute une entr�e dans le combobox si inexistante et
 *  charge le fichier SAP correspondant si existante
 */
static void add_combo_entry (HWND hWnd, const char path[], int side, int side_count,
                             int protection, struct FILE_VECTOR *vector)
{
    int index = disk_DiskVectorIndex (vector->path_list, path);

    if (index >= 0)
    {
        SendDlgItemMessage(hWnd,
                           IDC_DISK0_COMBO+vector->id,
                           CB_SETCURSEL,
                           index,
                           0);
    }
    else
    {
        vector->path_list = disk_DiskVectorAppend (vector->path_list,
                                                   path,
                                                   side,
                                                   side_count,
                                                   protection);
        SendDlgItemMessage(hWnd,
                           IDC_DISK0_COMBO+vector->id,
                           CB_ADDSTRING,
                           0,
                           (LPARAM) std_BaseName((char *)path));
        SendDlgItemMessage(hWnd,
                           IDC_DISK0_COMBO+vector->id,
                           CB_SETCURSEL,
                           vector->entry_max,
                           0);
        vector->combo_index = vector->entry_max;
        vector->entry_max++;
    }
}



/* combo_changed:
 *  Changement de s�lection du combobox.
 */
static void combo_changed (HWND hWnd, struct FILE_VECTOR *vector)
{
    HWND hwindow;
    int active_row;
    struct DISK_VECTOR *p;

    active_row = SendDlgItemMessage(hWnd,
                                    IDC_DISK0_COMBO+vector->id,
                                    CB_GETCURSEL,
                                    0,
                                    0);
    p = disk_DiskVectorPtr (vector->path_list, active_row);

    vector->combo_index = active_row;
    if (active_row == 0)
    {
        hwindow = GetDlgItem (hWnd, IDC_DISK0_EJECT_BUTTON+vector->id);
        Button_Enable (hwindow, FALSE);
        hwindow = GetDlgItem (hWnd, IDC_DISK0_SIDE_COMBO+vector->id);
        ComboBox_Enable (hwindow, FALSE);
        hwindow = GetDlgItem (hWnd, IDC_DISK0_PROT_CHECK+vector->id);
        Button_Enable (hwindow, FALSE);
        disk_Eject(vector->id);
    }
    else
    if ((active_row == 1) && (vector->direct))
    {
        hwindow = GetDlgItem (hWnd, IDC_DISK0_EJECT_BUTTON+vector->id);
        Button_Enable (hwindow, FALSE);
        hwindow = GetDlgItem (hWnd, IDC_DISK0_SIDE_COMBO+vector->id);
        ComboBox_Enable (hwindow, FALSE);
        hwindow = GetDlgItem (hWnd, IDC_DISK0_PROT_CHECK+vector->id);
        Button_Enable (hwindow, TRUE);
        (void)daccess_LoadDisk (vector->id, "");
    }
    else
    {
        hwindow = GetDlgItem (hWnd, IDC_DISK0_EJECT_BUTTON+vector->id);
        Button_Enable (hwindow, TRUE);
        hwindow = GetDlgItem (hWnd, IDC_DISK0_SIDE_COMBO+vector->id);
        ComboBox_Enable (hwindow, TRUE);
        hwindow = GetDlgItem (hWnd, IDC_DISK0_PROT_CHECK+vector->id);
        Button_Enable (hwindow, TRUE);
        (void)load_virtual_disk (hWnd, p->str, vector);
    }
    reset_side_combo (hWnd, p->side_count, vector);
    SendDlgItemMessage(hWnd,
                       IDC_DISK0_SIDE_COMBO+vector->id,
                       CB_SETCURSEL,
                       p->side,
                       0);
    set_side_combo (hWnd, vector);
    CheckDlgButton(hWnd,
                   IDC_DISK0_PROT_CHECK+vector->id,
                   (p->write_protect)?BST_CHECKED:BST_UNCHECKED);
    set_protection_check (hWnd, vector);
}



/* free_disk_entry:
 *  Lib�re la m�moire utilis�e par la liste des disquettes.
 */
static void free_disk_entry (struct FILE_VECTOR *vector)
{
    disk_DiskVectorFree (vector->path_list);
    vector->path_list=NULL;
    vector->entry_max = 0;
    vector->combo_index=0;
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (HWND hWnd, struct FILE_VECTOR *vector)
{
    add_combo_entry (hWnd, _("(None)"), 0, 1, FALSE, vector);
    if (vector->direct)
        add_combo_entry (hWnd,
                         _("(Direct Access)"),
                         vector->id,
                         vector->id+1,
                         TRUE,
                         vector);
}



/* emptying_button_clicked:
 *  Ejecte la disquette et vide le combobox.
 *  (sauf l'entr�e "aucune cartouche" et "direct access") 
 */
static void emptying_button_clicked (HWND hWnd, struct FILE_VECTOR *vector)
{
     disk_Eject(vector->id);
     free_disk_entry (vector);
     SendDlgItemMessage(hWnd,
                        IDC_DISK0_COMBO+vector->id,
                        CB_RESETCONTENT,
                        0,
                        0);
     init_combo (hWnd, vector);
     combo_changed (hWnd, vector);
}



/* open_file:
 *  Charge une nouvelle disquette.
 */
static void open_file (HWND hWnd, struct FILE_VECTOR *vector)
{
    static char current_file[MAX_PATH+1]="";
    char def_folder[] = ".\\disk";
    OPENFILENAME ofn;

    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    /*Gettext can't handle in-string \0. They are encoded
     * as \a(bell) and in-place converted to suit format specified
     * for OPENFILENAME lpstrFilter*/
    char *_filter = strdup(_("All files\a*.*\aHFE files\a*.hfe\aSAP files\a*.sap\aRaw floppy files\a*.fd\a"));
    int filter_len = strlen(_filter);
    for(int i = 0; i < filter_len; i++){
        if(_filter[i] == '\a')
            _filter[i] = '\0';
    }
    ofn.lpstrFilter = _filter;

    ofn.nFilterIndex = 1;
    ofn.lpstrFile = current_file;
    ofn.nMaxFile = BUFFER_SIZE;
    ofn.lpstrTitle = _("Select your disk:");
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
            add_combo_entry (hWnd, teo.disk[vector->id].file,
                             MIN (teo.disk[vector->id].side,
                                  disk[vector->id].side_count-1),
                             disk[vector->id].side_count,
                             teo.disk[vector->id].write_protect,
                             vector);
            combo_changed(hWnd, vector);
            teo.default_folder = std_free (teo.default_folder);
            teo.default_folder = std_strdup_printf ("%s",
                                                    teo.disk[vector->id].file);
            (void)snprintf (current_file,
                            MAX_PATH,
                            "%s",
                            teo.disk[vector->id].file);
        }
    }
   free(_filter);
}


/* ------------------------------------------------------------------------- */


/* wdisk_Free:
 *  Lib�re la m�moire utilis�e par les listes de disquettes.
 */
void wdisk_Free (void)
{
    int i;

    for (i=0; i<NBDRIVE; i++)
        free_disk_entry (&vector[i]);
}



/* wdisk_TabProc:
 *  Proc�dure pour l'onglet des disquettes.
 */
int CALLBACK wdisk_TabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static int first=1;
   int i;
//   int state;
   int index;
   HANDLE himg;
   HWND hw;
   struct DISK_VECTOR *slist;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         for (i=0; i<NBDRIVE; i++)
         {
             /* initialisation des images */
             himg=LoadImage (prog_inst,
                             "clearlst_ico",
                             IMAGE_ICON,
                             0,
                             0,
                             LR_DEFAULTCOLOR);
             hw = GetDlgItem(hWnd, IDC_DISK0_EJECT_BUTTON+i);
             SendMessage(hw, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)himg);

             himg=LoadImage (prog_inst,
                             "folder_ico",
                             IMAGE_ICON,
                             0,
                             0,
                             LR_DEFAULTCOLOR);
             SendMessage(GetDlgItem(hWnd, IDC_DISK0_MORE_BUTTON+i),
                         BM_SETIMAGE,
                         (WPARAM)IMAGE_ICON,
                         (LPARAM)himg);

             /* initialisation des textes */
             SetWindowText(GetDlgItem(hWnd, IDC_DISK0_SIDE_RTEXT),
                           _("side"));
             SetWindowText(GetDlgItem(hWnd, IDC_DISK1_SIDE_RTEXT),
                           _("side"));
             SetWindowText(GetDlgItem(hWnd, IDC_DISK2_SIDE_RTEXT),
                           _("side"));
             SetWindowText(GetDlgItem(hWnd, IDC_DISK3_SIDE_RTEXT),
                           _("side"));

             /* initialisation des info-bulles */
             wgui_CreateTooltip (hWnd, IDC_DISK0_EJECT_BUTTON+i,
                                 _("Clear the file list"));
             wgui_CreateTooltip (hWnd, IDC_DISK0_MORE_BUTTON+i,
                                 _("Open a disk file"));

             if (first)
             {
                 /* initialisation du s�lecteur de disquettes */
                 memset (&vector[i], 0x00, sizeof (struct FILE_VECTOR));
                 vector[i].id=i;
                 vector[i].direct=(direct_disk_support>>i)&1;
             }

             /* initialisation du combo des noms de fichiers */
             SendDlgItemMessage(hWnd, IDC_DISK0_COMBO+i, CB_RESETCONTENT, 0, 0);
             for (slist=vector[i].path_list; slist!=NULL; slist=slist->next)
                 SendDlgItemMessage(hWnd, IDC_DISK0_COMBO+i, CB_ADDSTRING, 0,
                                    (LPARAM) std_BaseName(slist->str));

#if 0
             /* initialisation du combo des faces de disquettes */
             reset_side_combo (hWnd, teo.disk[i].side, &vector[i]);

             /* initialisation de la protection */
             state = (teo.disk[i].write_protect == TRUE) ? BST_CHECKED
                                                         : BST_UNCHECKED;
             CheckDlgButton(hWnd, IDC_DISK0_PROT_CHECK+i, state);
#endif
             if (first)
             {
                 init_combo (hWnd, &vector[i]);
                 if (teo.disk[i].file != NULL)
                 {
                     add_combo_entry (hWnd,
                                      teo.disk[i].file,
                                      teo.disk[i].side,
                                      disk[i].side_count,
                                      teo.disk[i].write_protect,
                                      &vector[i]);
                     vector[i].current_dir = std_strdup_printf ("%s",
                                                         teo.disk[i].file);
                 }
             }
             hw = GetDlgItem(hWnd, IDC_DISK0_COMBO+i);
             ComboBox_SetCurSel (hw, vector[i].combo_index);
             combo_changed(hWnd, &vector[i]);
         }
         first=0;
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDC_DISK0_EJECT_BUTTON:
            case IDC_DISK1_EJECT_BUTTON:
            case IDC_DISK2_EJECT_BUTTON:
            case IDC_DISK3_EJECT_BUTTON: 
               index = LOWORD(wParam)-IDC_DISK0_EJECT_BUTTON;
               emptying_button_clicked (hWnd, &vector[index]);
               break;

            case IDC_DISK0_MORE_BUTTON:
            case IDC_DISK1_MORE_BUTTON:
            case IDC_DISK2_MORE_BUTTON:
            case IDC_DISK3_MORE_BUTTON: 
               index = LOWORD(wParam)-IDC_DISK0_MORE_BUTTON;
               open_file (hWnd, &vector[index]);
               break;

            case IDC_DISK0_COMBO:
            case IDC_DISK1_COMBO:
            case IDC_DISK2_COMBO:
            case IDC_DISK3_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
               {
                   index = LOWORD(wParam)-IDC_DISK0_COMBO;
                   combo_changed (hWnd, &vector[index]);
               }
               break;

            case IDC_DISK0_SIDE_COMBO:
            case IDC_DISK1_SIDE_COMBO:
            case IDC_DISK2_SIDE_COMBO:
            case IDC_DISK3_SIDE_COMBO:
               if (HIWORD(wParam)==CBN_SELCHANGE)
               {
                   index = LOWORD(wParam)-IDC_DISK0_SIDE_COMBO;
                   set_side_combo (hWnd, &vector[index]);
               }
               break;

            case IDC_DISK0_PROT_CHECK:
            case IDC_DISK1_PROT_CHECK:
            case IDC_DISK2_PROT_CHECK:
            case IDC_DISK3_PROT_CHECK: 
               index = LOWORD(wParam)-IDC_DISK0_PROT_CHECK;
               set_protection_check (hWnd, &vector[index]);
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}
