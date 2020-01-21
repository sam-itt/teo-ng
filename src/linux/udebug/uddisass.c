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
 *  Module     : linux/udebug/uddisass.c
 *  Version    : 1.8.5
 *  Cr�� par   : Fran�ois Mouret 14/07/2016
 *  Modifi� par:
 *
 *  D�bogueur 6809 - Affichage des mn�moniques.
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
#include "mc68xx/mc6809.h"
#include "to8dbg.h"
#include "debug/debug.h"
#include "linux/gui.h"

#define HIGHLIGHT_NAME  "teo_ddisass_highlight"

static struct MC6809_REGS regs;
static GtkWidget *text_view;
static GtkTextBuffer *text_buffer;
static GtkTextMark *mark_first;
static GtkTextMark *mark_last;
static char *old_text = NULL;


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



static int get_last_visible_line_number (void)
{
    int y;
    int y_next;
    int height;
    int height_next;
    GdkRectangle rect;
    GtkTextIter iter;
    GtkTextIter iter_next;
    GtkTextView *view = GTK_TEXT_VIEW (text_view);
    int offset;

    gtk_text_view_get_visible_rect (view, &rect);
    gtk_text_view_get_iter_at_location (view, &iter, 0, rect.y+rect.height-1);
    gtk_text_view_get_line_yrange (view, &iter, &y, &height);
    offset = rect.y+rect.height-1-(height-1);
    gtk_text_view_get_iter_at_location (view, &iter_next, 0, offset);
    gtk_text_view_get_line_yrange (view, &iter_next, &y_next, &height_next);
        
    if (y > y_next)
        y = y_next;

    return (height != 0) ? y/height : y;
}



/* set_text:
 *  Set the text in the textview.
 */
static void set_text (void)
{
    char *text = NULL;

    /* Get the new text */
    text = ddisass_GetText ("\n");

    /* Set the new text if different */
    if (text != NULL)
    {
        if ((old_text == NULL) || (memcmp (text, old_text, strlen(text)) != 0))
        {
            /* Wait for end of GTK work */
            while (gtk_events_pending ())
                gtk_main_iteration ();

            gtk_text_buffer_set_text (
                GTK_TEXT_BUFFER (text_buffer),
                text,
                -1);

            /* Wait for end of GTK work */
            while (gtk_events_pending ())
                gtk_main_iteration ();
        }
    }
    /* Keep new text as old text */
    if (old_text != NULL)
        free (old_text);
    old_text = text;
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
static void scroll_text (void)
{
    int count;

    count = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (text_buffer));
    if (count > 0)
    {
        add_mark (mark_first, debug.line, 0);
        add_mark (mark_last, debug.line, DASM_LINE_LENGTH-1);
        scroll_to_mark (GTK_TEXT_VIEW (text_view));
    }
}



/* debug_display_dasm:
 *  Display the disassembling.
 */
static void debug_display_dasm (char *text)
{ 
    int visible_line_first;
    int visible_line_count;

    /* set text and line positionning */
    mc6809_GetRegs(&regs);
    visible_line_first = get_first_visible_line_number();
    visible_line_count = get_last_visible_line_number()-visible_line_first+1;
    
    ddisass_EditPositionning (
        regs.pc&0xffff,
        visible_line_first,
        visible_line_count);

    unselect_range (GTK_TEXT_BUFFER (text_buffer));

    /* display and scroll the new text */
    if (debug.force_display == TRUE)
    {
        /* Set and scroll text */
        set_text ();
        debug.force_display = FALSE;
    }
    scroll_text ();
    select_range (GTK_TEXT_BUFFER (text_buffer));
}



