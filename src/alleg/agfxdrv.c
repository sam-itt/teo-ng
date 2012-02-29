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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2011 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Samuel Devulder
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
 *  Module     : alleg/gfxdrv.c
 *  Version    : 1.8.0
 *  Cr�� par   : Eric Botcazou octobre 1999
 *  Modifi� par: Eric Botcazou 23/09/2000
 *               Samuel Devulder 12/08/2011
 *
 *  S�lection du driver graphique.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <allegro.h>
#endif

#include "alleg/gfxdrv.h"
#include "alleg/mouse.h"
#include "alleg/main.h"
#include "to8.h"


int need_palette_refresh;

int  (*SetGraphicMode)(int);
void (*RefreshBorder)(void);
void (*RefreshPalette)(void);
void (*RefreshScreen)(void);
void (*RetraceScreen)(int, int, int, int);
int (*SetInterlaced)(int);


static struct GRAPHIC_DRIVER *graphic_driver_list[]={
    NULL,
    &mod4_driver,
    &mod8_driver,
    &tcol_driver,
};



/* InitGraphic:
 *  S�lectionne et initialise le mode graphique sp�cifi�.
 */
int InitGraphic(int mode, int depth, int allegro_driver, int border_support)
{
    if (graphic_driver_list[mode]->InitGraphic(depth, allegro_driver, border_support))
    {   
        to8_DrawGPL        = graphic_driver_list[mode]->DrawGPL;
        to8_DrawBorderLine = graphic_driver_list[mode]->DrawBorderLine;
        to8_SetColor       = graphic_driver_list[mode]->SetColor;
        to8_SetBorderColor = graphic_driver_list[mode]->SetBorderColor;
        to8_SetDiskLed     = graphic_driver_list[mode]->SetDiskLed;

        SetGraphicMode = graphic_driver_list[mode]->SetGraphicMode;
        RefreshPalette = graphic_driver_list[mode]->RefreshPalette;
        RefreshScreen  = graphic_driver_list[mode]->RefreshScreen;
        RetraceScreen  = graphic_driver_list[mode]->RetraceScreen;
        SetInterlaced  = graphic_driver_list[mode]->SetInterlaced;

        need_palette_refresh = FALSE;

        /* Initialisation de la souris */
        InitPointer(mode, border_support);

        return TRUE;
    }
    else
        return FALSE;
}



/* Screenshot:
 *  Prend une photo d'�cran de l'�mulateur.
 */
void Screenshot(void)
{
    static int count;
    char filename[FILENAME_LENGTH];
    BITMAP *bmp;
    PALETTE palette;

    get_palette(palette);
    bmp = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
    sprintf(filename, "teo_%02d.bmp", count++);
    save_bmp(filename, bmp, palette);
    destroy_bitmap(bmp);
}

