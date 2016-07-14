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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : debug/dmem.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 16/11/2015 14/07/2016
 *
 *  Débogueur 6809 - Affichage de la mémoire.
 */


#ifndef SCAN_DEPEND
    #include <stdio.h>
    #include <stdlib.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/dasm6809.h"


/* ------------------------------------------------------------------------- */


/* dmem_GetJumpAddress:
 *  Get the memory address to jump to.
 */
int dmem_GetJumpAddress (void)
{
    int i;
    struct MC6809_REGS regs;
    struct MC6809_DASM mc6809_dasm;
    int addr = -1;
    int offset;
    int opcode;
    unsigned char *code;
    int lreg[4];

    /* get registers state */
    mc6809_GetRegs(&regs);
    lreg[0] = regs.xr;
    lreg[1] = regs.yr;
    lreg[2] = regs.ur;
    lreg[3] = regs.sr;

    /* get the fetch */
    for (i=0; i<MC6809_DASM_FETCH_SIZE; i++)
        mc6809_dasm.fetch[i]=LOAD_BYTE((regs.pc&0xffff)+i);
    code = mc6809_dasm.fetch;

    /* skip 0x10 and 0x11 codes if necessary */
    opcode = code[0];
    if (opcode == 0x10)
        opcode = 256 + (code++[1]);
    else
    if (opcode == 0x11)
        opcode = 512 + (code++[1]);

    /* exclude disturbing codes */
    if ((code[0] == 0x0e)        /* JMP direct */
     || (code[0] == 0x6e)        /* JMP indexed */
     || (code[0] == 0x7e)        /* JMP extended */
     || (code[0] == 0x9d)        /* JSR direct */
     || (code[0] == 0xad)        /* JSR indexed */
     || (code[0] == 0xbd)        /* JSR extended */
     || (((code[0] >= 0x30) && (code[0] <= 0x33))  /* LEAX/Y/U/S */
       && ((code[1]&0x90) != 0x90)))  /* ... without indirection */
        return addr;

    switch (dasm6809_addr[opcode]) {
    case 0:  /* direct */
        addr = ((regs.dp<<8)+code[1]) & 0xffff;
        break;

    case 4: /* indexed */
        switch (code[1]&0x8F) {
        case 0x80:   /* ,r+  */
        case 0x81:   /* ,r++ */
        case 0x84:   /* ,r   */
            addr = (lreg[((int)code[1]&0x60)>>5]) & 0xffff;
            break;

        case 0x82:   /* ,-r  */
            addr = (lreg[((int)code[1]&0x60)>>5] - 1) & 0xffff;
            break;

        case 0x83:   /* ,--r */
            addr = (lreg[((int)code[1]&0x60)>>5] - 2) & 0xffff;
            break;

        case 0x85:   /* B,r  */
            offset = (int)((signed char)regs.br);
            addr = (lreg[((int)code[1]&0x60)>>5] + offset) & 0xffff;
            break;

        case 0x86:   /* A,r  */
            offset = (int)((signed char)regs.ar);
            addr = (lreg[((int)code[1]&0x60)>>5] + offset) & 0xffff;
            break;

        case 0x87:   /* error */
            break;

        case 0x88:   /* $00,r */
            offset = (int)((signed char)code[2]);
            addr = (lreg[((int)code[1]&0x60)>>5] + offset) & 0xffff;
            break;

        case 0x89:   /* $0000,r */
            offset = (int)(code[2]*256+code[3]);
            addr = (lreg[((int)code[1]&0x60)>>5] + offset) & 0xffff;
            break;

        case 0x8a:   /* error */
            break;

        case 0x8b:  /* D,r */
            offset = (int)(regs.ar*256+regs.br);
            addr = (lreg[((int)code[1]&0x60)>>5] + offset) & 0xffff;
            break;

        case 0x8c:  /* $00,PCR */
            offset = (int)((signed char)code[2]);
            offset += regs.pc + (int)(&code[3]-mc6809_dasm.fetch);
            addr = offset & 0xffff;
            break;

        case 0x8d:  /* $0000,PCR */
            offset = (int)(code[2]*256+code[3]);
            offset += regs.pc + (int)(&code[4]-mc6809_dasm.fetch);
            addr = offset & 0xffff;
            break;

        case 0x8e:   /* error */
            break;

        case 0x8f:  /* $0000 */
            addr = (int)(code[2]*256+code[3]);
            break;

        default:
            /* 0,x */
            offset = (int)(code[1]&0xF) - (int)(code[1]&0x10);
            addr = (lreg[((int)code[1]&0x60)>>5] + offset) & 0xffff;
            break;
        }  /* end of switch (code[1]&0x8f) */

        /* indirection */
        if ((code[1]&0x90) == 0x90)
            addr = (mc6809_interface.LoadByte(addr&0xffff)<<8)
                  + mc6809_interface.LoadByte((addr+1)&0xffff);
        break;

    case 5: /* extended */
        addr = (code[1]<<8)+code[2];
        break;

    default: /* error */
        break;
    }  /* end of switch (dasm6809_addr[opcode]) */

    return addr;
}



