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



static SDL_Window *window = NULL;
static SDL_Surface *screenSurface = NULL;
static SDL_Surface *screen_buffer = NULL;
static Uint8 scaledBlit = 0;
static double scaleXFactor = 0;
static double scaleYFactor = 0;
static int pixel_size;

/* variables globales */
static int *dirty_cell;

static SDL_Surface *gpl_buffer, *screen_buffer;
static Uint32 palette[TEO_NCOLORS+1];
static int installed_pointer = TEO_STATUS_MOUSE;

static int *lookupX1=NULL;
static int *lookupY1=NULL;
static int *lookupX2=NULL;
static int *lookupY2=NULL;



/*static functions prototypes*/
static void teoSDL_Gfx40SetColor(int index, int r, int g, int b);
static void teoSDL_Gfx40SetDiskLed(int led_on);
void teoSDL_Gfx40RetraceScreen(int x, int y, int width, int height);
static void teoSDL_Gfx40DrawGPL(int mode, int addr, int pt, int col);
static void teoSDL_Gfx40SetPointer(int pointer);

//Screen dimension constants
const extern int SCREEN_WIDTH;
const extern int SCREEN_HEIGHT;


#if 0
#define PUT2PIXEL(val) *gpl_src++ = val; \
                       *gpl_src++ = val;
#elif 0
#define PUT2PIXEL(i, val) putpixel(gpl_buffer, 2*(i),   0, (val)); \
                          putpixel(gpl_buffer, 2*(i)+1, 0, (val))
#else
#define PUT2PIXEL(i, val) putpixel(gpl_buffer, 2*(i),   0, (val)); \
                          putpixel(gpl_buffer, 2*(i)+1, 0, (val))
#endif

/* gpl_need_update:
 *  Helper pour les dirty rectangles.
 */
static inline int gpl_need_update(const unsigned char *gpl1, const unsigned char *gpl2)
{
    register int i = TEO_GPL_SIZE*pixel_size;

    while (i--)
        if (*gpl1++ != *gpl2++)
            return 1;

    return 0;
}



/* DrawGPL:
 *  Affiche un Groupe Point Ligne (un octet de VRAM).
 */
