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
 *  Module     : linux/ugui/usetting.c
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 07/08/2011 24/03/2012 19/10/2012
 *                               15/09/2013 13/04/2014 14/07/2016
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
#include "linux/sound.h"
#include "teo.h"

static GtkWidget *sound_widget = NULL;
static int bank_range;

static GtkWidget *ram_size_radio_1;
static GtkWidget *ram_size_radio_2;


/* toggle_speed:
 *  Positionne la vitesse de l'émulateur
 */
static void toggle_speed (GtkWidget *button, gpointer data)
{
    int flag = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    teo.setting.exact_speed = flag;
    if (sound_widget != NULL)
    {
        flag = teo.setting.exact_speed ? TRUE : FALSE;
        gtk_widget_set_sensitive (sound_widget, flag);
    }
    (void)data;
}



/* toggle_extension:
 *  Toggle the activation of memory extension
 */
static void toggle_extension (GtkWidget *button, gpointer data)
{
    int flag = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    teo.setting.bank_range = (flag == TRUE) ? 32 : 16;
    teo.command = (teo.setting.bank_range == bank_range)
                                 ? TEO_COMMAND_NONE
                                 : TEO_COMMAND_COLD_RESET;
    (void)button;
}



/* toggle_interlaced:
 *  Positionne l'état du mode entrelacé
 */
static void toggle_interlace (GtkWidget *button, gpointer data)
{
    int flag = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    teo.setting.interlaced_video = flag;
    (void)data;
}


/* toggle_sound:
 *  Positionne l'activation du son
 */
static void toggle_sound (GtkWidget *button, gpointer data)
{
    int flag = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    teo.setting.sound_enabled = flag;
    if (flag == TRUE)
        usound_Init ();
    else
        usound_Close ();
    (void)data;
}



static GtkWidget *create_new_frame (const char *title, GtkWidget *grid,
                                    int left, int top )
{
    GtkWidget *vbox;
    GtkWidget *frame;

    /* frame */
    frame=gtk_frame_new (title);
    gtk_grid_attach (GTK_GRID (grid), frame, left, top, 1, 1);

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


/* usetting_Init:
 *  Mémorise le nombre de banque.
 */
void usetting_Update (void)
{
    bank_range = teo.setting.bank_range;

    if (teo.setting.bank_range == 16)
    {
        gtk_button_set_label (GTK_BUTTON (ram_size_radio_1), "256k");
        gtk_button_set_label (GTK_BUTTON (ram_size_radio_2), "512k (+reset)");
    }
    else
    {
        gtk_button_set_label (GTK_BUTTON (ram_size_radio_1), "256k (+reset)");
        gtk_button_set_label (GTK_BUTTON (ram_size_radio_2), "512k");
    }
}



/* usetting_Init:
 *  Initialise la frame du notebook pour les réglages
 */
void usetting_Init (GtkWidget *notebook)
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *grid;
    GtkWidget *widget;
    GtkWidget *frame;

    /* frame des commandes et réglages */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    gtk_container_set_border_width (GTK_CONTAINER(frame), 5);
    widget=gtk_label_new((is_fr?"RÃ©glages":"Settings"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);
    
    /* grille associée à la frame */
    grid = gtk_grid_new ();
    gtk_container_add( GTK_CONTAINER(frame), grid);
    gtk_grid_set_column_homogeneous (GTK_GRID(grid), TRUE);
    gtk_grid_set_column_spacing (GTK_GRID(grid), 5);
    gtk_grid_set_row_spacing (GTK_GRID(grid), 5);

    /* ---------------- Speed ------------------ */

    /* Création de la frame */
    vbox = create_new_frame (is_fr?"Vitesse":"Speed", grid, 0, 0);

    /* bouton de vitesse maximale */
    hbox = create_new_hbox (vbox);
    gtk_box_set_homogeneous (GTK_BOX(hbox), TRUE);
    widget=gtk_radio_button_new_with_label(NULL, (is_fr?"rapide":"fast"));
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* bouton de vitesse exacte */
    hbox = create_new_hbox (vbox);
    widget=gtk_radio_button_new_with_label_from_widget(
                            GTK_RADIO_BUTTON (widget),
                            is_fr?"exacte":"exact");
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* Buttons connexion */
    g_signal_connect(G_OBJECT(widget),
                     "toggled",
                     G_CALLBACK(toggle_speed),
                     (gpointer)NULL);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget),
                                  teo.setting.exact_speed ? TRUE : FALSE);

    /* ---------------- Memory extension ------------------ */

    /* Création de la frame */
    vbox = create_new_frame (is_fr?"MÃ©moire":"Memory", grid, 1, 0);

    hbox = create_new_hbox (vbox);
    ram_size_radio_1 =gtk_radio_button_new_with_label(NULL, (""));
    gtk_box_pack_end( GTK_BOX(hbox), ram_size_radio_1, TRUE, TRUE, 0);
    hbox = create_new_hbox (vbox);
    ram_size_radio_2 = gtk_radio_button_new_with_label_from_widget(
                                GTK_RADIO_BUTTON (ram_size_radio_1),
                                "");
    gtk_box_pack_end( GTK_BOX(hbox), ram_size_radio_2, TRUE, TRUE, 0);

    /* Buttons connexion */
    g_signal_connect(G_OBJECT(ram_size_radio_2),
                     "toggled",
                     G_CALLBACK(toggle_extension),
                     (gpointer)NULL);
    gtk_toggle_button_set_active(
                     GTK_TOGGLE_BUTTON(ram_size_radio_2),
                     (teo.setting.bank_range == 32) ? TRUE : FALSE);

    /* ---------------- Display ------------------ */

    /* Création de la frame */
    vbox = create_new_frame (is_fr?"Affichage":"Display", grid, 2, 0);

    /* checkbox du mode entrelacé */
    hbox = create_new_hbox (vbox);
    widget=gtk_check_button_new_with_label((is_fr?"VidÃ©o entrelacÃ©"
                                                 :"Interlaced video"));
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget),
                                  teo.setting.interlaced_video);
    g_signal_connect(G_OBJECT(widget),
                     "toggled",
                     G_CALLBACK(toggle_interlace),
                     (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* ---------------- Sound ------------------ */

    /* Création de la frame */
    vbox = create_new_frame (is_fr?"Son":"Sound", grid, 0, 1);

    /* checkbox du son */
    hbox = create_new_hbox (vbox);
    sound_widget=gtk_check_button_new_with_label((is_fr?"Actif":"Activated"));
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(sound_widget),
                                  teo.setting.sound_enabled);
    g_signal_connect(G_OBJECT(sound_widget),
                     "toggled",
                     G_CALLBACK(toggle_sound),
                     (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), sound_widget, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (sound_widget,
                              teo.setting.exact_speed ? TRUE : FALSE);

}

