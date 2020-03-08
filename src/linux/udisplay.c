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
 *  Module     : linux/udisplay.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 24/11/2003
 *               François Mouret 26/01/2010 08/2011 02/06/2012 28/12/2012
 *                               23/08/2015 31/07/2016
 *               Gilles Fétis 07/2011
 *               Samuel Cuella 01/2020
 *
 *  Module d'interface avec le serveur X.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

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

#include "std.h"
#include "defs.h"
#include "teo.h"
#include "main.h"
#include "to8keys.h"
#include "media/keyboard.h"
#include "media/joystick.h"
#include "media/mouse.h"
#include "media/disk.h"
#include "linux/graphic.h"
#include "gettext.h"
#include "logsys.h"

GtkWidget *wMain;
GdkWindow *gwindow_win;

Display *display;
int screen;
int mit_shm_enabled;
Window screen_win;
Window window_win;

static int need_modifiers_reset = TRUE;

static int installed_pointer = TEO_STATUS_MOUSE;

/*TODO: Rename and move to an include*/
typedef struct{
    int tokey; /*TOKEY_ mappng when no modifier(shift,altgr) is set*/
    int shift; /*KEY_* to TOKEY_ mappng when altgr is set*/
    int altgr; /*KEY_* to TOKEY_ mappng when shift is set*/
    int joycode; /*joystick direction to emulate for that key*/
}teo_kmap_t;

static teo_kmap_t keymap[256]; /*TODO: Dynamic alloc with real range from XGetKeyboardMapping*/
static volatile int jdir_buffer[2][2]; /*joysticks state buffer*/

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
            gdk_window_set_cursor (gwindow_win, gdk_cursor_new_for_display (gdk_window_get_display (gwindow_win), GDK_PENCIL));
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

    return TRUE;
    (void)widget;
    (void)event;
    (void)user_data;
}



