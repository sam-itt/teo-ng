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
 *  Module     : to8.c
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 25/09/2006 26/01/2010 18/03/2012
 *               Gilles Fétis 27/07/2011
 *               Samuel Devulder 05/02/2012
 *
 *  Gestion des cartouches du TO8.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
#endif

#include "intern/defs.h"
#include "intern/hardware.h"
#include "intern/errors.h"
#include "intern/gui.h"
#include "to8.h"



/* EjectMemo7:
 *  Ejecte la cartouche Mémo7.
 */
void to8_EjectMemo7(void)
{
    register int i;

    for (i=0; i<4; i++)
    {
        if (mem.cart.bank[i] != NULL)
        {
            free(mem.cart.bank[i]);
            mem.cart.bank[i] = NULL;
        }
    }
    mem.cart.nbank = 0;
    gui->memo.file[0] = '\0';
}


/* LoadMemo7:
 *  Charge une cartouche Mémo7 et extrait son label.
 */
int to8_LoadMemo7(const char filename[])
{
    register int i;
    FILE *file;
    int length, c, nbank, label_offset;
    char *memo_name, *p;

    if ((file=fopen(filename,"rb")) == NULL)
        return ErrorMessage(TO8_CANNOT_OPEN_FILE, NULL);

    /* test (empirique) de reconnaissance du format */
    length = 0;

    /* recherche du premier espace */
    do
    {
        c=fgetc(file);

        if ((c == EOF) || (++length>65536))
            goto bad_format;
    }
    while (c != 32);

    label_offset = length;

    /* on détermine la longueur du fichier, qui doit être
       un multiple de 4096 sans excéder 65536 octets */
    while (fgetc(file) != EOF)
        if (++length>65536)
            goto bad_format;

    if (length%4096)
        goto bad_format;
        
    /* allocation de la mémoire nécessaire */
    nbank = (length-1)/mem.cart.size + 1;

    if (mem.cart.nbank < nbank)
    {
        for (i=mem.cart.nbank; i<nbank; i++)
            if ( (mem.cart.bank[i] = malloc(mem.cart.size*sizeof(char))) == NULL)
                return ErrorMessage(TO8_BAD_ALLOC, NULL);
    }
    else if (mem.cart.nbank > nbank)
    {
        for (i=mem.cart.nbank; i>nbank; i--)
            free(mem.cart.bank[i-1]);
    }

    mem.cart.nbank = nbank;

    /* chargement de la cartouche */
    fseek(file, 0, SEEK_SET);

    for (i=0; i<mem.cart.nbank; i++)
        length = fread(mem.cart.bank[i], sizeof(char), mem.cart.size, file);

    for (i=length; i<mem.cart.size; i++)
        mem.cart.bank[mem.cart.nbank-1][i]=0;
 
    fclose(file);

    /* extraction du label */
    memo_name = (char*)mem.cart.bank[0]+label_offset;
    if ((p = strchr (memo_name, '\4')) != NULL)
    {
        memset (gui->memo.label, 0x00, TO8_MEMO7_LABEL_LENGTH);
        memcpy (gui->memo.label, memo_name, (size_t)(p-memo_name));
    }

#ifdef DJGPP
     strncpy (gui->memo.file, filename, MAX_PATH);
#else
    (void) snprintf (gui->memo.file, MAX_PATH, "%s", filename);
#endif

    return TO8_READ_ONLY;

  bad_format:
    fclose(file);
    return ErrorMessage(TO8_BAD_FILE_FORMAT, NULL);
}

