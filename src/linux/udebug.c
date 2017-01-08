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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/udebug.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 08/10/2013
 *  Modifié par: François Mouret 04/06/2015 14/07/2016
 *
 *  Débogueur 6809.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "linux/gui.h"
#include "linux/display.h"
#include "debug.h"
#include "debug/debug.h"

static GtkWidget *window;
struct MC6809_DEBUG debug;



/* delete_event:
 *  Gestion du gadget de fermeture.
 */
static gboolean delete_event (GtkWidget *widget,
             GdkEvent *event,
             gpointer user_data)
{
    dbkpt_TraceOff();
    teo.command = TEO_COMMAND_NONE;
    gtk_widget_hide (window);
    gtk_main_quit ();
    return TRUE;
    (void)widget;
    (void)event;
    (void)user_data;
}



/* delete_event:
 *  Get maximize status of the debug window
 */
static gboolean window_state_event (GtkWidget *widget,
               GdkEvent *event,
               gpointer user_data)
{
    if ((event->window_state.new_window_state
     & GDK_WINDOW_STATE_MAXIMIZED) != 0)
        teo.debug.window_maximize = TRUE;
    else
        teo.debug.window_maximize = FALSE;

    return FALSE;
    (void)widget;
    (void)user_data;
}



/* configure_event:
 *  Get size and position of the debug window
 */
static gboolean configure_event (GtkWidget *widget,
               GdkEvent *event,
               gpointer user_data)
{
    gtk_window_get_position (
        GTK_WINDOW (window),
        &teo.debug.window_x,
        &teo.debug.window_y);
    gtk_window_get_size (
        GTK_WINDOW (window),
        &teo.debug.window_width,
        &teo.debug.window_height);

    return FALSE;
    (void)widget;
    (void)user_data;
}


/* update_display:
 *  Update the debugger display in step mode.
 *  The memory display programm is executed externaly.
 */
static void update_display (void)
{
    udstatus_Display();
    uddisass_Display();
    udreg_Display();
    udacc_Display();
}


/* ------------------------------------------------------------------------- */


void udebug_DoStepOver (void)
{
    int address;

    address = udmem_GetStepAddress();
    uddisass_DoStepOver(window);
    udmem_StepDisplay(address);
    update_display ();
}



void udebug_DoStepByStep (void)
{
    int address;

    address = udmem_GetStepAddress();
    uddisass_DoStep();
    udmem_StepDisplay(address);
    update_display ();
}



void udebug_Quit (int quit_mode)
{
    if (quit_mode == TRUE)
    {
        dbkpt_TraceOn();
    }
    else
    {
        dbkpt_TraceOff();
    }
    gtk_widget_hide (window);
    gtk_main_quit ();
}



void udebug_Free (void)
{
    uddisass_Free();
    udmem_Free();
    udreg_Free();
    udacc_Free();
    udbkpt_Free();
}



/* udebug_Init:
 *  Init the debugger dialog.
 */
void udebug_Init(void)
{
    GdkGeometry geometry;
    static struct MC6809_REGS regs;
    GtkWidget *main_box;
    GtkWidget *area_box;
    GtkWidget *box;
    GtkCssProvider *cssprovider;
    GdkDisplay *display = gdk_display_get_default ();
    GdkScreen *screen = gdk_display_get_default_screen (display);

    mc6809_GetRegs(&regs);
    mc6809_FlushExec ();
    mc6809_GetRegs(&regs);

    /* Set font style */
    cssprovider = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (
        cssprovider,
        "#"COURIER_DEBUG" {" \
        "    font-family: Courier;" \
        "    font-size: 1.0em;" \
        "}",
        -1,
        NULL);
    gtk_style_context_add_provider_for_screen (
        screen,
        GTK_STYLE_PROVIDER(cssprovider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref (cssprovider);

    /* Create window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(wMain));
    gtk_window_set_destroy_with_parent (GTK_WINDOW(wMain), TRUE);
    gtk_window_set_title (
        GTK_WINDOW(window),
        is_fr?"Teo - DÃ©bogueur"
             :"Teo - Debugger");

    /* Connect signals */
    gtk_widget_add_events (window, GDK_STRUCTURE_MASK);
    g_signal_connect ( G_OBJECT (window), "delete-event",
                       G_CALLBACK (delete_event), NULL);
    g_signal_connect ( G_OBJECT (window), "window-state-event",
                       G_CALLBACK (window_state_event), NULL);
    g_signal_connect ( G_OBJECT (window), "configure-event",
                       G_CALLBACK (configure_event), NULL);

    /* Set window size parameters */
    gtk_window_set_resizable (GTK_WINDOW(window), TRUE);
    geometry.min_width = 640;
    geometry.min_height = 440;
    gtk_window_set_geometry_hints (
        GTK_WINDOW(window),
        NULL,
        &geometry,
        GDK_HINT_MIN_SIZE);
    gtk_window_set_default_size (
        GTK_WINDOW (window),
        teo.debug.window_width,
        teo.debug.window_height);
    gtk_window_move (
        GTK_WINDOW (window),
        teo.debug.window_x,
        teo.debug.window_y);
    if (teo.debug.window_maximize == TRUE)
        gtk_window_maximize (GTK_WINDOW (window));

    /* Create boxes */
    main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), main_box);
    gtk_container_set_border_width (GTK_CONTAINER (window), 4);

    /* Add toolbar, box area and status bar */
    gtk_box_pack_start(GTK_BOX(main_box), udtoolb_Init(), FALSE, FALSE, 0);
    area_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(main_box), area_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), udstatus_Init(), FALSE, FALSE, 0);

    /* Add left box */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    /* The GTK documentation says : "This parameter [i.e. fill] has
       no effect if expand is set to FALSE". And yet, strangely, the
       EXPAND parameter must be set here to FALSE and the FILL one
       must be set to TRUE to provide the expected behaviour. A GTK
       bug ? To be closely watched ! */
    gtk_box_pack_start(GTK_BOX(area_box), box, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), udacc_Init(), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), udbkpt_Init(), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), udreg_Init(), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), udmem_Init(), TRUE, TRUE, 4);
    
    /* Add right box */
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(area_box), box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), uddisass_Init(), TRUE, TRUE, 4);
}



/* udebug_Panel:
 *  Display the debugger dialog.
 */
void udebug_Panel(void)
{
    /* Display content */
    udmem_Display ();
    update_display ();

    teo.command = TEO_COMMAND_NONE;

    gtk_widget_show_all (window);
    
    udreg_UpdateText();

    gtk_main ();
}
