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
 *  Module     : alleg/state.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou 24/09/2001
 *  Modifié par: Eric Botcazou 26/10/2003
 *               François Mouret 28/01/2010
 *
 *  Module de chargement/sauvegarde de l'état de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/main.h"
#include "alleg/sound.h"
#include "to8.h"



/* LoadState:
 *  Charge un état sauvegardé de l'émulateur.
 */
void LoadState(void)
{
    const char *str_p;

    str_p = get_config_string("teo", "speed", "exact");
    if (strcmp(str_p, "fast") == 0)
        teo.exact_speed = FALSE;

    if (teo.sound_enabled)
        SetVolume(MID(1, get_config_int("teo", "sound_vol", 128), 255));

    str_p = get_config_string("teo", "memo7", NULL);
    if (str_p)
        to8_LoadMemo7(str_p);

    str_p = get_config_string("teo", "k7", NULL);
    if (str_p)
        to8_LoadK7(str_p);

    str_p = get_config_string("teo", "disk_0", NULL);
    if (str_p)
        to8_LoadDisk(0, str_p);

    str_p = get_config_string("teo", "disk_1", NULL);
    if (str_p)
        to8_LoadDisk(1, str_p);

    str_p = get_config_string("teo", "disk_2", NULL);
    if (str_p)
        to8_LoadDisk(2, str_p);

    str_p = get_config_string("teo", "disk_3", NULL);
    if (str_p)
        to8_LoadDisk(3, str_p);

    to8_LoadImage(get_config_string("teo", "autosave", "autosave.img"));
}



/* SaveState:
 *  Sauvegarde l'état de l'émulateur.
 */
void SaveState(void)
{
    set_config_string("teo", "speed", teo.exact_speed ? "exact" : "fast");

    if (teo.sound_enabled)
        set_config_int("teo", "sound_vol", GetVolume());

    set_config_string("teo", "memo7", to8_GetMemo7Filename());

    set_config_string("teo", "k7", to8_GetK7Filename());

    set_config_string("teo", "disk_0", to8_GetDiskFilename(0));

    set_config_string("teo", "disk_1", to8_GetDiskFilename(1));

    set_config_string("teo", "disk_2", to8_GetDiskFilename(2));

    set_config_string("teo", "disk_3", to8_GetDiskFilename(3));

    to8_SaveImage(get_config_string("teo", "autosave", "autosave.img"));
}

