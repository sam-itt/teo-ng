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
 *  Module     : mc6846.h
 *  Version    : 2.3
 *  Créé par   : Eric Botcazou juin 1999
 *  Modifié par: Eric Botcazou 9/12/2000
 *               François Mouret 20/01/2017 
 *
 *  Emulation du PIA Motorola MC6846.
 */


#ifndef MC6846_H
#define MC6846_H

#include "mc6809.h"

struct MC6846_PIA {
    int csr;
    int crc;
    int ddrc;
    int prc;
    int w_mask;
    int tcr;
    int tmsb;
    int tlsb;
    int timer_ratio;
    mc6809_clock_t timeout;
    void (*callback)(void *);
};


#define MAKE_CSR()  if ( ((mc6846->csr&0x01) && (mc6846->tcr&0x40)) ||  \
                         ((mc6846->csr&0x02) && (mc6846->crc&0x01)) )   \
                    {                                                   \
                        mc6846->csr|=0x80;                              \
                        mc6809_irq=1;                                   \
                    }                                                   \
                    else                                                \
                    {                                                   \
                        mc6846->csr&=0x7F;                              \
                        mc6809_irq=0;                                   \
                    }


#define START_TIMER()  mc6809_SetTimer( (mc6846->timeout=mc6809_clock()        \
                       +((mc6846->tmsb<<8)+mc6846->tlsb)*mc6846->timer_ratio), \
                       mc6846->callback, (void *)mc6846)


extern void  mc6846_SetTcr(struct MC6846_PIA *mc6846, int val);
extern void  mc6846_SetTmsb(struct MC6846_PIA *mc6846, int val);
extern void  mc6846_SetTlsb(struct MC6846_PIA *mc6846, int val);
extern int   mc6846_tcr(struct MC6846_PIA *mc6846);
extern int   mc6846_tmsb(struct MC6846_PIA *mc6846);
extern int   mc6846_tlsb(struct MC6846_PIA *mc6846);
extern void  mc6846_WriteCommand(struct MC6846_PIA *mc6846, int val);
extern void  mc6846_WriteData(struct MC6846_PIA *mc6846, int val);
extern void  mc6846_Init(struct MC6846_PIA *, int, int, int);

#endif

