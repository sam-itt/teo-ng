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
 *  Copyright (C) 1997-2011 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.0
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

#if BEFORE_GTK_2_MIN
/* =================================================================
 *                          < GTK 2.12
 * ================================================================= */

/* partie privée de la classe FileEntry:
 */
enum {
    FILE_SELECTED_SIGNAL,
    LAST_SIGNAL
};

static guint file_entry_signals[LAST_SIGNAL]={0};

#else

/* =================================================================
 *                          >= GTK 2.12
 * ================================================================= */

static gchar previous_file_name[FILENT_LENGTH+1] = "";
static gchar new_file_name[FILENT_LENGTH+1] = "";

#endif

#if BEFORE_GTK_2_MIN
/* =================================================================
 *                          < GTK 2.12
 * ================================================================= */

static void file_entry_class_init(FileEntryClass *class)
{
    GtkObjectClass *object_class;

    object_class=(GtkObjectClass *) class;

    file_entry_signals[FILE_SELECTED_SIGNAL] = 
      g_signal_new ("file_selected",
		    G_TYPE_FROM_CLASS (object_class),
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(FileEntryClass, file_selected),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    class->file_selected = NULL;
}



static void file_entry_filesel_ok(GtkWidget *button, FileEntry *filent)
{
    strncpy(filent->filename, gtk_file_selection_get_filename( GTK_FILE_SELECTION(filent->filesel)), FILENT_LENGTH);

    gtk_widget_destroy( GTK_WIDGET(filent->filesel));

    /* On envoie le signal file_selected */
    gtk_signal_emit( GTK_OBJECT(filent), file_entry_signals[FILE_SELECTED_SIGNAL]);

    (void) button;
}



static void file_entry_popup_filesel(GtkWidget *button, FileEntry *filent)
{
    filent->filesel=gtk_file_selection_new((is_fr?"Sélection d'un fichier":"Select a file"));

    /* On passe le nom du fichier au sélecteur de fichier */
    gtk_file_selection_set_filename( GTK_FILE_SELECTION(filent->filesel), filent->filename);

    /* Connect the ok_button to filesel_ok function */
    gtk_signal_connect(GTK_OBJECT( GTK_FILE_SELECTION(filent->filesel)->ok_button), "clicked", GTK_SIGNAL_FUNC(file_entry_filesel_ok), filent);

    /* Connect the cancel_button to destroy the widget */
    gtk_signal_connect_object( GTK_OBJECT( GTK_FILE_SELECTION(filent->filesel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(filent->filesel));

    gtk_quit_add_destroy(1, GTK_OBJECT(filent->filesel));
    gtk_widget_show(filent->filesel);

    (void) button;
}



static void file_entry_init(FileEntry *filent)
{
    filent->hbox.box.homogeneous=FALSE;
    filent->hbox.box.spacing=5;

    filent->label=gtk_label_new("\0");
    gtk_box_pack_start( GTK_BOX(&filent->hbox), filent->label, FALSE, FALSE,0);
    gtk_widget_show(filent->label);

    filent->entry=gtk_entry_new();
    gtk_entry_set_editable( GTK_ENTRY(filent->entry), FALSE);
    gtk_box_pack_start( GTK_BOX(&filent->hbox), filent->entry, TRUE, TRUE, 0);
    gtk_widget_show(filent->entry);

    filent->button=gtk_button_new_with_label((is_fr?"Parcourir...":"Browse..."));
    gtk_signal_connect( GTK_OBJECT(filent->button), "clicked", GTK_SIGNAL_FUNC(file_entry_popup_filesel), filent);
    gtk_box_pack_start( GTK_BOX(&filent->hbox), filent->button, FALSE,FALSE,0);
    gtk_widget_show(filent->button);
}



/* partie publique de la classe FileEntry:
 */
GType file_entry_get_type(void)
{
    static GType filent_type = 0;

    if (!filent_type)
    {
	static const GTypeInfo filent_info =
	{
	    sizeof(FileEntryClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) file_entry_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(FileEntry),
	    0,    /* n_preallocs */
	    (GInstanceInitFunc) file_entry_init,
	    NULL
	};

        filent_type = g_type_register_static (GTK_TYPE_HBOX,
					      "FileEntry",
					      &filent_info,
					      0);
    }

    return filent_type;
}



GtkWidget *file_entry_new(const gchar *label)
{
    FileEntry *filent=FILE_ENTRY( gtk_type_new (file_entry_get_type() ));

    gtk_label_set_text( GTK_LABEL(filent->label), label);

    return GTK_WIDGET(filent);
}



void file_entry_set_entry(FileEntry *filent, const gchar *text)
{
    gtk_entry_set_text( GTK_ENTRY(filent->entry), text);
}



void file_entry_set_filename(FileEntry *filent, const gchar *filename)
{
    strncpy(filent->filename, filename, FILENT_LENGTH);
}



gchar *file_entry_get_filename(FileEntry *filent)
{
    return filent->filename;
}

#else
/* =================================================================
 *                          >= GTK 2.12
 * ================================================================= */

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
    xgtk_signal_connect( (GtkWidget *)dialog, "show", get_filename, (gpointer) previous_file_name);
    xgtk_signal_connect( dialog, "file-activated", get_filename, (gpointer) new_file_name);

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

#endif

