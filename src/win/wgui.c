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

#include "alleg/gfxdrv.h"
#include "alleg/main.h"
#include "alleg/sound.h"
#include "win/dialog.rh"
#include "win/license.h"
#include "to8.h"

/* Nom du programme pour les boîtes de dialogue. */
#define PROGNAME_STR  "Teo"


/* ressources globales de l'application */
#define NBTABS_MASTER 4
HINSTANCE prog_inst;
HWND prog_win;
HICON prog_icon;
HWND hTab[NBTABS_MASTER];

/* le support des listes associées aux comboboxes de l'interface est assuré par une
   implémentation réduite en Standard C du container vector du Standard C++ */

#define CHUNK_SIZE   5
#define NOT_FOUND   -1
#define BUFFER_SIZE  512

/* pair<string, string> */
struct STRING_PAIR {
    char *fullname;
    char *label;
};

/* vector< pair<string, string> > */
struct FILE_VECTOR {
    int id;
    int size;
    int capacity;
    int selected;
    int protection;
    struct STRING_PAIR *file;   
};

#ifdef OS_LINUX
extern int SetInterlaced(int);
#else
extern int  (*SetInterlaced)(int);
#endif

static int nCurrentTab = 0;

/* reset_vector:
 *  Efface les entrées périphériques
 */
static void reset_vector(struct FILE_VECTOR *vector)
{
    vector->size = 0;
    vector->capacity = 0;
    vector->selected = 0;
    if (vector->file != 0) {
        free (vector->file);
        vector->file = NULL;
    }
}


/* vector< pair<string, string> >::push_back( make_pair(string(), string()) ):
 *  Ajoute une paire de chaînes au vecteur spécifié en l'agrandissant si nécessaire.
 */
static int push_back(struct FILE_VECTOR *vector, const char fullname[], const char label[])
{
   if (vector->size == vector->capacity)
   {
       /* on alloue un espace mémoire plus important */
       struct STRING_PAIR *temp;
       vector->capacity += CHUNK_SIZE;
       temp = malloc( vector->capacity*sizeof(struct STRING_PAIR) );
       memcpy(temp, vector->file, vector->size*sizeof(struct STRING_PAIR));
       free(vector->file);
       vector->file = temp;
   }

   vector->file[vector->size].fullname = malloc( (strlen(fullname)+1)*sizeof(char) );
   strcpy(vector->file[vector->size].fullname, fullname);

   vector->file[vector->size].label = malloc( (strlen(label)+1)*sizeof(char) );
   strcpy(vector->file[vector->size].label, label);

   vector->size++;

   return (vector->size-1);
}



/* find:
 *  Cherche un nom de fichier dans un vecteur et retourne le cas échéant son index.
 */
static int find(struct FILE_VECTOR *vector, const char fullname[])
{
   int i;

   for (i=0; i<vector->size; i++)
      if (!strcmp(vector->file[i].fullname, fullname))
         return i;

   return NOT_FOUND;
}



/* extract_dir:
 *  Extrait le nom du répertoire du nom complet du fichier spécifié.
 */
static void extract_dir(char dir[], const char fullname[])
{
   int len = strlen(fullname);

   strcpy(dir, fullname);

   while (--len > 0)
      if (dir[len] == '\\')
      {
         dir[len] = '\0';
         break;
      }
}
 


/* get_filename:
 *  Retourne le nom du fichier à partir du nom complet du fichier spécifié.
 */
static const char* get_filename(const char fullname[])
{
   int len = strlen(fullname);

   while (--len > 0)
      if (fullname[len] == '\\')
         return fullname + len + 1;

   return fullname;
}



/***********************************************/
/* méthodes du vecteur associé à la cartouche  */
/***********************************************/


