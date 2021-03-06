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
 *  Copyright (C) 1997-2018 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Fran�ois Mouret,
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
 *  Version    : 1.8.5
 *  Cr�� par   : Eric Botcazou octobre 1999
 *  Modifi� par: Eric Botcazou 24/10/2003
 *               Fran�ois Mouret 26/01/2010 08/2011 25/04/2012
 *               Samuel Devulder 07/2011
 *               Samuel Cuella 02/2020
 *
 *  Gestion de l'affichage du TO8.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

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

#include "teo.h"
#include "std.h"
#include "main.h"
#include "gettext.h"
#include "linux/display.h"


/* variables globales */
static XVisualInfo visualinfo;
static GC gc;
static Colormap colormap;

static int *dirty_cell=NULL;
static int border_color;
static XImage *gpl_buffer, *screen_buffer;
static XImage *screen_zoom=NULL;
static XColor xcolorBuf[4096];
static XColor xcolor[TEO_NCOLORS+4];
static int pixel_size;

static int l_stretchBlit=0;
static int l_screenPhysWidth=320;
static int l_screenPhysHeight=200;


static void BuildXColorBuffer(void) 
{
    static int gamma[16] = {
        0  , 100, 127, 147,
        163, 179, 191, 203,
        215, 223, 231, 239,
        243, 247, 251, 255
    };

    int r,g,b,index;
    for (r=0;r<16;r++)
    for (g=0;g<16;g++)
    for (b=0;b<16;b++) {
	index=(r<<8)|(g<<4)|b;
    	xcolorBuf[index].red   = 0x101*gamma[r];
    	xcolorBuf[index].green = 0x101*gamma[g];
    	xcolorBuf[index].blue  = 0x101*gamma[b];
	xcolorBuf[index].flags=DoRed|DoGreen|DoBlue;
        XAllocColor(display, colormap, &xcolorBuf[index]);
    }

}

/* SetColor:
 *  Change une couleur de la palette du TO8.
 */
static void SetColor(int index, int r, int g, int b)
{
    static int inv_gamma[256] = {
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 0-9
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 10-19
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 20-29
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 30-39
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 40-49
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 50-59
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 60-69
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 70-79
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 80-89
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 90-99
        1  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 100-109
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 110-119
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 2  , 0  , 0  , // 120-129
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 130-139
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 3  , 0  , 0  , // 140-149
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 150-159
        0  , 0  , 0  , 4  , 0    , 0  , 0  , 0  , 0  , 0  , // 160-169
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 5  , // 170-179
        0  , 0  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 180-189
        0  , 6  , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 0  , // 190-199
        0  , 0  , 0  , 7  , 0    , 0  , 0  , 0  , 0  , 0  , // 200-209
        0  , 0  , 0  , 0  , 0    , 8  , 0  , 0  , 0  , 0  , // 210-219
        0  , 0  , 0  , 9  , 0    , 0  , 0  , 0  , 0  , 0  , // 220-229
        0  , 10 , 0  , 0  , 0    , 0  , 0  , 0  , 0  , 11 , // 230-239
        0  , 0  , 0  , 12 , 0    , 0  , 0  , 13 , 0  , 0  , // 240-249
        0  , 14 , 0  , 0  , 0    , 15                       // 250-255
    };

	int i;
    if (visualinfo.class == TrueColor) {
	i=(inv_gamma[r]<<8)|(inv_gamma[g]<<4)|inv_gamma[b];
	memcpy(&xcolor[index],&xcolorBuf[i],sizeof(XColor));
        // XAllocColor(display, colormap, &xcolor[index]);
    }
    else if (visualinfo.class == PseudoColor)
    {
    xcolor[index].red   = 0x101*r;
    xcolor[index].green = 0x101*g;
    xcolor[index].blue  = 0x101*b;
	xcolor[index].flags=DoRed|DoGreen|DoBlue;
	XStoreColor(display, colormap, &xcolor[index]);
    }
}



/* RECTF:
 *  Helper pour dessiner un rectangle plein � l'�cran.
 */
static void RECTF(int x1, int y1, int x2, int y2)
{
    register int i,j;

    for (j=y1; j<=y2; j++)
	for (i=x1; i<=x2; i++)
	    XPutPixel(screen_buffer, i, j, xcolor[border_color].pixel);
}



