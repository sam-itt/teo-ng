#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>

#include "teo.h"
#include "std.h"
#include "gfx.h"
#include "sdl2/sfront.h"
#include "sdl2/teo-sdl-vkbd.h"
#include "sdl2/gui/sdlgui.h"

#ifdef PLATFORM_OGXBOX
#include "hal/video.h"
#endif

static SDL_Surface *leds[2] = {NULL, NULL};
static SDL_Rect led_location;

static SDL_Window *window = NULL;
static SDL_Surface *screenSurface = NULL;
static SDL_Surface *screen_buffer = NULL;
static SDL_Surface *interlace_buffer = NULL;
static SDL_Surface *gpl_buffer = NULL;
static int border_color;
static int pixel_size;

static int *dirty_cell;

static Uint32 palette[TEO_NCOLORS+1];
static const struct SCREEN_PARAMS *tcol;
static Uint8 scaledBlit = 0;
static double scaleXFactor = 0;
static double scaleYFactor = 0;

static int installed_pointer = TEO_STATUS_MOUSE;

static SDL_Cursor *lightpen_cursor = NULL;
static SDL_Cursor *system_cursor = NULL;
static char scr80_pen_pointer_data[]={ 0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,0,
                                       1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,
                                       1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


#define SCR40_SIZE   8
#define SCR80_SIZE  16
#define SCR80_HOT_Y 7
#define SCR80_HOT_X 7

void printSDLErrorAndReboot(void);
//Screen dimension constants
const extern int SCREEN_WIDTH;
const extern int SCREEN_HEIGHT;



/* parametres d'affichage */
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

static const struct SCREEN_PARAMS tcol1={ TEO_WINDOW_W*2,
                                          TEO_WINDOW_H*2,
                                          0,
                                          0, 
                                          TEO_WINDOW_CW,
                                          TEO_WINDOW_CH,
                                          0,
                                          0 },  /* sans pourtour */

                                  tcol2={ TEO_SCREEN_W*2,
                                          TEO_SCREEN_H*2,
                                          TEO_BORDER_W*2,
                                          TEO_BORDER_H*2,
                                          TEO_SCREEN_CW,
                                          TEO_SCREEN_CH,
                                          TEO_BORDER_CW,
                                          TEO_BORDER_CH };  /* avec pourtour */


int *lookupX1=NULL;
int *lookupY1=NULL;
int *lookupX2=NULL;
int *lookupY2=NULL;

void teoSDL_GfxResizeLookup(int x,int y) {
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

    x_fact=(double)x/(double)(tcol->screen_w);
    y_fact=(double)y/(double)(tcol->screen_h);

    for (x_dest=0;x_dest<x;x_dest++) {
	lookupX1[x_dest]=(int)(x_fact*(double)x_dest+0.5);
	lookupX2[x_dest]=(int)((double)(x_dest+0.5)/x_fact);
    }

    for (y_dest=0;y_dest<y;y_dest++) {
	lookupY1[y_dest]=(int)(y_fact*(double)y_dest+0.5);
	lookupY2[y_dest]=(int)((double)(y_dest+0.5)/y_fact);
    }

#ifdef DEBUG
    fprintf(stderr,"recalc OK\n");
#endif
}



/* SetColor:
 *  Convertit la couleur du format TO8 au format 16-bit
 *  et la depose dans la palette.
 */
static void teoSDL_GfxSetColor(int index, int r, int g, int b)
{
    /*TODO: Experiment with 24-bits surfaces*/
//    palette[index] = SDL_MapRGB(screen_buffer->format, r, g, b);
    palette[index] = SDL_MapRGBA(screen_buffer->format, r, g, b, 255);
}


static void dump_sdl_rect(SDL_Rect *r)
{
    printf("%p: (x,y) = (%d,%d), size (w x h) =  (%dx%d)\n",r,r->x,r->y,r->w,r->h);
}

/*RECT makes a SDL_Rect from a set of up-left and bottom-right coordinates
 * SDL_Rect has (x,y) for the top-left point and  height + width
 * height and width must be computed as x2-x1 +1 because coordinates are zero
 * based: a 480 width means pixels between 0 and 479
 *
 * SDL_FillRect does treat width in a strict way, it doesn't include the 
 * width-ieth pixel (code does while(h--){} 
 * */
#define RECT(x1, y1, x2, y2) (SDL_Rect){(x1), (y1), (((x2) - (x1)) + 1), (((y2) - (y1)) + 1)}

/* SetBorderColor:
 *  Change la couleur du pourtour de l'ecran.
 *  (seulement pour l'ecran tcol2)
 */
static void teoSDL_GfxSetBorderColor(int mode, int color)
{
    register int i,j;
    int *dirty_cell_row = dirty_cell;
    SDL_Rect rects[4];

    /* on dessine dans le screen buffer */
    if (mode == TEO_PALETTE)
    {
        printf("Doing border as mode == TEO_PALETTE\n");
        border_color = TEO_NCOLORS;  /* couleur fixe de l'ecran de la palette */

        rects[0] = RECT(tcol2.border_w, 0, tcol2.screen_w-tcol2.border_w-1, tcol2.border_h-1);
        rects[1] = RECT(0, 0, tcol2.border_w-1, tcol2.border_h+(TEO_PALETTE_ADDR/TEO_WINDOW_GW)*2-1);
        rects[2] = RECT(tcol2.screen_w-tcol2.border_w, 0, tcol2.screen_w-1, tcol2.border_h+(TEO_PALETTE_ADDR/TEO_WINDOW_GW)*2-1);
        SDL_FillRects(screen_buffer, rects, 3, palette[border_color]);

        border_color = color;
        rects[0] = RECT(0, tcol2.border_h+(TEO_PALETTE_ADDR/TEO_WINDOW_GW)*2, tcol2.border_w-1, tcol2.screen_h-1);
        rects[1] = RECT(tcol2.screen_w-tcol2.border_w, tcol2.border_h+(TEO_PALETTE_ADDR/TEO_WINDOW_GW)*2, tcol2.screen_w-1, tcol2.screen_h-1);
        rects[2] = RECT(tcol2.border_w, tcol2.screen_h-tcol2.border_h, tcol2.screen_w-tcol2.border_w-1, tcol2.screen_h-1);
        SDL_FillRects(screen_buffer, rects, 3, palette[border_color]);
    }
    else 
    {
        border_color = color;

        rects[0] = RECT(tcol2.border_w, 0, tcol2.screen_w-tcol2.border_w-1, tcol2.border_w-1);
        rects[1] = RECT(tcol2.border_w, tcol2.screen_h-tcol2.border_h, tcol2.screen_w-tcol2.border_w-1, tcol2.screen_h-1);
        rects[2] = RECT(0, 0, tcol2.border_w-1, tcol2.screen_h-1);
        rects[3] = RECT(tcol2.screen_w-tcol2.border_w, 0, tcol2.screen_w-1, tcol2.screen_h-1);

        SDL_FillRects(screen_buffer, rects, 4, palette[border_color]);
    }

//DIRTYCELLS
    /* on coche les dirty cells */
    for (j=0; j<tcol2.screen_ch; j++)
    {
        for (i=0; i<tcol2.screen_cw; i++)
            if ( (i<tcol2.border_cw) || (i>=tcol2.screen_cw-tcol2.border_cw) ||
                 (j<tcol2.border_ch) || (j>=tcol2.screen_ch-tcol2.border_ch) )
                dirty_cell_row[i] = 1;
            
        /* ligne suivante */
        dirty_cell_row += tcol2.screen_cw;
    }
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
static inline int gpl_need_update(const Uint8 *gpl1, const Uint8 *gpl2)
{
    register int i = TEO_GPL_SIZE*2*pixel_size;

    while (i--)
        if (*gpl1++ != *gpl2++)
            return 1;

    return 0;
}



/* DrawGPL:
 *  Affiche un Groupe Point Ligne (un octet de VRAM).
 */
static void teoSDL_GfxDrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int x, y;
    Uint32 c1,c2;
    int *dirty_cell_row;
    unsigned char *gpl_src, *gpl_dest;

    SDL_LockSurface(gpl_buffer);

    switch (mode)
    {
        case TEO_BITMAP4: /* mode bitmap 4 couleurs */
            pt<<=1;

            for (i=0; i<8; i++)
            {
                c1 = palette[((pt>>(7-i))&2)+((col>>(7-i))&1)];
                PUT2PIXEL(i, c1);
            }
            break;

        case TEO_PAGE1: /* mode commutation page 1 */
            for (i=0; i<8; i++)
            {
                c1 = palette[(0x80>>i)&pt ? 1 : 0];
                PUT2PIXEL(i, c1);
            }
            break;

        case TEO_PAGE2: /* mode commutation page 2 */
            for (i=0; i<8; i++)
            {
                c1 = palette[(0x80>>i)&pt ? 2 : 0];
                PUT2PIXEL(i, c1);
            }
            break;

        case TEO_STACK2: /* mode superposition 2 pages */
            for (i=0; i<8; i++)
            {
                c1= palette[(0x80>>i)&pt ? 1 : ((0x80>>i)&col ? 2 : 0)];
                PUT2PIXEL(i, c1);
            }
            break;

        case TEO_COL80: /* mode 80 colonnes */
            for (i=0; i<8; i++)
            {
                putpixel(gpl_buffer, i,   0, palette[(0x80>>i)&pt  ? 1 : 0]);
                putpixel(gpl_buffer, i+8, 0, palette[(0x80>>i)&col ? 1 : 0]);
            }   
            break;

        case TEO_STACK4: /* mode superposition 4 pages */
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

        case TEO_BITMAP4b: /* mode bitmap 4 non documenté */
            for (i=0; i<4; i++)
            {
                c1 = palette[((pt>>(6-(i<<1)))&3)];
                PUT2PIXEL(i, c1);

                c2 = palette[((col>>(6-(i<<1)))&3)];
                PUT2PIXEL(i+4, c2);
            }
            break;

        case TEO_BITMAP16: /* mode bitmap 16 couleurs */
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

        case TEO_PALETTE: /* mode écran de la palette */
            if (addr<TEO_PALETTE_ADDR)
            {
                if ((col&0x78)==0x30)
                {
                    c1= SDL_MapRGBA(screen_buffer->format, TEO_PALETTE_COL1>>16, (TEO_PALETTE_COL1>>8)&0xFF, TEO_PALETTE_COL1&0xFF, 255);
                    c2= SDL_MapRGBA(screen_buffer->format, TEO_PALETTE_COL2>>16, (TEO_PALETTE_COL2>>8)&0xFF, TEO_PALETTE_COL2&0xFF, 255);
                }
                else
                {
                    c2= SDL_MapRGBA(screen_buffer->format, TEO_PALETTE_COL1>>16, (TEO_PALETTE_COL1>>8)&0xFF, TEO_PALETTE_COL1&0xFF, 255);
                    c1= SDL_MapRGBA(screen_buffer->format, TEO_PALETTE_COL2>>16, (TEO_PALETTE_COL2>>8)&0xFF, TEO_PALETTE_COL2&0xFF, 255);
                }

                for (i=0; i<8; i++)
                {
                    col = (0x80>>i)&pt ? c1 : c2;
                    PUT2PIXEL(i, col);
                }
                break;
            }
            /* no break */

        case TEO_COL40: /* mode 40 colonnes 16 couleurs */
        default:
            c1 = palette[((col>>3)&7)+(((~col)&0x40)>>3)];
            c2 = palette[(col&7)+(((~col)&0x80)>>4)];
 
            for (i=0; i<8; i++)
            {
                col = (0x80>>i)&pt ? c1 : c2;
                PUT2PIXEL(i, col);
            }
    } /* end of switch */

    x = tcol->border_w + (addr%TEO_WINDOW_GW)*TEO_GPL_SIZE*2;
    y = tcol->border_h + (addr/TEO_WINDOW_GW)*2;

    SDL_LockSurface(screen_buffer);

    gpl_src  = (Uint8 *)gpl_buffer->pixels;
    gpl_dest = (Uint8 *)screen_buffer->pixels + (y * screen_buffer->pitch) + (x * screen_buffer->format->BytesPerPixel);
    if (gpl_need_update(gpl_src, gpl_dest))
    {
        /* duplication des pixels */
        memcpy(gpl_dest, gpl_src, TEO_GPL_SIZE*2*pixel_size);
//        gpl_dest = (Uint8 *)screen_buffer->pixels + ((y+1) * screen_buffer->pitch) + (x * screen_buffer->format->BytesPerPixel);
        gpl_dest +=  screen_buffer->pitch;
        memcpy(gpl_dest, gpl_src, TEO_GPL_SIZE*2*pixel_size);

        /* dirty rectangles */
        x = tcol->border_cw + (addr%TEO_WINDOW_CW);
        y = tcol->border_ch + addr/(TEO_WINDOW_CW*TEO_CHAR_SIZE);
        dirty_cell_row = dirty_cell + y*tcol->screen_cw;
        dirty_cell_row[x] = 1;
    }

    SDL_UnlockSurface(gpl_buffer);
    SDL_UnlockSurface(screen_buffer);
}

/* DrawBorderLine:
 *  Affiche une ligne de pixels de la frontiere de l'ecran.
 *  (seulement pour l'ecran tcol2)
 */
static void teoSDL_GfxDrawBorderLine(int col, int line)
{
    int *dirty_cell_row = dirty_cell + (line/TEO_CHAR_SIZE)*tcol2.screen_cw;
    SDL_Rect rect;

    /*border_color is a global that is set by SetBorderColor*/

    if (col&TEO_LEFT_BORDER)
    {
        if (getpixel(screen_buffer, 0, line*2) != palette[border_color])
        {
            rect = RECT(0, line*2, tcol2.border_w-1, line*2+1);
            SDL_FillRect(screen_buffer, &rect, palette[border_color]);
        
            dirty_cell_row[0] = 1;
            dirty_cell_row[1] = 1;
        }
    } 
    else if (col&TEO_RIGHT_BORDER)
    {
        if (getpixel(screen_buffer, tcol2.screen_w-1, line*2) != palette[border_color])
        {
            rect = RECT(tcol2.screen_w-tcol2.border_w, line*2 , tcol2.screen_w-1, line*2+1);
            SDL_FillRect(screen_buffer, &rect, palette[border_color]);

            dirty_cell_row[tcol2.screen_cw-tcol2.border_cw]   = 1;
            dirty_cell_row[tcol2.screen_cw-tcol2.border_cw+1] = 1;
        } 
    }
    else if (getpixel(screen_buffer, tcol2.border_w+col*TEO_GPL_SIZE*2, line*2) != palette[border_color])
    {
        rect = RECT(tcol2.border_w+col*TEO_GPL_SIZE*2, line*2, tcol2.border_w+(col+1)*TEO_GPL_SIZE*2-1, line*2+1);
        SDL_FillRect(screen_buffer, &rect, palette[border_color]);
        
        dirty_cell_row[tcol2.border_cw+col] = 1;
    }
} 

/*TODO: Use renderer and SDL_RenderSetScale*/
void teoSDL_GfxRetraceWholeScreen()
{
    if(scaledBlit)
        SDL_BlitScaled(screen_buffer, &screen_buffer->clip_rect, screenSurface, &screenSurface->clip_rect);
    else
        SDL_BlitSurface(screen_buffer, NULL, screenSurface, NULL);
    SDL_UpdateWindowSurface(window);
}



/* RetraceScreen:
 *  Rafraichit une portion de l'ecran du TO8.
 */
void teo_sdl_RetraceScreen(int x, int y, int width, int height)
{
    SDL_Rect area, dst_area;

    area = (SDL_Rect){x, y, width, height};
    if(scaledBlit){
        dst_area.x = lookupX1[x];
        dst_area.y = lookupY1[y];
        dst_area.w = lookupX1[x+width]-lookupX1[x]+1;
        dst_area.h = lookupY1[y+height]-lookupY1[y]+1;

        SDL_BlitScaled(screen_buffer, &area, screenSurface, &dst_area);
    }else{
        SDL_BlitSurface(screen_buffer, &area, screenSurface, &area);
    }
}


/* RefreshScreen:
 *  Rafraichit l'ecran du TO8.
 */
void teoSDL_GfxRefreshScreen(void)
{
    register int i,j;
    int cell_start;
    int *dirty_cell_row = dirty_cell;
    int blend_done = 0;

    SDL_Rect area;
//    teoSDL_GfxRetraceWholeScreen();
//    return;


//    if (!graphic_mode)
//        return;

//    acquire_screen();
//    SDL_LockSurface(screenSurface);

    if (teo.setting.interlaced_video)
    {
        /* on groupe les dirty rectangles ligne par ligne */
        for (j=0; j<tcol->screen_ch; j++)
        {
            for (i=0; i<tcol->screen_cw; i++)
                if (dirty_cell_row[i])
                {
                    cell_start=i;
                    if(!blend_done) {
                        blend_done = 1;
                        SDL_SetSurfaceBlendMode(interlace_buffer, SDL_BLENDMODE_BLEND);
                        SDL_BlitSurface(screen_buffer, NULL, interlace_buffer, NULL);
                       // draw_trans_sprite(interlace_buffer(dst), screen_buffer(src), 0, 0);
                       // vsync(); // plus fluide ainsi?
                    }

                    while ((i<tcol->screen_cw) && dirty_cell_row[i])
                        dirty_cell_row[i++]<<=2;
                    
                    area = (SDL_Rect){cell_start*TEO_CHAR_SIZE*2, j*TEO_CHAR_SIZE*2, (i-cell_start)*TEO_CHAR_SIZE*2, TEO_CHAR_SIZE*2};

                    SDL_BlitSurface(interlace_buffer, &area, screenSurface, &area);
                    /*blit(interlace_buffer, screen,
                        cell_start*TEO_CHAR_SIZE*2, j*TEO_CHAR_SIZE*2,
                        cell_start*TEO_CHAR_SIZE*2, j*TEO_CHAR_SIZE*2,
                        (i-cell_start)*TEO_CHAR_SIZE*2, TEO_CHAR_SIZE*2);*/
                }

            /* ligne suivante */
            dirty_cell_row += tcol->screen_cw;
        }
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
                        dirty_cell_row[i++]=0;

                    teo_sdl_RetraceScreen(cell_start*TEO_CHAR_SIZE*2, j*TEO_CHAR_SIZE*2,
                                         (i-cell_start)*TEO_CHAR_SIZE*2, TEO_CHAR_SIZE*2);
                }

            /* ligne suivante */
            dirty_cell_row += tcol->screen_cw;
        }
    }

    if(sfront_show_vkbd){
        teoSDL_VKbdUpdate();
    }
    SDL_UpdateWindowSurface(window);
