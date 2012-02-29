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
 *  Copyright (C) 1997-2011 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/ustate.c
 *  Version    : 1.8.0
 *  Créé par   : François Mouret 27/01/2010
 *  Modifié par: François Mouret 14/09/2011
 *
 *  Module de chargement/sauvegarde de l'état de l'émulateur.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <sys/stat.h>
   #include <unistd.h>
#endif

#include "intern/defs.h"
#include "linux/main.h"
#include "to8.h"

enum {
    LIST_READ,
    LIST_WRITE,
    LIST_DELETE
};

struct CFG_LIST {
    char *line;
    struct CFG_LIST *next;
};

static struct CFG_LIST *list_start = NULL;


/* UnloadIni:
 * Libère toute la mémoire de la liste.
 */
static void UnloadCfg(void)
{
    struct CFG_LIST *list = list_start;
    struct CFG_LIST *next = NULL;

    while(list != NULL)
    {
        next = list->next;
        if (list->line != NULL)
            free(list->line);
        free(list);
        list = next;
    }
}



/* InsertConfigLine:
 * Insère une ligne dans la liste
 */
static struct CFG_LIST *InsertConfigLine(struct CFG_LIST *list_prev, char *line)
{
    struct CFG_LIST *list = NULL;

    if ((list = (struct CFG_LIST *)malloc (sizeof (struct CFG_LIST))) == NULL)
        return NULL;
    list->next = NULL;

    if ((list->line = (char *)malloc (strlen(line)+1)) == NULL) {
        free (list);
        return NULL; }
    strcpy (list->line, line);

    if (list_prev == NULL)
        list_start = list;
    else {
        list->next = list_prev->next;
        list_prev->next = list; }

    return list;
}



/* SkipSpaces:
 *  Passe les espaces préalables d'une string.
 */
static char *SkipSpaces(char *ptr)
{
    while ((*ptr <= 0x20) && (*ptr != '\0'))
        ptr++;
    return ptr;
}
    


/* FindCongigList:
 *  Pointe sur la CFG_LIST selon la section et la clef.
 */
static struct CFG_LIST *FindConfigList(char *section, char *key, int mode)
{
    struct CFG_LIST *list = list_start;
    struct CFG_LIST *list_last = NULL;
    struct CFG_LIST *list_prev = NULL;
    char line[50] = "";

    /* Recherche la section */
    while ((list != NULL)
        && ((list->line[0] != '[')
         || (strncmp(section, list->line+1, strlen(section)) != 0)
         || (list->line[strlen(section)+1] != ']')))
        list = list->next;

    if (list == NULL)
        return NULL;

    /* Recherche la clef */
    while (strncmp(key, list->line, strlen(key)) != 0)
    {
        list_prev = list;
        if (list->line[0] != '\0') list_last = list;
        list = list->next;

        if ((list == NULL)
         || (list->line[0] == '['))
        {
            if (mode == LIST_WRITE) {
                (void)snprintf(line, 50, "%s = ", key);
                list_last->next = InsertConfigLine(list_last, line);
                return list_last->next; }

            return NULL;
        }
    }

    if (mode == LIST_DELETE) {
        list_prev->next = list->next;
        free(list->line);
        free(list);
        list = NULL; }

    return list;
}



/* FindCongigString:
 *  Pointe sur la string selon la section et la clef.
 */
static char *FindConfigValuePtr(char *section, char *key, int mode)
{
    static char *value_ptr = NULL;
    struct CFG_LIST *list = FindConfigList(section, key, mode);

    if (list == NULL)
        return NULL;

    /* Positionne sur la valeur */
    value_ptr = SkipSpaces(list->line + strlen(key));
    if (*(value_ptr++) != '=')
        return NULL;

    value_ptr = SkipSpaces(value_ptr);
    if (*value_ptr < 0x20)
        return NULL;

    return value_ptr;
}



/* GetConfigString:
 * Retourne une valeur CHAR selon la section et la clef
 */
static char *GetConfigString(char *section, char *key, char *init_value)
{
    char *value_ptr = FindConfigValuePtr(section, key, LIST_READ);

    return ((value_ptr == NULL) || (value_ptr[0] == '\0')) ? init_value : value_ptr;
}



