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
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               19/09/2012 18/09/2013
 *
 *  Gestion des réglages.
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

/* Boîte de dialogue. */
static DIALOG commdial[]={
/*  dialog proc       x    y    w    h  fg bg  key flags   d1 d2  dp */
{ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,  0,     0, 0, NULL },
#ifdef FRENCH_LANGUAGE
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "Réglages" },
{ d_text_proc,        60,  44,   0,   0, 0, 0,   0,  0,     0, 0, "Vitesse:" },
{ d_radio_proc,      135,  44,  76,   8, 0, 0, 'e',  D_EXIT,     1, 0, "&exacte" },
{ d_radio_proc,      205,  44,  76,   8, 0, 0, 'r',  D_EXIT,     1, 0, "&rapide" },
{ d_check_proc,       78,  64, 120,  14, 5, 0, 's',  D_EXIT,     0, 0, "&Son" },
{ d_slider_proc,     144,  64, 100,  15, 0, 0,   0,  0,   254, 0, NULL },
{ d_text_proc,        60,  84,   0,   0, 0, 0,   0,  0,     0, 0, "Mémoire:" },
{ d_radio_proc,      135,  84,  76,   8, 0, 0, '2',  0,     2, 0, "&256k" },
{ d_radio_proc,      200,  84,  76,   8, 0, 0, '5',  0,     2, 0, "&512k" },
{ d_check_proc,       88, 104, 148,  14, 5, 0, 'v',  0,     0, 0, "&Vidéo entrelacée" },
#else
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "Settings" },
{ d_text_proc,        60,  44,   0,   0, 0, 0,   0,  0,     0, 0, " Speed:" },
{ d_radio_proc,      135,  44,  76,   8, 0, 0, 'e',  D_EXIT,     1, 0, "&exact" },
{ d_radio_proc,      205,  44,  76,   8, 0, 0, 'f',  D_EXIT,     1, 0, "&fast" },
{ d_check_proc,       78,  64, 120,  14, 5, 0, 's',  D_EXIT,     0, 0, "&Sound" },
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

/* In the scenario where a check/radio has D_EXIT,
 * Allegro4 makes user code manage the state.
 * i.e Allegro doesn't put D_SELECTED before exiting
 * to user code
 *
 * These functions are mainly there to handle this behavior
 * actual business code is much shorter than that.
 *
 * */

static void asetting_SetToggle(int idx, int state)
{
    commdial[idx].d1 = state;
    if(state)
        commdial[idx].flags |= D_SELECTED;
    else
        commdial[idx].flags &= ~D_SELECTED;
}

static void asetting_ShowSoundControls(int show)
{
    if(show){
        commdial[COMMDIAL_SOUND].flags &= ~D_DISABLED;
        if(commdial[COMMDIAL_SOUND].d1)
            commdial[COMMDIAL_SLIDER].flags &= ~D_DISABLED;
        else
            commdial[COMMDIAL_SLIDER].flags |= D_DISABLED;
    }else{
        commdial[COMMDIAL_SOUND].flags |= D_DISABLED;
        commdial[COMMDIAL_SLIDER].flags |= D_DISABLED;
    }
}

static void asetting_ToggleSound(void)
{
    if(commdial[COMMDIAL_SOUND].d1){ /*Was selected before the click, i.e user just unselected it*/
        asetting_SetToggle(COMMDIAL_SOUND, FALSE);
        commdial[COMMDIAL_SLIDER].flags |= D_DISABLED;
    }else{ /*User just selected it*/
        asetting_SetToggle(COMMDIAL_SOUND, TRUE);
        commdial[COMMDIAL_SLIDER].flags &= ~D_DISABLED;
    }
}


static void asetting_ToggleMaxSpeed(void)
{
//    printf("commdial[COMMDIAL_MAXSPEED].d1: %d\n",commdial[COMMDIAL_MAXSPEED].d1);

    if(commdial[COMMDIAL_MAXSPEED].d1){ /*Was selected before the click, i.e user just unselected it*/
        /*Boilerplate: Self selection handling*/
        asetting_SetToggle(COMMDIAL_MAXSPEED, FALSE);
    }else{ /*User just selected it*/
        asetting_SetToggle(COMMDIAL_MAXSPEED, TRUE);

        /*Real actions: Disable sound and hide sound controls*/
        if(commdial[COMMDIAL_SOUND].d1) /*Sound is currently enabled*/
            asetting_ToggleSound(); /*Disable it*/
        /*Hide sound*/
        asetting_ShowSoundControls(FALSE);
    }

    /*Toggle the other radio*/
    if(commdial[COMMDIAL_MAXSPEED].d1){ 
        asetting_SetToggle(COMMDIAL_EXACTSPEED, FALSE);
    }else{
        asetting_SetToggle(COMMDIAL_EXACTSPEED, TRUE);
    }
}