//    SDL_UnlockSurface(screenSurface);
}

/**
 * Load leds for the given resolution
 *
 */
static void teoSDL_GfxLoadLeds(int width, int height)
{
    char *fpath;
    char *ledfiles[] = {
        "led-on.bmp",
        "led-off.bmp"
    };

    if(leds[0] == NULL){
        for(int i = 0; i < 2; i++){
            fpath = std_GetTeoSystemFile(ledfiles[i]);
            if(!fpath){
                printf("Can't find mandatory file %s, bailing out\n",ledfiles[i]);
                exit(-1);
            }
            leds[i] = SDL_LoadBMP(fpath);
            if(!leds[i]){
                printf("Couldn't load %s, bailing out\n",ledfiles[i]);
                exit(-1);
            }
            SDL_SetColorKey(leds[i], SDL_TRUE, SDL_MapRGB(leds[i]->format, 0x0, 0x0, 0xff));
            SDL_SetSurfaceRLE(leds[i], 1);
            std_free(fpath);
        }
    }
    led_location.w = leds[0]->w;
    led_location.h = leds[0]->h;
    led_location.y = 0;
    led_location.x = width - led_location.w - 1;
}

/* SetDiskLed:
 *  Allume/éteint la Led du lecteur de disquettes.
 */
static void teoSDL_GfxSetDiskLed(int led_on)
{
//    if (!graphic_mode) return;
    if (led_on){
        SDL_BlitSurface(leds[0], NULL, screenSurface, &led_location);
    }else{
        SDL_BlitSurface(leds[1], NULL, screenSurface, &led_location);
        teo_sdl_RetraceScreen(led_location.x, led_location.y, led_location.w, led_location.h);
    }
}

