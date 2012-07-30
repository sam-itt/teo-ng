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
 *  Version    : 1.8.2
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


GtkWidget *wMain;
GdkWindow *gwindow_win;
Display *display;
int screen;
int mit_shm_enabled;
Window screen_win;
Window window_win;

static int need_modifiers_reset = TRUE;

static int installed_pointer = TO8_MOUSE;

static int x11_to_dos[256];

#define KB_SIZE  101
static struct {
    int keysym;
    int keycode;
} keyconv[KB_SIZE]={
    { XK_Escape          , TEO_KEY_ESC        },
    { XK_1               , TEO_KEY_1          },
    { XK_2               , TEO_KEY_2          },
    { XK_3               , TEO_KEY_3          },
    { XK_4               , TEO_KEY_4          },
    { XK_5               , TEO_KEY_5          },
    { XK_6               , TEO_KEY_6          },
    { XK_7               , TEO_KEY_7          },
    { XK_8               , TEO_KEY_8          },
    { XK_9               , TEO_KEY_9          },
    { XK_0               , TEO_KEY_0          },
    { XK_parenright      , TEO_KEY_MINUS      },
    { XK_equal           , TEO_KEY_EQUALS     },
    { XK_BackSpace       , TEO_KEY_BACKSPACE  },
    { XK_Tab             , TEO_KEY_TAB        },
    { XK_A               , TEO_KEY_Q          },
    { XK_Z               , TEO_KEY_W          },
    { XK_E               , TEO_KEY_E          },
    { XK_R               , TEO_KEY_R          },
    { XK_T               , TEO_KEY_T          },
    { XK_Y               , TEO_KEY_Y          },
    { XK_U               , TEO_KEY_U          },
    { XK_I               , TEO_KEY_I          },
    { XK_O               , TEO_KEY_O          },
    { XK_P               , TEO_KEY_P          },
    { XK_dead_circumflex , TEO_KEY_OPENBRACE  },
    { XK_dollar          , TEO_KEY_CLOSEBRACE },
    { XK_Return          , TEO_KEY_ENTER      },
    { XK_Control_L       , TEO_KEY_LCONTROL   },
    { XK_Q               , TEO_KEY_A          },
    { XK_S               , TEO_KEY_S          },
    { XK_D               , TEO_KEY_D          },
    { XK_F               , TEO_KEY_F          },
    { XK_G               , TEO_KEY_G          },
    { XK_H               , TEO_KEY_H          },
    { XK_J               , TEO_KEY_J          },
    { XK_K               , TEO_KEY_K          },
    { XK_L               , TEO_KEY_L          },
    { XK_M               , TEO_KEY_COLON      },
    { XK_percent         , TEO_KEY_QUOTE      },
    { XK_twosuperior     , TEO_KEY_TILDE      },
    { XK_Shift_L         , TEO_KEY_LSHIFT     },
    { XK_asterisk        , TEO_KEY_ASTERISK   },
    { XK_W               , TEO_KEY_Z          },
    { XK_X               , TEO_KEY_X          },
    { XK_C               , TEO_KEY_C          },
    { XK_V               , TEO_KEY_V          },
    { XK_B               , TEO_KEY_B          },
    { XK_N               , TEO_KEY_N          },
    { XK_comma           , TEO_KEY_M          },
    { XK_semicolon       , TEO_KEY_COMMA      },
    { XK_colon           , TEO_KEY_STOP       },
    { XK_exclam          , TEO_KEY_SLASH      },
    { XK_Shift_R         , TEO_KEY_RSHIFT     },
    { XK_KP_Multiply     , TEO_KEY_ASTERISK   },
    { XK_Alt_L           , TEO_KEY_ALT        },
    { XK_space           , TEO_KEY_SPACE      },
    { XK_Caps_Lock       , TEO_KEY_CAPSLOCK   },
    { XK_F1              , TEO_KEY_F1         },
    { XK_F2              , TEO_KEY_F2         },
    { XK_F3              , TEO_KEY_F3         },
    { XK_F4              , TEO_KEY_F4         },
    { XK_F5              , TEO_KEY_F5         },
    { XK_F6              , TEO_KEY_F6         },
    { XK_F7              , TEO_KEY_F7         },
    { XK_F8              , TEO_KEY_F8         },
    { XK_F9              , TEO_KEY_F9         },
    { XK_F10             , TEO_KEY_F10        },
    { XK_Num_Lock        , TEO_KEY_NUMLOCK    },
    { XK_Scroll_Lock     , TEO_KEY_SCRLOCK    },
    { XK_KP_7            , TEO_KEY_7_PAD      },
    { XK_KP_8            , TEO_KEY_8_PAD      },
    { XK_KP_9            , TEO_KEY_9_PAD      },
    { XK_KP_Subtract     , TEO_KEY_MINUS_PAD  },
    { XK_KP_4            , TEO_KEY_4_PAD      },
    { XK_KP_5            , TEO_KEY_5_PAD      },
    { XK_KP_6            , TEO_KEY_6_PAD      },
    { XK_KP_Add          , TEO_KEY_PLUS_PAD   },
    { XK_KP_1            , TEO_KEY_1_PAD      },
    { XK_KP_2            , TEO_KEY_2_PAD      },
    { XK_KP_3            , TEO_KEY_3_PAD      },
    { XK_KP_0            , TEO_KEY_0_PAD      },
    { XK_KP_Decimal      , TEO_KEY_DEL_PAD    },
    { XK_less            , TEO_KEY_BACKSLASH2 },
    { XK_F11             , TEO_KEY_F11        },
    { XK_F12             , TEO_KEY_F12        },
    { XK_KP_Enter        , TEO_KEY_ENTER_PAD  },
    { XK_Control_R       , TEO_KEY_RCONTROL   },
    { XK_KP_Divide       , TEO_KEY_SLASH_PAD  },
    { XK_ISO_Level3_Shift, TEO_KEY_ALTGR      },
    { XK_Home            , TEO_KEY_HOME       },
    { XK_Up              , TEO_KEY_UP         },
    { XK_Page_Up         , TEO_KEY_PGUP       },
    { XK_Left            , TEO_KEY_LEFT       },
    { XK_Right           , TEO_KEY_RIGHT      },
    { XK_End             , TEO_KEY_END        },
    { XK_Down            , TEO_KEY_DOWN       },
    { XK_Page_Down       , TEO_KEY_PGDN       },
    { XK_Insert          , TEO_KEY_INSERT     },
    { XK_Delete          , TEO_KEY_DEL        }
};



