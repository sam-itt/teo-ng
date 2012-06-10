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
 *  Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/display.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 24/11/2003
 *               François Mouret 26/01/2010 08/2011 02/06/2012
 *               Gille Fétis 07/2011
 *
 *  Module d'interface avec le serveur X.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <gtk/gtk.h>
   #include <gtk/gtkx.h>
   #include <gdk/gdkx.h>
   #include <X11/Xlib.h>
   #include <X11/XKBlib.h>
   #include <X11/Xutil.h>
   #include <X11/keysym.h>
#endif

#include "linux/gui.h"
#include "linux/display.h"
#include "linux/graphic.h"
#include "to8keys.h"
#include "to8.h"
#include "thomson.xpm"


GtkWidget *widget_win;
GdkWindow *gwindow_win;
Display *display;
int screen;
int mit_shm_enabled;
Window screen_win;
Window window_win;
Atom atomDeleteScreen;


static int need_modifiers_reset = FALSE;


// static int installed_pointer;
// static Cursor null_cursor;


int keyval_to_teo_key (int keyval)
{
    switch (keyval)
    {
        case GDK_KEY_Escape          : return TEO_KEY_ESC;
        case GDK_KEY_1               : return TEO_KEY_1;
        case GDK_KEY_2               : return TEO_KEY_2;
        case GDK_KEY_3               : return TEO_KEY_3;
        case GDK_KEY_4               : return TEO_KEY_4;
        case GDK_KEY_5               : return TEO_KEY_5;
        case GDK_KEY_6               : return TEO_KEY_6;
        case GDK_KEY_7               : return TEO_KEY_7;
        case GDK_KEY_8               : return TEO_KEY_8;
        case GDK_KEY_9               : return TEO_KEY_9;
        case GDK_KEY_0               : return TEO_KEY_0;
        case GDK_KEY_parenright      : return TEO_KEY_MINUS;
        case GDK_KEY_equal           : return TEO_KEY_EQUALS;
        case GDK_KEY_BackSpace       : return TEO_KEY_BACKSPACE;
        case GDK_KEY_Tab             : return TEO_KEY_TAB;
        case GDK_KEY_A               : return TEO_KEY_Q;
        case GDK_KEY_Z               : return TEO_KEY_W;
        case GDK_KEY_E               : return TEO_KEY_E;
        case GDK_KEY_R               : return TEO_KEY_R;
        case GDK_KEY_T               : return TEO_KEY_T;
        case GDK_KEY_Y               : return TEO_KEY_Y;
        case GDK_KEY_U               : return TEO_KEY_U;
        case GDK_KEY_I               : return TEO_KEY_I;
        case GDK_KEY_O               : return TEO_KEY_O;
        case GDK_KEY_P               : return TEO_KEY_P;
        case GDK_KEY_dead_circumflex : return TEO_KEY_OPENBRACE;
        case GDK_KEY_dollar          : return TEO_KEY_CLOSEBRACE;
        case GDK_KEY_Return          : return TEO_KEY_ENTER;
        case GDK_KEY_Control_L       : return TEO_KEY_LCONTROL;
        case GDK_KEY_Q               : return TEO_KEY_A;
        case GDK_KEY_S               : return TEO_KEY_S;
        case GDK_KEY_D               : return TEO_KEY_D;
        case GDK_KEY_F               : return TEO_KEY_F;
        case GDK_KEY_G               : return TEO_KEY_G;
        case GDK_KEY_H               : return TEO_KEY_H;
        case GDK_KEY_J               : return TEO_KEY_J;
        case GDK_KEY_K               : return TEO_KEY_K;
        case GDK_KEY_L               : return TEO_KEY_L;
        case GDK_KEY_M               : return TEO_KEY_COLON;
        case GDK_KEY_percent         : return TEO_KEY_QUOTE;
        case GDK_KEY_twosuperior     : return TEO_KEY_TILDE;
        case GDK_KEY_Shift_L         : return TEO_KEY_LSHIFT;
        case GDK_KEY_asterisk        : return TEO_KEY_ASTERISK;
        case GDK_KEY_W               : return TEO_KEY_Z;
        case GDK_KEY_X               : return TEO_KEY_X;
        case GDK_KEY_C               : return TEO_KEY_C;
        case GDK_KEY_V               : return TEO_KEY_V;
        case GDK_KEY_B               : return TEO_KEY_B;
        case GDK_KEY_N               : return TEO_KEY_N;
        case GDK_KEY_comma           : return TEO_KEY_M;
        case GDK_KEY_semicolon       : return TEO_KEY_COMMA;
        case GDK_KEY_colon           : return TEO_KEY_STOP ;
        case GDK_KEY_exclam          : return TEO_KEY_SLASH;
        case GDK_KEY_Shift_R         : return TEO_KEY_RSHIFT;
        case GDK_KEY_KP_Multiply     : return TEO_KEY_ASTERISK;
        case GDK_KEY_Alt_L           : return TEO_KEY_ALT;
        case GDK_KEY_space           : return TEO_KEY_SPACE;
        case GDK_KEY_Caps_Lock       : return TEO_KEY_CAPSLOCK;
        case GDK_KEY_F1              : return TEO_KEY_F1;
        case GDK_KEY_F2              : return TEO_KEY_F2;
        case GDK_KEY_F3              : return TEO_KEY_F3;
        case GDK_KEY_F4              : return TEO_KEY_F4;
        case GDK_KEY_F5              : return TEO_KEY_F5;
        case GDK_KEY_F6              : return TEO_KEY_F6;
        case GDK_KEY_F7              : return TEO_KEY_F7;
        case GDK_KEY_F8              : return TEO_KEY_F8;
        case GDK_KEY_F9              : return TEO_KEY_F9;
        case GDK_KEY_F10             : return TEO_KEY_F10;
        case GDK_KEY_Num_Lock        : return TEO_KEY_NUMLOCK;
        case GDK_KEY_Scroll_Lock     : return TEO_KEY_SCRLOCK;
        case GDK_KEY_KP_7            : return TEO_KEY_7_PAD;
        case GDK_KEY_KP_8            : return TEO_KEY_8_PAD;
        case GDK_KEY_KP_9            : return TEO_KEY_9_PAD;
        case GDK_KEY_KP_Subtract     : return TEO_KEY_MINUS_PAD;
        case GDK_KEY_KP_4            : return TEO_KEY_4_PAD;
        case GDK_KEY_KP_5            : return TEO_KEY_5_PAD;
        case GDK_KEY_KP_6            : return TEO_KEY_6_PAD;
        case GDK_KEY_KP_Add          : return TEO_KEY_PLUS_PAD;
        case GDK_KEY_KP_1            : return TEO_KEY_1_PAD;
        case GDK_KEY_KP_2            : return TEO_KEY_2_PAD;
        case GDK_KEY_KP_3            : return TEO_KEY_3_PAD;
        case GDK_KEY_KP_0            : return TEO_KEY_0_PAD;
        case GDK_KEY_KP_Decimal      : return TEO_KEY_DEL_PAD;
        case GDK_KEY_less            : return TEO_KEY_BACKSLASH2;
        case GDK_KEY_F11             : return TEO_KEY_F11;
        case GDK_KEY_F12             : return TEO_KEY_F12;
        case GDK_KEY_KP_Enter        : return TEO_KEY_ENTER_PAD;
        case GDK_KEY_Control_R       : return TEO_KEY_RCONTROL;
        case GDK_KEY_KP_Divide       : return TEO_KEY_SLASH_PAD;
        case GDK_KEY_ISO_Level3_Shift: return TEO_KEY_ALTGR;
        case GDK_KEY_Home            : return TEO_KEY_HOME;
        case GDK_KEY_Up              : return TEO_KEY_UP;
        case GDK_KEY_Page_Up         : return TEO_KEY_PGUP;
        case GDK_KEY_Left            : return TEO_KEY_LEFT;
        case GDK_KEY_Right           : return TEO_KEY_RIGHT;
        case GDK_KEY_End             : return TEO_KEY_END;
        case GDK_KEY_Down            : return TEO_KEY_DOWN;
        case GDK_KEY_Page_Down       : return TEO_KEY_PGDN;
        case GDK_KEY_Insert          : return TEO_KEY_INSERT;
        case GDK_KEY_Delete          : return TEO_KEY_DEL;
    }
    return 0;
}



