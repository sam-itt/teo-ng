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
 *                          Jérémie Guillaume
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
 *  Module     : intern/std.h
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 28/09/2012
 *  Modifié par:
 *
 *  Fonctions utilitaires.
 */


#ifndef STD_H
#define STD_H

struct STRING_LIST {
    char *str;
    struct STRING_LIST *next;
};

extern int  std_StringListIndex (struct STRING_LIST *p, char *str);
extern char *std_StringListText (struct STRING_LIST *p, int index);
extern struct STRING_LIST *std_StringListAppend (struct STRING_LIST *p, char *str);
extern void std_StringListFree (struct STRING_LIST *p);
extern void  std_CleanPath (char *filename);
extern char* std_LastDir(char *fullname);
extern char* std_BaseName(char *fullname);

extern void *std_free (void *p);
extern void *std_stralloc (void *p, char *s);
extern FILE *std_fclose (FILE *fp);
extern int  std_isfile (char *fname);
extern int  std_isdir (char *fname);
extern void std_rtrim (char *s);
extern char *std_skpspc(char *p);
extern char *std_strdup_printf (char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#endif

