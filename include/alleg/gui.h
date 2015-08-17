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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : alleg/gui.h
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 24/10/2003
 *               François Mouret 12/08/2011 03/11/2012
 *
 *  GUI Allegro de l'émulateur.
 */


#ifndef ALLEG_GUI_H
#define ALLEG_GUI_H

/* panneau de contrôle */
extern void agui_SetColors(int, int, int);
extern void agui_Panel(void);
extern void agui_Init(char [], int, int);
extern void agui_Free (void);

/* panneau d'imprimante */
extern void aprinter_SetColors(int fg_color, int bg_color, int bg_entry_color);
extern void aprinter_Panel(void);
extern void aprinter_Init(void);
extern void aprinter_Free(void);

/* panneau des disquettes */
extern void adisk_SetColors(int fg_color, int bg_color, int bg_entry_color);
extern void adisk_Panel(void);
extern void adisk_Init(int direct_disk_support);
extern void adisk_Free(void);

/* panneau de la cassette */
extern void acass_SetColors(int fg_color, int bg_color, int bg_entry_color);
extern void acass_Panel(void);
extern void acass_Init(void);
extern void acass_Free(void);

/* panneau de la cartouche */
extern void amemo_SetColors(int fg_color, int bg_color, int bg_entry_color);
extern void amemo_Panel(void);
extern void amemo_Init(void);
extern void amemo_Free(void);

/* panneau des réglages */
extern void asetting_SetColors(int fg_color, int bg_color, int bg_entry_color);
extern void asetting_Init(char version_name[], int gfx_mode);
extern void asetting_Panel(void);
extern void asetting_Free(void);

/* panneau "A propos..." */
extern void aabout_SetColors(int fg_color, int bg_color, int bg_entry_color);
extern void aabout_Panel(void);
extern void aabout_Init(char *title);
extern void aabout_Free(void);

/* fonctions de la GUI */
extern void agui_PopupMessage(const char message[]);

#endif
