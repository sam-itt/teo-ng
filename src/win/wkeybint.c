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
 *  Module     : win/keybint.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 17/09/2001
 *               François Mouret 01/11/2012 10/05/2014
 *
 *  Interface de gestion du clavier.
 */


#ifndef SCAN_DEPEND
   #include <allegro.h>
   #include <allegro/internal/aintern.h>
#endif

#include "teo.h"
#include "to8keys.h"
#include "media/keyboard.h"
#include "alleg/akeyboard.h"




/* ------------------------------------------------------------------------- */


/* wkeybint_Install:
 *  Installe le module de gestion bas-niveau du clavier.
 */
void wkeybint_Install(void)
{
    int mask=(1<<TEO_KEY_F_MAX)-1, value=0;

    if (_key_shifts&KB_NUMLOCK_FLAG)
        value |= TEO_KEY_F_NUMLOCK;

    if (_key_shifts&KB_CAPSLOCK_FLAG)
        value |= TEO_KEY_F_CAPSLOCK;

    if (_key_shifts&KB_SHIFT_FLAG)
        value |= TEO_KEY_F_SHIFT;

    if (_key_shifts&KB_CTRL_FLAG)
        value |= TEO_KEY_F_CTRL;

    if (_key_shifts&KB_ALT_FLAG)
        value |= TEO_KEY_F_ALTGR;

    teo_InputReset(mask, value);

    keyboard_lowlevel_callback=akeyboard_Handler;
}



/* wkeybint_ShutDown:
 *  Désinstalle le module de gestion bas-niveau du clavier.
 */
void wkeybint_ShutDown(void)
{
    keyboard_lowlevel_callback=NULL;
}



/* wkeybint_Init:
 *  Initialise le module de gestion bas-niveau clavier.
 */
void wkeybint_Init(void) /*Unused since akeyboard_Init has gone into afront*/
{
    akeyboard_Init();
}