/* SetPointer:
 *  Sélectionne le pointeur actif.
 */
static void SetPointer(int pointer)
{
#if 0
    if (pointer==TO8_MOUSE)
    {
        XDefineCursor(display, screen_win, null_cursor);
        installed_pointer=TO8_MOUSE;
    }
    else if (pointer==TO8_LIGHTPEN)
        installed_pointer=TO8_LIGHTPEN;
#endif
    (void)pointer;
}



/* InitDisplay:
 *  Ouvre la connexion avec le serveur X et initialise le clavier.
 */
void InitDisplay(void)
{
    int ret1, ret2, ret3;

    /* Connexion au serveur X */
    display=gdk_x11_get_default_xdisplay();
    screen=DefaultScreen(display);

    /* Test de présence de l'extension MIT-SHM */
    mit_shm_enabled = XQueryExtension(display, "MIT-SHM", &ret1, &ret2, &ret3);
}


/* DestroyWindow:
 *  Elimine la fenêtre principale
 */
void DestroyWindow (void)
{
    XDestroyWindow(display, window_win);
}



static gboolean delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    teo.command = QUIT;

    return TRUE;
    (void)widget;
    (void)event;
    (void)user_data;
}


static gboolean key_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    int value = 0;
    int teo_key = 0;

    if (need_modifiers_reset)
    {
        if (event->key.state & GDK_SHIFT_MASK)   value |= TO8_SHIFT_FLAG;
        if (event->key.state & GDK_CONTROL_MASK) value |= TO8_CTRL_FLAG;
        if (event->key.state & GDK_MOD3_MASK)    value |= TO8_ALTGR_FLAG;
        if (event->key.state & GDK_MOD2_MASK)    value |= TO8_NUMLOCK_FLAG;
        if (event->key.state & GDK_LOCK_MASK)    value |= TO8_CAPSLOCK_FLAG;
        to8_InputReset ((1<<TO8_MAX_FLAG)-1, value);
        need_modifiers_reset = FALSE;
    }

    teo_key = keyval_to_teo_key (event->key.keyval);

    switch (teo_key)
    {
        case TEO_KEY_ESC : teo.command=CONTROL_PANEL; break;
        case TEO_KEY_F12 : teo.command=DEBUGGER; break;
        default :      to8_HandleKeyPress (teo_key, FALSE); break;
    }

    return TRUE;
    (void)widget;
    (void)user_data;
}



