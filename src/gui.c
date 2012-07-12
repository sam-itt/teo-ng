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
 *                          Jérémie Guillaume, François Mouret,
 *                          Samuel Devulder
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
 *  Module     : gui.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 26/04/2012
 *  Modifié par:
 *
 *  Gestion de la GUI
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
#include "intern/errors.h"
#include "intern/gui.h"
#include "to8.h"

struct THOMSON_GUI *gui = NULL;



/* gui_StringListLast:
 *  Renvoit le pointeur sur le dernier élément de la stringlist.
 */
static struct STRING_LIST *gui_StringListLast (struct STRING_LIST *p)
{
    for (; p!=NULL; p=p->next)
        if (p->next==NULL)
            break;
    return p;
}



/* bad_alloc:
 *  Libère la mémoire occupée par la GUI et retourne une erreur.
 */
static int bad_alloc (void)
{
   gui_Free ();
   return ErrorMessage(TO8_BAD_ALLOC, NULL);
}



/* gui_Free:
 *  Libère la mémoire occupée par la GUI commune.
 */
void gui_Free (void)
{
    int i;

    if (gui == NULL)
        return;

    if (gui->default_folder != NULL)
    {
        free (gui->default_folder);
        gui->default_folder = NULL;
    }

    if (gui->lprt.folder != NULL)
    {
        free (gui->lprt.folder);
        gui->lprt.folder = NULL;
    }

    for (i=0; i<NBDRIVE; i++)
    {
        if (gui->disk[i].file != NULL)
        {
            free (gui->disk[i].file);
            gui->disk[i].file = NULL;
        }
    }

    if (gui->cass.file != NULL)
    {
        free (gui->cass.file);
        gui->cass.file = NULL;
    }

    if (gui->memo.file != NULL)
    {
        free (gui->memo.file);
        gui->memo.file = NULL;
    }

    if (gui->memo.label != NULL)
    {
        free (gui->memo.label);
        gui->memo.label = NULL;
    }

    if (gui->imag.file != NULL)
    {
        free (gui->imag.file);
        gui->imag.file = NULL;
    }

    free (gui);
    gui = NULL;
}

    

/* gui_Init:
 *  Initialise la GUI commune.
 */
int gui_Init (void)
{
    int i;

    if ((gui = (struct THOMSON_GUI *)calloc (sizeof(struct THOMSON_GUI), 1)) == NULL)
        return (bad_alloc());

    if ((gui->default_folder = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    if ((gui->lprt.folder = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    for (i=0; i<NBDRIVE; i++)
        if ((gui->disk[i].file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
            return (bad_alloc());

    if ((gui->cass.file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    if ((gui->memo.file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    if ((gui->memo.label = (char *)calloc (TO8_MEMO7_LABEL_LENGTH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    if ((gui->imag.file = (char *)calloc (MAX_PATH + 1, sizeof(char))) == NULL)
        return (bad_alloc());

    gui->setting.exact_speed = TRUE;
    
    return TO8_OK;
}



/* gui_StringListIndex:
 *  Renvoit l'index de l'élément de la stringlist.
 */
int gui_StringListIndex (struct STRING_LIST *p, char *str)
{
    int index;

    for (index=0; p!=NULL; p=p->next,index++)
        if (p->str!=NULL)
            if (strcmp (p->str, str) == 0)
                break;
    return (p==NULL)?-1:index;
}



/* gui_StringListText:
 *  Renvoit le pointeur du texte de l'élément de la stringlist.
 */
char *gui_StringListText (struct STRING_LIST *p, int index)
{
    for (;index>0;index--)
    {
        if (p!=NULL)
            p=p->next;
    }
    return (p!=NULL)?p->str:NULL;
}



/* gui_StringListAppend:
 *  Ajoute un élément à la stringlist.
 */
struct STRING_LIST *gui_StringListAppend (struct STRING_LIST *p, char *str)
{
    struct STRING_LIST *last_str = gui_StringListLast (p);
    struct STRING_LIST *new_str = calloc (1, sizeof (struct STRING_LIST));

    if (new_str!=NULL)
    {
        new_str->str=malloc (strlen (str)+1);
        if (new_str->str!=NULL)
        {
            *new_str->str='\0';
            strcpy (new_str->str, str);
        }
    }
    if ((last_str!=NULL) && (last_str->str!=NULL))
        last_str->next=new_str;

    return (p==NULL)?new_str:p;
}



/* gui_StringListFree:
 *  Libère la mémoire de la stringlist.
 */
void gui_StringListFree (struct STRING_LIST *p)
{
    struct STRING_LIST *next;

    while (p!=NULL)
    {
        next=p->next;
        if (p->str!=NULL)
            free (p->str);
        free (p);
        p=next;
    }
}



/* gui_BaseName:
 *  Retourne le nom du fichier à partir du nom complet du fichier spécifié.
 */
char* gui_BaseName(char *fullname)
{
   int len = strlen(fullname);

   while (--len > 0)
      if ((fullname[len] == '\\') || (fullname[len] == '/'))
         return fullname + len + 1;

   return fullname;
}



/* gui_LastDir:
 *  Retourne le nom du dernier répertoire à partir du nom complet du fichier spécifié.
 */
char* gui_LastDir(char *fullname)
{
   int len = strlen(fullname);

   while ((len > 0) && ((fullname[len-1] == '\\') || (fullname[len-1] == '/')))
       fullname[--len] = '\0';

   while (--len > 0)
      if ((fullname[len] == '\\') || (fullname[len] == '/'))
         return fullname + len + 1;

   return fullname;
}



/* gui_CleanPath:
 *  Efface le nom de fichier du chemin de fichier.
 */
void gui_CleanPath (char *filename)
{
   char *fname_pos = strrchr (filename, '\\');

   if (fname_pos == NULL)
       fname_pos = strrchr (filename, '/');
   if (fname_pos != NULL)
       *fname_pos = '\0';
}
