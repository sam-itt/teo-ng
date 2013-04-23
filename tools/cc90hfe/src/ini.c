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
 *  Module     : ini.c
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of INI file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#include "defs.h"
#include "std.h"
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
    { "folder" , "default"   , IARG_STR , &gui.default_folder },
    { "folder" , "archive"   , IARG_STR , &gui.archive_folder },
    { "folder" , "extract"   , IARG_STR , &gui.extract_folder },
    { "window" , "x"         , IARG_INT , &gui.window_x       },
    { "window" , "y"         , IARG_INT , &gui.window_y       },
    { "serial" , "port_name" , IARG_STR , &gui.port_name      },
    { "serial" , "timeout"   , IARG_INT , &gui.timeout        },
    { NULL     , NULL        , 0        , NULL                }
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
    char *name = NULL;

#ifdef DEBIAN_BUILD
    name = std_strdup_printf ("%s/.local/share/applications/teo/%s", getenv("HOME"), filename);
#else
    name = std_strdup_printf ("%s", filename);
#endif
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
    fprintf (file, ";-----------------------------------------------;\n");
    fprintf (file, "; cc90hfe ini file                              ;\n");
    fprintf (file, ";-----------------------------------------------;\n");
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
                         *s = std_free (*s);
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
    if (file != NULL)
        fprintf (file, "\n");
    file = std_fclose (file);
}

