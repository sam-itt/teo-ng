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
 *  Version    : 1.8.1
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
#include "to8.h"

struct CFG_LIST {
    char *line;
    struct CFG_LIST *next;
};

static struct CFG_LIST *list_start = NULL;



/* unload_cfg:
 *  Libère toute la mémoire de la liste.
 */
static void unload_cfg(void)
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



/* create_line:
 *  Insère une ligne dans la liste.
 */
static struct CFG_LIST *create_line (char *line)
{
    struct CFG_LIST *list = NULL;

    list = (struct CFG_LIST *)malloc (sizeof (struct CFG_LIST));
    if (list != NULL) {
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
static struct CFG_LIST *key_ptr (char *section, char *key, struct CFG_LIST **pList)
{
    struct CFG_LIST *list = list_start;

    while ((list != NULL)
        && (list->line != NULL)
        && ((*list->line != '[')
         || (strlen(list->line) < strlen(section)+2)
         || (list->line[strlen(section)+1] != ']')
         || (memcmp(section, list->line+1, strlen(section)) != 0)))
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
    struct CFG_LIST *pList = NULL;
    struct CFG_LIST *list = NULL;

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



/* delete_key:
 *  Efface une clef.
 */
static void delete_key(char *section, char *key)
{
    struct CFG_LIST *list = NULL;
    struct CFG_LIST *pList = NULL;

    list = key_ptr (section, key, &pList);
    if ((pList != NULL) && (list != NULL)) {
        pList->next = list->next;
        free (list);
    }
}



/* set_string:
 *  Enregistre une valeur selon la section et la clef.
 */
static void set_string(char *section, char *key, const char *value)
{
    struct CFG_LIST *list = NULL;
    struct CFG_LIST *pList = NULL;
    char line[300+1] = "";

    if (*value != '\0')
    {
#ifdef DJGPP
        (void)sprintf (line, "%s = %s", key, value);
#else
        (void)snprintf (line, 300, "%s = %s", key, value);
#endif
        list = key_ptr (section, key, &pList);
        if ((list != NULL) && (list->line != NULL))
        {
            list->line = realloc (list->line, strlen (line)+1);
            if (list->line != NULL)
            {
                *(list->line) = '\0';
#ifdef DJGPP
                (void) sprintf (list->line, "%s", line);
#else
                (void) snprintf (list->line, strlen(line)+1, "%s", line);
#endif
            }
        }
        else
        {
            list = create_line (line);
            if (pList != NULL)
            {
                pList->next = list;
            }
        }
    }
    else
        delete_key(section, key);
}



/* set_int:
 *  Enregistre un entier selon la section et la clef.
 */
static void set_int (char *section, char *key, int value)
{
    char string[10] = "";

    if (value != 0)
#ifdef DJGPP
        (void)sprintf (string, "%d", value);
#else
        (void)snprintf (string, 10, "%d", value);
#endif
    set_string(section, key, string);
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



/* set_bool:
 *  Enregistre un boléen selon la section et la clef.
 */
static void set_bool (char *section, char *key, int value)
{
    set_string(section, key, (value == TRUE) ? "yes" : "no");
}



/* get_bool:
 *  Lit un boléen selon la section et la clef.
 */
static int get_bool (char *section, char *key, int value)
{
    char *string = get_string(section, key, NULL);

    if (string != NULL)
    {
        if (strcmp (string, "yes") == 0) return TRUE;
        if (strcmp (string, "no") == 0) return FALSE;
    }
    return value;
}



/* load_cfg:
 *  Charge le fichier CFG.
 */
static void load_cfg(char *filename)
{
    int i;
    struct CFG_LIST *list = NULL;
    char line[300] = "";
    FILE *file;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";

    (void)snprintf (fname, MAX_PATH, "%s/.teo/%s", getenv("HOME"), filename);
    file=fopen(fname, "r");
    if (file == NULL)
    {
        (void)snprintf (fname, MAX_PATH, "/usr/share/teo/%s", filename);
        file=fopen(fname, "r");
    }
#else
    file=fopen(filename, "r");
#endif
    while (fgets(line, 300, file) != NULL)
    {
         /* Elimine les caractères de contrôle et les espaces de fin */
         i = strlen(line);
         if (i > 0)
             while ((i >= 0) && ((unsigned char)line[i] <= 0x20))
                 line[i--] = '\0';

        /* Enregistre la chaîne */
        if (list == NULL)
        {
            list_start = create_line(line);
            list = list_start;
        }
        else
        {
            list->next = create_line(line);
            list = list->next;
        }
        if (list == NULL)
        {
            fclose(file);
            return;
        }
    }
    fclose(file);
}



/* save_cfg:
 *  Sauve le fichier CFG.
 */
static void save_cfg(char *filename)
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
 *  Charge l'état sauvegardé de l'émulateur.
 */
void to8_LoadState(char *filename)
{
    char *p;

    load_cfg(filename);
    
    gui->loadstate_error = 0;

    gui->setting.exact_speed =  get_bool ("teo", "exact_speed", FALSE);
    gui->setting.interlaced_video =  get_bool ("teo", "interlaced_video", FALSE);
    if ((p = get_string ("teo", "memo_file", NULL)) != NULL)
        gui->loadstate_error += (to8_LoadMemo7(p) == TO8_ERROR) ? 1 : 0;
    if ((p = get_string ("teo", "k7_file", NULL)) != NULL)
        gui->loadstate_error += (to8_LoadK7(p) == TO8_ERROR) ? 1 : 0;
    if ((p = get_string ("teo", "disk_0_file", NULL)) != NULL)
        gui->loadstate_error += (to8_LoadDisk(0, p) == TO8_ERROR) ? 1 : 0;
    if ((p = get_string ("teo", "disk_1_file", NULL)) != NULL)
        gui->loadstate_error += (to8_LoadDisk(1, p) == TO8_ERROR) ? 1 : 0;
    if ((p = get_string ("teo", "disk_2_file", NULL)) != NULL)
        gui->loadstate_error += (to8_LoadDisk(2, p) == TO8_ERROR) ? 1 : 0;
    if ((p = get_string ("teo", "disk_3_file", NULL)) != NULL)
        gui->loadstate_error += (to8_LoadDisk(3, p) == TO8_ERROR) ? 1 : 0;
#ifdef DJGPP
    (void)sprintf (gui->lprt.folder, "%s", get_string ("teo", "printer_folder", ""));
#else
    (void)snprintf (gui->lprt.folder, MAX_PATH, "%s", get_string ("teo", "printer_folder", ""));
#endif

    gui->setting.sound_volume  = get_int ("teo", "sound_vol", 128);

    gui->cass.write_protect = get_bool ("teo", "k7_write_protect", TRUE);
    gui->disk[0].write_protect = get_bool ("teo", "disk_0_write_protect", FALSE);
    gui->disk[1].write_protect = get_bool ("teo", "disk_1_write_protect", FALSE);
    gui->disk[2].write_protect = get_bool ("teo", "disk_2_write_protect", FALSE);
    gui->disk[3].write_protect = get_bool ("teo", "disk_3_write_protect", FALSE);

    gui->lprt.number     = get_int  ("teo", "printer_number", 55);
    gui->lprt.dip        = get_bool ("teo", "printer_dip", FALSE);
    gui->lprt.nlq        = get_bool ("teo", "printer_nlq", FALSE);
    gui->lprt.raw_output = get_bool ("teo", "printer_raw_output", FALSE);
    gui->lprt.txt_output = get_bool ("teo", "printer_txt_output", FALSE);
    gui->lprt.gfx_output = get_bool ("teo", "printer_gfx_output", TRUE);

    if (access("autosave.img", F_OK) >= 0)
        to8_LoadImage("autosave.img");

    unload_cfg();
}



/* SaveState:
 *  Sauvegarde l'état de l'émulateur.
 */
void to8_SaveState (char *filename)
{
    load_cfg (filename);

    set_bool   ("teo", "exact_speed", gui->setting.exact_speed);
    set_bool   ("teo", "interlaced_video", gui->setting.interlaced_video);

    set_string ("teo", "memo_file", gui->memo.file);

    set_string ("teo", "k7_file", gui->cass.file);
    set_bool   ("teo", "k7_write_protect", gui->cass.write_protect);

    set_string ("teo", "disk_0_file", gui->disk[0].file);
    set_string ("teo", "disk_1_file", gui->disk[1].file);
    set_string ("teo", "disk_2_file", gui->disk[2].file);
    set_string ("teo", "disk_3_file", gui->disk[3].file);
    set_bool   ("teo", "disk_0_write_protect", gui->disk[0].write_protect);
    set_bool   ("teo", "disk_1_write_protect", gui->disk[1].write_protect);
    set_bool   ("teo", "disk_2_write_protect", gui->disk[2].write_protect);
    set_bool   ("teo", "disk_3_write_protect", gui->disk[3].write_protect);

    set_int    ("teo", "sound_vol", gui->setting.sound_volume);

    set_string ("teo", "printer_folder", gui->lprt.folder);
    set_int    ("teo", "printer_number", gui->lprt.number);
    set_bool   ("teo", "printer_dip", gui->lprt.dip);
    set_bool   ("teo", "printer_nlq", gui->lprt.nlq);
    set_bool   ("teo", "printer_raw_output", gui->lprt.raw_output);
    set_bool   ("teo", "printer_txt_output", gui->lprt.txt_output);
    set_bool   ("teo", "printer_gfx_output", gui->lprt.gfx_output);

    to8_SaveImage ("autosave.img");

    save_cfg (filename);

    unload_cfg();
}


#if 0
    to8_SaveImage(get_string("teo", "autosave", "autosave.img"));


    delete_key ("teo", "load_error");
    if (load_error != 0)
        set_string("teo", "load_error", "yes");
#endif


#if 0
    str_p = get_string("teo", "load_error", "no");
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
#endif


