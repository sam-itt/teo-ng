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
 *  Module     : linux/ugui.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 07/2011
 *               François Mouret 08/2011 26/03/2012
 *
 *  Interface utilisateur de l'émulateur basée sur GTK+ 2.x .
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
#include "to8.h"
#include "commands.xpm"
#include "intern/gui.h"

enum {
   TEO_RESPONSE_QUIT = 1,
   TEO_RESPONSE_END
};

/* fenêtre de l'interface utilisateur */
GtkWidget *wControl;


/* do_exit:
 *  Quitte le panneau de contrôle avec une commande.
 */
static void do_exit(GtkWidget *button, int command)
{
    if (command != NONE)
        teo.command=command;
    gtk_dialog_response (GTK_DIALOG(wControl), TEO_RESPONSE_END);

    (void) button;
}


/* display_box:
 *  Affiche une boîte à message
 */
void display_box (const gchar *message, GtkWidget *parent_window, int dialog_flag)
{
    GtkWidget *dialog = gtk_message_dialog_new  (
                          (GtkWindow *)parent_window,
                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                          dialog_flag, GTK_BUTTONS_OK,
                          "%s", message);
    gtk_window_set_title (GTK_WINDOW(dialog), "Teo");
    (void)gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy ((GtkWidget *)dialog);
}


/* --------------------------- Partie publique ----------------------------- */

#if 0
/* ask_box:
 *  Affiche une boîte de confirmation
 */
int ask_box (const gchar *message, GtkWidget *parent_window)
{
    GtkResponseType response;
    GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow*)parent_window,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                 "%s", message);
    gtk_window_set_title ((GtkWindow *)dialog, "Teo - confirmation");
    response = gtk_dialog_run (GTK_DIALOG(wControl);
    gtk_widget_destroy (dialog);
    return (response == GTK_RESPONSE_YES) ? TRUE : FALSE;
}
#endif


/* error_box:
 *  Affiche une boîte d'erreur
 */
void error_box (const gchar *message, GtkWidget *parent_window)
{
    display_box (message, parent_window, GTK_MESSAGE_ERROR);
}



/* warning_box:
 *  Affiche une boîte de prévention
 */
void warning_box (const gchar *message, GtkWidget *parent_window)
{
    display_box (message, parent_window, GTK_MESSAGE_WARNING);
}



/* FreeGUI:
 *  Libère la mémoire utilisée par l'interface
 */
void FreeGUI (void)
{
    free_memo_list ();
    free_cass_list ();
    free_disk_list ();
}



/* InitGUI:
 *  Initialise le module interface utilisateur.
 */
void InitGUI(void)
{
    GtkWidget *notebook;
    GtkWidget *content_area;
    GtkWidget *widget;
    GtkWidget *hidden_button;
    GtkWidget *hbox, *vbox;
    GdkPixbuf *pixbuf;

    /* fenêtre d'affichage */
    wControl = gtk_dialog_new_with_buttons (
                    is_fr?"Teo - Panneau de contrÃ´le":"Teo - Control panel",
                    GTK_WINDOW(wMain),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                    GTK_STOCK_QUIT, TEO_RESPONSE_QUIT,
                    NULL);
    hidden_button = gtk_dialog_add_button (GTK_DIALOG(wControl),"", TEO_RESPONSE_END);
    gtk_window_set_resizable (GTK_WINDOW(wControl), FALSE);
    content_area = gtk_dialog_get_content_area (GTK_DIALOG(wControl));

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
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) RESET);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, FALSE, 0);

    /* bouton de redémarrage à froid */
    widget=gtk_button_new_with_label((is_fr?"RedÃ©marrer Ã  froid le TO8":"TO8 cold reset"));
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) COLD_RESET);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, FALSE, 0);

    /* notebook */
    notebook=gtk_notebook_new();
    gtk_box_pack_start( GTK_BOX(content_area), notebook, TRUE, FALSE, 0);
    init_setting_notebook_frame (notebook);
    init_disk_notebook_frame (notebook);
    init_cass_notebook_frame (notebook);
    init_memo_notebook_frame (notebook);
    init_printer_notebook_frame (notebook);
    gtk_notebook_set_current_page( GTK_NOTEBOOK(notebook), 0);
    
    /* affiche tout l'intérieur */
    gtk_widget_show_all(content_area);
    gtk_widget_hide (hidden_button);

    /* Attend la fin du travail de GTK */
    while (gtk_events_pending ())
        gtk_main_iteration ();
}



/* ControlPanel:
 *  Affiche le panneau de contrôle.
 */
void ControlPanel(void)
{
    gint response;

    InitGUI ();

    /* Mise à jour du compteur de cassettes */
    update_counter_cass ();

    /* initialisation de la commande */
    teo.command = NONE;

    /* gestion des évènements */
    response = gtk_dialog_run (GTK_DIALOG(wControl));
    switch (response)
    {
        case TEO_RESPONSE_END   : break;
        case GTK_RESPONSE_ACCEPT: teo.command=NONE; break;
        case TEO_RESPONSE_QUIT  : teo.command=QUIT; break;
   }
   gtk_widget_destroy (wControl);
}

