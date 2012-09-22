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
 *  Module     : state.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 27/01/2010
 *  Modifié par: François Mouret 14/09/2011 17/01/2012 25/04/2012
 *
 *  Chargement/Sauvegarde de l'état de l'émulateur.
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
#include "intern/gui.h"
#include "to8.h"


#define INI_FILE_NAME  "teo.ini"

struct INI_LIST {
    char *line;
    struct INI_LIST *next;
};

static struct INI_LIST *list_start = NULL;


/* unload_ini:
 *  Libère toute la mémoire de la liste.
 */
static void unload_ini(void)
{
    struct INI_LIST *next = NULL;

    while(list_start != NULL)
    {
        next = list_start->next;

        if (list_start->line != NULL)
            free(list_start->line);

        free(list_start);
        list_start = next;
    }
}



/* add_line:
 *  Ajoute une ligne dans la liste.
 */
static struct INI_LIST *add_line (char *line)
{
    struct INI_LIST *list = NULL;

    list = (struct INI_LIST *)malloc (sizeof (struct INI_LIST));
    if (list != NULL)
    {
        list->next = NULL;
        list->line = (char *)malloc (strlen(line)+1);
        if (list->line != NULL) {
            *(list->line) = '\0';
#ifdef DJGPP
            (void) sprintf (list->line, "%s", line);
#else
            (void) snprintf (list->line, strlen(line)+1, "%s", line);
#endif
        } else {
            free (list);
            list = NULL;
        }
    }
    return list;
}



/* skpspc:
 *  Passe les espaces préalables d'une string.
 */
static char *skpspc(char *p)
{
    while ((*p <= 0x20) && (*p != '\0'))
        p++;
    return p;
}



/* key_ptr:
 *  Recherche le pointeur sur la clef de la section.
 */
static struct INI_LIST *key_ptr (char *section, char *key, struct INI_LIST **pList)
{
    struct INI_LIST *list = list_start;

    while ((list != NULL)
        && (list->line != NULL)
        && (memcmp(section, list->line, strlen(section)) != 0))
        list = list->next;

    if (list != NULL)
    {
        *pList = list;
        list = list->next;
    }

    while ((list != NULL)
        && (list->line != NULL)
        && (*list->line != '[')
        && (memcmp (key, list->line, strlen(key)) != 0))
   {
        if (isgraph ((int)*list->line) != 0)
            *pList = list;
        list = list->next;
    }
    return list;
}



/* get_string:
 *  Retourne une valeur selon la section et la clef.
 */
static char *get_string(char *section, char *key, char *init_value)
{
    static char *value_ptr = NULL;
    static char *p = NULL;
    struct INI_LIST *pList = NULL;
    struct INI_LIST *list = NULL;

    value_ptr = NULL;
    list = key_ptr (section, key, &pList);
    if ((list != NULL) && (list->line != NULL))
    {
        p = skpspc (list->line + strlen(key));
        if (*p == '=')
            value_ptr = skpspc (p+1);
    }
    return ((value_ptr == NULL) || (*value_ptr == '\0')) ? init_value : value_ptr;
}



/* get_int:
 *  Lit un entier selon la section et la clef.
 */
static int get_int (char *section, char *key, int value)
{
    char *string = get_string(section, key, NULL);
    char *p;
    int temp_value;

    if (string != NULL)
    {
        temp_value = (int)strtol (string, &p, 0);
        if (*p <= '\x20')
            value = temp_value;
    }
    return value;
}




/* get_bool:
 *  Lit un boléen selon la section et la clef.
 */
static int get_bool (char *section, char *key, int value)
{
    char *string = get_string(section, key, NULL);

    if (string != NULL)
    {
        if (strcmp (string, "yes") == 0)
           return TRUE;

        if (strcmp (string, "no") == 0)
           return FALSE;
    }
    return value;
}



/* load_ini:
 *  Charge le fichier INI.
 */