static void run_subroutine (GtkWidget *window, int next_pc)
{
    int watch=0;
    int sr = -1;
    int ptr = -1;

    do
    {
        mc6809_StepExec();
        mc6809_GetRegs(&regs);
        if (sr == -1)
        {
            sr = regs.sr&0xffff;
            ptr = (LOAD_BYTE(sr)<<8)|LOAD_BYTE(sr+1);
        }
    } while (((watch++)<WATCH_MAX)
          && (regs.pc != next_pc)
          && (regs.sr != (sr+2))
          && (ptr == ((LOAD_BYTE(sr)<<8)|LOAD_BYTE(sr+1))));

    if (ptr != ((LOAD_BYTE(sr)<<8)|LOAD_BYTE(sr+1)))
        ugui_Warning (is_fr
            ?"L'exécution pas-à-pas a été interrompue car le\n" \
             "pointeur de retour vient d'être changé dans la pile."
            :"The single-stepping has been aborted because the\n" \
             "return pointer has just been overwritten in stack.",
            window);
    else
    if ((regs.sr == (sr+2)) && (regs.pc != next_pc))
        ugui_Warning (is_fr
            ?"L'exécution pas-à-pas a été interrompue car le\n" \
             "pointeur de retour vient d'être dépilé avant le\n" \
             "retour du sous-programme."
            :"The single-stepping has been aborted because the\n" \
             "return pointer has just been pulled from stack before\n" \
             "the return from subroutine.",
            window);
    else
    if (watch > WATCH_MAX)
        ugui_Warning (is_fr
            ?"L'exécution pas-à-pas a été interrompue à cause du\n" \
             "trop grand nombre d'instructions exécutées.\n" \
             "La sous-routine peut comporter une boucle infinie."
            :"The single-stepping has been aborted because of the\n" \
             "great number of executed instructions.\n" \
             "The subroutine could have an infinite loop.",
            window);
    else
    if (regs.sr != (sr+2))
        ugui_Warning (is_fr
            ?"L'exécution pas-à-pas a été interrompue car\n" \
             "le pointeur de pile a changé."
            :"The single-stepping has been aborted because\n" \
             "the stack pointer has changed.",
            window);
}



static void exit_loop (GtkWidget *window, int next_pc)
{
    int watch=0;

    do
    {
        mc6809_StepExec();
        mc6809_GetRegs(&regs);
    } while (((watch++)<WATCH_MAX) && (regs.pc != next_pc));

    if (watch > WATCH_MAX)
        ugui_Warning (is_fr
            ?"L'exécution pas-à-pas a été interrompue à cause du\n" \
             "trop grand nombre d'instructions exécutées.\n" \
             "Il pourrait s'agir d'une boucle infinie."
            :"The single-stepping has been aborted because of the\n" \
             "great number of executed instructions.\n" \
             "It could be an infinite loop.",
            window);
}


/* ------------------------------------------------------------------------- */


/* uddisass_Free:
 *  Free the memory used by the disassembly area.
 */
void uddisass_Free(void)
{
    /* Free old text memory */
    if (old_text != NULL)
    {
        free (old_text);
        old_text = NULL;
    }
    debug.address = std_free(debug.address);
    debug.address_last = std_free(debug.address_last);
    debug.dump = std_free(debug.dump);
    debug.dump_last = std_free(debug.dump_last);
}



/* disass_init:
 *  Initialize the disassembly.
 */
GtkWidget *uddisass_Init (void)
{
    int i;
    int address;
    GtkWidget *box;
    GtkWidget *scrolled_window;
    GtkTextTagTable *tag_table;
    GtkTextTag *tag_highlight;

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
    mark_first  = gtk_text_mark_new ("teo_ddisass_mark_first", FALSE);
    mark_last   = gtk_text_mark_new ("teo_ddisass_mark_last", FALSE);

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
    gtk_scrolled_window_set_min_content_width (
        GTK_SCROLLED_WINDOW (scrolled_window),
        300);

    /* Pack text view */
    gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);

    /* Set Courier font */
    gtk_widget_set_name (scrolled_window, COURIER_DEBUG);

    /* Pack everything in box */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);

    debug.address = malloc((DASM_NLINES+1)*sizeof(int)*2);
    if (debug.address == NULL)
        return box;

    debug.address_last = malloc((DASM_NLINES+1)*sizeof(int)*2);
    if (debug.address_last == NULL)
        return box;

    debug.dump = malloc(DASM_NLINES*5);
    if (debug.dump == NULL)
        return box;

    debug.dump_last = malloc(DASM_NLINES*5);
    if (debug.dump_last == NULL)
        return box;

    mc6809_GetRegs(&regs);
    address = regs.pc&0xffff;
    if (debug.address != NULL)
    {
        for (i=0; i<DASM_NLINES+1; i++)
        {
            debug.address[i] = address;
            address = ddisass_GetNextAddress (address);
        }
        debug.force_display = TRUE;  /* force text display and scroll */
    }

    return box;
}



