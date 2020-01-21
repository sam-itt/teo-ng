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
 *  Module     : linux/udebug/udmem.c
 *  Version    : 1.8.5
 *  Créé par   : François Mouret 14/07/2016
 *  Modifié par:
 *
 *  Débogueur 6809 - Affichage de la mémoire.
 */


#ifndef SCAN_DEPEND
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/dasm6809.h"
#include "to8dbg.h"
#include "debug/debug.h"
#include "linux/gui.h"

#define HIGHLIGHT_NAME  "teo_dmem_highlight"

static GtkWidget *address_combo;
static GtkWidget *ram_combo;
static GtkWidget *video_combo;
static GtkWidget *cart_combo;
static GtkWidget *monitor_combo;

static GtkWidget *text_view;
static GtkTextBuffer *text_buffer;
static GtkTextMark *mark_first;
static GtkTextMark *mark_last;

static char *old_text = NULL;

static int prev_address = -1;



/* set_text:
 *  Set the text in the textview.
 */
static void set_text (int address, uint8 *addr_ptr)
{
    char *text = NULL;

    /* display the new text */
    text = dmem_GetText (address&0xe000, addr_ptr, "\n");

    /* Set the new text if different */
    if (text != NULL)
    {
        if ((old_text == NULL) || (memcmp (text, old_text, strlen(text)) != 0))
        {
            gtk_text_buffer_set_text (
                GTK_TEXT_BUFFER (text_buffer),
                text,
                -1);
        }
    }
    /* Keep new text as old text */
    if (old_text != NULL)
        free (old_text);
    old_text = text;
}

        

static int get_first_visible_line_number (void)
{
    int y;
    int y_next;
    int height;
    int height_next;
    GdkRectangle rect;
    GtkTextIter iter;
    GtkTextIter iter_next;
    GtkTextView *view = GTK_TEXT_VIEW (text_view);

    gtk_text_view_get_visible_rect (view, &rect);
    gtk_text_view_get_iter_at_location (view, &iter, 0, rect.y);
    gtk_text_view_get_line_yrange (view, &iter, &y, &height);
    gtk_text_view_get_iter_at_location (view, &iter_next, 0, rect.y+height);
    gtk_text_view_get_line_yrange (view, &iter_next, &y_next, &height_next);
        
    if (y < y_next)
        y = y_next;

    return (height != 0) ? y/height : y;
}


/* do_combo_changed:
 *  Update the combo row value
 */
static void do_combo_changed (GtkComboBox *combo_box, gpointer user_data)
{
    int *combo_row = user_data;

    *combo_row = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
    teo.debug.ram_number   = gtk_combo_box_get_active (
                                 GTK_COMBO_BOX (ram_combo));
    teo.debug.mon_number   = gtk_combo_box_get_active (
                                 GTK_COMBO_BOX (monitor_combo));
    teo.debug.video_number = gtk_combo_box_get_active (
                                 GTK_COMBO_BOX (video_combo));
    teo.debug.cart_number  = gtk_combo_box_get_active (
                                 GTK_COMBO_BOX (cart_combo));
    teo.debug.memory_address = gtk_combo_box_get_active (
                                 GTK_COMBO_BOX (address_combo))*0x2000;
    teo.debug.memory_address += get_first_visible_line_number() * 8;
    set_text (teo.debug.memory_address, dmem_GetDisplayPointer());
    gtk_widget_show (GTK_WIDGET (text_view));

}



/* create_address:
 *  Create address label and combo
 */
static void create_address_widgets (GtkWidget *grid)
{
    int i;
    char addr[20] = "";
    GtkWidget *label = gtk_label_new (is_fr?"Adresse:":"Address:");

    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

    address_combo = gtk_combo_box_text_new ();
    for (i=0; i<8; i++)
    {
        *addr = '\0';
        sprintf (addr, "$%04X", i*0x2000);
        gtk_combo_box_text_append_text (
            GTK_COMBO_BOX_TEXT (address_combo),
            addr);
    }
    gtk_combo_box_set_active (
        GTK_COMBO_BOX (address_combo),
        (teo.debug.memory_address >> 13) & 0x7);
    gtk_grid_attach (GTK_GRID (grid), address_combo, 0, 1, 1, 1);
    g_signal_connect(
        G_OBJECT(address_combo),
        "changed",
        G_CALLBACK (do_combo_changed),
        (gpointer) &teo.debug.memory_address);
}



/* create_cartridge_widgets:
 *  Create cartridge label and combo
 */
