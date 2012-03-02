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
 *  Module     : alleg/mode40.c
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 24/0/2003
 *               Samuel Devulder 30/07/2011
 *
 *  Gestion de l'affichage 40 colonnes du TO8.
 */


#ifndef SCAN_DEPEND
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/color8.h"
#include "alleg/gfxdrv.h"
#include "to8.h"


/* variables globales */
static int allegro_driver;
static int graphic_mode;
static BITMAP *gpl_buffer, *screen_buffer;
static int *dirty_cell;
static int interlaced;


#define PUT2PIXEL(val) *gpl_src++ = val; \
                       *gpl_src++ = val;


/* gpl_need_update:
 *  Helper pour les dirty rectangles.
 */
static inline int gpl_need_update(const unsigned char *gpl1, const unsigned char *gpl2)
{
    register int i = TO8_GPL_SIZE;

    while (i--)
        if (*gpl1++ != *gpl2++)
            return 1;

    return 0;
}



/* DrawGPL:
 *  Affiche un Groupe Point Ligne (un octet de VRAM).
 */
static void mod4_DrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int c1, c2, x, y;
             int *dirty_cell_row;
    unsigned char *gpl_src = gpl_buffer->line[0], *gpl_dest;

    switch (mode)
    {
        case TO8_BITMAP4: /* mode bitmap 4 couleurs */
            pt<<=1;

            for (i=0; i<8; i++)
                *gpl_src++ = bcell[((pt>>(7-i))&2)+((col>>(7-i))&1)].index;
            break;

        case TO8_PAGE1: /* mode commutation page 1 */
            for (i=0; i<8; i++)
                *gpl_src++ = bcell[(0x80>>i)&pt ? 1 : 0].index;
            break;

        case TO8_PAGE2: /* mode commutation page 2 */
            for (i=0; i<8; i++)
                *gpl_src++ = bcell[(0x80>>i)&col ? 2 : 0].index;
            break;

        case TO8_STACK2: /* mode superposition 2 pages */
            for (i=0; i<8; i++)
                *gpl_src++ = bcell[(0x80>>i)&pt ? 1 :
                                  ((0x80>>i)&col ? 2 : 0)].index;
            break;

        case TO8_COL80: /* mode 80 colonnes */
            for (i=0;i<4;i++)
                *gpl_src++ = bcell[( pt>>(6-(i<<1)) )&1].index;

            for (i=0;i<4;i++)
                *gpl_src++ = bcell[(col>>(6-(i<<1)) )&1].index;

            break;

        case TO8_STACK4: /* mode superposition 4 pages */
            for (i=0; i<4; i++)
            {
                c1 = bcell[ (0x80>>i)&pt  ? 1 :
                           ((0x08>>i)&pt  ? 2 :
                           ((0x80>>i)&col ? 3 :
                           ((0x08>>i)&col ? 4 : 0)))].index;

                /* on modifie les pixels deux par deux */
                PUT2PIXEL(c1);
            }
            break;


        case TO8_BITMAP4b: /* mode bitmap 4 non documenté */
            for (i=0;i<4;i++)
                *gpl_src++ = bcell[( pt>>(6-(i<<1)) )&3].index;

            for (i=0;i<4;i++)
                *gpl_src++ = bcell[(col>>(6-(i<<1)) )&3].index;

            break;

        case TO8_BITMAP16: /* mode bitmap 16 couleurs */
            /* on modifie les pixels deux par deux */
            c1=bcell[(pt&0xF0)>>4].index;
            PUT2PIXEL(c1);
            
            c1=bcell[pt&0xF].index;
            PUT2PIXEL(c1);
            
            c1=bcell[(col&0xF0)>>4].index;
            PUT2PIXEL(c1);
            
            c1=bcell[col&0xF].index;
            PUT2PIXEL(c1);
            break;

        case TO8_PALETTE: /* mode écran de la palette */
            if (addr<TO8_PALETTE_ADDR)
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
                    *gpl_src++=(0x80>>i)&pt ? c1 : c2;

                break;
            }
            /* no break */
    
        case TO8_COL40: /* mode 40 colonnes 16 couleurs */
        default:
            c1=((col>>3)&7)+(((~col)&0x40)>>3);
            c2=(col&7)+(((~col)&0x80)>>4);

            for (i=0; i<8; i++)
                *gpl_src++ = bcell[(0x80>>i)&pt ? c1 : c2].index;

    } /* end of switch */

    x=(addr%TO8_WINDOW_GW)*TO8_GPL_SIZE;
    y=(addr/TO8_WINDOW_GW);

    gpl_src  = gpl_buffer->line[0];
    gpl_dest = screen_buffer->line[y]+x;

    if (gpl_need_update(gpl_src, gpl_dest))
    {
        memcpy(gpl_dest, gpl_src, TO8_GPL_SIZE);
        
        /* dirty rectangles */
        x = addr%TO8_WINDOW_CW;
        y = addr/(TO8_WINDOW_CW*TO8_CHAR_SIZE);
        dirty_cell_row = dirty_cell + y*TO8_WINDOW_CW;
        dirty_cell_row[x] = TRUE;
    }
}



