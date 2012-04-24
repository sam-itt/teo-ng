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
 *  Version    : 1.8.1
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
#include "linux/gui.h"
#include "linux/main.h"
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




/* change_printer:
 *  Change l'imprimante.
 */
static void change_printer (GtkWidget *combo, gpointer data)
{
    printer_SetNumber(prt_list[gtk_combo_box_get_active (GTK_COMBO_BOX(combo))].number);
    (void)data;
}



/* toggle_raw_output:
 *  Positionne le type de sortie RAW.
 */
static void toggle_raw_output (GtkWidget *button, gpointer data)
{
    printer_SetRawOutput(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
    (void)data;
}



/* toggle_txt_output:
 *  Positionne le type de sortie TXT.
 */
static void toggle_txt_output (GtkWidget *button, gpointer data)
{
    printer_SetTxtOutput(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
    (void)data;
}



/* toggle_gfx_output:
 *  Positionne le type de sortie GFX.
 */
static void toggle_gfx_output (GtkWidget *button, gpointer data)
{
    printer_SetGfxOutput(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
    (void)data;
}



/* toggle_nlq:
 *  Positionne l'impression en NLQ.
 */
static void toggle_nlq (GtkWidget *button, gpointer data)
{
    printer_SetNlq(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
    (void)data;
}



/* toggle_dip:
 *  Positionne le commutateur DIP.
 */
static void toggle_dip (GtkWidget *button, gpointer data)
{
    printer_SetDip(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
    (void)data;
}



static void set_printer_folder (GtkFileChooser *chooser, gpointer user_data)
{
    gchar *filename = gtk_file_chooser_get_filename (chooser);
    
    to8_SetPrinterFolder (filename);
    g_free (filename);
    (void)user_data;
}


/* --------------------------- Partie publique ----------------------------- */


/* init_printer_notebook_frame:
 *  Initialise la frame du notebook pour la cartouche.
 */
void init_printer_notebook_frame (GtkWidget *notebook)
{
    int i;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *dialog;
    GtkWidget *frame;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Impr.":"Printer"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* boîte verticale associée à la frame */
    vbox=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* label pour le répertoire */
    widget=gtk_label_new(is_fr?"Sauver dans : ":"Save in : ");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* Dialog pour le répertoire */
    dialog = gtk_file_chooser_dialog_new (
             is_fr?"SÃ©lectionner un rÃ©pertoire de sauvegarde":"Select the folder to save in",
             (GtkWindow *) wdControl, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    if (strlen (to8_GetPrinterFolder()) != 0)
        (void)gtk_file_chooser_set_filename((GtkFileChooser *)dialog, to8_GetPrinterFolder());
    g_signal_connect(G_OBJECT(dialog), "current-folder-changed", G_CALLBACK(set_printer_folder), (gpointer) NULL);
    widget = gtk_file_chooser_button_new_with_dialog (dialog);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* label pour l'imprimante */
    widget=gtk_label_new(is_fr?"Imprimante :":"Printer :");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* combo pour les imprimantes */
    widget=gtk_combo_box_new_text();
    for (i=0; i<PRINTER_NUMBER; i++)
        gtk_combo_box_append_text (GTK_COMBO_BOX(widget), prt_list[i].name);
    gtk_combo_box_set_active (GTK_COMBO_BOX(widget), 0);
    g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(change_printer), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* bouton check pour le dip */
    widget=gtk_check_button_new_with_label("dip");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_dip), (gpointer) NULL);
    gtk_widget_set_tooltip_text (widget, is_fr?"Change le comportement de CR":"Change the behavior of CR");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour le nlq */
    widget=gtk_check_button_new_with_label("nlq");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_nlq), (gpointer) NULL);
    gtk_widget_set_tooltip_text (widget, is_fr?"Imprime en haute qualitÃ©":"High-quality print");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    
    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 8);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* boîte de centrage */
    widget=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* label pour le type de sortie */
    widget=gtk_label_new(is_fr?"Sortie :":"Output :");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour la sortie brute */
    widget=gtk_check_button_new_with_label(is_fr?"brute":"raw");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_raw_output), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour la sortie texte */
    widget=gtk_check_button_new_with_label(is_fr?"texte":"text");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_txt_output), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* bouton check pour la sortie graphique */
    widget=gtk_check_button_new_with_label(is_fr?"graphique":"graphic");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_gfx_output), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);

    /* boîte de centrage */
    widget=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);
}

