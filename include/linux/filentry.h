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
 *  Module     : linux/filentry.h
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               François Mouret 03/08/2011 13/01/2012
 *
 *  Classe FileEntry d'extension du toolkit GTK+ 2.x/3.x .
 */


#ifndef LINUX_FILENTRY_H
#define LINUX_FILENTRY_H

#ifndef SCAN_DEPEND
   #include <gtk/gtkhbox.h>
#endif

#include "linux/xgui.h"


#define FILENT_LENGTH  127

#if BEFORE_GTK_2_MIN

#define FILE_ENTRY(obj)          GTK_CHECK_CAST(obj, file_entry_get_type(), FileEntry)
#define FILE_ENTRY_CLASS(klass)  GTK_CHECK_CLASS_CAST(klass, file_entry_get_type(), FileEntryClass)
#define IS_FILE_ENTRY(obj)       GTK_CHECK_TYPE(obj, file_entry_get_type())

typedef struct _FileEntry       FileEntry;
typedef struct _FileEntryClass  FileEntryClass;

struct _FileEntry {
    GtkHBox hbox;
  
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *filesel;

    gchar filename[FILENT_LENGTH+1];
};

struct _FileEntryClass {
    GtkHBoxClass parent_class;

    void (*file_selected)(FileEntry *);
};

extern GType      file_entry_get_type(void);
extern GtkWidget *file_entry_new(const gchar *);
extern void       file_entry_set_entry(FileEntry *, const gchar *);
extern void       file_entry_set_filename(FileEntry *, const gchar *);
extern gchar     *file_entry_get_filename(FileEntry *);    
#else
extern GtkWidget *file_chooser_button_new(char *label, const gchar *title,
                      const gchar *patternname, char *patternfilter,
                      const gchar *current_file, char *current_dir,
                      GtkWidget *parent_window, GtkWidget *hbox);
extern gchar *file_chooser_get_filename(void);
extern void file_chooser_reset_filename(GtkFileChooserButton *chooser_button);

#endif

#endif