/* RetraceScreen:
 *  Rafraîchit une portion de l'écran du TO8.
 */
static void mod4_RetraceScreen(int x, int y, int width, int height)
{
    blit(screen_buffer, screen, x, y, x, y, width, height);
}

END_OF_FUNCTION(mod4_RetraceScreen)



/* RefreshScreen:
 *  Rafraîchit l'écran du TO8 en mode entrelacé.
 */
static void mod4_RefreshScreenInterlaced(void)
{
    register int j;
    static int odd = 1;
    odd ^= 1;

    acquire_screen();

    for(j=odd; j<TO8_WINDOW_H*2; j+=2) {
	blit(screen_buffer, screen, 0, j, 0, j, TO8_WINDOW_W*2, 1);
    }

    release_screen();
}



/* RefreshScreen:
 *  Rafraîchit l'écran du TO8.
 */
static void mod4_RefreshScreen(void)
{
    register int i,j;
             int cell_start;
             int *dirty_cell_row = dirty_cell;

    if (!graphic_mode)
        return;
	
    if(interlaced) {
        mod4_RefreshScreenInterlaced();
	return;
    }

    acquire_screen();

    /* on groupe les dirty rectangles ligne par ligne */ 
    for (j=0; j<TO8_WINDOW_CH; j++)
    {
        for (i=0; i<TO8_WINDOW_CW; i++)
            if (dirty_cell_row[i])
	        {
                cell_start=i;

                while ((i<TO8_WINDOW_CW) && dirty_cell_row[i])
                    dirty_cell_row[i++]=FALSE;

                mod4_RetraceScreen(cell_start*TO8_CHAR_SIZE, j*TO8_CHAR_SIZE, (i-cell_start)*TO8_CHAR_SIZE, TO8_CHAR_SIZE);
            }

        /* ligne suivante */
        dirty_cell_row += TO8_WINDOW_CW;
    }

    release_screen();
}



/* SetGraphicMode:
 *  Sélectionne le mode graphique de l'émulateur.
 */
static int mod4_SetGraphicMode(int mode)
{
    switch (mode)
    {
        case INIT:
            if (set_gfx_mode(allegro_driver, TO8_WINDOW_W, TO8_WINDOW_H, 0, 0))
                return FALSE;

            SetPalette8();
            graphic_mode=TRUE;
            break;

        case RESTORE:
            set_gfx_mode(allegro_driver, TO8_WINDOW_W, TO8_WINDOW_H, 0, 0);
            SetPalette8();
            blit(screen_buffer, screen, 0, 0, 0, 0, TO8_WINDOW_W, TO8_WINDOW_H);
            graphic_mode=TRUE;
            break;

        case SHUTDOWN:
            GetPalette8(); /* on sauvegarde la palette */
            set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
            graphic_mode=FALSE;
            break;
    } /* end of switch */

    return TRUE;
}


#define LED_SIZE 6


/* SetDiskLed:
 *  Allume/éteint la Led du lecteur de disquettes.
 */
static void mod4_SetDiskLed(int led_on)
{
    if (graphic_mode)
    {
        if (led_on)
        {
            rect    (screen, TO8_WINDOW_W-LED_SIZE  , 0, TO8_WINDOW_W-1, LED_SIZE-1, 1);
            rectfill(screen, TO8_WINDOW_W-LED_SIZE+1, 1, TO8_WINDOW_W-2, LED_SIZE-2, 6);
        }
        else
            RetraceScreen(TO8_WINDOW_W-LED_SIZE, 0, LED_SIZE, LED_SIZE);
    }
}



/* SetInterlace:
 *   change le mode d'affichage. Retourne le mode précédent.
 */
static int mod4_SetInterlaced(int onoff)
{
     int old = interlaced;
     if(onoff!=interlaced) {
         int i;
         interlaced = onoff;
	 if(!interlaced)
	      for(i = TO8_WINDOW_CW*TO8_WINDOW_CH; --i>=0;) 
	           dirty_cell[i] = TRUE;
     }
     return old;
}



/* InitGraphic:
 *  Initialise le pilote graphique 40 colonnes.
 */
static int mod4_InitGraphic(int depth, int _allegro_driver, int border_support)
{
    if (depth != 8)
        return FALSE;

    set_color_depth(8);
    InitColor8(FALSE);

    allegro_driver = _allegro_driver;

    if (!mod4_SetGraphicMode(INIT))
        return FALSE;

    gpl_buffer = create_bitmap(TO8_GPL_SIZE, 1);
    screen_buffer = create_bitmap(TO8_WINDOW_W, TO8_WINDOW_H);
    clear_bitmap(screen_buffer);
    dirty_cell = calloc(TO8_WINDOW_CW*TO8_WINDOW_CH, sizeof(int));

    (void) border_support;

    return TRUE;
}



struct GRAPHIC_DRIVER mod4_driver={
    mod4_InitGraphic,
    mod4_SetGraphicMode,
    RefreshPalette8,
    mod4_RefreshScreen,
    mod4_RetraceScreen,
    mod4_DrawGPL,
    NULL,
    SetColor8,
    NULL,
    mod4_SetDiskLed,
    mod4_SetInterlaced
};

