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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : win/wgui/wdebug.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 08/10/2013
 *  Modifié par: François Mouret 04/06/2015 15/07/2016
 *
 *  Débogueur 6809.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "debug.h"
#include "debug/debug.h"
#include "win/gui.h"

struct MC6809_DEBUG debug;

static LONG main_window_x_init = 0;
static LONG main_window_y_init = 0;
static LONG main_window_width_init = 0;
static LONG main_window_height_init = 0;

static LONG mem_window_h_interval;
static LONG dasm_window_w_interval;
static LONG dasm_window_h_interval;



/* resize_windows:
 *  Resize all the concerned windows.
 */
static void resize_windows (HWND hDlg)
{
    RECT rect;
    HWND hwnd;
    LONG main_window_rect_right;
    LONG main_window_rect_bottom;

    GetWindowRect(hDlg, &rect);
    main_window_rect_right = rect.right;
    main_window_rect_bottom = rect.bottom;
    
    /* resize the disassembling text */
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_DASM_EDIT);
    GetWindowRect(hwnd, &rect);
    SetWindowPos (hwnd, NULL, 0, 0,
        (int)(main_window_rect_right - rect.left - dasm_window_w_interval),
        (int)(main_window_rect_bottom - rect.top - dasm_window_h_interval),
        SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_EDIT);
    GetWindowRect(hwnd, &rect);
    SetWindowPos (hwnd, NULL, 0, 0,
        (int)(rect.right-rect.left),
        (int)(main_window_rect_bottom - rect.top - mem_window_h_interval),
        SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);

    hwnd = GetDlgItem(hDlg, IDC_DEBUG_STATUS_BAR);
    SendMessage(hwnd, WM_SIZE, 0, 0);

    SendMessage(GetDlgItem(hDlg, IDC_DEBUG_TOOL_BAR), TB_AUTOSIZE, 0, 0);
}



/* load_window_placement:
 *  Load the x, y, width, height and show command of the window.
 */
static void load_window_placement (HWND hDlg)
{
    WINDOWPLACEMENT lpwndpl;
    LPRECT lprect;

    lpwndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hDlg, &lpwndpl);
    lprect = &lpwndpl.rcNormalPosition;

    lprect->left   = main_window_x_init;
    lprect->right  = main_window_x_init + main_window_width_init;
    lprect->top    = main_window_y_init;
    lprect->bottom = main_window_y_init + main_window_height_init;
    lpwndpl.showCmd = SW_RESTORE;
  
    if (((LONG)teo.debug.window_width >= main_window_width_init)
     && ((LONG)teo.debug.window_height >= main_window_height_init))
    {
        lprect->left = (LONG)teo.debug.window_x;
        lprect->right = (LONG)(teo.debug.window_x + teo.debug.window_width);
        lprect->top = (LONG)teo.debug.window_y;
        lprect->bottom = (LONG)(teo.debug.window_y + teo.debug.window_height);
        lpwndpl.showCmd = (teo.debug.window_maximize == TRUE)
                                ? SW_SHOWMAXIMIZED
                                : SW_RESTORE;
    }

    SetWindowPlacement(hDlg, &lpwndpl);
}



/* save_window_placement:
 *  Save the x, y, width, height and show command of the window.
 */
static void save_window_placement (HWND hDlg)
{
    WINDOWPLACEMENT lpwndpl;
    LPRECT lprect;

    lpwndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hDlg, &lpwndpl);
    lprect = &lpwndpl.rcNormalPosition;
    teo.debug.window_x = (int)lprect->left;
    teo.debug.window_y = (int)lprect->top;
    teo.debug.window_width = (int)(lprect->right - lprect->left);
    teo.debug.window_height = (int)(lprect->bottom - lprect->top);

    if (lpwndpl.showCmd == SW_SHOWMAXIMIZED)
    {
        teo.debug.window_maximize = TRUE;
    }
    else
    {
        teo.debug.window_maximize = FALSE;
    }
}



