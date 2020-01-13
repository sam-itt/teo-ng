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
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 07/2011
 *               François Mouret 08/2011 26/03/2012 12/06/2012
 *                               19/09/2013 11/04/2014 31/05/2015
 *                               31/07/2016
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

enum {
   TEO_RESPONSE_END = 1,
   TEO_RESPONSE_QUIT
};

GtkWidget *wMain;
GdkWindow *gwindow_win;


/* fenêtre de l'interface utilisateur */
GtkWidget *wControl = NULL;
static GtkWidget *notebook;


/* do_exit:
 *  Sort avec une commande.
 */
static void do_exit (GtkWidget *button, gpointer user_data)
{
    if (GPOINTER_TO_INT(user_data) == TEO_COMMAND_FULL_RESET)
    {
        if (ugui_Question (is_fr?"Toute la mÃ©moire RAM sera effacÃ©e."
                                :"All the RAM memory will be cleared.",
                           wControl) == FALSE)
            return;
    }
    teo.command = (volatile enum teo_command)user_data;
    gtk_dialog_response (GTK_DIALOG(wControl), TEO_RESPONSE_END);
    (void) button;
}


/* ------------------------------------------------------------------------- */


/* ugui_MessageBox:
 *  Affiche une boîte à message
 */
int ugui_MessageBox (const gchar *message, GtkWidget *parent_window,
                      int message_type, int buttons_type)
{
    gint response;

    GtkWidget *dialog = gtk_message_dialog_new  (
                    (GtkWindow *)parent_window,
                    GTK_DIALOG_MODAL 
                     | ((parent_window!=NULL)?GTK_DIALOG_DESTROY_WITH_PARENT:0),
                    message_type, buttons_type, "%s", message);
    gtk_window_set_title (GTK_WINDOW(dialog), "Teo");
    response = gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy ((GtkWidget *)dialog);
    return (response == GTK_RESPONSE_OK) ? TRUE : FALSE;
}



/* ugui_Error:
 *  Affiche une boîte d'erreur
 */
void ugui_Error (const gchar *message, GtkWidget *parent_window)
{
    (void)ugui_MessageBox (message, parent_window ? parent_window : wMain,
                           GTK_MESSAGE_ERROR, GTK_BUTTONS_OK);
}



/* ugui_Warning:
 *  Affiche une boîte de prévention
 */
void ugui_Warning (const gchar *message, GtkWidget *parent_window)
{
    (void)ugui_MessageBox (message, parent_window,
                           GTK_MESSAGE_WARNING, GTK_BUTTONS_OK);
}



/* ugui_Question:
 *  Affiche une boîte de questionnement
 */
int ugui_Question (const gchar *message, GtkWidget *parent_window)
{
    return ugui_MessageBox (message, parent_window,
                            GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL);
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
    GtkWidget *widget;
    GtkWidget *hbox, *vbox;

    /* fenêtre d'affichage */
    wControl = gtk_dialog_new_with_buttons (
                is_fr?"Panneau de contrÃ´le":"Control panel",
                GTK_WINDOW (wMain),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                is_fr?"Quitter":"Quit", TEO_RESPONSE_QUIT,
                is_fr?"Valider":"OK", GTK_RESPONSE_ACCEPT,
                NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG(wControl));
    
    /* boîte verticale associée à la frame des commandes et réglages */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(content_area), vbox);

    /* bouton de "A Propos" */
    widget=gtk_button_new_with_label(is_fr?"Ã€ propos":"About");
    gtk_box_pack_start( GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK(uabout_Dialog),
                     (gpointer)NULL);

    /* boîte horizontale des resets */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* bouton de réinitialisation */
    widget=gtk_button_new_with_label(is_fr?"Reset Ã  chaud"
                                          :"Warm reset");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);
    gtk_widget_set_tooltip_text (widget,
                                 is_fr?"RedÃ©marre Ã  chaud sans\n" \
                                       "effacer la mÃ©moire RAM"
                                      :"Warm reset without to\n"
                                       "clear the RAM memory");
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK(do_exit),
                     (gpointer) TEO_COMMAND_RESET);

    /* bouton de redémarrage à froid */
    widget=gtk_button_new_with_label(is_fr?"Reset Ã  froid"
                                          :"Cold reset");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);
    gtk_widget_set_tooltip_text (widget,
                                 is_fr?"RedÃ©marre Ã  froid sans\n" \
                                       "effacer la mÃ©moire RAM"
                                      :"Cold reset without to\n" \
                                       "clear the RAM memory");
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK(do_exit),
                     (gpointer) TEO_COMMAND_COLD_RESET);

    /* bouton de redémarrage à froid avec effacement de la mémoire */
    widget=gtk_button_new_with_label(is_fr?"Reset total"
                                          :"Full reset");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);
    gtk_widget_set_tooltip_text (widget,
                                 is_fr?"RedÃ©marre Ã  froid et\n" \
                                       "efface la mÃ©moire RAM"
                                      :"Cold reset and\n" \
                                       "clear the RAM memory");
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK(do_exit),
                     (gpointer) TEO_COMMAND_FULL_RESET);

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

    /* Mises à jour de la GUI */
    ucass_UpdateCounter ();
    usetting_Update ();

    /* initialisation de la commande */
    teo.command = TEO_COMMAND_NONE;

    /* gestion des évènements */
    response = gtk_dialog_run (GTK_DIALOG(wControl));
    switch (response)
    {
        case TEO_RESPONSE_END    : break;
        case GTK_RESPONSE_ACCEPT : break;
        case TEO_RESPONSE_QUIT   :
            if (teo.command == TEO_COMMAND_COLD_RESET)
                teo_ColdReset();
            teo.command=TEO_COMMAND_QUIT;
            break;
   }
   gtk_widget_hide (wControl);
}