static void init_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
   static int first = 1;
   const char *str_p;
   int i;

   if (first)
   {
      str_p = to8_GetMemo7Filename();
      if (*str_p)
         push_back(vector, str_p, to8_GetMemo7Label());

      first = 0;
   }

   if (vector->size)
   {
      for (i=0; i<vector->size; i++)
         SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) vector->file[i].label);
   }
   else
      SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) (is_fr?"aucune cartouche":"no cartridge"));

   SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_SETCURSEL, vector->selected, 0);
}


static void open_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
   OPENFILENAME openfilename;
   char buffer[BUFFER_SIZE], dir[BUFFER_SIZE];
   int index;

   /* initialisation du sélecteur de fichiers */
   memset(&openfilename, 0, sizeof(OPENFILENAME));

   openfilename.lStructSize = sizeof(OPENFILENAME);
   openfilename.hwndOwner = hDlg;
   openfilename.lpstrFilter = is_fr?"Fichiers M7\0*.m7\0":"M7 files\0*.m7\0";
   openfilename.nFilterIndex = 1;
   openfilename.lpstrFile = buffer;
   openfilename.nMaxFile = BUFFER_SIZE;

   if (vector->size)
   {
      extract_dir(dir, vector->file[vector->selected].fullname);
      openfilename.lpstrInitialDir = dir;
   }
   else
      openfilename.lpstrInitialDir = ".\\memo7";

   openfilename.lpstrTitle = is_fr?"Choisissez votre cartouche:":"Choose your cartridge:";
   openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
   openfilename.lpstrDefExt ="m7";

   buffer[0] = '\0';

   if (GetOpenFileName(&openfilename))
   {
      if (to8_LoadMemo7(openfilename.lpstrFile))
      {        
         index = find(vector, openfilename.lpstrFile);
                         
         if (index == NOT_FOUND)
         {
            index = push_back(vector, openfilename.lpstrFile, to8_GetMemo7Label());

            if (!index)
               SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_DELETESTRING, 0, 0);
                             
            SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_ADDSTRING, 0, (LPARAM) vector->file[index].label);
         }

         SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_SETCURSEL, index, 0);
         vector->selected = index;
         teo.command = COLD_RESET;
      }
      else
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
   }
}


static void select_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
   int index;

   if (vector->size)
   {
      index = SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_GETCURSEL, 0, 0);

      if (to8_LoadMemo7(vector->file[index].fullname))
      {
         vector->selected= index;
         teo.command = COLD_RESET;
      }
      else
      {
         MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
         SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_SETCURSEL, vector->selected, 0);
      }
   }
}


static void eject_file_memo(struct FILE_VECTOR *vector, HWND hDlg)
{
    SendDlgItemMessage(hDlg, MEMO7_COMBO, CB_RESETCONTENT, 0, 0);
    reset_vector (vector);
    init_file_memo(vector, hDlg);
    to8_EjectMemo7();
    teo.command = COLD_RESET;
}         


/***********************************************/
/* méthodes du vecteur associé à la cassette   */
/***********************************************/


