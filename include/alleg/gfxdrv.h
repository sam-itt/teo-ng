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
 *                          Jérémie Guillaume, Samuel Devulder
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
 *  Module     : alleg/gfxdrv.h
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 29/11/2000
 *               Samuel Devulder 30/07/2011
 *
 *  Sélection du pilote graphique.
 */


#ifndef ALLEG_GFXDRV_H
#define ALLEG_GFXDRV_H

struct GRAPHIC_DRIVER {
    int  (*InitGraphic)(int, int, int);
    int  (*SetGraphicMode)(int);
    void (*RefreshPalette)(void);	
    void (*RefreshScreen)(void);
    void (*RetraceScreen)(int, int, int, int);
    void (*DrawGPL)(int, int, int, int);
    void (*DrawBorderLine)(int, int);
    void (*SetColor)(int, int, int, int);
    void (*SetBorderColor)(int, int);
    void (*SetDiskLed)(int);
};

extern struct GRAPHIC_DRIVER mod4_driver, mod8_driver, tcol_driver;

extern int need_palette_refresh;


enum {
     INIT,
     RESTORE,
     SHUTDOWN
};

extern int  (*SetGraphicMode)(int);
extern void (*RefreshBorder)(void);
extern void (*RefreshPalette)(void);
extern void (*RefreshScreen)(void);
extern void (*RetraceScreen)(int, int, int, int);

enum {
    NO_GFX,
    GFX_MODE40,
    GFX_MODE80,
    GFX_TRUECOLOR,
    GFX_WINDOW
};

extern int  InitGraphic(int, int, int, int);
extern void Screenshot(void);

#endif
