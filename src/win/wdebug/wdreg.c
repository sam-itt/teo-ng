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
 *  Copyright (C) 1997-2014 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : win/wdebug/wdreg.c
 *  Version    : 1.8.3
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: 
 *
 *  Débogueur 6809 - Affichage des registres.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/mc6821.h"
#include "mc68xx/dasm6809.h"
#include "media/disk/controlr.h"
#include "win/gui.h"

static HFONT hfont_normal = NULL;

/* extra registers frame */
static char *text;


/* debug_get_regs:
 *  Return the extra registers string.
 */
static void debug_get_regs(char *p)
{
    *p='\0';

    p += sprintf(p, "IRQ:%d\r\n", mc6809_irq);
    p += sprintf(p, " CSR:$%02X  CRC:$%02X\r\n", mc6846.csr, mc6846.crc);
    p += sprintf(p, "DDRC:$%02X  PRC:$%02X\r\n", mc6846.ddrc, mc6846.prc);
    p += sprintf(p, " TCR:$%02X TMSB:$%02X TLSB:$%02X\r\n",
                     mc6846.tcr,
                     mc6846.tmsb,
                     mc6846.tlsb);
    p += sprintf(p, "CRA:$%02X DDRA:$%02X PDRA:$%02X\r\n",
                     pia_int.porta.cr,
                     pia_int.porta.ddr,
                     mc6821_ReadPort(&pia_int.porta));
    p += sprintf(p, "CRB:$%02X DDRB:$%02X PDRB:$%02X\r\n",
                     pia_int.portb.cr,
                     pia_int.portb.ddr,
                     mc6821_ReadPort(&pia_int.portb));
    p += sprintf(p, "CRA:$%02X DDRA:$%02X PDRA:$%02X\r\n",
                     pia_ext.porta.cr,
                     pia_ext.porta.ddr,
                     mc6821_ReadPort(&pia_ext.porta));
    p += sprintf(p, "CRB:$%02X DDRB:$%02X PDRB:$%02X\r\n",
                     pia_ext.portb.cr,
                     pia_ext.portb.ddr,
                     mc6821_ReadPort(&pia_ext.portb));
    p += sprintf(p, "P_DATA:$%02X  P_ADDR:$%02X\r\n",
                     mode_page.p_data,
                     mode_page.p_addr);
    p += sprintf(p, "LGAMOD:$%02X    SYS1:$%02X\r\n",
                     mode_page.lgamod,
                     mode_page.system1);
    p += sprintf(p, "  SYS2:$%02X    DATA:$%02X\r\n",
                     mode_page.system2,
                     mode_page.ram_data);
    p += sprintf(p, "  CART:$%02X  COMMUT:$%02X\r\n",
                     mode_page.cart,
                     mode_page.commut);
    p += sprintf(p, "CMD0:$%02X CMD1:$%02X CMD2:$%02X\r\n",
                     dkc->wr0,
                     dkc->wr1,
                     dkc->wr2);
    p += sprintf(p, " STAT0:$%02X   STAT1:$%02X\r\n", dkc->rr0, dkc->rr1);
    p += sprintf(p, " WDATA:$%02X   RDATA:$%02X\r\n", dkc->wr3, dkc->rr3);
    p += sprintf(p, is_fr?"page de ROM cartouche :%d\r\n"
                         :"ROM cartridge page :%d\r\n",
                     mempager.cart.rom_page);
    p += sprintf(p, is_fr?"page de RAM cartouche :%d\r\n"
                         :"RAM cartridge page :%d\r\n",
                     mempager.cart.ram_page);
    p += sprintf(p, is_fr?"page de VRAM          :%d\r\n"
                         :"VRAM page          :%d\r\n",
                     mempager.screen.vram_page);
    p += sprintf(p, is_fr?"page de RAM (registre):%d\r\n"
                         :"RAM page (register):%d\r\n",
                     mempager.data.reg_page);
    p += sprintf(p, is_fr?"page de RAM (PIA)     :%d"
                         :"RAM page (PIA)     :%d",
                     mempager.data.pia_page);
}


void display (HWND hwnd)
{
    text = malloc (1000);
    if (text != NULL)
    {
        debug_get_regs (text);
        Edit_SetText (hwnd, text);
        Edit_Scroll (hwnd, teo.debug.extra_first_line, 0);
        free (text);
        text = NULL;
    }
}


/* ------------------------------------------------------------------------- */


/* wdreg_Init:
 *  Init register area.
 */
void wdreg_Init(HWND hDlg)
{
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_REG_EDIT);

    hfont_normal = wdebug_GetNormalFixedWidthHfont();
    if (hfont_normal != NULL)
        SetWindowFont(hwnd, hfont_normal, TRUE);

    display (hwnd);
}



/* wdreg_Display:
 *  Display extra registers.
 */
void wdreg_Display(HWND hDlg)
{
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_REG_EDIT);

    teo.debug.extra_first_line = (int)Edit_GetFirstVisibleLine(hwnd);
    display (hwnd);
}



/* wdreg_Exit:
 *  Exit the register area.
 */
void wdreg_Exit(HWND hDlg)
{
    HWND hwnd;

    if (hfont_normal != NULL)
    {
        (void)DeleteObject((HGDIOBJ)hfont_normal);
        hfont_normal = NULL;
    }

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_REG_EDIT);
    teo.debug.extra_first_line = (int)Edit_GetFirstVisibleLine(hwnd);

    text = std_free (text);
}
