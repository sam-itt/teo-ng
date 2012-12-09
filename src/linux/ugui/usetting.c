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
 *  Module     : linux/ugui/usetting.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 07/08/2011 24/03/2012 19/10/2012
 *
 *  Gestion des préférences.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <string.h>
   #include <libgen.h>
   #include <gtk/gtk.h>
#endif

#include "linux/gui.h"
#include "teo.h"

static GtkWidget *sound_widget = NULL;


/* toggle_speed:
 *  Positionne la vitesse de l'émulateur
 */
static void toggle_speed (GtkWidget *button, gpointer data)
{
    teo.setting.exact_speed=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    if (sound_widget != NULL)
        gtk_widget_set_sensitive (sound_widget, teo.setting.exact_speed ? TRUE : FALSE);
    (void)data;
}



/* toggle_interlaced:
 *  Positionne l'état du mode entrelacé
 */
static void toggle_interlace (GtkWidget *button, gpointer data)
{
    teo.setting.interlaced_video = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    (void)data;
}


/* toggle_sound:
 *  Positionne l'activation du son
 */
static void toggle_sound (GtkWidget *button, gpointer data)
{
    teo.setting.sound_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    (void)data;
}


/* ------------------------------------------------------------------------- */


/* usetting_Init:
 *  Initialise la frame du notebook pour les réglages
 */
void usetting_Init (GtkWidget *notebook)
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *widget;
    GtkWidget *frame;

    /* frame des commandes et réglages */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"RÃ©glages":"Settings"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);
    
    /* boîte verticale associée à la frame des commandes et réglages */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* boîte horizontale de la vitesse */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* label de la vitesse */
    widget=gtk_label_new((is_fr?"Vitesse de l'Ã©mulation:":"Emulation speed:"));
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton de vitesse maximale */
    widget=gtk_radio_button_new_with_label(NULL, (is_fr?"rapide":"fast"));
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton de vitesse exacte */
    widget=gtk_radio_button_new_with_label_from_widget((GtkRadioButton *) widget, (is_fr?"exacte":"exact"));
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_speed), (gpointer)NULL);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), teo.setting.exact_speed ? TRUE : FALSE);
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte horizontale du checkbox de son */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* checkbox du son */
    sound_widget=gtk_check_button_new_with_label((is_fr?"Son":"Sound"));
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(sound_widget), teo.setting.sound_enabled);
    g_signal_connect(G_OBJECT(sound_widget), "toggled", G_CALLBACK(toggle_sound), (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), sound_widget, TRUE, FALSE, 0);
    gtk_widget_set_sensitive (sound_widget, teo.setting.exact_speed ? TRUE : FALSE);

    /* boîte horizontale du mode entrelacé */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* checkbox du mode entrelacé */
    widget=gtk_check_button_new_with_label((is_fr?"Affichage vidÃ©o entrelacÃ©":"Interlaced video display"));
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), teo.setting.interlaced_video);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_interlace), (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
}

