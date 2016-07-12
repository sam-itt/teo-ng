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
 *  Module     : mc6821.h
 *  Version    : 4.0
 *  Créé par   : Eric Botcazou
 *  Modifié par: Eric Botcazou 23/02/2001
 *
 *  Emulation du PIA Motorola MC6821.
 */


#ifndef MC6821_H
#define MC6821_H

#if __GNUC__ && !__GNUC_STDC_INLINE__
# define INLINE extern inline
#else
# define INLINE inline
#endif

struct MC6821_PORT {
    int cr;         /* registre de commande                  */
    int ddr;        /* registre de direction de données      */
    int odr;        /* registre de données en sortie         */
    int idr;        /* registre de données en entrée         */
};

struct MC6821_PIA {
    struct MC6821_PORT porta;
    struct MC6821_PORT portb;
};


INLINE void mc6821_WriteCommand(struct MC6821_PORT *port, int val)
{
    if (val&0x30)  /* CP2 en sortie */
        port->cr = (port->cr&0xC0) | (val&0x3F);
    else
        port->cr = (port->cr&0xC8) | (val&0x37);
}


INLINE int mc6821_ReadCommand(struct MC6821_PORT *port)
{
    return port->cr;
}


INLINE void mc6821_WriteData(struct MC6821_PORT *port, int val)
{
    if (port->cr&4)
        port->odr = val;
    else
        port->ddr = val;
}


INLINE int mc6821_ReadPort(struct MC6821_PORT *port)
{
    return (port->idr&(port->ddr^0xFF)) | (port->odr&port->ddr);
}


INLINE int mc6821_ReadData(struct MC6821_PORT *port)
{
    if (port->cr&4)
        return mc6821_ReadPort(port);
    else
        return port->ddr;
}


INLINE void mc6821_Init(struct MC6821_PORT *port, int cr, int idr)
{
    port->cr  = cr;
    port->ddr = 0;
    port->odr = 0;
    port->idr = idr;
}

#endif
