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
 *  Copyright (C) 1997-2001 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume
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
 *  Module     : win/keybint.c
 *  Version    : 1.8.0
 *  Cr�� par   : Eric Botcazou octobre 1999
 *  Modifi� par: Eric Botcazou 17/09/2001
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
 *  Handler des �v�nements bas-niveau clavier.
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

        default:
            to8_HandleKeyPress(key, release);
    }
}



/* InstallKeybint:
 *  Installe le module de gestion bas-niveau du clavier.
 */
void InstallKeybint(void)
{
    int mask=(1<<TO8_MAX_FLAG)-1, value=0;

    if (_key_shifts&KB_NUMLOCK_FLAG)
        value |= TO8_NUMLOCK_FLAG;

    if (_key_shifts&KB_CAPSLOCK_FLAG)
        value |= TO8_CAPSLOCK_FLAG;

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
 *  D�sinstalle le module de gestion bas-niveau du clavier.
 */
void ShutDownKeybint(void)
{
    keyboard_lowlevel_callback=NULL;
}



/* InitKeybint:
 *  Initialise le module de gestion bas-niveau clavier.
 */
void InitKeybint(void) {}