/* SetPointer:
 *  Sélectionne le pointeur actif.
 */
static void SetPointer(int pointer)
{
    switch (pointer)
    {
        case TO8_MOUSE :
            gdk_window_set_cursor (gwindow_win, NULL);
            installed_pointer=TO8_MOUSE;
            break;

        case TO8_LIGHTPEN :
            gdk_window_set_cursor (gwindow_win, gdk_cursor_new (GDK_PENCIL));
            installed_pointer=TO8_LIGHTPEN;
            break;
    }
}


/* InitDisplay:
 *  Ouvre la connexion avec le serveur X et initialise le clavier.
 */
void InitDisplay(void)
{
    int i;
    int ret1, ret2, ret3;

    /* Connexion au serveur X */
    display=gdk_x11_get_default_xdisplay();
    screen=DefaultScreen(display);

    /* Calcul de la table de conversion des keycodes */
    for (i=0; i<KB_SIZE; i++)
        x11_to_dos[XKeysymToKeycode(display,keyconv[i].keysym)]=keyconv[i].keycode;

    /* Test de présence de l'extension MIT-SHM */
    mit_shm_enabled = XQueryExtension(display, "MIT-SHM", &ret1, &ret2, &ret3);
}


/* button_release_event:
 *  Gestion des touches enfoncées.
 */
static gboolean
delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    teo.command = QUIT;

    return FALSE;
    (void)widget;
    (void)event;
    (void)user_data;
}


/* key_press_event:
 *  Gestion des touches enfoncées.
 */
static gboolean
key_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
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

    teo_key = x11_to_dos[event->key.hardware_keycode];
    if (teo_key == 0)
        if (event->key.keyval == GDK_KEY_ISO_Level3_Shift)
            teo_key = TEO_KEY_ALTGR;

    switch (teo_key)
    {
        case TEO_KEY_ESC : teo.command=CONTROL_PANEL; break;
        case TEO_KEY_F12 : teo.command=DEBUGGER; break;
        default          : to8_HandleKeyPress (teo_key, FALSE); break;
    }
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* key_release_event:
 *  Gestion des touches relachées.
 */
static gboolean
key_release_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    int teo_key = x11_to_dos[event->key.hardware_keycode];

    if (teo_key == 0)
        if (event->key.keyval == GDK_KEY_ISO_Level3_Shift)
            teo_key = TEO_KEY_ALTGR;

    to8_HandleKeyPress (teo_key, TRUE);
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* button_press_event:
 *  Gestion des boutons de souris enfoncés.
 */
static gboolean
button_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    switch (event->button.button)
    {
        case 1 : to8_HandleMouseClick(1, FALSE); break;
        case 3 : if (installed_pointer == TO8_MOUSE)
                     to8_HandleMouseClick (2, FALSE);
                 break;
    }
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* button_release_event:
 *  Gestion des boutons de souris relachés.
 */
static gboolean
button_release_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    switch (event->button.button)
    {
        case 1 : to8_HandleMouseClick (1, TRUE); break;
        case 3 : to8_HandleMouseClick (2, TRUE); break;
    }
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* motion_notify_event:
 *  Gestion des mouvements de la souris.
 */
