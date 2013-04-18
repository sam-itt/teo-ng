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
 *  Module     : linux/ugui.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 07/2011
 *               François Mouret 08/2011 26/03/2012 12/06/2012
 *
 *  Interface utilisateur de l'émulateur basée sur GTK+ 3.x .
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <gdk/gdkx.h>
   #include <gtk/gtk.h>
#endif

#include "linux/display.h"
#include "linux/gui.h"
#include "linux/graphic.h"
#include "teo.h"
#include "commands.xpm"

enum {
   TEO_RESPONSE_END = 1,
   TEO_RESPONSE_QUIT
};

/* fenêtre de l'interface utilisateur */
GtkWidget *wControl = NULL;
static GtkWidget *notebook;


/* do_exit:
 *  Sort avec une commande.
 */
static void do_exit (GtkWidget *button, gpointer user_data)
{
    teo.command = (volatile enum teo_command)user_data;
    gtk_dialog_response (GTK_DIALOG(wControl), TEO_RESPONSE_END);

    (void) button;
}


/* ------------------------------------------------------------------------- */


/* ugui_MessageBox:
 *  Affiche une boîte à message
 */
void ugui_MessageBox (const gchar *message, GtkWidget *parent_window, int dialog_flag)
{
    GtkWidget *dialog = gtk_message_dialog_new  (
                    (GtkWindow *)parent_window,
                    GTK_DIALOG_MODAL 
                     | ((parent_window!=NULL)?GTK_DIALOG_DESTROY_WITH_PARENT:0),
                    dialog_flag, GTK_BUTTONS_OK, "%s", message);
    gtk_window_set_title (GTK_WINDOW(dialog), "Teo");
    (void)gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy ((GtkWidget *)dialog);
}



/* ugui_Error:
 *  Affiche une boîte d'erreur
 */
void ugui_Error (const gchar *message, GtkWidget *parent_window)
{
    ugui_MessageBox (message, parent_window, GTK_MESSAGE_ERROR);
}



/* ugui_Warning:
 *  Affiche une boîte de prévention
 */
void ugui_Warning (const gchar *message, GtkWidget *parent_window)
{
    ugui_MessageBox (message, parent_window, GTK_MESSAGE_WARNING);
}



/* ugui_Free:
 *  Libère la mémoire utilisée par l'interface
 */
void ugui_Free (void)
{
    umemo_Free ();
    ucass_Free ();
    udisk_Free ();
}



/* ugui_Init:
 *  Initialise le module interface utilisateur.
 */
void ugui_Init(void)
{
    GtkWidget *content_area;
    GtkWidget *action_area;
    GtkWidget *widget;
    GtkWidget *hbox, *vbox;
    GdkPixbuf *pixbuf;

    /* fenêtre d'affichage */
    wControl = gtk_dialog_new ();
    gtk_window_set_resizable (GTK_WINDOW(wControl), FALSE);
    gtk_window_set_title (GTK_WINDOW(wControl), is_fr?"Panneau de contrÃ´le":"Control panel");
    gtk_window_set_transient_for (GTK_WINDOW(wControl), GTK_WINDOW(wMain));
    gtk_window_set_destroy_with_parent (GTK_WINDOW(wControl), TRUE);
    gtk_window_set_modal (GTK_WINDOW(wControl), TRUE);
    
    content_area = gtk_dialog_get_content_area (GTK_DIALOG(wControl));
    action_area = gtk_dialog_get_action_area (GTK_DIALOG(wControl));
    
    /* bouton de "A Propos" */
    widget=gtk_button_new_from_stock(GTK_STOCK_ABOUT);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(uabout_Dialog), (gpointer)NULL);
    gtk_box_pack_end( GTK_BOX(action_area), widget, FALSE, FALSE, 0);

    /* boîte horizontale du titre */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start( GTK_BOX(action_area), hbox, TRUE, TRUE, 0);

    gtk_dialog_add_button (GTK_DIALOG(wControl), GTK_STOCK_QUIT , TEO_RESPONSE_QUIT  );
    gtk_dialog_add_button (GTK_DIALOG(wControl), GTK_STOCK_OK   , GTK_RESPONSE_ACCEPT);

    /* crée toutes les widgets de la fenêtre */
    /* boîte horizontale du titre */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start( GTK_BOX(content_area), hbox, TRUE, FALSE, 0);

    /* création du pixbuf */
    pixbuf=gdk_pixbuf_new_from_xpm_data ((const char **)commands_xpm);
    widget=gtk_image_new_from_pixbuf (pixbuf);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte verticale associée à la frame des commandes et réglages */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(content_area), vbox);

    /* bouton de réinitialisation */
    widget=gtk_button_new_with_label((is_fr?"RÃ©initialiser le TO8":"TO8 warm reset"));
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) TEO_COMMAND_RESET);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, FALSE, 0);

    /* bouton de redémarrage à froid */
    widget=gtk_button_new_with_label((is_fr?"RedÃ©marrer Ã  froid le TO8":"TO8 cold reset"));
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) TEO_COMMAND_COLD_RESET);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, FALSE, 0);

    /* notebook */
    notebook=gtk_notebook_new();
    gtk_box_pack_start( GTK_BOX(content_area), notebook, TRUE, FALSE, 0);
    usetting_Init (notebook);
    udisk_Init    (notebook);
    ucass_Init    (notebook);
    umemo_Init    (notebook);
    uprinter_Init (notebook);

    /* affiche tout l'intérieur */
    gtk_widget_show_all(content_area);

    /* Attend la fin du travail de GTK */
    while (gtk_events_pending ())
        gtk_main_iteration ();
}


/* ugui_Panel:
 *  Affiche le panneau de contrôle.
 */
void ugui_Panel(void)
{
    gint response;

    /* Mise à jour du compteur de cassettes */
    ucass_UpdateCounter ();

    /* initialisation de la commande */
    teo.command = TEO_COMMAND_NONE;

    /* gestion des évènements */
    response = gtk_dialog_run (GTK_DIALOG(wControl));
    switch (response)
    {
        case TEO_RESPONSE_END    : break;
        case GTK_RESPONSE_ACCEPT : break;
        case TEO_RESPONSE_QUIT   : teo.command=TEO_COMMAND_QUIT; break;
   }
   gtk_widget_hide (wControl);
}

