/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 Yves Charriau, François Mouret
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
 *  Module     : hfe.h
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of HFE format (MFM).
 */


#ifndef HFE_H
#define HFE_H 1

extern int  hfe_ReadTrack (void);
extern int  hfe_WriteTrack (void);
extern int  hfe_ReadOpen (const char filename[]);
extern int  hfe_WriteOpen (const char filename[]);
extern void hfe_Close (void);

#endif

