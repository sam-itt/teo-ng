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
 *                          Jérémie Guillaume
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
 *  Module     : alleg/color8.c
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou mai 2000
 *  Modifié par: Eric Botcazou 24/10/2003
 *               François Mouret 24/10/2012
 *
 *  Gestion de la palette dynamique 256 couleurs (modes 8-bit).
 */


#ifndef SCAN_DEPEND
   #include <allegro.h>
#endif

#include "alleg/color8.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h" 
#include "teo.h"
#include "defs.h"


#define PALETTE_START  (TEO_NCOLORS+1)
#define PALETTE_END                255
#define BCELL_SIZE                   4

#define MAKERGB(x) {(x>>18)&0x3F, (x>>10)&0x3F, (x>>2)&0x3F}

static PALETTE palette={ { 0, 0, 0},  /* bordure de l'écran */
                         { 0, 0, 0},  /* GUI */
                         {47,50,50},  /* GUI */
                         {60,63,63},  /* GUI */
                         {63, 0, 0},  /* LED rouge */
                         {63,63, 0},  /* LED jaune */
                         { 0,63, 0},  /* LED verte */
                         MAKERGB(TEO_PALETTE_COL1),   /* page palette */
                         MAKERGB(TEO_PALETTE_COL2) };

static unsigned int palette_flag[256];
static struct color_cell shcell;
       struct color_cell bcell[TEO_NCOLORS];
static int border_enabled;
static int border_color;

#define LOCK_MASK (1<<31)
#define LOCK(i)   palette_flag[i]|=LOCK_MASK
#define UNLOCK(i) palette_flag[i]&=~LOCK_MASK
#define ISEQUAL(rgb1,rgb2) (((rgb1).r==(rgb2).r)&&((rgb1).g==(rgb2).g)&&((rgb1).b==(rgb2).b))



/* AllocCellColor:
 *  Alloue un élément de la cellule au triplet RGB spécifié.
 */
static int AllocCellColor(struct color_cell *cell, RGB *rgb, unsigned int last_frame)
{
    register int i;

    for (i=cell->start; i<cell->end; i++)
        if (ISEQUAL(palette[i],*rgb))
            return i;

    for (i=cell->start; i<cell->end; i++)
        if (palette_flag[i]<=last_frame)
        {
            palette[i]=*rgb;
            palette_flag[i]=frame;

            set_color(i,rgb);
            return i;
        }

    return 0;
}



/* AllocColor:
 *  Alloue un élément de la palette au triplet RGB spécifié.
 */
static int AllocColor(int color, RGB *rgb, int last_frame)
{
    /* on cherche une case adaptée dans la cellule de base */
    int index=AllocCellColor(&bcell[color], rgb, last_frame);

    if (!index)
    {
        /* on cherche maintenant dans la cellule partagée */
        index=AllocCellColor(&shcell, rgb, last_frame);

        if (!index)
            return 0;
    }

    UNLOCK(bcell[color].index);
    bcell[color].index=index;
    LOCK(bcell[color].index);

    return index;
}


/* ------------------------------------------------------------------------- */


/* acolor8_SetPalette:
 *  Mise à jour de la palette hardware.
 */
void acolor8_SetPalette(void)
{
    set_palette(palette);
    agui_SetColors(1, 2, 3);
}


/* acolor8_GetPalette:
 *  Récupération de la palette hardware.
 */
void acolor8_GetPalette(void)
{
    get_palette(palette);
}



/* acolor8_SetColor:
 *  Convertit la couleur du format TO8 au format RGB et la dépose dans
 *  la palette; en mode VGA 13h, 0 est la couleur du bord de l'écran.
 */
void acolor8_SetColor(int color, int r, int g, int b)
{
    RGB rgb={r>>2, g>>2, b>>2};

    /* on cherche une case adaptée dans la palette */
    if (AllocColor(color, &rgb, frame-3))
        need_palette_refresh&=~(1<<color);
    else
    {
        /* on stocke provisoirement le triplet RGB */
        bcell[color].last_rgb=rgb;
        need_palette_refresh|=(1<<color);
    }
}



/* acolor8_SetBorderColor:
 *  Fixe la couleur du pourtour de l'écran en copiant
 *  le triplet RGB choisi sur la couleur 0.
 *  (dos/vga uniquement)
 */
void acolor8_SetBorderColor(int mode, int color)
{
    border_color=color;

    (void) mode;
}



/* acolor8_RefreshPalette:
 *  Essaie de mettre à jour la palette.
 */
 void acolor8_RefreshPalette(void)
{
    register int i;

    for (i=0; i<TEO_NCOLORS; i++)
        if (need_palette_refresh&(1<<i))
            if (AllocColor(i, &bcell[i].last_rgb, frame-2))
            {
                need_palette_refresh&=~(1<<i);
                teo_new_video_params=TRUE;
            }
}



/* acolor8_RefreshScreen:
 *  Met à jour le pourtour de l'écran.
 *  (dos/vga uniquement)
 */
void acolor8_RefreshScreen(void)
{
    static RGB current_rgb = {-1, -1, -1};
    RGB new_rgb = palette[bcell[border_color].index];

    if (!ISEQUAL(new_rgb, current_rgb))
    {
        set_color(0, &new_rgb);
        current_rgb = new_rgb;
    } 
}



/* acolor8_Init:
 *  Initialise la palette dynamique 256 couleurs.
 */
void acolor8_Init(int _border_enabled)
{
    register int i;

    /* initialisation des cellules de base de la palette */
    for (i=0; i<TEO_NCOLORS; i++)
    {
        bcell[i].start= PALETTE_START+    i*BCELL_SIZE;
        bcell[i].end  = PALETTE_START+(i+1)*BCELL_SIZE;
    }

    /* initialisation de la cellule partagée */
    shcell.start= PALETTE_START+TEO_NCOLORS*BCELL_SIZE;
    shcell.end  = PALETTE_END;

    border_enabled = _border_enabled;
}

