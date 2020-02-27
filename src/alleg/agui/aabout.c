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
 *  Module     : alleg/agui/aabout.c
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               19/09/2012
 *               Samuel Cuella   02/2020
 *
 *  Fenêtre à propos.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif
#include <assert.h>

#include "std.h"
#include "teo.h"
#include "gettext.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"


#define ABOUTDIAL_VERSION   1
#define ABOUTDIAL_COPYRIGHT 2
#define ABOUTDIAL_OK        11
#define ABOUTDIAL_LEN       14

static DIALOG *_aboutdial = NULL; 
#define aabout_GetDialog() (_aboutdial ? _aboutdial : aabout_AllocDialog())

/* ------------------------------------------------------------------------- */

static DIALOG *aabout_AllocDialog(void)
{
    DIALOG *rv;
    int i;

    if(_aboutdial)
        return _aboutdial;

    rv = malloc(sizeof(DIALOG)*ABOUTDIAL_LEN);
    i = 0;
                       /* (dialog proc)      x    y    w    h   fg bg key flags   d1 d2  dp */
    rv[i++] = (DIALOG){ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,  0,     0, 0, NULL };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, NULL };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  30,   0,   0, 0, 0,   0,  0,     0, 0, NULL };
    rv[i++] = (DIALOG){ d_text_proc,        30,  50,   0,   0, 0, 0,   0,  0,     0, 0, _("Authors:") };
#ifdef ENABLE_FULL_CREDITS
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  60,   0,   0, 0, 0,   0,  0,     0, 0, "Gilles Fétis - Eric Botcazou" };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  70,   0,   0, 0, 0,   0,  0,     0, 0, "Alex Pukall - Jérémie Guillaume" };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  80,   0,   0, 0, 0,   0,  0,     0, 0, "François Mouret - Samuel Devulder" };
#else
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  60,   0,   0, 0, 0,   0,  0,     0, 0, TEO_AUTHORS };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  70,   0,   0, 0, 0,   0,  0,     0, 0, "" };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  80,   0,   0, 0, 0,   0,  0,     0, 0, "" };
#endif
    rv[i++] = (DIALOG){ d_text_proc,        30, 100,   0,   0, 0, 0,   0,  0,     0, 0, _("Teo on SourceForge:") };
    rv[i++] = (DIALOG){ d_text_proc,        30, 110,   0,   0, 0, 0,   0,  0,     0, 0, "http://sourceforge.net/projects/" };
    rv[i++] = (DIALOG){ d_text_proc,        30, 120,   0,   0, 0, 0,   0,  0,     0, 0, "                     teoemulator/" };
    rv[i++] = (DIALOG){ d_ctext_proc,      160, 140,   0,   0, 0, 0,   0,  0,     0, 0, _("Licence: GPL 2.0") };
    rv[i++] = (DIALOG){ d_button_proc,     120, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" };
    rv[i++] = (DIALOG){ d_yield_proc,       20,  10,   0,   0, 0, 0,   0,  0,     0, 0, NULL };
    rv[i++] = (DIALOG){ NULL,                0,   0,   0,   0, 0, 0,   0,  0,     0, 0, NULL };

    assert(i <= ABOUTDIAL_LEN);

    _aboutdial = rv;

    return rv;
}


/* aabout_Panel:
 *  Affiche le menu des commandes et réglages.
 */
void aabout_Panel(void)
{
    static int first=1;
    DIALOG *aboutdial;

    aboutdial = aabout_GetDialog();

    if (first)
    {
        centre_dialog(aboutdial);
        first=0;
    }
    clear_keybuf();

    switch (popup_dialog(aboutdial, ABOUTDIAL_OK))
    {
        case -1:  /* ESC */
        case ABOUTDIAL_OK:
            return;
    }
}



/* aabout_SetColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void aabout_SetColors(int fg_color, int bg_color, int bg_entry_color)
{
    DIALOG *aboutdial;

    aboutdial = aabout_GetDialog();
    set_dialog_color(aboutdial, fg_color, bg_color);
    (void)bg_entry_color;
}



/* aabout_Init:
 *  Initialise le module interface utilisateur.
 */
void aabout_Init(char version_name[])
{
    DIALOG *aboutdial;

    aboutdial = aabout_GetDialog();

    /* Définit le titre de la fenêtre */
    aboutdial[ABOUTDIAL_VERSION].dp = std_strdup_printf ("%s", version_name);

    /* Définit le copyright de la fenêtre */
    aboutdial[ABOUTDIAL_COPYRIGHT].dp = std_strdup_printf ("Copyright (c) 1997-%s", TEO_YEAR_STRING);
}



/* aabout_Free:
 *  Libère le module interface utilisateur.
 */
void aabout_Free(void)
{
    DIALOG *aboutdial;

    aboutdial = aabout_GetDialog();
    aboutdial[ABOUTDIAL_VERSION].dp = std_free (aboutdial[ABOUTDIAL_VERSION].dp);
    aboutdial[ABOUTDIAL_COPYRIGHT].dp = std_free (aboutdial[ABOUTDIAL_COPYRIGHT].dp);
}
