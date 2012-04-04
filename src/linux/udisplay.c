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
 *               François Mouret 26/01/2010 08/2011
 *               Gille Fétis 07/2011
 *
 *  Module d'interface avec le serveur X.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <gtk/gtk.h>
   #include <gdk/gdkx.h>
   #include <X11/Xlib.h>
   #include <X11/Xutil.h>
   #include <X11/keysym.h>
#endif

#include "linux/gui.h"
#include "linux/display.h"
#include "linux/graphic.h"
#include "linux/main.h"
#include "to8keys.h"
#include "to8.h"
#include "thomson.xpm"


GtkWidget *widget_win;
GdkWindow *gscreen_win;
GdkWindow *gwindow_win;
Display *display;
int screen;
int mit_shm_enabled;
Window screen_win;
Window window_win;
Atom atomDeleteScreen;


static int installed_pointer;
static Cursor null_cursor;
static int x11_to_dos[256];

#define KB_SIZE  101
static struct {
    int keysym;
    int keycode;
} keyconv[KB_SIZE]={ 	{XK_Escape, 		KEY_ESC},
			{XK_1, 			KEY_1  },
			{XK_2,			KEY_2  },
			{XK_3,	 		KEY_3  },
			{XK_4,	 		KEY_4  },
			{XK_5,	 		KEY_5  },
			{XK_6,	 		KEY_6  },
			{XK_7,	 		KEY_7  },
			{XK_8,			KEY_8  },
			{XK_9,			KEY_9  },
			{XK_0,	 		KEY_0  },
			{XK_parenright,		KEY_MINUS },
			{XK_equal, 		KEY_EQUALS },
			{XK_BackSpace, 		KEY_BACKSPACE },
			{XK_Tab, 		KEY_TAB },
			{XK_A,	 		KEY_Q },
			{XK_Z,	 		KEY_W },
			{XK_E,			KEY_E },
			{XK_R,			KEY_R },
			{XK_T,	 		KEY_T },
			{XK_Y,	 		KEY_Y },
			{XK_U,	 		KEY_U },
			{XK_I,	 		KEY_I },
			{XK_O, 			KEY_O },
			{XK_P, 			KEY_P },
			{XK_dead_circumflex,    KEY_OPENBRACE },
			{XK_dollar, 		KEY_CLOSEBRACE },
			{XK_Return, 		KEY_ENTER },
			{XK_Control_L, 		KEY_LCONTROL },
			{XK_Q,	 		KEY_A },
			{XK_S,	 		KEY_S },
			{XK_D,	 		KEY_D },
			{XK_F,	 		KEY_F },
			{XK_G,	 		KEY_G },
			{XK_H,			KEY_H },
			{XK_J, 			KEY_J },
			{XK_K, 			KEY_K },
			{XK_L,			KEY_L },
			{XK_M, 			KEY_COLON },
			{XK_percent, 		KEY_QUOTE },
			{XK_twosuperior, 	KEY_TILDE },
			{XK_Shift_L, 		KEY_LSHIFT },
			{XK_asterisk, 		KEY_ASTERISK },
			{XK_W, 			KEY_Z },
			{XK_X, 			KEY_X },
			{XK_C, 			KEY_C },
			{XK_V, 			KEY_V },
			{XK_B, 			KEY_B },
			{XK_N, 			KEY_N },
			{XK_comma, 		KEY_M },
			{XK_semicolon, 		KEY_COMMA },
			{XK_colon, 		KEY_STOP },
			{XK_exclam, 		KEY_SLASH },
			{XK_Shift_R, 		KEY_RSHIFT },
			{XK_KP_Multiply, 	KEY_ASTERISK },
			{XK_Alt_L, 		KEY_ALT },
			{XK_space, 		KEY_SPACE },
			{XK_Caps_Lock, 		KEY_CAPSLOCK },
			{XK_F1, 		KEY_F1 },
			{XK_F2, 		KEY_F2 },
			{XK_F3, 		KEY_F3 },
			{XK_F4, 		KEY_F4 },
			{XK_F5, 		KEY_F5 },
			{XK_F6, 		KEY_F6 },
			{XK_F7, 		KEY_F7 },
			{XK_F8, 		KEY_F8 },
			{XK_F9, 		KEY_F9 },
			{XK_F10, 		KEY_F10 },
			{XK_Num_Lock, 		KEY_NUMLOCK },
			{XK_Scroll_Lock, 	KEY_SCRLOCK },
			{XK_KP_7, 		KEY_7_PAD },
			{XK_KP_8, 		KEY_8_PAD },
			{XK_KP_9, 		KEY_9_PAD },
			{XK_KP_Subtract, 	KEY_MINUS_PAD },
			{XK_KP_4, 		KEY_4_PAD },
			{XK_KP_5, 		KEY_5_PAD },
			{XK_KP_6, 		KEY_6_PAD },
			{XK_KP_Add, 		KEY_PLUS_PAD },
			{XK_KP_1, 		KEY_1_PAD },
			{XK_KP_2, 		KEY_2_PAD },
			{XK_KP_3, 		KEY_3_PAD },
			{XK_KP_0, 		KEY_0_PAD },
			{XK_KP_Decimal, 	KEY_DEL_PAD },
			{XK_less, 		KEY_BACKSLASH2 },
			{XK_F11, 		KEY_F11 },
			{XK_F12, 		KEY_F12 },
			{XK_KP_Enter, 		KEY_ENTER_PAD },
			{XK_Control_R, 		KEY_RCONTROL },
			{XK_KP_Divide, 		KEY_SLASH_PAD },
			{XK_Mode_switch, 	KEY_ALTGR }, /* KEY_ALTGR until Ubuntu 8.04 */
			{XK_ISO_Level3_Shift, 	0 },         /* to get KEY_ALTGR since Ubuntu 8.04 */
			{XK_Home, 		KEY_HOME },
			{XK_Up, 		KEY_UP },
			{XK_Page_Up, 		KEY_PGUP },
			{XK_Left, 		KEY_LEFT },
			{XK_Right, 		KEY_RIGHT },
			{XK_End, 		KEY_END },
			{XK_Down, 		KEY_DOWN },
			{XK_Page_Down, 		KEY_PGDN },
			{XK_Insert, 		KEY_INSERT },
			{XK_Delete, 		KEY_DEL } };