/* SetBorderColor:
 *  Change la couleur du pourtour de l'�cran.
 */
static void SetBorderColor(int mode, int color)
{
    register int i, j;
    int *dirty_cell_row = dirty_cell;

    /* on dessine dans le screen buffer */
    if (mode == TEO_PALETTE)
    {
        border_color = TEO_NCOLORS;  /* couleur fixe de l'�cran de la palette */
        RECTF(TEO_BORDER_W*2, 0, (TEO_SCREEN_W-TEO_BORDER_W)*2-1, TEO_BORDER_H*2-1);
        RECTF(0, 0, TEO_BORDER_W*2-1, (TEO_BORDER_H+(TEO_PALETTE_ADDR/TEO_WINDOW_GW))*2-1);
        RECTF((TEO_SCREEN_W-TEO_BORDER_W)*2, 0, TEO_SCREEN_W*2-1, (TEO_BORDER_H+(TEO_PALETTE_ADDR/TEO_WINDOW_GW))*2-1);

	border_color = color;
        RECTF(0, (TEO_BORDER_H+(TEO_PALETTE_ADDR/TEO_WINDOW_GW))*2, TEO_BORDER_W*2-1, TEO_SCREEN_H*2-1);
        RECTF((TEO_SCREEN_W-TEO_BORDER_W)*2, (TEO_BORDER_H+(TEO_PALETTE_ADDR/TEO_WINDOW_GW))*2, TEO_SCREEN_W*2-1, TEO_SCREEN_H*2-1);
        RECTF(TEO_BORDER_W*2, (TEO_SCREEN_H-TEO_BORDER_H)*2, (TEO_SCREEN_W-TEO_BORDER_W)*2-1, TEO_SCREEN_H*2-1);
    }
    else
    {
        border_color = color;
        RECTF(TEO_BORDER_W*2, 0, (TEO_SCREEN_W-TEO_BORDER_W)*2-1, TEO_BORDER_W*2-1);
        RECTF(TEO_BORDER_W*2, (TEO_SCREEN_H-TEO_BORDER_H)*2, (TEO_SCREEN_W-TEO_BORDER_W)*2-1, TEO_SCREEN_H*2-1);
        RECTF(0, 0, TEO_BORDER_W*2-1, TEO_SCREEN_H*2-1);
        RECTF((TEO_SCREEN_W-TEO_BORDER_W)*2, 0, TEO_SCREEN_W*2-1, TEO_SCREEN_H*2-1);
    }

    /* on coche les dirty cells */
    for (j=0; j<TEO_SCREEN_CH; j++)
    {
        for (i=0; i<TEO_SCREEN_CW; i++)
            if ( (i<TEO_BORDER_CW) || (i>=TEO_SCREEN_CW-TEO_BORDER_CW) ||
                 (j<TEO_BORDER_CH) || (j>=TEO_SCREEN_CH-TEO_BORDER_CH) )
                dirty_cell_row[i] = True ;

        /* ligne suivante */
        dirty_cell_row += TEO_SCREEN_CW;
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
    register int i = TEO_GPL_SIZE*2*pixel_size;

    while (i--)
        if (*gpl1++ != *gpl2++)
            return 1;

    return 0;
}



/* ugraphic_DrawGPL:
 *  Affiche un Groupe Point Ligne (un octet de VRAM).
 */
static void ugraphic_DrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int x, y, c1, c2;
             int *dirty_cell_row;
    char *gpl_src, *gpl_dest;

    switch (mode)
    {
	 case TEO_BITMAP4: /* mode bitmap 4 couleurs */
	    pt<<=1;

	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[((pt>>(7-i))&2)+((col>>(7-i))&1)].pixel;
		PUT2PIXEL(i, c1);
	    }
            break;

        case TEO_PAGE1: /* mode commutation page 1 */
	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[(0x80>>i)&pt ? 1 : 0].pixel;
		PUT2PIXEL(i, c1);
	    }
	    break;

        case TEO_PAGE2: /* mode commutation page 2 */
	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[(0x80>>i)&col ? 2 : 0].pixel;
		PUT2PIXEL(i, c1);
	    }
	    break;

        case TEO_STACK2: /* mode superposition 2 pages */
	    for (i=0; i<8; i++)
	    {
		c1 = xcolor[(0x80>>i)&pt ? 1 : ((0x80>>i)&col ? 2 : 0)].pixel;
		PUT2PIXEL(i, c1);
	    }
	    break;

        case TEO_COL80: /* mode 80 colonnes */
	    for (i=0; i<8; i++)
	    {
		XPutPixel(gpl_buffer, i,   0, xcolor[(0x80>>i)&pt  ? 1 : 0].pixel);
		XPutPixel(gpl_buffer, i+8, 0, xcolor[(0x80>>i)&col ? 1 : 0].pixel);
	    }
	    break;

	case TEO_STACK4: /* mode superposition 4 pages */
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

        case TEO_BITMAP4b: /* mode bitmap 4 non document� */
	    for (i=0; i<4; i++)
	    {
		c1 = xcolor[( pt>>(6-(i<<1)) )&3].pixel;
		PUT2PIXEL(i, c1);

		c2 = xcolor[(col>>(6-(i<<1)) )&3].pixel;
		PUT2PIXEL(i+4, c2);
	    }
	    break;

        case TEO_BITMAP16: /* mode bitmap 16 couleurs */
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

        case TEO_PALETTE: /* mode �cran de la palette */
	    if (addr<TEO_PALETTE_ADDR)
	    {
		if ((col&0x78)==0x30)
		{
		    c1=TEO_NCOLORS;
		    c2=TEO_NCOLORS+1;
		}
		else
		{
		    c1=TEO_NCOLORS+1;
		    c2=TEO_NCOLORS;
		}

		for (i=0; i<8; i++)
		{
		    col = xcolor[pt&(0x80>>i) ? c1 : c2].pixel;
		    PUT2PIXEL(i, col);
		}
		break;
	    }
	    /* no break */

        case TEO_COL40: /* mode 40 colonnes 16 couleurs */
        default:
	    c1=((col>>3)&7)+(((~col)&0x40)>>3);
	    c2=(col&7)+(((~col)&0x80)>>4);

	    for (i=0; i<8; i++)
	    {
		col = xcolor[pt&(0x80>>i) ? c1 : c2].pixel;
		PUT2PIXEL(i, col);
	    }
    } /* end of switch */

    x = TEO_BORDER_W*2 + (addr%TEO_WINDOW_GW)*TEO_GPL_SIZE*2;
    y = TEO_BORDER_H*2 + (addr/TEO_WINDOW_GW)*2;

    gpl_src  = gpl_buffer->data;
    gpl_dest = screen_buffer->data + y*screen_buffer->bytes_per_line + x*pixel_size;

    if (gpl_need_update(gpl_src, gpl_dest))
    {
        /* duplication des pixels */
        memcpy(gpl_dest, gpl_src, TEO_GPL_SIZE*2*pixel_size);
        gpl_dest += screen_buffer->bytes_per_line;
        memcpy(gpl_dest, gpl_src, TEO_GPL_SIZE*2*pixel_size);

	/* dirty rectangles */
	x = TEO_BORDER_CW + (addr%TEO_WINDOW_CW);
	y = TEO_BORDER_CH + addr/(TEO_WINDOW_CW*TEO_CHAR_SIZE);
	dirty_cell_row = dirty_cell + y*TEO_SCREEN_CW;
	dirty_cell_row[x] = True;
	teo_new_video_params = TRUE;
    }
}