static void init_file_cass(struct FILE_VECTOR *vector, HWND hDlg)
{
   static int first = 1;
   const char *str_p;
   int i;

   if (first)
   {
      str_p = to8_GetK7Filename();
      if (*str_p)
         push_back(vector, str_p, get_filename(str_p));

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
      extract_dir(dir, vector->file[vector->selected].fullname);
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
         index = find(vector, openfilename.lpstrFile);

         if (index == NOT_FOUND)
         {
            index = push_back(vector, openfilename.lpstrFile, openfilename.lpstrFile + openfilename.nFileOffset);

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
    reset_vector (vector);
    vector->protection = TRUE;
    init_file_cass(vector, hDlg);
    init_check_cass(vector, hDlg);
    to8_EjectK7();
    update_counter_cass(hDlg);
}



/***********************************************/
/* méthodes du vecteur associé à la disquette  */
/***********************************************/


static void init_file_disk(struct FILE_VECTOR *vector, HWND hDlg)
{
   static int first = 4;
   const char *str_p;
   int i;

   if (first)
   {
      str_p = to8_GetDiskFilename(vector->id);
      if (*str_p)
         vector->selected = push_back(vector, str_p, get_filename(str_p));

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
      extract_dir(dir, vector->file[vector->selected].fullname);
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
         index = find(vector, openfilename.lpstrFile);

         if (index == NOT_FOUND)
         {
            index = push_back(vector, openfilename.lpstrFile, openfilename.lpstrFile + openfilename.nFileOffset);

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
    reset_vector (vector);
    init_file_disk(vector, hDlg);
    to8_EjectDisk(vector->id);
}


/***********************************************/
/* méthodes de la barre du volume              */
/***********************************************/

#define MIN_POS    0
#define MAX_POS    254
#define LINE_STEP  2
#define PAGE_STEP  16


static void init_bar(HWND volume_bar)
{
   SendMessage(volume_bar, TBM_SETRANGE, TRUE, MAKELONG(MIN_POS, MAX_POS));
   SendMessage(volume_bar, TBM_SETLINESIZE, TRUE, LINE_STEP);
   SendMessage(volume_bar, TBM_SETPAGESIZE, TRUE, PAGE_STEP);
   SendMessage(volume_bar, TBM_SETTICFREQ, PAGE_STEP, 0);

   SendMessage(volume_bar, TBM_SETPOS, TRUE,
      (teo.sound_enabled && teo.exact_speed) ? GetVolume()-1 : (MAX_POS-MIN_POS)/2);
   
   if (!teo.sound_enabled || !teo.exact_speed)
      EnableWindow(volume_bar, FALSE);
}


static void update_bar(HWND volume_bar, WPARAM wParam)
{
   int pos = GetVolume()-1;

   switch(LOWORD(wParam))
   {
      case TB_TOP:
         pos = MIN_POS;
         break;

      case TB_LINEUP:
         pos -= LINE_STEP;
         break;

      case TB_PAGEUP:
         pos -= PAGE_STEP;
         if (pos<MIN_POS)
            pos = MIN_POS;
         break;

      case TB_BOTTOM:
         pos = MAX_POS;
         break;

      case TB_LINEDOWN:
         pos += LINE_STEP;
         break;

      case TB_PAGEDOWN:
         pos += PAGE_STEP;
         if (pos>MAX_POS)
            pos = MAX_POS;
         break;           

      case TB_THUMBPOSITION:
      case TB_THUMBTRACK:
         pos = HIWORD(wParam);
         break;
   }

   SetVolume(pos+1);
}



static int get_open_image_name(HWND hDlg, char filename[BUFFER_SIZE], int must_exist)
{
   OPENFILENAME openfilename;

   /* initialisation du sélecteur de fichiers */
   memset(&openfilename, 0, sizeof(OPENFILENAME));

   openfilename.lStructSize = sizeof(OPENFILENAME);
   openfilename.hwndOwner = hDlg;
   openfilename.lpstrFilter = is_fr?"Fichiers IMG\0*.img\0":"IMG files\0*.img\0";
   openfilename.nFilterIndex = 1;
   openfilename.lpstrFile = filename;
   openfilename.nMaxFile = BUFFER_SIZE;
   openfilename.lpstrInitialDir = ".";

   if (must_exist)
      openfilename.lpstrTitle = is_fr?"Choisissez votre image:":"Choose your image:";
   else
      openfilename.lpstrTitle = is_fr?"Spécifiez un nom pour votre image:":"Specify a name for your image";

   openfilename.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
   if (must_exist)
      openfilename.Flags |= OFN_FILEMUSTEXIST;

   openfilename.lpstrDefExt = "img";

   filename[0] = '\0';

   return GetOpenFileName(&openfilename);
}
   


/* ShowTab:
 * Masque l'onglet actif et affiche l'onglet demandé
 */
static void
ShowTab(HWND hDlg)
{
    ShowWindow(hTab[nCurrentTab], SW_HIDE);
    nCurrentTab = SendMessage(GetDlgItem(hDlg, CONTROL_TAB), TCM_GETCURSEL, 0, 0);
    ShowWindow(hTab[nCurrentTab], SW_SHOW);
}



/* CreateTab:
 * Crée un onglet
 */
static HWND
CreateTab(HWND hDlg, WORD number, char *title, WORD id, int (CALLBACK *prog)(HWND, UINT, WPARAM, LPARAM))
{
    RECT rect0;
    RECT rect1;
    HWND hTabItem;
    HWND hMyTab;
    TCITEM tcitem;

    tcitem.mask = TCIF_TEXT;
    hTabItem = GetDlgItem(hDlg, CONTROL_TAB);

    /* création du dialogue enfant */
    hMyTab = CreateDialog(prog_inst, MAKEINTRESOURCE(id), hDlg, prog);

    /* ajout de l'onglet */
    tcitem.pszText = (LPTSTR)title;
    SendMessage(hTabItem, TCM_INSERTITEM, number, (LPARAM)&tcitem);

    /* définit le rectangle en rapport à la boîte de dialogue parente */
    GetWindowRect(hTabItem, &rect0);
    SendMessage(hTabItem, TCM_ADJUSTRECT, FALSE, (LPARAM)&rect0);
    GetWindowRect(hMyTab, &rect1);
    SetWindowPos(hMyTab, NULL, rect0.left-rect1.left,
                               rect0.top-rect1.top,
                               rect0.right-rect0.left,
                               rect0.bottom-rect0.top,
                               SWP_NOZORDER|SWP_NOREDRAW);
    /* masque l'onglet */
    ShowWindow(hMyTab, SW_HIDE);

    return hMyTab;
}



/* DiskTabProc:
 * Procédure pour l'onglet des disquettes
 */
static int
CALLBACK SettingTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static HWND volume_bar;
   static int interlaced = 0;
   char filename[BUFFER_SIZE];

   switch(uMsg)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANG
         SetWindowText(GetDlgItem(hDlg, SPEED_LTEXT), "Vitesse:");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exacte");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "maximale");
         SetWindowText(GetDlgItem(hDlg, IMAGE_GROUP), "Images:");
         SetWindowText(GetDlgItem(hDlg, LOAD_BUTTON), "Charger");
         SetWindowText(GetDlgItem(hDlg, SAVE_BUTTON), "Sauver");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Mode vidéo entrelacé");
         SetWindowText(GetDlgItem(hDlg, VOLUME_LTEXT), "Volume sonore:");
#else
         SetWindowText(GetDlgItem(hDlg, SPEED_LTEXT), "Speed:");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exact");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "fast");
         SetWindowText(GetDlgItem(hDlg, IMAGE_GROUP), "Images:");
         SetWindowText(GetDlgItem(hDlg, LOAD_BUTTON), "Load");
         SetWindowText(GetDlgItem(hDlg, SAVE_BUTTON), "Save");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Interlaced video");
         SetWindowText(GetDlgItem(hDlg, VOLUME_LTEXT), "Sound volume:");
#endif
         /* initialisation des boutons radio de la vitesse */
         CheckRadioButton(hDlg, EXACT_SPEED_BUTTON, MAX_SPEED_BUTTON, 
               (teo.exact_speed ? EXACT_SPEED_BUTTON : MAX_SPEED_BUTTON));

         /* initialisation du mode entrelacé */
         CheckDlgButton(hDlg, INTERLACED_CHECK, interlaced ? BST_CHECKED : BST_UNCHECKED);

         /* initialisation de la barre du volume */
         volume_bar = GetDlgItem(hDlg, VOLUME_BAR);
         init_bar(volume_bar);

         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case EXACT_SPEED_BUTTON:
               teo.exact_speed = TRUE;
               EnableWindow(volume_bar, TRUE);
               break;

            case MAX_SPEED_BUTTON:
               teo.exact_speed = FALSE;
               EnableWindow(volume_bar, FALSE);
               break;

            case INTERLACED_CHECK:
               SetInterlaced ((IsDlgButtonChecked(hDlg, INTERLACED_CHECK)) ? 1 : 0);
               interlaced = IsDlgButtonChecked(hDlg, INTERLACED_CHECK) ? 1 : 0;
               break;
               
            case LOAD_BUTTON:
               if (get_open_image_name(hDlg, filename, TRUE))
               {
                  if (to8_LoadImage(filename) == TO8_ERROR)
                     MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
               }
               break;

            case SAVE_BUTTON:
               if (get_open_image_name(hDlg, filename, FALSE))
               {
                  if (to8_SaveImage(filename) == TO8_ERROR)
                     MessageBox(hDlg, to8_error_msg, PROGNAME_STR, MB_OK | MB_ICONERROR);
               }
               break;
         }
         return TRUE;

      case WM_HSCROLL:
         if ((HWND)lParam == volume_bar)
         {
            if (teo.sound_enabled && teo.exact_speed)
               update_bar(volume_bar, wParam);
         }
         return TRUE;

      default:
         return FALSE;
   }
}



/* DiskTabProc:
 * Procédure pour l'onglet des disquettes
 */
static int
CALLBACK DiskTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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



/* CassetteTabProc:
 * Procédure pour l'onglet des cassettes
 */
static int
CALLBACK CassetteTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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



/* CartridgeTabProc:
 * Procédure pour l'onglet des cartouches
 */
static int
CALLBACK CartridgeTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static struct FILE_VECTOR memo7;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         /* initialisation du sélecteur de cartouches */
         init_file_memo(&memo7, hDlg);

         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case MEMO7_EJECT_BUTTON:
               eject_file_memo(&memo7, hDlg);
               break;

            case MEMO7_MORE_BUTTON:
               open_file_memo(&memo7, hDlg);
               break;

            case MEMO7_COMBO:
               select_file_memo(&memo7, hDlg);
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}