static gboolean udisplay_HandleKeyEvent(GdkEventKey *event, int release)
{
    int tokey;

    if(event->hardware_keycode > 255){
        log_msgf(LOG_DEBUG,"Ignoring OOB hardware_keycode %d while max is %d\n",event->hardware_keycode,255);
        return FALSE;
    }

    /*Special (emulator) keys handling:
     * do the emulator command and return.
     * The virtual TO8 won't see the key
     * */
    if(event->keyval == GDK_KEY_Escape && !release){
        teo.command=TEO_COMMAND_PANEL;
        return FALSE;
    }

    if(event->keyval == GDK_KEY_F12 && !release){
        teo.command=TEO_COMMAND_DEBUGGER;
        return FALSE;
    }

    /*Setting the flags on the virtual TO8 keyboard.
     * Actual scancodes are not sent to the virtual TO8
     * except for Capslock      
     **/
    if(event->keyval == GDK_KEY_Control_L){
        keyboard_ToggleState(TEO_KEY_F_CTRL, release);
        return FALSE;
    }

    if(event->keyval == GDK_KEY_ISO_Level3_Shift){
        keyboard_ToggleState(TEO_KEY_F_ALTGR, release);
        return FALSE;
    }

    if(event->keyval == GDK_KEY_Num_Lock){
        keyboard_ToggleState(TEO_KEY_F_NUMLOCK, release);
        return FALSE;
    }

    if(event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_L){
        keyboard_ToggleState(TEO_KEY_F_SHIFT, release);
        return FALSE;
    }

    if(event->keyval == GDK_KEY_Caps_Lock && !release){ 
        keyboard_ToggleState(TEO_KEY_F_CAPSLOCK, release);
        /*No return ! The existing code wanted to a) set the kb_state flag
         * b) pass the code to the lower level*/
    }

    /*Special mode where joysticks are emulated using the keyboard*/
    if(!keyboard_hasFlag(TEO_KEY_F_NUMLOCK) && keymap[event->hardware_keycode].joycode != -1){
        int jdx; 
        int jdir;

        log_msgf(LOG_DEBUG,"Magic key enabled(NUMLOCK off), interpreting %s(%d) as a joystick action\n",gdk_keyval_name(event->keyval), event->hardware_keycode);
        joystick_VerboseDebugCommand(keymap[event->hardware_keycode].joycode);

        jdx = TEO_JOYN(keymap[event->hardware_keycode].joycode);
        jdir = TEO_JOY_DIRECTIONS(keymap[event->hardware_keycode].joycode);

        if(keymap[event->hardware_keycode].joycode & TEO_JOYSTICK_BUTTON_A){
                joystick_Button (jdx-1, 0,
                                 (release != 0) ? TEO_JOYSTICK_FIRE_OFF
                                                : TEO_JOYSTICK_FIRE_ON);
        }
        if(keymap[event->hardware_keycode].joycode & TEO_JOYSTICK_BUTTON_B){
                joystick_Button (jdx-1, 1,
                                 (release != 0) ? TEO_JOYSTICK_FIRE_OFF
                                                : TEO_JOYSTICK_FIRE_ON);
        }

        if (release != 0){
            if (jdir == jdir_buffer[jdx-1][0]){
                jdir_buffer[jdx-1][0] = jdir_buffer[jdx-1][1];
                jdir_buffer[jdx-1][1]=TEO_JOYSTICK_CENTER;
            }else{
                if (jdir == jdir_buffer[jdx-1][1])
                    jdir_buffer[jdx-1][1]=TEO_JOYSTICK_CENTER;
            }
        }else{
            if (jdir != jdir_buffer[jdx-1][0]){
                jdir_buffer[jdx-1][1] = jdir_buffer[jdx-1][0];
                jdir_buffer[jdx-1][0]=jdir;
            }
        }
        joystick_Move (jdx-1, jdir_buffer[jdx-1][0]);

        /*Here we don't return as in the original code where
         * the thomson keycode were also passed to the virtual TO8
         * after having been interpreted as joystick directions.
         * A press on a joystick-bound key will move the virtual joystick
         * AND emit it's own scancode as if the key were pressed on the TO8.
         * Wether or not this is desirable is questionnable and could/should be
         * configurable.
         * */
    }


    /*Sending a scancode to the virtual TO8*/
    tokey = 0;
    if(keyboard_hasFlag(TEO_KEY_F_SHIFT) || 
       (keyboard_hasFlag(TEO_KEY_F_CAPSLOCK) && (keymap[event->hardware_keycode].tokey&SPECIAL_UPC) != 0) ){
        tokey = keymap[event->hardware_keycode].shift;
    }
    if(keyboard_hasFlag(TEO_KEY_F_ALTGR) ){
        tokey = keymap[event->hardware_keycode].altgr;
    }
    if(!tokey){
        tokey = keymap[event->hardware_keycode].tokey; 
    }

    if(tokey){
        keyboard_Press_ng(tokey, release);
    }
    return FALSE;
}

/* key_press_event:
 *  Gestion des touches enfoncées.
 */
static gboolean
key_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    int value = 0;

    if (need_modifiers_reset)
    {
        if (event->key.state & GDK_SHIFT_MASK)   value |= TEO_KEY_F_SHIFT;
        if (event->key.state & GDK_CONTROL_MASK) value |= TEO_KEY_F_CTRL;
        if (event->key.state & GDK_MOD3_MASK)    value |= TEO_KEY_F_ALTGR;
        if (event->key.state & GDK_MOD2_MASK)    value |= TEO_KEY_F_NUMLOCK;
        if (event->key.state & GDK_LOCK_MASK)    value |= TEO_KEY_F_CAPSLOCK;
        keyboard_Reset ((1<<TEO_KEY_F_MAX)-1, value);
        need_modifiers_reset = FALSE;
    }
    return udisplay_HandleKeyEvent(&(event->key), FALSE);
}


/* key_release_event:
 *  Gestion des touches relachées.
 */
static gboolean
key_release_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    return udisplay_HandleKeyEvent(&(event->key), TRUE);
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
    gdk_event_request_motions ((GdkEventMotion *) event);
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
        keyboard_Reset (0, 0);
        need_modifiers_reset = TRUE;
    }
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* visibility_notify_event:
 *  Gestion du retraçage de l'écran.
 */
static gboolean 
visibility_notify_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    if (event->visibility.state == GDK_VISIBILITY_UNOBSCURED)
        ugraphic_Retrace(0, 0, TEO_SCREEN_W*2, TEO_SCREEN_H*2);

    return FALSE;
    (void)widget;
    (void)user_data;
}

/* signal_size_allocate:
 *  Gestion du resize de l'écran.
 */