/* DrawBorderLine:
 *  Affiche une ligne de pixels de la fronti�re de l'�cran.
 */
static void DrawBorderLine(int col, int line)
{
    int *dirty_cell_row = dirty_cell + (line/TEO_CHAR_SIZE)*TEO_SCREEN_CW;

    if (col&TEO_LEFT_BORDER)
    {
	if (XGetPixel(screen_buffer, 0, line*2) != xcolor[border_color].pixel)
        {
            RECTF(0, line*2, TEO_BORDER_W*2-1, line*2+1);

	    dirty_cell_row[0] = True;
	    dirty_cell_row[1] = True;
	}
    }
    else if (col&TEO_RIGHT_BORDER)
    {
	if (XGetPixel(screen_buffer, TEO_SCREEN_W*2-1, line*2) != xcolor[border_color].pixel)
        {
            RECTF((TEO_SCREEN_W-TEO_BORDER_W)*2, line*2 , TEO_SCREEN_W*2-1, line*2+1);

	    dirty_cell_row[TEO_SCREEN_CW-TEO_BORDER_CW] = True;
	    dirty_cell_row[TEO_SCREEN_CW-TEO_BORDER_CW+1] = True;
	}
    }
    else if (XGetPixel(screen_buffer, (TEO_BORDER_W+col*TEO_GPL_SIZE)*2, line*2) != xcolor[border_color].pixel)
    {
        RECTF((TEO_BORDER_W+col*TEO_GPL_SIZE)*2, line*2, (TEO_BORDER_W+(col+1)*TEO_GPL_SIZE)*2-1, line*2+1);

        dirty_cell_row[TEO_BORDER_CW+col] = True;
    }
}



