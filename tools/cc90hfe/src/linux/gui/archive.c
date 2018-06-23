/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2018 Yves Charriau, François Mouret
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
 *  Module     : linux/gui/archive.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 31/05/2015
 *
 *  Archive callback.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <stdlib.h>
   #include <gtk/gtk.h>
   #include <gtk/gtkx.h>
   #include <gdk/gdkx.h>
#endif

#include "defs.h"
#include "std.h"
#include "errors.h"
#include "main.h"
#include "linux/progress.h"
#include "linux/gui.h"

extern int main_archive_disk (void);

/* open_write_hfe_file:
 *  Charge une disquette en écriture.
 */
static char *open_write_hfe_file (void)
{
    GtkFileFilter *filter;
    static GtkWidget *dialog;
    char *folder_name;
    char *file_name;

    disk.file_name = std_free (disk.file_name);

    /* open dialog */
    dialog = gtk_file_chooser_dialog_new (
                 is_fr?"Ecrire une disquette HFE":"Write a HFE disk",
                 GTK_WINDOW(main_window),
                 GTK_FILE_CHOOSER_ACTION_SAVE,
                 is_fr?"_Annuler":"_Cancel", GTK_RESPONSE_CANCEL,
                 is_fr?"_Enregistrer":"_Save", GTK_RESPONSE_ACCEPT,
                 NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

    /* add filters */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, is_fr?"Fichiers HFE":"HFE files");
    gtk_file_filter_add_mime_type (filter, "application/x-hfe-floppy-image");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    /* waits for construction to be finished */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    if (gui.archive_folder != NULL)
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, gui.archive_folder);
    else
    if (gui.default_folder != NULL)
        (void)gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), gui.default_folder);

    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "Untitled document.hfe");

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        file_name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        disk.file_name = std_strdup_printf ("%s", file_name);

        folder_name = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
        if (folder_name != NULL)
        {
            gui.default_folder = std_free(gui.default_folder);
            gui.default_folder = std_strdup_printf ("%s", folder_name);
            gui.archive_folder = std_free(gui.archive_folder);
            gui.archive_folder = std_strdup_printf ("%s", folder_name);
        }
        g_free (folder_name);
        g_free (file_name);
    }
    gtk_widget_destroy (dialog);
    return disk.file_name;
}


/* ------------------------------------------------------------------------- */


void archive_Callback (GtkButton *button, gpointer user_data)
{
    int ret;

    if (open_write_hfe_file () == NULL)
        return;

    ret = gui_InformationDialog (
                     is_fr?"Introduisez une disquette dans le lecteur\n" \
                           "du Thomson et clickez sur OK pour commencer\n" \
                           "la lecture."
                          :"Insert a disk in the Thomson drive and click\n" \
                           "OK to start the reading.");

    if (ret == FALSE)
        return;

    gui_SetProgressText (is_fr?"Lecture des pistes...":"Reading tracks...");

    progress_Run (main_ArchiveDisk);

    (void)button;
    (void)user_data;
}