/* SetPointer:
 *  Sélectionne le pointeur actif.
 */
static void SetPointer(int pointer)
{
    if (pointer==TO8_MOUSE)
    {
	XDefineCursor(display, screen_win, null_cursor);
	installed_pointer=TO8_MOUSE;
    }
    else if (pointer==TO8_LIGHTPEN)
	installed_pointer=TO8_LIGHTPEN;
}



/* InitDisplay:
 *  Ouvre la connexion avec le serveur X et initialise le clavier.
 */
void InitDisplay(void)
{
    register int i;
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


/* DestroyWindow:
 *  Elimine la fenêtre principale
 */
void DestroyWindow (void)
{
    gtk_widget_destroy (widget_win);
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

    printf((is_fr?"CrÃ©ation de la fenÃªtre principale...":"Creates main window..."));

    widget_win=gtk_window_new (GTK_WINDOW_TOPLEVEL);
    hints.min_width = TO8_SCREEN_W*2;
    hints.max_width = TO8_SCREEN_W*2;
    hints.min_height = TO8_SCREEN_H*2;
    hints.max_height = TO8_SCREEN_H*2;
    gtk_window_set_geometry_hints (GTK_WINDOW(widget_win), widget_win, &hints, GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
    gtk_window_set_resizable (GTK_WINDOW(widget_win), FALSE);
    pixbuf=gdk_pixbuf_new_from_xpm_data ((const char **)thomson_xpm);
    gtk_window_set_default_icon(pixbuf);
    gtk_window_set_icon (GTK_WINDOW(widget_win),pixbuf);
    gtk_window_set_title (GTK_WINDOW(widget_win), window_name);
    gtk_window_move (GTK_WINDOW(widget_win), x, y);
    gtk_widget_realize (widget_win);
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    gtk_widget_modify_bg (widget_win, GTK_STATE_NORMAL, &color);
    
    /* Attend que la GtkWindow ait tout enregistré */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    gtk_widget_show (widget_win);
    gscreen_win=GTK_WIDGET(widget_win)->window;
    screen_win=GDK_WINDOW_XID(gscreen_win);

    /* Connecte le callback pour le gadget de fermeture */
    atomDeleteScreen = XInternAtom(display, "WM_DELETE_WINDOW", FALSE);
    XSetWMProtocols(display, screen_win, &atomDeleteScreen, 1);

    window_win=screen_win;
    argc=argc;
    argv=argv;
    user_flags=user_flags;

    to8_SetPointer=SetPointer;

    printf("ok\n");
}


/* HandleEvents:
 *  Lit et traite les évènements envoyés par le serveur X.
 */
void HandleEvents(void)
{
    XEvent ev;
    static int need_modifiers_reset;
           int key;

    while (XPending(display))
    {
        XNextEvent(display, &ev);

        switch (ev.type)
        {
            case ClientMessage:  /* screen_win */
                if (atomDeleteScreen == (Atom)ev.xclient.data.l[0])
                    if (ask_box (is_fr?"Voulez-vous vraiment quitter l'Ã©mulateur ?"
                                                :"Do you really want to quit the emulator ?", widget_win) == TRUE)
                        teo.command = QUIT;
                break;

	    case FocusIn:  /* screen_win */
                to8_InputReset(0, 0);
		need_modifiers_reset=True;
		break;

	    case KeyRelease:  /* screen_win */
                key = x11_to_dos[ev.xkey.keycode];
                if (!key)
                    /* Cas spécial du ALTGR à partir de Ubuntu 8.04 */
                    if (XKeycodeToKeysym(display,ev.xkey.keycode,0) == (KeySym)XK_ISO_Level3_Shift)
                        key=KEY_ALTGR;
                to8_HandleKeyPress(key, True);
                break;
              
	    case KeyPress:  /* screen_win */
		if (need_modifiers_reset)
		{
		    int value = 0;

		    if (ev.xkey.state&ShiftMask)
			value |= TO8_SHIFT_FLAG;

		    if (ev.xkey.state&ControlMask)
			value |= TO8_CTRL_FLAG;

		    if (ev.xkey.state&(1<<13))  /* should be Mod3Mask */
			value |= TO8_ALTGR_FLAG;

		    if (ev.xkey.state&Mod2Mask)
			value |= TO8_NUMLOCK_FLAG;

		    if (ev.xkey.state&LockMask)
			value |= TO8_CAPSLOCK_FLAG;

		    to8_InputReset((1<<TO8_MAX_FLAG)-1, value); 
		    need_modifiers_reset=False;
		}

                key=x11_to_dos[ev.xkey.keycode];

                if (!key)
                    /* Cas spécial du ALTGR à partir de Ubuntu 8.04 */
                    if (XKeycodeToKeysym(display,ev.xkey.keycode,0) == (KeySym)XK_ISO_Level3_Shift)
                        key=KEY_ALTGR;

                if (key==KEY_ESC)
                    teo.command=CONTROL_PANEL;
                else if (key==KEY_F10)
                    teo.command=DEBUGGER;
                else   
                    to8_HandleKeyPress(key, False);

                break;

	    case Expose:  /* screen_win */
		RetraceScreen(ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height);
#ifdef DEBUG
		fprintf(stderr, "Expose event: x=%d, y=%d, w=%d, h=%d\n", ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height);
#endif     
                break;
                 
	    case ButtonRelease:  /* window_win */
                if (ev.xbutton.button==Button1)
                    to8_HandleMouseClick(1, True);
                else if (ev.xbutton.button==Button3)
                    to8_HandleMouseClick(2, True);
                break;
                                             
            case ButtonPress:  /* window_win */
                if (ev.xbutton.button==Button1)
                    to8_HandleMouseClick(1, False);
                else if (ev.xbutton.button==Button3)
		{
		    static int pointer_on;

		    if (installed_pointer==TO8_MOUSE)
			to8_HandleMouseClick(2, False);
		    else 
		    {
			pointer_on^=1;
			XDefineCursor(display,screen_win, pointer_on ? None : null_cursor);
		    }
		}
                break;

            case MotionNotify:  /* window_win */
                to8_HandleMouseMotion((ev.xbutton.x/2<TO8_BORDER_W) ? 0 : ev.xbutton.x/2-TO8_BORDER_W,
                                      (ev.xbutton.y/2<TO8_BORDER_H) ? 0 : ev.xbutton.y/2-TO8_BORDER_H);
                break;

        } /* end of switch(ev.type) */
    }  /* end of while XPending */
}

