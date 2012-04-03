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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2012 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Fran�ois Mouret
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
 *  Cr�� par   : Eric Botcazou juillet 1999
 *  Modifi� par: Eric Botcazou 19/11/2006
 *               Gilles F�tis 27/07/2011
 *               Fran�ois Mouret 07/08/2011 24/03/2012
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
#include "linux/main.h"
#include "to8.h"

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
            error_box (to8_error_msg, wdControl);
            break;

        case TO8_READ_ONLY :
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(vector->check_prot), TRUE);
            break;

        default : break;
    }
    return ret;
}



/* toggle_check_disk:
 *  V�rifie si l'�tat de la protection peut �tre chang�e.
 */
static void toggle_check_disk(GtkWidget *button, struct FILE_VECTOR *vector)
{
    if (GTK_TOGGLE_BUTTON (button)->active )
        to8_SetDiskMode (vector->id, TO8_READ_ONLY);

    else if (to8_SetDiskMode (vector->id, TO8_READ_WRITE)==TO8_READ_ONLY)
    {
        error_box(is_fr?"Ecriture impossible sur ce support."
                       :"Writing unavailable on this device.", wdControl);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
    }
}



/* add_combo_entry
 *  Ajoute une entr�e dans le combobox si inexistante.
 */
static void add_combo_entry (const char *path, struct FILE_VECTOR *vector)
{
    GList *path_node = g_list_find_custom (vector->path_list, (gconstpointer)path, (GCompareFunc)g_strcmp0);

    if (path_node != NULL)
        gtk_combo_box_set_active (GTK_COMBO_BOX(vector->combo), g_list_position (vector->path_list, path_node));
    else
    {
        vector->path_list = g_list_append (vector->path_list, (gpointer)(g_strdup_printf (path,"%s")));
        gtk_combo_box_append_text (GTK_COMBO_BOX(vector->combo), basename((char *)path));
        gtk_combo_box_set_active (GTK_COMBO_BOX(vector->combo), vector->entry_max);
        vector->entry_max++;
    }
}



/* free_disk_entry:
 *  Lib�re la m�moire utilis�e par la liste des disquettes.
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

    ret = to8_VirtualSetDrive(vector->id);
    if ((vector->direct) && (gtk_combo_box_get_active (GTK_COMBO_BOX(vector->combo)) == 1))
        ret = to8_DirectSetDrive(vector->id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vector->check_prot), (ret==TO8_READ_ONLY) ? TRUE : FALSE);
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (struct FILE_VECTOR *vector)
{
    add_combo_entry (is_fr?"-aucune disquette-":"-no disk-", vector);
    if (vector->direct)
        add_combo_entry (is_fr?"-accès direct-":"-direct access-", vector);
}



/* reset_combo:
 *  Ejecte la disquette et vide le combobox (callback).
 *  (sauf l'entr�e "aucune cartouche" et "direct access") 
 */
static void reset_combo (GtkButton *button, struct FILE_VECTOR *vector)
{
    /* Bloque l'intervention de combo_changed */
    g_signal_handler_block (vector->combo, vector->combo_changed_id);

    free_disk_entry (vector);
    for (; vector->entry_max>0; vector->entry_max--)
        gtk_combo_box_remove_text (GTK_COMBO_BOX(vector->combo), (gint)(vector->entry_max-1));
    to8_EjectDisk(vector->id);
    init_combo (vector);
    set_access_mode (vector);

    /* D�bloque l'intervention de combo_changed */
    g_signal_handler_unblock (vector->combo, vector->combo_changed_id);

    (void)button;
}



/* combo_changed:
 *  Changement de s�lection du combobox (callback).
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

    if (vector->first) {
        dialog = gtk_file_chooser_dialog_new (
                 is_fr?"Sélectionner une disquette":"Select a disk",
                 (GtkWindow *) wdControl, GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, is_fr?"Fichiers disquette (.sap)":"Disk files (.sap)");
        gtk_file_filter_add_pattern (filter, "*.sap");
        gtk_file_filter_add_pattern (filter, "*.SAP");
        gtk_file_chooser_add_filter ((GtkFileChooser *)dialog, filter);

        if (strlen (to8_GetDiskFilename (vector->id)) != 0)
            (void)gtk_file_chooser_set_filename((GtkFileChooser *)dialog, to8_GetDiskFilename(vector->id));
        else
        if (access("./disks/", F_OK) == 0)
            (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, "./disks/");

        /* Attend que le dialog ait tout assimil� */
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

            add_combo_entry (to8_GetDiskFilename(vector->id), vector);

            /* D�bloque l'intervention de combo_changed */
            g_signal_handler_unblock (vector->combo, vector->combo_changed_id); 
        }
    }
    gtk_widget_hide(dialog);
    (void)button;
}


/* --------------------------- Partie publique ----------------------------- */


/* free_disk_list:
 *  Lib�re la m�moire utilis�e par les listes de disquettes.
 */
void free_disk_list (void)
{
    int i;

    for (i=0; i<NDISKS; i++)
        free_disk_entry (&vector[i]);
}



/* init_disk_notebook_frame:
 *  Initialise la frame du notebook pour la cartouche.
 */
void init_disk_notebook_frame (GtkWidget *notebook, int direct_disk_support)
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
    widget=gtk_label_new((is_fr?"Disquettes":"Disks"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* bo�te verticale associ�e � la frame */
    vbox=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    for (i=0; i<NDISKS; i++)
    {
        memset (&vector[i], 0x00, sizeof (struct FILE_VECTOR));
        vector[i].id=i;
        vector[i].first=1;

        /* bo�te horizontale */
        hbox=gtk_hbox_new(FALSE, 2);
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
        (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(reset_combo), (gpointer)&vector[i]);

        /* boutons protection de la disquette */
        vector[i].check_prot=gtk_check_button_new_with_label("prot.");
        g_signal_connect(G_OBJECT(vector[i].check_prot), "toggled", G_CALLBACK(toggle_check_disk), (gpointer)&vector[i]);
        gtk_box_pack_end( GTK_BOX(hbox), vector[i].check_prot, FALSE, TRUE,0);

        /* combobox pour le rappel de disquette */
        vector[i].combo=gtk_combo_box_new_text();
        gtk_box_pack_start( GTK_BOX(hbox), vector[i].combo, TRUE, TRUE,0);
        vector[i].direct=(direct_disk_support>>i)&1;
        init_combo (&vector[i]);
        if (strlen(to8_GetDiskFilename(i)))
        {
            add_combo_entry (to8_GetDiskFilename(i), &vector[i]);
        }
        set_access_mode (&vector[i]);
        vector[i].combo_changed_id = g_signal_connect (G_OBJECT(vector[i].combo), "changed",
                                                       G_CALLBACK(combo_changed), (gpointer)&vector[i]);
        /* bouton d'ouverture de fichier */
        widget = gtk_button_new ();
        image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(widget), image);
        gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
        (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(open_file), (gpointer)&vector[i]);

    }
}
