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
 *  Module     : alleg/joyint.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 14/02/2001
 *  Modifié par: Eric Botcazou 22/03/2001
 *               François Mouret 08/2011 02/11/2012
 *               Samuel Cuella   02/2020
 *
 *  Interface de gestion des manettes.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <allegro.h>
#endif

#include "media/joystick.h"
#include "teo.h"
#include "gettext.h"


static int njoy;
static int old_pos[TEO_NJOYSTICKS];


/* ------------------------------------------------------------------------- */


/* ajoyint_Update:
 *  L'interface joystick d'Allegro ne fonctionne pas selon le mode
 *  asynchrone, donc cette fonction est nécessaire pour lire pério-
 *  diquement la position des joysticks.
 */
void ajoyint_Update(void)
{
    int j, pos = TEO_JOYSTICK_CENTER;

    if (!njoy)
       return;

    /* lecture des joysticks par Allegro */
    poll_joystick();

    for (j=0; j<njoy; j++)
    {
        /* gestion des "arbres" */
        if (joy[j].stick[0].axis[0].d1)
            pos |= TEO_JOYSTICK_LEFT;
        else if (joy[j].stick[0].axis[0].d2)
            pos |= TEO_JOYSTICK_RIGHT;

        if (joy[j].stick[0].axis[1].d1)
            pos |= TEO_JOYSTICK_DOWN;
        else if (joy[j].stick[0].axis[1].d2)
            pos |= TEO_JOYSTICK_UP;

        if (pos != old_pos[j])
        {
            joystick_Move(j, pos);
            old_pos[j] = pos;
        }

        /* gestion des boutons */
        joystick_Button(j, 0, joy[j].button[0].b ?
                               TEO_JOYSTICK_FIRE_ON : TEO_JOYSTICK_FIRE_OFF);

        if (joy[j].num_buttons>1)
            joystick_Button(j, 1, joy[j].button[1].b ?
                                   TEO_JOYSTICK_FIRE_ON : TEO_JOYSTICK_FIRE_OFF);
    }
}



/* ajoyint_Init:
 *  Initialise l'interface joystick.
 */
void ajoyint_Init(int num_joy)
{
    /* affichage du nombre de joystick(s) detecte(s) */
    printf(_("Initialization of input devices : %d joystick(s) detected)\n"), num_joy);

    njoy = num_joy;
}

