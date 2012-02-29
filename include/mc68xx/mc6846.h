/*
 *  Module d'émulation des micro-circuits Motorola MC68xx:
 *    - microprocesseur MC6809E
 *    - PIA MC6846
 *    - PIA MC6821
 *
 *  Copyright (C) 1996 Sylvain Huet, 1999 Eric Botcazou.
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



extern inline void mc6846_SetTcr(struct MC6846_PIA *mc6846, int val)
{
    mc6846->tcr=val;
    mc6846->timer_ratio=(mc6846->tcr&4 ? 8 : 1);

    if (mc6846->tcr&1) /* timer disabled */
    {
        mc6809_SetTimer(MC6809_TIMER_DISABLED, mc6846->callback, (void *)mc6846);
        mc6846->csr&=0xFE;
        MAKE_CSR();
    }
    else
        START_TIMER();
}


extern inline void mc6846_SetTmsb(struct MC6846_PIA *mc6846, int val)
{
    mc6846->tmsb=val;
}


extern inline void mc6846_SetTlsb(struct MC6846_PIA *mc6846, int val)
{
    mc6846->tlsb=val;

    if (!(mc6846->tcr&0x38))
    {
        mc6846->csr&=0xFE;
        MAKE_CSR();

        mc6846->tcr&=0xFE; /* timer enabled */
        START_TIMER();
    }
}


extern inline int mc6846_tcr(struct MC6846_PIA *mc6846)
{
    return mc6846->tcr;
}


extern inline int mc6846_tmsb(struct MC6846_PIA *mc6846)
{
    mc6846->csr&=0xFE;
    MAKE_CSR();

    if (mc6846->tcr&1) /* timer disabled */
        return mc6846->tmsb;
    else
        return ((mc6846->timeout-mc6809_clock())/mc6846->timer_ratio)>>8;
}


extern inline int mc6846_tlsb(struct MC6846_PIA *mc6846)
{
    if (mc6846->tcr&1) /* timer disabled */
        return mc6846->tlsb;
    else
        return ((mc6846->timeout-mc6809_clock())/mc6846->timer_ratio)&0xFF;
}


extern inline void mc6846_WriteCommand(struct MC6846_PIA *mc6846, int val)
{
    if ((val&2) != (mc6846->crc&2)) /* mode CP1 modifié */
    {
        mc6846->csr&=0xFD;
        MAKE_CSR();
    }

    if (val&0x30)  /* CP2 en sortie */
        mc6846->crc = (mc6846->crc&0xC0) | (val&0x3F);
    else
        mc6846->crc = (mc6846->crc&0xC8) | (val&0x37);
}


extern inline void mc6846_SetCP1(struct MC6846_PIA *mc6846, int state)
{
    if ( (mc6846->crc&0x80) != (state<<7) )
    {
        mc6846->crc=(mc6846->crc&0x7F) | (state<<7);

        if  ((mc6846->crc&2) == (state<<1))
            mc6846->csr|=0x02;   /* bit CSR1 à 1 */
        else
            mc6846->csr&=0xFD;

        MAKE_CSR();
    }
}


extern inline void mc6846_WriteData(struct MC6846_PIA *mc6846, int val)
{
    int mask = mc6846->ddrc & mc6846->w_mask;
    mc6846->prc = (mc6846->prc&(mask^0xFF)) | (val&mask);
}


extern void mc6846_Init(struct MC6846_PIA *, int, int, int);

#endif
