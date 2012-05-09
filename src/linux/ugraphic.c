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
 *  Module     : linux/graphic.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 24/10/2003
 *               François Mouret 26/01/2010 08/2011 25/04/2012
 *               Samuel Devulder 07/2011
 *
 *  Gestion de l'affichage du TO8.
 */


#ifndef SCAN_DEPEND
   #include <string.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <sys/ipc.h>
   #include <sys/shm.h>
   #include <X11/Xlib.h>
   #include <X11/Xutil.h>
   #include <X11/extensions/XShm.h>
#endif

#include "linux/display.h"
#include "intern/gui.h"
#include "to8.h"


/* variables globales */
static XVisualInfo visualinfo;
static GC gc;
static Colormap colormap;

static int *dirty_cell;
static int border_color;
static XImage *gpl_buffer, *screen_buffer;
static XColor xcolor[TO8_NCOLORS+4];
static int pixel_size;

/* SetColor:
 *  Change une couleur de la palette du TO8.
 */
static void SetColor(int index, int r, int g, int b)
{
    xcolor[index].red   = 0x101*r;
    xcolor[index].green = 0x101*g;
    xcolor[index].blue  = 0x101*b;

    if (visualinfo.class == TrueColor)
        XAllocColor(display, colormap, &xcolor[index]);
    else if (visualinfo.class == PseudoColor)
    {
	xcolor[index].flags=DoRed|DoGreen|DoBlue;
	XStoreColor(display, colormap, &xcolor[index]);
    }
}



/* RECTF:
 *  Helper pour dessiner un rectangle plein à l'écran.
 */
static void RECTF(int x1, int y1, int x2, int y2)
{
    register int i,j;

    for (j=y1; j<=y2; j++)
	for (i=x1; i<=x2; i++)
	    XPutPixel(screen_buffer, i, j, xcolor[border_color].pixel);
}



/* SetBorderColor:
 *  Change la couleur du pourtour de l'écran.
 */
static void SetBorderColor(int mode, int color)
{
    register int i, j;
    int *dirty_cell_row = dirty_cell;

    /* on dessine dans le screen buffer */
    if (mode == TO8_PALETTE)
    {
        border_color = TO8_NCOLORS;  /* couleur fixe de l'écran de la palette */
        RECTF(TO8_BORDER_W*2, 0, (TO8_SCREEN_W-TO8_BORDER_W)*2-1, TO8_BORDER_H*2-1);
        RECTF(0, 0, TO8_BORDER_W*2-1, (TO8_BORDER_H+(TO8_PALETTE_ADDR/TO8_WINDOW_GW))*2-1);
        RECTF((TO8_SCREEN_W-TO8_BORDER_W)*2, 0, TO8_SCREEN_W*2-1, (TO8_BORDER_H+(TO8_PALETTE_ADDR/TO8_WINDOW_GW))*2-1);

	border_color = color;
        RECTF(0, (TO8_BORDER_H+(TO8_PALETTE_ADDR/TO8_WINDOW_GW))*2, TO8_BORDER_W*2-1, TO8_SCREEN_H*2-1);
        RECTF((TO8_SCREEN_W-TO8_BORDER_W)*2, (TO8_BORDER_H+(TO8_PALETTE_ADDR/TO8_WINDOW_GW))*2, TO8_SCREEN_W*2-1, TO8_SCREEN_H*2-1);
        RECTF(TO8_BORDER_W*2, (TO8_SCREEN_H-TO8_BORDER_H)*2, (TO8_SCREEN_W-TO8_BORDER_W)*2-1, TO8_SCREEN_H*2-1);
    }
    else
    {
        border_color = color;
        RECTF(TO8_BORDER_W*2, 0, (TO8_SCREEN_W-TO8_BORDER_W)*2-1, TO8_BORDER_W*2-1);
        RECTF(TO8_BORDER_W*2, (TO8_SCREEN_H-TO8_BORDER_H)*2, (TO8_SCREEN_W-TO8_BORDER_W)*2-1, TO8_SCREEN_H*2-1);
        RECTF(0, 0, TO8_BORDER_W*2-1, TO8_SCREEN_H*2-1);
        RECTF((TO8_SCREEN_W-TO8_BORDER_W)*2, 0, TO8_SCREEN_W*2-1, TO8_SCREEN_H*2-1);
    }

    /* on coche les dirty cells */
    for (j=0; j<TO8_SCREEN_CH; j++)
    {
        for (i=0; i<TO8_SCREEN_CW; i++)
            if ( (i<TO8_BORDER_CW) || (i>=TO8_SCREEN_CW-TO8_BORDER_CW) ||
                 (j<TO8_BORDER_CH) || (j>=TO8_SCREEN_CH-TO8_BORDER_CH) )
                dirty_cell_row[i] = True ;

        /* ligne suivante */
        dirty_cell_row += TO8_SCREEN_CW;
    }

    (void) mode;
}


