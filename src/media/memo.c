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
 *  Module     : memo.c
 *  Version    : 1.8.2
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
#include "intern/std.h"
#include "to8.h"



/* check_if_cartridge_like_file:
 *  Vérifie le format de la cartouche.
 *
 *  Renvoie la position du caractère terminateur, ou -1 si erreur.
 */
static int is_memo_like (char *fbuf, int minVal, int maxVal)
{
   int i;
   char checksum = '\x55';

    /* Vérifie checksum */
    for (i = 0; i < 27; i++) checksum += fbuf[i];
    if (checksum != fbuf[27])
        return -1;

    /* Vérifie reset à chaud et à froid */
    if ((fbuf[28] < minVal) || (fbuf[28] > maxVal)
     || (fbuf[30] < minVal) || (fbuf[30] > maxVal))
        return -1;

    /* first character */
    if (fbuf[0] != ' ') return -1;

    /* Vérifie la fin du nom de cartouche */
    for (i = 1; i < 25; i++)
       if (fbuf[i] < ' ')
           break;
    if (fbuf[i] != '\x04')
        return -1;

    return i;
}



/* EjectMemo:
 *  Ejecte la cartouche.
 */
void to8_EjectMemo(void)
{
    register int i;

    for (i=0; i<4; i++)
        mem.cart.bank[i] = std_free (mem.cart.bank[i]);
    mem.cart.nbank = 0;
    std_free (teo.memo.file);
    std_free (teo.memo.label);
}



/* LoadMemo:
 *  Charge une cartouche et extrait son label.
 */
int to8_LoadMemo(const char filename[])
{
    register int i;
    FILE *file;
    long int length;
    char memo_header[32];
    int memo_header_length;

    /* charge le header de cartouche (32 octets) */
    if ((file=fopen(filename,"rb")) == NULL)
        return ErrorMessage(TO8_CANNOT_OPEN_FILE, NULL);

    if (fread (memo_header, 32, 1, file) != 32)
        goto bad_format;
        
    /* test (empirique) de reconnaissance du format */
    if ((memo_header_length = is_memo_like (memo_header, 0x00, 0x40)) > 1)
        goto bad_format;
    
    /* récupération de la taille du fichier */
    fseek(file, 0, SEEK_END);
    length = ftell (file);
    if ((length > 65536) || ((length % 4096) != 0))
        goto bad_format;

        
    /* chargement de la cartouche */
    to8_EjectMemo();
    fseek(file, 0, SEEK_SET);
    mem.cart.nbank = ((int)length-1)/mem.cart.size + 1;
    for (i=0; i<mem.cart.nbank; i++) {
        if ( (mem.cart.bank[i] = calloc(sizeof(char), mem.cart.size*sizeof(char))) == NULL) {
            to8_EjectMemo();
            return ErrorMessage(TO8_BAD_ALLOC, NULL);
        }
        length = fread(mem.cart.bank[i], sizeof(char), mem.cart.size, file);
    }
 
    fclose(file);

    /* récupération du label et du nom de fichier */
    memo_header[memo_header_length] = '\0';
    teo.memo.label = std_free (teo.memo.label);
    teo.memo.label = std_strdup_printf ("%s", memo_header+1);
    teo.memo.file  = std_free (teo.memo.file);
    teo.memo.file  = std_strdup_printf ("%s", filename);

    return TO8_READ_ONLY;

  bad_format:
    fclose(file);
    return ErrorMessage(TO8_BAD_FILE_FORMAT, NULL);
}