/* SetConfigString:
 * Enregistre une valeur CHAR selon la section et la clef
 */
static void SetConfigString(char *section, char *key, const char *value)
{
    size_t new_size = strlen(key)+3+strlen(value)+1;
    struct CFG_LIST *list = NULL;

    if ((value == NULL) || (value[0] == '\0'))
        (void)FindConfigList(section, key, LIST_DELETE);
    else
    {
        list = FindConfigList(section, key, LIST_WRITE);

        if (list == NULL) return;

        free(list->line);
        list->line = malloc(new_size);
        (void)snprintf(list->line, new_size, "%s = %s", key, value);
    }
}



/* LoadCfg:
 *  Charge le fichier CFG.
 */
static void LoadCfg(char *filename)
{
    struct CFG_LIST *list = NULL;
    char line[300] = "";
    char *ptr;
    FILE *file;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";

    (void)snprintf (fname, MAX_PATH, "%s/.teo/%s", getenv("HOME"), filename);
    if (access (fname, F_OK) < 0)
        (void)snprintf (fname, MAX_PATH, "/usr/share/teo/%s", filename);
    file=fopen(fname, "r");
#else
    file=fopen(filename, "r");
#endif
    while (fgets(line, 300, file) != NULL)
    {
         /* Elimine les caractères de contrôle et les espaces de fin */
         ptr = line+strlen(line);
         while((*ptr < ' ') && (ptr >= line))
            *(ptr--) = '\0';

        /* Enregistre la chaîne */
        if ((list = InsertConfigLine(list, line)) == NULL) {
            fclose(file);
            return; }
    }
    fclose(file);
}



/* SaveIni:
 *  Sauve le fichier CFG.
 */
static void SaveCfg(char *filename)
{
    FILE *file;
    struct CFG_LIST *list = list_start;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";

    (void)snprintf (fname, MAX_PATH, "%s/.teo/%s", getenv("HOME"), filename);
    file=fopen(fname, "w");
#else
    file = fopen(filename, "w");
#endif
    for (; list!=NULL; list=list->next)
        fprintf(file, "%s\n", list->line);

    fclose(file);
}



/* LoadStateFirst:
 *  Charge l'état sauvegardé de l'émulateur (widgets).
 */
void LoadState(char *filename)
{
    const char *str_p;

    LoadCfg(filename);

    str_p = GetConfigString("teo", "speed", "exact");
    if (strcmp(str_p, "fast") == 0)
        teo.exact_speed = FALSE;

    str_p = GetConfigString("teo", "memo7", NULL);
    if (str_p)
        to8_LoadMemo7(str_p);

    str_p = GetConfigString("teo", "k7", NULL);
    if (str_p)
        to8_LoadK7(str_p);

    str_p = GetConfigString("teo", "disk_0", NULL);
    if (str_p)
        to8_LoadDisk(0, str_p);

    str_p = GetConfigString("teo", "disk_1", NULL);
    if (str_p)
        to8_LoadDisk(1, str_p);

    str_p = GetConfigString("teo", "disk_2", NULL);
    if (str_p)
        to8_LoadDisk(2, str_p);

    str_p = GetConfigString("teo", "disk_3", NULL);
    if (str_p)
        to8_LoadDisk(3, str_p);

    str_p = GetConfigString("teo", "autosave", "autosave.img");
    if (str_p)
        to8_LoadImage(str_p);
    
    UnloadCfg();
}



/* SaveState:
 *  Sauvegarde l'état de l'émulateur.
 */
void SaveState(char *filename)
{
    LoadCfg(filename);

    SetConfigString("teo", "speed", teo.exact_speed ? "exact" : "fast");

    SetConfigString("teo", "memo7", to8_GetMemo7Filename());

    SetConfigString("teo", "k7", to8_GetK7Filename());

    SetConfigString("teo", "disk_0", to8_GetDiskFilename(0));

    SetConfigString("teo", "disk_1", to8_GetDiskFilename(1));

    SetConfigString("teo", "disk_2", to8_GetDiskFilename(2));

    SetConfigString("teo", "disk_3", to8_GetDiskFilename(3));

    to8_SaveImage(GetConfigString("teo", "autosave", "autosave.img"));

    SaveCfg(filename);

    UnloadCfg();
}

