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
 *  Module     : linux/ustate.c
 *  Version    : 1.8.1
 *  Créé par   : François Mouret 27/01/2010
 *  Modifié par: François Mouret 14/09/2011 17/01/2012
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
   #include <gtk/gtk.h>
#endif

#include "intern/defs.h"
#include "linux/main.h"
#include "to8.h"

struct CFG_LIST {
    char *line;
    struct CFG_LIST *next;
};

static struct CFG_LIST *list_start = NULL;
static int load_error = 0;



/* UnloadIni:
 * Libère toute la mémoire de la liste.
 */
static void UnloadCfg(void)
{
    struct CFG_LIST *next = NULL;

    while(list_start != NULL) {
        next = list_start->next;
        if (list_start->line != NULL)
            free(list_start->line);
        free(list_start);
        list_start = next;
    }
}



/* CreateConfigLine:
 * Insère une ligne dans la liste
 */
static struct CFG_LIST *CreateConfigLine (char *line)
{
    struct CFG_LIST *list = NULL;

    list = (struct CFG_LIST *)malloc (sizeof (struct CFG_LIST));
    if (list != NULL) {
        list->next = NULL;
        list->line = (char *)malloc (strlen(line)+1);
        if (list->line != NULL) {
            *(list->line) = '\0';
            (void) snprintf (list->line, strlen(line)+1, "%s", line);
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


/* KeyPtr:
 *  Recherche le pointeur sur la clef de la section.
 */
static struct CFG_LIST *KeyPtr (char *section, char *key, struct CFG_LIST **pList)
{
    struct CFG_LIST *list = list_start;

    while ((list != NULL)
        && (list->line != NULL)
        && ((*list->line != '[')
         || (strlen(list->line) < strlen(section)+2)
         || (list->line[strlen(section)+1] != ']')
         || (memcmp(section, list->line+1, strlen(section)) != 0)))
        list = list->next;
    if (list != NULL) {
        *pList = list;
        list = list->next;
    }
    while ((list != NULL)
        && (list->line != NULL)
        && (*list->line != '[')
        && (memcmp (key, list->line, strlen(key)) != 0)) {
        if (isgraph ((int)*list->line) != 0)
            *pList = list;
        list = list->next;
    }
    return list;
}



/* ValuePtr:
 * Retourne une valeur selon la section et la clef
 */
static char *ValuePtr(char *section, char *key, char *init_value)
{
    static char *value_ptr = NULL;
    static char *p = NULL;
    struct CFG_LIST *pList = NULL;
    struct CFG_LIST *list = NULL;

    value_ptr = NULL;
    list = KeyPtr (section, key, &pList);
    if ((list != NULL) && (list->line != NULL)) {
        p = skpspc (list->line + strlen(key));
        if (*p == '=')
            value_ptr = skpspc (p+1);
    }
    return ((value_ptr == NULL) || (*value_ptr == '\0')) ? init_value : value_ptr;
}



/* DeleteKey:
 * Efface une clef
 */
static void DeleteKey(char *section, char *key)
{
    struct CFG_LIST *list = NULL;
    struct CFG_LIST *pList = NULL;

    list = KeyPtr (section, key, &pList);
    if ((pList != NULL) && (list != NULL)) {
        pList->next = list->next;
        free (list);
    }
}



/* SetValue:
 * Enregistre une valeur selon la section et la clef
 */
static void SetValue(char *section, char *key, const char *value)
{
    struct CFG_LIST *list = NULL;
    struct CFG_LIST *pList = NULL;
    char line[300+1] = "";

    if (*value != '\0') {
        (void)snprintf (line, 300, "%s = %s", key, value);
        list = KeyPtr (section, key, &pList);
        if ((list != NULL) && (list->line != NULL)) {
            list->line = realloc (list->line, strlen (line)+1);
            if (list->line != NULL) {
                *(list->line) = '\0';
                (void) snprintf (list->line, strlen(line)+1, "%s", line);
            }
        } else {
            list = CreateConfigLine (line);
            if (pList != NULL) {
                pList->next = list;
            }
        }
    }
    else
        DeleteKey(section, key);
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
    file=fopen(fname, "r");
    if (file == NULL) {
        (void)snprintf (fname, MAX_PATH, "/usr/share/teo/%s", filename);
        file=fopen(fname, "r");
    }
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
        if (list == NULL) {
            list_start = CreateConfigLine(line);
            list = list_start;
        } else {
            list->next = CreateConfigLine(line);
            list = list->next;
        }
        if (list == NULL) {
            fclose(file);
            return;
        }
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
    GtkWidget *dialog;
    const char *str_p;

    LoadCfg(filename);

    str_p = ValuePtr("teo", "speed", "exact");
    if (strcmp(str_p, "fast") == 0)
        teo.exact_speed = FALSE;

    str_p = ValuePtr("teo", "memo7", NULL);
    if (str_p)
        load_error += (to8_LoadMemo7(str_p) == TO8_ERROR) ? 1 : 0;

    str_p = ValuePtr("teo", "k7", NULL);
    if (str_p)
        load_error += (to8_LoadK7(str_p) == TO8_ERROR) ? 1 : 0;

    str_p = ValuePtr("teo", "disk_0", NULL);
    if (str_p)
        load_error += (to8_LoadDisk(0, str_p) == TO8_ERROR) ? 1 : 0;

    str_p = ValuePtr("teo", "disk_1", NULL);
    if (str_p)
        load_error += (to8_LoadDisk(1, str_p) == TO8_ERROR) ? 1 : 0;

    str_p = ValuePtr("teo", "disk_2", NULL);
    if (str_p)
        load_error += (to8_LoadDisk(2, str_p) == TO8_ERROR) ? 1 : 0;

    str_p = ValuePtr("teo", "disk_3", NULL);
    if (str_p)
        load_error += (to8_LoadDisk(3, str_p) == TO8_ERROR) ? 1 : 0;

    str_p = ValuePtr("teo", "autosave", "autosave.img");
    if ((str_p) && (load_error == 0))
        to8_LoadImage(str_p);

    str_p = ValuePtr("teo", "load_error", "no");
    if (strcmp(str_p, "yes") == 0)
        to8_Reset();

    if (load_error != 0) {
        dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                 is_fr?"Un fichier de configuration n'a pas pu Ãªtre " \
                       "chargÃ©. VÃ©rifiez qu'il n'a pas Ã©tÃ© dÃ©placÃ©, " \
                       "dÃ©truit et que le pÃ©riphÃ©rique a bien Ã©tÃ© montÃ©."
                      :"A configuration file was unable to be loaded. " \
                       "Check if this file has been moved, deleted and that " \
                       "the device has been successfully mounted."
                 );
        gtk_window_set_title (GTK_WINDOW(dialog), is_fr?"Teo - Message":"Teo - Warning");
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
    }

    UnloadCfg();
}



/* SaveState:
 *  Sauvegarde l'état de l'émulateur.
 */
void SaveState(char *filename)
{
    LoadCfg(filename);

    SetValue("teo", "speed", teo.exact_speed ? "exact" : "fast");

    SetValue("teo", "memo7", to8_GetMemo7Filename());

    SetValue("teo", "k7", to8_GetK7Filename());

    SetValue("teo", "disk_0", to8_GetDiskFilename(0));

    SetValue("teo", "disk_1", to8_GetDiskFilename(1));

    SetValue("teo", "disk_2", to8_GetDiskFilename(2));

    SetValue("teo", "disk_3", to8_GetDiskFilename(3));

    to8_SaveImage(ValuePtr("teo", "autosave", "autosave.img"));

    DeleteKey ("teo", "load_error");
    if (load_error != 0)
        SetValue("teo", "load_error", "yes");

    SaveCfg(filename);

    UnloadCfg();
}

