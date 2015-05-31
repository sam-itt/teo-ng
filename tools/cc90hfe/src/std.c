/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2015 François Mouret
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
 *  Module     : std.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Utility functions.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
   #include <sys/stat.h>
   #include <unistd.h>
   #include <stdarg.h>
#endif

#include "defs.h"
#include "main.h"
#include "std.h"



/* std_StringListLast:
 *  Renvoit le pointeur sur le dernier élément de la stringlist.
 */
static struct STRING_LIST *std_StringListLast (struct STRING_LIST *p)
{
    while ((p!=NULL) && (p->next!=NULL))
        p=p->next;

    return p;
}



/* std_strdup_sprintf_add:
 *  Concatenate two strings.
 *
 *  String memory is dynamically allocated.
 *  Must be MSDOS compatible.
 *  Return pointer to the concatenated string.
 */
static char *std_strdup_printf_add (char *p0, char *p1, int length)
{
    char *p = NULL;
    int size;

    if ((p0 != NULL) && (p1 != NULL))
    {
        size = strlen (p0);
        p = calloc (1, size+length+1);
        if (p != NULL)
        {
            strcpy  (p, p0);
            strncat (p, p1, length);
        }
        free (p0);
    }
    return p;
}



/* std_strdup_printf_run:
 *  Strings concatenation.
 *
 *  String memory is dynamically allocated.
 *  Must be MSDOS compatible, that's why vsnprintf isn't used.
 *  Only for char*, char, and int.
 */
static char *std_strdup_printf_run (char *fmt, va_list ap) 
{
    int  i;
    int  d;
    char c;
    char *s;
    char *buf = calloc(1,33);
    char *ptr = calloc(1,1);
    char fill_char = '\0';
    char fill_size = 0;
    char *fmt_tmp = calloc(1,33);
    
    while ((ptr != NULL)
        && (buf != NULL)
        && (fmt != NULL)
        && (*fmt != '\0')
        && ((ptr = std_strdup_printf_add(ptr, fmt, strcspn (fmt, "%"))) != NULL))
    {
        fmt += strcspn (fmt, "%");
        if (*fmt == '%')
        {
            fmt++;

            if ((*(fmt) == '0') || (*(fmt) == ' '))
            {
                fill_char = *fmt;
                fmt++;
            }

            if ((*(fmt) >= '1') && (*(fmt) <= '9'))
                fill_size = (int)strtol (fmt, &fmt, 0);

            switch (*fmt)
            {
                /* Chaîne */
                case 's': s = va_arg (ap, char *);
                          ptr = std_strdup_printf_add(ptr, s, (s)?strlen(s):0);
                          break;

                /* Entier */
                case 'd': d = va_arg (ap, int);
                          sprintf (fmt_tmp, "%%d");
                          if (fill_size != 0)
                          {
                              if (fill_char != '\0')
                                  sprintf (fmt_tmp, "%%%c%dd", fill_char, fill_size);
                              else
                                  sprintf (fmt_tmp, "%%%dd", fill_size);
                          }
                          i = sprintf (buf, fmt_tmp, d);
                          
                          ptr = std_strdup_printf_add(ptr, buf, i);
                          break;

                /* Caractère, % */
                case '%':
                case 'c': c = va_arg (ap, int);
                          buf[0] = (char)c;
                          buf[1] = '\0';
                          ptr = std_strdup_printf_add(ptr, buf, 1);
                          break;

                /* Passe */
                default : break;
            }
            fill_char = '\0';
            fill_size = 0;
            fmt++;
        }
    }
    buf = std_free (buf);
    if (ptr == NULL)
        ptr = calloc(1,1);

    return ptr;
}

/* ------------------------------------------------------------------------- */


/* std_free:
 *  Libère une mémoire.
 */
void *std_free (void *p)
{
    if (p != NULL)
        free (p);

    return NULL;
}



/* std_stralloc:
 *  Alloue de la mémoire pour une chaîne.
 */
void *std_stralloc (void *p, char *s)
{
    p = std_free (p);
    p = calloc (1, strlen(s)+1);
    if (p != NULL) strcpy (p, s);
    return p;
}



/* std_fclose:
 *  Referme un fichier.
 */
FILE *std_fclose (FILE *fp)
{
    if (fp != NULL)
        fclose (fp);
    return NULL;
}



/* std_IsFile:
 *  Vérifie si le chemin est un fichier.
 */
int std_IsFile (const char filename[])
{
    struct stat st;

    return ((stat(filename, &st) == 0)
         && (S_ISREG(st.st_mode) != 0)) ? TRUE : FALSE;
}



/* std_IsDir:
 *  Vérifie si le chemin est un répertoire.
 */
int std_IsDir (const char filename[])
{
    struct stat st;

    return ((stat(filename, &st) == 0)
         && (S_ISDIR(st.st_mode) != 0)) ? TRUE : FALSE;
}



/* std_FileSize:
 *  Retourne la taille d'un fichier.
 */
