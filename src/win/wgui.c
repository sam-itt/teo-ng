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
#include "win/dialog.rh"
#include "win/gui.h"
#include "to8.h"

/* ressources globales de l'application */
#define NBTABS_MASTER 4
HINSTANCE prog_inst;
HWND prog_win;
HICON prog_icon;
HWND hTab[NBTABS_MASTER];

/* le support des listes associées aux comboboxes de l'interface est assuré par une
   implémentation réduite en Standard C du container vector du Standard C++ */
#define CHUNK_SIZE   5

static int nCurrentTab = 0;



/* stringlist_last:
 *  Renvoit le pointeur sur le dernier élément de la stringlist.
 */
static struct STRING_LIST *stringlist_last (struct STRING_LIST *p)
{
    for (; p!=NULL; p=p->next)
        if (p->next==NULL)
            break;
    return p;
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
         return TRUE;

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



/* stringlist_index:
 *  Renvoit l'index de l'élément de la stringlist.
 */
int stringlist_index (struct STRING_LIST *p, char *str)
{
    int index;

    for (index=0; p!=NULL; p=p->next,index++)
        if (p->str!=NULL)
            if (strcmp (p->str, str) == 0)
                break;
    return (p==NULL)?-1:index;
}



/* stringlist_text:
 *  Renvoit le pointeur du texte de l'élément de la stringlist.
 */
char *stringlist_text (struct STRING_LIST *p, int index)
{
    for (;index>0;index--)
    {
        if (p!=NULL)
            p=p->next;
    }
    return (p!=NULL)?p->str:NULL;
}



/* stringlist_append:
 *  Ajoute un élément à la stringlist.
 */
struct STRING_LIST *stringlist_append (struct STRING_LIST *p, char *str)
{
    struct STRING_LIST *last_str = stringlist_last (p);
    struct STRING_LIST *new_str = calloc (1, sizeof (struct STRING_LIST));

    if (new_str!=NULL)
    {
        new_str->str=malloc (strlen (str)+1);
        if (new_str->str!=NULL)
        {
            *new_str->str='\0';
            strcpy (new_str->str, str);
        }
    }
    if ((last_str!=NULL) && (last_str->str!=NULL))
        last_str->next=new_str;

    return (p==NULL)?new_str:p;
}



/* stringlist_free:
 *  Libère la mémoire de la stringlist.
 */
void stringlist_free (struct STRING_LIST *p)
{
    struct STRING_LIST *next;

    while (p!=NULL)
    {
        next=p->next;
        if (p->str!=NULL)
            free (p->str);
        free (p);
        p=next;
    }
}



/* WGUI_basename:
 *  Retourne le nom du fichier à partir du nom complet du fichier spécifié.
 */
const char* basename_ptr(const char fullname[])
{
   int len = strlen(fullname);

   while (--len > 0)
      if (fullname[len] == '\\')
         return fullname + len + 1;

   return fullname;
}



/* create_tooltip:
 *  Crée une info-bulle.
 */
void create_tooltip (HWND hWnd, WORD id, char *text)
{
    RECT rect;
    TOOLINFO ti;
    HWND hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
                            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            hWnd, NULL, prog_inst,
                            NULL);

    GetClientRect (hWnd, &rect);

    memset (&ti, 0x00, sizeof(TOOLINFO));
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = GetDlgItem(hWnd, id);
    ti.hinst = prog_inst;
    ti.lpszText = text;
    ti.rect.left = rect.left;
    ti.rect.top = rect.top;
    ti.rect.right = rect.right;
    ti.rect.bottom = rect.bottom;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
}



/* FreeGUI:
 *  Libère la mémoire utilisée par l'interface
 */
void FreeGUI (void)
{
    free_memo_list ();
    free_cass_list ();
    free_disk_list ();
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
