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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume
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
 *  Module     : errors.c
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou 22/11/2000
 *  Modifié par: Eric Botcazou 06/03/2001
 *               François Mouret 08/2011 13/01/2012 17/11/2012
 *                               23/08/2015
 *
 *  Gestion des erreurs générées par l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stddef.h>
   #include <string.h>
#endif

#include "defs.h"
#include "std.h"
#include "errors.h"
#include "teo.h"

char *teo_error_msg = NULL;

struct ERROR_TABLE {
    int   err;
    char  *str;
};


/* ------------------------------------------------------------------------- */


/* error_Message:
 *  Renvoie une erreur générée par l'émulateur.
 */
int error_Message(int error, const char moreinfo[])
{
    int i = 0;
#ifdef UNIX_TOOL
    int is_unix = 1;
#else
    int is_unix = 0;
#endif
    struct ERROR_TABLE error_table[]= {
        { TEO_ERROR_ALLOC           , is_fr?(is_unix?"Plus de mÃ©moire"
                                                    :"Plus de mémoire")
                                           :"No more memory space"},
        { TEO_ERROR_BMP_FORMAT   , is_fr?"Format BMP incorrect"
                                           :"Bad BMP format"},
        { TEO_ERROR_DIRECTORY_FULL, is_fr?(is_unix?"RÃ©pertoire plein"
                                                  :"Répertoire plein")
                                         :"Directory full"},
        { TEO_ERROR_DISK_CREATE, is_fr?(is_unix?"Impossible de crÃ©er la disquette"
                                               :"Impossible de créer la disquette")
                                      :"Cannot create disk"},
        { TEO_ERROR_DISK_IO, is_fr?"Erreur sur le disque"
                                  :"Disk I/O error"},
        { TEO_ERROR_DISK_NONE, is_fr?"Disque absent":"No disk"},
        { TEO_ERROR_DISK_PROTECT, is_fr?(is_unix?"Disque protÃ©gÃ©e en Ã©criture"
                                               :"Disque protégé en écriture")
                                      :"Disk write protected"},
        { TEO_ERROR_FILE_EMPTY      , is_fr?"Fichier vide"
                                           :"File is empty"},
        { TEO_ERROR_FILE_FORMAT     , is_fr?"Mauvais format de fichier"
                                           :"Bad file format"},
        { TEO_ERROR_FILE_NOT_FOUND  , is_fr?"Fichier introuvable"
                                           :"File not found"},
        { TEO_ERROR_FILE_OPEN       , is_fr?"Ouverture impossible"
                                           :"Unable to open"},
        { TEO_ERROR_FILE_READ       , is_fr?"Erreur de lecture du fichier"
                                           :"Error while reading file"},
        { TEO_ERROR_FILE_TOO_LARGE  , is_fr?"Fichier trop important"
                                           :"File too large"},
        { TEO_ERROR_FILE_WRITE      , is_fr?"Erreur d'Ã©criture du fichier"
                                           :"Error while writing file"},
        { TEO_ERROR_JOYSTICK_NUM    , is_fr?"Nombre de joysticks incorrect"
                                           :"Bad count of joysticks"},
        { TEO_ERROR_MEDIA_ALREADY_SET, is_fr?"Plus de media libre"
                                             :"Media not free"},
        { TEO_ERROR_MEMO_HEADER_CHECKSUM, is_fr?(is_unix?"Checksum d'en-tÃªte de memo erronÃ©"
                                                        :"Checksum d'en-tête de memo erroné")
                                               :"Bad checksum for memo header"},
        { TEO_ERROR_MEMO_HEADER_NAME, is_fr?(is_unix?"Nom d'en-tÃªte de memo incorrect"
                                                    :"Nom d'en-tête de memo incorrect")
                                           :"Bad name for memo header"},
        { TEO_ERROR_MULTIPLE_INIT   , is_fr?"Instance multiple de Teo"
                                          :"Multiple instance of Teo"},
        { TEO_ERROR_UNSUPPORTED_MODEL, is_fr?(is_unix?"Image d'un modÃ¨le non supportÃ©"
                                                     :"Image d'un modèle non supporté")
                                            :"Unsupported image format"},
        { TEO_ERROR_VALID_SAP, is_fr?"Archive SAP invalide"
                                    :"SAP archive not valid"},
        { 0 , is_fr?"Erreur inconnue"
                    :"Unknown error"}
    };    

    if (error < TEO_ERROR)
    {
        while ((error_table[i].err != 0)
            && (error_table[i].err != error))
               i++;
    
        teo_error_msg = std_free (teo_error_msg);

        if (moreinfo != NULL)
            teo_error_msg = std_strdup_printf ("%s.\n%s",
                                               error_table[i].str,
                                               moreinfo);
        else
            teo_error_msg = std_strdup_printf ("%s.", error_table[i].str);
    }
    return TEO_ERROR;
}

