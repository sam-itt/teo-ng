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
 *  Copyright (C) 1997-2012 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : joystick.c
 *  Version    : 1.8.1
 *  Cr�� par   : Eric Botcazou 12/02/2001
 *  Modifi� par: Eric Botcazou 06/03/2001
 *
 *  Gestion des manettes du TO8.
 */


#include "intern/hardware.h"
#include "intern/joystick.h"  /* MacOS */



/* ResetJoystick:
 *  Remet au repos le port manettes.
 */
void ResetJoystick(void)
{
    pia_ext.porta.idr = 0xFF;
    pia_ext.portb.idr = 0xCF;
    pia_ext.porta.cr  |= 0xC0;
    pia_ext.portb.cr  |= 0xC0;
}

END_OF_FUNCTION(ResetJoystick)



/**********************************/
/* partie publique                */
/**********************************/


/* HandleJoystickMove:
 *  Prend en compte un mouvement des manches des joysticks.
 */
void to8_HandleJoystickMove(int joy, int pos)
{
    int qval = 0xf;

    if (pos & TO8_JOYSTICK_LEFT)
       qval &= 0x0b;
    else if (pos & TO8_JOYSTICK_RIGHT)
       qval &= 0x07;

    if (pos & TO8_JOYSTICK_UP)
       qval &= 0x0d;
    else if (pos & TO8_JOYSTICK_DOWN)
       qval &= 0x0e; 

    if (joy)
        pia_ext.porta.idr = (pia_ext.porta.idr&0xf) | (qval<<4);
    else
        pia_ext.porta.idr = (pia_ext.porta.idr&0xf0) | qval;
}

END_OF_FUNCTION(to8_HandleJoystickMove)


/* HandleJoystickFire:
 *  Prend en compte un changement d'�tat des boutons des joysticks.
 */
void to8_HandleJoystickFire(int joy, int button, int state)
{
    if (joy)
    {
        /* joystick 1 */
        if (button)
        {
            /* button B */
            if (state == TO8_JOYSTICK_FIRE_ON)
            {
                pia_ext.portb.idr &= 0xf7;
                pia_ext.portb.cr  &= 0x7f;
            }
            else
            {
                pia_ext.portb.idr |= 0x08;
                pia_ext.portb.cr  |= 0x80;
            }
        }
        else
        {
            /* button A */
            if (state == TO8_JOYSTICK_FIRE_ON)
            {
                pia_ext.portb.idr &= 0x7f;
                pia_ext.portb.cr  &= 0xbf;
            }
            else
            {
                pia_ext.portb.idr |= 0x80;
                pia_ext.portb.cr  |= 0x40;
            }
        }
    }
    else
    {
        /* joystick 0 */
        if (button)
        {
            /* button B */
            if (state == TO8_JOYSTICK_FIRE_ON)
            {
                pia_ext.portb.idr &= 0xfb;
                pia_ext.porta.cr  &= 0x7f;
            }
            else
            {
                pia_ext.portb.idr |= 0x04;
                pia_ext.porta.cr  |= 0x80;
            }
        }
        else
        {
            /* button A */
            if (state == TO8_JOYSTICK_FIRE_ON)
            {
                pia_ext.portb.idr &= 0xbf;
                pia_ext.porta.cr  &= 0xbf;
            }
            else
            {
                pia_ext.portb.idr |= 0x40;
                pia_ext.porta.cr  |= 0x40;
            }
        }
    }
}

END_OF_FUNCTION(to8_HandleJoystickFire)


/* InitJoystick:
 *  Initialise le module joystick.
 */
void InitJoystick(void)
{
    LOCK_FUNCTION(ResetJoystick);
    LOCK_FUNCTION(to8_HandleJoystickMove);
    LOCK_FUNCTION(to8_HandleJoystickFire);
}
