/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 Yves Charriau, François Mouret, Samuel Devulder
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
 *  Module     : windows/gui.c
 *  Version    : 0.5.0
 *  Créé par   : François Mouret & Samuel Devulder 27/02/2013
 *  Modifié par:
 *
 *  Gui functions.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <windows.h>
   #include <windowsx.h>
   #include <commctrl.h>
#endif

#include "defs.h"
#include "main.h"
#include "std.h"
#include "errors.h"
#include "encode.h"
#include "win/gui.h"
#include "win/progress.h"
#include "win/resource.h"

HINSTANCE hInst;
HWND main_window;
HWND main_dialog;

static HWND archive_button;
static HWND extract_button;
static HWND install_button;
static HWND about_button;
static HWND progres_ltext;
static HWND progress_bar;
static HWND cancel_button;


#define BUFFER_SIZE  1024


/* MainWndProc:
 *  Create the main dialog procedure.
 */
static LRESULT CALLBACK MainDlgProc(HWND hDlg, UINT uMsg,
                                    WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            /* Initialisation des pointeurs de contrôles */
            archive_button = GetDlgItem (hDlg, IDC_MAIN_ARCHIVE_BUTTON);
            extract_button = GetDlgItem (hDlg, IDC_MAIN_EXTRACT_BUTTON);
            install_button = GetDlgItem (hDlg, IDC_MAIN_INSTALL_BUTTON);
            about_button   = GetDlgItem (hDlg, IDC_MAIN_ABOUT_BUTTON);
            progres_ltext  = GetDlgItem (hDlg, IDC_MAIN_PROGRESS_LTEXT);
            progress_bar   = GetDlgItem (hDlg, IDC_MAIN_PROGRESS_BAR);
            cancel_button  = GetDlgItem (hDlg, IDC_MAIN_CANCEL);
            Button_SetText (archive_button,
                            is_fr?"Copier une disquette vers un fichier HFE"
                                 :"Copy a floppy onto a HFE file");
            Button_SetText (extract_button,
                            is_fr?"Copier un fichier HFE vers une disquette"
                                 :"Copy a HFE file onto a floppy");
            Button_SetText (install_button,
                            is_fr?"Installer CC90 sur le Thomson"
                                 :"Install CC90 on the Thomson");
            Button_SetText (about_button,
                            is_fr?"A propos..."
                                 :"About...");
            Button_SetText (cancel_button,
                            is_fr?"Annuler"
                                 :"Cancel");
            gui_ResetProgress ();
            return 0;

        case WM_COMMAND :
            switch(LOWORD(wParam))
            {
                case IDC_MAIN_ARCHIVE_BUTTON :
                     archive_Prog ();
                     break;

                case IDC_MAIN_EXTRACT_BUTTON :
                     extract_Prog ();
                     break;

                case IDC_MAIN_INSTALL_BUTTON :
                     install_Prog (hInst, hDlg);
                     break;

                case IDC_MAIN_ABOUT_BUTTON :
                     about_Prog (hInst, hDlg);
                     break;

                case IDC_MAIN_CANCEL :
                     progress_Stop();
                     gui_ResetProgress ();
                     break;
            }
            break;

        case WM_DESTROY :
            PostQuitMessage (0);
            break;
    }
    return 0;

    (void)lParam;
    (void)hDlg;
}


/* MainWndProc:
 *  Create the main window procedure.
 */