static gboolean key_release_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    to8_HandleKeyPress (keyval_to_teo_key (event->key.keyval), TRUE);

    return TRUE;
    (void)widget;
    (void)user_data;
}



static gboolean button_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    switch (event->button.button)
    {
        case 1 : to8_HandleMouseClick(1, FALSE); break;
        case 3 :
/*
                if (installed_pointer == TO8_MOUSE)
                    to8_HandleMouseClick (2, FALSE);
                else 
                {
                    pointer_on^=1;
                    XDefineCursor (display,screen_win, pointer_on ? None : null_cursor);
                }
                */
                break;
    }
    return TRUE;
    (void)widget;
    (void)user_data;
}



static gboolean button_release_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    switch (event->button.button)
    {
        case 1 : to8_HandleMouseClick (1, TRUE); break;
        case 3 : to8_HandleMouseClick (2, TRUE); break;
    }
    return TRUE;
    (void)widget;
    (void)user_data;
}



/* motion_notify_event:
 *  Gestion des mouvements de la souris.
 */
static gboolean motion_notify_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    to8_HandleMouseMotion ((int)event->button.x/2, (int)event->button.y/2);

    return TRUE;
    (void)widget;
    (void)user_data;
}



static gboolean focus_in_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    if (event->focus_change.in == TRUE)
    {
        to8_InputReset(0, 0);
        need_modifiers_reset = TRUE;
    }
    return TRUE;
    (void)widget;
    (void)user_data;
}



