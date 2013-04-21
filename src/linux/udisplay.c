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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/udisplay.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 24/11/2003
 *               François Mouret 26/01/2010 08/2011 02/06/2012 28/12/2012
 *               Gilles Fétis 07/2011
 *
 *  Module d'interface avec le serveur X.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <gtk/gtk.h>
   #include <gtk/gtkx.h>
   #include <gdk/gdkx.h>
   #include <X11/Xlib.h>
   #include <X11/XKBlib.h>
   #include <X11/Xutil.h>
   #include <X11/keysym.h>
#endif

#include "defs.h"
#include "teo.h"
#include "thomson.xpm"
#include "to8keys.h"
#include "media/keyboard.h"
#include "media/mouse.h"
#include "media/disk/controlr.h"
#include "media/disk.h"
#include "linux/gui.h"
#include "linux/display.h"
#include "linux/graphic.h"


GtkWidget *wMain;
GdkWindow *gwindow_win;
Display *display;
int screen;
int mit_shm_enabled;
Window screen_win;
Window window_win;

static int installed_pointer = TEO_STATUS_MOUSE;

struct KEY_TABLE {
    int keyflag;
    int keycode;
};

static struct KEY_TABLE key00[256];
static struct KEY_TABLE keyFF[256];
static struct KEY_TABLE keyFE[256];

