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
 *                          Jérémie Guillaume, François Mouret,
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
 *  Module     : gui.c
 *  Version    : 1.8.1
 *  Créé par   : François Mouret 26/04/2012
 *  Modifié par:
 *
 *  Gestion de la GUI
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
   #include <sys/stat.h>
   #include <unistd.h>
#endif

#include "intern/errors.h"
#include "to8.h"

struct THOMSON_GUI *gui = NULL;


/* bad_alloc:
 *  Libère la mémoire occupée par la GUI et retourne une erreur.
 */
static int bad_alloc (void)
{
   to8_FreeGUI ();
   return ErrorMessage(TO8_BAD_ALLOC, NULL);
}



/* to8_FreeGUI:
 *  Libère la mémoire occupée par la GUI commune.
 */
void to8_FreeGUI (void)
{
    int i;

    if (gui == NULL)
        return;

    if (gui->lprt.folder != NULL)
    {
        free (gui->lprt.folder);
        gui->lprt.folder = NULL;
    }

    for (i=0; i<NBDRIVE; i++)
    {
        if (gui->disk[i].file != NULL)
        {
            free (gui->disk[i].file);
            gui->disk[i].file = NULL;
        }
    }

    if (gui->cass.file != NULL)
    {
        free (gui->cass.file);
        gui->cass.file = NULL;
    }

    if (gui->memo.file != NULL)
    {
        free (gui->memo.file);
        gui->memo.file = NULL;
    }

    if (gui->memo.label != NULL)
    {
        free (gui->memo.label);
        gui->memo.label = NULL;
    }

    if (gui->imag.file != NULL)
    {
        free (gui->imag.file);
        gui->imag.file = NULL;
    }

    free (gui);
    gui = NULL;
}

    

/* to8_InitGUI:
 *  Initialise la GUI commune.
 */
int to8_InitGUI (void)
{
    int i;

    if ((gui = (struct THOMSON_GUI *)calloc (sizeof(struct THOMSON_GUI), 1)) == NULL)
        return (bad_alloc());
    
    if ((gui->lprt.folder = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    for (i=0; i<NBDRIVE; i++)
        if ((gui->disk[i].file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
            return (bad_alloc());

    if ((gui->cass.file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    if ((gui->memo.file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    if ((gui->memo.label = (char *)calloc (TO8_MEMO7_LABEL_LENGTH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    if ((gui->imag.file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    return TO8_OK;
}

