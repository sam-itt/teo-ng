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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 04/12/2000
 *               François Mouret 01/04/2012 10/05/2014
 *
 *  Interface utilisateur Windows native.
 */


#ifndef WIN_GUI_H
#define WIN_GUI_H

/* Windows includes */
#ifndef SCAN_DEPEND
   #include <windows.h>
   #include <windowsx.h>
   #include <winuser.h>
   #include <commctrl.h>
   #include <shellapi.h>
   #include <shlobj.h>
#endif

/* Windows GUI includes */
#include "win/dialog.rh"

#define PROGNAME_STR  "Teo"
#define BUFFER_SIZE  512
#define NOT_FOUND   -1

#define FIXED_WIDTH_FONT_NAME    "Courier New"
#define FIXED_WIDTH_FONT_HEIGHT  14

extern HINSTANCE prog_inst;
extern HWND prog_win;
extern HICON prog_icon;

extern void wgui_Error (HWND hwnd, const char *message);
extern void wgui_Warning (HWND hwnd, const char *message);

extern void SelectGraphicMode(int *, int *);

extern void wgui_Panel(void);
extern void wgui_Free (void);
extern int  CALLBACK wabout_Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void wabout_Free (void);
extern int  CALLBACK wsetting_TabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void wsetting_Free (void);
extern int  CALLBACK wcass_TabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void wcass_Free (void);
extern int  CALLBACK wmemo_TabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void wmemo_Free (void);
extern int  CALLBACK wdisk_TabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void wdisk_Free (void);
extern int  CALLBACK wprinter_TabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void wprinter_Free (void);

extern void wdebug_Panel(void);
/* disassembly */
extern void wddisass_Init(HWND hDlg);
extern void wddisass_DoStep(void);
extern void wddisass_DoStepOver(HWND hDlg);
extern void wddisass_Display(HWND hDlg);
extern void wddisass_Exit(HWND hDlg);
/* extras registers */
extern void wdreg_Init(HWND hDlg);
extern void wdreg_Display(HWND hDlg);
extern void wdreg_Exit(HWND hDlg);
/* memory */
extern void wdmem_Init(HWND hDlg);
extern void wdmem_Display(HWND hDlg);
extern int  wdmem_GetStepAddress(void);
extern void wdmem_StepDisplay(HWND hDlg, int address);
extern void wdmem_Exit(HWND hDlg);
/* accumulators */
extern void wdacc_Init(HWND hDlg);
extern void wdacc_Display(HWND hDlg);
extern void wdacc_Exit(HWND hDlg);
/* breakpoints */
extern void wdbkpt_Init(HWND hDlg);
extern void wdbkpt_Update (HWND hDlg, int number);
extern void wdbkpt_TraceOn(void);
extern void wdbkpt_TraceOff(void);
extern void wdbkpt_Exit(HWND hDlg);
/* status bar */
extern void wdstatus_Init (HWND hDlg);
extern void wdstatus_Display (HWND hDlg);
/* tool bar */
extern void wdtoolb_Init (HWND hDlg);
extern void wdtoolb_DisplayTooltips (LPARAM lParam);


extern HFONT wdebug_GetNormalFixedWidthHfont (void);
extern HFONT wdebug_GetBoldFixedWidthHfont (void);

extern void wgui_CreateTooltip (HWND hWnd, WORD id, char *text);

#endif