/* InitWindow:
 *  Met en place les propriétés et crée la fenêtre principale,
 *  calcule la position et crée la fenêtre de l'écran.
 */
void InitWindow(int argc, char *argv[], int x, int y, int user_flags)
{
    char *window_name=(is_fr?"Teo - l'Ã©mulateur TO8 (menu:ESC)":"Teo - thomson TO8 emulator (menu:ESC)");
    GdkPixbuf *pixbuf;
    GdkGeometry hints;
    GdkColor color;
    int event_mask = GDK_FOCUS_CHANGE_MASK
                   | GDK_KEY_RELEASE_MASK
                   | GDK_KEY_PRESS_MASK
                   | GDK_EXPOSURE_MASK
                   | GDK_BUTTON_RELEASE_MASK
                   | GDK_BUTTON_PRESS_MASK
                   | GDK_POINTER_MOTION_MASK;

    /* Crée la fenêtre GTK */
    widget_win=gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_add_events (GTK_WIDGET(widget_win), event_mask);
    g_signal_connect (G_OBJECT (widget_win), "delete-event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (G_OBJECT (widget_win), "key-press-event", G_CALLBACK (key_press_event), NULL);
    g_signal_connect (G_OBJECT (widget_win), "key-release-event", G_CALLBACK (key_release_event), NULL);
    g_signal_connect (G_OBJECT (widget_win), "button-press-event", G_CALLBACK (button_press_event), NULL);
    g_signal_connect (G_OBJECT (widget_win), "button-release-event", G_CALLBACK (button_release_event), NULL);
    g_signal_connect (G_OBJECT (widget_win), "motion-notify-event", G_CALLBACK (motion_notify_event), NULL);
    g_signal_connect (G_OBJECT (widget_win), "focus-in-event", G_CALLBACK (focus_in_event), NULL);
    hints.min_width = TO8_SCREEN_W*2;
    hints.max_width = TO8_SCREEN_W*2;
    hints.min_height = TO8_SCREEN_H*2;
    hints.max_height = TO8_SCREEN_H*2;
    gtk_window_set_geometry_hints (GTK_WINDOW(widget_win), widget_win, &hints,
                                   GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
    gtk_window_set_resizable (GTK_WINDOW(widget_win), FALSE);
    pixbuf=gdk_pixbuf_new_from_xpm_data ((const char **)thomson_xpm);
    gtk_window_set_icon (GTK_WINDOW(widget_win),pixbuf);
    gtk_window_set_default_icon(pixbuf);
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    gtk_widget_modify_bg (widget_win, GTK_STATE_NORMAL, &color);
    gtk_window_set_title (GTK_WINDOW(widget_win), window_name);

    gtk_widget_set_can_focus (widget_win, TRUE);

    gtk_widget_show_all (widget_win);

    to8_SetPointer=SetPointer;

    gtk_widget_realize (widget_win);
    gwindow_win = gtk_widget_get_window (widget_win);
    window_win = GDK_WINDOW_XID (gwindow_win);
    screen_win = window_win;

    /* Attend que la GtkWindow ait tout enregistré */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    printf("ok\n");
    (void)argc;
    (void)argv;
    (void)x;
    (void)y;
    (void)user_flags;
}

