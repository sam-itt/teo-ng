/*
 *  Module d'émulation des micro-circuits Motorola MC68xx:
 *    - microprocesseur MC6809E
 *    - PIA MC6846
 *    - PIA MC6821
 *    - PIA MC6804
 *
 *  Copyright (C) 1996 Sylvain Huet, 1999 Eric Botcazou, 2017 François Mouret.
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
 *  Module     : mc6804.h
 *  Version    : 1.0
 *  Créé par   : François Mouret 27/08/2017
 *  Modifié par:
 *
 *  Emulation du PIA MC6804.
 */


#ifndef MC68XX_MC6804_H
#define MC68XX_MC6804_H 1

extern void mc6804_Init (struct MC6846_PIA *mc6846);
extern int  mc6804_SetACK (struct MC6846_PIA *mc6846, int state, int kb_state);
extern void mc6804_UpdateCP1 (struct MC6846_PIA *mc6846);
extern void mc6804_SetScanCode (struct MC6846_PIA *mc6846, int code);

#endif

