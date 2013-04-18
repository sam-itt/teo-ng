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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/ugui/ucass.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 07/08/2011 24/03/2012 12/06/2012
 *                               04/11/2012
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

#include "teo.h"
#include "std.h"
#include "error.h"
#include "media/cass.h"
#include "linux/gui.h"

#define COUNTER_MAX  999

static GtkWidget *combo;
static gulong combo_id;
static GtkWidget *check_prot;
static gulong check_prot_id;
static GtkWidget *emptying_button;
static gulong emptying_button_id;
static GtkWidget *counter_box;
static int entry_max=0;
static GtkWidget *spinner_cass;
static GList *path_list = NULL;



static void block_all (void)
{
    g_signal_handler_block (combo, combo_id);
    g_signal_handler_block (check_prot, check_prot_id);
    g_signal_handler_block (emptying_button, emptying_button_id);
}



static void unblock_all (void)
{
    g_signal_handler_unblock (combo, combo_id);
    g_signal_handler_unblock (check_prot, check_prot_id);
    g_signal_handler_unblock (emptying_button, emptying_button_id);
}



/* update_params:
 *  Ajuste les paramètres de cassette.
 */
static void update_params (void)
{
    int combo_index;
    
    if (combo_id != 0)
    {
        block_all ();

        combo_index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

        if (combo_index == 0)
        {
            gtk_widget_set_sensitive (emptying_button, FALSE);
            gtk_widget_set_sensitive (check_prot, FALSE);
            gtk_widget_set_sensitive (counter_box, FALSE);
        }
        else
        {
            gtk_widget_set_sensitive (emptying_button, TRUE);
            gtk_widget_set_sensitive (check_prot, TRUE);
            gtk_widget_set_sensitive (counter_box, TRUE);
        }

        unblock_all ();
    }
}



/* set_counter_cass:
 *  Positionne le compteur de cassette.
 */
static void set_counter_cass (void)
{
   gtk_spin_button_set_value((GtkSpinButton *)spinner_cass, cass_GetCounter());
}



/* rewind_cass:
 *  Met le compteur de cassette à 0.
 */
static void rewind_cass (void)
{
   cass_SetCounter(0);
   set_counter_cass ();
}



/* eject_cass:
 *  Ejecte la cassette.
 */
static void eject_cass (void)
{
    rewind_cass ();
    cass_Eject();
    update_params ();
}



/* click_rewind_cass:
 *  Met le compteur de cassette à 0 (callback).
 */
static void click_rewind_cass (GtkWidget *button, gpointer data)
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
    int ret = cass_Load (filename);

    rewind_cass();
    switch (ret)
    {
        case TEO_ERROR :
            ugui_Error (teo_error_msg, wControl);
            break;

        case TRUE :
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check_prot), TRUE);
            teo.cass.write_protect = TRUE;
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
        cass_SetProtection(TRUE);
        teo.cass.write_protect = TRUE;
    }
    else
    {
        teo.cass.write_protect = FALSE;

        if (cass_SetProtection(FALSE) == TRUE)
        {
            ugui_Error ((is_fr?"Ecriture impossible sur ce support."
                              :"Writing unavailable on this device."), wControl);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
            teo.cass.write_protect = TRUE;
        }
    }
    set_counter_cass ();
    update_params ();
    (void)data;
}



/* change_counter_cass:
 *  Change le compteur de la cassette (callback).
 */
static void change_counter_cass (GtkWidget *widget, gpointer data)
{
   cass_SetCounter(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinner_cass)));
   (void)widget;
   (void)data;
}



/* add_combo_entry
 *  Ajoute une entrée dans le combobox si inexistante.
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
    update_params ();
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
    block_all ();

    ucass_Free ();
    gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT(combo));
    eject_cass ();
    init_combo ();

    unblock_all ();

    update_params ();

    (void)button;
    (void)data;
}



/* combo_changed:
 *  Changement de sélection du combobox (callback).
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
    update_params ();
    
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
    gchar *folder_name;
    gchar *file_name;

    if (first) {
        dialog = gtk_file_chooser_dialog_new (
                 is_fr?"SÃ©lectionner une cassette":"Select a tape",
                 (GtkWindow *) wControl, GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, is_fr?"Fichiers cassette (.k7)":"Tape files (.k7)");
        gtk_file_filter_add_pattern (filter, "*.k7");
        gtk_file_filter_add_pattern (filter, "*.K7");
        gtk_file_chooser_add_filter ((GtkFileChooser *)dialog, filter);

        /* Attend que le dialog ait tout assimilé */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        first=0;
    }

    if (teo.cass.file != NULL)
    {
        folder_name = std_strdup_printf ("%s", teo.cass.file);
        std_CleanPath (folder_name);
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, folder_name);
        folder_name = std_free (folder_name);
    }
    else
    if (teo.default_folder != NULL)
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, teo.default_folder);
    else
    if (access("./disks/", F_OK) == 0)
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, "./k7/");

    if (gtk_dialog_run ((GtkDialog *)dialog) == GTK_RESPONSE_ACCEPT)
    {
        file_name = gtk_file_chooser_get_filename((GtkFileChooser *)dialog);
        if (load_cass (file_name) >= 0)
        {
            block_all ();

            add_combo_entry (teo.cass.file);
            folder_name = gtk_file_chooser_get_current_folder ((GtkFileChooser *)dialog);
            teo.default_folder = std_free(teo.default_folder);
            if (folder_name != NULL)
                teo.default_folder = std_strdup_printf ("%s",folder_name);
            g_free (folder_name);

            unblock_all ();
            update_params ();
        }
        g_free (file_name);
    }
    gtk_widget_hide(dialog);
    (void)button;
    (void)data;
}