/* SetDiskLed:
 *  Allume/�teint la led du lecteur de disquettes.
 */
#define LED_SIZE 12
static void SetDiskLed(int led_on)
{
    static int count = 0;
	XGCValues values;

    if (led_on)
    {
	    values.foreground=xcolor[TEO_NCOLORS+2].pixel;
        XChangeGC(display, gc, GCForeground, &values);
	    XDrawRectangle(display, screen_win, gc,
	                   TEO_SCREEN_W*2-LED_SIZE-4,
	                   2,
	                   LED_SIZE,
	                   LED_SIZE);

	    values.foreground=xcolor[TEO_NCOLORS+3].pixel;
	    XChangeGC(display, gc, GCForeground, &values);
	    XFillRectangle(display, screen_win, gc,
	                   TEO_SCREEN_W*2-LED_SIZE-3,
	                   3,
	                   LED_SIZE-1,
	                   LED_SIZE-1);

	    values.foreground=xcolor[TEO_NCOLORS+2].pixel;
        XChangeGC(display, gc, GCForeground, &values);
	    XDrawRectangle(display, screen_win, gc,
	                   TEO_SCREEN_W*2-LED_SIZE-3+count,
	                   3+count,
	                   LED_SIZE-2-(count*2),
	                   LED_SIZE-2-(count*2));

        count = (count+1)%(LED_SIZE/2);

        dirty_cell[TEO_SCREEN_CW-1] = False;
    }
    else
	/* ugraphic_Retrace() laisse parfois la led allum�e... */
        dirty_cell[TEO_SCREEN_CW-1] = True;
}


/* ------------------------------------------------------------------------- */



int *lookupX1=NULL;
int *lookupY1=NULL;
int *lookupX2=NULL;
int *lookupY2=NULL;

void ugraphic_resize_lookup(int x,int y) {
    int x_dest,y_dest;
    double x_fact,y_fact;

    if (lookupX1!=NULL) {
        free(lookupX1);
    }
    lookupX1=(int*)malloc(sizeof(int)*x);

    if (lookupX2!=NULL) {
        free(lookupX2);
    }
    lookupX2=(int*)malloc(sizeof(int)*x);

    if (lookupY1!=NULL) {
        free(lookupY1);
    }
    lookupY1=(int*)malloc(sizeof(int)*y);

    if (lookupY2!=NULL) {
        free(lookupY2);
    }
    lookupY2=(int*)malloc(sizeof(int)*y);

    x_fact=(double)x/(double)(TEO_SCREEN_W*2);
    y_fact=(double)y/(double)(TEO_SCREEN_H*2);

    for (x_dest=0;x_dest<x;x_dest++) {
	lookupX1[x_dest]=(int)(x_fact*(double)x_dest+0.5);
	lookupX2[x_dest]=(int)((double)(x_dest+0.5)/x_fact);
    }

    for (y_dest=0;y_dest<y;y_dest++) {
	lookupY1[y_dest]=(int)(y_fact*(double)y_dest+0.5);
	lookupY2[y_dest]=(int)((double)(y_dest+0.5)/y_fact);
    }

#ifdef DEBUG
    log_msgf(LOG_TRACE,"recalc OK\n");
#endif
}