/* wddisass_DoStep:
 *  Execute the instructions step by step.
 */
void uddisass_DoStep (void)
{
    mc6809_StepExec();
}



/* wddisass_DoStepOver:
 *  Execute the instructions step by step skipping the jumps to subroutine.
 */
void uddisass_DoStepOver (GtkWidget *window)
{
    int pc;
    int next_pc;
    int offset;

    mc6809_GetRegs(&regs);
    pc = regs.pc;
    next_pc = ddisass_GetNextAddress (pc);

    switch (mc6809_dasm.fetch[0])
    {
        /* Jump to subroutine */
        case 0x9d : /* JSR direct */
        case 0xad : /* JSR indexed */
        case 0xbd : /* JSR extended */
        case 0x8d : /* BSR */
        case 0x17 : /* LBSR */
            run_subroutine (window, next_pc);
            break;

        /* Short branches */
        case 0x22 : /* BHI */
        case 0x23 : /* BLS */
        case 0x24 : /* BCC */
        case 0x25 : /* BCS */
        case 0x26 : /* BNE */
        case 0x27 : /* BEQ */
        case 0x28 : /* BVC */
        case 0x29 : /* BVS */
        case 0x2a : /* BPL */
        case 0x2b : /* BMI */
        case 0x2c : /* BGE */
        case 0x2d : /* BLT */
        case 0x2e : /* BGT */
        case 0x2f : /* BLE */
            /* Only backward branch */
            offset = mc6809_dasm.fetch[1]&0xff;
            if ((offset <= 0xfd) && (offset >= 0x80))
            {
                exit_loop (window, next_pc);
            }
            else
                mc6809_StepExec();
            break;

        /* Long branches */
        case 0x10 : /* Long branch */
            switch (mc6809_dasm.fetch[1])
            {
                case 0x22 : /* LBHI */
                case 0x23 : /* LBLS */
                case 0x24 : /* LBCC */
                case 0x25 : /* LBCS */
                case 0x26 : /* LBNE */
                case 0x27 : /* LBEQ */
                case 0x28 : /* LBVC */
                case 0x29 : /* LBVS */
                case 0x2a : /* LBPL */
                case 0x2b : /* LBMI */
                case 0x2c : /* LBGE */
                case 0x2d : /* LBLT */
                case 0x2e : /* LBGT */
                case 0x2f : /* LBLE */
                    /* Only backward branch */
                    offset = (mc6809_dasm.fetch[2]&0xff)<<8;
                    offset |= mc6809_dasm.fetch[3]&0xff;
                    if ((offset <= 0xfffb) && (offset >= 0x8000))
                    {
                        exit_loop (window, next_pc);
                    }
                    else
                        mc6809_StepExec();
                    break;

                /* Others */
                default :
                    mc6809_StepExec();
                    break;
            }
            break;

        /* Others */
        default :
            mc6809_StepExec();
            break;
    }
}



/* uddisass_Display:
 *  Display the disassembly.
 */
void uddisass_Display(void)
{ 
    char *text;

    if ((debug.address != NULL)
     && (debug.address_last != NULL)
     && (debug.dump != NULL)
     && (debug.dump_last != NULL))
    {
        text = ddisass_GetText ("\r\n");
        if (text != NULL)
        {
            debug_display_dasm(text);
            text = std_free(text);
        }
    }
}
