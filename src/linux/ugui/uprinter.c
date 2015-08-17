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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/ugui/uprinter.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 18/04/2012
 *  Modifié par: François Mouret 29/10/2012 13/04/2014 31/05/2015
 *
 *  Gestion des imprimantes.
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
#include "media/printer.h"
#include "linux/gui.h"

GtkWidget *widget_nlq = NULL;
GtkWidget *widget_dip = NULL;



/* folder_changed:
 *  Callback pour le changement de répertoire.
 */
static void folder_changed (GtkFileChooser *chooser, gpointer user_data)
{
    gchar *filename = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(chooser));
    teo.lprt.folder = std_free (teo.lprt.folder);
    teo.lprt.folder = std_strdup_printf ("%s", filename);
    g_free (filename);
    (void)user_data;
}



/* check_button_toggled:
 *  Callback commun pour un changement d'état de checkbox.
 */
static void check_button_toggled (GtkToggleButton *button, int *reg)
{
    *reg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}



/* combo_changed:
 *  Callback pour le changement de combo.
 */
static void combo_changed (GtkComboBox *combo, gpointer user_data)
{
    int combo_index = gtk_combo_box_get_active (GTK_COMBO_BOX(combo));

    teo.lprt.number = (printer_code_list[combo_index].number);
    if (teo.lprt.number < 600)
    {
        gtk_widget_set_sensitive (widget_nlq, FALSE);
        gtk_widget_set_sensitive (widget_dip, FALSE);
    }
    else
    {
        gtk_widget_set_sensitive (widget_nlq, TRUE);
        gtk_widget_set_sensitive (widget_dip, TRUE);
    }

    (void)user_data;
}



static GtkWidget *create_new_frame (GtkWidget *mainbox, const char *title)
{
    GtkWidget *vbox;
    GtkWidget *frame;

    /* frame */
    frame=gtk_frame_new (title);
    gtk_box_pack_start (GTK_BOX(mainbox), frame, TRUE, TRUE, 0);

    /* vertical box */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    return vbox;
}



static GtkWidget *create_new_hbox (GtkWidget *mainbox)
{
    GtkWidget *hbox;

    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_container_add( GTK_CONTAINER(mainbox), hbox);

    return hbox;
}


/* ------------------------------------------------------------------------- */


/* uprinter_Init:
 *  Initialise la frame du notebook pour la cartouche.
 */
