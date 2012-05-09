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
 *  Module     : win/gui.h
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 04/12/2000
 *               François Mouret 01/04/2012
 *
 *  Interface utilisateur Windows native.
 */


#ifndef WIN_GUI_H
#define WIN_GUI_H

#ifndef SCAN_DEPEND
   #include <windows.h>
#endif

#define PROGNAME_STR  "Teo"
#define BUFFER_SIZE  512
#define NOT_FOUND   -1

extern HINSTANCE prog_inst;
extern HWND prog_win;
extern HICON prog_icon;

extern void SelectGraphicMode(int *, int *);
extern void DisplayControlPanelWin(void);

extern int  CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern int  CALLBACK SettingTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern int  CALLBACK CassetteTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern int  CALLBACK CartridgeTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern int  CALLBACK DiskTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern int  CALLBACK PrinterTabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK StartDialogProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);

extern void free_cass_list (void);
extern void free_memo_list (void);
extern void free_disk_list (void);
extern void FreeGUI (void);

extern void create_tooltip (HWND hWnd, WORD id, char *text);

#endif
