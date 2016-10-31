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
 *  Module     : linux/udebug/udreg.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 14/07/2016
 *  Modifié par:
 *
 *  Débogueur 6809 - Affichage des registres.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/mc6821.h"
#include "mc68xx/dasm6809.h"
#include "media/disk.h"
#include "debug.h"
#include "debug/debug.h"
#include "linux/gui.h"

static GtkWidget *text_view;
static GtkTextBuffer *text_buffer;
static GtkTextMark *text_mark;

static int get_first_visible_line_number (void)
{
    int y;
    int height;
    GdkRectangle rect;
    GtkTextIter iter;
    GtkTextView *view = GTK_TEXT_VIEW (text_view);

    gtk_text_view_get_visible_rect (view, &rect);
    gtk_text_view_get_iter_at_location (view, &iter, 0, rect.y);
    gtk_text_view_get_line_yrange (view, &iter, &y, &height);

    return (height != 0) ? y/height : y;
}



/* scroll_to_mark :
 *  Scroll to the mark in the textview
 */
static void scroll_to_mark (GtkTextView *view)
{
    GtkTextIter iter;
    GdkRectangle rect_view;
    GdkRectangle rect_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);

    do
    {
        gtk_text_view_scroll_to_mark (view, text_mark, 0.0, TRUE, 0.0, 0.0);

        /* Wait for end of GTK work */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        gtk_text_buffer_get_iter_at_mark (buffer, &iter, text_mark);
        gtk_text_view_get_iter_location (view, &iter, &rect_iter);
        gtk_text_view_get_visible_rect (view, &rect_view);

    } while (((rect_view.y + rect_view.height) < (rect_iter.y+rect_iter.height))
           || (rect_view.y > rect_iter.y));
}



/* add_mark:
 *  Add/Move a mark in the textview.
 */
static void add_mark (GtkTextMark *mark, int line, int char_pos)
{
    GtkTextIter iter;
    GtkTextBuffer *buff = GTK_TEXT_BUFFER (text_buffer);

    gtk_text_buffer_get_iter_at_line_offset (buff, &iter, line, char_pos);
    if (gtk_text_mark_get_deleted (mark) == TRUE)
    {
        gtk_text_buffer_add_mark (buff, mark, &iter);
    }
    else
    {
        gtk_text_buffer_move_mark (buff, mark, &iter);
    }
}



/* scroll_text:
 *  Scroll the text in the textview.
 */
static void scroll_text (int value_line)
{
    int count;

    count = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (text_buffer));
    if (count > 0)
    {
        add_mark (text_mark, value_line, 0);
        scroll_to_mark (GTK_TEXT_VIEW (text_view));
    }
}



static void set_text (void)
{
    char *text = NULL;

    text = dreg_GetText ("\n");
    if (text != NULL)
    {
        /* Wait for end of GTK work */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        gtk_text_buffer_set_text (GTK_TEXT_BUFFER (text_buffer), text, -1);

        /* Wait for end of GTK work */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        std_free(text);
    }
}


/* ------------------------------------------------------------------------- */


/* udreg_Free:
 *  Free the memory used by the register area.
 */
void udreg_Free(void)
{
}



/* udreg_Init:
 *  Init register area.
 */
GtkWidget *udreg_Init(void)
{
    GtkWidget *box;
    GtkWidget *scrolled_window;

    text_buffer = gtk_text_buffer_new (NULL);
    text_mark = gtk_text_mark_new ("teo_dreg_mark_first", FALSE);

    /* Create source view */
    text_view = gtk_text_view_new_with_buffer (text_buffer);
    gtk_text_view_set_editable (GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(text_view), FALSE);

    /* Create scroll window */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
        GTK_SCROLLED_WINDOW (scrolled_window),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height (
        GTK_SCROLLED_WINDOW (scrolled_window),
        80);

    /* Set Courier font */
    gtk_widget_set_name (scrolled_window, COURIER_DEBUG);

    /* Pack text view */
    gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);

    /* Pack everything in box */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX(box), scrolled_window, FALSE, FALSE, 0);

    return box;
}



/* udreg_Display:
 *  Display extra registers.
 */
void udreg_Display(void)
{
    teo.debug.extra_first_line = get_first_visible_line_number ();
    udreg_UpdateText ();
}



/* udreg_UpdateText:
 *  Set and ccroll text.
 */
void udreg_UpdateText(void)
{
    set_text ();
    scroll_text (teo.debug.extra_first_line);
}