#define PUT2PIXEL(i, val) XPutPixel(gpl_buffer, 2*(i),   0, (val)); \
                          XPutPixel(gpl_buffer, 2*(i)+1, 0, (val))

#define PUT4PIXEL(i, val) XPutPixel(gpl_buffer, 4*(i),   0, (val)); \
                          XPutPixel(gpl_buffer, 4*(i)+1, 0, (val)); \
                          XPutPixel(gpl_buffer, 4*(i)+2, 0, (val)); \
                          XPutPixel(gpl_buffer, 4*(i)+3, 0, (val))


/* gpl_need_update:
 *  Helper pour les dirty rectangles.
 */
static inline int gpl_need_update(const char *gpl1, const char *gpl2)
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
static void DrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int x, y, c1, c2;
             int *dirty_cell_row;
    char *gpl_src, *gpl_dest;

    switch (mode)
    {
	 case TO8_BITMAP4: /* mode bitmap 4 couleurs */
	    pt<<=1;

	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[((pt>>(7-i))&2)+((col>>(7-i))&1)].pixel;
		PUT2PIXEL(i, c1);
	    }
            break;

        case TO8_PAGE1: /* mode commutation page 1 */
	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[(0x80>>i)&pt ? 1 : 0].pixel;
		PUT2PIXEL(i, c1);
	    }
	    break;

        case TO8_PAGE2: /* mode commutation page 2 */
	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[(0x80>>i)&col ? 2 : 0].pixel;
		PUT2PIXEL(i, c1);
	    }
	    break;

        case TO8_STACK2: /* mode superposition 2 pages */
	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[(0x80>>i)&pt ? 1 : ((0x80>>i)&col ? 2 : 0)].pixel;
		PUT2PIXEL(i, c1);
	    }
	    break;

        case TO8_COL80: /* mode 80 colonnes */
	    for (i=0; i<8; i++)
	    {
		XPutPixel(gpl_buffer, i,   0, xcolor[(0x80>>i)&pt  ? 1 : 0].pixel);
		XPutPixel(gpl_buffer, i+8, 0, xcolor[(0x80>>i)&col ? 1 : 0].pixel);
	    }
	    break;

	case TO8_STACK4: /* mode superposition 4 pages */
	    /* on modifie les pixels 4 par 4 */
	    for (i=0; i<4; i++)
	    {
		c1= xcolor[ (0x80>>i)&pt  ? 1 :
		           ((0x08>>i)&pt  ? 2 :
		           ((0x80>>i)&col ? 3 :
		           ((0x08>>i)&col ? 4 : 0)) )].pixel;

		PUT4PIXEL(i, c1);
	    }
	    break;

        case TO8_BITMAP4b: /* mode bitmap 4 non documenté */
	    for (i=0; i<4; i++)
	    {
		c1 = xcolor[( pt>>(6-(i<<1)) )&3].pixel;
		PUT2PIXEL(i, c1);

		c2 = xcolor[(col>>(6-(i<<1)) )&3].pixel;
		PUT2PIXEL(i+4, c2);
	    }
	    break;

        case TO8_BITMAP16: /* mode bitmap 16 couleurs */
	    /* on modifie les pixels 4 par 4 */
	    c1 = xcolor[(pt&0xF0)>>4].pixel;
	    PUT4PIXEL(0, c1);

	    c1 = xcolor[pt&0xF].pixel;
	    PUT4PIXEL(1, c1);

	    c1 = xcolor[(col&0xF0)>>4].pixel;
	    PUT4PIXEL(2, c1);

	    c1 = xcolor[col&0xF].pixel;
	    PUT4PIXEL(3, c1);
	    break;

        case TO8_PALETTE: /* mode écran de la palette */
	    if (addr<TO8_PALETTE_ADDR)
	    {
		if ((col&0x78)==0x30)
		{
		    c1=TO8_NCOLORS;
		    c2=TO8_NCOLORS+1;
		}
		else
		{
		    c1=TO8_NCOLORS+1;
		    c2=TO8_NCOLORS;
		}

		for (i=0; i<8; i++)
		{
		    col = xcolor[pt&(0x80>>i) ? c1 : c2].pixel;
		    PUT2PIXEL(i, col);
		}
		break;
	    }
	    /* no break */

        case TO8_COL40: /* mode 40 colonnes 16 couleurs */
        default:
	    c1=((col>>3)&7)+(((~col)&0x40)>>3);
	    c2=(col&7)+(((~col)&0x80)>>4);

	    for (i=0; i<8; i++)
	    {
		col = xcolor[pt&(0x80>>i) ? c1 : c2].pixel;
		PUT2PIXEL(i, col);
	    }
    } /* end of switch */

    x = TO8_BORDER_W*2 + (addr%TO8_WINDOW_GW)*TO8_GPL_SIZE*2;
    y = TO8_BORDER_H*2 + (addr/TO8_WINDOW_GW)*2;

    gpl_src  = gpl_buffer->data;
    gpl_dest = screen_buffer->data + y*screen_buffer->bytes_per_line + x*pixel_size;

    if (gpl_need_update(gpl_src, gpl_dest))
    {
        /* duplication des pixels */
        memcpy(gpl_dest, gpl_src, TO8_GPL_SIZE*2*pixel_size);
        gpl_dest += screen_buffer->bytes_per_line;
        memcpy(gpl_dest, gpl_src, TO8_GPL_SIZE*2*pixel_size);

	/* dirty rectangles */
	x = TO8_BORDER_CW + (addr%TO8_WINDOW_CW);
	y = TO8_BORDER_CH + addr/(TO8_WINDOW_CW*TO8_CHAR_SIZE);
	dirty_cell_row = dirty_cell + y*TO8_SCREEN_CW;
	dirty_cell_row[x] = True;
    }
}



