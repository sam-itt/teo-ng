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
 *  Module     : std.c
 *  Version    : 1.8.5
 *  Créé par   : François Mouret 28/09/2012
 *  Modifié par:
 *
 *  Fonctions utilitaires.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE 1 /*Needed to silence vasprintf warning*/

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
   #include <sys/stat.h>
   #include <unistd.h>
   #include <stdarg.h>
#endif
#include <limits.h>

#include "defs.h"
#include "std.h"
#include "teo.h"
#include "media/printer.h"

#ifndef SYSCONFDIR
# define SYSCONFDIR "/etc"
#endif

#ifndef DATAROOTDIR
# define DATAROOTDIR "/usr/share"
#endif

#ifdef __MINGW32__
#include <shlobj.h>
#include <windows.h>
# define  mkdir( D, M )   mkdir( D )
#endif



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


/* Checks if a file exists 
 * returns 0 on failure, 1 on success
 *
 * */
unsigned char std_FileExists(char *fname)
{
    return ( access(fname, F_OK) != -1 );
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

/* Function with behaviour like `mkdir -p'  
 *
 * http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
*/ /*S_IRWXU */
static void mkdir_p(const char *dir, mode_t mode) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++){
        if(*p == '/') {
            *p = 0;
            mkdir(tmp, mode);
            *p = '/';
        }
    }
    mkdir(tmp, mode);
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
#if HAVE_VASPRINTF
    vasprintf(&ptr, fmt, ap);
#else
    ptr = std_strdup_printf_run ((char *)fmt, ap);
#endif
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


/* Will search for a file in pre-defined directories 
 * Caller must free the return value
 *
 * */
char *std_GetTeoSystemFile(char *name)
{
#if PLATFORM_UNIX
    const char *search_path[] = {
        DATAROOTDIR"/teo", /*Debian*/
        DATAROOTDIR"/games/teo",
        NULL
    };
#elif PLATFORM_WIN32 
    char path[MAX_PATH];
    char *search_path[2] = {NULL, NULL};

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path))) {
        search_path[0] = std_strdup_printf("%s\\teo", path);
	}

    GetModuleFileName(NULL, path, ARRAYSIZE(path));
    search_path[1] = strdup(dirname(path));
#elif PLATFORM_MSDOS 
/*TODO: Lookup where is teo.exe (argv[0] ?) and use it as TEO_HOME is not defined*/
/*TODO: declare TEO_HOME it in a BAT file for launching teo*/
    char *search_path[2] = {NULL, NULL};
    char *teo_home;

    teo_home = getenv("TEO_HOME");
    if(teo_home)
        search_path[0] = strdup(teo_home);
    else
        printf("%s: TEO_HOME not set, using the current directory as a fallback.\n", __FUNCTION__);
    search_path[1] = get_current_dir_name(); /*TODO: configure check for me*/
#endif
    char *fname;
    char **candidate;
    char *rv;
    
    rv = NULL;
    for(candidate = (char **)search_path; *candidate != NULL; candidate++){
        fname = std_strdup_printf("%s/%s", *candidate, name);
        printf("%s checking for %s\n", __FUNCTION__, fname);
        if(std_FileExists(fname)){
            rv = fname;
            break;
        }
        free(fname);
    }
#if PLATFORM_WIN32 || PLATFORM_MSDOS
    std_free(search_path[0]);
    std_free(search_path[1]);
#endif
    printf("%s returning %s\n",__FUNCTION__,rv);
    return rv;
}


/* Return the system-wide directory for teo settings i.e:
 *  /etc/teo on Unix
 *  C:\Documents and Settings\All Users\Application Data\teo on Win32
 *  The current directory on MS-DOS (atm)
 *
 *  Caller must free the returned value
 * */
char *std_getSystemConfigDir()
{
    char *rv;

    rv = NULL;
#if PLATFORM_UNIX
    rv = strdup(SYSCONFDIR"/teo");
#elif PLATFORM_WIN32
    char path[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path))) {
        rv = std_strdup_printf("%s\\teo", path);
	}
#elif PLATFORM_MSDOS /*TODO: Lookup where is teo.exe (argv[0] ?) and use it as conf+system dir*/
    char *teo_home;

    teo_home = getenv("TEO_HOME");
    if(teo_home)
        rv = strdup(teo_home);
    else
        rv = get_current_dir_name();
#endif
    printf("%s returning %s\n",__FUNCTION__,rv);
    return rv;
}

/* Returns per-user config dir for Teo, if available
 * NULL on failure
 * caller must free rv
 * */