static SDL_Cursor *teoSDL_GetLightPenCursor(void)
{
	int i, row, col, idx;
	Uint8 data[SCR80_SIZE*(SCR80_SIZE/2)];
	Uint8 mask[SCR80_SIZE*(SCR80_SIZE/2)];

	i = -1;
	for(row = 0; row < SCR80_SIZE; row++){
		for( col = 0; col < SCR80_SIZE; col++){
			idx = (row * SCR80_SIZE) + col;
			if(col % 8){
				data[i] <<= 1;
				mask[i] <<= 1;
			}else{
				++i;
				data[i] = mask[i] = 0;
			}
			switch(scr80_pen_pointer_data[idx]){
				case 1: //Black
				  data[i] |= 0x01;
				  mask[i] |= 0x01;
				  break;
				case 2: //White
				  mask[i] |= 0x01;
				  break;
				default: //transp
				  break;
			}
		}
	}
    return SDL_CreateCursor(data, mask, SCR80_SIZE, SCR80_SIZE, SCR80_HOT_X, SCR80_HOT_Y);
}



/**
 * Called from *within* the virtual TO8 when users selects
 * mouse vs lightpen *in the vm*:
 * "Settings and preferences (3)"/"Choose lightpen/mouse(2)"
 * This allow the frontend to reflect this change to the user
 * by changing the cursor
 */
