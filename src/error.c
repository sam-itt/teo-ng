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
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 22/11/2000
 *  Modifié par: Eric Botcazou 06/03/2001
 *               François Mouret 08/2011 13/01/2012
 *
 *  Gestion des erreurs générées par l'émulateur.
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

static const struct ERROR_TABLE error_table_fr[]= {
#ifdef UNIX_TOOL
    { TEO_ERROR_MULTIPLE_INIT           , "Initialisation multiple de l'Ã©mulateur." },
    { TEO_ERROR_BAD_ALLOC               , "Allocation d'espace impossible."          },
    { TEO_ERROR_CANNOT_FIND_FILE        , "Impossible de trouver "                   },
    { TEO_ERROR_CANNOT_OPEN_FILE        , "Ouverture impossible."                    },
    { TEO_ERROR_BAD_FILE_FORMAT         , "Mauvais format de fichier."               },
    { TEO_ERROR_UNSUPPORTED_MODEL       , "Image d'un modÃ¨le non supportÃ©."        },
    { TEO_ERROR_BAD_JOYSTICK_NUM        , "Nombre de joysticks incorrect."           },
    { TEO_ERROR_BAD_MEMO_HEADER_CHECKSUM, "Checksum d'en-tÃªte de memo erronÃ©."     },
    { TEO_ERROR_BAD_MEMO_HEADER_NAME    , "Nom d'en-tÃªte de memo incorrect."        },
    { TEO_ERROR_MEDIA_ALREADY_SET       , "Plus de media libre."                     },
    { TEO_ERROR_CANNOT_CREATE_DISK      , "Impossible de crÃ©er la disquette."       },
    { TEO_ERROR_SAP_NOT_VALID           , "Archive SAP invalide."                    },
    { TEO_ERROR_FILE_TOO_LARGE          , "Fichier trop important."                  },
    { TEO_ERROR_FILE_IS_EMPTY           , "Fichier vide."                            },
    { TEO_ERROR_SAP_DIRECTORY_FULL      , "RÃ©pertoire SAP plein"                    },
    { 0                                 , "Erreur inconnue."                         }
#else
    { TEO_ERROR_MULTIPLE_INIT           , "Initialisation multiple de l'émulateur."  },
    { TEO_ERROR_BAD_ALLOC               , "Allocation d'espace impossible."          },
    { TEO_ERROR_CANNOT_FIND_FILE        , "Impossible de trouver "                   },
    { TEO_ERROR_CANNOT_OPEN_FILE        , "Ouverture impossible."                    },
    { TEO_ERROR_BAD_FILE_FORMAT         , "Mauvais format de fichier."               },
    { TEO_ERROR_UNSUPPORTED_MODEL       , "Image d'un modèle non supporté."          },
    { TEO_ERROR_BAD_JOYSTICK_NUM        , "Nombre de joysticks incorrect."           },
    { TEO_ERROR_BAD_MEMO_HEADER_CHECKSUM, "Checksum d'en-tête de memo erroné."       },
    { TEO_ERROR_BAD_MEMO_HEADER_NAME    , "Nom d'en-tête de memo incorrect."         },
    { TEO_ERROR_MEDIA_ALREADY_SET       , "Plus de media libre."                     },
    { TEO_ERROR_CANNOT_CREATE_DISK      , "Impossible de créer la disquette."        },
    { TEO_ERROR_SAP_NOT_VALID           , "Archive SAP invalide."                    },
    { TEO_ERROR_FILE_TOO_LARGE          , "Fichier trop important."                  },
    { TEO_ERROR_FILE_IS_EMPTY           , "Fichier vide."                            },
    { TEO_ERROR_SAP_DIRECTORY_FULL      , "Répertoire SAP plein"                     },
    { 0                                 , "Erreur inconnue."                         }
#endif
};

static const struct ERROR_TABLE error_table_en[]= {
    { TEO_ERROR_MULTIPLE_INIT           , "Multiple initialization of the emulator." },
    { TEO_ERROR_BAD_ALLOC               , "Cannot allocate space."                   },
    { TEO_ERROR_CANNOT_FIND_FILE        , "Unable to find "                          },
    { TEO_ERROR_CANNOT_OPEN_FILE        , "Unable to open."                          },
    { TEO_ERROR_BAD_FILE_FORMAT         , "Bad file format."                         },
    { TEO_ERROR_UNSUPPORTED_MODEL       , "Unsupported image format."                },
    { TEO_ERROR_BAD_JOYSTICK_NUM        , "Bad count of joysticks."                  },
    { TEO_ERROR_BAD_MEMO_HEADER_CHECKSUM, "Bad checksum for memo header."            },
    { TEO_ERROR_BAD_MEMO_HEADER_NAME    , "Bad name for memo header."                },
    { TEO_ERROR_MEDIA_ALREADY_SET       , "Media not free."                          },
    { TEO_ERROR_CANNOT_CREATE_DISK      , "Cannot create disk."                      },
    { TEO_ERROR_SAP_NOT_VALID           , "SAP archive not valid."                   },
    { TEO_ERROR_FILE_TOO_LARGE          , "File toot large."                         },
    { TEO_ERROR_FILE_IS_EMPTY           , "File is empty."                           },
    { TEO_ERROR_SAP_DIRECTORY_FULL      , "Directory SAP full."                      },
    { 0                                 , "Unknown error."                           }
};



/* error_Message:
 *  Renvoie une erreur générée par l'émulateur.
 */
int error_Message(int error, const char moreinfo[])
{
    int i = 0;
    const struct ERROR_TABLE *err_table = is_fr? error_table_fr
                                               : error_table_en;
    if (error < TEO_ERROR)
    {
        while ((err_table[i].err != 0)
           && (err_table[i].err != error))
            i++;
    
        teo_error_msg = std_free (teo_error_msg);
        
#ifdef DJGPP
        teo_error_msg = std_strdup_printf ("%s.", err_table[i].str);
#else
        if (moreinfo == NULL)
            teo_error_msg = std_strdup_printf ("%s.", err_table[i].str);
        else
            teo_error_msg = std_strdup_printf ("%s : %s", err_table[i].str, moreinfo);
#endif
    }
    return TEO_ERROR;
}

