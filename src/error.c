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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2012 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume
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
 *  Version    : 1.8.2
 *  Cr�� par   : Eric Botcazou 22/11/2000
 *  Modifi� par: Eric Botcazou 06/03/2001
 *               Fran�ois Mouret 08/2011 13/01/2012 17/11/2012
 *
 *  Gestion des erreurs g�n�r�es par l'�mulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stddef.h>
   #include <string.h>
#endif

#include "std.h"
#include "error.h"
#include "teo.h"

char *teo_error_msg = NULL;

struct ERROR_TABLE {
    int   err;
    char  *str;
};


/* ------------------------------------------------------------------------- */


/* error_Message:
 *  Renvoie une erreur g�n�r�e par l'�mulateur.
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
        { TEO_ERROR_MULTIPLE_INIT   , is_fr?"Instance multiple de Teo"
                                          :"Multiple instance of Teo"},
        { TEO_ERROR_ALLOC           , is_fr?(is_unix?"Plus de mémoire"
                                                    :"Plus de m�moire")
                                           :"No more memory space"},
        { TEO_ERROR_FILE_NOT_FOUND  , is_fr?"Fichier introuvable"
                                           :"File not found"},
        { TEO_ERROR_FILE_OPEN       , is_fr?"Ouverture impossible"
                                           :"Unable to open"},
        { TEO_ERROR_FILE_FORMAT     , is_fr?"Mauvais format de fichier"
                                           :"Bad file format"},
        { TEO_ERROR_FILE_TOO_LARGE  , is_fr?"Fichier trop important"
                                           :"File too large"},
        { TEO_ERROR_FILE_EMPTY      , is_fr?"Fichier vide"
                                           :"File is empty"},
        { TEO_ERROR_UNSUPPORTED_MODEL, is_fr?(is_unix?"Image d'un modèle non supporté"
                                                     :"Image d'un mod�le non support�")
                                            :"Unsupported image format"},
        { TEO_ERROR_BMP_FORMAT   , is_fr?"Format BMP incorrect"
                                           :"Bad BMP format"},
        { TEO_ERROR_JOYSTICK_NUM    , is_fr?"Nombre de joysticks incorrect"
                                           :"Bad count of joysticks"},
        { TEO_ERROR_MEMO_HEADER_CHECKSUM, is_fr?(is_unix?"Checksum d'en-tête de memo erroné"
                                                        :"Checksum d'en-t�te de memo erron�")
                                               :"Bad checksum for memo header"},
        { TEO_ERROR_MEMO_HEADER_NAME, is_fr?(is_unix?"Nom d'en-tête de memo incorrect"
                                                    :"Nom d'en-t�te de memo incorrect")
                                           :"Bad name for memo header"},
        { TEO_ERROR_MEDIA_ALREADY_SET, is_fr?"Plus de media libre"
                                             :"Media not free"},
        { TEO_ERROR_VALID_SAP, is_fr?"Archive SAP invalide"
                                    :"SAP archive not valid"},
        { TEO_ERROR_DISK_IO, is_fr?"Erreur sur le disque"
                                  :"Disk I/O error"},
        { TEO_ERROR_DISK_PROTECT, is_fr?(is_unix?"Disque protégée en écriture"
                                               :"Disque prot�g� en �criture")
                                      :"Disk write protected"},
        { TEO_ERROR_DISK_CREATE, is_fr?(is_unix?"Impossible de créer la disquette"
                                               :"Impossible de cr�er la disquette")
                                      :"Cannot create disk"},
        { TEO_ERROR_DISK_ACCESS, is_fr?(is_unix?"Impossible d'accéder au disque"
                                               :"Impossible d'acc�der au disque")
                                      :"Unable to access disk"},
        { TEO_ERROR_DISK_CONVERSION, is_fr?"Impossible de convertir le disque"
                                          :"Unable to convert disk"},
        { TEO_ERROR_DIRECTORY_FULL, is_fr?(is_unix?"Répertoire plein"
                                                  :"R�pertoire plein")
                                         :"Directory full"},
        { TEO_ERROR_SERIAL_OPEN, is_fr?(is_unix?"Port série introuvable"
                                             :"Port s�rie introuvable")
                                     :"No serial port found"},
        { TEO_ERROR_SERIAL_IO, is_fr?(is_unix?"Erreur de transfert série"
                                               :"Erreur de transfert CRC s�rie")
                                     :"Serial transfer error"},
        { TEO_ERROR_CC90_CRC, is_fr?(is_unix?"Erreur de CRC série"
                                               :"Erreur de CRC s�rie")
                                     :"CRC serial error"},
        { TEO_ERROR_CC90_BSTART, is_fr?(is_unix?"Erreur de BSTART série"
                                                  :"Erreur de BSTART s�rie")
                                        :"BSTART serial error"},
        { TEO_ERROR_CC90_VERSION, is_fr?"Mauvaise version de CC90"
                                       :"Bad version of CC90"},
        { TEO_ERROR_CC90_MEMORY, is_fr?(is_unix?"Erreur de mémoire Thomson"
                                                    :"Erreur de m�moire Thomson")
                                           :"Thomson memory error"},
        { 0 , is_fr?"Erreur inconnue"
                    :"Unknown error"}
    };    

    if (error < TEO_ERROR)
    {
        while ((error_table[i].err != 0)
            && (error_table[i].err != error))
               i++;
    
        teo_error_msg = std_free (teo_error_msg);
        
#ifdef DJGPP
        teo_error_msg = std_strdup_printf ("%s.", error_table[i].str);
#else
        if (moreinfo == NULL)
            teo_error_msg = std_strdup_printf ("%s.", error_table[i].str);
        else
            teo_error_msg = std_strdup_printf ("%s : %s", error_table[i].str, moreinfo);
#endif
    }
    return TEO_ERROR;
}