static gboolean 
configure_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    static int last_width=1;
    static int last_height=1;
  
    if ( (event->configure.width != last_width) || (event->configure.height != last_height) ) {
        last_width=event->configure.width;
        last_height=event->configure.height; 
        log_msgf(LOG_DEBUG, "Resize x=%d y=%d\n",last_width,last_height);
        ugraphic_resize_zoom();
        /* ugraphic_Retrace(0, 0, TEO_SCREEN_W*2, TEO_SCREEN_H*2); */
    }
    return FALSE;
    (void)widget;
    (void)user_data;
}


/* ------------------------------------------------------------------------- */
static gboolean udisplay_GetEntriesForKeyval(char *keyval_name, XkbStateRec *xkbState, GdkKeymapKey **keys, gint *n_keys)
{
    char *ksym;
    GdkKeymap *okeymap;
    GdkDisplay *odisplay;
    guint okeyval;

    ksym = strstr(keyval_name,"GDK_KEY_");
    if(!ksym){
        log_msgf(LOG_DEBUG,"Not a recognized GDK KeySym: %s\n",keyval_name);
        return FALSE;
    }
    ksym = strchr(keyval_name,'_');
    ksym++;
    ksym = strchr(ksym,'_');
    ksym++;

    XkbGetState(display, XkbUseCoreKbd, xkbState);
    log_msgf(LOG_DEBUG,"Active group would be %d\n",xkbState->group);

    odisplay = gdk_display_get_default(); 
    okeymap = gdk_keymap_get_for_display(odisplay);
    okeyval = gdk_keyval_from_name(ksym);
    if(okeyval == GDK_KEY_VoidSymbol){
        return FALSE;
    }
    return gdk_keymap_get_entries_for_keyval(okeymap, okeyval, keys, n_keys);
}



static int udisplay_GksymToKeycode(char *gksym)
{
    gboolean rv;
    GdkKeymapKey *keys;
    gint n_keys;
    gboolean has_group_match;

    XkbStateRec xkbState;

    rv = udisplay_GetEntriesForKeyval(gksym, &xkbState, &keys, &n_keys);
    if(!rv)
        return -1;

    has_group_match = FALSE;
    for(int i = 0; i < n_keys; i++){
        if(keys[i].group == xkbState.group){
            has_group_match = TRUE;
            break;
        }
    }
    for(int i = 0; i < n_keys; i++){
        if(keys[i].group == xkbState.group || !has_group_match)
            return keys[i].keycode;
    }
    return -1;
}

static void udisplay_RegisterJoystickBinding(char *gksym, int jdx, char *jdir, char *jdir2)
{
    int a_int, jd_int;

    a_int = udisplay_GksymToKeycode(gksym);
    if(a_int < 0){
        log_msgf(LOG_DEBUG,"%s not bound: Couldn't find keycodes for keyval %s\n",jdir, gksym);
        return;
    }

    jd_int = joystick_SymbolToInt(jdir);
    if(jdir2)
        jd_int |= joystick_SymbolToInt(jdir2);

    log_msgf(LOG_DEBUG,"GDK key %s(%d) will produce %s + %s (%d)\n",gksym,a_int,jdir,jdir2,jd_int);
    jd_int |= ((jdx == 1) ? TEO_JOY1 : TEO_JOY2); 
    keymap[a_int].joycode = jd_int;
    log_msgf(LOG_DEBUG,"keymap[%d].joycode = %d\n",a_int,keymap[a_int].joycode);
}

static gboolean udisplay_ReadJoystickBindings(GKeyFile *key_file, char *section, int jdx)
{

    char **bindings;
    gsize n_bindings;
    char *gksym;
    char *jdir, *jdir2;

    log_msgf(LOG_INFO,"Loading up joystick emulation key mappings\n");
    bindings = g_key_file_get_keys(key_file, section, &n_bindings, NULL);
    if(!bindings) return FALSE;

    for(int i = 0;  i < n_bindings; i++){
        gksym = bindings[i]; 
        jdir = g_key_file_get_value(key_file, section, gksym, NULL);
        if(!jdir) continue;

        log_msgf(LOG_DEBUG,"Key %s will emit %s\n", gksym, jdir);

        jdir2 = strchr(jdir,'+');
        if(jdir2){
            *jdir2 = '\0';
            jdir2++;
        }

        udisplay_RegisterJoystickBinding(gksym, jdx, (char*)jdir, jdir2);
        g_free(jdir);
    }
    g_strfreev(bindings);
    return True;
}



