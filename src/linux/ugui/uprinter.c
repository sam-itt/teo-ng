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











/* change_printer:
 *  Change le type d'imprimante
 */
static void change_printer (GtkWidget *button, gpointer printer_number)
{
//    teo.exact_speed=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    (void)button;
    (void)printer_number;
}



/* toggle_connection:
 *  Allume/éteint l'imprimante
 */
static void toggle_connection (GtkWidget *button, gpointer data)
{
    GdkColor color;
    static int onoff = 0;

    color.red = 0xffff;
    color.green = 0xf000;
    color.blue = 0;
    onoff ^= 1;
    gtk_widget_modify_bg (button, GTK_STATE_NORMAL, (onoff == 0) ? NULL : &color);
    gtk_widget_modify_bg (button, GTK_STATE_PRELIGHT, (onoff == 0) ? NULL : &color);
        
//    teo.exact_speed=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    (void)button;
    (void)data;
}



/* toggle_output:
 *  Positionne le type de sortie
 */
static void toggle_output (GtkWidget *button, gpointer output_bit)
{
    static int output = 0;

    output ^= (int)output_bit;
    
    (void)button;
    (void)output;
}



/* toggle_nlq:
 *  Positionne l'impression en NLQ
 */
static void toggle_nlq (GtkWidget *button, gpointer data)
{
//    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    (void)button;
    (void)data;
}



/* toggle_dip:
 *  Positionne le commutateur DIP
 */
static void toggle_dip (GtkWidget *button, gpointer data)
{
//    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    (void)button;
    (void)data;
}



/* --------------------------- Partie publique ----------------------------- */



/* init_disk_notebook_frame:
 *  Initialise la frame du notebook pour la cartouche.
 */
void init_printer_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *frame;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Imprimante":"Printer"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* boîte verticale associée à la frame */
    vbox=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton radio pour la PR90-055 */
    widget=gtk_radio_button_new_with_label(NULL, "PR90-055");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(change_printer), (gpointer) 055);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    
    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton radio pour la PR90-600 */
    widget=gtk_radio_button_new_with_label_from_widget((GtkRadioButton *)widget, "PR90-600");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(change_printer), (gpointer) 600);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton radio pour la PR90-612 */
    widget=gtk_radio_button_new_with_label_from_widget((GtkRadioButton *)widget, "PR90-612");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(change_printer), (gpointer) 612);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* label pour le type de sortie */
    widget=gtk_label_new(is_fr?"Sortie:":"Output:");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton check pour la sortie brute */
    widget=gtk_check_button_new_with_label(is_fr?"brute":"raw");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_output), (gpointer) PRINTER_OUTPUT_RAW);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton check pour la sortie texte */
    widget=gtk_check_button_new_with_label(is_fr?"texte":"text");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_output), (gpointer) PRINTER_OUTPUT_TEXT);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton check pour la sortie graphique */
    widget=gtk_check_button_new_with_label(is_fr?"graphique":"graphic");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_output), (gpointer) PRINTER_OUTPUT_GFX);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton check pour le dip */
    widget=gtk_check_button_new_with_label("dip");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_dip), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton check pour le nlq */
    widget=gtk_check_button_new_with_label("nlq");
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_nlq), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton "On-Line" */
    widget = gtk_button_new_with_label ("On Line");
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(toggle_connection), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* bouton "Eject" */
    widget = gtk_button_new_with_label ("Eject");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);
}

