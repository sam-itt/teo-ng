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
 *  Module     : alleg/truecol.c
 *  Version    : 1.8.2
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Samuel Devulder 30/07/2011
 *               François Mouret 25/04/2012 24/10/2012
 *
 *  Gestion de l'affichage 80 colonnes 16-bit du TO8.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "teo.h"


/* paramètres d'affichage */
struct SCREEN_PARAMS
{
    int screen_w;
    int screen_h;
    int border_w;
    int border_h;
    int screen_cw;
    int screen_ch;
    int border_cw;
    int border_ch;
};

static const struct SCREEN_PARAMS tcol1={ TO8_WINDOW_W*2,
                                          TO8_WINDOW_H*2,
                                          0,
                                          0, 
                                          TO8_WINDOW_CW,
                                          TO8_WINDOW_CH,
                                          0,
                                          0 },  /* sans pourtour */

                                  tcol2={ TO8_SCREEN_W*2,
                                          TO8_SCREEN_H*2,
                                          TO8_BORDER_W*2,
                                          TO8_BORDER_H*2,
                                          TO8_SCREEN_CW,
                                          TO8_SCREEN_CH,
                                          TO8_BORDER_CW,
                                          TO8_BORDER_CH };  /* avec pourtour */

/* variables globales */
static int allegro_driver;
static int graphic_mode;
static const struct SCREEN_PARAMS *tcol;
static int *dirty_cell;
static int border_color;
static BITMAP *gpl_buffer, *screen_buffer;
static int palette[TO8_NCOLORS+1];
static int pixel_size;



/* SetColor:
 *  Convertit la couleur du format TO8 au format 16-bit
 *  et la dépose dans la palette.
 */
static void tcol_SetColor(int index, int r, int g, int b)
{
    palette[index]=makecol(r, g, b);
}


#define RECTF(x1, y1, x2, y2)  rectfill(screen_buffer, (x1), (y1), (x2), (y2), palette[border_color])


/* SetBorderColor:
 *  Change la couleur du pourtour de l'écran.
 *  (seulement pour l'écran tcol2)
 */
static void tcol_SetBorderColor(int mode, int color)
{
    register int i,j;
             int *dirty_cell_row = dirty_cell;

    /* on dessine dans le screen buffer */
    if (mode == TO8_PALETTE)
    {
        border_color = TO8_NCOLORS;  /* couleur fixe de l'écran de la palette */
        RECTF(tcol2.border_w, 0, tcol2.screen_w-tcol2.border_w-1, tcol2.border_h-1);
        RECTF(0, 0, tcol2.border_w-1, tcol2.border_h+(TO8_PALETTE_ADDR/TO8_WINDOW_GW)*2-1);
        RECTF(tcol2.screen_w-tcol2.border_w, 0, tcol2.screen_w-1, tcol2.border_h+(TO8_PALETTE_ADDR/TO8_WINDOW_GW)*2-1);

        border_color = color;
        RECTF(0, tcol2.border_h+(TO8_PALETTE_ADDR/TO8_WINDOW_GW)*2, tcol2.border_w-1, tcol2.screen_h-1);
        RECTF(tcol2.screen_w-tcol2.border_w, tcol2.border_h+(TO8_PALETTE_ADDR/TO8_WINDOW_GW)*2, tcol2.screen_w-1, tcol2.screen_h-1);
        RECTF(tcol2.border_w, tcol2.screen_h-tcol2.border_h, tcol2.screen_w-tcol2.border_w-1, tcol2.screen_h-1);
    }
    else 
    {
        border_color = color;
        RECTF(tcol2.border_w, 0, tcol2.screen_w-tcol2.border_w-1, tcol2.border_w-1);
        RECTF(tcol2.border_w, tcol2.screen_h-tcol2.border_h, tcol2.screen_w-tcol2.border_w-1, tcol2.screen_h-1);
        RECTF(0, 0, tcol2.border_w-1, tcol2.screen_h-1);
        RECTF(tcol2.screen_w-tcol2.border_w, 0, tcol2.screen_w-1, tcol2.screen_h-1);
    }

    /* on coche les dirty cells */
    for (j=0; j<tcol2.screen_ch; j++)
    {
        for (i=0; i<tcol2.screen_cw; i++)
            if ( (i<tcol2.border_cw) || (i>=tcol2.screen_cw-tcol2.border_cw) ||
                 (j<tcol2.border_ch) || (j>=tcol2.screen_ch-tcol2.border_ch) )
                dirty_cell_row[i] = TRUE;
            
        /* ligne suivante */
        dirty_cell_row += tcol2.screen_cw;
    }
        
    (void) mode;
}