/* ugraphic_resize_zoom:
 *  Changement de taille de l'�cran (resize window en provenance de l'event GTK)
 */
void ugraphic_resize_zoom() {
    int ret,i;
    static int last_width=1;
    static int last_height=1;
  

    XWindowAttributes attr;
    ret=XGetWindowAttributes(display,screen_win,&attr);
    if (ret==0) return; 
    /* seems strange... but Xlib windows is at correct size much sooner than gtk events on event fifo...*/
    if ( (attr.width != last_width) || (attr.height != last_height) ) {
        last_width=attr.width;
        last_height=attr.height;
#ifdef DEBUG
            log_msgf(LOG_TRACE,"XGetWindowAttributes width=%d height=%d changed\n",attr.width,attr.height);
#endif
            if ( ((TEO_SCREEN_W*2)==attr.width) && ((TEO_SCREEN_H*2)==attr.height) ) {
                l_stretchBlit=0;
            } else {
                l_stretchBlit=1;
                l_screenPhysWidth=attr.width;
                l_screenPhysHeight=attr.height;
		ugraphic_resize_lookup(l_screenPhysWidth,l_screenPhysHeight);

                /*TODO we may only expand if needed here */
		if (screen_zoom!=NULL) {
                    XDestroyImage(screen_zoom); 
                }
        	screen_zoom = XCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, 0, NULL, l_screenPhysWidth, l_screenPhysHeight, 32, 0);
	        screen_zoom->data = malloc(screen_zoom->height * screen_zoom->bytes_per_line);
	    }
	    if (dirty_cell!=NULL) {
                for(i=0;i<TEO_SCREEN_W*TEO_SCREEN_H;i++) {
                    dirty_cell[i]=True;
                }
            }
	}
}

/* ugraphic_slow_Retrace:
 *  Rafra�chit une portion de l'�cran du TO8 avec mise a l'echelle de le fenetre
 */

void ugraphic_slow_Retrace(int x, int y, int w, int h)
{
    int x_dest,y_dest;
    int w_dest,h_dest;
    int c;

    

    for (x_dest=lookupX1[x];x_dest<lookupX1[x+w];x_dest++) {
        for (y_dest=lookupY1[y];y_dest<lookupY1[y+h];y_dest++) {
            c=XGetPixel(screen_buffer,lookupX2[x_dest],lookupY2[y_dest]);
            XPutPixel(screen_zoom,x_dest,y_dest,c);
        }
    }
    w_dest=lookupX1[x+w]-lookupX1[x]+1;
    h_dest=lookupY1[y+h]-lookupY1[y]+1;

    XPutImage(display, screen_win, gc, screen_zoom, lookupX1[x], lookupY1[y], lookupX1[x], lookupY1[y], w_dest, h_dest);
    
}

/* ugraphic_Retrace:
 *  Rafra�chit une portion de l'�cran du TO8.
 */

void ugraphic_Retrace(int x, int y, int w, int h)
{
    if (l_stretchBlit==1) {
        ugraphic_slow_Retrace(x,y,w,h);
        return;
    }
    if (mit_shm_enabled)
	XShmPutImage(display, screen_win, gc, screen_buffer, x, y, x, y, w, h, True);
    else
	XPutImage(display, screen_win, gc, screen_buffer, x, y, x, y, w, h);
}



/* ugraphic_Refresh:
 *  Rafra�chit l'�cran du TO8.
 */