static void create_cartridge_widgets (GtkWidget *grid)
{
    int i;
    char addr[20] = "";
    GtkWidget *label = gtk_label_new (is_fr?"Cartouche:":"Cartridge:");

    gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);

    cart_combo = gtk_combo_box_text_new ();
    for (i=0; i<4; i++)
    {
        *addr = '\0';
        sprintf (addr, "%d", i);
        gtk_combo_box_text_append_text (
            GTK_COMBO_BOX_TEXT (cart_combo),
            addr);
    }
    gtk_combo_box_text_append_text (
        GTK_COMBO_BOX_TEXT (cart_combo),
        is_fr?"Memo":"Memo");
        
    for (i=0; i<teo.setting.bank_range; i++)
    {
        *addr = '\0';
        sprintf (addr, is_fr?"Banque %d":"Bank %d", i);
        gtk_combo_box_text_append_text (
            GTK_COMBO_BOX_TEXT (cart_combo),
            addr);
    }
    gtk_combo_box_set_active (
        GTK_COMBO_BOX (cart_combo),
        teo.debug.cart_number);
    gtk_grid_attach (GTK_GRID (grid), cart_combo, 1, 1, 1, 1);
    g_signal_connect(
        G_OBJECT(cart_combo),
        "changed",
        G_CALLBACK (do_combo_changed),
        (gpointer) &teo.debug.cart_number);
}



/* create_video_widgets:
 *  Create video label and combo
 */
static void create_video_widgets (GtkWidget *grid)
{
    GtkWidget *label = gtk_label_new (is_fr?"VidÃ©o:":"Video:");

    gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);

    video_combo = gtk_combo_box_text_new ();
    gtk_combo_box_text_append_text (
        GTK_COMBO_BOX_TEXT (video_combo),
        is_fr?"Forme":"Form");
    gtk_combo_box_text_append_text (
        GTK_COMBO_BOX_TEXT (video_combo),
        is_fr?"Couleur":"Colour");
    gtk_combo_box_set_active (
        GTK_COMBO_BOX (video_combo),
        teo.debug.video_number);
    gtk_grid_attach (GTK_GRID (grid), video_combo, 2, 1, 1, 1);
    g_signal_connect(
        G_OBJECT(video_combo),
        "changed",
        G_CALLBACK (do_combo_changed),
        (gpointer) &teo.debug.video_number);
}



/* create_ram_widgets:
 *  Create ram label and combo
 */
static void create_ram_widgets (GtkWidget *grid)
{
    int i;
    char addr[20] = "";
    GtkWidget *label = gtk_label_new (is_fr?"RAM:":"RAM:");

    gtk_grid_attach (GTK_GRID (grid), label, 3, 0, 1, 1);

    ram_combo = gtk_combo_box_text_new ();
    for (i=0; i<teo.setting.bank_range; i++)
    {
        *addr = '\0';
        sprintf (addr, "%d", i);
        gtk_combo_box_text_append_text (
            GTK_COMBO_BOX_TEXT (ram_combo),
            addr);
    }
    gtk_combo_box_set_active (
        GTK_COMBO_BOX (ram_combo),
        teo.debug.ram_number);
    gtk_grid_attach (GTK_GRID (grid), ram_combo, 3, 1, 1, 1);
    g_signal_connect(
        G_OBJECT(ram_combo),
        "changed",
        G_CALLBACK (do_combo_changed),
        (gpointer) &teo.debug.ram_number);
}



/* create_monitor_widgets:
 *  Create monitor label and combo
 */
static void create_monitor_widgets (GtkWidget *grid)
{
    int i;
    char addr[20] = "";
    GtkWidget *label = gtk_label_new (is_fr?"Moniteur:":"Monitor:");

    gtk_grid_attach (GTK_GRID (grid), label, 4, 0, 1, 1);

    monitor_combo = gtk_combo_box_text_new ();
    for (i=0; i<2; i++)
    {
        *addr = '\0';
        sprintf (addr, "%d", i);
        gtk_combo_box_text_append_text (
            GTK_COMBO_BOX_TEXT (monitor_combo),
            addr);
    }
    gtk_combo_box_set_active (
        GTK_COMBO_BOX (monitor_combo),
        teo.debug.mon_number);
    gtk_grid_attach (GTK_GRID (grid), monitor_combo, 4, 1, 1, 1);
    g_signal_connect(
        G_OBJECT(monitor_combo),
        "changed",
        G_CALLBACK (do_combo_changed),
        (gpointer) &teo.debug.mon_number);
}



/* init_combos:
 *  Initialization of the combobox.
 */
static GtkWidget *init_combos (void)
{
    int i;
    GtkWidget *box;
    GtkWidget *grid;

    /* Create combo grid */
    grid = gtk_grid_new ();
    for (i=0; i<3; i++)
        gtk_grid_insert_row (GTK_GRID (grid), 0);
    for (i=0; i<5; i++)
        gtk_grid_insert_column (GTK_GRID (grid), 0);

    /* Create widgets */
    create_address_widgets (grid);
    create_cartridge_widgets (grid);
    create_video_widgets (grid);
    create_ram_widgets (grid);
    create_monitor_widgets (grid);

    /* initialize the combo line */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start( GTK_BOX(box), GTK_WIDGET(grid), FALSE, FALSE, 0);

    return box;
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
        gtk_text_view_scroll_to_mark (view, mark_first, 0.0, TRUE, 0.0, 0.5);

        /* Wait for end of GTK work */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark_first);
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
static void scroll_text (int value_line, int address)
{
    int count;

    count = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (text_buffer));
    if (count > 0)
    {
        add_mark (mark_first, value_line, 6+(address&0x07)*3);
        add_mark (mark_last, value_line, 6+(address&0x07)*3+2);
        scroll_to_mark (GTK_TEXT_VIEW (text_view));
    }
}



