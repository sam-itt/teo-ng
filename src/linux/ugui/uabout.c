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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2018 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Fran�ois Mouret
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
 *  Module     : linux/ugui/uabout.c
 *  Version    : 1.8.5
 *  Cr�� par   : Fran�ois Mouret 21/03/2012
 *  Modifi� par: Fran�ois Mouret 22/09/2012
 *               Samuel Cuella   01/2020
 *
 *  Fen�tre "A propos"
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <gdk/gdkx.h>
   #include <gtk/gtk.h>
#endif

#include "teo.h"
#include "linux/gui.h"
#include <gtk/gtk.h>
#include "gettext.h"

/* run_about_window:
 *  Affiche le panneau "A propos"
 */
void uabout_Dialog (GtkButton *button, gpointer user_data)
{
    const gchar *authors_list[] = { "Gilles Fétis", "Eric Botcazou", "Alexandre Pukall",
                                    "Jérémie Guillaume", "François Mouret",
                                    "Samuel Devulder", "Samuel Cuella", NULL };
#ifdef GFX_BACKEND_ALLEGRO
    GdkPixbuf *pixbuf;
    pixbuf=gdk_pixbuf_new_from_resource("/net/sourceforge/teoemulator/teo.png", NULL);
    gtk_window_set_default_icon(pixbuf);
#endif

    gtk_show_about_dialog (GTK_WINDOW(wControl),
                           "program-name", PACKAGE_NAME,
                           "version", TEO_VERSION_STR,
                           "copyright", "Copyright © 1997-"TEO_YEAR_STRING,
                           "comments", "Linux/X11",
                           "website", PACKAGE_HOMEPAGE,
                           "website-label", _(PACKAGE_NAME" homepage"),
                           "authors", authors_list,
                           "license-type", GTK_LICENSE_GPL_2_0,
                           NULL);

    (void)button;
    (void)user_data;
}

