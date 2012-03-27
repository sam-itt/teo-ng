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
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 30/07/2011
 *               François Mouret 21/03/2012
 *
 *  Interface utilisateur de l'émulateur basée sur GTK+ 2.x .
 */


#ifndef LINUX_GUI_H
#define LINUX_GUI_H

#define FILENT_LENGTH  127

extern GtkWidget *wdControl;

/* main panel */
extern void InitGUI(int);
extern void FreeGUI (void);
extern void ControlPanel(void);

/* boxes */
extern int  ask_box (const gchar *message, GtkWidget *parent_window);
extern void error_box (const gchar *message, GtkWidget *parent_window);

/* sub panels */
extern void DebugPanel(void);
extern void init_disk_notebook_frame (GtkWidget *notebook, int direct_disk_support);
extern void free_disk_list (void);
extern void run_about_window (GtkWidget *button, gpointer user_data);
extern void init_memo_notebook_frame (GtkWidget *notebook);
extern void free_memo_list (void);
extern void init_cass_notebook_frame (GtkWidget *notebook);
extern void update_counter_cass (void);
extern void free_cass_list (void);
extern void init_setting_notebook_frame (GtkWidget *notebook);

#endif

