/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 François Mouret
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
 *  Module     : linux/gui/extract.c
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Extract callback.
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>


#include "defs.h"
#include "std.h"
#include "errors.h"
#include "main.h"
#include "linux/progress.h"
#include "linux/gui.h"

extern int main_extract_disk (void);



/* open_read_hfe_file:
 *  Charge une disquette en lecture.
 */
static char *open_read_hfe_file (void)
{
    GtkFileFilter *filter;
    static GtkWidget *dialog;
    char *folder_name;
    char *file_name;

    disk.file_name = std_free (disk.file_name);

    /* open dialog */
    dialog = gtk_file_chooser_dialog_new (
                 is_fr?"Lire une disquette HFE":"Read a HFE disk",
                 GTK_WINDOW(main_window),
                 GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                 NULL);

    /* add filters */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, is_fr?"Fichiers HFE":"HFE files");
    gtk_file_filter_add_pattern (filter, "*.hfe");
    gtk_file_filter_add_pattern (filter, "*.HFE");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    /* waits for construction to be finished */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    if (gui.extract_folder != NULL)
        (void)gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), gui.extract_folder);
    else
    if (gui.default_folder != NULL)
        (void)gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), gui.default_folder);

    if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        file_name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));
        disk.file_name = std_strdup_printf ("%s", file_name);

        folder_name = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
        if (folder_name != NULL)
        {
            gui.default_folder = std_free(gui.default_folder);
            gui.default_folder = std_strdup_printf ("%s", folder_name);
            gui.extract_folder = std_free(gui.extract_folder);
            gui.extract_folder = std_strdup_printf ("%s", folder_name);
        }
        g_free (folder_name);
        g_free (file_name);
    }
    gtk_widget_destroy(dialog);
    return disk.file_name;
}



void extract_Callback (GtkButton *button, gpointer user_data)
{
    int ret;

    if (open_read_hfe_file () == NULL)
        return;

    ret = gui_InformationDialog (
                     is_fr?"Introduisez une disquette dans le lecteur\n" \
                           "du Thomson et clickez sur OK pour commencer\n" \
                           "l'écriture.\n"
                          :"Insert a disk in the Thomson drive and click\n" \
                           "OK to start the writing.");

    if (ret == FALSE)
        return;

    gui_SetProgressText (is_fr?"Ecriture des pistes...":"Writing tracks...");

    progress_Run (main_ExtractDisk);

    (void)button;
    (void)user_data;
}