static void teoSDL_Gfx40DrawGPL(int mode, int addr, int pt, int col)
{
    register int i;
    unsigned int c1, c2, x, y;
             int *dirty_cell_row;
    //unsigned char *gpl_src = gpl_buffer->line[0], *gpl_dest;
    unsigned char *gpl_src, *gpl_dest;
    Uint32 pixel;

    SDL_LockSurface(gpl_buffer);
    switch (mode)
    {
        case TEO_BITMAP4: /* mode bitmap 4 couleurs */
            pt<<=1;

            for (i=0; i<8; i++){
                pixel = palette[((pt>>(7-i))&2)+((col>>(7-i))&1)];
                putpixel(gpl_buffer, i, 0, pixel);
            }
            break;

        case TEO_PAGE1: /* mode commutation page 1 */
            for (i=0; i<8; i++){
                pixel = palette[(0x80>>i)&pt ? 1 : 0];
                putpixel(gpl_buffer, i, 0, pixel);
            }
            break;

        case TEO_PAGE2: /* mode commutation page 2 */
            for (i=0; i<8; i++){
                pixel = palette[(0x80>>i)&col ? 2 : 0];
                putpixel(gpl_buffer, i, 0, pixel);
            }
            break;

        case TEO_STACK2: /* mode superposition 2 pages */
            for (i=0; i<8; i++){
                pixel = palette[(0x80>>i)&pt ? 1 :
                                ((0x80>>i)&col ? 2 : 0)];
                putpixel(gpl_buffer, i, 0, pixel);
            }
            break;

        case TEO_COL80: /* mode 80 colonnes */
            for (i=0;i<4;i++){
                pixel = palette[( pt>>(6-(i<<1)) )&1];
                putpixel(gpl_buffer, i, 0, pixel);
            }

            for (i=0;i<4;i++){
                pixel = palette[(col>>(6-(i<<1)) )&1];
                putpixel(gpl_buffer, i, 0, pixel);
            }
            break;

        case TEO_STACK4: /* mode superposition 4 pages */
            for (i=0; i<4; i++)
            {
                c1 = palette[ (0x80>>i)&pt  ? 1 :
                           ((0x08>>i)&pt  ? 2 :
                           ((0x80>>i)&col ? 3 :
                           ((0x08>>i)&col ? 4 : 0)))];

                /* on modifie les pixels deux par deux */
                PUT2PIXEL(i, c1);
            }
            break;


        case TEO_BITMAP4b: /* mode bitmap 4 non documente */
            for (i=0;i<4;i++){
                pixel = palette[( pt>>(6-(i<<1)) )&3];
                putpixel(gpl_buffer, i, 0, pixel);
            }

            for (i=0;i<4;i++){
                pixel = palette[(col>>(6-(i<<1)) )&3];
                putpixel(gpl_buffer, i, 0, pixel);
            }

            break;

        case TEO_BITMAP16: /* mode bitmap 16 couleurs */
            /* on modifie les pixels deux par deux */
            c1=palette[(pt&0xF0)>>4];
            PUT2PIXEL(0, c1);

            c1=palette[pt&0xF];
            PUT2PIXEL(2,c1);

            c1=palette[(col&0xF0)>>4];
            PUT2PIXEL(4,c1);

            c1=palette[col&0xF];
            PUT2PIXEL(6,c1);
            break;

        case TEO_PALETTE: /* mode ecran de la palette */
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

                for (i=0; i<8; i++){
                    pixel = (0x80>>i)&pt ? c1 : c2;
                    putpixel(gpl_buffer, i, 0, pixel);
                }

                break;
            }
            /* no break */

        case TEO_COL40: /* mode 40 colonnes 16 couleurs */
        default:
            c1 = palette[((col>>3)&7)+(((~col)&0x40)>>3)];
            c2 = palette[(col&7)+(((~col)&0x80)>>4)];

            for (i=0; i<8; i++){
                pixel = (0x80>>i)&pt ? c1 : c2;
                putpixel(gpl_buffer, i, 0, pixel);
            }

    } /* end of switch */

    x=(addr%TEO_WINDOW_GW)*TEO_GPL_SIZE;
    y=(addr/TEO_WINDOW_GW);

    SDL_LockSurface(screen_buffer);

    gpl_src  = (Uint8 *)gpl_buffer->pixels;
    gpl_dest = (Uint8 *)screen_buffer->pixels + (y * screen_buffer->pitch) + (x * screen_buffer->format->BytesPerPixel);
    if (gpl_need_update(gpl_src, gpl_dest))
    {
        memcpy(gpl_dest, gpl_src, TEO_GPL_SIZE*pixel_size);

        /* dirty rectangles */
        x = addr%TEO_WINDOW_CW;
        y = addr/(TEO_WINDOW_CW*TEO_CHAR_SIZE);
        dirty_cell_row = dirty_cell + y*TEO_WINDOW_CW;
        dirty_cell_row[x] = true;
    }

    SDL_UnlockSurface(gpl_buffer);
    SDL_UnlockSurface(screen_buffer);
}

/*TODO: Use renderer and SDL_RenderSetScale*/
void teoSDL_Gfx40RetraceWholeScreen()
{
    if(scaledBlit)
        SDL_BlitScaled(screen_buffer, &screen_buffer->clip_rect, screenSurface, &screenSurface->clip_rect);
    else
        SDL_BlitSurface(screen_buffer, NULL, screenSurface, NULL);
    SDL_UpdateWindowSurface(window);
}



/* RetraceScreen:
 *  Rafraîchit une portion de l'écran du TO8.
 */
