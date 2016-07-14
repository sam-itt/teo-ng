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
 *  Module     : linux/udebug/udbkpt.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 14/07/2016
 *  Modifié par:
 *
 *  Débogueur 6809 - Gestion des breakpoints.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <ctype.h>
   #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "linux/gui.h"

static GtkWidget *entry[MAX_BREAKPOINTS];


/* key_press_event:
 *  Gestion des touches enfoncées.
 */
static gboolean
key_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    guint32 unicode = gdk_keyval_to_unicode (event->key.keyval);

    if ((unicode < 32) || (isxdigit ((int)unicode) != 0))
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
    const char *hex_text;

    /* disable the RUN button if no breakpoint */
    for (i=0; i<MAX_BREAKPOINTS; i++)
    {
        hex_text = gtk_entry_get_text GTK_ENTRY(entry[i]);
        if (strlen (hex_text) != 0)
            break;
    }
    udtoolb_SetRunButtonSensitivity ((i == MAX_BREAKPOINTS)?FALSE:TRUE);
    (void)editable;
    (void)user_data;
}    
    

/* ------------------------------------------------------------------------- */


/* udbkpt_Init:
 *  Init breakpoints.
 */
GtkWidget *udbkpt_Init (void)
{
    int i;
    char hex_text[6] = "";
    GtkWidget *frame;
    GtkWidget *grid;

    /* Entries grid */
    grid = gtk_grid_new ();
    for (i=0; i<(MAX_BREAKPOINTS/8); i++)
        gtk_grid_insert_row (GTK_GRID (grid), 0);
    for (i=0; i<(MAX_BREAKPOINTS/2); i++)
        gtk_grid_insert_column (GTK_GRID (grid), 0);
    
    for (i=0; i<MAX_BREAKPOINTS; i++)
    {
        entry[i] = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry[i]), 4);
        gtk_entry_set_width_chars (GTK_ENTRY (entry[i]), 4);
        gtk_entry_set_input_purpose (
            GTK_ENTRY (entry[i]),
            GTK_INPUT_PURPOSE_FREE_FORM);
        gtk_grid_attach (GTK_GRID (grid),
                         entry[i],
                         i%(MAX_BREAKPOINTS/2),
                         i/(MAX_BREAKPOINTS/2),
                         1,
                         1);
        if (teo.debug.breakpoint[i] != -1)
        {
            sprintf (hex_text, "%x", teo.debug.breakpoint[i]&0xffff);
            gtk_entry_set_text (GTK_ENTRY (entry[i]), hex_text);
        }
        gtk_widget_add_events (entry[i], GDK_KEY_PRESS_MASK);
        g_signal_connect (G_OBJECT (entry[i]), "key-press-event",
                          G_CALLBACK (key_press_event), (gpointer) i);
        g_signal_connect (G_OBJECT (entry[i]), "changed",
                          G_CALLBACK (entry_changed), (gpointer) i);
    }
    frame = gtk_frame_new (is_fr?"Points d'arrÃªts":"Breakpoints");
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (grid));

    return frame;
}



/* udbkpt_Exit:
 *  Exit the breakpoints.
 */
void udbkpt_Exit (void)
{
    int i;
    int addr = 0;
    const char *hex_text;
    
    for (i=0; i<MAX_BREAKPOINTS; i++)
    {
        hex_text = gtk_entry_get_text GTK_ENTRY(entry[i]);
        if (strlen (hex_text) == 0)
        {
            teo.debug.breakpoint[i] = -1;
        }
        else
        {
            sscanf (hex_text, "%x", &addr);
            teo.debug.breakpoint[i] = addr;
        }
    }
}

