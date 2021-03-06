/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2018 Yves Charriau, Fran�ois Mouret
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
 *  Module     : linux/progress.c
 *  Version    : 0.7.0
 *  Cr�� par   : Fran�ois Mouret 27/02/2013
 *  Modifi� par:
 *
 *  Progression dialog.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <stdlib.h>
   #include <pthread.h>
   #include <gtk/gtk.h>
#endif

#include "main.h"
#include "errors.h"
#include "cc90.h"
#include "linux/progress.h"
#include "linux/gui.h"

int progress_on = FALSE;
int progress_dead = FALSE;

static int progress_err = 0;
static int progress_percent = -1 ;
static int progress_percent_new = 0;

static GThread *thread_process = NULL;
static int data = 0;



/* progress_exit:
 *  Exit the progression.
 */
static void progress_exit (void)
{
    progress_on = FALSE;
    if (thread_process != NULL)
    {
        g_thread_join (thread_process);
        thread_process = NULL;
    }
    if (progress_err < 0)
    {
        gui_ErrorDialog (error_msg);
        progress_err = 0;
    }
    gui_ResetProgress ();
    cc90_Close();
    progress_dead = TRUE;
}


static gboolean progress_update_bar (gpointer data)
{
    gboolean ret = progress_on;
    (void)data;

    if (ret == TRUE)
    {
        if (progress_percent_new != progress_percent)
        {
            progress_percent = progress_percent_new;
            gui_SetProgressBar ((double)progress_percent/(double)100);
        }
    }
    else
        progress_exit();

    return ret;
}



static gpointer progress_process (gpointer data)
{
    int (*process)(void) = data;
    progress_err = (*process)();
    progress_on = FALSE;
    return NULL;
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
    progress_dead = FALSE;
    progress_percent = -1 ;
    
    if (windowed_mode != 0)
    {
        gui_EnableButtons (FALSE);
        if ((progress_err = cc90_Open()) == 0)
        {
            (void)g_timeout_add_full (
                                    G_PRIORITY_DEFAULT,
                                    250,
                                    (GSourceFunc)progress_update_bar,
                                    GINT_TO_POINTER(data),
                                    (GDestroyNotify)progress_exit);

            thread_process = g_thread_new (
                                    "cc90hfeProcess",
                                    (GThreadFunc)progress_process,
                                    (gpointer)process);
        }
        else
            progress_exit();
    }
}



/* progress_Stop:
 *  Stop the progression.
 */
void progress_Stop (GtkWidget *widget, gpointer user_data)
{
    progress_on = FALSE;
    (void)widget;
    (void)user_data;
}