/* DrawBorderLine:
 *  Affiche une ligne de pixels de la frontière de l'écran.
 */
static void DrawBorderLine(int col, int line)
{
    int *dirty_cell_row = dirty_cell + (line/TO8_CHAR_SIZE)*TO8_SCREEN_CW;

    if (col&TO8_LEFT_BORDER)
    {
	if (XGetPixel(screen_buffer, 0, line*2) != xcolor[border_color].pixel)
        {
            RECTF(0, line*2, TO8_BORDER_W*2-1, line*2+1);

	    dirty_cell_row[0] = True;
	    dirty_cell_row[1] = True;
	}
    }
    else if (col&TO8_RIGHT_BORDER)
    {
	if (XGetPixel(screen_buffer, TO8_SCREEN_W*2-1, line*2) != xcolor[border_color].pixel)
        {
            RECTF((TO8_SCREEN_W-TO8_BORDER_W)*2, line*2 , TO8_SCREEN_W*2-1, line*2+1);

	    dirty_cell_row[TO8_SCREEN_CW-TO8_BORDER_CW] = True;
	    dirty_cell_row[TO8_SCREEN_CW-TO8_BORDER_CW+1] = True;
	}
    }
    else if (XGetPixel(screen_buffer, (TO8_BORDER_W+col*TO8_GPL_SIZE)*2, line*2) != xcolor[border_color].pixel)
    {
        RECTF((TO8_BORDER_W+col*TO8_GPL_SIZE)*2, line*2, (TO8_BORDER_W+(col+1)*TO8_GPL_SIZE)*2-1, line*2+1);

        dirty_cell_row[TO8_BORDER_CW+col] = True;
    }
}



/* RetraceScreen:
 *  Rafraîchit une portion de l'écran du TO8.
 */
inline void RetraceScreen(int x, int y, int w, int h)
{
    if (mit_shm_enabled)
	XShmPutImage(display, screen_win, gc, screen_buffer, x, y, x, y, w, h, True);
    else
	XPutImage(display, screen_win, gc, screen_buffer, x, y, x, y, w, h);
}



/* RefreshScreen:
 *  Rafraîchit l'écran du TO8.
 */
