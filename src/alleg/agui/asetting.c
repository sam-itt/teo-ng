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
 *  Module     : alleg/agui/asetting.c
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *
 *  Gestion des réglages.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "intern/gui.h"
#include "to8.h"

extern void PopupMessage(const char message[]);

/* Boîte de dialogue. */
static DIALOG commdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)    (fg)   (bg)   (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,   280,  180,     0,     0,      0,   0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,     0,    0,     0,     0,      0,   0,       0,    0,    NULL },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  30,     0,    0,     0,     0,      0,   0,       0,    0,    "Commandes et Réglages" },
{ d_button_proc,      30,  45,   260,   16,     0,     0,    'r',  D_EXIT,   0,    0,    "&Réinitialiser le TO8" },
{ d_button_proc,      30,  68,   260,   16,     0,     0,    'f',  D_EXIT,   0,    0,    "&Redémarrer à &froid le TO8" },
{ d_text_proc,        32,  94,     0,    0,     0,     0,      0,   0,       0,    0,    "Vitesse:" },
{ d_radio_proc,      107,  94,   126,    8,     0,     0,    'e',   0,       1,    0,    "&exacte" },
{ d_radio_proc,      107, 108,   126,    8,     0,     0,    'p',   0,       1,    0,    "ra&pide" },
{ d_text_proc,        32, 124,     0,    0,     0,     0,      0,   0,       0,    0,    "&Volume:" },
{ d_slider_proc,      92, 122,   100,   15,     0,     0,    'v',   0,     254,    0,    NULL },
{ d_check_proc,       30, 142,   148,   14,     0,     0,    't',   0,       0,    0,    "Vidéo en&trelacée" },
{ d_box_proc,        209,  94,    80,   74,     0,     0,      0,   0,       0,    0,    NULL },
{ d_ctext_proc,      248, 100,     0,    0,     0,     0,      0,   0,       0,    0,    "Images:" },
{ d_button_proc,     217, 117,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Charger" },
{ d_button_proc,     217, 142,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Sauver" },
#else
{ d_ctext_proc,      160,  30,     0,    0,     0,     0,      0,   0,       0,    0,    "Commands and Settings" },
{ d_button_proc,      30,  45,   260,   16,     0,     0,    'w',  D_EXIT,   0,    0,    "TO8 &warm reset" },
{ d_button_proc,      30,  68,   260,   16,     0,     0,    'c',  D_EXIT,   0,    0,    "TO8 &cold reset" },
{ d_text_proc,        32,  94,     0,    0,     0,     0,      0,   0,       0,    0,    "  Speed:" },
{ d_radio_proc,      107,  94,   126,    8,     0,     0,    'e',   0,       1,    0,    "&exact" },
{ d_radio_proc,      107, 108,   126,    8,     0,     0,    'f',   0,       1,    0,    "&fast" },
{ d_text_proc,        32, 124,     0,    0,     0,     0,      0,   0,       0,    0,    "&Volume:" },
{ d_slider_proc,      92, 122,   100,   15,     0,     0,    'v',   0,     254,    0,    NULL },
{ d_check_proc,       30, 142,   147,   14,     5,     0,    't',   0,       0,    0,    "In&terlaced video" },
{ d_box_proc,        209,  94,    80,   74,     0,     0,      0,   0,       0,    0,    NULL },
{ d_ctext_proc,      248, 100,     0,    0,     0,     0,      0,   0,       0,    0,    "Images:" },
{ d_button_proc,     217, 117,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Load" },
{ d_button_proc,     217, 142,    64,   15,     0,     0,      0,  D_EXIT,   0,    0,    "Save" },
#endif
{ d_button_proc,      30, 170,   126,   16,     0,     0,    'o',  D_EXIT,   0,    0,    "&OK" },
{ d_yield_proc,       20,  10,     0,    0,     0,     0,      0,   0,       0,    0,    NULL },
{ NULL,                0,   0,     0,    0,     0,     0,      0,   0,       0,    0,    NULL }
};

#define COMMDIAL_INIT        3
#define COMMDIAL_COLDINIT    4
#define COMMDIAL_EXACTSPEED  6
#define COMMDIAL_MAXSPEED    7
#define COMMDIAL_VOLUME      8
#define COMMDIAL_SLIDER      9
#define COMMDIAL_INTERLACE   10
#define COMMDIAL_SPAN        11
#define COMMDIAL_LOAD        13
#define COMMDIAL_SAVE        14
#define COMMDIAL_OK          15


/* MenuComm:
 *  Affiche le menu des commandes et réglages.
 */
void MenuComm(void)
{
    int first = 1;
    static char filename[MAX_PATH];

    if (gui->setting.exact_speed)
    {
        commdial[COMMDIAL_EXACTSPEED].flags=D_SELECTED;
        commdial[COMMDIAL_MAXSPEED].flags=0;
    }
    else
    {
        commdial[COMMDIAL_EXACTSPEED].flags=0;
        commdial[COMMDIAL_MAXSPEED].flags=D_SELECTED;
    }

    if (first)
    {
        SetVolume(gui->setting.sound_volume);
        commdial[COMMDIAL_SLIDER].d2=gui->setting.sound_volume-1;
        first = 0;
    }
    else
       commdial[COMMDIAL_SLIDER].d2=GetVolume()-1;

    clear_keybuf();
    centre_dialog(commdial);

    switch (popup_dialog(commdial, COMMDIAL_OK))
    {
        case COMMDIAL_INIT:
            teo.command=RESET;
            break;

        case COMMDIAL_COLDINIT:
            teo.command=COLD_RESET;
            break;

        case COMMDIAL_LOAD:
            if (file_select_ex(is_fr?"Choisissez votre image:":"Choose your image:", filename, "img",
                               MAX_PATH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
            {
                if (to8_LoadImage(filename) == TO8_ERROR)
                    PopupMessage(to8_error_msg);
            }
            break;
        
        case COMMDIAL_SAVE:
            if (file_select_ex(is_fr?"Spécifiez un nom pour votre image:":"Specify a name for your image:", filename, "img",
                               MAX_PATH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
            {
                if (to8_SaveImage(filename) == TO8_ERROR)
                    PopupMessage(to8_error_msg);
            }
            break;
    }
    gui->setting.interlaced_video = (commdial[COMMDIAL_INTERLACE].flags & D_SELECTED) ? TRUE : FALSE;
    SetVolume(commdial[COMMDIAL_SLIDER].d2+1);
    gui->setting.sound_volume = GetVolume();
    gui->setting.exact_speed=(commdial[COMMDIAL_EXACTSPEED].flags&D_SELECTED ? TRUE : FALSE);
}



/* SetCommGUIColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void SetCommGUIColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(commdial, fg_color, bg_color);
}



/* InitCommGUI:
 *  Initialise le module interface utilisateur.
 */
void InitCommGUI(char version_name[], int gfx_mode, char *title)
{
    if ((strstr (version_name, "MSDOS") != NULL) && (gfx_mode == GFX_MODE40))
        commdial[COMMDIAL_INTERLACE].flags |= D_DISABLED;

    if (!teo.sound_enabled)
        commdial[COMMDIAL_VOLUME].flags=commdial[COMMDIAL_SPAN].flags=commdial[COMMDIAL_SLIDER].flags=D_DISABLED;

    /* Définit le titre de la fenêtre */
    commdial[1].dp = title;
}
