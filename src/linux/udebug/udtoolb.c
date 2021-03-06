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
 *  Copyright (C) 1997-2018 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/udebug/udstatus.c
 *  Version    : 1.8.5
 *  Cr�� par   : Fran�ois Mouret 14/07/2016
 *  Modifi� par: Samuel Cuella 02/2020
 *
 *  D�bogueur 6809 - Gestion de la barre d'outils.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
    #include <stdio.h>
    #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "linux/gui.h"
#include "std.h"
#include "gettext.h"

#define STEP_ICON "system/icons/step.ico"
#define STEP_OVER_ICON "system/icons/stepover.ico"
#define RUN_ICON "system/icons/run.ico"
#define LEAVE_ICON "system/icons/leave.ico"






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
    int quit_mode = GPOINTER_TO_INT (user_data);

    udebug_Quit (quit_mode);

    (void)toolbutton;
    (void)user_data;
}


/* ------------------------------------------------------------------------- */


/* udtoolb_Free:
 *  Free the memory used by the tool bar.
 */
void udtoolb_Free (void)
{
}



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
    char *sysicon;

    /* Box */
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    /* Tool bar */
    tool_bar = gtk_toolbar_new ();
    gtk_toolbar_set_style (GTK_TOOLBAR (tool_bar), GTK_TOOLBAR_BOTH);
    gtk_box_pack_start (GTK_BOX(box), tool_bar, FALSE, FALSE, 0);

    /* Tool item step by step */
    sysicon = std_GetTeoSystemFile(STEP_ICON, false);
    pixbuf = gdk_pixbuf_new_from_file (sysicon, NULL);

    image = gtk_image_new_from_pixbuf (pixbuf);
    tool_button = gtk_tool_button_new (
        image,
        _("Step by step"));
    gtk_widget_set_tooltip_text (GTK_WIDGET (tool_button),
        _("Execute the machine code step by step"));
    g_signal_connect(G_OBJECT(tool_button),
                     "clicked",
                     G_CALLBACK (do_step_by_step),
                     (gpointer) NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (tool_button), -1);

    /* Tool item step over */
    sysicon = std_GetTeoSystemFile(STEP_OVER_ICON, false);
    pixbuf = gdk_pixbuf_new_from_file (sysicon, NULL);

    image = gtk_image_new_from_pixbuf (pixbuf);
    tool_button = gtk_tool_button_new (
        image,
        _("Step over"));
    gtk_widget_set_tooltip_text (GTK_WIDGET (tool_button),
        _("Execute the machine code step by step\nbut don't jump to sub-programs and\nbackward loops"));
    g_signal_connect(G_OBJECT(tool_button),
                     "clicked",
                     G_CALLBACK (do_step_over),
                     (gpointer) TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (tool_button), -1);

    /* Tool item run */
    sysicon = std_GetTeoSystemFile(RUN_ICON, false);
    pixbuf = gdk_pixbuf_new_from_file (sysicon, NULL);

    image = gtk_image_new_from_pixbuf (pixbuf);
    run_button = gtk_tool_button_new (image, _("Run"));
    g_signal_connect(G_OBJECT(run_button),
                     "clicked",
                     G_CALLBACK (do_quit),
                     (gpointer) TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (run_button), -1);

    /* Tool item leave */
    sysicon = std_GetTeoSystemFile(LEAVE_ICON, false);
    pixbuf = gdk_pixbuf_new_from_file (sysicon, NULL);

    image = gtk_image_new_from_pixbuf (pixbuf);
    tool_button = gtk_tool_button_new (
        image,
        _("Leave"));
    g_signal_connect(G_OBJECT(tool_button),
                     "clicked",
                     G_CALLBACK (do_quit),
                     (gpointer) FALSE);
    gtk_toolbar_insert (GTK_TOOLBAR (tool_bar), GTK_TOOL_ITEM (tool_button), -1);

    return box;
}
