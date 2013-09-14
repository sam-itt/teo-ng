/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 François Mouret
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
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 26/07/2013
 *
 *  Management of errors.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stddef.h>
   #include <string.h>
#endif

#include "defs.h"
#include "std.h"
#include "encode.h"
#include "errors.h"

char *error_msg = NULL;

struct ERROR_TABLE {
    int   err;
    char  *fr_str;
    char  *en_str;
};

static struct ERROR_TABLE error_table[]= {
    { CC90HFE_ERROR_ALLOC,
         "Plus de mémoire",
         "No more memory space"},
    { CC90HFE_ERROR_CC90_BSTART,
         "CC90: erreur de BSTART",
         "CC90: BSTART error" },
    { CC90HFE_ERROR_CC90_CRC,
         "CC90: erreur de CRC",
         "CC90: CRC error" },
    { CC90HFE_ERROR_CC90_MEMORY,
         "CC90: erreur de mémoire",
         "CC90: memory error" },
    { CC90HFE_ERROR_CC90_THMFC1,
         "CC90: THMFC1 requis",
         "CC90: THMFC1 needed" },
    { CC90HFE_ERROR_DISK_ACCESS,
         "Accession disquette impossible",
         "Unable to access disk" },
    { CC90HFE_ERROR_DISK_CONVERSION,
         "Impossible de convertir le disque",
         "Unable to convert disk"},
    { CC90HFE_ERROR_DISK_IO,
         "Erreur sur le disque",
         "Disk I/O error" },
    { CC90HFE_ERROR_DISK_NONE,
         "Disque absent",
         "No disk" },
    { CC90HFE_ERROR_DISK_PROTECT,
         "Disque protégé en écriture",
         "Disk write protected"},
    { CC90HFE_ERROR_FILE_FORMAT,
         "Mauvais format de fichier",
         "Bad file format"},
    { CC90HFE_ERROR_FILE_NOT_FOUND,
         "Fichier introuvable",
         "File not found"},
    { CC90HFE_ERROR_FILE_OPEN,
         "Ouverture du fichier impossible",
         "Unable to open file "},
    { CC90HFE_ERROR_FILE_READ,
         "Erreur de lecture du fichier",
         "Error while reading file"},
    { CC90HFE_ERROR_FILE_WRITE,
         "Erreur d'écriture du fichier",
         "Error while writing file"},
    { CC90HFE_ERROR_TRACK_READ,
         "Erreur de lecture d'une piste",
         "Error while reading a track"},
    { CC90HFE_ERROR_PORT_NONE,
          "Aucun port série valide n'a été trouvé.\n\n" \
          "Vérifiez que le Thomson est allumé, " \
          "que l'interface série a été correctement\n" \
          "connectée et que CC90 ou " \
          "INSTALL tourne sur votre Thomson",
          "No valid serial port has been found.\n\n" \
          "Be assured that the Thomson is switch on, that " \
          "the serial interface \n" \
           "has been correctly connected and that CC90 or " \
           "INSTALL is running on your Thomson" },
    { CC90HFE_ERROR_SERIAL_OPEN,
         "Impossible d'ouvrir le port série",
         "Can not open serial port" },
    { CC90HFE_ERROR_SERIAL_IO,
         "Erreur de transfert série",
         "Serial transfer error" },

    { 0 ,
         "Erreur inconnue",
         "Unknown error"}
};
/* ------------------------------------------------------------------------- */


/* error_Message:
 *  Renvoie une erreur générée par le programme.
 */
int error_Message(int error, const char moreinfo[])
{
    int i = 0;
    char *msg = NULL;

    if (error < CC90HFE_ERROR)
    {
        while ((error_table[i].err != 0)
            && (error_table[i].err != error))
               i++;
    
        error_msg = std_free (error_msg);
        if (is_fr)
            msg = error_table[i].fr_str;
        else
            msg = error_table[i].en_str;

        if (moreinfo == NULL)
            error_msg = std_strdup_printf ("%s.", msg);
        else
            error_msg = std_strdup_printf ("%s : %s", msg, moreinfo);
    }
    return CC90HFE_ERROR;
}

