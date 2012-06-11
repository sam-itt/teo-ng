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
 *  Module     : intern/gui.h
 *  Version    : 1.8.1
 *  Créé par   : François Mouret 09/05/2012
 *  Modifié par:
 *
 *  Parties communes de la GUI.
 */


#ifndef GUI_H
#define GUI_H

struct STRING_LIST {
    char *str;
    struct STRING_LIST *next;
};

struct LPRT_GUI {
    int  number;
    int  nlq;
    int  dip;
    int  raw_output;
    int  txt_output;
    int  gfx_output;
    char *folder;
};

struct DISK_GUI {
    int  direct_access_allowed;
    int  write_protect;
    char *file;
};

struct CASS_GUI  {
    int  write_protect;
    char *file;
};

struct MEMO_GUI {
    char *file;
    char *label;
};

struct SETTING_GUI {
    int  exact_speed;
    int  sound_volume;
    int  interlaced_video;
};

struct IMAGE_GUI {
    char *file;
};

#define NBDRIVE    4

struct THOMSON_GUI {
    char *default_folder;
    struct SETTING_GUI setting;
    struct DISK_GUI disk[NBDRIVE];
    struct CASS_GUI cass;
    struct MEMO_GUI memo;
    struct LPRT_GUI lprt;
    struct IMAGE_GUI imag;
};

extern struct THOMSON_GUI *gui;

extern int  gui_Init (void);
extern void gui_Free (void);
extern int  gui_StringListIndex (struct STRING_LIST *p, char *str);
extern char *gui_StringListText (struct STRING_LIST *p, int index);
extern struct STRING_LIST *gui_StringListAppend (struct STRING_LIST *p, char *str);
extern void gui_StringListFree (struct STRING_LIST *p);

extern void  gui_CleanPath (char *filename);
extern char* gui_LastDir(char *fullname);
extern char* gui_BaseName(char *fullname);

#endif