static void asetting_ToggleExactSpeed(void)
{
    printf("commdial[COMMDIAL_EXACTSPEED].d1: %d\n",commdial[COMMDIAL_EXACTSPEED].d1);
    if(commdial[COMMDIAL_EXACTSPEED].d1){ /*Was selected before the click, i.e user just unselected it*/
        asetting_SetToggle(COMMDIAL_EXACTSPEED, FALSE);
    }else{ /*User just selected it*/
        asetting_SetToggle(COMMDIAL_EXACTSPEED, TRUE);
        
        /*Show sound controls*/
        asetting_ShowSoundControls(TRUE);
    }

    /*Toggle the other radio*/
    if(commdial[COMMDIAL_EXACTSPEED].d1){ 
        asetting_SetToggle(COMMDIAL_MAXSPEED, FALSE);
    }else{
        asetting_SetToggle(COMMDIAL_MAXSPEED, TRUE);
    }

}

/* asetting_Panel:
 *  Affiche le menu des commandes et réglages.
 */
void asetting_Panel(void)
{
    int flag;
    int first = 1;
    int ret;

    /*Speed*/
    asetting_SetToggle(COMMDIAL_EXACTSPEED, FALSE);
    asetting_SetToggle(COMMDIAL_MAXSPEED, FALSE);
    if(teo.setting.exact_speed){
        asetting_SetToggle(COMMDIAL_EXACTSPEED, TRUE);
    }else{
        asetting_SetToggle(COMMDIAL_MAXSPEED, TRUE);
    }

    /*Memory*/
    flag = (teo.setting.bank_range == 32) ? 0 : D_SELECTED;
    commdial[COMMDIAL_256K].flags = flag;
    flag = (teo.setting.bank_range == 32) ? D_SELECTED : 0;
    commdial[COMMDIAL_512K].flags = flag;

    /*Sound*/
    asetting_SetToggle(COMMDIAL_SOUND, FALSE);
    commdial[COMMDIAL_SLIDER].flags |= D_DISABLED; 
    if(teo.setting.sound_enabled){
        asetting_SetToggle(COMMDIAL_SOUND, TRUE);
        commdial[COMMDIAL_SLIDER].flags &= ~D_DISABLED; 
    }

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

    do{
        ret = popup_dialog(commdial, COMMDIAL_OK);
        printf("Doing command\n");
        switch(ret){
            case COMMDIAL_EXACTSPEED:
                asetting_ToggleExactSpeed();
                break;
            case COMMDIAL_MAXSPEED:
                asetting_ToggleMaxSpeed();
                break;
            case COMMDIAL_SOUND:
                asetting_ToggleSound();
                break;
        }
    }while(ret != COMMDIAL_OK);


    flag = (commdial[COMMDIAL_INTERLACE].flags & D_SELECTED) ? TRUE : FALSE;
    teo.setting.interlaced_video = flag;
    asound_SetVolume(commdial[COMMDIAL_SLIDER].d2+1);
    teo.setting.sound_volume = asound_GetVolume();
    flag = (commdial[COMMDIAL_EXACTSPEED].flags&D_SELECTED) ? TRUE : FALSE;
    teo.setting.exact_speed = flag;
    flag = (commdial[COMMDIAL_SOUND].flags&D_SELECTED) ? TRUE : FALSE;
    /*Sound must be turned off to enable max speed*/
    teo.setting.sound_enabled = flag && teo.setting.exact_speed; 
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
        commdial[COMMDIAL_INTERLACE].flags = D_DISABLED;
#endif
/*    if (!teo.setting.sound_enabled)
    {
        commdial[COMMDIAL_SOUND].flags = D_DISABLED;
        commdial[COMMDIAL_SLIDER].flags = D_DISABLED;
    }*/
}


/* asetting_Free:
 *  Libère le module interface utilisateur.
 */
void asetting_Free(void)
{
}