void get_dimensions_at_init (HWND hDlg)
{
    HWND hwnd;
    WINDOWPLACEMENT lpwndpl;
    LPRECT lprect;
    RECT rect;
    LONG main_window_rect_right;
    LONG main_window_rect_bottom;

    lpwndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hDlg, &lpwndpl);
    lprect = &lpwndpl.rcNormalPosition;
    main_window_x_init = lprect->left;
    main_window_y_init = lprect->top;
    main_window_width_init = lprect->right - lprect->left;
    main_window_height_init = lprect->bottom - lprect->top;

    GetWindowRect(hDlg, &rect);
    main_window_rect_right = rect.right;
    main_window_rect_bottom = rect.bottom;

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_EDIT);
    GetWindowRect(hwnd, &rect);
    mem_window_h_interval = main_window_rect_bottom - rect.bottom;

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_DASM_EDIT);
    GetWindowRect(hwnd, &rect);
    dasm_window_w_interval = main_window_rect_right - rect.right;
    dasm_window_h_interval = main_window_rect_bottom - rect.bottom;
}



/* init_display:
 *  Initialize the display.
 */
static void init_display (HWND hDlg)
{
    wdstatus_Init(hDlg);
    wddisass_Init(hDlg);
    wdmem_Init(hDlg);
    wdreg_Init(hDlg);
    wdacc_Init(hDlg);
    wdbkpt_Init(hDlg);
}



/* update_display:
 *  Update the debugger display in step mode.
 *  The memory display programm is executed externaly.
 */
static void update_display (HWND hDlg)
{
    wdstatus_Display(hDlg);
    wddisass_Display(hDlg);
    wdreg_Display(hDlg);
    wdacc_Display(hDlg);
}



static void exit_display (HWND hDlg)
{
    wddisass_Exit(hDlg);
    wdmem_Exit(hDlg);
    wdreg_Exit(hDlg);
    wdacc_Exit(hDlg);
    wdbkpt_Exit(hDlg);
}



/* get_fixed_width_hfont:
 *  Get the pointer to the fixed width font.
 */
static HFONT get_fixed_width_hfont (int weight)
{
    static HFONT hfont;

    hfont = CreateFont(14,          /* font height */
                       0,
                       0,
                       0,
                       weight,      /* font weight */
                       FALSE,
                       FALSE,
                       FALSE,
                       DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY,
                       DEFAULT_PITCH,
                       "Courier New");
    return hfont;
}



/* wdebug_Proc:
 *  Debugger procedure.
 */
