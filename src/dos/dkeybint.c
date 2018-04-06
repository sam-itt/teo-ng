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
 *                          Jérémie Guillaume
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
 *  Module     : dos/keybint.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 23/09/2001
 *               François Mouret 28/12/2012 20/09/2013
 *
 *  Interface de gestion du clavier.
 */


#ifndef SCAN_DEPEND
   #include <allegro.h>
   #include <allegro/internal/aintern.h>
#endif

#include "defs.h"
#include "teo.h"
#include "to8keys.h"
#include "media/keyboard.h"



/* keyboard_handler:
 *  Handler des évènements bas-niveau clavier.
 */
static void keyboard_handler(int key)
{
    int release=key&0x80;

    key&=0x7F;

    switch (key)
    {
        case KEY_ESC:
            if (!release)
                teo.command=TEO_COMMAND_PANEL;
            break;

        case KEY_F11:
            if (!release)
                teo.command=TEO_COMMAND_SCREENSHOT;
            break;

        case KEY_F12:
            if (!release)
                teo.command=TEO_COMMAND_DEBUGGER;
            break;

        default:
            keyboard_Press (key, release);
    }
}

END_OF_FUNCTION(keyboard_handler)



/* set_keyboard_led:
 *  Modifie l'état des Leds du clavier physique.
 */
static void set_keyboard_led(int state)
{
    int flags = 0;

    if (state&TEO_KEY_F_NUMLOCK)
        flags |= KB_NUMLOCK_FLAG;

    if (state&TEO_KEY_F_CAPSLOCK)
        flags |= KB_CAPSLOCK_FLAG;
    
    /* mise à jour des leds */
    set_leds(flags);
}


/* ------------------------------------------------------------------------- */

/* dkeybint_Install:
 *  Installe le module de gestion bas-niveau du clavier.
 */
void dkeybint_Install(void)
{
    static int first=1;
    int mask=0, value=0;
  
    if (first)
    {
        mask |= (TEO_KEY_F_NUMLOCK | TEO_KEY_F_CAPSLOCK);

        if (_key_shifts&KB_NUMLOCK_FLAG)
            value |= TEO_KEY_F_NUMLOCK;

        if (_key_shifts&KB_CAPSLOCK_FLAG)
            value |= TEO_KEY_F_CAPSLOCK;

        /* on prend définitivement le contrôle des leds */
        key_led_flag = FALSE;
        first=0;
    }
       
    mask |= (TEO_KEY_F_SHIFT | TEO_KEY_F_CTRL | TEO_KEY_F_ALTGR);

    if (_key_shifts&KB_SHIFT_FLAG)
        value |= TEO_KEY_F_SHIFT;

    if (_key_shifts&KB_CTRL_FLAG)
        value |= TEO_KEY_F_CTRL;

    if (_key_shifts&KB_ALT_FLAG)
        value |= TEO_KEY_F_ALTGR;

    teo_InputReset(mask, value);

    keyboard_lowlevel_callback=keyboard_handler;
}



/* dkeybint_ShutDown:
 *  Désinstalle le module de gestion bas-niveau du clavier.
 */
void dkeybint_ShutDown(void)
{
    keyboard_lowlevel_callback=NULL;
}



/* dkeybint_Init:
 *  Initialise le module de gestion bas-niveau clavier.
 */
void dkeybint_Init(void)
{
    teo_SetKeyboardLed=set_keyboard_led;
    LOCK_FUNCTION(keyboard_handler);
}

