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
 *  Module     : state.c
 *  Version    : 1.8.1
 *  Créé par   : François Mouret 25/04/2012
 *  Modifié par:
 *
 *  Chargement/Sauvegarde de l'état de l'émulateur.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
#endif

#include "to8.h"


void to8_LoadStateFiles (void)
{
    int i;

    for (i=0; i<NBDRIVE; i++)
        if (strlen (gui.disk[i].file) != 0)
            gui.loadstate_error += (to8_LoadDisk(i, gui.disk[i].file) == TO8_ERROR) ? 1 : 0;

    if (strlen (gui.memo.file) != 0)
        gui.loadstate_error += (to8_LoadMemo7(gui.memo.file) == TO8_ERROR) ? 1 : 0;

    if (strlen (gui.cass.file) != 0)
        gui.loadstate_error += (to8_LoadK7(gui.cass.file) == TO8_ERROR) ? 1 : 0;

    if (access("autosave.img"))
        to8_LoadImage("autosave.img");
}