/* AboutProc:
 * Procédure pour la boîte "A propos"
 */
static int
CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static HWND aboutLink;
   static HWND aboutTitle;
   static HWND aboutCopyright;
   static HWND aboutLicense;
   static HFONT hLinkStyle;
   static HFONT hTitleStyle;
   static HFONT hCopyrightStyle;
   static HFONT hLicenseStyle;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         aboutLink  = GetDlgItem (hDlg, ABOUT_STATIC_LINK);
         aboutTitle = GetDlgItem (hDlg, ABOUT_CTEXT_TITLE);
         aboutCopyright = GetDlgItem (hDlg, ABOUT_CTEXT_COPYRIGHT);
         aboutLicense = GetDlgItem (hDlg, ABOUT_EDIT_LICENSE);
         hTitleStyle = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
         hLinkStyle = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, TRUE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
         hCopyrightStyle = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
         hLicenseStyle = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH, "Courier New");
         SendMessage(aboutLink, WM_SETFONT, (WPARAM)hLinkStyle, TRUE);
         SendMessage(aboutTitle, WM_SETFONT, (WPARAM)hTitleStyle, TRUE);
         SendMessage(aboutLicense, WM_SETFONT, (WPARAM)hLicenseStyle, TRUE);
         SendMessage(aboutCopyright, WM_SETFONT, (WPARAM)hCopyrightStyle, TRUE);
         SetWindowText(aboutLicense, license_text);