static struct {
    int keysym;
    int keyflag;
    int keycode;
} keyconv[]={
    /* thomson function keys */
    { GDK_KEY_F1              , TEO_KEY_F_NONE , TEO_KEY_F1         },
    { GDK_KEY_F2              , TEO_KEY_F_NONE , TEO_KEY_F2         },
    { GDK_KEY_F3              , TEO_KEY_F_NONE , TEO_KEY_F3         },
    { GDK_KEY_F4              , TEO_KEY_F_NONE , TEO_KEY_F4         },
    { GDK_KEY_F5              , TEO_KEY_F_NONE , TEO_KEY_F5         },
    { GDK_KEY_F6              , TEO_KEY_F_NONE , TEO_KEY_F6         },
    { GDK_KEY_F7              , TEO_KEY_F_NONE , TEO_KEY_F7         },
    { GDK_KEY_F8              , TEO_KEY_F_NONE , TEO_KEY_F8         },
    { GDK_KEY_F9              , TEO_KEY_F_NONE , TEO_KEY_F9         },
    { GDK_KEY_F10             , TEO_KEY_F_NONE , TEO_KEY_F10        },
    /* first row of thomson keyboard */
    { GDK_KEY_numbersign      , TEO_KEY_F_ALTGR, TEO_KEY_3          }, /* # */
    { GDK_KEY_at              , TEO_KEY_F_ALTGR, TEO_KEY_0          }, /* @ */
    { GDK_KEY_asterisk        , TEO_KEY_F_NONE , TEO_KEY_ASTERISK   }, /* * */
    { GDK_KEY_1               , TEO_KEY_F_SHIFT, TEO_KEY_1          }, /* 1 */
    { GDK_KEY_eacute          , TEO_KEY_F_NONE , TEO_KEY_2          }, /* é */
    { GDK_KEY_Eacute          , TEO_KEY_F_NONE , TEO_KEY_2          }, /* é */
    { GDK_KEY_2               , TEO_KEY_F_SHIFT, TEO_KEY_2          }, /* 2 */
    { GDK_KEY_quotedbl        , TEO_KEY_F_NONE , TEO_KEY_3          }, /* " */
    { GDK_KEY_3               , TEO_KEY_F_SHIFT, TEO_KEY_3          }, /* 3 */
    { GDK_KEY_apostrophe      , TEO_KEY_F_NONE , TEO_KEY_4          }, /* ' */
    { GDK_KEY_4               , TEO_KEY_F_SHIFT, TEO_KEY_4          }, /* 4 */
    { GDK_KEY_parenleft       , TEO_KEY_F_NONE , TEO_KEY_5          }, /* ( */
    { GDK_KEY_5               , TEO_KEY_F_SHIFT, TEO_KEY_5          }, /* 5 */
    { GDK_KEY_underscore      , TEO_KEY_F_NONE , TEO_KEY_8          }, /* _ */
    { GDK_KEY_6               , TEO_KEY_F_SHIFT, TEO_KEY_6          }, /* 6 */
    { GDK_KEY_egrave          , TEO_KEY_F_NONE , TEO_KEY_7          }, /* è */
    { GDK_KEY_Egrave          , TEO_KEY_F_NONE , TEO_KEY_7          }, /* è */
    { GDK_KEY_7               , TEO_KEY_F_SHIFT, TEO_KEY_7          }, /* 7 */
    { GDK_KEY_exclam          , TEO_KEY_F_NONE , TEO_KEY_SLASH      }, /* ! */
    { GDK_KEY_8               , TEO_KEY_F_SHIFT, TEO_KEY_8          }, /* 8 */
    { GDK_KEY_ccedilla        , TEO_KEY_F_NONE , TEO_KEY_9          }, /* ç */
    { GDK_KEY_Ccedilla        , TEO_KEY_F_NONE , TEO_KEY_9          }, /* ç */
    { GDK_KEY_9               , TEO_KEY_F_SHIFT, TEO_KEY_9          }, /* 9 */
    { GDK_KEY_agrave          , TEO_KEY_F_NONE , TEO_KEY_0          }, /* à */
    { GDK_KEY_Agrave          , TEO_KEY_F_NONE , TEO_KEY_0          }, /* à */
    { GDK_KEY_0               , TEO_KEY_F_SHIFT, TEO_KEY_0          }, /* 0 */
    { GDK_KEY_parenright      , TEO_KEY_F_NONE , TEO_KEY_MINUS      }, /* ) */
    { GDK_KEY_degree          , TEO_KEY_F_SHIFT, TEO_KEY_MINUS      }, /* ° */
    { GDK_KEY_minus           , TEO_KEY_F_NONE , TEO_KEY_6          }, /* - */
    { GDK_KEY_backslash       , TEO_KEY_F_ALTGR, TEO_KEY_8          }, /* \ */
    { GDK_KEY_equal           , TEO_KEY_F_NONE , TEO_KEY_EQUALS     }, /* = */
    { GDK_KEY_plus            , TEO_KEY_F_SHIFT, TEO_KEY_EQUALS     }, /* + */
    { GDK_KEY_BackSpace       , TEO_KEY_F_NONE , TEO_KEY_BACKSPACE  },
    /* second row of thomson keyboard */
    { GDK_KEY_Tab             , TEO_KEY_F_NONE , TEO_KEY_TAB        },
    { GDK_KEY_a               , TEO_KEY_F_NONE , TEO_KEY_Q          }, /* a */
    { GDK_KEY_A               , TEO_KEY_F_SHIFT, TEO_KEY_Q          }, /* A */
    { GDK_KEY_z               , TEO_KEY_F_NONE , TEO_KEY_W          }, /* z */
    { GDK_KEY_Z               , TEO_KEY_F_SHIFT, TEO_KEY_W          }, /* Z */
    { GDK_KEY_e               , TEO_KEY_F_NONE , TEO_KEY_E          }, /* e */
    { GDK_KEY_E               , TEO_KEY_F_SHIFT, TEO_KEY_E          }, /* E */
    { GDK_KEY_r               , TEO_KEY_F_NONE , TEO_KEY_R          }, /* r */
    { GDK_KEY_R               , TEO_KEY_F_SHIFT, TEO_KEY_R          }, /* R */
    { GDK_KEY_t               , TEO_KEY_F_NONE , TEO_KEY_T          }, /* t */
    { GDK_KEY_T               , TEO_KEY_F_SHIFT, TEO_KEY_T          }, /* T */
    { GDK_KEY_y               , TEO_KEY_F_NONE , TEO_KEY_Y          }, /* y */
    { GDK_KEY_Y               , TEO_KEY_F_SHIFT, TEO_KEY_Y          }, /* Y */
    { GDK_KEY_u               , TEO_KEY_F_NONE , TEO_KEY_U          }, /* u */
    { GDK_KEY_U               , TEO_KEY_F_SHIFT, TEO_KEY_U          }, /* U */
    { GDK_KEY_i               , TEO_KEY_F_NONE , TEO_KEY_I          }, /* i */
    { GDK_KEY_I               , TEO_KEY_F_SHIFT, TEO_KEY_I          }, /* I */
    { GDK_KEY_o               , TEO_KEY_F_NONE , TEO_KEY_O          }, /* o */
    { GDK_KEY_O               , TEO_KEY_F_SHIFT, TEO_KEY_O          }, /* O */
    { GDK_KEY_p               , TEO_KEY_F_NONE , TEO_KEY_P          }, /* p */
    { GDK_KEY_P               , TEO_KEY_F_SHIFT, TEO_KEY_P          }, /* P */
    { GDK_KEY_asciicircum     , TEO_KEY_F_NONE , TEO_KEY_OPENBRACE  }, /* ^ */
    { GDK_KEY_dead_circumflex , TEO_KEY_F_NONE , TEO_KEY_OPENBRACE  }, /* ^ */
    { GDK_KEY_diaeresis       , TEO_KEY_F_SHIFT, TEO_KEY_OPENBRACE  }, /* " */
    { GDK_KEY_dead_diaeresis  , TEO_KEY_F_SHIFT, TEO_KEY_OPENBRACE  }, /* " */
    { GDK_KEY_ampersand       , TEO_KEY_F_NONE , TEO_KEY_1          }, /* & */
    { GDK_KEY_dollar          , TEO_KEY_F_NONE , TEO_KEY_CLOSEBRACE }, /* $ */
    { GDK_KEY_Return          , TEO_KEY_F_NONE , TEO_KEY_ENTER      },
    /* third row of thomson keyboard */
    { GDK_KEY_Control_L       , TEO_KEY_F_NONE , TEO_KEY_LCONTROL   },
    { GDK_KEY_braceleft       , TEO_KEY_F_ALTGR, TEO_KEY_4          }, /* } */
    { GDK_KEY_bracketleft     , TEO_KEY_F_ALTGR, TEO_KEY_5          }, /* ] */
    { GDK_KEY_q               , TEO_KEY_F_NONE , TEO_KEY_A          }, /* q */
    { GDK_KEY_Q               , TEO_KEY_F_SHIFT, TEO_KEY_A          }, /* Q */
    { GDK_KEY_s               , TEO_KEY_F_NONE , TEO_KEY_S          }, /* s */
    { GDK_KEY_S               , TEO_KEY_F_SHIFT, TEO_KEY_S          }, /* S */
    { GDK_KEY_d               , TEO_KEY_F_NONE , TEO_KEY_D          }, /* d */
    { GDK_KEY_D               , TEO_KEY_F_SHIFT, TEO_KEY_D          }, /* D */
    { GDK_KEY_f               , TEO_KEY_F_NONE , TEO_KEY_F          }, /* f */
    { GDK_KEY_F               , TEO_KEY_F_SHIFT, TEO_KEY_F          }, /* F */
    { GDK_KEY_g               , TEO_KEY_F_NONE , TEO_KEY_G          }, /* g */
    { GDK_KEY_G               , TEO_KEY_F_SHIFT, TEO_KEY_G          }, /* G */
    { GDK_KEY_h               , TEO_KEY_F_NONE , TEO_KEY_H          }, /* h */
    { GDK_KEY_H               , TEO_KEY_F_SHIFT, TEO_KEY_H          }, /* H */
    { GDK_KEY_j               , TEO_KEY_F_NONE , TEO_KEY_J          }, /* j */
    { GDK_KEY_J               , TEO_KEY_F_SHIFT, TEO_KEY_J          }, /* J */
    { GDK_KEY_k               , TEO_KEY_F_NONE , TEO_KEY_K          }, /* k */
    { GDK_KEY_K               , TEO_KEY_F_SHIFT, TEO_KEY_K          }, /* K */
    { GDK_KEY_l               , TEO_KEY_F_NONE , TEO_KEY_L          }, /* l */
    { GDK_KEY_L               , TEO_KEY_F_SHIFT, TEO_KEY_L          }, /* L */
    { GDK_KEY_m               , TEO_KEY_F_NONE , TEO_KEY_COLON      }, /* m */
    { GDK_KEY_M               , TEO_KEY_F_SHIFT, TEO_KEY_COLON      }, /* M */
    { GDK_KEY_ugrave          , TEO_KEY_F_NONE , TEO_KEY_QUOTE      }, /* ù */
    { GDK_KEY_percent         , TEO_KEY_F_SHIFT, TEO_KEY_QUOTE      }, /* % */
    { GDK_KEY_braceright      , TEO_KEY_F_ALTGR, TEO_KEY_EQUALS     }, /* } */
    { GDK_KEY_bracketright    , TEO_KEY_F_ALTGR, TEO_KEY_MINUS      }, /* ] */
    /* fourth row of thomson keyboard */
    { GDK_KEY_Caps_Lock       , TEO_KEY_F_NONE , TEO_KEY_CAPSLOCK   },
    { GDK_KEY_Shift_L         , TEO_KEY_F_NONE , TEO_KEY_LSHIFT     },
    { GDK_KEY_w               , TEO_KEY_F_NONE , TEO_KEY_Z          }, /* w */
    { GDK_KEY_W               , TEO_KEY_F_SHIFT, TEO_KEY_Z          }, /* W */
    { GDK_KEY_x               , TEO_KEY_F_NONE , TEO_KEY_X          }, /* x */
    { GDK_KEY_X               , TEO_KEY_F_SHIFT, TEO_KEY_X          }, /* X */
    { GDK_KEY_c               , TEO_KEY_F_NONE , TEO_KEY_C          }, /* c */
    { GDK_KEY_C               , TEO_KEY_F_SHIFT, TEO_KEY_C          }, /* C */
    { GDK_KEY_v               , TEO_KEY_F_NONE , TEO_KEY_V          }, /* v */
    { GDK_KEY_V               , TEO_KEY_F_SHIFT, TEO_KEY_V          }, /* V */
    { GDK_KEY_b               , TEO_KEY_F_NONE , TEO_KEY_B          }, /* b */
    { GDK_KEY_B               , TEO_KEY_F_SHIFT, TEO_KEY_B          }, /* B */
    { GDK_KEY_n               , TEO_KEY_F_NONE , TEO_KEY_N          }, /* n */
    { GDK_KEY_N               , TEO_KEY_F_SHIFT, TEO_KEY_N          }, /* N */
    { GDK_KEY_comma           , TEO_KEY_F_NONE , TEO_KEY_M          }, /* , */
    { GDK_KEY_question        , TEO_KEY_F_SHIFT, TEO_KEY_M          }, /* ? */
    { GDK_KEY_semicolon       , TEO_KEY_F_NONE , TEO_KEY_COMMA      }, /* ; */
    { GDK_KEY_period          , TEO_KEY_F_SHIFT, TEO_KEY_COMMA      }, /* . */
    { GDK_KEY_colon           , TEO_KEY_F_NONE , TEO_KEY_STOP       }, /* : */
    { GDK_KEY_slash           , TEO_KEY_F_SHIFT, TEO_KEY_STOP       }, /* / */
    { GDK_KEY_less            , TEO_KEY_F_NONE , TEO_KEY_BACKSLASH2 }, /* < */
    { GDK_KEY_greater         , TEO_KEY_F_SHIFT, TEO_KEY_BACKSLASH2 }, /* > */
    { GDK_KEY_Shift_R         , TEO_KEY_F_NONE , TEO_KEY_RSHIFT     },
    { GDK_KEY_Home            , TEO_KEY_F_NONE , TEO_KEY_HOME       },
    { GDK_KEY_Insert          , TEO_KEY_F_NONE , TEO_KEY_INSERT     },
    { GDK_KEY_Delete          , TEO_KEY_F_NONE , TEO_KEY_DEL        },
    { GDK_KEY_space           , TEO_KEY_F_NONE , TEO_KEY_SPACE      }, /*   */
    /* thomson numeric pad */
    { GDK_KEY_KP_7            , TEO_KEY_F_NONE , TEO_KEY_7_PAD      }, /* 7 */
    { GDK_KEY_KP_8            , TEO_KEY_F_NONE , TEO_KEY_8_PAD      }, /* 8 */
    { GDK_KEY_KP_9            , TEO_KEY_F_NONE , TEO_KEY_9_PAD      }, /* 9 */
    { GDK_KEY_KP_4            , TEO_KEY_F_NONE , TEO_KEY_4_PAD      }, /* 4 */
    { GDK_KEY_KP_5            , TEO_KEY_F_NONE , TEO_KEY_5_PAD      }, /* 5 */
    { GDK_KEY_KP_6            , TEO_KEY_F_NONE , TEO_KEY_6_PAD      }, /* 6 */
    { GDK_KEY_KP_1            , TEO_KEY_F_NONE , TEO_KEY_1_PAD      }, /* 1 */
    { GDK_KEY_KP_2            , TEO_KEY_F_NONE , TEO_KEY_2_PAD      }, /* 2 */
    { GDK_KEY_KP_3            , TEO_KEY_F_NONE , TEO_KEY_3_PAD      }, /* 3 */
    { GDK_KEY_KP_0            , TEO_KEY_F_NONE , TEO_KEY_0_PAD      }, /* 0 */
    { GDK_KEY_KP_Enter        , TEO_KEY_F_NONE , TEO_KEY_ENTER_PAD  },
    /* thomson arrow keys */
    { GDK_KEY_Up              , TEO_KEY_F_NONE , TEO_KEY_UP         },
    { GDK_KEY_Down            , TEO_KEY_F_NONE , TEO_KEY_DOWN       },
    { GDK_KEY_Left            , TEO_KEY_F_NONE , TEO_KEY_LEFT       },
    { GDK_KEY_Right           , TEO_KEY_F_NONE , TEO_KEY_RIGHT      },
    /* PC function keys */
    { GDK_KEY_F11             , TEO_KEY_F_NONE , TEO_KEY_F11        },
    { GDK_KEY_F12             , TEO_KEY_F_NONE , TEO_KEY_F12        },
    /* PC numeric pad */
    { GDK_KEY_Num_Lock        , TEO_KEY_F_NONE , TEO_KEY_NUMLOCK    },
    { GDK_KEY_KP_Divide       , TEO_KEY_F_NONE , TEO_KEY_SLASH_PAD  },
    { GDK_KEY_KP_Multiply     , TEO_KEY_F_NONE , TEO_KEY_ASTERISK   },
    { GDK_KEY_KP_Subtract     , TEO_KEY_F_NONE , TEO_KEY_MINUS_PAD  },
    { GDK_KEY_KP_Add          , TEO_KEY_F_NONE , TEO_KEY_PLUS_PAD   },
    { GDK_KEY_KP_Decimal      , TEO_KEY_F_NONE , TEO_KEY_DEL_PAD    },
    /* PC joystick pad */
    { GDK_KEY_KP_Home         , TEO_KEY_F_NONE , TEO_KEY_7_PAD      },       
    { GDK_KEY_KP_Up           , TEO_KEY_F_NONE , TEO_KEY_8_PAD      },
    { GDK_KEY_KP_Page_Up      , TEO_KEY_F_NONE , TEO_KEY_9_PAD      },
    { GDK_KEY_KP_Left         , TEO_KEY_F_NONE , TEO_KEY_4_PAD      },
    { GDK_KEY_KP_Begin        , TEO_KEY_F_NONE , TEO_KEY_5_PAD      },
    { GDK_KEY_KP_Right        , TEO_KEY_F_NONE , TEO_KEY_6_PAD      },
    { GDK_KEY_KP_End          , TEO_KEY_F_NONE , TEO_KEY_1_PAD      },
    { GDK_KEY_KP_Down         , TEO_KEY_F_NONE , TEO_KEY_2_PAD      },
    { GDK_KEY_KP_Page_Down    , TEO_KEY_F_NONE , TEO_KEY_3_PAD      },
    { GDK_KEY_KP_Insert       , TEO_KEY_F_NONE , TEO_KEY_INSERT     },
    { GDK_KEY_KP_Delete       , TEO_KEY_F_NONE , TEO_KEY_DEL        },
    /* PC special keys */
    { GDK_KEY_Escape          , TEO_KEY_F_NONE , TEO_KEY_ESC        },
    { GDK_KEY_Alt_L           , TEO_KEY_F_NONE , TEO_KEY_ALT        },
    { GDK_KEY_ISO_Level3_Shift, TEO_KEY_F_NONE , TEO_KEY_ALTGR      },
    { GDK_KEY_Control_R       , TEO_KEY_F_NONE , TEO_KEY_RCONTROL   },
    { GDK_KEY_Scroll_Lock     , TEO_KEY_F_NONE , TEO_KEY_SCRLOCK    },
    { GDK_KEY_Page_Up         , TEO_KEY_F_NONE , TEO_KEY_PGUP       },
    { GDK_KEY_Page_Down       , TEO_KEY_F_NONE , TEO_KEY_PGDN       },
    { GDK_KEY_End             , TEO_KEY_F_NONE , TEO_KEY_END        },
    /* end of list */
    { -1                      , 0              , 0                  }
};



