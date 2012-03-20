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
 *  Module     : linux/filentry.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 07/08/2011 13/01/2012
 *
 *  Classe FileEntry d'extension du toolkit GTK+ 2.x .
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <string.h>
   #include <gtk/gtk.h>
#endif

#include "linux/filentry.h"
#include "linux/main.h"
#include "to8.h"


static gchar previous_file_name[FILENT_LENGTH+1] = "";
static gchar new_file_name[FILENT_LENGTH+1] = "";


static void get_filename (GtkFileChooser *chooser, gpointer user_data)
{
    gchar *file_name;

    file_name = gtk_file_chooser_get_filename (chooser);
    (void)snprintf ((gchar *)user_data, FILENT_LENGTH, "%s", file_name);
    g_free(file_name);
    (void)user_data;
}



GtkWidget *file_chooser_button_new(char *label, const gchar *title, const gchar *patternname,
                   char *patternfilter, const gchar *current_file, char *current_dir,
                   GtkWidget *parent_window, GtkWidget *hbox)
{
    GtkFileFilter *filter;
    GtkWidget *dialog;
    GtkWidget *hlabel;
    GtkWidget *button;
    char *p;
    char pattern[30] = "";

    hlabel=gtk_label_new(label);
    gtk_box_pack_start( GTK_BOX(hbox), hlabel, FALSE, FALSE,0);
    gtk_widget_show(hlabel);

    dialog = gtk_file_chooser_dialog_new (
                 title,
                 (GtkWindow *) parent_window,
                 GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                 NULL);
    if (*patternfilter != '\0')
    {
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, patternname);
        strcpy (pattern, patternfilter);
        p = strtok (pattern, "|");
        while (p != NULL) {
            gtk_file_filter_add_pattern (filter, p);
            p = strtok (NULL, "|");
        }
        gtk_file_chooser_add_filter ((GtkFileChooser *)dialog, filter);
    }
    if (strlen (current_file) != 0)
        (void)gtk_file_chooser_set_filename((GtkFileChooser *)dialog, current_file);
    else    
    if (access(current_dir, F_OK) == 0)
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, current_dir);
    g_signal_connect(G_OBJECT (dialog), "show", G_CALLBACK(get_filename), (gpointer) previous_file_name);
    g_signal_connect(G_OBJECT(dialog), "file-activated", G_CALLBACK(get_filename), (gpointer) new_file_name);

    button = gtk_file_chooser_button_new_with_dialog (dialog);
    
    /* Attend que le dialog ait tout assimilé */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    return button;
}



gchar *file_chooser_get_filename(void)
{
    return new_file_name;
}



void file_chooser_reset_filename(GtkFileChooserButton *chooser_button)
{
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(chooser_button), previous_file_name);
}