#ifdef FRENCH_LANG
         SetWindowText(hDlg, "Teo - A propos");
         SetWindowText(aboutLink, "Teo sur le site Web de Nostalgies Thomsonistes");
#else
         SetWindowText(hDlg, "Teo - About");
         SetWindowText(aboutLink, "Teo on Nostalgies Thomsonistes Web site");
#endif
         SetClassLongPtr(aboutLink, GCLP_HCURSOR, (ULONG_PTR)LoadCursor(NULL, IDC_HAND));
         return TRUE;

      case WM_CTLCOLORSTATIC :
          if ((HWND)lParam == aboutLink)
          {
              SetTextColor((HDC)wParam, RGB(0, 0, 255));
              SetBkMode((HDC)wParam, TRANSPARENT);
              return (BOOL)GetStockObject(HOLLOW_BRUSH);
          }
          if ((HWND)lParam == aboutLicense)
          {
              SetTextColor((HDC)wParam, RGB(0, 0, 0));
              SetBkMode((HDC)wParam, OPAQUE);
              SetBkColor((HDC)wParam, RGB(255, 255, 255));
              return (BOOL)GetStockObject(HOLLOW_BRUSH);
          }
          return TRUE;

      case WM_CTLCOLOREDIT :
          return TRUE;

      case WM_DESTROY :
          (void)DeleteObject((HGDIOBJ)hLinkStyle);
          (void)DeleteObject((HGDIOBJ)hTitleStyle);
          (void)DeleteObject((HGDIOBJ)hCopyrightStyle);
          (void)DeleteObject((HGDIOBJ)hLicenseStyle);
          EndDialog(hDlg, IDOK);
          return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDOK:
               (void)DeleteObject((HGDIOBJ)hLinkStyle);
               (void)DeleteObject((HGDIOBJ)hTitleStyle);
               (void)DeleteObject((HGDIOBJ)hCopyrightStyle);
               (void)DeleteObject((HGDIOBJ)hLicenseStyle);
               EndDialog(hDlg, IDOK);
               break;

            case ABOUT_STATIC_LINK :
               ShellExecute(NULL, "open", "http://nostalgies.thomsonistes.org/teo_home.html", 0, 0, SW_SHOWNORMAL);
               break;

         }
         return TRUE;

      default:
         return FALSE;
   }
}



