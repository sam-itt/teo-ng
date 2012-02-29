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
 *  Copyright (C) 1997-2011 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret
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
 *  Module     : dos/vga.c
 *  Version    : 1.8.0
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 22/09/2001
 *               Samuel Devulder 07/2011
 *               François Mouret 08/2011
 *
 *  Gestion de l'affichage 40 colonnes du TO8.
 */


#ifndef SCAN_DEPEND
   #include <pc.h>
   #include <go32.h>
   #include <allegro.h>
#endif

#include "alleg/color8.h"
#include "alleg/gfxdrv.h"
#include "to8.h"


static int graphic_mode;
static unsigned short vga_sel;
static BITMAP *gpl_buffer, *screen_buffer;

#define BLIT_GPL(src, addr)  __asm__ __volatile__ (                     \
                                 "movw %w0, %%fs\n"                     \
                                 "roll $3, %%eax\n"                     \
                                 "addl $0xa0000, %%eax\n"               \
                                 "movl (%%ebx), %%edx\n"                \
                                 "movl %%edx, %%fs: (%%eax)\n"          \
                                 "movl 4(%%ebx), %%edx\n"               \
                                 "movl %%edx, %%fs: 4(%%eax)\n"         \
                                 :                                      \
                                 : "o" (vga_sel), "b" (src), "a" (addr) \
                                 : "edx"                                \
                             )



#define PUT2PIXEL(val) *gpl_src++ = val; \
                       *gpl_src++ = val;

static int interlaced = 0;


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
static void vga_DrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int c1,c2;
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

    gpl_src  = gpl_buffer->line[0];
    gpl_dest = screen_buffer->line[addr/40]+(addr%40)*8;

    if (gpl_need_update(gpl_src, gpl_dest))
    {
        memcpy(gpl_dest, gpl_src, TO8_GPL_SIZE);

        if (graphic_mode)
            BLIT_GPL(gpl_src, addr);
    }
}

END_OF_FUNCTION(vga_DrawGPL)



/* RetraceScreen:
 *  Rafraîchit une portion de l'écran du TO8.
 */
static void vga_RetraceScreen(int x, int y, int width, int height)
{
    blit(screen_buffer, screen, x, y, x, y, width, height);
}



/* SetGraphicMode:
 *  Sélectionne le mode graphique de l'émulateur.
 */
static int vga_SetGraphicMode(int mode)
{
    switch (mode)
    {
        case INIT:
            if (set_gfx_mode(GFX_VGA,320,200,0,0))
                return FALSE;
                
            SetPalette8();
            graphic_mode=TRUE;
            break;

        case RESTORE:
            set_gfx_mode(GFX_VGA, 320, 200, 0, 0);
            SetPalette8();
            blit(screen_buffer, screen, 0, 0, 0, 0, 320, 200);
            graphic_mode=TRUE;
            break;

        case SHUTDOWN:
            GetPalette8(); /* on sauvegarde la palette */
            set_gfx_mode(GFX_TEXT,0,0,0,0);
            graphic_mode=FALSE;
            break;
    } /* end of switch */

    return TRUE;
}



/* SetDiskLed:
 *  Allume/éteint la Led du lecteur de disquettes.
 */
static void vga_SetDiskLed(int led_on)
{
    if (graphic_mode)
    {
        if (led_on)
        {
            rect(screen,314,0,319,5,1);
            rectfill(screen,315,1,318,4,6);
	}
	else
	    blit(screen_buffer,screen,314,0,314,0,6,6);
    }
}



/* InitGraphic:
 *  Initialise le pilote graphique 40 colonnes.
 */ 
static int vga_InitGraphic(int depth, int _allegro_driver, int border_support)
{
    if (depth != 8)
        return FALSE;

    set_color_depth(8);
    InitColor8(border_support);

    if (!vga_SetGraphicMode(INIT))
        return FALSE;

    vga_sel = _dos_ds;
    gpl_buffer = create_bitmap(TO8_GPL_SIZE, 1);
    screen_buffer = create_bitmap(TO8_WINDOW_W, TO8_WINDOW_H);
    clear_bitmap(screen_buffer);
    LOCK_FUNCTION(vga_DrawGPL);

    return TRUE;

    (void) _allegro_driver;
}



static int vga_SetInterlaced (int onoff)
{
     int old = interlaced;
     if(onoff!=interlaced)
         interlaced = onoff;
     return old;
}


struct GRAPHIC_DRIVER mod4_driver={
    vga_InitGraphic,
    vga_SetGraphicMode,
    RefreshPalette8,
    RefreshScreen8,
    vga_RetraceScreen,
    vga_DrawGPL,
    NULL,
    SetColor8,
    SetBorderColor8,
    vga_SetDiskLed,
    vga_SetInterlaced
};