void uprinter_Init (GtkWidget *notebook)
{
    int i;
    int combo_index = 0;
    GtkWidget *vbox, *vbox2;
    GtkWidget *hbox, *hbox2;
    GtkWidget *widget;
    GtkWidget *combo;
    GtkWidget *dialog;
    GtkWidget *frame;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Imprimante":"Printer"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* boîte verticale associée à la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 0);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    hbox2 = create_new_hbox (vbox);
    widget=gtk_label_new(is_fr?"Ecrire les fichiers de sortie dans"
                              :"Write output files in");
    gtk_box_pack_start( GTK_BOX(hbox2), widget, FALSE, FALSE, 0);

    /* dialog pour le répertoire */
    dialog = gtk_file_chooser_dialog_new (
             is_fr?"SÃ©lectionner un rÃ©pertoire de sauvegarde"
                  :"Select the folder to save in",
             (GtkWindow *) wControl,
             GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
             is_fr?"_Annuler":"_Cancel", GTK_RESPONSE_CANCEL,
             is_fr?"_Ouvrir":"_Open", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder(
                     (GtkFileChooser *)dialog,
                     (teo.lprt.folder == NULL)?"":teo.lprt.folder);
    g_signal_connect(G_OBJECT(dialog),
                     "current-folder-changed",
                     G_CALLBACK(folder_changed),
                     (gpointer) NULL);
    widget = gtk_file_chooser_button_new_with_dialog (dialog);
    gtk_box_pack_start( GTK_BOX(hbox2), widget, FALSE, FALSE, 0);

    /* ---------------- Options ------------------- */

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_container_set_border_width( GTK_CONTAINER(hbox), 5);
    gtk_box_set_homogeneous (GTK_BOX(hbox), FALSE);
    gtk_container_add( GTK_CONTAINER(vbox), hbox);

    /* Création de la frame */
    vbox2 = create_new_frame (hbox, is_fr?"Imprimante":"Printer");

    /* label pour l'imprimante */
    hbox2 = create_new_hbox (vbox2);
    widget=gtk_label_new("Type ");
    gtk_box_pack_start( GTK_BOX(hbox2), widget, FALSE, FALSE, 0);

    /* combo pour les imprimantes */
    combo=gtk_combo_box_text_new();
    for (i=0; i<PRINTER_NUMBER; i++)
    {
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(combo),
                                   NULL,
                                   printer_code_list[i].name);
        if (teo.lprt.number == printer_code_list[i].number)
            combo_index = i;
    }
    g_signal_connect(G_OBJECT(combo),
                     "changed",
                     G_CALLBACK(combo_changed),
                     (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox2), combo, TRUE, TRUE, 0);

    /* bouton check pour le dip */
    hbox2 = create_new_hbox (vbox2);
    widget_dip=gtk_check_button_new_with_label(
                        is_fr?"Double interligne"
                             :"Double spacing");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_dip),
                                 teo.lprt.dip);
    g_signal_connect(G_OBJECT(widget_dip),
                     "toggled",
                     G_CALLBACK(check_button_toggled),
                     (gpointer) &teo.lprt.dip);
    gtk_box_pack_start( GTK_BOX(hbox2), widget_dip, TRUE, TRUE, 0);

    /* bouton check pour le nlq */
    hbox2 = create_new_hbox (vbox2);
    widget_nlq=gtk_check_button_new_with_label(
                        is_fr?"Imprime en haute qualitÃ©"
                             :"High-quality print");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_nlq),
                                 teo.lprt.nlq);
    g_signal_connect(G_OBJECT(widget_nlq),
                     "toggled",
                     G_CALLBACK(check_button_toggled),
                     (gpointer) &teo.lprt.nlq);
    gtk_box_pack_start( GTK_BOX(hbox2), widget_nlq, TRUE, TRUE, 0);

    /* ---------------- Output ------------------- */

    /* Création de la frame */
    vbox2 = create_new_frame (hbox, is_fr?"Sortie":"Output");

    /* label pour le répertoire */
    /* bouton check pour la sortie brute */
    hbox2 = create_new_hbox (vbox2);
    widget=gtk_check_button_new_with_label(is_fr?"brute":"raw");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                 teo.lprt.raw_output);
    g_signal_connect(G_OBJECT(widget),
                     "toggled",
                     G_CALLBACK(check_button_toggled),
                     (gpointer) &teo.lprt.raw_output);
    gtk_box_pack_start( GTK_BOX(hbox2), widget, TRUE, TRUE, 0);

    /* bouton check pour la sortie texte */
    hbox2 = create_new_hbox (vbox2);
    widget=gtk_check_button_new_with_label(is_fr?"texte":"text");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                 teo.lprt.txt_output);
    g_signal_connect(G_OBJECT(widget),
                     "toggled",
                     G_CALLBACK(check_button_toggled),
                     (gpointer) &teo.lprt.txt_output);
    gtk_box_pack_start( GTK_BOX(hbox2), widget, TRUE, TRUE, 0);

    /* bouton check pour la sortie graphique */
    hbox2 = create_new_hbox (vbox2);
    widget=gtk_check_button_new_with_label(is_fr?"graphique":"graphic");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                 teo.lprt.gfx_output);
    g_signal_connect(G_OBJECT(widget),
                     "toggled",
                     G_CALLBACK(check_button_toggled),
                     (gpointer) &teo.lprt.gfx_output);
    gtk_box_pack_start( GTK_BOX(hbox2), widget, TRUE, TRUE, 0);

    /* actualize the combo */
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo), combo_index);
}

