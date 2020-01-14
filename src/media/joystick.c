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
 *  Module     : joystick.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 12/02/2001
 *  Modifié par: Eric Botcazou 06/03/2001
 *
 *  Gestion des manettes du TO8.
 */

#include <stdio.h>
#include <string.h>

#include "hardware.h"
#include "media/joystick.h"  /* MacOS */

/* ------------------------------------------------------------------------- */

/* joystick_Reset:
 *  Remet au repos le port manettes.
 */
void joystick_Reset(void)
{
    pia_ext.porta.idr = 0xFF;
    pia_ext.portb.idr = 0xCF;
    pia_ext.porta.cr  |= 0xC0;
    pia_ext.portb.cr  |= 0xC0;
}

END_OF_FUNCTION(joystick_Reset)



/* joystick_Move:
 *  Prend en compte un mouvement des manches des joysticks.
 */
void joystick_Move(int joy, int pos)
{
    int qval = 0xf;

    if (pos & TEO_JOYSTICK_LEFT)
       qval &= 0x0b;
    else if (pos & TEO_JOYSTICK_RIGHT)
       qval &= 0x07;

    if (pos & TEO_JOYSTICK_UP)
       qval &= 0x0d;
    else if (pos & TEO_JOYSTICK_DOWN)
       qval &= 0x0e; 

    if (joy)
        pia_ext.porta.idr = (pia_ext.porta.idr&0xf) | (qval<<4);
    else
        pia_ext.porta.idr = (pia_ext.porta.idr&0xf0) | qval;
}

END_OF_FUNCTION(joystick_Move)


/* joystick_Button:
 *  Prend en compte un changement d'état des boutons des joysticks.
 */
void joystick_Button(int joy, int button, int state)
{
    if (joy)
    {
        /* joystick 1 */
        if (button)
        {
            /* button B */
            if (state == TEO_JOYSTICK_FIRE_ON)
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
            if (state == TEO_JOYSTICK_FIRE_ON)
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
            if (state == TEO_JOYSTICK_FIRE_ON)
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
            if (state == TEO_JOYSTICK_FIRE_ON)
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

END_OF_FUNCTION(joystick_Button)


/* joystick_Init:
 *  Initialise le module joystick.
 */
void joystick_Init(void)
{
    LOCK_FUNCTION(joystick_Reset);
    LOCK_FUNCTION(joystick_Move);
    LOCK_FUNCTION(joystick_Button);
}

int joystick_SymbolToInt(char *symbol)
{
    if(strcmp(symbol,"TEO_JOYSTICK_CENTER") == 0) return TEO_JOYSTICK_CENTER;
    if(strcmp(symbol,"TEO_JOYSTICK_LEFT") == 0) return TEO_JOYSTICK_LEFT;
    if(strcmp(symbol,"TEO_JOYSTICK_RIGHT") == 0) return TEO_JOYSTICK_RIGHT;
    if(strcmp(symbol,"TEO_JOYSTICK_UP") == 0) return TEO_JOYSTICK_UP;
    if(strcmp(symbol,"TEO_JOYSTICK_DOWN") == 0) return TEO_JOYSTICK_DOWN;
    if(strcmp(symbol,"TEO_JOYSTICK_BUTTON_A") == 0) return TEO_JOYSTICK_BUTTON_A;
    if(strcmp(symbol,"TEO_JOYSTICK_BUTTON_B") == 0) return TEO_JOYSTICK_BUTTON_B;
    return -1;
}

void joystick_VerboseDebugCommand(int value)
{
    printf("Joystick command for joystick %d. Raw value is: %d, meaning: ",TEO_JOYN(value),value);
    if(value&TEO_JOYSTICK_CENTER) printf("TEO_JOYSTICK_CENTER ");
    if(value&TEO_JOYSTICK_UP) printf("TEO_JOYSTICK_UP ");
    if(value&TEO_JOYSTICK_DOWN) printf("TEO_JOYSTICK_DOWN ");
    if(value&TEO_JOYSTICK_LEFT) printf("TEO_JOYSTICK_LEFT ");
    if(value&TEO_JOYSTICK_RIGHT) printf("TEO_JOYSTICK_RIGHT ");
    if(value&TEO_JOYSTICK_RIGHT) printf("TEO_JOYSTICK_RIGHT ");
    if(value&TEO_JOYSTICK_BUTTON_A) printf("TEO_JOYSTICK_BUTTON_A ");
    if(value&TEO_JOYSTICK_BUTTON_B) printf("TEO_JOYSTICK_BUTTON_B ");

    printf("\n");
}

