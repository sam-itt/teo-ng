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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : media/printer/pr90042.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 26/11/2012
 *  Modifié par:
 *
 *  Emulation de la PR90-042.
 */

#include "defs.h"
#include "teo.h"
#include "media/printer.h"

#define COUNTER_8_BITS  1

#define CHARS_PER_LINE  40

static int  buffer_size = 0;
static char buffer[CHARS_PER_LINE] = "";
static int last_data = 0;



static void flush_buffer (void)
{
    int i;

    for (i=0; i<buffer_size; i++)
        printer_DrawableChar((int)buffer[i]&0xff);

    buffer_size = 0;
}



/* line_feed:
 *  Imprime le tampon et passe une ligne.
 */
static void line_feed (void)
{
    flush_buffer();
    printer_LineFeed();
    printer_Forget ();
}



/* form_feed:
 *  Imprime le tampon et passe un groupe de 8 interlignes.
 */
static void form_feed (void)
{
    int i;

    flush_buffer();
    for (i=0; i<8; i++)
         printer_LineFeed ();

    printer_Forget ();
}



/* vertical_tabulation:
 *  Imprime le tampon et passe un groupe de X interlignes.
 */
static void vertical_tabulation (void)
{
    int i;

    flush_buffer();
    for (i=printer.data&0x7f; i>0; i--)
         printer_LineFeed ();

    printer_Forget ();
}



/* right_justified:
 *  Imprime le tampon justifié à droite et passe une ligne.
 */
static void right_justified (void)
{
    int i;

    for (i=0; i<(CHARS_PER_LINE-buffer_size); i++)
        printer_DrawableChar(32);

    flush_buffer();
    printer_LineFeed();
    printer_Forget ();
}



/* pr90042_start:
 *  Traite le code d'engagement.
 */
static void start_code (void)
{
    switch (printer.data)
    {
        case 7 :
            flush_buffer();
            printer_ScreenPrint();
            break;

        case 10 :
            if (last_data == 13)
            {
                last_data = 0;
                return;
            }
            line_feed();
            break;

        case 11 :
            printer.prog = vertical_tabulation;
            break;

        case 12 :
            form_feed();
            break;

        case 13 :
            if (last_data == 10)
            {
                last_data = 0;
                return;
            }
            line_feed();
            break;

        case 18 :
            right_justified();
            break;

        default :
            if ((printer.data >= 32) && (printer.data <= 127))
            {
                if (buffer_size == CHARS_PER_LINE)
                    line_feed();
                buffer[buffer_size++] = (char)printer.data;
            }
            break;
    }
    last_data = printer.data;
}


/* ------------------------------------------------------------------------- */


/* pr90042_Setparameters:
 *  Ouvre l'imprimante.
 */
void pr90042_SetParameters (void)
{
    printer.chars_per_line = CHARS_PER_LINE;
    printer.screenprint_delay = 60;
    printer.nlq_allowed = FALSE;
    printer.restart_prog = start_code;
}