/* unselect_range :
 *  Unselect the text
 */
static void unselect_range (GtkTextBuffer *buf)
{
    GtkTextIter start;
    GtkTextIter end;

    gtk_text_buffer_get_start_iter (buf, &start);
    gtk_text_buffer_get_end_iter (buf, &end);
    gtk_text_buffer_remove_tag_by_name (buf, HIGHLIGHT_NAME, &start, &end);

    /* Wait for end of GTK work */
    while (gtk_events_pending ())
        gtk_main_iteration ();
}



/* select_range :
 *  Select and highlight the current value
 */
static void select_range (GtkTextBuffer *buf)
{
    GtkTextIter start;
    GtkTextIter end;

    gtk_text_buffer_get_iter_at_mark (buf, &start, mark_first);
    gtk_text_buffer_get_iter_at_mark (buf, &end, mark_last);
    gtk_text_buffer_apply_tag_by_name (buf, HIGHLIGHT_NAME, &start, &end);

    /* Wait for end of GTK work */
    while (gtk_events_pending ())
        gtk_main_iteration ();
}



/* display:
 *  Display the memory dump.
 */
static void display (int address, uint8 *addr_ptr)
{ 
    int value_line = (address & 0x1fff) >> 3;

    /* Set and scroll text */
    unselect_range (GTK_TEXT_BUFFER (text_buffer));
    set_text (address, addr_ptr);
    scroll_text (value_line, address);
    select_range (GTK_TEXT_BUFFER (text_buffer));
}



/* init_source_text:
 *  Initialization of the source text.
 */
static GtkWidget *init_source_text (void)
{
    GtkWidget *box;
    GtkWidget *scrolled_window;
    GtkTextTagTable *tag_table;
    GtkTextTag *tag_highlight;

    /* Create source buffer */
    tag_table = gtk_text_tag_table_new ();
    text_buffer = gtk_text_buffer_new (tag_table);
    tag_highlight = gtk_text_tag_new (HIGHLIGHT_NAME);
    g_object_set (
        (gpointer)tag_highlight,
        "foreground", "white",
        "background", "crimson",
        "weight", PANGO_WEIGHT_BOLD,
         NULL);
    gtk_text_tag_table_add (tag_table, tag_highlight);
    mark_first  = gtk_text_mark_new ("teo_dmem_mark_first", FALSE);
    mark_last   = gtk_text_mark_new ("teo_dmem_mark_last", FALSE);

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

    /* Pack text view */
    gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);

    /* Set Courier font */
    gtk_widget_set_name (scrolled_window, COURIER_DEBUG);

    /* Pack everything in box */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);

    return box;
}


/* ------------------------------------------------------------------------- */


/* udmem_Free:
 *  Free the memory used by the memory area.
 */
void udmem_Free(void)
{
    /* Free old text memory */
    if (old_text != NULL)
    {
        free (old_text);
        old_text = NULL;
    }
}



/* udmem_Init:
 *  Create widgets.
 */
GtkWidget *udmem_Init(void)
{ 
    GtkWidget *box;

    prev_address = -1;

    /* initialize the source box */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start( GTK_BOX(box), init_combos (), FALSE, FALSE, 0);
    gtk_box_pack_start( GTK_BOX(box), init_source_text (), TRUE, TRUE, 0);
    
    return box;
}



/* udmem_GetStepAddress:
 *  Get the jump address in step mode.
 */
int udmem_GetStepAddress(void)
{
    return dmem_GetJumpAddress();
}



/* udmem_StepDisplay:
 *  Display memory for disassembling.
 */
void udmem_StepDisplay(int address)
{
    int index = 0;

    if  (address == -1)
        address = prev_address;

    if (address != -1)
    {
        gtk_combo_box_set_active (
            GTK_COMBO_BOX (address_combo),
            (address >> 13) & 0x0f);
        
        gtk_combo_box_set_active (
            GTK_COMBO_BOX (ram_combo),
            mempager.data.reg_page);

        gtk_combo_box_set_active (
            GTK_COMBO_BOX (video_combo),
            1-mempager.screen.page);

        gtk_combo_box_set_active (
            GTK_COMBO_BOX (monitor_combo),
            mempager.mon.page);

        if (mode_page.cart&0x20)
            index = 5+mempager.cart.ram_page;
        else
        if (mc6846.prc&4)
            index = mempager.cart.rom_page;
        else
            index = 4;

        gtk_combo_box_set_active (GTK_COMBO_BOX (cart_combo),index);

        display(address, NULL);
    }
}



/* udmem_Display:
 *  Display choosen memory slice.
 */
void udmem_Display(void)
{
    uint8 *addr_ptr = NULL;

    addr_ptr = dmem_GetDisplayPointer();
    set_text (teo.debug.memory_address, addr_ptr);
    scroll_text (
        (teo.debug.memory_address&0x1fff)>>3,
        teo.debug.memory_address);
        
}