static void teoSDL_GfxSetPointer(int pointer)
{
    SDL_Cursor *current;

    current = SDL_GetCursor();
    switch (pointer)
    {
        case TEO_STATUS_MOUSE :
            installed_pointer=TEO_STATUS_MOUSE;
            if(current != system_cursor)
                SDL_SetCursor(system_cursor);
            SDL_ShowCursor(SDL_DISABLE);
            break;

        case TEO_STATUS_LIGHTPEN :
            installed_pointer=TEO_STATUS_LIGHTPEN;
            SDL_SetCursor(lightpen_cursor);
            SDL_ShowCursor(SDL_ENABLE);
            break;
    }
}

int teoSDL_GfxGetPointer(void)
{
    return installed_pointer;
}




void teoSDL_GfxReset()
{
    screenSurface = SDL_GetWindowSurface(window);
    SDLGui_SetWindow(window);
    teoSDL_VKbdSetWindow(window);
    teoSDL_GfxLoadLeds(screenSurface->w, screenSurface->h);

    printf("screenSurface is now: %dx%d\n",screenSurface->w,screenSurface->h);
 
    scaledBlit = 0;
    if( screenSurface->w != tcol->screen_w || screenSurface->h != tcol->screen_h){
        scaledBlit = 1;
        scaleXFactor = (screenSurface->w*1.0)/(tcol->screen_w);
        scaleYFactor = (screenSurface->h*1.0)/(tcol->screen_h);
        printf("Scaling. Using factors: X=%0.2f and Y=%0.2f\n",scaleXFactor,scaleYFactor);

        teoSDL_GfxResizeLookup(screenSurface->w, screenSurface->h);
    }

    memset(dirty_cell, 1, (tcol->screen_cw*tcol->screen_ch)*sizeof(int));

    teoSDL_GfxRetraceWholeScreen();

}