char *std_getUserConfigDir()
{
    char *rv;

    rv = NULL;
#if PLATFORM_UNIX
    char *cfghome, *home;
    cfghome = getenv("XDG_CONFIG_HOME");
    if (!cfghome) {
        home = getenv("HOME");
        if (!home){ 
            printf("Couldn't get a value for $HOME - this shoudl't happen\n");
            printf("%s returning NULL\n",__FUNCTION__);
            return NULL;
        }
    }
    if(cfghome)
        rv = std_strdup_printf("%s/teo", cfghome);
    else
        rv = std_strdup_printf("%s/.config/teo", home);
    if(!std_FileExists(rv))
        mkdir_p(rv, S_IRWXU);
#elif PLATFORM_WIN32
    char path[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        rv = std_strdup_printf("%s\\teo", path);
        if(!std_FileExists(rv))
            mkdir_p(rv, S_IRWXU);
	}
#endif
    printf("%s returning %s\n",__FUNCTION__,rv);
    return rv;
}

/* Returns per-user data dir for Teo, if available
 * NULL on failure
 * caller must free rv
 * */
char *std_getUserDataDir()
{
    char *rv;

    rv = NULL;
#if PLATFORM_UNIX
    char *cfghome, *home;
    cfghome = getenv("XDG_DATA_HOME");
    if (!cfghome) {
        home = getenv("HOME");
        if (!home){ 
            printf("Couldn't get a value for $HOME - this shoudl't happen\n");
            printf("%s returning NULL\n",__FUNCTION__);
            return NULL;
        }
    }
    if(cfghome)
        rv = std_strdup_printf("%s/teo", cfghome);
    else
        rv = std_strdup_printf("%s/.local/teo", home);
    if(!std_FileExists(rv))
        mkdir_p(rv, S_IRWXU);
#elif PLATFORM_WIN32
    char path[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        rv = std_strdup_printf("%s\\teo", path);
        if(!std_FileExists(rv))
            mkdir_p(rv, S_IRWXU);
	}
#elif PLATFORM_MSDOS
    char *teo_home;

    teo_home = getenv("TEO_HOME");
    if(teo_home)
        rv = strdup(teo_home);
    else
        rv = get_current_dir_name();
#endif
    printf("%s returning %s\n",__FUNCTION__,rv);
    return rv;
}

/*  Will add the datadir path to filename, if possible
 * 
 *  will always return something, at least a copy of filename
 *  the caller must free the return value
 * */
char *std_GetUserDataFile(char *filename)
{
    char *datadir;
    char *rv;

    datadir = std_getUserDataDir();
    if(datadir){
#if PLATFORM_UNIX
        rv = std_strdup_printf("%s/%s", datadir, filename);
#else
        rv = std_strdup_printf("%s\\%s", datadir, filename);
#endif
        std_free(datadir);
        printf("%s: Found datadir, returning %s\n", __FUNCTION__, rv);
    }else{
        rv = strdup(filename);
    }
    return rv;
}

/* Search for an existing file in 
 *  a) user config dir
 *  b) system config dir
 *  c) Win32-only: Exe directory
 *  
 *  Return first found file or NULL
 * 
 *  Caller must free the return value
 */

char *std_GetFirstExistingConfigFile(char *filename)
{
    char *dir;
    char *rv;

    dir = std_getUserConfigDir();
    if(dir){
        printf("%s: Got user config dir: %s\n", __FUNCTION__, dir);
#if PLATFORM_UNIX
        rv = std_strdup_printf("%s/%s", dir, filename);
#else
        rv = std_strdup_printf("%s\\%s", dir, filename);
#endif
        if(std_FileExists(rv)){
            printf("%s: User config file %s exists, using it\n", __FUNCTION__, rv);
            std_free(dir);
            return rv;
        }
        std_free(dir);
    }

    dir = std_getSystemConfigDir();
    if(dir){
        printf("%s: Got sys config dir: %s\n", __FUNCTION__, dir);
#if PLATFORM_UNIX
        rv = std_strdup_printf("%s/%s", dir, filename);
#else
        rv = std_strdup_printf("%s\\%s", dir, filename);
#endif
        if(std_FileExists(rv)){
            printf("%s: User config file %s DOES NOT exist, falling back on system-wide config file %s\n", __FUNCTION__, filename, rv);
            std_free(dir);
            return rv;
        }
        std_free(dir);
    }
#if PLATFORM_WIN32
    char path[MAX_PATH];

    GetModuleFileName(NULL, path, ARRAYSIZE(path));
    dir = dirname(path);
    if(dir){
        printf("%s: Got exe dir: %s\n", __FUNCTION__, dir);
        rv = std_strdup_printf("%s\\%s", dir, filename);
        if(std_FileExists(rv)){
            printf("%s: User config file %s DOES NOT exist, falling back on exe-dir config file %s\n", __FUNCTION__, filename, rv);
            return rv;
        }
    }
#endif
    printf("%s: Neither user or system config file exists for %s, returning NULL\n", __FUNCTION__, filename);
    return NULL;
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

