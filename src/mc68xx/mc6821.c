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
 *  Module     : mc6821.c
 *  Version    : 4.0
 *  Créé par   : Eric Botcazou
 *  Modifié par: Eric Botcazou 23/02/2001
 *
 *  Emulation du PIA Motorola MC6821.
 */


#ifdef DEBUG

#include "mc68xx/mc6821.h"



/* WriteCommand:
 *  Dépose une valeur dans le registre de commande du port.
 */
void mc6821_WriteCommand(struct MC6821_PORT *port, int val)
{
    if (val&0x30)  /* CP2 en sortie */
        port->cr = (port->cr&0xC0) | (val&0x3F);
    else
        port->cr = (port->cr&0xC8) | (val&0x37);
}



/* ReadCommand:
 *  Lit une valeur dans le registre de commande du port.
 */
int mc6821_ReadCommand(struct MC6821_PORT *port)
{
    return port->cr;
}



/* WriteData:
 *  Ecrit une valeur sur le port en sortie.
 */
void mc6821_WriteData(struct MC6821_PORT *port, int val)
{
    if (port->cr&4)
        port->odr = val;
    else
        port->ddr = val;
}



/* ReadPort:
 *  Retourne la valeur du port.
 */
int mc6821_ReadPort(struct MC6821_PORT *port)
{
    return (port->idr&(port->ddr^0xFF)) | (port->odr&port->ddr);
}



/* ReadData:
 *  Retourne la valeur du registre de données sélectionné.
 */
int mc6821_ReadData(struct MC6821_PORT *port)
{
    if (port->cr&4)
        return mc6821_ReadPort(port);
    else
        return port->ddr;
}



/* Init:
 *  Initialise le port spécifié.
 */
void mc6821_Init(struct MC6821_PORT *port, int cr, int idr)
{
    port->cr  = cr;
    port->ddr = 0;
    port->odr = 0;
    port->idr = idr;
}

#endif