static void teoSDL_InitScreenParams(void)
{
    int border_support = 1;

    if (border_support){
        tcol = &tcol2;
    }else{
        tcol = &tcol1;
//        teo_DrawBorderLine = NULL;
//        teo_SetBorderColor = NULL;
    }
}


SDL_Window *teoSDL_GfxWindow(int windowed_mode, const char *w_title)
{
    teoSDL_InitScreenParams();
#ifdef PLATFORM_OGXBOX
    VIDEO_MODE vparams;

    vparams = XVideoGetMode();
    debugPrint(
        "Got video mode: w:%d, h:%d, bpp:%d, refresh:%d\n",
        vparams.width,
        vparams.height,
        vparams.bpp,
        vparams.refresh
    );

    debugPrint("Builtin dimensions are: %dx%d\n",SCREEN_WIDTH,SCREEN_HEIGHT);
//    Sleep(8000);

    window = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);
//        SDL_WINDOW_SHOWN|SDL_WINDOW_FULLSCREEN);

    if(window == NULL)
    {
        debugPrint( "Window could not be created!\n");
        SDL_VideoQuit();
        printSDLErrorAndReboot();
    }
//    XReboot();
    debugPrint("Window will be (w x h): (%d x %d), while TEO_SCREEN_{W,H}*2 are: (%d, %d)\n",SCREEN_WIDTH, SCREEN_HEIGHT, TEO_SCREEN_W*2,TEO_SCREEN_H*2);
