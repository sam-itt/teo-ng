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

#include "intern/defs.h"
#include "intern/printer.h"
#include "intern/std.h"
#include "intern/xargs.h"
#include "to8.h"


#define INI_FILE_NAME  "teo.ini"

#define Sc_TEO       "[teo]"
#define Sc_SETTINGS  "[settings]"
#define Sc_MEMO      "[memo]"
#define Sc_CASS      "[cass]"
#define Sc_DISK0     "[disk0]"
#define Sc_DISK1     "[disk1]"
#define Sc_DISK2     "[disk2]"
#define Sc_DISK3     "[disk3]"
#define Sc_PRINTER   "[printer]"

static struct STRING_LIST *list_start = NULL;
static FILE *file = NULL;
static int ini_load_error = 0;


/* pString:
 *  Retourne une valeur selon la section et la clef.
 */
static char *pString (char *section, char *key)
{
    size_t length;
    char *p = NULL;
    struct STRING_LIST *list = list_start;

    /* Cherche la section */
    length = strlen(section);
    while ((list != NULL) && (list->str != NULL)
      && (memcmp(section, list->str, length) != 0))
        list = list->next;

    /* Passe la section */
    if (list != NULL) list = list->next;

    /* Cherche la clef */
    length = strlen(key);
    while ((list != NULL) && (list->str != NULL)
      && (*list->str != '[') && (memcmp (key, list->str, length) != 0))
        list = list->next;

    /* Cherche la valeur */
    if ((list != NULL) && (list->str != NULL)) {
        p = std_skpspc (list->str + length);
        p = (*p=='=') ? std_skpspc(p+1) : NULL;
    }

    return ((p!=NULL) && (*p!='\0')) ? p : NULL;
}



/* getInt:
 *  Lit un entier selon la section et la clef.
 */
static void getInt (char *section, char *key, int *val)
{
    char *p;
    int tmp_val;
    char *s = pString(section, key);

    if (s!=NULL) {
        tmp_val = (int)strtol (s, &p, 0);
        if (*p <= '\x20')
            *val = tmp_val;
    }
}



/* getBool:
 *  Read a boolean.
 */
static void getBool (char *section, char *key, int *val)
{
    char *s = pString(section, key);

    if (s!=NULL) {
        if (strcmp (s,"yes") == 0) *val = TRUE;
        else
        if (strcmp (s,"no" ) == 0) *val = FALSE;
    }
}



/* getDir:
 *  Read a dir name.
 */
static void getDir (char *section, char *key, char **ptr)
{
    char *s = pString(section, key);

    if (s!=NULL) {
        if (std_isdir (s))
            *ptr = std_strdup_printf ("%s", s);
        else
            ini_load_error++;
    }
}


#if 0
/* getFile:
 *  Read a file name.
 */
static void getFile (char *section, char *key, char **ptr)
{
    char *s = pString(section, key);

    if (s!=NULL) {
        if (std_isfile(s) == TRUE)
            *ptr = std_strdup_printf ("%s", s);
        else
            ini_load_error++;
    }
}
#endif


/* load_ini_file:
 *  Charge le fichier INI.
 */
static void load_ini_file(void)
{
    int go_on = TRUE;
    char *line = NULL;

#ifdef DEBIAN_BUILD
    char *fname = NULL;

    file = NULL;
    fname = std_strdup_printf ("%s/.teo/%s", getenv("HOME"), INI_FILE_NAME);
    if ((fname != NULL) && ((file = fopen(fname, "r")) == NULL))
    {
        std_free (fname);
        fname = std_strdup_printf ("/usr/share/teo/%s", INI_FILE_NAME);
        if (fname != NULL) file = fopen(fname, "r");
    }
    std_free (fname);
#else
    file=fopen(INI_FILE_NAME, "r");
#endif

    if (file != NULL)
    {
        line = malloc (300+1);
        if (line != NULL)
        {
            while ((go_on == TRUE) && (fgets(line, 300, file) != NULL))
            {    
                /* Nettoie la chaîne */
                std_rtrim (line);

                /* Enregistre la chaîne */
                list_start = std_StringListAppend (list_start, line);

                go_on = (list_start!=NULL)?TRUE:FALSE;
            }
            free (line);
        }
        fclose(file);
    }
}



static void putSection (char *section) {
    if ((file!=NULL) && (fprintf (file, "%s\n", section) < 0))
        file = std_fclose (file);
}

static void putBool (char *key, int boolean) {
    if ((file!=NULL)
     && (fprintf(file, "%s=%s\n", key, (boolean==FALSE) ? "no" : "yes") < 0))
        file = std_fclose (file);
}

static void putInt (char *key, int value) {
    if ((file!=NULL) && (fprintf (file, "%s=%d\n", key, value) < 0))
        file = std_fclose (file);
}

static void putDir (char *key, char *dir_name) {
    if ((file!=NULL)
     && (fprintf (file, "%s=%s\n", key, (dir_name==NULL)?"":dir_name) < 0))
        file = std_fclose (file);
}

static void putFile (char *key, char *file_name) {
    if ((file!=NULL)
     && (fprintf (file, "%s=%s\n", key, (file_name==NULL)?"":file_name) < 0))
        file = std_fclose (file);
}

static void putBlank (void) {
    if ((file!=NULL) && (fprintf (file, "\n") < 0))
        file = std_fclose (file);
}


/*===========================================================================*/


/* ini_Load:
 *  Charge l'état sauvegardé de l'émulateur.
 */
