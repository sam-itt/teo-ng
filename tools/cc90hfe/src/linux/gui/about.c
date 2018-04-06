/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2018 Yves Charriau, François Mouret
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
 *  Module     : linux/gui/about.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Linux About window.
 */

#ifndef SCAN_DEPEND
   #include <gdk/gdkx.h>
   #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "linux/gui.h"



/* run_about_window:
 *  Affiche le panneau "A propos"
 */
void about_Callback (GtkButton *button, gpointer user_data)
{
    const gchar *authors_list[] = { "FranÃ§ois Mouret", NULL };

    gtk_show_about_dialog (GTK_WINDOW(main_window),
                           "program-name", PROG_NAME,
                           "version", "version "PROG_VERSION_STRING,
                           "copyright", "Copyright Â© 2012-2018"PROG_CREATION_YEAR,
                           "website", PROG_WEB_SITE,
                           "website-label", is_fr?PROG_NAME" sur SourceForge (Teo module)"
                                                 :PROG_NAME" on SourceForge (Teo module)",
                           "authors", authors_list,
                           "license-type", GTK_LICENSE_GPL_2_0,
                           NULL);
    (void)button;
    (void)user_data;
}

