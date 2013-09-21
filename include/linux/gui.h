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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/gui.h
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 30/07/2011
 *               François Mouret 21/03/2012 20/09/2013
 *
 *  Interface utilisateur de l'émulateur basée sur GTK+ 3.x .
 */


#ifndef LINUX_GUI_H
#define LINUX_GUI_H

#define FILENT_LENGTH  127

extern GtkWidget *wControl;

/* main panel */
extern void ugui_Init (void);
extern void ugui_Free (void);
extern void ugui_Panel (void);

/* boxes */
extern void ugui_Error   (const gchar *message, GtkWidget *parent_window);
extern void ugui_Warning (const gchar *message, GtkWidget *parent_window);

/* sub panels */
extern int  udebug_Panel(void);
extern void udisk_Init (GtkWidget *notebook);
extern void udisk_Free (void);

extern void uabout_Dialog (GtkButton *button, gpointer user_data);

extern void umemo_Init (GtkWidget *notebook);
extern void umemo_Free (void);

extern void ucass_Init (GtkWidget *notebook);
extern void ucass_UpdateCounter (void);
extern void ucass_Free (void);

extern void usetting_Init (GtkWidget *notebook);
extern void usetting_Update (void);

extern void uprinter_Init (GtkWidget *notebook);

#endif

