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
 *  Module     : linux/ugui/uprinter.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 18/04/2012
 *  Modifié par: 
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

#include "intern/printer.h"
#include "intern/gui.h"
#include "linux/gui.h"
#include "to8.h"

#define PRINTER_NUMBER 3

struct PRINTER_CODE_LIST {
    char name[9];
    int  number;
};

static struct PRINTER_CODE_LIST prt_list[PRINTER_NUMBER] = {
    { "PR90-055",  55 },
    { "PR90-600", 600 },
    { "PR90-612", 612 }
};




void folder_changed (GtkFileChooser *chooser, gpointer user_data)
{
    gchar *filename = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(chooser));

    (void)snprintf(gui->lprt.folder, MAX_PATH, "%s", filename);
    g_free (filename);
    (void)user_data;
}



void check_button_toggled (GtkToggleButton *button, int *reg)
{
    *reg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}



void combo_changed (GtkComboBox *combo, gpointer user_data)
{
    gui->lprt.number = (prt_list[gtk_combo_box_get_active (GTK_COMBO_BOX(combo))].number);
    (void)user_data;
}



/* init_printer_notebook_frame:
 *  Initialise la frame du notebook pour la cartouche.
 */
void init_printer_notebook_frame (GtkWidget *notebook)
{
    int i;
    int combo_index = 0;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *dialog;
    GtkWidget *frame;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Imprimante":"Printer"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* boîte verticale associée à la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* label pour le répertoire */
    widget=gtk_label_new(is_fr?"Sauver dans : ":"Save in : ");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* Dialog pour le répertoire */
    dialog = gtk_file_chooser_dialog_new (
             is_fr?"SÃ©lectionner un rÃ©pertoire de sauvegarde":"Select the folder to save in",
             (GtkWindow *) wControl, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, gui->lprt.folder);
    g_signal_connect(G_OBJECT(dialog), "current-folder-changed", G_CALLBACK(folder_changed), (gpointer) NULL);
    widget = gtk_file_chooser_button_new_with_dialog (dialog);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* label pour l'imprimante */
    widget=gtk_label_new(is_fr?"Imprimante :":"Printer :");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* combo pour les imprimantes */
    widget=gtk_combo_box_text_new();
    for (i=0; i<PRINTER_NUMBER; i++)
    {
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(widget), NULL, prt_list[i].name);
        if (gui->lprt.number == prt_list[i].number)
            combo_index = i;
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX(widget), combo_index);
    g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(combo_changed), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* bouton check pour le dip */
    widget=gtk_check_button_new_with_label("dip");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), gui->lprt.dip);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(check_button_toggled), (gpointer) &gui->lprt.dip);
    gtk_widget_set_tooltip_text (widget, is_fr?"Change le comportement de CR":"Change the behavior of CR");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour le nlq */
    widget=gtk_check_button_new_with_label("nlq");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), gui->lprt.nlq);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(check_button_toggled), (gpointer) &gui->lprt.nlq);
    gtk_widget_set_tooltip_text (widget, is_fr?"Imprime en haute qualitÃ©":"High-quality print");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    
    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* boîte de centrage */
    widget=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* label pour le type de sortie */
    widget=gtk_label_new(is_fr?"Sortie :":"Output :");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour la sortie brute */
    widget=gtk_check_button_new_with_label(is_fr?"brute":"raw");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), gui->lprt.raw_output);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(check_button_toggled), (gpointer) &gui->lprt.raw_output);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour la sortie texte */
    widget=gtk_check_button_new_with_label(is_fr?"texte":"text");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), gui->lprt.txt_output);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(check_button_toggled), (gpointer) &gui->lprt.txt_output);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour la sortie graphique */
    widget=gtk_check_button_new_with_label(is_fr?"graphique":"graphic");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), gui->lprt.gfx_output);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(check_button_toggled), (gpointer) &gui->lprt.gfx_output);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* boîte de centrage */
    widget=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);
}

