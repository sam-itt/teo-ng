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
 *  Module     : linux/udebug/udstatus.c
 *  Version    : 1.8.5
 *  Créé par   : François Mouret 14/07/2016
 *  Modifié par: Samuel Cuella 02/2020
 *
 *  Débogueur 6809 - Gestion de la barre d'état.
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
#include "gettext.h"

static GtkWidget *label_clock;
static GtkWidget *label_frame;
static GtkWidget *label_line;
static GtkWidget *label_column;

static mc6809_clock_t prev_clock = 0;



/* init:
 *  Initialize the status bar.
 */
static GtkWidget *init (void)
{
    GtkWidget *box;
    GtkWidget *grid;

    /* Create grid */
    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (grid), 20);
    
    /* Create clock label */
    label_clock = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID(grid), label_clock, 0, 0, 1, 1);

    /* Create frame label */
    label_frame = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID(grid), label_frame, 1, 0, 1, 1);

    /* Create line label */
    label_line = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID(grid), label_line, 2, 0, 1, 1);

    /* Create line label */
    label_column = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID(grid), label_column, 3, 0, 1, 1);

    /* Pack in main box */
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);

    return box;
}



/* display:
 *  Update the display of the status bar.
 */
static void display (void)
{
    char str[40] = "";
    mc6809_clock_t clock = mc6809_clock();

    /* display the clock */
    str[0] = '\0';
    sprintf (str, "%s: %lld",
             _("Clock"),
             clock-prev_clock);
    gtk_label_set_text (GTK_LABEL (label_clock), str);
    prev_clock = clock;

    /* display the frame */
    clock %= TEO_CYCLES_PER_FRAME;
    str[0] = '\0';
    sprintf (str, "%s: %lld", _("Frame"), clock);
    gtk_label_set_text (GTK_LABEL (label_frame), str);

    /* display the number of the line */

    str[0] = '\0';
    sprintf (str, "%s: %lld", _("Line"), clock/FULL_LINE_CYCLES);
    gtk_label_set_text (GTK_LABEL (label_line), str);

    /* display the number of the column */
    str[0] = '\0';
    sprintf (str, "%s: %lld", _("Column"), clock%FULL_LINE_CYCLES);
    gtk_label_set_text (GTK_LABEL (label_column), str);
}


/* ------------------------------------------------------------------------- */


/* udstatus_Free:
 *  Free the memory used by the status bar.
 */
void udstatus_Free (void)
{
}



/* udstatus_Init:
 *  Initialize the status bar.
 */
GtkWidget *udstatus_Init (void)
{
    return init ();
}



/* udstatus_Display:
 *  Update the display of the status bar.
 */
void udstatus_Display (void)
{
    display ();
}