#define PUT2PIXEL(i, val) putpixel(gpl_buffer, 2*(i),   0, (val)); \
                          putpixel(gpl_buffer, 2*(i)+1, 0, (val))

#define PUT4PIXEL(i, val) putpixel(gpl_buffer, 4*(i),   0, (val)); \
                          putpixel(gpl_buffer, 4*(i)+1, 0, (val)); \
                          putpixel(gpl_buffer, 4*(i)+2, 0, (val)); \
                          putpixel(gpl_buffer, 4*(i)+3, 0, (val))


/* gpl_need_update:
 *  Helper pour les dirty rectangles.
 */
static inline int gpl_need_update(const unsigned char *gpl1, const unsigned char *gpl2)
{
    register int i = TO8_GPL_SIZE*2*pixel_size;

    while (i--)
        if (*gpl1++ != *gpl2++)
            return 1;

    return 0;
}



/* DrawGPL:
 *  Affiche un Groupe Point Ligne (un octet de VRAM).
 */
static void tcol_DrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int c1, c2, x, y;
             int *dirty_cell_row;
    unsigned char *gpl_src, *gpl_dest;

    switch (mode)
    {
        case TO8_BITMAP4: /* mode bitmap 4 couleurs */
            pt<<=1;

            for (i=0; i<8; i++)
            {
                c1 = palette[((pt>>(7-i))&2)+((col>>(7-i))&1)];
                PUT2PIXEL(i, c1);
            }
            break;

        case TO8_PAGE1: /* mode commutation page 1 */
            for (i=0; i<8; i++)
            {
                c1 = palette[(0x80>>i)&pt ? 1 : 0];
                PUT2PIXEL(i, c1);
            }
            break;

        case TO8_PAGE2: /* mode commutation page 2 */
            for (i=0; i<8; i++)
            {
                c1 = palette[(0x80>>i)&pt ? 2 : 0];
                PUT2PIXEL(i, c1);
            }
            break;

        case TO8_STACK2: /* mode superposition 2 pages */
            for (i=0; i<8; i++)
            {
                c1= palette[(0x80>>i)&pt ? 1 : ((0x80>>i)&col ? 2 : 0)];
                PUT2PIXEL(i, c1);
            }
            break;

        case TO8_COL80: /* mode 80 colonnes */
            for (i=0; i<8; i++)
            {
                putpixel(gpl_buffer, i,   0, palette[(0x80>>i)&pt  ? 1 : 0]);
                putpixel(gpl_buffer, i+8, 0, palette[(0x80>>i)&col ? 1 : 0]);
            }   
            break;

        case TO8_STACK4: /* mode superposition 4 pages */
            /* on modifie les pixels 4 par 4 */
            for (i=0; i<4; i++)
            {
                c1 = palette[(0x80>>i)&pt  ?                1 :
                                           ((0x08>>i)&pt  ? 2 :
                                           ((0x80>>i)&col ? 3 :
                                           ((0x08>>i)&col ? 4 : 0)))];
                PUT4PIXEL(i, c1);
            }
            break;

        case TO8_BITMAP4b: /* mode bitmap 4 non documenté */
            for (i=0; i<4; i++)
            {
                c1 = palette[((pt>>(6-(i<<1)))&3)];
                PUT2PIXEL(i, c1);

                c2 = palette[((col>>(6-(i<<1)))&3)];
                PUT2PIXEL(i+4, c2);
            }
            break;

        case TO8_BITMAP16: /* mode bitmap 16 couleurs */
            /* on modifie les pixels 4 par 4 */
            c1 = palette[(pt&0xF0)>>4];
            PUT4PIXEL(0, c1);

            c1 = palette[pt&0xF];
            PUT4PIXEL(1, c1);

            c1 = palette[(col&0xF0)>>4];
            PUT4PIXEL(2, c1);

            c1 = palette[col&0xF];
            PUT4PIXEL(3, c1);
            break;

        case TO8_PALETTE: /* mode écran de la palette */
            if (addr<TO8_PALETTE_ADDR)
            {
                if ((col&0x78)==0x30)
                {
                    c1=makecol(TO8_PALETTE_COL1>>16, (TO8_PALETTE_COL1>>8)&0xFF, TO8_PALETTE_COL1&0xFF);
                    c2=makecol(TO8_PALETTE_COL2>>16, (TO8_PALETTE_COL2>>8)&0xFF, TO8_PALETTE_COL2&0xFF);
                }
                else
                {
                    c2=makecol(TO8_PALETTE_COL1>>16, (TO8_PALETTE_COL1>>8)&0xFF, TO8_PALETTE_COL1&0xFF);
                    c1=makecol(TO8_PALETTE_COL2>>16, (TO8_PALETTE_COL2>>8)&0xFF, TO8_PALETTE_COL2&0xFF);
                }

                for (i=0; i<8; i++)
                {
                    col = (0x80>>i)&pt ? c1 : c2;
                    PUT2PIXEL(i, col);
                }
                break;
            }
            /* no break */

        case TO8_COL40: /* mode 40 colonnes 16 couleurs */
        default:
            c1 = palette[((col>>3)&7)+(((~col)&0x40)>>3)];
            c2 = palette[(col&7)+(((~col)&0x80)>>4)];
 
            for (i=0; i<8; i++)
            {
                col = (0x80>>i)&pt ? c1 : c2;
                PUT2PIXEL(i, col);
            }
    } /* end of switch */

    x = tcol->border_w + (addr%TO8_WINDOW_GW)*TO8_GPL_SIZE*2;
    y = tcol->border_h + (addr/TO8_WINDOW_GW)*2;

    gpl_src  = gpl_buffer->line[0];
    gpl_dest = screen_buffer->line[y]+x*pixel_size;

    if (gpl_need_update(gpl_src, gpl_dest))
    {
        /* duplication des pixels */
        memcpy(gpl_dest, gpl_src, TO8_GPL_SIZE*2*pixel_size);
        gpl_dest = screen_buffer->line[y+1]+x*pixel_size;
        memcpy(gpl_dest, gpl_src, TO8_GPL_SIZE*2*pixel_size);

        /* dirty rectangles */
        x = tcol->border_cw + (addr%TO8_WINDOW_CW);
        y = tcol->border_ch + addr/(TO8_WINDOW_CW*TO8_CHAR_SIZE);
        dirty_cell_row = dirty_cell + y*tcol->screen_cw;
        dirty_cell_row[x] = TRUE;
    }
}