/* ControlDialogProc:
 *  Procédure du panneau de contrôle.
 */
static BOOL CALLBACK ControlDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LPNMHDR lpnmhdr;
   int i;

   switch(uMsg)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANG
         SetWindowText(hDlg, "Teo - Panneau de contrôle");
         SetWindowText(GetDlgItem(hDlg, QUIT_BUTTON), "Quitter");
         SetWindowText(GetDlgItem(hDlg, COMMANDS_LTEXT), "Commandes et Réglages");
         SetWindowText(GetDlgItem(hDlg, RESET_BUTTON), "Réinitialiser le TO8");
         SetWindowText(GetDlgItem(hDlg, COLDRESET_BUTTON), "Redémarrer à froid le TO8");
         SetWindowText(GetDlgItem(hDlg, SPEED_LTEXT), "Vitesse:");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exacte");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "maximale");
         SetWindowText(GetDlgItem(hDlg, IMAGE_GROUP), "Images");
         SetWindowText(GetDlgItem(hDlg, LOAD_BUTTON), "Charger");
         SetWindowText(GetDlgItem(hDlg, SAVE_BUTTON), "Sauver");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Mode vidéo entrelacé");
         SetWindowText(GetDlgItem(hDlg, VOLUME_LTEXT), "Volume sonore:");
         SetWindowText(GetDlgItem(hDlg, ABOUT_BUTTON), "A propos");
#else
         SetWindowText(hDlg, "Teo - Control panel");
         SetWindowText(GetDlgItem(hDlg, QUIT_BUTTON), "Quit");
         SetWindowText(GetDlgItem(hDlg, COMMANDS_LTEXT), "Commands and Settings");
         SetWindowText(GetDlgItem(hDlg, RESET_BUTTON), "TO8 warm reset");
         SetWindowText(GetDlgItem(hDlg, COLDRESET_BUTTON), "TO8 cold reset");
         SetWindowText(GetDlgItem(hDlg, SPEED_LTEXT), "  Speed:");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exact");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "fastest");
         SetWindowText(GetDlgItem(hDlg, IMAGE_GROUP), "Images");
         SetWindowText(GetDlgItem(hDlg, LOAD_BUTTON), "Load");
         SetWindowText(GetDlgItem(hDlg, SAVE_BUTTON), "Save");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Interlaced video");
         SetWindowText(GetDlgItem(hDlg, VOLUME_LTEXT), " Sound volume:");
         SetWindowText(GetDlgItem(hDlg, ABOUT_BUTTON), "About");
