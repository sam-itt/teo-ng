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
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 23/09/2001
 *
 *  Interface de gestion du clavier.
 */


#ifndef SCAN_DEPEND
   #include <allegro.h>
   #include <allegro/internal/aintern.h>
#endif

#include "alleg/main.h"
#include "to8.h"



/* KeyboardHandler:
 *  Handler des évènements bas-niveau clavier.
 */
static void KeyboardHandler(int key)
{
    int release=key&0x80;

    key&=0x7F;

    switch (key)
    {
        case KEY_ESC:
            if (!release)
                teo.command=CONTROL_PANEL;
            break;

        case KEY_F11:
            if (!release)
                teo.command=SCREENSHOT;
            break;

        case KEY_F12:
            if (!release)
                teo.command=DEBUGGER;
            break;

        default:
            to8_HandleKeyPress(key, release);
    }
}

END_OF_FUNCTION(KeyboardHandler)



/* SetKeyboardLed:
 *  Modifie l'état des Leds du clavier physique.
 */
static void SetKeyboardLed(int state)
{
    int flags = 0;

    if (state&TO8_NUMLOCK_FLAG)
        flags |= KB_NUMLOCK_FLAG;

    if (state&TO8_CAPSLOCK_FLAG)
        flags |= KB_CAPSLOCK_FLAG;
    
    /* mise à jour des leds */
    set_leds(flags);
}



/* InstallKeybint:
 *  Installe le module de gestion bas-niveau du clavier.
 */
void InstallKeybint(void)
{
    static int first=1;
    int mask=0, value=0;
  
    if (first)
    {
        mask |= (TO8_NUMLOCK_FLAG | TO8_CAPSLOCK_FLAG);

        if (_key_shifts&KB_NUMLOCK_FLAG)
            value |= TO8_NUMLOCK_FLAG;

        if (_key_shifts&KB_CAPSLOCK_FLAG)
            value |= TO8_CAPSLOCK_FLAG;

        /* on prend définitivement le contrôle des leds */
        key_led_flag = FALSE;
        first=0;
    }
       
    mask |= (TO8_SHIFT_FLAG | TO8_CTRL_FLAG | TO8_ALTGR_FLAG);

    if (_key_shifts&KB_SHIFT_FLAG)
        value |= TO8_SHIFT_FLAG;

    if (_key_shifts&KB_CTRL_FLAG)
        value |= TO8_CTRL_FLAG;

    if (_key_shifts&KB_ALT_FLAG)
        value |= TO8_ALTGR_FLAG;

    to8_InputReset(mask, value);

    keyboard_lowlevel_callback=KeyboardHandler;
}



/* ShutDownKeybint:
 *  Désinstalle le module de gestion bas-niveau du clavier.
 */
void ShutDownKeybint(void)
{
    keyboard_lowlevel_callback=NULL;
}



/* InitKeybint:
 *  Initialise le module de gestion bas-niveau clavier.
 */
void InitKeybint(void)
{
    to8_SetKeyboardLed=SetKeyboardLed;
    LOCK_FUNCTION(KeyboardHandler);
}

