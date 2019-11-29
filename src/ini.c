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
 *  Module     : ini.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 24/09/2001
 *  Modifié par: Eric Botcazou 26/10/2003
 *               François Mouret 28/01/2010 14/09/2011 17/01/2012 25/04/2012
 *                               28/09/2012 10/05/2014
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
#include <errno.h>

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
    int  init;
};


static FILE *file = NULL;
static struct STRING_LIST *list_start = NULL;

static const struct INI_LIST ini_entry[] = {
    /* SECTION    KEY                 TYPE       REGISTER                       INIT  */
    { "teo"     , "default_folder"  , IARG_DIR , &teo.default_folder           , 0     },
    /* ------------------------------------------------------------------------------ */
    { "settings", "exact_speed"     , IARG_BOOL, &teo.setting.exact_speed      , TRUE  },
    { "settings", "interlaced_video", IARG_BOOL, &teo.setting.interlaced_video , FALSE },
    { "settings", "sound_volume"    , IARG_INT , &teo.setting.sound_volume     , 128   },
    { "settings", "sound_enabled"   , IARG_BOOL, &teo.setting.sound_enabled    , TRUE  },
    /* ------------------------------------------------------------------------------ */
    { "memo"    , "file"            , IARG_STR , &teo.memo.file                , 0     },
    { "memo"    , "label"           , IARG_STR , &teo.memo.label               , 0     },
    /* ------------------------------------------------------------------------------ */
    { "cass"    , "file"            , IARG_STR , &teo.cass.file                , 0     },
    { "cass"    , "write_protect"   , IARG_BOOL, &teo.cass.write_protect       , TRUE  },
    /* ------------------------------------------------------------------------------ */
    { "disk0"   , "file"            , IARG_STR , &teo.disk[0].file             , 0     },
    { "disk0"   , "side"            , IARG_INT , &teo.disk[0].side             , 0     },
    { "disk0"   , "write_protect"   , IARG_BOOL, &teo.disk[0].write_protect    , FALSE },
    /* ------------------------------------------------------------------------------ */
    { "disk1"   , "file"            , IARG_STR , &teo.disk[1].file             , 0     },
    { "disk1"   , "side"            , IARG_INT , &teo.disk[1].side             , 0     },
    { "disk1"   , "write_protect"   , IARG_BOOL, &teo.disk[1].write_protect    , FALSE },
    /* ------------------------------------------------------------------------------ */
    { "disk2"   , "file"            , IARG_STR , &teo.disk[2].file             , 0     },
    { "disk2"   , "side"            , IARG_INT , &teo.disk[2].side             , 0     },
    { "disk2"   , "write_protect"   , IARG_BOOL, &teo.disk[2].write_protect    , FALSE },
    /* ------------------------------------------------------------------------------ */
    { "disk3"   , "file"            , IARG_STR , &teo.disk[3].file             , 0     },
    { "disk3"   , "side"            , IARG_INT , &teo.disk[3].side             , 0     },
    { "disk3"   , "write_protect"   , IARG_BOOL, &teo.disk[3].write_protect    , FALSE },
    /* ------------------------------------------------------------------------------ */
    { "printer" , "folder"          , IARG_DIR , &teo.lprt.folder              , 0     },
    { "printer" , "number"          , IARG_INT , &teo.lprt.number              , 55    },
    { "printer" , "dip"             , IARG_BOOL, &teo.lprt.dip                 , FALSE },
    { "printer" , "nlq"             , IARG_BOOL, &teo.lprt.nlq                 , FALSE },
    { "printer" , "raw_output"      , IARG_BOOL, &teo.lprt.raw_output          , FALSE },
    { "printer" , "txt_output"      , IARG_BOOL, &teo.lprt.txt_output          , FALSE },
    { "printer" , "gfx_output"      , IARG_BOOL, &teo.lprt.gfx_output          , TRUE  },
    /* ------------------------------------------------------------------------------ */
    { "debug"   , "window_maximize" , IARG_BOOL, &teo.debug.window_maximize    , FALSE },
    { "debug"   , "window_x"        , IARG_INT , &teo.debug.window_x           , -1    },
    { "debug"   , "window_y"        , IARG_INT , &teo.debug.window_y           , -1    },
    { "debug"   , "window_width"    , IARG_INT , &teo.debug.window_width       , -1    },
    { "debug"   , "window_height"   , IARG_INT , &teo.debug.window_height      , -1    },
    { "debug"   , "breakpoint_1"    , IARG_INT , &teo.debug.breakpoint[0]      , -1    },
    { "debug"   , "breakpoint_2"    , IARG_INT , &teo.debug.breakpoint[1]      , -1    },
    { "debug"   , "breakpoint_3"    , IARG_INT , &teo.debug.breakpoint[2]      , -1    },
    { "debug"   , "breakpoint_4"    , IARG_INT , &teo.debug.breakpoint[3]      , -1    },
    { "debug"   , "breakpoint_5"    , IARG_INT , &teo.debug.breakpoint[4]      , -1    },
    { "debug"   , "breakpoint_6"    , IARG_INT , &teo.debug.breakpoint[5]      , -1    },
    { "debug"   , "breakpoint_7"    , IARG_INT , &teo.debug.breakpoint[6]      , -1    },
    { "debug"   , "breakpoint_8"    , IARG_INT , &teo.debug.breakpoint[7]      , -1    },
    { "debug"   , "breakpoint_9"    , IARG_INT , &teo.debug.breakpoint[8]      , -1    },
    { "debug"   , "breakpoint_10"   , IARG_INT , &teo.debug.breakpoint[9]      , -1    },
    { "debug"   , "breakpoint_11"   , IARG_INT , &teo.debug.breakpoint[10]     , -1    },
    { "debug"   , "breakpoint_12"   , IARG_INT , &teo.debug.breakpoint[11]     , -1    },
    { "debug"   , "breakpoint_13"   , IARG_INT , &teo.debug.breakpoint[12]     , -1    },
    { "debug"   , "breakpoint_14"   , IARG_INT , &teo.debug.breakpoint[13]     , -1    },
    { "debug"   , "breakpoint_15"   , IARG_INT , &teo.debug.breakpoint[14]     , -1    },
    { "debug"   , "breakpoint_16"   , IARG_INT , &teo.debug.breakpoint[15]     , -1    },
    { "debug"   , "memory_address"  , IARG_INT , &teo.debug.memory_address     , 0     },
    { "debug"   , "ram_number"      , IARG_INT , &teo.debug.ram_number         , 2     },
    { "debug"   , "mon_number"      , IARG_INT , &teo.debug.mon_number         , 0     },
    { "debug"   , "video_number"    , IARG_INT , &teo.debug.video_number       , 0     },
    { "debug"   , "cart_number"     , IARG_INT , &teo.debug.cart_number        , 0     },
    { "debug"   , "extra_first_line", IARG_INT , &teo.debug.extra_first_line   , 0     },
    /* ------------------------------------------------------------------------------ */
    { NULL      , NULL              , 0        , NULL                          , 0     }
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

    /*TODO: Remove unused name and std_ApplicationPath*/
    char *name = NULL;

//    name = std_ApplicationPath (APPLICATION_DIR, filename);
    name = (char *)filename;
    file = fopen(name, mode);
//    name = std_free (name);
    return file;
}