#endif
         /* Crée les onglets */
         hTab[0] = CreateTab(hDlg, 0, is_fr?"Réglage":"Setting", SETTING_TAB, SettingTabProc);
         hTab[1] = CreateTab(hDlg, 1, is_fr?"Disquette":"Disk", DISK_TAB, DiskTabProc);
         hTab[2] = CreateTab(hDlg, 2, is_fr?"Cassette":"Tape", K7_TAB, CassetteTabProc);
         hTab[3] = CreateTab(hDlg, 3, is_fr?"Cartouche":"Cartridge", MEMO7_TAB, CartridgeTabProc);

         /* Affiche l'onglet des disquettes */
         SendMessage(GetDlgItem(hDlg, CONTROL_TAB), TCM_SETCURSEL, nCurrentTab, 0);
         ShowTab(hDlg);

         /* mise en place de l'icône */
         SetClassLong(hDlg, GCL_HICON,   (LONG) prog_icon);
         SetClassLong(hDlg, GCL_HICONSM, (LONG) prog_icon);

         return TRUE;

      case WM_DESTROY :
         for(i=0;i<NBTABS_MASTER;i++)
            if (hTab[i] != NULL)
                DestroyWindow(hTab[i]);
         return FALSE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDOK:
               EndDialog(hDlg, IDOK);
               break;

            case QUIT_BUTTON:
               if (MessageBox(hDlg, is_fr?"Voulez-vous vraiment quitter l'émulateur ?"
                                         :"Do you really want to quit the emulator ?", "Teo - confirmation",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SYSTEMMODAL) == IDYES)
                  EndDialog(hDlg, IDCANCEL);
               break; 

            case ABOUT_BUTTON:
               (void)DialogBox (prog_inst, MAKEINTRESOURCE(ABOUT_DIALOG), hDlg, (DLGPROC)AboutProc);
               break;

            case RESET_BUTTON:
               teo.command = RESET;
               EndDialog(hDlg, IDOK);
               break;

            case COLDRESET_BUTTON:
               teo.command = COLD_RESET;
               EndDialog(hDlg, IDOK);
               break;

            case WM_DESTROY:
               EndDialog(hDlg, IDOK);
               break;
         }
         return TRUE;

      case WM_NOTIFY :
         /* Change d'onglet */
         lpnmhdr = (LPNMHDR)lParam;
         if(lpnmhdr->code == TCN_SELCHANGE)
             ShowTab(hDlg);
         return FALSE;

      default:
         return FALSE;
   }
}



/* DisplayControlPanelWin:
 *  Affiche le panneau de contrôle natif Windows.
 */
void DisplayControlPanelWin(void)
{
   static int first = 1;
   int ret;

   if (first)
   {
      /* initialise la librairie comctl32.dll */
      InitCommonControls();
      first = 0;
   }

   ret = DialogBox(prog_inst, "CONTROL_DIALOG", prog_win, ControlDialogProc);

   if (ret == IDCANCEL) 
      teo.command = QUIT;
}



/* StartDialogProc:
 *  Procédure de la boîte de dialogue de démarrage.
 */
