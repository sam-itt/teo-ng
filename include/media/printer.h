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
 *  Module     : media/printer.h
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou 22/03/2001
 *  Modifié par: Eric Botcazou 24/03/2001
 *               François Mouret 18/04/2012 01/11/2012
 *
 *  Emulation de l'imprimante.
 */


#ifndef MEDIA_PRINTER_H
#define MEDIA_PRINTER_H

#define TEO_PRINTER_FACE_PICA         (0<<3)
#define TEO_PRINTER_FACE_ITALIC       (1<<3)
#define TEO_PRINTER_FACE_ELITE        (2<<3)
#define TEO_PRINTER_FACE_CONDENSED    (3<<3)
#define TEO_PRINTER_FACE_SUPERSCRIPT  (4<<3)
#define TEO_PRINTER_FACE_SUBSCRIPT    (5<<3)
#define TEO_PRINTER_FACE_FONT_MASK    (7<<3)

#define TEO_PRINTER_FBIT_NLQ          (1<<0)
#define TEO_PRINTER_FBIT_DOUBLE_WIDTH (1<<1)
#define TEO_PRINTER_FBIT_PROPORTIONAL (1<<2)
#define TEO_PRINTER_FBIT_STYLE_MASK   3

struct PRINTER {
    int   data;
    int   screenprint_delay;
    void  (*prog)();
    void  (*restart_prog)();
    int   counter_value;
    int   chars_per_line;
    int   mode7;
    int   nlq_allowed;
    struct EMUTEO_LPRT lprt;
};

extern struct PRINTER printer;

#define PRINTER_NUMBER 5
struct PRINTER_CODE_LIST {
    char name[9];
    int  number;
};

extern struct PRINTER_CODE_LIST printer_code_list[];

/* printer commands */
extern void printer_DigitCounter (int length, void (*jump)());
extern void printer_BinaryCounter (int length, void (*jump)());
extern void printer_Forget (void);
extern void printer_LineFeedPerInch (int nblines);
extern void printer_LineFeed144 (void);
extern void printer_LeftMargin (void);
extern void printer_DotPrintPosition (void);
extern void printer_SpaceDot (void);
extern void printer_Gfx7Data (void);
extern void printer_Gfx8 (void);
extern void printer_Gfx16 (void);
extern void printer_ScreenPrint (void);
extern void printer_Gfx8Repeat (void);
extern void printer_Gfx16Repeat (void);
extern void printer_PrintPosition (void);
extern void printer_CharPositionning (void);
extern void printer_PicaPositionning (void);
extern void printer_LineFeed (void);
extern void printer_LineStart (void);
extern void printer_LineStartDip (void);
extern void printer_FormFeed (void);
extern void printer_SelectFont (int face);
extern void printer_PageLength (void);
extern void printer_Underline (void);
extern void printer_NoUnderline (void);
extern void printer_Bold (void);
extern void printer_Thin (void);
extern void printer_DoubleWidth (void);
extern void printer_SimpleWidth (void);
extern void printer_Reset (void);
extern void printer_ClearGfxMode7(void);
extern void printer_ClearBuffer (void);

/* print a char */
extern void printer_DrawableChar(int data);

/* printer functions */
extern void printer_Init(void);
extern void printer_WriteData(int mask, int value);
extern void printer_SetStrobe(int state);
extern void printer_Close(void);

#endif