/* load_ini_file:
 *  Charge le fichier INI.
 */
static void load_ini_file(void)
{
    int  stop  = FALSE;
    char *line = NULL;
    char *fpath;

    fpath = std_GetFirstExistingConfigFile(INI_FILE_NAME);
    file_open(fpath, "r");

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
 *  Load the INI file.
 */
void ini_Load(void)
{
    int  i = 0;
    int  tmp_val;
    char *pval;
    char *p;
    char **s;
    int  *d;
  
    /* initialize structure */
    memset (&teo, 0x00, sizeof(struct EMUTEO));
    teo.sound_enabled = TRUE;
    teo.command = TEO_COMMAND_NONE;

    /* Load the ini file */
    load_ini_file();

    /* Read the ini file */
    for (i=0; ini_entry[i].section != NULL; i++) {
        /* initialize int and bool value */
        switch (ini_entry[i].type) {
        case IARG_BOOL :
        case IARG_INT  : d = (int*)ini_entry[i].ptr;
                         *d = ini_entry[i].init;
                         break;
        }
        /* get ini value */
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
    }
    /* Free the ini list */
    std_StringListFree (list_start);
}

/* ini_Save:
 *  Sauve le fichier INI.
 */
void ini_Save (void)
{
    int i;
    int res;
    char *p = NULL;
    char *key;
    int *d;
    char **s;

    char *fpath;
    char *sys_file, *user_file;

    /* Ouvre le fichier ini */
    user_file = NULL;
    fpath = std_getUserConfigDir();
    if(fpath){
        printf("%s: Got user config dir: %s\n", __FUNCTION__, fpath);
        user_file = std_strdup_printf("%s/%s", fpath, INI_FILE_NAME );
        fpath = std_free(fpath);
    }
    
    sys_file = NULL;
    fpath = std_getSystemConfigDir();
    if(fpath){
        printf("%s: Got sys config dir: %s\n", __FUNCTION__, fpath);
        sys_file = std_strdup_printf("%s/%s", fpath, INI_FILE_NAME );
        fpath = std_free(fpath);
    }

    if(user_file){
        printf("%s: User config file %s usable for writting using it\n", __FUNCTION__, user_file);
        file_open(user_file,  "w");
    }else{
        printf("%s: User config file %s NOT usable for writting, using system file %s instead\n", __FUNCTION__, user_file, sys_file);
        file_open(sys_file,  "w");
    }

    if (file == NULL){
        printf("%s: Couldn't open file for writing, won't be saving ini specs\n", __FUNCTION__);
        printf("Erro is: %d: %s\n", errno, strerror(errno));
        return;
    }
    fprintf (file, ";-----------------------------------------------------\n");
    fprintf (file, "; INI file generated by Teo %s\n", TEO_VERSION_STR);
    fprintf (file, ";-----------------------------------------------------\n");

    /* Sauvegarde le fichier ini */
    for (i=0; ini_entry[i].section != NULL; i++) {
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
    }
    fprintf (file, "\n");
    file = std_fclose (file);
    std_free(user_file);
    std_free(sys_file);
}