//    Sleep(4000);
#else
    Uint32 flags;

    flags = SDL_WINDOW_SHOWN;
    if(!windowed_mode)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    window = SDL_CreateWindow(
                w_title,
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                tcol->screen_w, tcol->screen_h,
                flags
                );
    if (window == NULL){
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return NULL;
    }
    printf("Window will be (w x h): (%d x %d), while TEO_SCREEN_{W,H} are: (%d, %d)\n",tcol->screen_w,tcol->screen_h,TEO_SCREEN_W,TEO_SCREEN_H);
#endif

    screenSurface = SDL_GetWindowSurface(window);
    SDLGui_SetWindow(window);
    teoSDL_VKbdSetWindow(window);
    printf("screenSurface is: %dx%d\n",screenSurface->w,screenSurface->h);
    teoSDL_GfxLoadLeds(screenSurface->w, screenSurface->h);

    lightpen_cursor  = teoSDL_GetLightPenCursor();
    system_cursor = SDL_GetCursor();

    teo_SetPointer=teoSDL_GfxSetPointer;
    /* teo_SetPointer is NOT called during the virtual TO8 init.
     * it is only invoked when going through the virtual TO8 settings screen
     * TODO: Have a function init installed_pointer instead of the static assignmedn
     * AND call SDL_ShowCursor
     * */
    SDL_ShowCursor(SDL_DISABLE); 
    printf("ok\n");
    return window;
}

