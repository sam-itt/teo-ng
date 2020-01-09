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
 *                          J�r�mie Guillaume, Fran�ois Mouret
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
 *  Module     : alleg/agui/asetting.c
 *  Version    : 1.8.5
 *  Cr�� par   : Gilles F�tis 1998
 *  Modifi� par: J�r�mie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               Fran�ois Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               19/09/2012 18/09/2013
 *
 *  Gestion des r�glages.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "defs.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "image.h"
#include "teo.h"

/* Bo�te de dialogue. */
static DIALOG commdial[]={
/*  dialog proc       x    y    w    h  fg bg  key flags   d1 d2  dp */
{ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,  0,     0, 0, NULL },
#ifdef FRENCH_LANGUAGE
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "R�glages" },
{ d_text_proc,        60,  44,   0,   0, 0, 0,   0,  0,     0, 0, "Vitesse:" },
{ d_radio_proc,      135,  44,  76,   8, 0, 0, 'e',  0,     1, 0, "&exacte" },
{ d_radio_proc,      205,  44,  76,   8, 0, 0, 'r',  0,     1, 0, "&rapide" },
{ d_check_proc,       78,  64, 120,  14, 5, 0, 's',  0,     0, 0, "&Son" },
{ d_slider_proc,     144,  64, 100,  15, 0, 0,   0,  0,   254, 0, NULL },
{ d_text_proc,        60,  84,   0,   0, 0, 0,   0,  0,     0, 0, "M�moire:" },
{ d_radio_proc,      135,  84,  76,   8, 0, 0, '2',  0,     2, 0, "&256k" },
{ d_radio_proc,      200,  84,  76,   8, 0, 0, '5',  0,     2, 0, "&512k" },
{ d_check_proc,       88, 104, 148,  14, 5, 0, 'v',  0,     0, 0, "&Vid�o entrelac�e" },
#else
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "Settings" },
{ d_text_proc,        60,  44,   0,   0, 0, 0,   0,  0,     0, 0, " Speed:" },
{ d_radio_proc,      135,  44,  76,   8, 0, 0, 'e',  0,     1, 0, "&exact" },
{ d_radio_proc,      205,  44,  76,   8, 0, 0, 'f',  0,     1, 0, "&fast" },
{ d_check_proc,       78,  64, 120,  14, 5, 0, 's',  0,     0, 0, "&Sound" },
{ d_slider_proc,     144,  64, 100,  15, 0, 0,   0,  0,   254, 0, NULL },
{ d_text_proc,        60,  84,   0,   0, 0, 0,   0,  0,     0, 0, "Memory:" },
{ d_radio_proc,      135,  84,  76,   8, 0, 0, '2',  0,     2, 0, "&256k" },
{ d_radio_proc,      200,  84,  76,   8, 0, 0, '5',  0,     2, 0, "&512k" },
{ d_check_proc,       88, 104, 147,  14, 5, 0, 'i',  0,     0, 0, "&Interlaced video" },
#endif
{ d_button_proc,     210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" },
{ d_yield_proc,       20,  10,   0,   0, 0, 0,   0,  0,     0, 0, NULL },
{ NULL,                0,   0,   0,   0, 0, 0,   0,  0,     0, 0, NULL }
};

#define COMMDIAL_EXACTSPEED  3
#define COMMDIAL_MAXSPEED    4
#define COMMDIAL_SOUND       5
#define COMMDIAL_SLIDER      6
#define COMMDIAL_256K        8
#define COMMDIAL_512K        9
#define COMMDIAL_INTERLACE   10
#define COMMDIAL_OK          11


/* ------------------------------------------------------------------------- */


/* asetting_Panel:
 *  Affiche le menu des commandes et r�glages.
 */
void asetting_Panel(void)
{
    int flag;
    int first = 1;

    flag = (teo.setting.exact_speed) ? D_SELECTED : 0;
    commdial[COMMDIAL_EXACTSPEED].flags = flag;
    flag = (teo.setting.exact_speed) ? 0 : D_SELECTED;
    commdial[COMMDIAL_MAXSPEED].flags = flag;
    flag = (teo.setting.bank_range == 32) ? 0 : D_SELECTED;
    commdial[COMMDIAL_256K].flags = flag;
    flag = (teo.setting.bank_range == 32) ? D_SELECTED : 0;
    commdial[COMMDIAL_512K].flags = flag;
    flag = (teo.setting.sound_enabled) ? D_SELECTED : 0;
    commdial[COMMDIAL_SOUND].flags = flag;

    if (first)
    {
        centre_dialog(commdial);

        asound_SetVolume(teo.setting.sound_volume);
        commdial[COMMDIAL_SLIDER].d2=teo.setting.sound_volume-1;
        first = 0;
    }
    else
       commdial[COMMDIAL_SLIDER].d2=asound_GetVolume()-1;

    clear_keybuf();

    popup_dialog(commdial, COMMDIAL_OK);

    flag = (commdial[COMMDIAL_INTERLACE].flags & D_SELECTED) ? TRUE : FALSE;
    teo.setting.interlaced_video = flag;
    asound_SetVolume(commdial[COMMDIAL_SLIDER].d2+1);
    teo.setting.sound_volume = asound_GetVolume();
    flag = (commdial[COMMDIAL_SOUND].flags&D_SELECTED) ? TRUE : FALSE;
    teo.setting.sound_enabled = flag;
    flag = (commdial[COMMDIAL_EXACTSPEED].flags&D_SELECTED) ? TRUE : FALSE;
    teo.setting.exact_speed = flag;
    flag = (commdial[COMMDIAL_512K].flags&D_SELECTED) ? 32 : 16;
    if (flag != teo.setting.bank_range)
        teo.command = TEO_COMMAND_COLD_RESET;
    teo.setting.bank_range = flag;
}



/* asetting_SetColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void asetting_SetColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(commdial, fg_color, bg_color);
}



/* asetting_Init:
 *  Initialise le module interface utilisateur.
 */
void asetting_Init(int gfx_mode)
{
#ifdef PLATFORM_MSDOS
    if(gfx_mode == GFX_MODE40)
        commdial[COMMDIAL_INTERLACE].flags |= D_DISABLED;
#endif
    if (!teo.setting.sound_enabled)
    {
        commdial[COMMDIAL_SOUND].flags = D_DISABLED;
        commdial[COMMDIAL_SLIDER].flags = D_DISABLED;
    }
}


/* asetting_Free:
 *  Lib�re le module interface utilisateur.
 */
void asetting_Free(void)
{
}