static FILE *load_ini(void)
{
    int i;
    int stop = 0;
    struct INI_LIST *list = NULL;
    char line[300] = "";
    static FILE *file;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";

    (void)snprintf (fname, MAX_PATH, "%s/.teo/%s", getenv("HOME"), INI_FILE_NAME);
    file=fopen(fname, "r");
    if (file == NULL)
    {
        (void)snprintf (fname, MAX_PATH, "/usr/share/teo/%s", INI_FILE_NAME);
        file=fopen(fname, "r");
    }
#else
    file=fopen(INI_FILE_NAME, "r");
#endif
    if (file != NULL)
    {
        while ((stop == 0) && (fgets(line, 300, file) != NULL))
        {    
            /* Elimine les caractères de contrôle et les espaces de fin */
            i = strlen(line);
            if (i > 0)
                while ((i >= 0) && ((unsigned char)line[i] <= 0x20))
                    line[i--] = '\0';

            /* Enregistre la chaîne */
            if (list == NULL)
            {
                list_start = add_line(line);
                list = list_start;
            }
            else
            {
                list->next = add_line(line);
                list = list->next;
            }
            if (list == NULL)
                stop = 1;
        }
        fclose(file);
    }
    return file;
}



/* LoadState:
 *  Charge l'état sauvegardé de l'émulateur.
 */
int to8_LoadState(void)
{
    struct stat st;
    char *p;
    int load_error = 0;

    if (load_ini() == NULL)
        return load_error;
    
    gui->setting.exact_speed =  get_bool ("[settings]", "exact_speed", TRUE);
    gui->setting.interlaced_video = get_bool ("[settings]", "interlaced_video", FALSE);
    gui->setting.sound_volume = get_int ("[settings]", "sound_volume", 128);
    gui->setting.sound_enabled = get_bool ("[settings]", "sound_enabled", TRUE);

    if ((p = get_string ("[m7]", "file", NULL)) != NULL)
        load_error += (to8_LoadMemo7(p) == TO8_ERROR) ? 1 : 0;

    gui->cass.write_protect = get_bool ("[k7]", "write_protect", TRUE);
    if ((p = get_string ("[k7]", "file", NULL)) != NULL)
        load_error += (to8_LoadK7(p) == TO8_ERROR) ? 1 : 0;

    gui->disk[0].write_protect = get_bool("[disk0]", "write_protect", FALSE);
    if ((p = get_string ("[disk0]", "file", NULL)) != NULL)
        load_error += (to8_LoadDisk(0, p) == TO8_ERROR) ? 1 : 0;

    gui->disk[1].write_protect = get_bool("[disk1]", "write_protect", FALSE);
    if ((p = get_string ("[disk1]", "file", NULL)) != NULL)
        load_error += (to8_LoadDisk(1, p) == TO8_ERROR) ? 1 : 0;

    gui->disk[2].write_protect = get_bool("[disk2]", "write_protect", FALSE);
    if ((p = get_string ("[disk2]", "file", NULL)) != NULL)
        load_error += (to8_LoadDisk(2, p) == TO8_ERROR) ? 1 : 0;

    gui->disk[3].write_protect = get_bool("[disk3]", "write_protect", FALSE);
    if ((p = get_string ("[disk3]", "file", NULL)) != NULL)
        load_error += (to8_LoadDisk(3, p) == TO8_ERROR) ? 1 : 0;

    gui->lprt.number     = get_int  ("[printer]", "number", 55);
    gui->lprt.dip        = get_bool ("[printer]", "dip", FALSE);
    gui->lprt.nlq        = get_bool ("[printer]", "nlq", FALSE);
    gui->lprt.raw_output = get_bool ("[printer]", "raw_output", FALSE);
    gui->lprt.txt_output = get_bool ("[printer]", "txt_output", FALSE);
    gui->lprt.gfx_output = get_bool ("[printer]", "gfx_output", TRUE);
#ifdef DJGPP
    (void)sprintf (gui->lprt.folder, "%s",
                   get_string ("[printer]", "folder", ""));
#else
    (void)snprintf (gui->lprt.folder, MAX_PATH, "%s",
                    get_string ("[printer]", "folder", ""));
#endif
    if((stat(gui->lprt.folder, &st) < 0) || (S_ISDIR(st.st_mode) == 0))
       *gui->lprt.folder = '\0';
#ifdef DJGPP
    (void)sprintf (gui->default_folder, "%s",
                   get_string ("[teo]", "folder", ""));
#else
    (void)snprintf (gui->default_folder, MAX_PATH, "%s",
                    get_string ("[teo]", "folder", ""));
#endif
    if((stat(gui->default_folder, &st) < 0) || (S_ISDIR(st.st_mode) == 0))
       *gui->default_folder = '\0';

    if (load_error)
        to8_ColdReset();
    else
        if (access("autosave.img", F_OK) >= 0)
            to8_LoadImage("autosave.img");

    unload_ini();

    return load_error;
}