SDL_Window *teoSDL_GfxGetWindow()
{
    return window;
}

/**
 * Kind of misleading... GfxInit gets called after GfxWindow
 * GfxWindow creates the window while GfxInit inits the "binding"
 * with the emulator core and create memory buffers.
 * TODO: Make it clearer, swap names ? merge the functions ?
 */
void teoSDL_GfxInit()
{
    teoSDL_InitScreenParams();


    /*TODO: Experiment with 24-bits surfaces*/
    gpl_buffer = SDL_CreateRGBSurfaceWithFormat(0, TEO_GPL_SIZE*2, 1, 24, SDL_PIXELFORMAT_RGBA8888);
    screen_buffer = SDL_CreateRGBSurfaceWithFormat(0, tcol->screen_w, tcol->screen_h, 24, SDL_PIXELFORMAT_RGBA8888);
    SDL_FillRect(screen_buffer, NULL, 0x00000000);
    interlace_buffer = SDL_CreateRGBSurfaceWithFormat(0, tcol->screen_w, tcol->screen_h, 24, SDL_PIXELFORMAT_RGBA8888);
    SDL_FillRect(interlace_buffer, NULL, 0x00000000);


    dirty_cell = calloc(tcol->screen_cw*tcol->screen_ch, sizeof(int));
    //teoSDL_GfxSetColor(TEO_NCOLORS, TEO_PALETTE_COL1>>16, (TEO_PALETTE_COL1>>8)&0xFF, TEO_PALETTE_COL1&0xFF);
    palette[TEO_NCOLORS] = SDL_MapRGB(screen_buffer->format, TEO_PALETTE_COL1>>16, (TEO_PALETTE_COL1>>8)&0xFF, TEO_PALETTE_COL1&0xFF);
    pixel_size = screen_buffer->format->BytesPerPixel;
    /*TODO: Check whether or not using SDL_Surface integrated 
     * palette could be a good idea (or not)
     * 
     * The idea is to have a SDL_Surface that have the exact same pixel format as the TO8
     * and let SDL do format conversion when blitting to the screen instead of maintaing
     * a palette and transformation code from color to RGB in the emulator code
     * */

    teo_SetColor = teoSDL_GfxSetColor;
    teo_SetBorderColor = teoSDL_GfxSetBorderColor;
    teo_DrawGPL = teoSDL_GfxDrawGPL;
    teo_DrawBorderLine = teoSDL_GfxDrawBorderLine;
    teo_SetDiskLed = teoSDL_GfxSetDiskLed; /*Already done in teoSDL_GfxWindow()*/

}
