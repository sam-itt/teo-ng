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
 *  Module     : teo.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 27/01/2010
 *  Modifié par: François Mouret 14/09/2011 17/01/2012 25/04/2012
 *                               28/09/2012
 *
 *  Chargement/Sauvegarde du fichier de configuration.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
   #include <sys/stat.h>
   #include <unistd.h>
#endif

#include "defs.h"
#include "teo.h"
#include "media/printer.h"
#include "std.h"


#define INI_FILE_NAME  "teo.ini"

enum {
    IARG_BOOL = 0,
    IARG_INT,
    IARG_STR,
    IARG_DIR
};

struct INI_LIST {
    char *section;
    char *key;
    int  type;
    void *ptr;
};

static FILE *file = NULL;
static struct STRING_LIST *list_start = NULL;
static const struct INI_LIST ini_entry[] = {
    { "teo"     , "default_folder"  , IARG_DIR , &teo.default_folder           },
    /* --------------------------------------------------------------------- */
    { "settings", "exact_speed"     , IARG_BOOL, &teo.setting.exact_speed      },
    { "settings", "interlaced_video", IARG_BOOL, &teo.setting.interlaced_video },
    { "settings", "sound_volume"    , IARG_INT , &teo.setting.sound_volume     },
    { "settings", "sound_enabled"   , IARG_BOOL, &teo.setting.sound_enabled    },
    /* --------------------------------------------------------------------- */
    { "memo"    , "file"            , IARG_STR , &teo.memo.file                },
    { "memo"    , "label"           , IARG_STR , &teo.memo.label               },
    /* --------------------------------------------------------------------- */
    { "cass"    , "file"            , IARG_STR , &teo.cass.file                },
    { "cass"    , "write_protect"   , IARG_BOOL, &teo.cass.write_protect       },
    /* --------------------------------------------------------------------- */
    { "disk"    , "file0"           , IARG_STR , &teo.disk[0].file             },
    { "disk"    , "write_protect0"  , IARG_BOOL, &teo.disk[0].write_protect    },
    { "disk"    , "file1"           , IARG_STR , &teo.disk[1].file             },
    { "disk"    , "write_protect1"  , IARG_BOOL, &teo.disk[1].write_protect    },
    { "disk"    , "file2"           , IARG_STR , &teo.disk[2].file             },
    { "disk"    , "write_protect2"  , IARG_BOOL, &teo.disk[2].write_protect    },
    { "disk"    , "file3"           , IARG_STR , &teo.disk[3].file             },
    { "disk"    , "write_protect3"  , IARG_BOOL, &teo.disk[3].write_protect    },
    /* --------------------------------------------------------------------- */
    { "printer" , "folder"          , IARG_DIR , &teo.lprt.folder              },
    { "printer" , "number"          , IARG_INT , &teo.lprt.number              },
    { "printer" , "dip"             , IARG_BOOL, &teo.lprt.dip                 },
    { "printer" , "nlq"             , IARG_BOOL, &teo.lprt.nlq                 },
    { "printer" , "raw_output"      , IARG_BOOL, &teo.lprt.raw_output          },
    { "printer" , "txt_output"      , IARG_BOOL, &teo.lprt.txt_output          },
    { "printer" , "gfx_output"      , IARG_BOOL, &teo.lprt.gfx_output          },
    { NULL      , NULL              , 0        , NULL                          }
};



/* value_pointer:
 *  Retourne une valeur selon la section et la clef.
 */
static char *value_pointer (char *section, char *key)
{
    size_t length;
    char *s = NULL;
    struct STRING_LIST *list = list_start;

    /* Cherche la section */
    s = std_strdup_printf ("[%s]", section);
    while ((list != NULL) && (list->str != NULL)
      && (strcmp(s, list->str) != 0))
        list = list->next;
    s = std_free(s);

    /* Passe la section */
    if (list != NULL)
        list = list->next;

    /* Cherche la clef */
    length = strlen(key);
    while ((list != NULL) && (list->str != NULL)
      && (*list->str != '[') && (memcmp (key, list->str, length) != 0))
        list = list->next;

    /* Cherche la valeur */
    if ((list != NULL) && (list->str != NULL)) {
        s = std_skpspc (list->str + length);
        s = (*s=='=') ? std_skpspc(s+1) : NULL;
    }

    return ((s!=NULL) && (*s!='\0')) ? s : NULL;
}



/* load_ini_file:
 *  Charge le fichier INI.
 */
