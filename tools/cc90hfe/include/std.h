/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2015 Yves Charriau, François Mouret
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
 *  Module     : std.h
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Utility functions.
 */


#ifndef STD_H
#define STD_H 1

#include <stdio.h>

struct STRING_LIST {
    char *str;
    struct STRING_LIST *next;
};

extern int    std_StringListLength (struct STRING_LIST *p);
extern int    std_StringListIndex (struct STRING_LIST *p, char *str);
extern char   *std_StringListText (struct STRING_LIST *p, int index);
extern struct STRING_LIST *std_StringListAppend (struct STRING_LIST *p, char *str);
extern void   std_StringListFree (struct STRING_LIST *p);
extern void   std_CleanPath (char *filename);
extern char   *std_LastDir(char *fullname);
extern char   *std_BaseName(char *fullname);
extern char   *std_ApplicationPath (const char dirname[], const char filename[]);

extern void   *std_free (void *p);
extern void   *std_stralloc (void *p, char *s);
extern FILE   *std_fclose (FILE *fp);
extern int    std_IsFile (const char filename[]);
extern int    std_IsDir (const char filename[]);
extern size_t std_FileSize (const char filename[]);
extern void   std_rtrim (char *s);
extern char   *std_skpspc(char *p);
extern char   *std_strdup_printf (char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern size_t std_snprintf (char *dest, size_t size, const char*fmt, ...) __attribute__ ((format (printf, 3, 4)));

#endif

