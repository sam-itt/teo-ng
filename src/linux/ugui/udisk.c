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
 *  Module     : linux/ugui/udisk.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 07/08/2011 24/03/2012
 *
 *  Gestion des disquettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <string.h>
   #include <libgen.h>
   #include <gtk/gtk.h>
#endif

#include "linux/gui.h"
#include "to8.h"
#include "intern/gui.h"

#define NDISKS 4

struct FILE_VECTOR {
    int first;
    int id;
    int direct;
    int entry_max;
    gulong combo_changed_id;
    GtkWidget *combo;
    GtkWidget *check_prot;
    GList *path_list;
};

static struct FILE_VECTOR vector[NDISKS];



/* load_disk:
 *  Charge une disquette.
 */
static int load_disk (gchar *filename, struct FILE_VECTOR *vector)
{
    int ret = to8_LoadDisk (vector->id, filename);

    switch (ret)
    {
        case TO8_ERROR :
            error_box (to8_error_msg, wControl);
            break;

        case TO8_READ_ONLY :
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(vector->check_prot), TRUE);
            gui->disk[vector->id].write_protect = TRUE;
            break;

        default : break;
    }
    return ret;
}



/* toggle_check_disk:
 *  Vérifie si l'état de la protection peut être changée.
 */
static void toggle_check_disk(GtkWidget *button, struct FILE_VECTOR *vector)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
        to8_SetDiskMode (vector->id, TO8_READ_ONLY);
        gui->disk[vector->id].write_protect = TRUE;
    }
    else if (to8_SetDiskMode (vector->id, TO8_READ_WRITE)==TO8_READ_ONLY)
    {
        error_box(is_fr?"Ecriture impossible sur ce support."
                       :"Writing unavailable on this device.", wControl);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
        gui->disk[vector->id].write_protect = TRUE;
    }
}



/* add_combo_entry
 *  Ajoute une entrée dans le combobox si inexistante.
 */
static void add_combo_entry (const char *path, struct FILE_VECTOR *vector)
{
    GList *path_node = g_list_find_custom (vector->path_list, (gconstpointer)path, (GCompareFunc)g_strcmp0);

    if (path_node != NULL)
        gtk_combo_box_set_active (GTK_COMBO_BOX(vector->combo), g_list_position (vector->path_list, path_node));
    else
    {
        vector->path_list = g_list_append (vector->path_list, (gpointer)(g_strdup_printf (path,"%s")));
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(vector->combo), NULL, basename((char *)path));
        gtk_combo_box_set_active (GTK_COMBO_BOX(vector->combo), vector->entry_max);
        vector->entry_max++;
    }
}



/* free_disk_entry:
 *  Libère la mémoire utilisée par la liste des disquettes.
 */
static void free_disk_entry (struct FILE_VECTOR *vector)
{
    g_list_foreach (vector->path_list, (GFunc)g_free, (gpointer) NULL);
    g_list_free (vector->path_list);
    vector->path_list=NULL;
}



/* set_access_mode:
 *  Positionne le checkbox de protection de disquette.
 */
static void set_access_mode (struct FILE_VECTOR *vector)
{
    int ret;

    (void)to8_VirtualSetDrive(vector->id);
    ret = (gui->disk[vector->id].write_protect == TRUE) ? TO8_READ_ONLY : TO8_READ_WRITE;
    if ((vector->direct) && (gtk_combo_box_get_active (GTK_COMBO_BOX(vector->combo)) == 1))
        ret = to8_DirectSetDrive(vector->id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vector->check_prot), (ret==TO8_READ_ONLY) ? TRUE : FALSE);
    gui->disk[vector->id].write_protect = (ret==TO8_READ_ONLY) ? TRUE : FALSE;
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (struct FILE_VECTOR *vector)
{
    add_combo_entry (is_fr?"(Aucun)":"(None)", vector);
    if (vector->direct)
        add_combo_entry (is_fr?"(AccÃ¨s Direct)":"(Direct Access)", vector);
}



/* reset_combo:
 *  Ejecte la disquette et vide le combobox (callback).
 *  (sauf l'entrée "aucune cartouche" et "direct access") 
 */
static void reset_combo (GtkButton *button, struct FILE_VECTOR *vector)
{
    /* Bloque l'intervention de combo_changed */
    g_signal_handler_block (vector->combo, vector->combo_changed_id);

    free_disk_entry (vector);
    gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT(vector->combo));
    to8_EjectDisk(vector->id);
    init_combo (vector);
    set_access_mode (vector);

    /* Débloque l'intervention de combo_changed */
    g_signal_handler_unblock (vector->combo, vector->combo_changed_id);

    (void)button;
}



/* combo_changed:
 *  Changement de sélection du combobox (callback).
 */
static void combo_changed (GtkComboBox *combo_box, struct FILE_VECTOR *vector)
{
    guint active_row = gtk_combo_box_get_active (combo_box);

    if (active_row==0)
        to8_EjectDisk(vector->id);
    else
    if ((active_row!=1) || (vector->direct!=1))
        (void)load_disk ((char *)g_list_nth_data (vector->path_list, active_row), vector);

    set_access_mode (vector);
}