/* dmem_GetText:
 * Get the memory dump.
 */
char *dmem_GetText (int address, uint8 *addr_ptr, char *special_cr)
{ 
    int i;
    int offset;
    int code;
    char *text;
    char *p;

    text = malloc(50000);
    if (text == NULL)
        return NULL;

    p = text;
    *p= '\0';

    for (offset=address; offset<address+0x2000; offset+=8)
    {
        if (offset > address)
            p += sprintf (p, "%s", special_cr);

        /* write address */
        p += sprintf (p, "%04X  ", offset);

        /* write bytes */
        for (i=0; i<8; i++)
        {
            if ((((offset+i)>=0xe7b0) && ((offset+i)<0xe800))
             || (addr_ptr == NULL))
                code = mc6809_interface.LoadByte((offset+i)&0xffff);
            else
                code = addr_ptr[(offset&0x1fff)+i];
            p += sprintf(p, "%02X ", (int)(code&0xff));
        }
                
        /* write characters */
        p += sprintf(p, " ");
        for (i=0; i<8; i++)
        {
            if ((((offset+i)>=0xe7b0) && ((offset+i)<0xe800))
             || (addr_ptr == NULL))
                code = mc6809_interface.LoadByte((offset+i)&0xffff);
            else
                code = addr_ptr[(offset&0x1fff)+i];

            if ((code<0x20) || (code>0x7e))
                p += sprintf(p, ".");
            else
                p += sprintf(p, "%c", code);
        }
    }
    p += sprintf (p, "%s", special_cr);
    return text;
}



/* dmem_GetStepAddress:
 *  Get the jump address in step mode.
 */
int dmem_GetStepAddress(void)
{
    return dmem_GetJumpAddress();
}



/* dmem_GetDisplayPointer:
 *  Get the memory pointer for display.
 */
uint8 *dmem_GetDisplayPointer(void)
{
    uint8 *addr_ptr = NULL;
    int page = teo.debug.memory_address&0xe000;

    switch (page) {
    /* cartridge space */
    case 0x0000:
    case 0x2000:
        switch (teo.debug.cart_number) {
        case 0 :   /* if software bank 0 */
        case 1 :   /* if software bank 1 */
        case 2 :   /* if software bank 2 */
        case 3 :   /* if software bank 3 */
            addr_ptr = mem.rom.bank[teo.debug.cart_number]+(page&0x2000);
            break;
        case 4 :   /* if mapped on Memo7 */
            addr_ptr = mem.cart.bank[0]+(page&0x2000);
            break;
        default :  /* if mapped on RAM */
            addr_ptr = mem.ram.bank[teo.debug.cart_number-5]+(page&0x2000);
            break;
        }
        break;

    /* video space */
    case 0x4000:
        addr_ptr = mem.ram.bank[0]+teo.debug.video_number*0x2000;
        break;

    /* system space */
    case 0x6000:
    case 0x8000:
        addr_ptr = NULL;
        break;

    /* ram space */
    case 0xa000:
    case 0xc000:
        addr_ptr = mem.ram.bank[teo.debug.ram_number]+(page&0x2000);
        break;

    /* monitor space */
    case 0xe000:
    case 0xf000:
        addr_ptr = mem.mon.bank[teo.debug.mon_number];
        break;
    }
    return addr_ptr;
}

