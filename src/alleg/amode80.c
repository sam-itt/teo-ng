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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : alleg/mode80.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Samuel Devulder 30/07/2011
 *               François Mouret 25/04/2012 24/10/2012
 *
 *  Gestion de l'affichage 80 colonnes du TO8.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/color8.h"
#include "alleg/gfxdrv.h"
#include "teo.h"


/* variables globales */
static int allegro_driver;
static int graphic_mode;
static BITMAP *gpl_buffer, *screen_buffer;
static int *dirty_cell;


#define PUT2PIXEL(val) *gpl_src++ = val; \
                       *gpl_src++ = val


/* gpl_need_update:
 *  Helper pour les dirty rectangles.
 */
static inline int gpl_need_update(const unsigned char *gpl1, const unsigned char *gpl2)
{
    register int i = TEO_GPL_SIZE*2;

    while (i--)
        if (*gpl1++ != *gpl2++)
            return 1;

    return 0;
}



/* DrawGPL:
 *  Affiche un Groupe Point Ligne (un octet de VRAM).
 */
static void amode80_DrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int c1, c2, x, y;
             int *dirty_cell_row; 
    unsigned char *gpl_src = gpl_buffer->line[0], *gpl_dest;

    switch (mode)
    {
        case TEO_BITMAP4: /* mode bitmap 4 couleurs */
            pt<<=1;

            for (i=0; i<8; i++)
            {
                c1 = bcell[((pt>>(7-i))&2)+((col>>(7-i))&1)].index;
                PUT2PIXEL(c1);
            }
            break;

        case TEO_PAGE1: /* mode commutation page 1 */
            for (i=0; i<8; i++)
            {
                c1 = bcell[(0x80>>i)&pt ? 1 : 0].index;
                PUT2PIXEL(c1);
            }
            break;

        case TEO_PAGE2: /* mode commutation page 2 */
            for (i=0; i<8; i++)
            {
                c1 = bcell[(0x80>>i)&pt ? 2 : 0].index;
                PUT2PIXEL(c1);
            }
            break;

        case TEO_STACK2: /* mode superposition 2 pages */
            for (i=0; i<8; i++)
            {
                c1= bcell[(0x80>>i)&pt ? 1 : ((0x80>>i)&col ? 2 : 0)].index;
                PUT2PIXEL(c1);
            }
            break;

        case TEO_COL80: /* mode 80 colonnes */
            for (i=0; i<8; i++)
                *gpl_src++ = bcell[(0x80>>i)&pt ? 1 : 0].index;
            
            for (i=0; i<8; i++)
                *gpl_src++ = bcell[(0x80>>i)&col ? 1 : 0].index;
                
            break;

        case TEO_STACK4: /* mode superposition 4 pages */
            /* on modifie les pixels 4 par 4 */
            for (i=0; i<4; i++)
            {
                c1 = bcell[(0x80>>i)&pt  ? 1 :
                          ((0x08>>i)&pt  ? 2 :
                          ((0x80>>i)&col ? 3 :
                          ((0x08>>i)&col ? 4 : 0)))].index;
                PUT2PIXEL(c1);
                PUT2PIXEL(c1);
            }
            break;

        case TEO_BITMAP4b: /* mode bitmap 4 non documenté */
            for (i=0;i<4;i++)
            {
                c1 = bcell[((pt>>(6-(i<<1)))&3)].index;
                PUT2PIXEL(c1);
            }

            for (i=0;i<4;i++)
            {
                c1 = bcell[((col>>(6-(i<<1)))&3)].index;
                PUT2PIXEL(c1);
            }
            break;

        case TEO_BITMAP16: /* mode bitmap 16 couleurs */
            /* on modifie les pixels 4 par 4 */
            c1 = bcell[(pt&0xF0)>>4].index;
            PUT2PIXEL(c1);
            PUT2PIXEL(c1);

            c1 = bcell[pt&0xF].index;
            PUT2PIXEL(c1);
            PUT2PIXEL(c1);

            c1 = bcell[(col&0xF0)>>4].index;
            PUT2PIXEL(c1);
            PUT2PIXEL(c1);

            c1 = bcell[col&0xF].index;
            PUT2PIXEL(c1);
            PUT2PIXEL(c1);
            break;

        case TEO_PALETTE: /* mode écran de la palette */
            if (addr<TEO_PALETTE_ADDR)
            {
                if ((col&0x78)==0x30)
                {
                    c1=7;
                    c2=8;
                }
                else
                {
                    c1=8;
                    c2=7;
                }

                for (i=0; i<8; i++)
                {
                    col = (0x80>>i)&pt ? c1 : c2;
                    PUT2PIXEL(col);
                }
                break;
            }
            /* no break */
            
        case TEO_COL40: /* mode 40 colonnes 16 couleurs */
        default:
            c1 = bcell[((col>>3)&7)+(((~col)&0x40)>>3)].index;
            c2 = bcell[(col&7)+(((~col)&0x80)>>4)].index;
 
            for (i=0; i<8; i++)
            {
                col = (0x80>>i)&pt ? c1 : c2;
                PUT2PIXEL(col);
            }
    } /* end of switch */

    x=(addr%TEO_WINDOW_GW)*TEO_GPL_SIZE*2;
    y=(addr/TEO_WINDOW_GW)*2;

    gpl_src  = gpl_buffer->line[0];
    gpl_dest = screen_buffer->line[y]+x;

    if (gpl_need_update(gpl_src, gpl_dest))
    {
        /* duplication des pixels */
        memcpy(gpl_dest, gpl_src, TEO_GPL_SIZE*2);
        gpl_dest = screen_buffer->line[y+1]+x;
        memcpy(gpl_dest, gpl_src, TEO_GPL_SIZE*2);

        /* dirty rectangles */
        x = addr%TEO_WINDOW_CW;
        y = addr/(TEO_WINDOW_CW*TEO_CHAR_SIZE);
        dirty_cell_row = dirty_cell + y*TEO_WINDOW_CW;
        dirty_cell_row[x] = TRUE;
    }
}



