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
 *  Copyright (C) 1997-2016 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 30/07/2011
 *               François Mouret 21/03/2012 20/09/2013 14/07/2016
 *
 *  Interface utilisateur de l'émulateur basée sur GTK+ 3.x .
 */


#ifndef LINUX_GUI_H
#define LINUX_GUI_H

#define COURIER_DEBUG "courier_debug"

#define FILENT_LENGTH  127

extern GtkWidget *wControl;

/* main panel */
extern void ugui_Init (void);
extern void ugui_Free (void);
extern void ugui_Panel (void);

/* boxes */
extern void ugui_Error    (const gchar *message, GtkWidget *parent_window);
extern void ugui_Warning  (const gchar *message, GtkWidget *parent_window);
extern int  ugui_Question (const gchar *message, GtkWidget *parent_window);

/* sub panels */
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

extern void udebug_Init(void);
extern void udebug_Panel(void);
extern void udebug_DoStepByStep (void);
extern void udebug_DoStepOver (void);
extern void udebug_Quit (int quit_mode);
extern void udebug_Free (void);

/* disassembly */
extern GtkWidget *uddisass_Init(void);
extern void uddisass_DoStep(void);
extern void uddisass_DoStepOver(GtkWidget *window);
extern void uddisass_Display(void);
extern void uddisass_Free(void);
/* extras registers */
extern GtkWidget *udreg_Init(void);
extern void udreg_Display(void);
extern void udreg_UpdateText(void);
extern void udreg_Free(void);
/* memory */
extern GtkWidget *udmem_Init(void);
extern void udmem_Display(void);
extern int  udmem_GetStepAddress(void);
extern void udmem_StepDisplay(int address);
extern void udmem_Free(void);
/* accumulators */
extern GtkWidget *udacc_Init(void);
extern void udacc_Display(void);
extern void udacc_Free(void);
/* breakpoints */
extern GtkWidget *udbkpt_Init(void);
extern void udbkpt_Update (int number);
extern void udbkpt_Free(void);
/* status bar */
extern GtkWidget *udstatus_Init (void);
extern void udstatus_Display (void);
extern void udstatus_Free(void);
/* tool bar */
extern GtkWidget *udtoolb_Init (void);
extern void udtoolb_SetRunButtonSensitivity (int state);
extern void udtoolb_Free(void);

#endif

