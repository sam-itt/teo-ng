/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2016 Yves Charriau, François Mouret
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
 *  Module     : windows/progress.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Progression dialog.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <windows.h>
   #include <windowsx.h>
   #include <commctrl.h>
   #include <shlobj.h>
#endif

#include "defs.h"
#include "main.h"
#include "errors.h"
#include "cc90.h"
#include "win/gui.h"
#include "win/progress.h"
#include "win/resource.h"

#define BUFFER_SIZE  512

int progress_on = FALSE;

static int progress_err = 0;
static int progress_percent = -1 ;
static int progress_percent_new = 0;

static HANDLE bar_thread = NULL;
static DWORD  bar_thread_id;
static HANDLE prg_thread = NULL;
static DWORD  prg_thread_id;



static void progress_exit (void)
{
    progress_on = FALSE;
    if (progress_err < 0)
    {
        gui_ErrorDialog (error_msg);
        progress_err = 0;
    }
    gui_ResetProgress ();
    cc90_Close();
}



static void progress_bar (void)
{
    while (progress_on == TRUE)
    {
        if (progress_percent_new != progress_percent)
        {
            progress_percent = progress_percent_new;
            gui_SetProgressBar (progress_percent * BAR_LENGTH / 100);
        }
    }
}



static void progress_process (int (*process)(void))
{
    progress_err = (*process)();
    gui_EmitStop ();
}


/* ------------------------------------------------------------------------- */


/* progress_Update:
 *  Update the progression.
 */
void progress_Update (int percent)
{
    progress_percent_new = percent;
}    



/* progress_Run:
 *  Run the progression.
 */
void progress_Run (int (*process)(void))
{
    progress_err = 0;
    progress_on = TRUE;
    progress_percent = -1 ;

    if (windowed_mode != 0)
    {
        gui_EnableButtons (FALSE);
        if ((progress_err = cc90_Open()) == 0)
        {
            bar_thread = CreateThread (
                               NULL, 0,
                               (LPTHREAD_START_ROUTINE)progress_bar,
                               NULL, 0, &bar_thread_id);
            prg_thread = CreateThread (
                               NULL, 0,
                               (LPTHREAD_START_ROUTINE)progress_process,
                               (LPVOID)process, 0, &prg_thread_id);
        }
        else
            progress_exit();
    }
}



/* progress_Stop:
 *  Stop the progression.
 */
void progress_Stop (void)
{
    progress_on = FALSE;
    if (prg_thread != NULL)
    {
        WaitForSingleObjectEx (bar_thread, TRUE, INFINITE);
        WaitForSingleObjectEx (prg_thread, TRUE, INFINITE);
        CloseHandle (bar_thread);
        CloseHandle (prg_thread);
        prg_thread = NULL;
        bar_thread = NULL;
        progress_exit();
    }
}

