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
 *  Module     : dos/debug.c
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Eric Botcazou 27/11/2002
 *               François Mouret 08/2011 01/11/2012 02/06/2014 28/10/2017
 *               Samuel Cuella   02/2020
 *
 *  Débogueur du TO8.
 */


#ifndef SCAN_DEPEND
   #include <conio.h>
   #include <stdio.h>
#endif
#include <stdlib.h>
#include <assert.h>


#include "defs.h"
#include "teo.h"
#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"
#include "alleg/gfxdrv.h"
#include "dos/memmng.h"
#include "media/disk.h"
#include "to8dbg.h"
#include "teo.h"
#include "gettext.h"

#define MENU_NLINES 8
#define MENU_POS_X  4
#define MENU_POS_Y 42
static char **_menu_line = NULL;

#undef DASM_NLINES
#define DASM_NLINES 40

#define BREAK_MENU_NLINES 4
char **_break_menu_line = NULL;

static int breakpoint[MAX_BREAKPOINTS];

static struct MC6809_REGS regs, prev_regs;


static char **ddebug_GetMenu(void)
{
    if(!_menu_line){
        int i = 0;
        _menu_line = (char**)malloc(sizeof(char *)*MENU_NLINES);
        _menu_line[i++] = _("Commands:");
        _menu_line[i++] = _(" c: continue");
        _menu_line[i++] = _(" n: run next locale instruction");
        _menu_line[i++] = _(" s: run next instruction");
        _menu_line[i++] = _(" b: manage breakpoints");
        _menu_line[i++] = _(" a: show TO8 screen");
        _menu_line[i++] = _(" m: show TO8 memory manager");
        _menu_line[i++] = _(" q: quit the debugger   ");

        assert(i <= MENU_NLINES);
    }
    return _menu_line;
}

static char **ddebug_GetBreakMenu(void)
{
    if(!_break_menu_line){
        int i = 0;
        _break_menu_line = (char**)malloc(sizeof(char *)*BREAK_MENU_NLINES);
        _break_menu_line[i++] = _("Commands:                     ");
        _break_menu_line[i++] = _(" b: add a breakpoint       ");
        _break_menu_line[i++] = _(" d: delete a breakpoint     ");
        _break_menu_line[i++] = _(" q: quit                      ");

        assert(i <= BREAK_MENU_NLINES);
    }
    return _break_menu_line;
}


/* DisplayRegs:
 *  Affiche les registres du 6809.
 */
