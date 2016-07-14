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
 *  Module     : linux/udebug/udstatus.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 14/07/2016
 *  Modifié par:
 *
 *  Débogueur 6809 - Gestion de la barre d'outils.
 */

#ifndef SCAN_DEPEND
    #include <stdio.h>
    #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "linux/gui.h"

GtkToolItem *run_button;


static void do_step_by_step (GtkToolButton *toolbutton, gpointer user_data)
{
    udebug_DoStepByStep ();
    (void)toolbutton;
    (void)user_data;
}



static void do_step_over (GtkToolButton *toolbutton, gpointer user_data)
{
    udebug_DoStepOver ();
    (void)toolbutton;
    (void)user_data;
}
    

static void do_quit (GtkToolButton *toolbutton, gpointer user_data)
{
    int quit_mode = (int)user_data;

    udebug_Quit (quit_mode);

    (void)toolbutton;
    (void)user_data;
}


/* ------------------------------------------------------------------------- */


/* udtoolb_SetRunButtonSensitivity:
 *  Set the run button sensitivity.
 */
void udtoolb_SetRunButtonSensitivity (int state)
{
    gtk_widget_set_sensitive (GTK_WIDGET (run_button), state);
}



/* udtoolb_Init:
 *  Initialize the tool bar.
 */
GtkWidget *udtoolb_Init (void)
{
    GtkWidget *tool_bar;
    GtkToolItem *tool_button;
    GtkWidget *box;
    GtkWidget *image;
    GdkPixbuf *pixbuf;

    /* Box */
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    /* Tool bar */
    tool_bar = gtk_toolbar_new ();
    gtk_toolbar_set_style (GTK_TOOLBAR (tool_bar), GTK_TOOLBAR_BOTH);
    gtk_box_pack_start (GTK_BOX(box), tool_bar, FALSE, FALSE, 0);

    /* Tool item step by step */
#ifdef DEBIAN_BUILD
    pixbuf = gdk_pixbuf_new_from_file ("/usr/share/teo/system/icons/step.ico", NULL);
#else
    pixbuf = gdk_pixbuf_new_from_file ("system/icons/step.ico", NULL);
#endif
    image = gtk_image_new_from_pixbuf (pixbuf);
    tool_button = gtk_tool_button_new (
        image,
        is_fr?"Pas-Ã -pas":"Step by step");
    gtk_widget_set_tooltip_text (GTK_WIDGET (tool_button),
        is_fr?"ExÃ©cute le code machine pas Ã  pas"
             :"Execute the machine code step by step");
    g_signal_connect(G_OBJECT(tool_button),
                     "clicked",
                     G_CALLBACK (do_step_by_step),
                     (gpointer) NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (tool_button), -1);

    /* Tool item step over */
#ifdef DEBIAN_BUILD
    pixbuf = gdk_pixbuf_new_from_file ("/usr/share/teo/system/icons/stepover.ico", NULL);
#else
    pixbuf = gdk_pixbuf_new_from_file ("system/icons/stepover.ico", NULL);
#endif
    image = gtk_image_new_from_pixbuf (pixbuf);
    tool_button = gtk_tool_button_new (
        image,
        is_fr?"Passe jumps":"Step over");
    gtk_widget_set_tooltip_text (GTK_WIDGET (tool_button),
        is_fr?"ExÃ©cute le code machine pas Ã  pas\n" \
              "mais ne saute pas aux sous-programmes\n" \
              "et aux boucles vers l'arriÃ¨re"
             :"Execute the machine code step by step\n" \
              "but don't jump to sub-programs and\n" \
              "backward loops");
    g_signal_connect(G_OBJECT(tool_button),
                     "clicked",
                     G_CALLBACK (do_step_over),
                     (gpointer) TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (tool_button), -1);

    /* Tool item run */
#ifdef DEBIAN_BUILD
    pixbuf = gdk_pixbuf_new_from_file ("/usr/share/teo/system/icons/run.ico", NULL);
#else
    pixbuf = gdk_pixbuf_new_from_file ("system/icons/run.ico", NULL);
#endif
    image = gtk_image_new_from_pixbuf (pixbuf);
    run_button = gtk_tool_button_new (
        image,
        is_fr?"Lancer":"Run");
    g_signal_connect(G_OBJECT(run_button),
                     "clicked",
                     G_CALLBACK (do_quit),
                     (gpointer) TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (run_button), -1);

    /* Tool item leave */
#ifdef DEBIAN_BUILD
    pixbuf = gdk_pixbuf_new_from_file ("/usr/share/teo/system/icons/leave.ico", NULL);
#else
    pixbuf = gdk_pixbuf_new_from_file ("system/icons/leave.ico", NULL);
#endif
    image = gtk_image_new_from_pixbuf (pixbuf);
    tool_button = gtk_tool_button_new (
        image,
        is_fr?"Abandonner":"Leave");
    g_signal_connect(G_OBJECT(tool_button),
                     "clicked",
                     G_CALLBACK (do_quit),
                     (gpointer) FALSE);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (tool_button), -1);

    return box;
}



/* udtoolb_Exit:
 *  Exit the tool bar.
 */
void udtoolb_Exit (void)
{
}