static BOOL CALLBACK StartDialogProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
   int ret;

   switch(Message)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANG
         SetWindowText(hDlg, "Teo - l'émulateur TO8");
         SetWindowText(GetDlgItem(hDlg, START_LTEXT), 
                    "Teo (Windows/DirectX)\r\n"\
                    "Version "\
                    TO8_VERSION_STR\
                    "\r\n"\
                    "Copyright © 1997-2012  Gilles Fétis, Eric Botcazou,\r\n"\
                    "Alexandre Pukall, Jérémie Guillaume, François Mouret,\r\n"\
                    "Samuel Devulder\r\n\n"\
                    "Ce programme est un logiciel libre; vous pouvez le redistri-\r\nbuer "\
                    "et/ou le modifier selon les termes de la GNU General Public License; "\
                    "version 2 de la licence, ou (à votre conve-\r\nnance) toute version ultérieure.");
         SetWindowText(GetDlgItem(hDlg, GFXMODE_GROUP), "Mode graphique");
         SetWindowText(GetDlgItem(hDlg, MODE40_BUTTON), "Plein écran 40 colonnes 16 couleurs (rapide)");
         SetWindowText(GetDlgItem(hDlg, MODE80_BUTTON), "Plein écran 80 colonnes 16 couleurs (médium)");
         SetWindowText(GetDlgItem(hDlg, TRUECOLOR_BUTTON), "Plein écran 80 colonnes 4096 couleurs (lent)");
         SetWindowText(GetDlgItem(hDlg, WINDOWED_BUTTON), "Fenêtré 80 colonnes 4096 couleurs");
#else
         SetWindowText(hDlg, "Teo - the TO8 emulator");
         SetWindowText(GetDlgItem(hDlg, START_LTEXT), 
                   "Teo (Windows/DirectX)\r\n"\
                    "Version "\
                    TO8_VERSION_STR\
                    "\r\n"\
                    "Copyright © 1997-2012  Gilles Fétis, Eric Botcazou,\r\n"\
                    "Alexandre Pukall, Jérémie Guillaume, François Mouret,\r\n"\
                    "Samuel Devulder\r\n\n"\
                    "This program is free software; you can redistribute it and/or modify "\
                    "it under the terms of the GNU General Public License; either version "\
                    "2 of the License, or (at your option) any later version.");
         SetWindowText(GetDlgItem(hDlg, GFXMODE_GROUP), "Graphic mode");
         SetWindowText(GetDlgItem(hDlg, MODE40_BUTTON), "Full screen 40 columns 16 colors (fast)");
         SetWindowText(GetDlgItem(hDlg, MODE80_BUTTON), "Full screen 80 columns 16 colors (medium)");
         SetWindowText(GetDlgItem(hDlg, TRUECOLOR_BUTTON), "Full screen 80 columns 4096 colors (slow)");
         SetWindowText(GetDlgItem(hDlg, WINDOWED_BUTTON), "Windowed 80 columns 4096 colors");
#endif
         /* mise en place de l'icône */
         SetClassLong(hDlg, GCL_HICON,   (LONG) prog_icon);
         SetClassLong(hDlg, GCL_HICONSM, (LONG) prog_icon);

         CheckRadioButton(hDlg, MODE40_BUTTON, WINDOWED_BUTTON, WINDOWED_BUTTON);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDOK:
               if (IsDlgButtonChecked(hDlg, MODE40_BUTTON))
                  ret = GFX_MODE40;
               else if (IsDlgButtonChecked(hDlg, MODE80_BUTTON))
                  ret = GFX_MODE80;
               else if (IsDlgButtonChecked(hDlg, TRUECOLOR_BUTTON))
                  ret = GFX_TRUECOLOR;
               else
                  ret = GFX_WINDOW;

               EndDialog(hDlg, ret);
               break;

            case WM_DESTROY:
               EndDialog(hDlg, NO_GFX);
               exit(EXIT_SUCCESS);
               break;
         }
         return TRUE;

      default:
         return FALSE;
   }
}



/* SelectGraphicMode:
 *  Affiche la boîte de dialogue de démarrage et permet 
 *  à l'utilisateur de choisir le mode graphique.
 */
void SelectGraphicMode(int *graphics_mode, int *ctrl_pressed)
{
   *graphics_mode = DialogBox(prog_inst, "START_DIALOG", NULL, StartDialogProc);

   if (ctrl_pressed)
      *ctrl_pressed = GetKeyState(VK_CONTROL)&0x8000;
}

