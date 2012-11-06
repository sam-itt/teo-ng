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
 *                          Jérémie Guillaume, François Mouret, 
 *                          Samuel Devulder
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
 *  Module     : mouse.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 1999
 *  Modifié par: Eric Botcazou 13/02/2001
 *               Samuel Devulder 05/02/2012
 *               François Mouret 02/11/2012
 *
 *  Gestion de la souris et du crayon optique.
 */


#include "media/joystick.h"
#include "media/mouse.h"  /* MacOS */
#include "hardware.h"
#include "to8.h"


static int mouse_x, mouse_y;


/* ------------------------------------------------------------------------- */

/* mouse_Reset:
 *  Réinitialise les périphériques de pointage.
 */
void mouse_Reset(void)
{
    mc6846.prc&=0xFD;    /* bouton crayon optique relâché */

    joystick_Reset();     /* bouton souris relâché */
}



/* GetLightPen:
 *  Lit l'état du crayon optique.
 */
void mouse_GetLightpen(int *xr, int *yr, int *cc)
{
    switch (mode_page.lgamod)
    {
        case TO8_COL80:    /* mode 80 colonnes */
            *xr=mouse_x*2;
            break;

        case TO8_STACK4:   /* mode superposition 4 pages */
        case TO8_BITMAP16: /* mode bitmap 16 couleurs */
            *xr=mouse_x/2;
            break;

        default:
            *xr=mouse_x;
            break;
    }

    *yr=mouse_y;

    *cc&=0xfe;  /* la lecture est toujours bonne */
}



/* mouse_Motion:
 *  Prend en compte un mouvement de la souris.
 */
void mouse_Motion(int xpos, int ypos)
{
    mouse_x=xpos;
    mouse_y=ypos;

    switch (mode_page.lgamod)
    {
        case TO8_COL80: /* mode 80 colonnes */
            STORE_BYTE(0x60D8, (mouse_x*2)/256);
            STORE_BYTE(0x60D9, (mouse_x*2)%256);
            break;

        case TO8_STACK4:   /* mode superposition 4 pages */
        case TO8_BITMAP16: /* mode bitmap 16 couleurs */
            STORE_BYTE(0x60D8, (mouse_x/2)/256);
            STORE_BYTE(0x60D9, (mouse_x/2)%256);
            break;

        default:
            STORE_BYTE(0x60D8, mouse_x/256);
            STORE_BYTE(0x60D9, mouse_x%256);
            break;
        }

        STORE_BYTE(0x60D6, 0);
        STORE_BYTE(0x60D7, mouse_y);
}

END_OF_FUNCTION(mouse_Motion)



/* mouse_Click:
 *  Prend en compte un changement d'état des boutons de la souris.
 */
void mouse_Click(int button, int release)
{
    if (LOAD_BYTE(0x6074)&0x80)  /* souris */
    {
        if (release)
	    pia_ext.porta.idr |= button;
        else
	    pia_ext.porta.idr &= ~button;
    }
    else if (button==1)  /* crayon optique */
    {
        if (release)
            mc6846.prc &= 0xFD;
        else
            mc6846.prc |= 2;
    }
}

END_OF_FUNCTION(mouse_Click)



/* mouse_Init:
 *  Initialise le module souris.
 */
void mouse_Init(void)
{
    /* on force l'autodétection de la souris au démarrage */
    mem.rom.bank[3][0x2E6C]=0x21; /* BRanch Never */

    /* on désactive la routine SCAN souris */
    mem.mon.bank[1][0x13AF]=0x39;

    /* appel routine de sélection souris/crayon optique */
    mem.rom.bank[3][0x3159]=TO8_TRAP_CODE;

    /* appel routine GETL crayon optique */
    mem.rom.bank[3][0x337D]=TO8_TRAP_CODE;
    mem.rom.bank[3][0x337E]=0x39;

    /* appel routine GETL crayon optique 2 */
    mem.rom.bank[3][0x3F96]=TO8_TRAP_CODE;
    mem.rom.bank[3][0x3F97]=0x39;

    LOCK_VARIABLE(mouse_x);
    LOCK_VARIABLE(mouse_y);
    LOCK_FUNCTION(mouse_Motion);
    LOCK_FUNCTION(mouse_Click);
}