/* DrawBorderLine:
 *  Affiche une ligne de pixels de la frontière de l'écran.
 *  (seulement pour l'écran tcol2)
 */
static void tcol_DrawBorderLine(int col, int line)
{
    int *dirty_cell_row = dirty_cell + (line/TO8_CHAR_SIZE)*tcol2.screen_cw;

    if (col&TO8_LEFT_BORDER)
    {
        if (getpixel(screen_buffer, 0, line*2) != palette[border_color])
        {
            RECTF(0, line*2, tcol2.border_w-1, line*2+1);
        
            dirty_cell_row[0] = TRUE;
            dirty_cell_row[1] = TRUE;
        }
    } 
    else if (col&TO8_RIGHT_BORDER)
    {
        if (getpixel(screen_buffer, tcol2.screen_w-1, line*2) != palette[border_color])
        {
            RECTF(tcol2.screen_w-tcol2.border_w, line*2 , tcol2.screen_w-1, line*2+1);

            dirty_cell_row[tcol2.screen_cw-tcol2.border_cw]   = TRUE;
            dirty_cell_row[tcol2.screen_cw-tcol2.border_cw+1] = TRUE;
        } 
    }
    else if (getpixel(screen_buffer, tcol2.border_w+col*TO8_GPL_SIZE*2, line*2) != palette[border_color])
    {
        RECTF(tcol2.border_w+col*TO8_GPL_SIZE*2, line*2, tcol2.border_w+(col+1)*TO8_GPL_SIZE*2-1, line*2+1);
        
        dirty_cell_row[tcol2.border_cw+col] = TRUE;
    }
} 



/* RetraceScreen:
 *  Rafraîchit une portion de l'écran du TO8.
 */
static inline void tcol_RetraceScreen(int x, int y, int width, int height)
{
    blit(screen_buffer, screen, x, y, x, y, width, height);     
}

END_OF_FUNCTION(tcol_RetraceScreen)



/* RefreshScreen:
 *  Rafraîchit l'écran du TO8.
 */
