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
 *  Copyright (C) 1997-2014 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret, Samuel Devulder
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
 *  Module     : media/printer/pr90055.c
 *  Version    : 1.8.3
 *  Créé par   : François Mouret 26/11/2012
 *  Modifié par:
 *
 *  Emulation de la PR90-055.
 */

#include "defs.h"
#include "teo.h"
#include "media/printer.h"


/* vertical_tabulation:
 *  Passe un groupe d'interligne.
 */
static void vertical_tabulation (void)
{
    int i;

    for (i=printer.data&0x7f; i>0; i--)
         printer_LineFeed ();

    printer_Forget ();
}



/* escape_code:
 *  Traite le code introduit par ESC pour la PR90-055.
 */
static void escape_code (void)
{
    switch ((char)printer.data)
    {
        case '6':
            printer_LineFeedPerInch(6);
            break;

        case '9':
            printer_LineFeedPerInch(9);
            break;

        default :
            printer_Forget();
            break;
    }
}



/* start_code:
 *  Traite le code d'engagement.
 */
static void start_code (void)
{
    switch (printer.data)
    {
        case 7 :
            printer_ScreenPrint();
            break;

        case 10 :
            printer_LineFeed();
            break;

        case 11 :
            printer.prog = vertical_tabulation;
            break;

        case 12 :
            printer_FormFeed();
            break;

        case 13 :
            printer_LineStart();
            break;

        case 14 :
            printer_DoubleWidth();
            break;

        case 15 :
            printer_SimpleWidth();
            break;

        case 18 :
            printer_LineFeed();
            break;

        case 20 :
            printer_LineStart();
            break;

        case 27 :
            printer.prog = escape_code;
            break;

        default :
            printer_DrawableChar(printer.data);
            break;
    }
}


/* ------------------------------------------------------------------------- */


/* pr90055_SetParameters:
 *  Ouvre l'imprimante.
 */
void pr90055_SetParameters (void)
{
    printer.chars_per_line = 40;
    printer.screenprint_delay = 100;
    printer.nlq_allowed = FALSE;
    printer.restart_prog = start_code;
}