void ugraphic_Refresh(void)
{
    register int i,j;
    int cell_start;
    int *dirty_cell_row = dirty_cell;
    static int odd = 1;

    if (teo.setting.interlaced_video)
    {
        odd ^= 1;
        dirty_cell_row = (odd == 0) ? dirty_cell : dirty_cell + TEO_SCREEN_CW;
        /* on groupe les dirty rectangles ligne par ligne */
        for (j=odd; j<TEO_SCREEN_CH; j+=2)
        {
            for (i=0; i<TEO_SCREEN_CW; i++)
                if (dirty_cell_row[i])
	        {
                    cell_start=i;

                    while ((i<TEO_SCREEN_CW) && dirty_cell_row[i])
                        dirty_cell_row[i++]=False;

                    ugraphic_Retrace(cell_start*TEO_CHAR_SIZE*2, j*TEO_CHAR_SIZE*2, (i-cell_start)*TEO_CHAR_SIZE*2, TEO_CHAR_SIZE*2);
                }

            /* ligne suivante */
            dirty_cell_row += (TEO_SCREEN_CW<<1);
        }
    }
    else
    {
        /* on groupe les dirty rectangles ligne par ligne */
        for (j=0; j<TEO_SCREEN_CH; j++)
        {
            for (i=0; i<TEO_SCREEN_CW; i++)
                if (dirty_cell_row[i])
	        {
                    cell_start=i;

                    while ((i<TEO_SCREEN_CW) && dirty_cell_row[i])
                        dirty_cell_row[i++]=False;

                    ugraphic_Retrace(cell_start*TEO_CHAR_SIZE*2, j*TEO_CHAR_SIZE*2, (i-cell_start)*TEO_CHAR_SIZE*2, TEO_CHAR_SIZE*2);
                }

            /* ligne suivante */
            dirty_cell_row += TEO_SCREEN_CW;
        }
    }
}


/** ugraphic_Init:
 * Create and allocate palette, buffers and plug gfx-related
 * teo_ callbacks
 */
void ugraphic_Init(void)
{
    register int i;
    XShmSegmentInfo *shminfo;

    /* Recherche et s�lection du visual */
    if (!XMatchVisualInfo(display, screen, 16, TrueColor, &visualinfo))
	if (!XMatchVisualInfo(display, screen, 24, TrueColor, &visualinfo))
	    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &visualinfo))
		if (!XMatchVisualInfo(display, screen, 8, PseudoColor, &visualinfo))
		{
            main_ExitMessage(_("%s: no available visual.\n"),PROG_NAME);
		}

    /* Initialisation de la palette de couleurs */
    colormap=DefaultColormap(display, screen);
    gc=DefaultGC(display, screen);

    /* Couleurs de la page de r�glage de la palette */
    xcolor[TEO_NCOLORS].red   = (TEO_PALETTE_COL1>>16)*0x101;
    xcolor[TEO_NCOLORS].green = ((TEO_PALETTE_COL1>>8)&0xFF)*0x101;
    xcolor[TEO_NCOLORS].blue  = (TEO_PALETTE_COL1&0xFF)*0x101;

    xcolor[TEO_NCOLORS+1].red   = (TEO_PALETTE_COL2>>16)*0x101;
    xcolor[TEO_NCOLORS+1].green = ((TEO_PALETTE_COL2>>8)&0xFF)*0x101;
    xcolor[TEO_NCOLORS+1].blue  = (TEO_PALETTE_COL2&0xFF)*0x101;

    /* Couleurs de la led du lecteur de disquettes */
    xcolor[TEO_NCOLORS+2].red   = 0x8888;
    xcolor[TEO_NCOLORS+2].green = 0x8888;
    xcolor[TEO_NCOLORS+2].blue  = 0x8888;

    xcolor[TEO_NCOLORS+3].red   = 0;
    xcolor[TEO_NCOLORS+3].green = 0xFFFF;
    xcolor[TEO_NCOLORS+3].blue  = 0;

    if (visualinfo.class == TrueColor) {
	for (i=0; i<4; i++)
	    XAllocColor(display, colormap, &xcolor[TEO_NCOLORS+i]);

	BuildXColorBuffer();
	}
    else if (visualinfo.class == PseudoColor)
    {
	long int pixels[TEO_NCOLORS+4];

	if (!XAllocColorCells(display, colormap, False, 0, 0, (long unsigned int *)pixels, TEO_NCOLORS+4))
	{
	    colormap=XCreateColormap(display, screen_win, visualinfo.visual, AllocNone);

	    if (colormap == DefaultColormap(display, screen))
	    {
            main_ExitMessage(_("%s: immutable color palette.\n"),PROG_NAME);
	    }

	    XSetWindowColormap(display, screen_win, colormap);
	}

	for (i=0; i<TEO_NCOLORS+4; i++)
	{
	    xcolor[i].pixel=pixels[i];

	    if (i>=TEO_NCOLORS)
		xcolor[i].flags=DoRed|DoGreen|DoBlue;

	    XStoreColor(display, colormap, &xcolor[i]);
	}
    }

    /* Cr�ation du buffer d'affichage */
    if (mit_shm_enabled)
    {
	shminfo = malloc(sizeof(XShmSegmentInfo));

        screen_buffer=XShmCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, NULL, shminfo, TEO_SCREEN_W*2, TEO_SCREEN_H*2);

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
		    main_ConsoleOutput(_("MIT-SHM extention enabled.\n"));
		}
	    }
	}
    }

    if (!mit_shm_enabled)
    {
	screen_buffer = XCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, 0, NULL, TEO_SCREEN_W*2, TEO_SCREEN_H*2, 32, 0);
	screen_buffer->data = malloc(screen_buffer->height * screen_buffer->bytes_per_line);
    }

    /* check window size vs buffer (for retina screens) */
    {
        XWindowAttributes attr;
        int ret;
        ret=XGetWindowAttributes(display,screen_win,&attr);
#ifdef DEBUG
        log_msgf(LOG_TRACE,"XGetWindowAttributes ret=%d\n",ret);
#endif
        if (ret!=0) {
#ifdef DEBUG
            log_msgf(LOG_TRACE,"XGetWindowAttributes width=%d height=%d\n",attr.width,attr.height);
#endif
            if ( ((TEO_SCREEN_W*2)==attr.width) && ((TEO_SCREEN_H*2)==attr.height) ) {
                l_stretchBlit=0;
            } else {
                l_stretchBlit=1;
                l_screenPhysWidth=attr.width;
                l_screenPhysHeight=attr.height;
		ugraphic_resize_lookup(l_screenPhysWidth,l_screenPhysHeight);
        	screen_zoom = XCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, 0, NULL, l_screenPhysWidth, l_screenPhysHeight, 32, 0);
	        screen_zoom->data = malloc(screen_zoom->height * screen_zoom->bytes_per_line);
            }
        }
    }

