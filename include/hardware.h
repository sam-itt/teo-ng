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
 *  Module     : intern/hardware.h
 *  Version    : 1.8.3
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 1/12/2000
 *               François Mouret 18/09/2006 02/11/2012
 *
 *  Emulation de l'environnement matériel du MC6809E:
 *	- carte mémoire
 *	- circuits d'entrée/sortie du système
 *	- circuits d'entrée/sortie des périphériques
 */


#ifndef HARDWARE_H
#define HARDWARE_H

#include "mc68xx/mc6821.h"
#include "mc68xx/mc6846.h"
#include "defs.h"
#include "teo.h"

#define  LOAD_BYTE(addr)       (int) mempager.segment[((addr)>>12)&0xF][(addr)&0xFFF]
#define  LOAD_WORD(addr)       (LOAD_BYTE(addr)<<8)+LOAD_BYTE(addr+1)
#define STORE_BYTE(addr, val)  mempager.segment[((addr)>>12)&0xF][(addr)&0xFFF]=(uint8) (val)
#define STORE_WORD(addr, val)  STORE_BYTE(addr, val>>8);STORE_BYTE(addr+1, val&0xFF)

extern struct MC6846_PIA mc6846;
extern struct MC6821_PIA pia_int;
extern struct MC6821_PIA pia_ext;
extern struct GATE_ARRAY mode_page;
extern struct EF9369 pal_chip;
extern struct MEMORY_PAGER mempager;
extern struct MEMORY mem;
extern struct MOTHERBOARD mb;

extern mc6809_clock_t screen_clock;


extern inline void DrawGPL(int addr)
{ 
    int pt,col;

    if (addr>=0x1F40)
        return;

    pt =mem.ram.bank[mempager.screen.vram_page][addr];
    col=mem.ram.bank[mempager.screen.vram_page][addr+0x2000];

    teo_DrawGPL(mode_page.lgamod, addr, pt, col);
}

extern void hardware_Init(void);
extern void hardware_StoreByte(int addr, int val);

#endif