static void tcol_RefreshScreen(void)
{
    register int i,j;
             int cell_start, *dirty_cell_row = dirty_cell;
    static int odd = 1;

    if (!graphic_mode)
        return;

    acquire_screen();

    if (teo.setting.interlaced_video)
    {
        odd ^= 1;
        for(j=odd; j<tcol->screen_h; j+=2)
	    blit(screen_buffer, screen, 0, j, 0, j, tcol->screen_w, 1);
    }
    else
    {
        /* on groupe les dirty rectangles ligne par ligne */ 
        for (j=0; j<tcol->screen_ch; j++)
        {
            for (i=0; i<tcol->screen_cw; i++)
                if (dirty_cell_row[i])
                {
                    cell_start=i;

                    while ((i<tcol->screen_cw) && dirty_cell_row[i])
                        dirty_cell_row[i++]=FALSE;

                    tcol_RetraceScreen(cell_start*TO8_CHAR_SIZE*2, j*TO8_CHAR_SIZE*2,
                                         (i-cell_start)*TO8_CHAR_SIZE*2, TO8_CHAR_SIZE*2);
                }

            /* ligne suivante */
            dirty_cell_row += tcol->screen_cw;
        }
    }

    release_screen();
}



/* SetGraphicMode:
 *  Sélectionne le mode graphique de l'émulateur.
 */
static int tcol_SetGraphicMode(int mode)
{
    switch (mode)
    {
        case INIT:
            if (set_gfx_mode(allegro_driver, tcol->screen_w, tcol->screen_h, 0, 0))
                return FALSE;
            
            agui_SetColors(0x0, makecol(192, 204, 204), makecol(240, 255, 255));
            graphic_mode=TRUE;
            break;

        case RESTORE:
            set_gfx_mode(allegro_driver, tcol->screen_w, tcol->screen_h, 0, 0);
            agui_SetColors(0x0, makecol(192, 204, 204), makecol(240, 255, 255));
            blit(screen_buffer, screen, 0, 0, 0, 0, tcol->screen_w, tcol->screen_h);
            graphic_mode=TRUE;
            break;
            
        case SHUTDOWN:
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
static void tcol_SetDiskLed(int led_on)
{
    if (graphic_mode)
    {
        if (led_on)
        {
            rect    (screen, tcol->screen_w-LED_SIZE  , 0, tcol->screen_w-1, LED_SIZE-1, 0);
            rectfill(screen, tcol->screen_w-LED_SIZE+1, 1, tcol->screen_w-2, LED_SIZE-2, makecol(0, 255, 0));
        }
        else
            RetraceScreen(tcol->screen_w-LED_SIZE, 0, LED_SIZE, LED_SIZE);
    }
}


extern struct GRAPHIC_DRIVER tcol_driver;


/* InitGraphic:
 *  Initialise le pilote graphique 80 colonnes 16-bit.
 */
static int tcol_InitGraphic(int depth, int _allegro_driver, int border_support)
{
    set_color_depth(depth);

    allegro_driver = _allegro_driver;

    if (border_support)
        tcol = &tcol2;
    else
    {
        tcol = &tcol1;
        tcol_driver.DrawBorderLine = NULL; 
        tcol_driver.SetBorderColor = NULL;
    }

    if (!tcol_SetGraphicMode(INIT))
       return FALSE;

    gpl_buffer = create_bitmap(TO8_GPL_SIZE*2, 1);
    screen_buffer = create_bitmap(tcol->screen_w, tcol->screen_h);
    clear_bitmap(screen_buffer);
    dirty_cell = calloc(tcol->screen_cw*tcol->screen_ch, sizeof(int));
    palette[TO8_NCOLORS] = makecol(TO8_PALETTE_COL1>>16, (TO8_PALETTE_COL1>>8)&0xFF, TO8_PALETTE_COL1&0xFF);

    pixel_size = (depth+1)/8;

    /* objets touchés par l'interruption souris (djgpp) */
    LOCK_VARIABLE(screen_buffer);
    LOCK_DATA(screen_buffer, sizeof(BITMAP));
    LOCK_FUNCTION(tcol_RetraceScreen);

    return TRUE;
}



struct GRAPHIC_DRIVER tcol_driver={
    tcol_InitGraphic,
    tcol_SetGraphicMode,
    NULL,
    tcol_RefreshScreen,
    tcol_RetraceScreen,
    tcol_DrawGPL,
    tcol_DrawBorderLine, 
    tcol_SetColor,
    tcol_SetBorderColor,
    tcol_SetDiskLed,
};

