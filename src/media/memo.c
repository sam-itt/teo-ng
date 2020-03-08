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
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 25/09/2006 26/01/2010 18/03/2012
 *                               01/11/2012 25/10/2018
 *               Gilles Fétis 27/07/2011
 *               Samuel Devulder 05/02/2012
 *
 *  Gestion des cartouches.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <sys/stat.h>
#endif

#include "defs.h"
#include "hardware.h"
#include "errors.h"
#include "main.h"
#include "std.h"
#include "teo.h"

#ifdef PLATFORM_OGXBOX
#include "windows.h"
#endif

/* ------------------------------------------------------------------------- */


/* memo_IsMemo:
 *  Vérifie le format de la cartouche.
 *
 *  Renvoie la position du caractère terminateur, ou une erreur.
 */
int memo_IsMemo (const char filename[])
{
    FILE *file = NULL;
    int i;
    size_t size = 0;
    char checksum = '\x55';
    char memo_header[32];
    struct stat st;

    /* check size of file */
#if PLATFORM_OGXBOX
    HANDLE h;
    DWORD rv;

    /*Xbox SDK has no separate open(), one should use 
     * CreateFile to create or open an existing file
     * */
    rv = 0;
    h = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, 
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( h == INVALID_HANDLE_VALUE)
        return TEO_ERROR_FILE_OPEN;
        
    size = (size_t)GetFileSize(h, NULL);
    CloseHandle(h);
#else
    if (stat (filename, &st) != 0)
    {
        return TEO_ERROR_FILE_OPEN;
    }
    size = (size_t)st.st_size;
#endif
    if ((size > 65536) || ((size % 4096) != 0))
    {
        return TEO_ERROR_FILE_FORMAT;
    }
    /* load the header of the file */
    if ((file = fopen (filename,"rb")) == NULL)
    {
        return TEO_ERROR_FILE_OPEN;
    }
    size = fread (memo_header, 1, 32, file);
    fclose (file);
    if (size != 32)
    {
        return TEO_ERROR_DISK_IO;
    }
 
    /* first character */
    if (memo_header[0] != ' ')
    {
        return TEO_ERROR_MEMO_HEADER_NAME;
    }

    /* check the checksum */
    for (i = 0; i < 26; i++)
    {
        checksum += memo_header[i];
    }
    if (checksum != memo_header[26])
    {
        return TEO_ERROR_MEMO_HEADER_CHECKSUM;
    }

    /* check end of the name */
    for (i = 0; i < 26; i++)
    {
        if (memo_header[i] == '\x04')
            break;
    }
    if (i == 26)
    {
        return TEO_ERROR_MEMO_HEADER_NAME;
    }

    return 0;
}



/* memo_Eject:
 *  Ejecte la cartouche.
 */
void memo_Eject(void)
{
    register int i;

    for (i=0; i<mem.cart.nbank; i++)
        mem.cart.bank[i] = std_free (mem.cart.bank[i]);
    mem.cart.nbank = 0;
    
    teo.memo.file = std_free (teo.memo.file);
    teo.memo.label = std_free (teo.memo.label);
}



/* memo_Load:
 *  Charge une cartouche et extrait son label.
 */
int memo_Load(const char filename[])
{
    int err;
    register int i;
    FILE *file;
    size_t length;
    char memo_name[32] = "";
    char *pc_memo_name;

    /* vérification du format de la cartouche */
    if ((err = memo_IsMemo(filename)) < 0)
        return error_Message(err, filename);

    /* chargement de la cartouche */
    if ((file=fopen(filename,"rb")) == NULL)
        return error_Message(TEO_ERROR_FILE_OPEN, filename);

    memo_Eject();
    mem.cart.nbank = 0;
    while (feof (file) == 0)
    {
        if ( (mem.cart.bank[mem.cart.nbank] = calloc(sizeof(char), mem.cart.size*sizeof(char))) == NULL)
        {
            fclose(file);
            memo_Eject();
            return error_Message(TEO_ERROR_ALLOC, filename);
        }
        length = fread(mem.cart.bank[mem.cart.nbank], sizeof(char), mem.cart.size, file);
        length = length;
        mem.cart.nbank++;
    }
    fclose(file);

    /* récupération du label et du nom de fichier */
    i = strcspn ((char*)mem.cart.bank[0]+1, "\04");
    strncpy (memo_name, (char*)mem.cart.bank[0]+1, i);
    pc_memo_name = main_ThomsonToPcText (memo_name);
    teo.memo.label = std_free (teo.memo.label);
    teo.memo.label = std_strdup_printf ("%s", pc_memo_name);
    teo.memo.file  = std_free (teo.memo.file);
    teo.memo.file  = std_strdup_printf ("%s", filename);

    return TRUE;
}



/* memo_FirstLoad:
 *  Premier chargement de la memo.
 */
int memo_FirstLoad (void)
{
    int err = 0;
    char *s;

    if (teo.memo.file !=NULL) {
        s = std_strdup_printf ("%s", teo.memo.file);
        memo_Eject();
        if ((s != NULL) && (*s != '\0'))
        {
            if (memo_Load(s) < 0)
            {
                main_DisplayMessage (teo_error_msg);
                err = TEO_ERROR;
            }
        }
        s = std_free (s);
    }
    return err;
}