static void load_ini_file(void)
{
    int  stop  = FALSE;
    char *line = NULL;

#ifdef DEBIAN_BUILD
    char *fname = NULL;

    file = NULL;
    fname = std_strdup_printf ("%s/.teo/%s", getenv("HOME"), INI_FILE_NAME);
    if ((fname != NULL) && ((file = fopen(fname, "r")) == NULL)) {
        fname = std_free (fname);
        fname = std_strdup_printf ("/usr/share/teo/%s", INI_FILE_NAME);
        if (fname != NULL)
            file = fopen(fname, "r");
    }
    fname = std_free (fname);
#else
    file=fopen(INI_FILE_NAME, "r");
#endif
    if ((file != NULL) && ((line = malloc (300+1)) != NULL)) {
        while ((stop == FALSE) && (fgets(line, 300, file) != NULL)) {
            std_rtrim (line);
            list_start = std_StringListAppend (list_start, line);
            stop = (list_start == NULL) ? TRUE : FALSE;
        }
    }
    file = std_fclose(file);
    line = std_free(line);
}


/* ------------------------------------------------------------------------- */


/* ini_Load:
 *  Charge le fichier INI.
 */
void ini_Load(void)
{
    int  i = 0;
    int  tmp_val;
    char *pval;
    char *p;
    char **s;
    int  *d;
  
    /* Initialise les paramètres par défaut de la structure générale */
    memset (&teo, 0x00, sizeof(struct EMUTEO));
    teo.sound_enabled = TRUE;
    teo.command = TEO_COMMAND_NONE;
    teo.lprt.number = 55;
    teo.lprt.gfx_output = TRUE;
    teo.setting.exact_speed = TRUE;
    teo.setting.sound_volume = 128;
    teo.setting.sound_enabled = TRUE;

    /* Charge le fichier ini */
    load_ini_file();

    /* Lit le fichier ini */
    while (ini_entry[i].section != NULL) {
        pval = value_pointer (ini_entry[i].section, ini_entry[i].key);
        if (pval != NULL) {
            switch (ini_entry[i].type) {
            case IARG_BOOL : d = (int*)ini_entry[i].ptr;
                             if (strcmp (pval,"yes") == 0) *d = TRUE ; else
                             if (strcmp (pval,"no" ) == 0) *d = FALSE;
                             break;
            case IARG_INT  : d = (int*)ini_entry[i].ptr;
                             tmp_val = (int)strtol (pval, &p, 0);
                             if (*p <= '\x20') *d = tmp_val;
                             break;
            case IARG_STR  : s = (char**)ini_entry[i].ptr;
                             *s = std_strdup_printf ("%s", pval);
                             break;
            case IARG_DIR :  s = (char**)ini_entry[i].ptr;
                             if (access (pval, F_OK) >= 0)
                                 *s = std_strdup_printf ("%s", pval);
                             break;
            }
        }
        i++;
    }

    /* Libère la mémoire occupée par le fichier ini */
    std_StringListFree (list_start);
}



/* ini_Free:
 *  Libère la mémoire des variables.
 */
void ini_Free (void)
{
    int i = 0;
    char **s = NULL;

    while (ini_entry[i].section != NULL) {
        switch (ini_entry[i].type) {
        case IARG_STR  :
        case IARG_DIR  : s = (char**)ini_entry[i].ptr;
                         *s = std_free (*s);
                         break;
        }
        i++;
    }
}

    

/* ini_Save:
 *  Sauve le fichier INI.
 */
void ini_Save (void)
{
    int i = 0;
    int res;
    char *p = NULL;
    char *key;
    int *d;
    char **s;

    /* Ouvre le fichier ini */
#ifdef DEBIAN_BUILD
    p = std_strdup_printf ("%s/.teo/%s", getenv("HOME"), INI_FILE_NAME);
    file = fopen(p, "w");
    p = std_free(p);
#else
    file = fopen(INI_FILE_NAME, "w");
#endif

    /* Sauvegarde le fichier ini */
    while ((file != NULL) && (ini_entry[i].section != NULL))
    {
        res = 0;

        if ((p == NULL) || (strcmp (p, ini_entry[i].section) != 0))
            if ((res = fprintf (file, "\n[%s]\n", ini_entry[i].section)) < 0)
                file = std_fclose (file);
        p = ini_entry[i].section;
            
        key = ini_entry[i].key;
        switch (ini_entry[i].type) {
        case IARG_STR  :
        case IARG_DIR  : s = (char**)ini_entry[i].ptr;
                         res = fprintf (file, "%s=%s\n", key, (*s==NULL)?"":*s);
                         break;
        case IARG_BOOL : d = (int*)ini_entry[i].ptr;
                         res = fprintf (file, "%s=%s\n", key, (*d==FALSE)?"no":"yes");
                         break;
        case IARG_INT  : d = (int*)ini_entry[i].ptr;
                         res = fprintf (file, "%s=%d\n", key, *d);
                         break;
        }
        if (res < 0)
            file = std_fclose (file);
        i++;
    }
    if (file!=NULL)
        fprintf (file, "\n");
    file = std_fclose (file);

    /* Libère la mémoire des variables */
    ini_Free ();
}
