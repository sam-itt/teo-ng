/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2018 François Mouret
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
 *  Module     : ini.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 27/07/2013 14/10/2013 31/05/2015
 *
 *  Management of INI file.
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
#include "std.h"
#include "main.h"
#include "ini.h"
#include "serial.h"


#define INI_FILE_NAME  "cc90hfe.ini"

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
    { "cc90hfe", "default_folder"    , IARG_STR , &gui.default_folder      },
    { "cc90hfe", "archive_folder"    , IARG_STR , &gui.archive_folder      },
    { "cc90hfe", "extract_folder"    , IARG_STR , &gui.extract_folder      },
    { "cc90hfe", "side-0-not-thomson", IARG_BOOL, &gui.not_thomson_side[0] },
    { "cc90hfe", "side-1-not-thomson", IARG_BOOL, &gui.not_thomson_side[1] },
    { "cc90hfe", "read-retry-max"    , IARG_INT , &gui.read_retry_max      },
    { "serial" , "port-name"         , IARG_STR , &gui.port_name           },
    { "serial" , "timeout"           , IARG_INT , &gui.timeout             },
    { NULL     , NULL                , 0        , NULL                     }
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



static FILE *file_open (const char filename[], const char mode[])
{
    char *name = std_ApplicationPath (APPLICATION_DIR, filename);

    file = fopen(name, mode);
    name = std_free (name);
    return file;
}



/* load_ini_file:
 *  Charge le fichier INI.
 */
static void load_ini_file (void)
{
    int  stop  = FALSE;
    char *line = NULL;

    file_open(INI_FILE_NAME, "rb");

    if (file != NULL)
    {
        if ((line = malloc (300+1)) != NULL)
        {
            while ((stop == FALSE) && (fgets(line, 300, file) != NULL))
            {
                std_rtrim (line);
                list_start = std_StringListAppend (list_start, line);
                stop = (list_start == NULL) ? TRUE : FALSE;
            }
            line = std_free (line);
        }
        file = std_fclose (file);
    }
}


/* ------------------------------------------------------------------------- */


/* ini_Load:
 *  Charge le fichier INI.
 */
void ini_Load (void)
{
    int  i = 0;
    int  tmp_val;
    char *pval;
    char *p;
    char **s;
    int  *d;
  
    /* Charge le fichier ini */
    load_ini_file();

    /* Lit le fichier ini */
    while (ini_entry[i].section != NULL)
    {
        pval = value_pointer (ini_entry[i].section, ini_entry[i].key);
        if (pval != NULL)
        {
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



/* ini_Save:
 *  Sauve le fichier INI et libère la mémoire des chaînes.
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
    file_open(INI_FILE_NAME, "wb");
    if (file == NULL)
        return;

    /* Sauvegarde le fichier ini */
    fprintf (file, ";--------------------------------------------------;\r\n");
    fprintf (file, "; Generated by %s %s.%s.%s\r\n", PROG_NAME, PROG_VERSION_MAJOR, PROG_VERSION_MINOR, PROG_VERSION_MICRO);
    fprintf (file, ";--------------------------------------------------;\r\n");
    while ((file != NULL) && (ini_entry[i].section != NULL))
    {
        res = 0;

        if ((p == NULL) || (strcmp (p, ini_entry[i].section) != 0))
            if ((res = fprintf (file, "\r\n[%s]\r\n", ini_entry[i].section)) < 0)
                file = std_fclose (file);
        p = ini_entry[i].section;
            
        key = ini_entry[i].key;
        switch (ini_entry[i].type) {
        case IARG_STR  :
        case IARG_DIR  : s = (char**)ini_entry[i].ptr;
                         res = fprintf (file, "%s=%s\r\n", key, (*s==NULL)?"":*s);
                         *s = std_free (*s);
                         break;
        case IARG_BOOL : d = (int*)ini_entry[i].ptr;
                         res = fprintf (file, "%s=%s\r\n", key, (*d==FALSE)?"no":"yes");
                         break;
        case IARG_INT  : d = (int*)ini_entry[i].ptr;
                         res = fprintf (file, "%s=%d\r\n", key, *d);
                         break;
        }
        if (res < 0)
            file = std_fclose (file);
        i++;
    }
    if (file != NULL)
        fprintf (file, "\r\n");
    file = std_fclose (file);
}