#ifdef DEBUG
    log_msgf(LOG_TRACE, "buffer:  width=%d, height=%d\n", screen_buffer->width, screen_buffer->height);
    log_msgf(LOG_TRACE, "         format=%s\n", screen_buffer->format==XYBitmap ? "XYBitmap" : (screen_buffer->format==XYPixmap ? "XYPixmap" : "ZPixmap"));
    log_msgf(LOG_TRACE, "         byte_order=%s\n", screen_buffer->byte_order==LSBFirst ? "LSBFirst" : "MSBFirst");
    log_msgf(LOG_TRACE, "         bitmap_unit=%d\n", screen_buffer->bitmap_unit);
    log_msgf(LOG_TRACE, "         bitmap_bit_order=%s\n", screen_buffer->bitmap_bit_order==LSBFirst ? "LSBFirst" : "MSBFirst");
    log_msgf(LOG_TRACE, "         bitmap_pad=%d\n", screen_buffer->bitmap_pad);
    log_msgf(LOG_TRACE, "         depth=%d\n", screen_buffer->depth);
    log_msgf(LOG_TRACE, "         bytes_per_lines=%d\n", screen_buffer->bytes_per_line);
    log_msgf(LOG_TRACE, "         bits_per_pixel=%d\n",  screen_buffer->bits_per_pixel);
    log_msgf(LOG_TRACE, "          red_mask=%lx\n",  screen_buffer->red_mask);
    log_msgf(LOG_TRACE, "          green_mask=%lx\n",  screen_buffer->green_mask);
    log_msgf(LOG_TRACE, "          blue_mask=%lx\n",  screen_buffer->blue_mask);
#endif

    gpl_buffer = XCreateImage(display, visualinfo.visual, visualinfo.depth, ZPixmap, 0, NULL, TEO_GPL_SIZE*2, 1, 32, 0);
    gpl_buffer->data = malloc(gpl_buffer->bytes_per_line);

    dirty_cell = calloc(TEO_SCREEN_W*TEO_SCREEN_H, sizeof(int));
    pixel_size = (screen_buffer->bits_per_pixel+1)/8;

    teo_SetColor=SetColor;
    teo_SetBorderColor=SetBorderColor;
    teo_DrawGPL=ugraphic_DrawGPL;
    teo_DrawBorderLine=DrawBorderLine;
    teo_SetDiskLed=SetDiskLed;
}