static gboolean udisplay_RegisterBinding(char *xkb_symbol, char *tokey)
{
    int to_int;
    gboolean rv;
    GdkKeymapKey *okeys;
    gint on_keys;

    XkbStateRec xkbState;
    
    to_int = keyboard_TokeyToInt(tokey);

    rv = udisplay_GetEntriesForKeyval(xkb_symbol, &xkbState, &okeys, &on_keys);
    if(!rv)
        log_msgf(LOG_DEBUG,"%s not bound: Couldn't find keycodes for keyval %s\n",tokey, xkb_symbol);

    /*Groups seems to be used to manage multiple keyboard layouts (at least using MATE) 
     * instead/on top of their orignal meaning. Getting the active layout seems to be 
     * acheivable using XkbGetState. Some keys seems to be shared/stay on group 0, example 
     * Return/Enter.
     * 
     * Therefore we'll do a first pass to check if there is a match for the active group first
     * if so we'll restrict to that group, otherwise we'll take what's come first. 
     * */
    gboolean has_group_match = FALSE;
    for(int i = 0; i < on_keys; i++){
        if(okeys[i].group == xkbState.group){
            has_group_match = TRUE;
            break;
        }
    }
    for(int i = 0; i < on_keys; i++){
        log_msgf(LOG_DEBUG,"Checking key %d: keycode: %d, group: %d, level: %d\n",i, okeys[i].keycode,okeys[i].group,okeys[i].level);
        if(okeys[i].group == xkbState.group || !has_group_match){
            switch(okeys[i].level){
                case 0:
                    log_msgf(LOG_DEBUG,"GDK keycode %d will produce %s(%d)\n", okeys[i].keycode, tokey, to_int);
                    keymap[okeys[i].keycode].tokey = to_int;
                    break;
                case 1:
                    /*When dealing with keypad keys,  level 1 is the value when numlock is pressed
                     * i.e the digit. In Teo we need to have keypads bound to digits
                     * */
                    if(!strstr(xkb_symbol,"GDK_KEY_KP")){ 
                        keymap[okeys[i].keycode].shift = to_int;
                        log_msgf(LOG_DEBUG,"GDK keycode %d + SHIFT will produce %s(%d)\n", okeys[i].keycode, tokey, to_int);
                    }else{
                        keymap[okeys[i].keycode].tokey = to_int;
                        log_msgf(LOG_DEBUG,"GDK keycode %d will produce %s(%d)\n", okeys[i].keycode, tokey, to_int);
                    }
                    break;
                case 2:
                    log_msgf(LOG_DEBUG,"GDK keycode %d + ALTGR will produce %s(%d)\n", okeys[i].keycode, tokey, to_int);
                    keymap[okeys[i].keycode].altgr = to_int;
                    break;
                default:
                    log_msgf(LOG_DEBUG,"Got an unsupported combination for %s: keycode %d and level %d. NOT BOUND\n",tokey, okeys[i].keycode, okeys[i].level);
                    break;
                }
         }
    }
    return TRUE;
}





static void udisplay_LoadKeyBinding(char *filename)
{
    GError *error = NULL;
    GKeyFile *key_file;
    gboolean rv;
    gchar *binding, *b2;

    char **tokey;
    char **tokeys;

    key_file = g_key_file_new();
    rv = g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &error);
    if(!rv){
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("Error loading key file: %s", error->message);
        g_error_free(error);
        return;
    }

    log_msgf(LOG_DEBUG,"Loading up key mappings\n");
    tokeys = keyboard_GetTokeys();
    for(tokey = tokeys; *tokey != NULL; tokey++){
        log_msgf(LOG_DEBUG,"Resolving mapping for emulator definition %s... ", *tokey);
        binding = g_key_file_get_value(key_file, "keymapping", *tokey, NULL);
        log_msgf(LOG_DEBUG,"got %s\n", binding);
        if(!binding) continue;

        b2 = strchr(binding,',');
        if(b2){
            *b2 = '\0';
            b2++;
            udisplay_RegisterBinding(b2, *tokey);
        }
        udisplay_RegisterBinding(binding, *tokey);
        g_free(binding);
    }
    udisplay_ReadJoystickBindings(key_file, "joyemu1", 1);
    udisplay_ReadJoystickBindings(key_file, "joyemu2", 2);
    g_key_file_free(key_file);

}