void RefreshScreen(void)
{
    register int i,j;
    int cell_start;
    int *dirty_cell_row = dirty_cell;
    static int odd = 1;

    if (gui->setting.interlaced_video)
    {
        odd ^= 1;
        for(j=odd; j<TO8_SCREEN_CH*TO8_CHAR_SIZE<<1; j+=2)
            RetraceScreen(0, j, TO8_SCREEN_CW*TO8_CHAR_SIZE<<1, 1);
    }        
    else
    {
        /* on groupe les dirty rectangles ligne par ligne */
        for (j=0; j<TO8_SCREEN_CH; j++)
        {
            for (i=0; i<TO8_SCREEN_CW; i++)
                if (dirty_cell_row[i])
	        {
                    cell_start=i;

                    while ((i<TO8_SCREEN_CW) && dirty_cell_row[i])
                        dirty_cell_row[i++]=False;

                    RetraceScreen(cell_start*TO8_CHAR_SIZE*2, j*TO8_CHAR_SIZE*2, (i-cell_start)*TO8_CHAR_SIZE*2, TO8_CHAR_SIZE*2);
                }

            /* ligne suivante */
            dirty_cell_row += TO8_SCREEN_CW;
        }
    }
}


#define LED_SIZE 12


/* SetDiskLed:
 *  Allume/éteint la led du lecteur de disquettes.
 */
static void SetDiskLed(int led_on)
{
    if (led_on)
    {
	XGCValues values;

	values.foreground=xcolor[TO8_NCOLORS+2].pixel;
	XChangeGC(display, gc, GCForeground, &values);
	XDrawRectangle(display, screen_win, gc, TO8_SCREEN_W*2-LED_SIZE, 0, LED_SIZE-1, LED_SIZE-1);

	values.foreground=xcolor[TO8_NCOLORS+3].pixel;
	XChangeGC(display, gc, GCForeground, &values);
	XFillRectangle(display, screen_win, gc, TO8_SCREEN_W*2-LED_SIZE+1, 1, LED_SIZE-2, LED_SIZE-2);
    }
    else
	/* RetraceScreen() laisse parfois la led allumée... */
        dirty_cell[TO8_SCREEN_CW-1] = True;
}



/* InitGraphic:
 *  Sélectionne le visual, met en place la palette de couleurs et
 *  initialise le mécanisme de bufferisation (dirty rectangles).
 */
void InitGraphic(void)
{
    register int i;

    /* Recherche et sélection du visual */
    if (!XMatchVisualInfo(display, screen, 16, TrueColor, &visualinfo))
	if (!XMatchVisualInfo(display, screen, 24, TrueColor, &visualinfo))
	    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &visualinfo))
		if (!XMatchVisualInfo(display, screen, 8, PseudoColor, &visualinfo))
		{
		    fprintf(stderr,is_fr?"%s: aucun visual disponible.\n":"%s: no available visual.\n",PROG_NAME);
		    exit(EXIT_FAILURE);
		}

    /* Initialisation de la palette de couleurs */
    colormap=DefaultColormap(display, screen);
    gc=DefaultGC(display, screen);

    /* Couleurs de la page de réglage de la palette */
    xcolor[TO8_NCOLORS].red   = (TO8_PALETTE_COL1>>16)*0x101;
    xcolor[TO8_NCOLORS].green = ((TO8_PALETTE_COL1>>8)&0xFF)*0x101;
    xcolor[TO8_NCOLORS].blue  = (TO8_PALETTE_COL1&0xFF)*0x101;

    xcolor[TO8_NCOLORS+1].red   = (TO8_PALETTE_COL2>>16)*0x101;
    xcolor[TO8_NCOLORS+1].green = ((TO8_PALETTE_COL2>>8)&0xFF)*0x101;
    xcolor[TO8_NCOLORS+1].blue  = (TO8_PALETTE_COL2&0xFF)*0x101;

    /* Couleurs de la led du lecteur de disquettes */
    xcolor[TO8_NCOLORS+2].red   = 0;
    xcolor[TO8_NCOLORS+2].green = 0;
    xcolor[TO8_NCOLORS+2].blue  = 0;

    xcolor[TO8_NCOLORS+3].red   = 0;
    xcolor[TO8_NCOLORS+3].green = 0xFFFF;
    xcolor[TO8_NCOLORS+3].blue  = 0;

    if (visualinfo.class == TrueColor)
	for (i=0; i<4; i++)
	    XAllocColor(display, colormap, &xcolor[TO8_NCOLORS+i]);
    else if (visualinfo.class == PseudoColor)
    {
	long int pixels[TO8_NCOLORS+4];

	if (!XAllocColorCells(display, colormap, False, 0, 0, (long unsigned int *)pixels, TO8_NCOLORS+4))
	{
	    colormap=XCreateColormap(display, screen_win, visualinfo.visual, AllocNone);

	    if (colormap == DefaultColormap(display, screen))
	    {
		fprintf(stderr,is_fr?"%s: palette de couleurs immuable.\n":"%s: immutable color palette.\n",PROG_NAME);
		exit(EXIT_FAILURE);
	    }

	    XSetWindowColormap(display, screen_win, colormap);
	}

	for (i=0; i<TO8_NCOLORS+4; i++)
	{
	    xcolor[i].pixel=pixels[i];

	    if (i>=TO8_NCOLORS)
		xcolor[i].flags=DoRed|DoGreen|DoBlue;

	    XStoreColor(display, colormap, &xcolor[i]);
	}
    }

    /* Création du buffer d'affichage */
    if (mit_shm_enabled)
    {
	XShmSegmentInfo *shminfo = malloc(sizeof(XShmSegmentInfo));

        screen_buffer=XShmCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, NULL, shminfo, TO8_SCREEN_W*2, TO8_SCREEN_H*2);

	if (screen_buffer == NULL)
	    mit_shm_enabled = 0;
	else
	{
	    shminfo->shmid=shmget(IPC_PRIVATE, screen_buffer->height * screen_buffer->bytes_per_line, IPC_CREAT | 0777);

	    if (shminfo->shmid<0)
	    {
		XDestroyImage(screen_buffer);
		mit_shm_enabled = 0;
	    }
	    else
	    {
		shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);

		if (shminfo->shmaddr == (char *) -1)
		{
		    XDestroyImage(screen_buffer);
		    mit_shm_enabled = 0;
		}
		else
		{
		    shminfo->readOnly = False;
		    XShmAttach(display, shminfo);
		    shmctl(shminfo->shmid, IPC_RMID, 0);
		    screen_buffer->data = shminfo->shmaddr;
		    XSync(display, False);

		    /* enfin ... */
		    printf(is_fr?"Extension MIT-SHM utilisÃ©e.\n":"MIT-SHM extention used.\n");
		}
	    }
	}
    }

    if (!mit_shm_enabled)
    {
	screen_buffer = XCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, 0, NULL, TO8_SCREEN_W*2, TO8_SCREEN_H*2, 32, 0);
	screen_buffer->data = malloc(screen_buffer->height * screen_buffer->bytes_per_line);
    }

