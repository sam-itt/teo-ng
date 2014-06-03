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
 *  Module     : alleg/color8.h
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou mai 2000
 *  Modifié par: François Mouret 01/11/2012
 *
 *  Gestion de la palette dynamique 256 couleurs (modes 8-bit).
 */


#ifndef ALLEG_COLOR8_H
#define ALLEG_COLOR8_H

struct color_cell {
    int start;
    int end;
    int index;
    RGB last_rgb;
};

extern struct color_cell bcell[];

extern void acolor8_SetPalette(void);
extern void acolor8_GetPalette(void);
extern void acolor8_SetColor(int, int, int, int);
extern void acolor8_SetBorderColor(int, int);
extern void acolor8_RefreshPalette(void);
extern void acolor8_RefreshScreen(void);
extern void acolor8_Init(int);

#endif
