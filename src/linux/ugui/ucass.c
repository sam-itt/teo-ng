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
 *  Module     : linux/ugui/ucass.c
 *  Version    : 1.8.1
 *  Cr�� par   : Eric Botcazou juillet 1999
 *  Modifi� par: Eric Botcazou 19/11/2006
 *               Gilles F�tis 27/07/2011
 *               Fran�ois Mouret 07/08/2011 24/03/2012 12/06/2012
 *
 *  Gestion des cassettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <string.h>
   #include <libgen.h>
   #include <gtk/gtk.h>
#endif

#include "intern/gui.h"
#include "linux/gui.h"
#include "to8.h"

#define COUNTER_MAX  999

static GtkWidget *combo;
static int entry_max=0;
static gulong combo_changed_id;
static GtkWidget *check_prot;
static GtkWidget *spinner_cass;
static GList *path_list = NULL;


/* set_counter_cass:
 *  Positionne le compteur de cassette.
 */
static void set_counter_cass (void)
{
   gtk_spin_button_set_value((GtkSpinButton *)spinner_cass, to8_GetK7Counter());
}



/* rewind_cass:
 *  Met le compteur de cassette � 0.
 */
static void rewind_cass (void)
{
   to8_SetK7Counter(0);
   set_counter_cass ();
}



/* eject_cass:
 *  Ejecte la cassette.
 */
static void eject_cass (void)
{
    rewind_cass ();
    to8_EjectK7();
}



/* click_rewind_cass:
 *  Met le compteur de cassette � 0 (callback).
 */static void click_rewind_cass (GtkWidget *button, gpointer data)
{
    rewind_cass ();
    (void)button;
    (void)data;
}



/* load_cass:
 *  Charge une cassette.
 */
static int load_cass (gchar *filename)
{
    int ret = to8_LoadK7 (filename);

    rewind_cass();
    switch (ret)
    {
        case TO8_ERROR :
            error_box (to8_error_msg, wControl);
            break;

        case TO8_READ_ONLY :
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check_prot), TRUE);
            gui->cass.write_protect = TRUE;
            break;

        default : break;
    }
    return ret;
}



/* toggle_check_cass:
 *  Gestion de la protection (callback).
 */
static void toggle_check_cass (GtkWidget *button, gpointer data)
{
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    {
        to8_SetK7Mode(TO8_READ_ONLY);
        gui->cass.write_protect = TRUE;
    }
    else
    {
        gui->cass.write_protect = FALSE;

        if (to8_SetK7Mode(TO8_READ_WRITE)==TO8_READ_ONLY)
        {
            error_box((is_fr?"Ecriture impossible sur ce support."
                            :"Writing unavailable on this device."), wControl);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
            gui->cass.write_protect = TRUE;
        }
    }
    set_counter_cass ();
    (void)data;
}



/* change_counter_cass:
 *  Change le compteur de la cassette (callback).
 */
static void change_counter_cass (GtkWidget *widget, gpointer data)
{
   to8_SetK7Counter(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinner_cass)));
   (void)widget;
   (void)data;
}



/* add_combo_entry
 *  Ajoute une entr�e dans le combobox si inexistante.
 */
static void add_combo_entry (const char *path)
{
    GList *path_node = g_list_find_custom (path_list, (gconstpointer)path, (GCompareFunc)g_strcmp0);

    if (path_node != NULL)
    {
        gtk_combo_box_set_active (GTK_COMBO_BOX(combo), g_list_position (path_list, path_node));
    }   
    else
    {
        path_list = g_list_append (path_list, (gpointer)(g_strdup_printf (path,"%s")));
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(combo), NULL, (gchar *)basename((char *)path));
        gtk_combo_box_set_active (GTK_COMBO_BOX(combo), entry_max);
        entry_max++;
    }
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (void)
{
    add_combo_entry (is_fr?"(Aucun)":"(None)");
}



/* reset_combo:
 *  Vide le combobox (callback).
 */
static void reset_combo (GtkButton *button, gpointer data)
{
    /* Bloque l'intervention de combo_changed */
    g_signal_handler_block (combo, combo_changed_id);

    free_cass_list ();
    gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT(combo));
    eject_cass ();
    init_combo ();

    /* D�bloque l'intervention de combo_changed */
    g_signal_handler_unblock (combo, combo_changed_id);

    (void)button;
    (void)data;
}



/* combo_changed:
 *  Changement de s�lection du combobox (callback).
 */
static void combo_changed (GtkComboBox *combo_box, gpointer data)
{
    int entry_selected = gtk_combo_box_get_active (combo_box);

    if (entry_selected == 0)
    {
        eject_cass ();
    }
    else
    {
        (void)load_cass((char *)g_list_nth_data (path_list, (guint)entry_selected));
    }
    
    (void)data;
}



/* open_file:
 *  Charge une nouvelle cassette (callback).
 */