/** 
 * Init the file-wide X handle and tries to enable X-SHM
 * Also inits the keyboard
 */
void udisplay_Init(const char *keyfile)
{
    int ret1, ret2, ret3;

    /* Connexion au serveur X */
    display=gdk_x11_get_default_xdisplay();
    screen=DefaultScreen(display);

    for(int i = 0; i < 256; i++){
        keymap[i] = (teo_kmap_t){0,0,0,-1};
    }

    udisplay_LoadKeyBinding((char*)keyfile);
    jdir_buffer[0][0] =  jdir_buffer[0][1] = TEO_JOYSTICK_CENTER; 
    jdir_buffer[1][0] =  jdir_buffer[1][1] = TEO_JOYSTICK_CENTER; 

    /* Test de présence de l'extension MIT-SHM */
    mit_shm_enabled = XQueryExtension(display, "MIT-SHM", &ret1, &ret2, &ret3);

}


/**
 * GTK works: Create a window of appropriate size and 
 * activates callbacks for keypresses, focus and visibility.
 *
 * Init file-wide X-related symbols
 *
 * Sets the callback for teo_SetPointer
 *
 */
void udisplay_Window(void)
{
    GdkPixbuf *pixbuf;
    GdkGeometry hints;
    GtkCssProvider *provider;
    GtkStyleContext *context;

    wMain = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_resizable (GTK_WINDOW(wMain), TRUE);
    gtk_window_set_title (GTK_WINDOW(wMain), _("Teo - Thomson TO8 emulator (menu:ESC/debugger:F12)"));

    gtk_widget_add_events (wMain,
                     GDK_FOCUS_CHANGE_MASK
                   | GDK_VISIBILITY_NOTIFY_MASK
                   | GDK_KEY_RELEASE_MASK
                   | GDK_KEY_PRESS_MASK
                   | GDK_STRUCTURE_MASK
                   | GDK_BUTTON_RELEASE_MASK
                   | GDK_BUTTON_PRESS_MASK
                   | GDK_POINTER_MOTION_MASK);

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
    g_signal_connect (G_OBJECT (wMain), "visibility-notify-event",
                      G_CALLBACK (visibility_notify_event), NULL);

    g_signal_connect (G_OBJECT (wMain), "configure-event",
                      G_CALLBACK (configure_event), NULL);

    /* Set window size */
    hints.min_width = TEO_SCREEN_W*2;
    hints.max_width = TEO_SCREEN_W*8;
    hints.min_height = TEO_SCREEN_H*2;
    hints.max_height = TEO_SCREEN_H*8;
    gtk_window_set_geometry_hints (GTK_WINDOW(wMain), wMain, &hints,
                                   GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);

    /* Move the window */
    gtk_window_move (GTK_WINDOW(wMain), 0, 0);

    /* Set program icon */
    pixbuf=gdk_pixbuf_new_from_resource("/net/sourceforge/teoemulator/teo.png", NULL);
    gtk_window_set_icon (GTK_WINDOW(wMain),pixbuf);
    gtk_window_set_default_icon(pixbuf);

    /* Set black background */
    provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (provider),
                                     "* { background-color: #000000; }",
                                     -1, NULL);
    context = gtk_widget_get_style_context (wMain);
    gtk_style_context_add_provider (context,
                                    GTK_STYLE_PROVIDER (provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (provider);

    gtk_widget_set_double_buffered (wMain, FALSE);  /* only one buffer for drawing */
    gtk_widget_set_app_paintable (wMain, TRUE);
    gtk_widget_set_can_focus (wMain, TRUE);

    gtk_widget_show_all (wMain);

    gwindow_win = gtk_widget_get_window (wMain);

#ifndef SCAN_DEPEND
#if GTK_CHECK_VERSION(3,12,0)
    gdk_window_set_event_compression (gwindow_win, FALSE);
#endif
#endif
    window_win = GDK_WINDOW_XID (gwindow_win);
    screen_win = window_win;

    teo_SetPointer=SetPointer;

    main_ConsoleOutput("ok\n");
}