/* open_file:
 *  Charge une nouvelle disquette (callback).
 */
static void open_file (GtkButton *button, struct FILE_VECTOR *vector)
{
    GtkFileFilter *filter;
    static GtkWidget *dialog;
    char *folder_name;

    if (vector->first) {
        dialog = gtk_file_chooser_dialog_new (
                 is_fr?"SÃ©lectionner une disquette":"Select a disk",
                 (GtkWindow *) wControl, GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, is_fr?"Fichiers disquette (.sap)":"Disk files (.sap)");
        gtk_file_filter_add_pattern (filter, "*.sap");
        gtk_file_filter_add_pattern (filter, "*.SAP");
        gtk_file_chooser_add_filter ((GtkFileChooser *)dialog, filter);

        if (strlen (gui->disk[vector->id].file) != 0)
            (void)gtk_file_chooser_set_filename((GtkFileChooser *)dialog, gui->disk[vector->id].file);
        else
        if (strlen (gui->default_folder) != 0)
            (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, gui->default_folder);
        else
        if (access("./disks/", F_OK) == 0)
            (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, "./disks/");

        /* Attend que le dialog ait tout assimilé */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        vector->first=0;
    }

    if (gtk_dialog_run ((GtkDialog *)dialog) == GTK_RESPONSE_ACCEPT)
    {
        if (load_disk (gtk_file_chooser_get_filename((GtkFileChooser *)dialog), vector) != TO8_ERROR)
        {
            /* Bloque l'intervention de combo_changed */
            g_signal_handler_block (vector->combo, vector->combo_changed_id);

            add_combo_entry (gui->disk[vector->id].file, vector);
            folder_name = gtk_file_chooser_get_current_folder ((GtkFileChooser *)dialog);
            (void)snprintf (gui->default_folder, MAX_PATH, "%s", folder_name);
            g_free (folder_name);

            /* Débloque l'intervention de combo_changed */
            g_signal_handler_unblock (vector->combo, vector->combo_changed_id); 
        }
    }
    gtk_widget_hide(dialog);
    (void)button;
}


/* --------------------------- Partie publique ----------------------------- */


/* free_disk_list:
 *  Libère la mémoire utilisée par les listes de disquettes.
 */
void free_disk_list (void)
{
    int i;

    for (i=0; i<NDISKS; i++)
        free_disk_entry (&vector[i]);
}



/* init_disk_notebook_frame:
 *  Initialise la frame du notebook pour les disquettes.
 */
void init_disk_notebook_frame (GtkWidget *notebook)
{
    int i;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *image;
    GtkWidget *frame;
    char *str;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Disq.":"Disks"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* boîte verticale associée à la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    for (i=0; i<NDISKS; i++)
    {
        memset (&vector[i], 0x00, sizeof (struct FILE_VECTOR));
        vector[i].id=i;
        vector[i].first=1;

        /* boîte horizontale */
        hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        /* label */
        widget=gtk_label_new((str = g_strdup_printf("%d:",i)));
        g_free (str);
        gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE,0);

        /* bouton de vidange */
        widget = gtk_button_new ();
        image = gtk_image_new_from_stock ("gtk-clear", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(widget), image);
        gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
        gtk_widget_set_tooltip_text (widget, is_fr?"Vide la liste des fichiers":"Empty the file list");
        (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(reset_combo), (gpointer)&vector[i]);

        /* boutons protection de la disquette */
        vector[i].check_prot=gtk_check_button_new_with_label("prot.");
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(vector[i].check_prot), gui->disk[i].write_protect);
        g_signal_connect(G_OBJECT(vector[i].check_prot), "toggled", G_CALLBACK(toggle_check_disk), (gpointer)&vector[i]);
        gtk_box_pack_end( GTK_BOX(hbox), vector[i].check_prot, FALSE, TRUE,0);

        /* combobox pour le rappel de disquette */
        vector[i].combo=gtk_combo_box_text_new();
        gtk_box_pack_start( GTK_BOX(hbox), vector[i].combo, TRUE, TRUE,0);
        vector[i].direct=gui->disk[i].direct_access_allowed;
        init_combo (&vector[i]);
        if (strlen (gui->disk[i].file) != 0)
        {
            add_combo_entry (gui->disk[i].file, &vector[i]);
        }
        if (vector[i].direct)
            set_access_mode (&vector[i]);
        vector[i].combo_changed_id = g_signal_connect (G_OBJECT(vector[i].combo), "changed",
                                                       G_CALLBACK(combo_changed), (gpointer)&vector[i]);
        /* bouton d'ouverture de fichier */
        widget = gtk_button_new ();
        image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(widget), image);
        gtk_widget_set_tooltip_text (widget, is_fr?"Ouvrir un fichier disquette":"Open a disk file");
        gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
        (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(open_file), (gpointer)&vector[i]);

    }
}

