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
 *  Module     : serial.h
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of RS232 connection.
 */


#ifndef SERIAL_H
#define SERIAL_H 1

#define SERIAL_TIME_OUT 20

extern void serial_Close(void);
extern void serial_RestoreTimeout(void);
extern void serial_InfiniteTimeout(void);
extern int  serial_Open (char *port_name);
extern int  serial_Write (uint8 *buf, int size);
extern int  serial_Read (uint8 *buf, int size);
extern int  serial_OpenPort (void);

#endif