static void DisplayRegs(void)
{
    textbackground(BLUE);
    textcolor(LIGHTGRAY);

    gotoxy(1,1);
    cprintf("        CPU MC6809E           ");

    gotoxy(1,11);
    cprintf(_("      PIA 6846 system         "));
    gotoxy(1,16);
    cprintf(_("      PIA 6821 system         "));
    gotoxy(1,20);
    cprintf(_("   PIA 6821 music and games   "));
    gotoxy(1,24);
    cprintf("    Gate Array mode_page      ");

    gotoxy(1,30);
    cprintf("    Gate Array disk_ctrl      ");

    gotoxy(1,35);
    cprintf(_("     Memory pages status      "));
    textbackground(BLACK);
    textcolor(WHITE);

    gotoxy(2,2);
    cprintf("PC: %04X [%04X] DP: %02X",regs.pc,prev_regs.pc,regs.dp);
    gotoxy(2,3);
    cprintf(" X: %04X         Y: %04X",regs.xr,regs.yr);
    gotoxy(2,4);
    cprintf(" U: %04X         S: %04X",regs.ur,regs.sr);
    gotoxy(2,5);
    cprintf(" A: %02X           B: %02X",regs.ar,regs.br);
    gotoxy(2,6);
    cprintf("CC: %c%c%c%c%c%c%c%c   IRQ: %d" ,regs.cc&0x80 ? 'E' : '.'
                                             ,regs.cc&0x40 ? 'F' : '.'
                                             ,regs.cc&0x20 ? 'H' : '.'
                                             ,regs.cc&0x10 ? 'I' : '.'
                                             ,regs.cc&0x08 ? 'N' : '.'
                                             ,regs.cc&0x04 ? 'Z' : '.'
                                             ,regs.cc&0x02 ? 'V' : '.'
                                             ,regs.cc&0x01 ? 'C' : '.'
                                             ,mc6809_irq);

    gotoxy(2,8);
    cprintf("[S ] [S+1] [S+2] [S+3] [S+4]");
    gotoxy(2,9);
    cprintf(" %2X   %2X    %2X    %2X    %2X",LOAD_BYTE(regs.sr),LOAD_BYTE(regs.sr+1),
              LOAD_BYTE(regs.sr+2),LOAD_BYTE(regs.sr+3),LOAD_BYTE(regs.sr+4));

    gotoxy(2,12);
    cprintf(" CSR: %02X   CRC: %02X",mc6846.csr,mc6846.crc);
    gotoxy(2,13);
    cprintf("DDRC: %02X   PRC: %02X",mc6846.ddrc,mc6846.prc);
    gotoxy(2,14);
    cprintf(" TCR: %02X  TMSB: %02X  TLSB: %02X",mc6846.tcr,mc6846.tmsb,
      mc6846.tlsb);

    gotoxy(2,17);
    cprintf("CRA: %02X  DDRA: %02X  PDRA: %02X",pia_int.porta.cr,
      pia_int.porta.ddr,mc6821_ReadPort(&pia_int.porta));
    gotoxy(2,18);
    cprintf("CRB: %02X  DDRB: %02X  PDRB: %02X",pia_int.portb.cr,
      pia_int.portb.ddr,mc6821_ReadPort(&pia_int.portb));

    gotoxy(2,21);
    cprintf("CRA: %02X  DDRA: %02X  PDRA: %02X",pia_ext.porta.cr,
      pia_ext.porta.ddr,mc6821_ReadPort(&pia_ext.porta));
    gotoxy(2,22);
    cprintf("CRB: %02X  DDRB: %02X  PDRB: %02X",pia_ext.portb.cr,
      pia_ext.portb.ddr,mc6821_ReadPort(&pia_ext.portb));

    gotoxy(2,25);
    cprintf("P_DATA: %02X   P_ADDR: %02X",mode_page.p_data,mode_page.p_addr);
    gotoxy(2,26);
    cprintf("LGAMOD: %02X     SYS1: %02X",mode_page.lgamod,mode_page.system1);
    gotoxy(2,27);
    cprintf("  SYS2: %02X     DATA: %02X",mode_page.system2,mode_page.ram_data);
    gotoxy(2,28);
    cprintf("  CART: %02X   COMMUT: %02X",mode_page.cart,mode_page.commut);

    gotoxy(2,31);
    cprintf("CMD0: %02X  CMD1: %02X  CMD2: %02X", disk[0].dkc->wr0,
      disk[0].dkc->wr1, disk[0].dkc->wr2);
       gotoxy(2,32);
    cprintf(" STAT0: %02X    STAT1: %02X",disk[0].dkc->rr0, disk[0].dkc->rr1);
    gotoxy(2,33);
    cprintf(" WDATA: %02X    RDATA: %02X",disk[0].dkc->wr3, disk[0].dkc->rr3);

    gotoxy(2,36);
    cprintf(_("ROM cartridge page    : %d"),
                                                     mempager.cart.rom_page);
    gotoxy(2,37);
    cprintf(_("RAM cartridge page    : %d"),
                                                     mempager.cart.ram_page);
    gotoxy(2,38);
    cprintf(_("VRAM page             : %d"),
                                                     mempager.screen.vram_page);
    gotoxy(2,39);
    cprintf(_("RAM page (register)   : %d"),
                                                     mempager.data.reg_page);
    gotoxy(2,40);
    cprintf(_("RAM page (PIA)        : %d"),
                                                     mempager.data.pia_page);
}



static void ReadAddress(int *addr, int pos_x, int pos_y, const char *mesg)
{
    textbackground(BLACK);
    textcolor(WHITE);
    gotoxy(pos_x,pos_y);
    cputs(mesg);
    _setcursortype(_NORMALCURSOR);
    cscanf("%4X",addr);
    *addr&=0xFFFF;
    _setcursortype(_NOCURSOR);
}



static void ReadByte(int *val, int pos_x, int pos_y, const char *mesg)
{
    textbackground(BLACK);
    textcolor(WHITE);
    gotoxy(pos_x,pos_y);
    cputs(mesg);
    _setcursortype(_NORMALCURSOR);
    cscanf("%2x",val);
    *val&=0xFF;
    _setcursortype(_NOCURSOR);
}



/* SetBreakPoints:
 *  Gestion des points d'arrêt.
 */
