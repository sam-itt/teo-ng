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
 *  Module     : linux/udebug/udbkpt.c
 *  Version    : 1.8.5
 *  Cr�� par   : Fran�ois Mouret 14/07/2016
 *  Modifi� par: Samuel Cuella 02/2020
 *
 *  D�bogueur 6809 - Gestion des breakpoints.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <ctype.h>
   #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "linux/gui.h"
#include "gettext.h"

static GtkWidget *entry[MAX_BREAKPOINTS];


/* key_press_event:
 *  Gestion des touches enfonc�es.
 */
static gboolean
key_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    guint32 unicode = gdk_keyval_to_unicode (event->key.keyval);

    if ((isxdigit ((int)unicode) != 0) || (isgraph ((int)unicode) == 0))
    {
        return gtk_entry_im_context_filter_keypress (
                    GTK_ENTRY (widget),
                    (GdkEventKey*)event);
    }
    return TRUE;
    (void)widget;
    (void)user_data;
}



/* entry_changed:
 *  Update the display.
 */
static void entry_changed (GtkEditable *editable, gpointer user_data)
{
    int i;
    int addr;
    int run_button_activation = FALSE;
    const char *hex_text;

    /* disable the RUN button if no breakpoint */
    for (i=0; i<MAX_BREAKPOINTS; i++)
    {
        hex_text = gtk_entry_get_text GTK_ENTRY(entry[i]);
        if (strlen (hex_text) == 0)
        {
            teo.debug.breakpoint[i] = -1;
        }
        else
        {
            run_button_activation = TRUE;
            sscanf (hex_text, "%x", &addr);
            teo.debug.breakpoint[i] = addr;
        }
    }

    udtoolb_SetRunButtonSensitivity (run_button_activation);
    (void)editable;
    (void)user_data;
}    
    

/* ------------------------------------------------------------------------- */



/* udbkpt_Free:
 *  Free the memory used by the breakpoints.
 */
void udbkpt_Free(void)
{
}



/* udbkpt_Init:
 *  Init breakpoints.
 */
GtkWidget *udbkpt_Init (void)
{
    int i;
    char hex_text[6] = "";
    GtkWidget *grid;
    GtkWidget *frame;
    GtkWidget *box;

    /* Create breakpoint grid */
    grid = gtk_grid_new ();
    gtk_widget_set_hexpand (grid, FALSE);
    gtk_widget_set_vexpand (grid, FALSE);
    for (i=0; i<2; i++)
        gtk_grid_insert_row (GTK_GRID (grid), 0);
    for (i=0; i<8; i++)
        gtk_grid_insert_column (GTK_GRID (grid), 0);

    for (i=0; i<MAX_BREAKPOINTS; i++)
    {
        /* The GtkEntry seems to resize incorrectly */
        entry[i] = gtk_entry_new ();
        gtk_widget_set_hexpand (entry[i], FALSE);
        gtk_widget_set_vexpand (entry[i], FALSE);
        gtk_entry_set_max_length (GTK_ENTRY (entry[i]), 4);
        gtk_entry_set_width_chars (GTK_ENTRY (entry[i]), 4);

        box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX(box), entry[i], FALSE, FALSE, 0);
        gtk_grid_attach (GTK_GRID (grid), box,
                         i%(MAX_BREAKPOINTS/2), i/(MAX_BREAKPOINTS/2),
                         1, 1);

        if (teo.debug.breakpoint[i] != -1)
        {
            sprintf (hex_text, "%x", teo.debug.breakpoint[i]&0xffff);
            gtk_entry_set_text (GTK_ENTRY (entry[i]), hex_text);
        }
        gtk_widget_add_events (entry[i], GDK_KEY_PRESS_MASK);
        g_signal_connect (G_OBJECT (entry[i]), "key-press-event",
                          G_CALLBACK (key_press_event), GINT_TO_POINTER (i));
        g_signal_connect (G_OBJECT (entry[i]), "changed",
                          G_CALLBACK (entry_changed), GINT_TO_POINTER (i));
    }

    frame = gtk_frame_new (_("Breakpoints"));
    gtk_widget_set_hexpand (frame, FALSE);
    gtk_widget_set_vexpand (frame, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), grid);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX(box), frame, FALSE, FALSE, 0);

    return box;
}

