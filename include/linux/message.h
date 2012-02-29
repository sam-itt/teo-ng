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
 *  Module     : linux/message.h
 *  Version    : 1.8.0
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               François Mouret 31/07/2011
 *
 *  Classe Message d'extension du toolkit GTK+ 2.x .
 */


#ifndef LINUX_MESSAGE_H
#define LINUX_MESSAGE_H

#ifndef SCAN_DEPEND
   #include <gtk/gtkwindow.h>
#endif

#include "linux/xgui.h"

#if BEFORE_GTK_2_MIN

#define MESSAGE(obj)          GTK_CHECK_CAST(obj, message_get_type(), Message)
#define MESSAGE_CLASS(klass)  GTK_CHECK_CLASS_CAST(klass, message_get_type(), MessageClass)
#define IS_MESSAGE(obj)       GTK_CHECK_TYPE(obj, message_get_type())

typedef struct _Message       Message;
typedef struct _MessageClass  MessageClass;

struct _Message {
    GtkWindow window;

    GtkWidget *label;
};

struct _MessageClass {
    GtkWindowClass parent_class;
};

extern GtkWidget *message_new(const gchar *);

#else

extern void message_box (const gchar *message, GtkWidget *parent_window);

#endif

#endif