static void SetBreakpoints(void)
{
    int c=0, i;
    char **break_menu_line;

    break_menu_line = ddebug_GetBreakMenu();

    do
    {
	if ((c=='b') || (c=='B'))
        {
            for (i=0; i < MAX_BREAKPOINTS; i++)
                if (breakpoint[i] == 0)
                    break;

            if (i < MAX_BREAKPOINTS)
            {
                int addr;
                ReadAddress(&addr, 2, 14+MAX_BREAKPOINTS/2+BREAK_MENU_NLINES,
                                                _("address: "));
                if ((0<=addr) && (addr<=0xFFFF))
                    breakpoint[i] = addr+1;
            }
        }

	if ((c=='d') || (c=='D'))
        {
            int n;
            ReadByte(&n, 2, 14+MAX_BREAKPOINTS/2+BREAK_MENU_NLINES,
                             _("number: "));
            if ((1<=n) && (n<=MAX_BREAKPOINTS))
                breakpoint[n-1] = 0;
        }

        textbackground(BLUE);
        textcolor(LIGHTGRAY);

        gotoxy(1,11);
        cprintf(_("         Breakpoint          "));

        textbackground(BLACK);
        textcolor(WHITE);

        for (i=0; i<MAX_BREAKPOINTS/2; i++)
        {
            gotoxy(1,12+i);
            cprintf(" point %1d:      point %2d:      ", i+1, i+1+MAX_BREAKPOINTS/2);

            gotoxy(11, 12+i);
            if (breakpoint[i])
                cprintf("%04X", breakpoint[i]-1);

            gotoxy(26, 12+i);
            if (breakpoint[i+MAX_BREAKPOINTS/2])
                cprintf("%04X", breakpoint[i+MAX_BREAKPOINTS/2]-1);
        }

        gotoxy(1,12+MAX_BREAKPOINTS/2);
        cprintf("                              ");

        textcolor(LIGHTCYAN);

        /* le menu */
        for (i=0; i<BREAK_MENU_NLINES; i++)
        {
           gotoxy(1,13+MAX_BREAKPOINTS/2+i);
           cputs(break_menu_line[i]);
        }

        for (i=13+MAX_BREAKPOINTS/2+BREAK_MENU_NLINES; i<34; i++)
        {
            gotoxy(1,i);
            cprintf("                              ");
        }

        c=getch();

    } while ((c != 'q') && (c != 'Q'));
}


/* ------------------------------------------------------------------------- */


/* ddebug_Run:
 *  Fonction principale du débogueur.
 */
void ddebug_Run(void)
{
    register int i,j;
             int c=0,pc;
    struct MC6809_DASM mc6809_dasm;
    char **menu_line;

    menu_line = ddebug_GetMenu();
    mc6809_FlushExec ();

    _set_screen_lines(50);
    _setcursortype(_NOCURSOR);

    do
    {
        mc6809_GetRegs(&prev_regs);

        if ((c=='c') || (c=='C'))
        {
            int done = 0;

            while (!done && !kbhit())
            {
                mc6809_GetRegs(&prev_regs);
                mc6809_StepExec();
                mc6809_GetRegs(&regs);

                for (i=0; i<MAX_BREAKPOINTS; i++)
                {
                    if (breakpoint[i]-1 == regs.pc)
                    {
                        done = 1;
                        break;
                    }
                }
            }
        }

        if ((c=='n') || (c=='N')) {
            mc6809_GetRegs(&regs);

            switch (LOAD_BYTE(regs.pc))
            {
                case 0x17:  /* LBSR         */
                case 0x8D:  /* BSR          */
                case 0x9D:  /* JSR direct   */
                case 0xAD:  /* JSR indexed  */
                case 0xBD:  /* JSR extended */
                    pc = regs.pc;
                    do
                    {
                        mc6809_GetRegs(&prev_regs);
                        mc6809_StepExec();
                        mc6809_GetRegs(&regs);
                    } while (((regs.pc<pc) || (regs.pc>pc+5)) && !kbhit());
                    break;

                default:
                    mc6809_StepExec();
                    break;
            }
        }

        if ((c=='s') || (c=='S'))
            mc6809_StepExec();

        if ((c=='b') || (c=='B'))
            SetBreakpoints();

        if ((c=='a') || (c=='A'))
        {
            SetGraphicMode(RESTORE);
            getch();
            SetGraphicMode(SHUTDOWN);
            _set_screen_lines(50);
            _setcursortype(_NOCURSOR);
        }

        if ((c=='m') || (c=='M'))
            MemoryManager();

        textbackground(BLACK);
        textcolor(LIGHTCYAN);
        clrscr();

        /* le menu */
        for (i=0; i<MENU_NLINES; i++)
        {
            gotoxy(MENU_POS_X,MENU_POS_Y+i);
            cputs(menu_line[i]);
        }

        mc6809_GetRegs(&regs);
        DisplayRegs();

        pc=regs.pc;

        textbackground(BLUE);
        textcolor(LIGHTGRAY);

        /* le désassembleur */
        gotoxy(33,1);
        cprintf(_("               MC6809E Disassembler              "));

        textbackground(BLACK);
        textcolor(WHITE);

        for (i=0; i<DASM_NLINES; i++)
        {
            for (j=0; j<5; j++)
                mc6809_dasm.fetch[j]=LOAD_BYTE(pc+j);

            mc6809_dasm.addr = pc;
            mc6809_dasm.mode = MC6809_DASM_BINASM_MODE;
            pc=(pc+dasm6809_Disassemble(&mc6809_dasm))&0xFFFF;
            gotoxy(33,i+2);
            cputs(mc6809_dasm.str);
        }
        c=getch();

    } while ((c != 'q') && (c != 'Q'));
}

