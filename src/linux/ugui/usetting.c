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
 *  Module     : linux/ugui/usetting.c
 *  Version    : 1.8.1
 *  Cr�� par   : Eric Botcazou juillet 1999
 *  Modifi� par: Eric Botcazou 19/11/2006
 *               Gilles F�tis 27/07/2011
 *               Fran�ois Mouret 07/08/2011 24/03/2012
 *
 *  Gestion des pr�f�rences.
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

extern int SetInterlaced(int);



/* toggle_speed:
 *  Positionne la vitesse de l'�mulateur
 */
static void toggle_speed (GtkWidget *button, gpointer data)
{
    teo.exact_speed=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    (void)data;
}



/* toggle_interlaced:
 *  Positionne l'�tat du mode entrelac�
 */
static void toggle_interlaced (GtkWidget *button, gpointer data)
{
    SetInterlaced (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) ? 1 : 0);
    (void)data;
}


/* --------------------------- Partie publique ----------------------------- */


/* init_setting_notebook:
 *  Initialise la frame du notebook pour les r�glages
 */
void init_setting_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *widget;
    GSList *group_speed;
    GtkWidget *frame;

    /* frame des commandes et r�glages */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Réglages":"Settings"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);
    
    /* bo�te verticale associ�e � la frame des commandes et r�glages */
    vbox=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    /* bo�te horizontale de la vitesse */
    hbox=gtk_hbox_new(FALSE,2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* label de la vitesse */
    widget=gtk_label_new((is_fr?"Vitesse de l'émulation:":"Emulation speed:"));
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton de vitesse maximale */
    widget=gtk_radio_button_new_with_label(NULL, (is_fr?"rapide":"fast"));
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bouton de vitesse exacte */
    group_speed=gtk_radio_button_get_group (GTK_RADIO_BUTTON(widget));
    widget=gtk_radio_button_new_with_label(group_speed, (is_fr?"exacte":"exact"));
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_speed), (gpointer)NULL);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), teo.exact_speed ? TRUE : FALSE);
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* bo�te horizontale du mode entrelac� */
    hbox=gtk_hbox_new(FALSE,2);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* checkbox du mode entrelac� */
    widget=gtk_check_button_new_with_label((is_fr?"Affichage vidéo entrelacé":"Interlaced video display"));
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), FALSE);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_interlaced), (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
}
