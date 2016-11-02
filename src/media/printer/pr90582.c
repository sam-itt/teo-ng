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
 *  Copyright (C) 1997-2016 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : media/printer/pr90582.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 26/11/2012
 *  Modifié par:
 *
 *  Emulation de la PR90-582.
 */

#include "defs.h"
#include "teo.h"
#include "media/printer.h"

enum {
    COUNTER_8_BITS = 1,
    COUNTER_16_BITS,
    COUNTER_24_BITS,
};



/* escape_code:
 *  Traite le code introduit par ESC pour la PR90-582
 */
static void escape_code (void)
{
    if (printer.data == 16)
    {
        printer_BinaryCounter (COUNTER_16_BITS, printer_PrintPosition);
        return;
    }

    printer_ClearGfxMode7();

    switch ((char)printer.data)
    {
        case '\14' :
            printer_DoubleWidth();
            break;

        case '\15' :
            printer_SimpleWidth();
            break;

        case '#':
            printer_Bold();
            break;

        case '$':
            printer_Thin();
            break;

        case '6':
            printer_LineFeedPerInch(6);
            break;

        case '7':
            printer_LineFeedPerInch(12);
            break;

        case '8':
            printer_LineFeedPerInch(8);
            break;

        case '9':
            printer_LineFeedPerInch(9);
            break;

        case 'B':
            printer_SelectFont (TEO_PRINTER_FACE_ITALIC
                              | TEO_PRINTER_FBIT_NLQ);
            break;

        case 'C':
            printer_SelectFont (TEO_PRINTER_FACE_CONDENSED);
            break;

        case 'D':
            printer_SelectFont (TEO_PRINTER_FACE_SUBSCRIPT
                              | TEO_PRINTER_FBIT_NLQ);
            break;

        case 'E':
            printer_SelectFont (TEO_PRINTER_FACE_ELITE);
            break;

        case 'G':
            printer_DigitCounter (COUNTER_24_BITS, printer_Gfx8);
            break;

        case 'H':
            printer_SelectFont (TEO_PRINTER_FACE_PICA
                              | TEO_PRINTER_FBIT_NLQ);
            break;

        case 'I':
            printer_DigitCounter (COUNTER_24_BITS, printer_Gfx16);
            break;

        case 'N':
            printer_SelectFont (TEO_PRINTER_FACE_PICA);
            break;

        case 'P':
            printer_SelectFont (TEO_PRINTER_FACE_PICA
                              | TEO_PRINTER_FBIT_PROPORTIONAL
                              | TEO_PRINTER_FBIT_NLQ);
            break;

        case 'Q':
            printer_SelectFont (TEO_PRINTER_FACE_ELITE
                              | TEO_PRINTER_FBIT_NLQ);
            break;

        case 'T':
            printer_DigitCounter (COUNTER_16_BITS, printer_LineFeed144);
            break;

        case 'U':
            printer_SelectFont (TEO_PRINTER_FACE_SUPERSCRIPT
                              | TEO_PRINTER_FBIT_NLQ);
            break;

        case 'V':
            printer_DigitCounter (COUNTER_24_BITS, printer_Gfx8Repeat);
            break;

        case 'W':
            printer_DigitCounter (COUNTER_24_BITS, printer_Gfx16Repeat);
            break;

        case 'X':
            printer_Underline();
            break;

        case 'Y':
            printer_NoUnderline();
            break;

        case 'Z':
            printer_DigitCounter (COUNTER_24_BITS, printer_PageLength);
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
    printer.mode7 <<= 1;

    switch (printer.data)
    {
        case 10 :
            printer_LineFeed();
            return;

        case 13 :
            printer_LineStartDip();
            return;

        case 16 :
            printer_DigitCounter (COUNTER_16_BITS, printer_PicaPositionning);
            return;

        case 27 :
            printer.prog = escape_code;
            return;
    }

    printer_ClearGfxMode7();

    switch (printer.data)
    {
        case 7 :
            printer_ScreenPrint();
            break;

        case 12 :
            printer_FormFeed();
            break;

        case 14 :
            printer_DoubleWidth();
            break;

        case 15 :
            printer_SimpleWidth();
            break;

        case 24 :
            printer_ClearBuffer();
            break;

        default :
            printer_DrawableChar(printer.data);
            break;
    }
}


/* ------------------------------------------------------------------------- */


/* pr90582_SetParameters:
 *  Set printer parameters.
 */
void pr90582_SetParameters (void)
{
    printer.chars_per_line = 80;
    printer.screenprint_delay = 100;
    printer.nlq_allowed = FALSE;
    printer.restart_prog = start_code;
}