/* SetPointer:
 *  Sélectionne le pointeur actif.
 */
static void SetPointer(int pointer)
{
    switch (pointer)
    {
        case TEO_STATUS_MOUSE :
            gdk_window_set_cursor (gwindow_win, NULL);
            installed_pointer=TEO_STATUS_MOUSE;
            break;

        case TEO_STATUS_LIGHTPEN :
            gdk_window_set_cursor (gwindow_win, gdk_cursor_new (GDK_PENCIL));
            installed_pointer=TEO_STATUS_LIGHTPEN;
            break;
    }
}



/* button_release_event:
 *  Gestion des touches enfoncées.
 */
static gboolean
delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    teo.command = TEO_COMMAND_QUIT;

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

    if (event->key.state & GDK_CONTROL_MASK) value |= TEO_KEY_F_CTRL;
    if (event->key.state & GDK_MOD2_MASK)    value |= TEO_KEY_F_NUMLOCK;
    if (event->key.state & GDK_LOCK_MASK)    value |= TEO_KEY_F_CAPSLOCK;

    /* special case of HOME+SHIFT */
    if ((event->key.keyval == GDK_KEY_Home)
     && ((event->key.state & GDK_SHIFT_MASK) != 0))
        value |= TEO_KEY_F_SHIFT;

    switch (event->key.keyval&0xff00)
    {
        case 0x0000 : value |= key00[event->key.keyval&0xff].keyflag;
                      keyboard_Reset ((1<<TEO_KEY_F_MAX)-1, value);
                      teo_key = key00[event->key.keyval&0xff].keycode;
                      break;

        case 0xff00 : value |= keyFF[event->key.keyval&0xff].keyflag;
                      keyboard_Reset ((1<<TEO_KEY_F_MAX)-1, value);
                      teo_key = keyFF[event->key.keyval&0xff].keycode;
                      break;

        case 0xfe00 : value |= keyFE[event->key.keyval&0xff].keyflag;
                      keyboard_Reset ((1<<TEO_KEY_F_MAX)-1, value);
                      teo_key = keyFE[event->key.keyval&0xff].keycode;
                      break;
    }

    switch (teo_key)
    {
        case TEO_KEY_ESC : teo.command=TEO_COMMAND_PANEL; break;
        case TEO_KEY_F12 : teo.command=TEO_COMMAND_DEBUGGER; break;
        default          : keyboard_Press (teo_key, FALSE); break;
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
    int teo_key = 0;

    switch (event->key.keyval&0xff00)
    {
        case 0x0000 :
            teo_key = key00[event->key.keyval&0xff].keycode;
            break;

        case 0xff00 :
            teo_key = keyFF[event->key.keyval&0xff].keycode;
            break;

        case 0xfe00 :
            teo_key = keyFE[event->key.keyval&0xff].keycode;
            break;
    }
    keyboard_Press (teo_key, TRUE);
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
        case 1 : mouse_Click(1, FALSE); break;
        case 3 : if (installed_pointer == TEO_STATUS_MOUSE)
                     mouse_Click (2, FALSE);
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
        case 1 : mouse_Click (1, TRUE); break;
        case 3 : mouse_Click (2, TRUE); break;
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
    if (((int)event->button.x > (TEO_BORDER_W*2))
     && ((int)event->button.y > (TEO_BORDER_H*2)))
        mouse_Motion ((int)event->button.x/2-TEO_BORDER_W,
                      (int)event->button.y/2-TEO_BORDER_H);
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
        keyboard_Reset (0, 0);

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
            ugraphic_Retrace(0, 0, TEO_SCREEN_W*2, TEO_SCREEN_H*2);

    return FALSE;
    (void)widget;
    (void)user_data;
}


