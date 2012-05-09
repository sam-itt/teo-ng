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

/* fenêtre de l'interface utilisateur */
GtkWidget *wdControl;



/* retrace_callback:
 *  Callback de retraçage de la fenêtre principale.
 */
static GdkFilterReturn retrace_callback(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    XEvent *ev = (XEvent *) xevent;

    if (ev->type == Expose)
        RetraceScreen(ev->xexpose.x, ev->xexpose.y, ev->xexpose.width, ev->xexpose.height);

    return GDK_FILTER_REMOVE;
    (void) event;
    (void) data;
}



/* do_exit:
 *  Quitte le panneau de contrôle avec une commande.
 */
static void do_exit(GtkWidget *button, int command)
{
    if (command != NONE)
        teo.command=command;

    gtk_widget_hide(wdControl);
    gtk_main_quit();

    (void) button;
}



/* do_hide:
 *  Quitte le panneau de contrôle par le gadget de fermeture.
 */
static int do_hide(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    gtk_widget_hide(widget);
    gtk_main_quit();
    return TRUE;

    (void) event;
    (void) user_data;
}



/* ask_to_quit:
 *  Demande à quitter Teo.
 */
static void ask_to_quit(GtkWidget *button, gpointer unused)
{
/*
    if (ask_box (is_fr?"Voulez-vous vraiment quitter l'Ã©mulateur ?"
                      :"Do you really want to quit the emulator ?", wdControl) == TRUE)
*/
        do_exit (NULL, QUIT);
    (void) button;
    (void) unused;
}



/* iconify_window:
 *  Iconification de la fenêtre principale.
 */
static void iconify_window (GtkWidget *widget, GdkEvent *event, void *user_data)
{
    if (event->window_state.new_window_state == GDK_WINDOW_STATE_ICONIFIED) {
        gtk_window_iconify (GTK_WINDOW(widget_win));
    }
    (void)widget;
    (void)user_data;
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
    gtk_window_set_title ((GtkWindow *)dialog, "Teo");
    (void)gtk_dialog_run ((GtkDialog *)dialog);
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
    response = gtk_dialog_run ((GtkDialog*)dialog);
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
void InitGUI(int direct_disk_support)
{
    GtkWidget *main_vbox;
    GtkWidget *notebook;
    GtkWidget *widget;
    GtkWidget *hbox, *vbox;
    GdkPixbuf *pixbuf;

    printf("Initialisation de l'interface..."); fflush (stdout);

    /* fenêtre d'affichage */
    wdControl=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_transient_for (GTK_WINDOW(wdControl), GTK_WINDOW(widget_win));
    gtk_window_set_position(GTK_WINDOW(wdControl), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_destroy_with_parent (GTK_WINDOW(wdControl), TRUE);
    gtk_window_set_modal (GTK_WINDOW(wdControl), TRUE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(wdControl), TRUE);
    gtk_window_set_resizable (GTK_WINDOW(wdControl), FALSE);
    gtk_window_set_title(GTK_WINDOW(wdControl), (is_fr?"Teo - Panneau de contrÃ´le":"Teo - Control panel"));
    g_signal_connect(G_OBJECT(wdControl), "window-state-event", G_CALLBACK(iconify_window), (gpointer)NULL);
    g_signal_connect(G_OBJECT(wdControl), "delete-event", G_CALLBACK(do_hide), (gpointer)NULL);
    gtk_widget_realize(wdControl); /* nécessaire pour charger le pixmap */

    /* boîte verticale associée à la fenêtre */
    main_vbox=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(main_vbox), 5);
    gtk_container_add( GTK_CONTAINER(wdControl), main_vbox);

    /* crée toutes les widgets de la fenêtre */
    /* boîte horizontale du titre */
    hbox=gtk_hbox_new(TRUE,0);
    gtk_box_pack_start( GTK_BOX(main_vbox), hbox, TRUE, FALSE, 0);

    /* création du pixbuf */
    pixbuf=gdk_pixbuf_new_from_xpm_data ((const char **)commands_xpm);
    widget=gtk_image_new_from_pixbuf (pixbuf);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);

    /* boîte verticale associée à la frame des commandes et réglages */
    vbox=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(main_vbox), vbox);

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
    gtk_box_pack_start( GTK_BOX(main_vbox), notebook, TRUE, FALSE, 0);
    init_setting_notebook_frame (notebook);
    init_disk_notebook_frame (notebook, direct_disk_support);
    init_cass_notebook_frame (notebook);
    init_memo_notebook_frame (notebook);
    init_printer_notebook_frame (notebook);
    gtk_notebook_set_page( GTK_NOTEBOOK(notebook), 0);
    
    /* boîte horizontale des boutons de sortie */
    hbox=gtk_hbutton_box_new();
    gtk_box_set_spacing ( GTK_BOX(hbox), 7);
    gtk_box_pack_start( GTK_BOX(main_vbox), hbox, TRUE, FALSE, 10);
    gtk_button_box_set_layout ((GtkButtonBox *)hbox, GTK_BUTTONBOX_END);

    /* bouton à propos */
    widget=gtk_button_new_from_stock (GTK_STOCK_ABOUT);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(run_about_window), (gpointer) NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    gtk_button_box_set_child_secondary  ((GtkButtonBox *)hbox, widget, TRUE);
    
    /* bouton quitter */
    widget=gtk_button_new_from_stock (GTK_STOCK_QUIT);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(ask_to_quit), (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    gtk_button_box_set_child_secondary  ((GtkButtonBox *)hbox, widget, FALSE);

    /* bouton retour */
    widget=gtk_button_new_from_stock (GTK_STOCK_OK);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) NONE);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    gtk_button_box_set_child_secondary  ((GtkButtonBox *)hbox, widget, FALSE);

    /* affiche tout */
    gtk_widget_show_all(main_vbox);

    /* mise en place d'un hook pour assurer le retraçage de la fenêtre
       principale de l'émulateur */
    gdk_window_add_filter(GTK_WIDGET(widget_win)->window, retrace_callback, NULL);

    /* Attend la fin du travail de GTK */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    printf("ok\n");
}



/* ControlPanel:
 *  Affiche le panneau de contrôle.
 */
void ControlPanel(void)
{
    /* Mise à jour du compteur de cassettes */
    update_counter_cass ();

    /* affichage de la fenêtre principale et de ses éléments */
    gtk_widget_show(wdControl);

    /* initialisation de la commande */
    teo.command = NONE;
    
    /* boucle de gestion des évènements */
    gtk_main();
}