size_t std_FileSize (const char filename[])
{
    struct stat st;
 
    return (stat(filename, &st) == 0) ? (size_t)st.st_size : 0;
}



/* std_rtrim:
 *  Elimine les caractères de contrôle en fin de chaîne.
 */
void std_rtrim (char *s)
{
    if (s != NULL)
    {
        while ((unsigned char)*s >= 0x20)
            s++;
        *s ='\0';
    }
}



/* std_skpspc:
 *  Passe les espaces dans une chaîne.
 */
char *std_skpspc(char *p)
{
    if (p != NULL)
        while (((unsigned int)*p <= 0x20) && (*p!=0))
            p++;
    return p;
}



/* std_strdup_printf:
 *  Strings concatenation.
 */
char *std_strdup_printf (char *fmt, ...)
{
    char *ptr = NULL;
    va_list ap;
    
    va_start (ap, fmt);
    ptr = std_strdup_printf_run ((char *)fmt, ap);
    return ptr;
}



/* std_snprintf:
 *  Strings concatenation.
 */
size_t std_snprintf (char *dest, size_t size, const char *fmt, ...)
{
    char *ptr = NULL;
    va_list ap;
    
    va_start (ap, fmt);
    ptr = std_strdup_printf_run ((char *)fmt, ap);
    if (ptr != NULL)
    {
        *dest = '\0';
        strncat (dest, ptr, size);
        ptr = std_free (ptr);
    }
    return strlen (dest);
}



/* std_StringListIndex:
 *  Renvoit l'index de l'élément de la stringlist.
 */
int std_StringListIndex (struct STRING_LIST *p, char *str)
{
    int index;

    for (index=0; p!=NULL; p=p->next,index++)
        if (p->str!=NULL)
            if (strcmp (p->str, str) == 0)
                break;
    return (p==NULL)?-1:index;
}



/* std_StringListLength:
 *  Renvoit le nombre d'éléments de la stringlist.
 */
int std_StringListLength (struct STRING_LIST *p)
{
    int i = 0;

    while (p!=NULL)
    {
        i++;
        p=p->next;
    }

    return i;
}



char *std_ApplicationPath (const char dirname[], const char filename[])
{
    static char *fname = NULL;

    fname = NULL;
#ifdef DEBIAN_BUILD
    /* create private directory if necessary */
    fname = std_strdup_printf ("%s/.config/%s", getenv("HOME"), dirname);
    if (access (fname, F_OK) < 0)
    {
        (void)mkdir (fname, S_IRWXU);
    }
    /* set file path */
    fname = std_free (fname);
    fname = std_strdup_printf ("%s/.config/%s/%s", getenv("HOME"), dirname, filename);
#else
    /* set file path */
    fname = std_strdup_printf ("%s", filename);
    dirname = dirname;
#endif
    return fname;
}




/* std_StringListText:
 *  Renvoit le pointeur du texte de l'élément de la stringlist.
 */
char *std_StringListText (struct STRING_LIST *p, int index)
{
    for (;index>0;index--)
        if (p!=NULL)
            p=p->next;
    return (p!=NULL)?p->str:NULL;
}



/* std_StringListAppend:
 *  Ajoute un élément à la stringlist.
 */
struct STRING_LIST *std_StringListAppend (struct STRING_LIST *p, char *str)
{
    struct STRING_LIST *last_str = std_StringListLast (p);
    struct STRING_LIST *new_str = NULL;
    
    if (str != NULL)
    {
        new_str = calloc (1, sizeof (struct STRING_LIST));

        if (new_str!=NULL)
            new_str->str = std_strdup_printf ("%s", str);

        if ((last_str!=NULL) && (last_str->str!=NULL))
            last_str->next=new_str;
    }
    return (p==NULL)?new_str:p;
}



/* std_StringListFree:
 *  Libère la mémoire de la stringlist.
 */
void std_StringListFree (struct STRING_LIST *p)
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



/* std_BaseName:
 *  Retourne le nom du fichier à partir du chemin complet.
 */
char* std_BaseName(char *fullname)
{
   int len = strlen(fullname);

   while (--len > 0)
      if ((fullname[len] == '\\') || (fullname[len] == '/'))
         return fullname + len + 1;

   return fullname;
}



/* std_LastDir:
 *  Retourne le nom du dernier répertoire à partir du chemin complet.
 */
char* std_LastDir(char *fullname)
{
   int len = strlen(fullname);

   while ((len > 0) && ((fullname[len-1] == '\\') || (fullname[len-1] == '/')))
       fullname[--len] = '\0';

   while (--len > 0)
      if ((fullname[len] == '\\') || (fullname[len] == '/'))
         return fullname + len + 1;

   return fullname;
}



/* std_CleanPath:
 *  Efface le nom de fichier du chemin de fichier.
 */
void std_CleanPath (char *fname)
{
   char *p;

   if (fname != NULL)
   {
       if ((p = strrchr (fname, '\\')) == NULL)
           p = strrchr (fname, '/');

       if (p != NULL)
           *p = '\0';
   }
}