/* ------------------------------------------------------------------------- */


/* udisplay_Init:
 *  Ouvre la connexion avec le serveur X et initialise le clavier.
 */
void udisplay_Init(void)
{
    int i;
    int ret1=0, ret2=0, ret3=0;

    /* Connexion au serveur X */
    display=gdk_x11_get_default_xdisplay();
    screen=DefaultScreen(display);

    /* Calcul de la table de conversion des keycodes */
    memset (key00, 0x00, sizeof (struct KEY_TABLE) * 256);
    memset (keyFF, 0x00, sizeof (struct KEY_TABLE) * 256);
    memset (keyFE, 0x00, sizeof (struct KEY_TABLE) * 256);
    for (i=0; keyconv[i].keysym!=-1; i++)
    {
        switch (keyconv[i].keysym&0xff00)
        {
            case 0x0000 :
                key00[keyconv[i].keysym&0xff].keycode = keyconv[i].keycode;
                key00[keyconv[i].keysym&0xff].keyflag = keyconv[i].keyflag;
                break;

            case 0xff00 :
                keyFF[keyconv[i].keysym&0xff].keycode = keyconv[i].keycode;
                keyFF[keyconv[i].keysym&0xff].keyflag = keyconv[i].keyflag;
                break;

            case 0xfe00 :
                keyFE[keyconv[i].keysym&0xff].keycode = keyconv[i].keycode;
                keyFE[keyconv[i].keysym&0xff].keyflag = keyconv[i].keyflag;
                break;
        }
    }

    /* Test de présence de l'extension MIT-SHM */
    mit_shm_enabled = XQueryExtension(display, "MIT-SHM", &ret1, &ret2, &ret3);
}



/* udisplay_Window:
 *   Crée la fenêtre principale.
 */
void udisplay_Window(void)
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
    hints.min_width = TEO_SCREEN_W*2;
    hints.max_width = TEO_SCREEN_W*2;
    hints.min_height = TEO_SCREEN_H*2;
    hints.max_height = TEO_SCREEN_H*2;
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

    teo_SetPointer=SetPointer;

    printf("ok\n");
}