/* SaveState:
 *  Sauvegarde l'état de l'émulateur.
 */
void to8_SaveState (void)
{
    FILE *file;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";

    (void)snprintf (fname, MAX_PATH, "%s/.teo/%s", getenv("HOME"), INI_FILE_NAME);
    file=fopen(fname, "w");
#else
    file = fopen(INI_FILE_NAME, "w");
#endif

    if (file == NULL)
        return;

    fprintf (file, "[teo]\n");
    fprintf (file, "folder=%s\n", gui->default_folder);
    fprintf (file, "\n");

    fprintf (file, "[settings]\n");
    fprintf (file, "exact_speed=%s\n", (gui->setting.exact_speed) ? "yes" : "no");
    fprintf (file, "interlaced_video=%s\n", (gui->setting.interlaced_video) ? "yes" : "no");
    fprintf (file, "sound_volume=%d\n", gui->setting.sound_volume);
    fprintf (file, "sound_enabled=%s\n", (gui->setting.sound_enabled) ? "yes" : "no");
    fprintf (file, "\n");

    fprintf (file, "[m7]\n");
    fprintf (file, "file=%s\n", gui->memo.file);
    fprintf (file, "\n");

    fprintf (file, "[k7]\n");
    fprintf (file, "file=%s\n", gui->cass.file);
    fprintf (file, "write_protect=%s\n", (gui->cass.write_protect) ? "yes" : "no");
    fprintf (file, "\n");

    fprintf (file, "[disk0]\n");
    fprintf (file, "file=%s\n", gui->disk[0].file);
    fprintf (file, "write_protect=%s\n", (gui->disk[0].write_protect) ? "yes" : "no");
    fprintf (file, "\n");
    
    fprintf (file, "[disk1]\n");
    fprintf (file, "file=%s\n", gui->disk[1].file);
    fprintf (file, "write_protect=%s\n", (gui->disk[1].write_protect) ? "yes" : "no");
    fprintf (file, "\n");
    
    fprintf (file, "[disk2]\n");
    fprintf (file, "file=%s\n", gui->disk[2].file);
    fprintf (file, "write_protect=%s\n", (gui->disk[2].write_protect) ? "yes" : "no");
    fprintf (file, "\n");
    
    fprintf (file, "[disk3]\n");
    fprintf (file, "file=%s\n", gui->disk[3].file);
    fprintf (file, "write_protect=%s\n", (gui->disk[3].write_protect) ? "yes" : "no");
    fprintf (file, "\n");
    
    fprintf (file, "[printer]\n");
    fprintf (file, "folder=%s\n", gui->lprt.folder);
    fprintf (file, "number=%d\n", gui->lprt.number);
    fprintf (file, "dip=%s\n", (gui->lprt.dip) ? "yes" : "no");
    fprintf (file, "nlq=%s\n", (gui->lprt.nlq) ? "yes" : "no");
    fprintf (file, "raw_output=%s\n", (gui->lprt.raw_output) ? "yes" : "no");
    fprintf (file, "txt_output=%s\n", (gui->lprt.txt_output) ? "yes" : "no");
    fprintf (file, "gfx_output=%s\n", (gui->lprt.gfx_output) ? "yes" : "no");
    fprintf (file, "\n");

    fclose (file);

    to8_SaveImage ("autosave.img");
}