#ifdef DEBUG
    fprintf(stderr, "buffer:  width=%d, height=%d\n", screen_buffer->width, screen_buffer->height);
    fprintf(stderr, "         format=%s\n", screen_buffer->format==XYBitmap ? "XYBitmap" : (screen_buffer->format==XYPixmap ? "XYPixmap" : "ZPixmap"));
    fprintf(stderr, "         byte_order=%s\n", screen_buffer->byte_order==LSBFirst ? "LSBFirst" : "MSBFirst");
    fprintf(stderr, "         bitmap_unit=%d\n", screen_buffer->bitmap_unit);
    fprintf(stderr, "         bitmap_bit_order=%s\n", screen_buffer->bitmap_bit_order==LSBFirst ? "LSBFirst" : "MSBFirst");
    fprintf(stderr, "         bitmap_pad=%d\n", screen_buffer->bitmap_pad);
    fprintf(stderr, "         depth=%d\n", screen_buffer->depth);
    fprintf(stderr, "         bytes_per_lines=%d\n", screen_buffer->bytes_per_line);
    fprintf(stderr, "         bits_per_pixel=%d\n",  screen_buffer->bits_per_pixel);
    fprintf(stderr, "          red_mask=%lx\n",  screen_buffer->red_mask);
    fprintf(stderr, "          green_mask=%lx\n",  screen_buffer->green_mask);
    fprintf(stderr, "          blue_mask=%lx\n",  screen_buffer->blue_mask);
#endif

    gpl_buffer = XCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, 0, NULL, TO8_GPL_SIZE*2, 1, 32, 0);
    gpl_buffer->data = malloc(gpl_buffer->bytes_per_line);

    dirty_cell = calloc(TO8_SCREEN_W*TO8_SCREEN_H, sizeof(int));
    pixel_size = (screen_buffer->bits_per_pixel+1)/8;

    to8_SetColor=SetColor;
    to8_SetBorderColor=SetBorderColor;
    to8_DrawGPL=DrawGPL;
    to8_DrawBorderLine=DrawBorderLine;
    to8_SetDiskLed=SetDiskLed;
}