void teoSDL_Gfx40RetraceScreen(int x, int y, int width, int height)
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
//    SDL_UpdateWindowSurface(window);
}




/* RefreshScreen:
 *  Rafraîchit l'écran du TO8.
 */
void teoSDL_Gfx40RefreshScreen(void)
{
    register int i,j;
    int cell_start;
    int *dirty_cell_row = dirty_cell;
    static int odd = 1;
    SDL_Rect s_area, d_area;
    bool need_refresh = false;

    if (teo.setting.interlaced_video)
    {
        odd ^= 1;
        for(j=odd; j<TEO_WINDOW_H<<1; j+=2){
            s_area.x = 0;
            s_area.y = j;
            s_area.w = TEO_WINDOW_W<<1;
            s_area.h = 1;

            d_area.x = 0;
            d_area.y = j;
            d_area.w = TEO_WINDOW_W<<1;
            d_area.h = 1;
            SDL_BlitSurface(screen_buffer, &s_area, screenSurface, &d_area);
            need_refresh = true;
        }
    }
    else
    {
        /* on groupe les dirty rectangles ligne par ligne */
        for (j=0; j<TEO_WINDOW_CH; j++)
        {
            for (i=0; i<TEO_WINDOW_CW; i++)
            {
                if (dirty_cell_row[i] == true)
                {
                    need_refresh = true;
                    cell_start=i;

                    while ((i<TEO_WINDOW_CW) && dirty_cell_row[i])
                        dirty_cell_row[i++]=false;

                    teoSDL_Gfx40RetraceScreen(cell_start*TEO_CHAR_SIZE, j*TEO_CHAR_SIZE,
                                              (i-cell_start)*TEO_CHAR_SIZE, TEO_CHAR_SIZE);
                }
            }
            /* ligne suivante */
            dirty_cell_row += TEO_WINDOW_CW;
        }
    }
    if(need_refresh)
        SDL_UpdateWindowSurface(window);
}




/* SetDiskLed:
 *  Allume/éteint la Led du lecteur de disquettes.
 */
static void teoSDL_Gfx40SetDiskLed(int led_on)
{
//    static int count = 0;

    if (led_on)
    {
        printf("Disk\n");
    }
}

static void teoSDL_Gfx40SetColor(int index, int r, int g, int b)
{
    /*TODO: Experiment with 24-bits surfaces*/
//    palette[index] = SDL_MapRGB(screen_buffer->format, r, g, b);
    palette[index] = SDL_MapRGBA(screen_buffer->format, r, g, b, 255);
}



