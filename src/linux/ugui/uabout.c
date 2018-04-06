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
 *  Module     : linux/ugui/uabout.c
 *  Version    : 1.8.5
 *  Créé par   : François Mouret 21/03/2012
 *  Modifié par: François Mouret 22/09/2012
 *
 *  Fenêtre "A propos"
 */


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

/* run_about_window:
 *  Affiche le panneau "A propos"
 */
void uabout_Dialog (GtkButton *button, gpointer user_data)
{
    const gchar *authors_list[] = { "Gilles FÃ©tis", "Eric Botcazou", "Alexandre Pukall",
                                    "JÃ©rÃ©mie Guillaume", "FranÃ§ois Mouret",
                                    "Samuel Devulder", NULL };

    gtk_show_about_dialog (GTK_WINDOW(wControl),
                           "program-name", "Teo",
                           "version", TEO_VERSION_STR,
                           "copyright", "Copyright Â© 1997-"TEO_YEAR_STRING,
                           "comments", "Linux/X11",
                           "website", "http://sourceforge.net/projects/teoemulator/",
                           "website-label", is_fr?"Teo sur Sourceforge":"Teo on Sourceforge",
                           "authors", authors_list,
                           "license-type", GTK_LICENSE_GPL_2_0,
                           NULL);

    (void)button;
    (void)user_data;
}