static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg,
                                    WPARAM wParam, LPARAM lParam)
{
    int ret;

    switch(uMsg)
    {
        case WM_CREATE :
            main_dialog = CreateDialog (hInst, MAKEINTRESOURCE(ID_MAIN),
                                    hWnd, (DLGPROC)MainDlgProc);
            break;

        case WM_CLOSE :
            ret = IDOK;
            if (progress_on == TRUE)
                ret = MessageBox(hWnd,
                    is_fr?encode_String(
                          "Un processus est encore en cours d'exécution.\n" \
                          "Voulez-vous vraiment quitter ?")
                         :"A process is still running\n" \
                          "Do you really want to quit ?",
                    PROG_NAME" - Confirmation",
                    MB_OKCANCEL | MB_ICONINFORMATION);
 
            if (ret == IDOK)
            {
                progress_Stop();
                DestroyWindow (hWnd);
            }
            break;

        case WM_DESTROY :
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}


/* RunWindowProg:
 *  Open the main window and run the windowed program.
 */
static int RunWindowProg (HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASS wc;
    MSG msg;
    
    hInst = hInstance;
    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon((HANDLE)hInst, "IDI_ICON1");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "CC90HFEClass";
    if (!RegisterClass(&wc)) return FALSE;

    main_window = CreateWindow("CC90HFEClass", PROG_NAME,
                          WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
                          0, 0, 350, 314,
                          NULL, NULL, hInstance, NULL);
    if (main_window == NULL)
        return FALSE;

    ShowWindow (main_window, nCmdShow);
    UpdateWindow (main_window);
    while (GetMessage (&msg, NULL, 0, 0) == TRUE)
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    return msg.wParam;
    (void) nCmdShow;
}


/* ------------------------------------------------------------------------- */

void gui_EmitStop (void)
{
    (void)SendMessage(main_dialog, WM_COMMAND, IDC_MAIN_CANCEL, 0);
}



void gui_SetProgressText (char *message)
{
    Static_SetText (progres_ltext, message);
}



void gui_EnableButtons (int flag)
{
    Button_Enable(archive_button, flag);
    Button_Enable(extract_button, flag);
    Button_Enable(install_button, flag);
    Button_Enable(cancel_button, (flag == TRUE) ? FALSE : TRUE);
}



void gui_ResetProgress (void)
{
    Static_SetText (progres_ltext, is_fr?"En attente.":"Waiting.");
   (void)SendMessage (progress_bar, PBM_SETRANGE, 0, MAKELPARAM (0, BAR_LENGTH));
   (void)SendMessage (progress_bar, PBM_SETPOS, 0, 0);
   gui_EnableButtons (TRUE);
}



void gui_SetProgressBar (int value)
{
    (void)SendMessage (progress_bar, PBM_SETPOS, value, 0);
}



void gui_ErrorDialog (char *message)
{
    MessageBox(main_dialog, encode_String(message),
               is_fr?PROG_NAME" - Erreur":PROG_NAME" - Error",
                MB_OK | MB_ICONERROR);
}



int gui_InformationDialog (char *message)
{
    int ret = MessageBox(main_dialog, encode_String(message),
                   PROG_NAME" - Confirmation",
                   MB_OKCANCEL | MB_ICONINFORMATION);
    return (ret == IDCANCEL) ? FALSE : TRUE;
}


/* gui_OpenFile:
 *  Open a new HFE disk or an already existing one.
 */
int gui_OpenFile (int flags, char *title)
{
    static char current_file[MAX_PATH+1]="";
    OPENFILENAME ofn;

    disk.file_name = std_free (disk.file_name);

    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = main_dialog;
    ofn.lpstrFilter = is_fr?"Fichiers HFE\0*.hfe\0" \
                           :"HFE files\0*.hfe\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = current_file;
    ofn.nMaxFile = BUFFER_SIZE;
    ofn.lpstrTitle = title;
    ofn.Flags = flags;
    ofn.lpstrDefExt ="hfe";

    if (gui.archive_folder != NULL)
        ofn.lpstrInitialDir = gui.archive_folder;
    else
    if (gui.default_folder != NULL)
        ofn.lpstrInitialDir = gui.default_folder;

    if (GetOpenFileName(&ofn))
    {
          disk.file_name = std_strdup_printf ("%s", ofn.lpstrFile);
          gui.default_folder = std_free(gui.default_folder);
          gui.default_folder = std_strdup_printf ("%s", ofn.lpstrFile);
          gui.archive_folder = std_free(gui.archive_folder);
          gui.archive_folder = std_strdup_printf ("%s", ofn.lpstrFile);
          return TRUE;
    }
    return FALSE;
}


//***************************************************************************************
// WinMain :
//***************************************************************************************


void gui_ProgressUpdate (int percent)
{
    progress_Update (percent);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc=0;
#ifndef __MINGW32__	
    char *argv[16];

    if (*lpCmdLine)
    {
        argv[argc++]=lpCmdLine++;

        while (*lpCmdLine)
            if (*lpCmdLine == ' ')
            {
                *lpCmdLine++ = '\0';
                argv[argc++]=lpCmdLine++;
            }
            else
                lpCmdLine++;
    }
#else
    char **argv;

	/* Windows fourni des argc/argv déjà parsés qui tient 
	   compte des guillemets et des blancs. */
	argc = __argc;
	argv = (void*)__argv;
#endif

#ifdef FRENCH_LANGUAGE
    is_fr = 1;
#else
    is_fr = 0;
#endif

    main_InitAll ();

#ifndef CONSOLE_MODE
    windowed_mode = 1;
#endif

    if (windowed_mode == 0)
    {
        encode_Set (CODESET_PC850);
        main_ConsoleReadCommandLine (argc, argv);
        main_Console ();
    }
    else
    {
        InitCommonControls ();
        OleInitialize(0);
        windowed_mode = 1;
        encode_Set (CODESET_WINDOWS1252);
        RunWindowProg (hInstance, nCmdShow);
        OleUninitialize ();
    }

    windowed_mode = 0;
    main_FreeAll ();
    return EXIT_SUCCESS;

    (void)lpCmdLine;
    (void)hPrevInstance;
    (void)hInstance;
    (void)nCmdShow;
}