static gboolean
motion_notify_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    if (((int)event->button.x > (TO8_BORDER_W*2))
     && ((int)event->button.y > (TO8_BORDER_H*2)))
        to8_HandleMouseMotion ((int)event->button.x/2-TO8_BORDER_W,
                               (int)event->button.y/2-TO8_BORDER_H);
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* focus_in_event:
 *  Gestion des activations de fenêtres.
 */
static gboolean
focus_in_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    if (event->focus_change.in == TRUE)
    {
        to8_InputReset(0, 0);
        need_modifiers_reset = TRUE;
    }
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* window_state_event:
 *  Gestion du retraçage de l'écran.
 */
static gboolean 
window_state_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    if ((event->window_state.changed_mask & GDK_WINDOW_STATE_ICONIFIED) != 0)
        if ((event->window_state.new_window_state & GDK_WINDOW_STATE_ICONIFIED) == 0)
            RetraceScreen(0, 0, TO8_SCREEN_W*2, TO8_SCREEN_H*2);

    return FALSE;
    (void)widget;
    (void)user_data;
}


/* InitWindow:
 *  Met en place les propriétés et crée la fenêtre principale,
 *  calcule la position et crée la fenêtre de l'écran.
 */
void InitWindow(int argc, char *argv[], int x, int y, int user_flags)
{
    GdkPixbuf *pixbuf;
    GdkGeometry hints;
    GdkRGBA rgba;
    
    wMain = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_resizable (GTK_WINDOW(wMain), FALSE);
    gtk_window_set_title (GTK_WINDOW(wMain),
                          is_fr?"Teo - l'Ã©mulateur TO8 (menu:ESC)"
                               :"Teo - thomson TO8 emulator (menu:ESC)");

    gtk_widget_add_events (wMain,
                     GDK_FOCUS_CHANGE_MASK
                   | GDK_KEY_RELEASE_MASK
                   | GDK_KEY_PRESS_MASK
                   | GDK_STRUCTURE_MASK
                   | GDK_BUTTON_RELEASE_MASK
                   | GDK_BUTTON_PRESS_MASK
                   | GDK_POINTER_MOTION_MASK
                   | GDK_POINTER_MOTION_HINT_MASK);

    g_signal_connect (G_OBJECT (wMain), "delete-event",
                      G_CALLBACK (delete_event), NULL);
    g_signal_connect (G_OBJECT (wMain), "key-press-event",
                      G_CALLBACK (key_press_event), NULL);
    g_signal_connect (G_OBJECT (wMain), "key-release-event",
                      G_CALLBACK (key_release_event), NULL);
    g_signal_connect (G_OBJECT (wMain), "button-press-event",
                      G_CALLBACK (button_press_event), NULL);
    g_signal_connect (G_OBJECT (wMain), "button-release-event",
                      G_CALLBACK (button_release_event), NULL);
    g_signal_connect (G_OBJECT (wMain), "motion-notify-event",
                      G_CALLBACK (motion_notify_event), NULL);
    g_signal_connect (G_OBJECT (wMain), "focus-in-event",
                      G_CALLBACK (focus_in_event), NULL);
    g_signal_connect (G_OBJECT (wMain), "window-state-event",
                      G_CALLBACK (window_state_event), NULL);

    /* Set window size */
    hints.min_width = TO8_SCREEN_W*2;
    hints.max_width = TO8_SCREEN_W*2;
    hints.min_height = TO8_SCREEN_H*2;
    hints.max_height = TO8_SCREEN_H*2;
    gtk_window_set_geometry_hints (GTK_WINDOW(wMain), wMain, &hints,
                                   GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);

    /* Set program icon */
    pixbuf=gdk_pixbuf_new_from_xpm_data ((const char **)thomson_xpm);
    gtk_window_set_icon (GTK_WINDOW(wMain),pixbuf);
    gtk_window_set_default_icon(pixbuf);

    /* Set black background */
    rgba.red   = 0;
    rgba.green = 0;
    rgba.blue  = 0;
    rgba.alpha = 1;
    gtk_widget_override_background_color (wMain, GTK_STATE_NORMAL, &rgba);

    gtk_widget_set_double_buffered (wMain, FALSE);  /* only one buffer for drawing */
    gtk_widget_set_app_paintable (wMain, TRUE);
    gtk_widget_set_can_focus (wMain, TRUE);

    gtk_widget_show_all (wMain);

    gwindow_win = gtk_widget_get_window (wMain);
    window_win = GDK_WINDOW_XID (gwindow_win);
    screen_win = window_win;

    to8_SetPointer=SetPointer;

    printf("ok\n");
    
    (void)argc;
    (void)argv;
    (void)x;
    (void)y;
    (void)user_flags;
}

