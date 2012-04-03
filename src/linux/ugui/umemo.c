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
 *  Module     : linux/ugui/umemo.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 07/08/2011 24/03/2012
 *
 *  Gestion des cartouches.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <string.h>
   #include <gtk/gtk.h>
#endif

#include "linux/gui.h"
#include "linux/main.h"
#include "to8.h"

static GtkWidget *combo;
static int entry_max=0;
static gulong combo_changed_id;
static GList *name_list = NULL;
static GList *path_list = NULL;



/* add_combo_entry:
 *  Ajoute une entrée dans le combobox si inexistante.
 *  et sélectionne l'entrée demandée
 */
static void add_combo_entry (const char *name, const char *path)
{
    GList *path_node = g_list_find_custom (path_list, (gconstpointer)path, (GCompareFunc)g_strcmp0);
    GList *name_node = g_list_find_custom (name_list, (gconstpointer)name, (GCompareFunc)g_strcmp0);
    gint path_index = g_list_position (path_list, path_node);
    gint name_index = g_list_position (name_list, name_node);

    if ((path_node != NULL) && (name_index == path_index))
        gtk_combo_box_set_active (GTK_COMBO_BOX(combo), name_index);
    else
    {
        name_list = g_list_append (name_list, (gpointer)(g_strdup_printf (name,"%s")));
        path_list = g_list_append (path_list, (gpointer)(g_strdup_printf (path,"%s")));
        gtk_combo_box_append_text (GTK_COMBO_BOX(combo), name);
        gtk_combo_box_set_active (GTK_COMBO_BOX(combo), entry_max);
        entry_max++;
    }
    teo.command = COLD_RESET;
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (void)
{
    add_combo_entry (is_fr?"-aucune cartouche-":"-no cartridge-", "");
}



/* reset_combo:
 *  Ejecte la mémo7 et vide le combobox (sauf l'entrée "aucune cartouche") (callback).
 */
static void reset_combo (GtkButton *button, gpointer data)
{
    /* Bloque l'intervention de combo_changed */
    g_signal_handler_block (combo, combo_changed_id);

    free_memo_list ();
    for (; entry_max>0; entry_max--)
        gtk_combo_box_remove_text (GTK_COMBO_BOX(combo), (gint)(entry_max-1));
    init_combo();
    to8_EjectMemo7();

    /* Débloque l'intervention de combo_changed */
    g_signal_handler_unblock (combo, combo_changed_id);

    (void)button;
    (void)data;
}



/* combo_changed:
 *  Changement de sélection du combobox (callback).
 */
static void combo_changed (GtkComboBox *combo_box, gpointer data)
{
    char *filename;
    
    filename = (char *) g_list_nth_data (path_list, (guint)gtk_combo_box_get_active (combo_box));

    if (*filename == '\0')
        to8_EjectMemo7();
    else
    if (to8_LoadMemo7(filename) == TO8_ERROR)
        error_box(to8_error_msg, wdControl);

    teo.command=COLD_RESET;

    (void)data;
}



/* open_file:
 *  Charge une nouvelle cartouche.
 */
static void open_file (GtkButton *button, gpointer data)
{
    static int first=1;
    GtkFileFilter *filter;
    static GtkWidget *dialog;

    if (first) {
        dialog = gtk_file_chooser_dialog_new (
                 is_fr?"SÃ©lectionner une cartouche":"Select a cartridge",
                 (GtkWindow *) wdControl, GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, is_fr?"Fichiers cartouche (.m7)":"Cartridge files (.m7)");
        gtk_file_filter_add_pattern (filter, "*.m7");
        gtk_file_filter_add_pattern (filter, "*.M7");
        gtk_file_chooser_add_filter ((GtkFileChooser *)dialog, filter);

        if (strlen (to8_GetMemo7Filename()) != 0)
            (void)gtk_file_chooser_set_filename((GtkFileChooser *)dialog, to8_GetMemo7Filename());
        else
        if (access("./memo7/", F_OK) == 0)
            (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, "./memo7/");

        /* Attend que le dialog ait tout assimilé */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        first=0;
    }

    if (gtk_dialog_run ((GtkDialog *)dialog) == GTK_RESPONSE_ACCEPT)
    {
        if (to8_LoadMemo7(gtk_file_chooser_get_filename((GtkFileChooser *)dialog)) == TO8_ERROR)
            error_box(to8_error_msg, wdControl);
        else
        {
            /* Bloque l'intervention de combo_changed */
            g_signal_handler_block (combo, combo_changed_id);

            add_combo_entry (to8_GetMemo7Label(), to8_GetMemo7Filename());

            /* Débloque l'intervention de combo_changed */
            g_signal_handler_unblock (combo, combo_changed_id); 
        }
    }
    gtk_widget_hide(dialog);
    (void)button;
    (void)data;
}


/* --------------------------- Partie publique ----------------------------- */


/* free_memo_list
 *  Libère la mémoire utilisée par la liste des cartouches.
 */
void free_memo_list (void)
{
    g_list_foreach (name_list, (GFunc)g_free, (gpointer) NULL);
    g_list_foreach (path_list, (GFunc)g_free, (gpointer) NULL);
    g_list_free (name_list);
    g_list_free (path_list);
    name_list=NULL;
    path_list=NULL;
}



/* init_memo_notebook_frame:
 *  Initialise la frame du notebook pour la cartouche.
 */
void init_memo_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *image;
    GtkWidget *frame;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Cartouche":"Cartridge"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* boîte verticale associée à la frame */
    vbox=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton de vidange */
    widget = gtk_button_new ();
    image = gtk_image_new_from_stock ("gtk-clear", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(widget), image);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(reset_combo), (gpointer) NULL);

    /* combobox pour le rappel de cartouche */
    combo=gtk_combo_box_new_text();
    gtk_box_pack_start( GTK_BOX(hbox), combo, TRUE, TRUE,0);
    init_combo();
    if (strlen(to8_GetMemo7Label()))
        add_combo_entry (to8_GetMemo7Label(), to8_GetMemo7Filename());
    combo_changed_id = g_signal_connect (G_OBJECT(combo), "changed", G_CALLBACK(combo_changed), (gpointer) NULL);

    /* bouton d'ouverture de fichier */
    widget = gtk_button_new ();
    image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(widget), image);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(open_file), (gpointer) NULL);
}