int ini_Load(void)
{
    ini_load_error = 0;
  
    /* Initialise la structure TEO */
    memset (&teo, 0x00, sizeof(struct EMUTEO));

    /* Initialise les paramètres par défaut */
    teo.sound_enabled = TRUE;
    teo.command = NONE;
    teo.lprt.number = 55;
    teo.lprt.gfx_output = TRUE;
    teo.setting.exact_speed = TRUE;
    teo.setting.sound_enabled = TRUE;

    /* Charge le fichier de configuration */
    load_ini_file();
    /*-----------------------------------------------------------------------*/
    getDir  (Sc_TEO     , "default_folder"  , &teo.default_folder);
    /*-----------------------------------------------------------------------*/
    getBool (Sc_SETTINGS, "exact_speed"     , &teo.setting.exact_speed);
    getBool (Sc_SETTINGS, "interlaced_video", &teo.setting.interlaced_video);
    getInt  (Sc_SETTINGS, "sound_volume"    , &teo.setting.sound_volume);
    getBool (Sc_SETTINGS, "sound_enabled"   , &teo.setting.sound_enabled);
    /*-----------------------------------------------------------------------*/
//    getFile (Sc_MEMO    , "file"            , &xarg->memo);
    /*-----------------------------------------------------------------------*/
//    getFile (Sc_CASS    , "file"            , &xarg->cass);
    getBool (Sc_CASS    , "write_protect"   , &teo.cass.write_protect);
    /*-----------------------------------------------------------------------*/
//    getFile (Sc_DISK0   , "file"            , &xarg->disk[0]);
    getBool (Sc_DISK0   , "write_protect"   , &teo.disk[0].write_protect);
    /*-----------------------------------------------------------------------*/
//    getFile (Sc_DISK1   , "file"            , &xarg->disk[1]);
    getBool (Sc_DISK1   , "write_protect"   , &teo.disk[1].write_protect);
    /*-----------------------------------------------------------------------*/
//    getFile (Sc_DISK2   , "file"            , &xarg->disk[2]);
    getBool (Sc_DISK2   , "write_protect"   , &teo.disk[2].write_protect);
    /*-----------------------------------------------------------------------*/
//    getFile (Sc_DISK3   , "file"            , &xarg->disk[3]);
    getBool (Sc_DISK3   , "write_protect"   , &teo.disk[3].write_protect);
    /*-----------------------------------------------------------------------*/
    getDir  (Sc_PRINTER , "folder"          , &teo.lprt.folder);
    getInt  (Sc_PRINTER , "number"          , &teo.lprt.number);
    getBool (Sc_PRINTER , "dip"             , &teo.lprt.dip);
    getBool (Sc_PRINTER , "nlq"             , &teo.lprt.nlq);
    getBool (Sc_PRINTER , "raw_output"      , &teo.lprt.raw_output);
    getBool (Sc_PRINTER , "txt_output"      , &teo.lprt.txt_output);
    getBool (Sc_PRINTER , "gfx_output"      , &teo.lprt.gfx_output);
    /*-----------------------------------------------------------------------*/
    std_StringListFree (list_start);

    return ini_load_error;
}



/* ini_Save:
 *  Sauvegarde l'état de l'émulateur.
 */
void ini_Save (void)
{
#ifdef DEBIAN_BUILD
    char *fname=std_strdup_printf ("%s/.teo/%s", getenv("HOME"), INI_FILE_NAME);
    file = fopen(fname, "w");
    fname = std_free(fname);
#else
    file = fopen(INI_FILE_NAME, "w");
#endif
    /*-----------------------------------------------------------------------*/
    putSection (Sc_TEO);
        putDir   ("default_folder"  , teo.default_folder);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_SETTINGS);
        putBool  ("exact_speed"     , teo.setting.exact_speed);
        putBool  ("interlaced_video", teo.setting.interlaced_video);
        putInt   ("sound_volume"    , teo.setting.sound_volume);
        putBool  ("sound_enabled"   , teo.setting.sound_enabled);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_MEMO);
        putFile  ("file"            , teo.memo.file);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_CASS);
        putFile  ("file"            , teo.cass.file);
        putBool  ("write_protect"   , teo.cass.write_protect);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_DISK0);
        putFile  ("file"            , teo.disk[0].file);
        putBool  ("write_protect"   , teo.disk[0].write_protect);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_DISK1);
        putFile  ("file"            , teo.disk[1].file);
        putBool  ("write_protect"   , teo.disk[1].write_protect);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_DISK2);
        putFile  ("file"            , teo.disk[2].file);
        putBool  ("write_protect"   , teo.disk[2].write_protect);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_DISK3);
        putFile  ("file"            , teo.disk[3].file);
        putBool  ("write_protect"   , teo.disk[3].write_protect);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    putSection (Sc_PRINTER);
        putDir   ("folder"          , teo.lprt.folder);
        putInt   ("number"          , teo.lprt.number);
        putBool  ("dip"             , teo.lprt.dip);
        putBool  ("nlq"             , teo.lprt.nlq);
        putBool  ("raw_output"      , teo.lprt.raw_output);
        putBool  ("txt_output"      , teo.lprt.txt_output);
        putBool  ("gfx_output"      , teo.lprt.gfx_output);
        putBlank ();
    /*-----------------------------------------------------------------------*/
    file = std_fclose (file);

    /* Libère la mémoire de la structure INI */
    teo.default_folder = std_free (teo.default_folder);
    teo.lprt.folder    = std_free (teo.lprt.folder);
    teo.disk[0].file   = std_free (teo.disk[0].file);
    teo.disk[1].file   = std_free (teo.disk[1].file);
    teo.disk[2].file   = std_free (teo.disk[2].file);
    teo.disk[3].file   = std_free (teo.disk[3].file);
    teo.cass.file      = std_free (teo.cass.file);
    teo.memo.file      = std_free (teo.memo.file);
    teo.memo.label     = std_free (teo.memo.label);
    teo.imag.file      = std_free (teo.imag.file);
}

