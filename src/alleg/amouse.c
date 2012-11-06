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
 *                          Jérémie Guillaume, François Mouret
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
 *  Module     : alleg/mouse.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 1998
 *  Modifié par: Eric Botcazou 14/10/2000
 *               François Mouret 01/11/2012
 *
 *  Interface de la souris.
 */


#ifndef SCAN_DEPEND
   #include <allegro.h>
#endif

#include "alleg/gfxdrv.h"
#include "alleg/mouse.h"
#include "media/mouse.h"
#include "to8.h"


#define SCR40_SIZE   8
#define SCR80_SIZE  16

static int screen_ratio;
static int border_supported;
static int installed_pointer;
static BITMAP *pen_pointer;
static char scr40_pen_pointer_data[]={ 0,0,1,1,1,0,0,0,
                                       0,0,1,2,1,0,0,0,
                                       1,1,1,2,1,1,1,0,
                                       1,2,2,2,2,2,1,0,
                                       1,1,1,2,1,1,1,0,
                                       0,0,1,2,1,0,0,0,
                                       0,0,1,1,1,0,0,0,
                                       0,0,0,0,0,0,0,0 };

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


/* Helper pour cliper les coordonnées:
 */
static inline int clip_x(void)
{
    if (border_supported)
    {
        int x = (mouse_x/screen_ratio) - TO8_BORDER_W;

        if (x<0)
            return 0;
        else if (x>=TO8_WINDOW_W)
            return TO8_WINDOW_W;
        else 
            return x;
    }
    else
        return mouse_x/screen_ratio;
}


static inline int clip_y(void)
{
    if (border_supported)
    {
        int y = (mouse_y/screen_ratio) - TO8_BORDER_W;

        if (y<0)
            return 0;
        else if (y>=TO8_WINDOW_H)
            return TO8_WINDOW_H;
        else 
            return y;
    }
    else
        return mouse_y/screen_ratio;
}
         


/* MouseCallBack:
 *  Routine de gestion de la souris du TO8 appelée par
 *  l'interruption matérielle souris INT 0x33 via Allegro.
 */
static void MouseCallback(int flags)
{
    if (flags&MOUSE_FLAG_LEFT_UP)
        mouse_Click(1, TRUE);
    else if (flags&MOUSE_FLAG_LEFT_DOWN)
        mouse_Click(1, FALSE);
 
    if (flags&MOUSE_FLAG_RIGHT_UP)
        mouse_Click(2, TRUE);
    else if (flags&MOUSE_FLAG_RIGHT_DOWN)
        mouse_Click(2, FALSE);

    if (flags&MOUSE_FLAG_MOVE)
        mouse_Motion(clip_x(), clip_y());
}

END_OF_FUNCTION(MouseCallback)



/* LightpenCallback:
 *  Routine de gestion du crayon optique du TO8 appelée par
 *  l'interruption matérielle souris INT 0x33 via Allegro.
 */
static void LightpenCallback(int flags)
{
    static int pointer_on, xpos, ypos;

    if (flags&MOUSE_FLAG_LEFT_UP)
        mouse_Click(1, TRUE);
    if (flags&MOUSE_FLAG_LEFT_DOWN)
        mouse_Click(1, FALSE);

    if (flags&(MOUSE_FLAG_MOVE|MOUSE_FLAG_RIGHT_DOWN))
    {
        if (pointer_on)
        {
            acquire_screen();

            RetraceScreen(xpos-pen_pointer->w/2+1, ypos-pen_pointer->h/2+1,
                          pen_pointer->w         , pen_pointer->h );

            release_screen();
        }
                               
        mouse_Motion(clip_x(), clip_y());

        if (flags&MOUSE_FLAG_RIGHT_DOWN)
            pointer_on^=1;

        if (pointer_on)
        {
            xpos = mouse_x;
            ypos = mouse_y;
            draw_sprite(screen, pen_pointer, xpos-pen_pointer->w/2+1,
                                             ypos-pen_pointer->h/2+1 );
        }
    }
}

END_OF_FUNCTION(LightpenCallback)


/* ------------------------------------------------------------------------- */


/* amouse_Install:
 *  Installe le périphérique de pointage choisi.
 */
void amouse_Install(int pointer)
{
    switch (pointer)
    {
        case TO8_MOUSE:
            mouse_callback=MouseCallback;
            installed_pointer=TO8_MOUSE;
            break;

        case TO8_LIGHTPEN:
            mouse_callback=LightpenCallback;
            installed_pointer=TO8_LIGHTPEN;
            break;

        case LAST_POINTER:
            mouse_callback=(installed_pointer == TO8_MOUSE ? MouseCallback
                                                         : LightpenCallback);
            break;
    }
}



/* amouse_ShutDown:
 *  Désactive le périphérique de pointage du TO8.
 */
void amouse_ShutDown(void)
{
    mouse_callback=NULL;
}



/* amouse_Init:
 *  Initialise le module souris/crayon optique.
 */
void amouse_Init(int gfx_mode, int bsupp)
{
    register int i;

    switch (gfx_mode)
    {
        case GFX_TRUECOLOR:
            /* création du pointeur crayon optique */
            pen_pointer=create_system_bitmap(SCR80_SIZE, SCR80_SIZE);

            for (i=0; i<SCR80_SIZE*SCR80_SIZE; i++)
                switch (scr80_pen_pointer_data[i])
                {
                    case 1:
                        putpixel(pen_pointer, i%SCR80_SIZE, i/SCR80_SIZE, 0);
                        break;

                    case 2:
                        putpixel(pen_pointer, i%SCR80_SIZE, i/SCR80_SIZE, makecol(255, 255, 255));
                        break;

                    default:
                        putpixel(pen_pointer, i%SCR80_SIZE, i/SCR80_SIZE, makecol(255, 0, 255));  /* mask */
                }

            screen_ratio = 2;
            break;

        case GFX_MODE80:
            /* création du pointeur crayon optique */
            pen_pointer=create_system_bitmap(SCR80_SIZE, SCR80_SIZE);

            for (i=0; i<SCR80_SIZE*SCR80_SIZE; i++)
                switch (scr80_pen_pointer_data[i])
                {
                    case 1:
                        putpixel(pen_pointer, i%SCR80_SIZE, i/SCR80_SIZE, 1);
                        break;

                    case 2:
                        putpixel(pen_pointer, i%SCR80_SIZE, i/SCR80_SIZE, 2);
                        break;

                    default:
                        putpixel(pen_pointer, i%SCR80_SIZE, i/SCR80_SIZE, 0);
                }

            screen_ratio = 2;
            break;

        case GFX_MODE40:
            /* création du pointeur crayon optique */
            pen_pointer=create_system_bitmap(SCR40_SIZE, SCR40_SIZE);

            for (i=0; i<SCR40_SIZE*SCR40_SIZE; i++)
                switch (scr40_pen_pointer_data[i])
                {
                    case 1:
                        putpixel(pen_pointer, i%SCR40_SIZE, i/SCR40_SIZE, 1);
                        break;

                    case 2:
                        putpixel(pen_pointer, i%SCR40_SIZE, i/SCR40_SIZE, 2);
                        break;

                    default:
                        putpixel(pen_pointer, i%SCR40_SIZE, i/SCR40_SIZE, 0);
                }
            
            screen_ratio = 1;
            break;
    }

    border_supported = bsupp;

    to8_SetPointer=amouse_Install;
    install_mouse();

    lock_bitmap(pen_pointer);
    LOCK_FUNCTION(MouseCallback);
    LOCK_FUNCTION(LightpenCallback);
}