/* --------------------------- Partie publique ----------------------------- */


/* ucass_Free
 *  Libère la mémoire utilisée par la liste des cassettes.
 */
void ucass_Free (void)
{
    g_list_foreach (path_list, (GFunc)g_free, (gpointer) NULL);
    g_list_free (path_list);
    path_list=NULL;
    entry_max = 0;
}



/* ucass_UpdateCounter:
 *  Positionne le compteur de cassette.
 */ 
void ucass_UpdateCounter (void)
{
    set_counter_cass ();
}



/* ucass_Init:
 *  Initialise la frame du notebook pour la cassette.
 */
void ucass_Init (GtkWidget *notebook)
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

    /* boîte verticale associée à la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton de vidange */
    emptying_button = gtk_button_new ();
    image = gtk_image_new_from_stock ("gtk-clear", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(emptying_button), image);
    gtk_widget_set_tooltip_text (emptying_button,
                                 is_fr?"Vide la liste des fichiers"
                                       :"Empty the file list");
    gtk_box_pack_start( GTK_BOX(hbox), emptying_button, FALSE, FALSE, 0);
    emptying_button_id = g_signal_connect(G_OBJECT(emptying_button),
                                          "clicked",
                                          G_CALLBACK(reset_combo),
                                          (gpointer) NULL);

    /* combobox pour le rappel de cassette */
    combo=gtk_combo_box_text_new();
    gtk_box_pack_start( GTK_BOX(hbox), combo, TRUE, TRUE,0);
    init_combo ();
    if (teo.cass.file != NULL)
        add_combo_entry (teo.cass.file);
    combo_id = g_signal_connect (G_OBJECT(combo),
                                         "changed",
                                         G_CALLBACK(combo_changed),
                                         (gpointer) NULL);

    /* bouton protection de la cassette */
    check_prot=gtk_check_button_new_with_label("prot.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_prot),
                                 teo.cass.write_protect);
    check_prot_id = g_signal_connect(G_OBJECT(check_prot),
                                     "toggled",
                                     G_CALLBACK(toggle_check_cass),
                                     (gpointer)NULL);
    gtk_box_pack_end( GTK_BOX(hbox), check_prot, FALSE, TRUE, 0);

    /* bouton d'ouverture de fichier */
    widget = gtk_button_new ();
    image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(widget), image);
    gtk_widget_set_tooltip_text (widget, is_fr?"Ouvrir un fichier cassette"
                                              :"Open a tape file");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    (void)g_signal_connect(G_OBJECT(widget),
                           "clicked",
                           G_CALLBACK(open_file),
                           (gpointer) check_prot);

    /* molette du compteur de cassette */
    adjustment = gtk_adjustment_new (0, 0, COUNTER_MAX, 1, 10, 0);
    spinner_cass = gtk_spin_button_new (adjustment, 0.5, 0);

    /* seconde boîte horizontale de la cassette */
    counter_box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(vbox), counter_box, FALSE, FALSE, 0);

    /* molette du compteur de cassette */
    widget=gtk_label_new((is_fr?"Compteur:":"Counter:"));
    gtk_box_pack_start (GTK_BOX (counter_box), widget, FALSE, FALSE, 0);

    /* Connecter le signal directement au spin button ne marche pas. */
    g_signal_connect(G_OBJECT(adjustment),
                     "value_changed",
                     G_CALLBACK(change_counter_cass),
                     (gpointer)NULL);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner_cass), FALSE);
    gtk_box_pack_start (GTK_BOX (counter_box), spinner_cass, FALSE, FALSE, 0);

    /* bouton rembobinage */
    widget=gtk_button_new_with_label((is_fr?"Rembobiner la cassette":"Rewind tape"));
    g_signal_connect(G_OBJECT(widget), "clicked",
                     G_CALLBACK(click_rewind_cass), (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(counter_box), widget, TRUE, FALSE, 0);

    update_params ();
}

