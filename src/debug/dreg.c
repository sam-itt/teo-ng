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
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : debug/dreg.c
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 16/11/2015 09/03/2016 14/07/2016
 *
 *  Débogueur 6809 - Affichage des registres.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/mc6821.h"
#include "mc68xx/dasm6809.h"
#include "media/disk.h"


/* ------------------------------------------------------------------------- */


/* debug_get_regs:
 *  Return the extra registers string.
 */
char *dreg_GetText (char *special_cr)
{
    char *text = NULL;
    char *p;

    text = malloc (1000);
    if (text != NULL)
    {
        p = text;
        *p='\0';
        p += sprintf(p, "IRQ:%d%s",
                         mc6809_irq,
                         special_cr);
        p += sprintf(p, " CSR:$%02X  CRC:$%02X%s",
                         mc6846.csr,
                         mc6846.crc,
                         special_cr);
        p += sprintf(p, "DDRC:$%02X  PRC:$%02X%s",
                         mc6846.ddrc,
                         mc6846.prc,
                         special_cr);
        p += sprintf(p, " TCR:$%02X TMSB:$%02X TLSB:$%02X%s",
                         mc6846.tcr,
                         mc6846.tmsb,
                         mc6846.tlsb,
                         special_cr);
        p += sprintf(p, "CRA:$%02X DDRA:$%02X PDRA:$%02X%s",
                         pia_int.porta.cr,
                         pia_int.porta.ddr,
                         mc6821_ReadPort(&pia_int.porta),
                         special_cr);
        p += sprintf(p, "CRB:$%02X DDRB:$%02X PDRB:$%02X%s",
                         pia_int.portb.cr,
                         pia_int.portb.ddr,
                         mc6821_ReadPort(&pia_int.portb),
                         special_cr);
        p += sprintf(p, "CRA:$%02X DDRA:$%02X PDRA:$%02X%s",
                         pia_ext.porta.cr,
                         pia_ext.porta.ddr,
                         mc6821_ReadPort(&pia_ext.porta),
                         special_cr);
        p += sprintf(p, "CRB:$%02X DDRB:$%02X PDRB:$%02X%s",
                         pia_ext.portb.cr,
                         pia_ext.portb.ddr,
                         mc6821_ReadPort(&pia_ext.portb),
                         special_cr);
        p += sprintf(p, "P_DATA:$%02X  P_ADDR:$%02X%s",
                         mode_page.p_data,
                         mode_page.p_addr,
                         special_cr);
        p += sprintf(p, "LGAMOD:$%02X    SYS1:$%02X%s",
                         mode_page.lgamod,
                         mode_page.system1,
                         special_cr);
        p += sprintf(p, "  SYS2:$%02X    DATA:$%02X%s",
                         mode_page.system2,
                         mode_page.ram_data,
                         special_cr);
        p += sprintf(p, "  CART:$%02X  COMMUT:$%02X%s",
                         mode_page.cart,
                         mode_page.commut,
                         special_cr);
        p += sprintf(p, "CMD0:$%02X CMD1:$%02X CMD2:$%02X%s",
                         disk[0].dkc->wr0,
                         disk[0].dkc->wr1,
                         disk[0].dkc->wr2,
                         special_cr);
        p += sprintf(p, " STAT0:$%02X   STAT1:$%02X%s",
                         disk[0].dkc->rr0,
                         disk[0].dkc->rr1,
                         special_cr);
        p += sprintf(p, " WDATA:$%02X   RDATA:$%02X%s",
                         disk[0].dkc->wr3,
                         disk[0].dkc->rr3,
                         special_cr);
        p += sprintf(p, is_fr?"page de ROM cartouche :%d%s"
                             :"ROM cartridge page :%d%s",
                         mempager.cart.rom_page,
                         special_cr);
        p += sprintf(p, is_fr?"page de RAM cartouche :%d%s"
                             :"RAM cartridge page :%d%s",
                         mempager.cart.ram_page,
                         special_cr);
        p += sprintf(p, is_fr?"page de VRAM          :%d%s"
                             :"VRAM page          :%d%s",
                         mempager.screen.vram_page,
                         special_cr);
        p += sprintf(p, is_fr?"page de RAM (registre):%d%s"
                             :"RAM page (register):%d%s",
                         mempager.data.reg_page,
                         special_cr);
        p += sprintf(p, is_fr?"page de RAM (PIA)     :%d"
                             :"RAM page (PIA)     :%d",
                         mempager.data.pia_page);
    }
    return text;
}