static void open_file (GtkButton *button, gpointer data)
{
    static int first=1;
    GtkFileFilter *filter;
    static GtkWidget *dialog;
    char *folder_name;

    if (first) {
        dialog = gtk_file_chooser_dialog_new (
                 is_fr?"Sélectionner une cassette":"Select a tape",
                 (GtkWindow *) wControl, GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, is_fr?"Fichiers cassette (.k7)":"Tape files (.k7)");
        gtk_file_filter_add_pattern (filter, "*.k7");
        gtk_file_filter_add_pattern (filter, "*.K7");
        gtk_file_chooser_add_filter ((GtkFileChooser *)dialog, filter);

        if (strlen (gui->cass.file) != 0)
            (void)gtk_file_chooser_set_filename((GtkFileChooser *)dialog, gui->cass.file);
        else
        if (strlen (gui->default_folder) != 0)
            (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, gui->default_folder);
        else
        if (access("./k7/", F_OK) == 0)
            (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, "./k7/");

        /* Attend que le dialog ait tout assimil� */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        first=0;
    }

    if (gtk_dialog_run ((GtkDialog *)dialog) == GTK_RESPONSE_ACCEPT)
    {
        if (load_cass (gtk_file_chooser_get_filename((GtkFileChooser *)dialog)) != TO8_ERROR)
        {
            /* Bloque l'intervention de combo_changed */
            g_signal_handler_block (combo, combo_changed_id);

            add_combo_entry (gui->cass.file);
            folder_name = gtk_file_chooser_get_current_folder ((GtkFileChooser *)dialog);
            (void)snprintf (gui->default_folder, MAX_PATH, "%s", folder_name);
            g_free (folder_name);

            /* D�bloque l'intervention de combo_changed */
            g_signal_handler_unblock (combo, combo_changed_id); 
        }
    }
    gtk_widget_hide(dialog);
    (void)button;
    (void)data;
}


/* --------------------------- Partie publique ----------------------------- */


/* free_cass_list
 *  Lib�re la m�moire utilis�e par la liste des cartouches.
 */
void free_cass_list (void)
{
    g_list_foreach (path_list, (GFunc)g_free, (gpointer) NULL);
    g_list_free (path_list);
    path_list=NULL;
    entry_max = 0;
}



/* update_counter_cass:
 *  Positionne le compteur de cassette.
 */ 
void update_counter_cass (void)
{
    set_counter_cass ();
}



/* init_cass_notebook_frame:
 *  Initialise la frame du notebook pour la cassette.
 */
void init_cass_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *image;
    GtkWidget *frame;
    GtkAdjustment *adjustment;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Cassette":"Tape"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* bo�te verticale associ�e � la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* bo�te horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton de vidange */
    widget = gtk_button_new ();
    image = gtk_image_new_from_stock ("gtk-clear", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(widget), image);
    gtk_widget_set_tooltip_text (widget, is_fr?"Vide la liste des fichiers":"Empty the file list");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(reset_combo), (gpointer) NULL);

    /* combobox pour le rappel de cassette */
    combo=gtk_combo_box_text_new();
    gtk_box_pack_start( GTK_BOX(hbox), combo, TRUE, TRUE,0);

    init_combo ();
    printf ("'%s'\n", gui->cass.file);
    if (strlen(gui->cass.file) != 0)
        add_combo_entry (gui->cass.file);
    combo_changed_id = g_signal_connect (G_OBJECT(combo), "changed", G_CALLBACK(combo_changed), (gpointer) NULL);

    /* bouton protection de la cassette */
    check_prot=gtk_check_button_new_with_label("prot.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_prot), gui->cass.write_protect);
    g_signal_connect(G_OBJECT(check_prot), "toggled", G_CALLBACK(toggle_check_cass), (gpointer)NULL);
    gtk_box_pack_end( GTK_BOX(hbox), check_prot, FALSE, TRUE, 0);

    /* bouton d'ouverture de fichier */
    widget = gtk_button_new ();
    image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(widget), image);
    gtk_widget_set_tooltip_text (widget, is_fr?"Ouvrir un fichier cassette":"Open a tape file");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    (void)g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(open_file), (gpointer) check_prot);

    /* molette du compteur de cassette */
    adjustment = gtk_adjustment_new (0, 0, COUNTER_MAX, 1, 10, 0);
    spinner_cass = gtk_spin_button_new (adjustment, 0.5, 0);

    /* seconde bo�te horizontale de la cassette */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* molette du compteur de cassette */
    widget=gtk_label_new((is_fr?"Compteur:":"Counter:"));
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

    /* Connecter le signal directement au spin button ne marche pas. */
    g_signal_connect(G_OBJECT(adjustment), "value_changed", G_CALLBACK(change_counter_cass), (gpointer)NULL);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner_cass), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), spinner_cass, FALSE, FALSE, 0);

    /* bouton rembobinage */
    widget=gtk_button_new_with_label((is_fr?"Rembobiner la cassette":"Rewind tape"));
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(click_rewind_cass), (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
}