static int CALLBACK wdebug_Proc(HWND hDlg, UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    static int bkpt_number = 0;
    static int address = 0;
    MINMAXINFO *minmaxinfo;
    HWND hwnd;

    switch(uMsg)
    {
        case WM_INITDIALOG:
#ifdef FRENCH_LANGUAGE
            SetWindowText(hDlg, "Teo - Débogueur");
#else
            SetWindowText(hDlg, "Teo - Debugger");
#endif
            get_dimensions_at_init (hDlg);
            wdtoolb_Init (hDlg);
            wdstatus_Init (hDlg);
            load_window_placement (hDlg);

            /* window updates */
            init_display (hDlg);
            wdmem_Display (hDlg);
            update_display (hDlg);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDM_DEBUG_BUTTON_STEP:
                    address = dmem_GetStepAddress();
                    ddisass_DoStep();
                    wdmem_StepDisplay(hDlg, address);
                    update_display (hDlg);
                    return TRUE;

                case IDM_DEBUG_BUTTON_STEP_OVER:
                    address = dmem_GetStepAddress();
                    wddisass_DoStepOver(hDlg);
                    wdmem_StepDisplay(hDlg, address);
                    update_display (hDlg);
                    return TRUE;
 
                case IDC_DEBUG_EDIT_BKPT1:
                case IDC_DEBUG_EDIT_BKPT2:
                case IDC_DEBUG_EDIT_BKPT3:
                case IDC_DEBUG_EDIT_BKPT4:
                case IDC_DEBUG_EDIT_BKPT5:
                case IDC_DEBUG_EDIT_BKPT6:
                case IDC_DEBUG_EDIT_BKPT7:
                case IDC_DEBUG_EDIT_BKPT8:
                case IDC_DEBUG_EDIT_BKPT9:
                case IDC_DEBUG_EDIT_BKPT10:
                case IDC_DEBUG_EDIT_BKPT11:
                case IDC_DEBUG_EDIT_BKPT12:
                case IDC_DEBUG_EDIT_BKPT13:
                case IDC_DEBUG_EDIT_BKPT14:
                case IDC_DEBUG_EDIT_BKPT15:
                case IDC_DEBUG_EDIT_BKPT16:
                    bkpt_number = LOWORD(wParam)-IDC_DEBUG_EDIT_BKPT1;
                    switch(HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            wdbkpt_Update (hDlg, bkpt_number);
                            return TRUE;
                    }
                    return FALSE;

                case IDC_DEBUG_MEM_COMBO:
                case IDC_DEBUG_RAM_COMBO:
                case IDC_DEBUG_MON_COMBO:
                case IDC_DEBUG_VIDEO_COMBO:
                case IDC_DEBUG_CART_COMBO:
                    if (HIWORD(wParam)==CBN_SELCHANGE)
                    {
                        hwnd = GetDlgItem(hDlg, IDC_DEBUG_RAM_COMBO);
                        teo.debug.ram_number = ComboBox_GetCurSel(hwnd);
                        hwnd = GetDlgItem(hDlg, IDC_DEBUG_MON_COMBO);
                        teo.debug.mon_number = ComboBox_GetCurSel(hwnd);
                        hwnd = GetDlgItem(hDlg, IDC_DEBUG_VIDEO_COMBO);
                        teo.debug.video_number = ComboBox_GetCurSel(hwnd);
                        hwnd = GetDlgItem(hDlg, IDC_DEBUG_CART_COMBO);
                        teo.debug.cart_number = ComboBox_GetCurSel(hwnd);
                        hwnd = GetDlgItem(hDlg, IDC_DEBUG_MEM_COMBO);
                        teo.debug.memory_address = ComboBox_GetCurSel(hwnd)*0x2000;
                        hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_EDIT);
                        teo.debug.memory_address += Edit_GetFirstVisibleLine(hwnd) * 8;
                        wdmem_Display(hDlg);
                    }
                    return TRUE;

                case IDCANCEL:
                case IDM_DEBUG_BUTTON_LEAVE:
                case IDM_DEBUG_BUTTON_RUN:
                    save_window_placement (hDlg);
                    exit_display (hDlg);
                    if (LOWORD(wParam) == IDM_DEBUG_BUTTON_RUN)
                    {
                        dbkpt_TraceOn();
                        EndDialog(hDlg, TRUE);
                    }
                    else
                    {
                        dbkpt_TraceOff();
                        EndDialog(hDlg, FALSE);
                    }
                    break;
            }
            break;

        /* resize all the windows */
        case WM_SIZE :
            resize_windows (hDlg);
            break;

        /* limit minimal size of window */
        case WM_GETMINMAXINFO:
            minmaxinfo = (MINMAXINFO *)lParam;
            minmaxinfo->ptMinTrackSize.x = main_window_width_init;
            minmaxinfo->ptMinTrackSize.y = main_window_height_init;
            return FALSE;

        case WM_NOTIFY:
            /* display toolbar button tooltip */
            if ((((LPNMHDR) lParam)->code) == TTN_GETDISPINFO)
                wdtoolb_DisplayTooltips (lParam);
            break;

        default:
            return FALSE;
   }
   return TRUE;
}


/* ------------------------------------------------------------------------- */


/* wdebug_GetNormalFixedWidthHfont:
 *  Get the pointer to the normal fixed width font.
 */
HFONT wdebug_GetNormalFixedWidthHfont (void)
{
    return get_fixed_width_hfont (FW_NORMAL);
}



/* wdebug_GetBoldFixedWidthHfont:
 *  Get the pointer to the bold fixed width font.
 */
HFONT wdebug_GetBoldFixedWidthHfont (void)
{
    return get_fixed_width_hfont (FW_BOLD);
}



/* wdebug_Panel:
 *  Display the debugger dialog.
 */
void wdebug_Panel(void)
{
    mc6809_FlushExec ();

    (void)DialogBox (prog_inst,
                     MAKEINTRESOURCE(IDC_DEBUG_DIALOG),
                     prog_win,
                     (DLGPROC)wdebug_Proc);

    teo.command = TEO_COMMAND_NONE;
}