void teoSDL_Gfx40ResizeLookup(int x,int y)
{
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

    x_fact=(double)x/(double)(TEO_WINDOW_W);
    y_fact=(double)y/(double)(TEO_WINDOW_H);

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


void teoSDL_Gfx40Reset()
{
    screenSurface = SDL_GetWindowSurface(window);
    SDLGui_SetWindow(window);
    teoSDL_VKbdSetWindow(window);
//    teoSDL_Gfx40LoadLeds(screenSurface->w, screenSurface->h);

    printf("screenSurface is now: %dx%d\n",screenSurface->w,screenSurface->h);

    scaledBlit = 0;
    if( screenSurface->w != TEO_WINDOW_W || screenSurface->h != TEO_WINDOW_H){
        scaledBlit = 1;
        scaleXFactor = (screenSurface->w*1.0)/(TEO_WINDOW_W);
        scaleYFactor = (screenSurface->h*1.0)/(TEO_WINDOW_H);
        printf("Scaling. Using factors: X=%0.2f and Y=%0.2f\n",scaleXFactor,scaleYFactor);

        teoSDL_Gfx40ResizeLookup(screenSurface->w, screenSurface->h);
    }

    memset(dirty_cell, 1, (TEO_WINDOW_CW*TEO_WINDOW_CH)*sizeof(int));

    teoSDL_Gfx40RetraceWholeScreen();

}



SDL_Window *teoSDL_Gfx40Window(int windowed_mode, const char *w_title)
{
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
                TEO_WINDOW_W, TEO_WINDOW_H,
                flags
                );
    if (window == NULL){
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return NULL;
    }
    printf("Window will be (w x h): (%d x %d), while TEO_SCREEN_{W,H} are: (%d, %d)\n",TEO_WINDOW_W,TEO_WINDOW_H,TEO_SCREEN_W,TEO_SCREEN_H);
#endif

    screenSurface = SDL_GetWindowSurface(window);
    SDLGui_SetWindow(window);
    teoSDL_VKbdSetWindow(window);
    printf("screenSurface is: %dx%d\n",screenSurface->w,screenSurface->h);
#if 0
    teoSDL_GfxLoadLeds(screenSurface->w, screenSurface->h);

    lightpen_cursor  = teoSDL_GetLightPenCursor();
    system_cursor = SDL_GetCursor();
#endif

    teo_SetPointer=teoSDL_Gfx40SetPointer;
    /* teo_SetPointer is NOT called during the virtual TO8 init.
     * it is only invoked when going through the virtual TO8 settings screen
     * TODO: Have a function init installed_pointer instead of the static assignmedn
     * AND call SDL_ShowCursor
     * */
    SDL_ShowCursor(SDL_DISABLE);
    printf("ok\n");
    return window;
}

SDL_Window *teoSDL_Gfx40GetWindow()
{
    return window;
}


/* InitGraphic:
 *  Initialise le pilote graphique 40 colonnes.
 */
void  teoSDL_Gfx40Init(void)
{

    gpl_buffer = SDL_CreateRGBSurfaceWithFormat(0, TEO_GPL_SIZE, 1, 24, SDL_PIXELFORMAT_RGBA8888);
    screen_buffer = SDL_CreateRGBSurfaceWithFormat(0, TEO_WINDOW_W, TEO_WINDOW_H, 32, SDL_PIXELFORMAT_RGBA8888);
    pixel_size = screen_buffer->format->BytesPerPixel;
    SDL_FillRect(screen_buffer, NULL, 0x00000000);
    dirty_cell = calloc(TEO_WINDOW_CW*TEO_WINDOW_CH, sizeof(int));
    palette[TEO_NCOLORS] = SDL_MapRGB(screen_buffer->format, TEO_PALETTE_COL1>>16, (TEO_PALETTE_COL1>>8)&0xFF, TEO_PALETTE_COL1&0xFF);


    teo_SetColor = teoSDL_Gfx40SetColor;
    teo_DrawGPL = teoSDL_Gfx40DrawGPL;
    teo_SetDiskLed = teoSDL_Gfx40SetDiskLed;
    teo_SetBorderColor = NULL;
    teo_DrawBorderLine = NULL;

}


/**
 * Called from *within* the virtual TO8 when users selects
 * mouse vs lightpen *in the vm*:
 * "Settings and preferences (3)"/"Choose lightpen/mouse(2)"
 * This allow the frontend to reflect this change to the user
 * by changing the cursor
 */
static void teoSDL_Gfx40SetPointer(int pointer)
{
    SDL_Cursor *current;

    current = SDL_GetCursor();
    switch (pointer)
    {
        case TEO_STATUS_MOUSE :
            installed_pointer=TEO_STATUS_MOUSE;
#if 0
            if(current != system_cursor)
                SDL_SetCursor(system_cursor);
            SDL_ShowCursor(SDL_DISABLE);
#endif
            break;

        case TEO_STATUS_LIGHTPEN :
            installed_pointer=TEO_STATUS_LIGHTPEN;
#if 0
            SDL_SetCursor(lightpen_cursor);
            SDL_ShowCursor(SDL_ENABLE);
#endif
            break;
    }
}

int teoSDL_Gfx40GetPointer(void)
{
    return installed_pointer;
}