/* RetraceScreen:
 *  Rafraîchit une portion de l'écran du TO8.
 */
static inline void amode80_RetraceScreen(int x, int y, int width, int height)
{
    blit(screen_buffer, screen, x, y, x, y, width, height);
}

END_OF_FUNCTION(amode80_RetraceScreen)



/* RefreshScreen:
 *  Rafraîchit l'écran du TO8.
 */
static void amode80_RefreshScreen(void)
{
    register int i,j;
             int cell_start;
             int *dirty_cell_row = dirty_cell;
    static int odd = 1;

    if (!graphic_mode)
        return;

    acquire_screen();

    if (teo.setting.interlaced_video)
    {
        odd ^= 1;
        for(j=odd; j<TEO_WINDOW_H<<1; j+=2)
	        blit(screen_buffer, screen, 0, j, 0, j, TEO_WINDOW_W<<1, 1);
    }
    else
    {
        /* on groupe les dirty rectangles ligne par ligne */ 
        for (j=0; j<TEO_WINDOW_CH; j++)
        {
            for (i=0; i<TEO_WINDOW_CW; i++)
                if (dirty_cell_row[i])
                {
                    cell_start=i;

                    while ((i<TEO_WINDOW_CW) && dirty_cell_row[i])
                        dirty_cell_row[i++]=FALSE;

                    amode80_RetraceScreen(cell_start*TEO_CHAR_SIZE*2, j*TEO_CHAR_SIZE*2,
                                       (i-cell_start)*TEO_CHAR_SIZE*2, TEO_CHAR_SIZE*2);
                }

            /* ligne suivante */
            dirty_cell_row += TEO_WINDOW_CW;
        }
    }

    release_screen();
}



/* SetGraphicMode:
 *  Sélectionne le mode graphique de l'émulateur.
 */
static int amode80_SetGraphicMode(int mode)
{   
    switch (mode)
    {
        case INIT:
            if (set_gfx_mode(allegro_driver, TEO_WINDOW_W*2, TEO_WINDOW_H*2, 0, 0))
                return FALSE;
            
            acolor8_SetPalette();
            graphic_mode=TRUE;
            break;

        case RESTORE:
            set_gfx_mode(allegro_driver, TEO_WINDOW_W*2, TEO_WINDOW_H*2, 0, 0);
            acolor8_SetPalette();
            blit(screen_buffer, screen, 0, 0, 0, 0, TEO_WINDOW_W*2, TEO_WINDOW_H*2);
            graphic_mode=TRUE;
            break;
            
        case SHUTDOWN:
            acolor8_GetPalette(); /* on sauvegarde la palette */
            set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
            graphic_mode=FALSE;
            break;
    }

    return TRUE;
}

 
#define LED_SIZE 12


/* SetDiskLed:
 *  Allume/éteint la Led du lecteur de disquettes.
 */
static void amode80_SetDiskLed(int led_on)
{
    static int count = 0;

    if (graphic_mode)
    {
        if (led_on)
        {
            rect    (screen, TEO_WINDOW_W*2-LED_SIZE,
                             0,
                             TEO_WINDOW_W*2-1,
                             LED_SIZE-1,
                             1);
            rectfill(screen, TEO_WINDOW_W*2-LED_SIZE+1,
                             1,
                             TEO_WINDOW_W*2-2,
                             LED_SIZE-2,
                             6);
            rect    (screen, TEO_WINDOW_W*2-LED_SIZE+count,
                             count,
                             TEO_WINDOW_W*2-1-count,
                             LED_SIZE-1-count,
                             1);
            count = (count+1)%(LED_SIZE/2);
        }
        else
            RetraceScreen(TEO_WINDOW_W*2-LED_SIZE, 0, LED_SIZE, LED_SIZE);
    }
}



/* InitGraphic:
 *  Initialise le pilote graphique 80 colonnes.
 */
static int amode80_InitGraphic(int depth, int _allegro_driver, int border_support)
{
    if (depth != 8)
        return FALSE;
    
    set_color_depth(8);
    acolor8_Init(FALSE);

    allegro_driver = _allegro_driver;

    if (!amode80_SetGraphicMode(INIT))
        return FALSE;

    gpl_buffer = create_bitmap(TEO_GPL_SIZE*2, 1);
    screen_buffer = create_bitmap(TEO_WINDOW_W*2, TEO_WINDOW_H*2);
    clear_bitmap(screen_buffer);
    dirty_cell = calloc(TEO_WINDOW_CW*TEO_WINDOW_CH, sizeof(int));

    /* objets touchés par l'interruption souris (djgpp) */
    LOCK_VARIABLE(screen_buffer);
    LOCK_DATA(screen_buffer, sizeof(BITMAP));
    LOCK_FUNCTION(amode80_RetraceScreen);

    (void) border_support;
 
    return TRUE;
}



struct GRAPHIC_DRIVER amode80_driver={
    amode80_InitGraphic,
    amode80_SetGraphicMode,
    acolor8_RefreshPalette,
    amode80_RefreshScreen,
    amode80_RetraceScreen,
    amode80_DrawGPL,
    NULL,
    acolor8_SetColor,
    NULL,
    amode80_SetDiskLed
};

