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
 *                  L'émulateur Thomson TO8D
 *
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume
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
 *  Module     : dos/memmng.c
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou 1998
 *  Modifié par: Eric Botcazou 23/11/2000
 *               François Mouret 08/2011 20/09/2013 02/06/2014
 *
 *  Gestionnaire de mémoire du TO8.
 *   Ce module est indépendant du reste de l'émulateur et vient se greffer
 *   sur le débogueur. Il permet de naviguer dans la mémoire du TO8 sans
 *   influer sur son fonctionnement.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <conio.h>
#endif

#include "defs.h"
#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"
#include "debug.h"
#include "teo.h"


/* paramètres de disposition des modules à l'écran */
#define MENU_NLINE      7
#define MENU_POS_X      4
#define MENU_POS_Y     28
#define DASM_POS_X     48
#define DASM_POS_Y     13
#define   DUMP_POS_X    3
#define   DUMP_POS_Y   13
#define   DUMP_NBYTES   8
#define   DUMP_NLINES  10
#define DIALOG_POS_X    6
#define DIALOG_POS_Y   39

static char map_line[11][81]={
#ifdef FRENCH_LANGUAGE
"                          Gestionnaire de m‚moire du TO8                        ",
#else
"                               TO8 memory manager                               ",
#endif
"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄ¿",
"³                  ³         ³                   ³                   ³         ³",
"³0                3³4       5³6                 9³A                 D³E       F³",
"³0     BASICs     F³0 Video F³0     System      F³0      User       F³0  I/O  F³",
"³0      ROM       F³0  RAM  F³0      RAM        F³0      RAM        F³0  ROM  F³",
"³0                F³0       F³0                 F³0                 F³0       F³",
"³                  ³         ³                   ³                   ³         ³",
"ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄ´",
"³     bank   /3    ³bank   /1³                   ³     bank   /31    ³bank   /1³",
"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÙ                   ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÙ"
};

#ifdef FRENCH_LANGUAGE
static char menu_line[MENU_NLINE][30]={ "Commandes:",
                                        " c: charger un bloc m‚moire",
                                        " s: sauver un bloc m‚moire",
                                        " a: aller … l'<adresse>",
                                        " e: ‚diter un octet",
                                        " TAB,+,-: naviguer",
                                        " q: quitter le gestionnaire"};
#else
static char menu_line[MENU_NLINE][30]={ "Commands:",
                                        " c: load a memory block",
                                        " s: save a memory block",
                                        " a: go to <address>",
                                        " e: editing a byte",
                                        " TAB,+,-: navigating",
                                        " q: quit the manager"};
#endif

static struct P2D {
    int x;
    int y;
} hotspot_loc[5]={{12,10},{26,10},{61,10},{76,10},{45,50}};
static int   current_bank[4];
static int       max_bank[4]={  3,   1,  31,   1};
static int       bank_loc[4]={  0,   4, 0xA, 0xE};
static int      bank_size[4]={  4,   2,   4,   2};
static unsigned char *VRAM[2];
static unsigned char **mapper_table[4]={mem.rom.bank, VRAM, mem.ram.bank, mem.mon.bank};



/* Helper routines:
 */
static void ReadFileName(char *file_name,int pos_x, int pos_y, const char *mesg)
{
    textbackground(BLACK);
    textcolor(WHITE);
    gotoxy(pos_x,pos_y);
    cputs(mesg);
    _setcursortype(_NORMALCURSOR);
    cscanf("%s",file_name);
    _setcursortype(_NOCURSOR);
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


static void DeleteBox(int left, int top, int right, int bottom)
{
    register int i,j;

    for (i=top; i<=bottom; i++)
    {
        gotoxy(left,i);
        for (j=left; j<=right; j++)
            putch(32);
    }
}



/* SetHotspot:
 *  Déplace le curseur de sélection à l'emplacement voulu.
 */
static void SetHotspot(int hotspot)
{
    static int prev_hotspot;

    /* on éteint l'emplacement précédent du curseur */
    gotoxy(hotspot_loc[prev_hotspot].x,hotspot_loc[prev_hotspot].y);

    if (prev_hotspot == 4)
    {
        textbackground(BLACK);
        textcolor(WHITE);
        cprintf("->");
    }
    else
    {
        textbackground(BLACK);
        textcolor(LIGHTGREEN);
        cprintf("%2d",current_bank[prev_hotspot]);
    }

    /* on allume le nouveau */
    gotoxy(hotspot_loc[hotspot].x,hotspot_loc[hotspot].y);

    if (hotspot == 4)
    {
        textbackground(WHITE);
        textcolor(BLACK);
        cprintf("->");
    }
    else
    {
        textbackground(LIGHTGREEN);
        textcolor(BLACK);
        cprintf("%2d",current_bank[hotspot]);
    }
    
    prev_hotspot=hotspot;
}



/* DumpMemory:
 *  Transcrit un bloc mémoire au format ASCII.
 */
static void DumpMemory(char *out_str, int addr, int nbytes)
{
    register int i;
    int c;
    
    sprintf(out_str, "%04X ", addr);  /* affiche l'adresse */
    out_str+=5;

    for (i=0; i<nbytes; i++)
    {
        sprintf(out_str, "%02X ", LOAD_BYTE(addr+i));
        out_str+=3;
    }

    sprintf(out_str++, " ");
    
    for (i=0; i<nbytes; i++)
    {
        c=LOAD_BYTE(addr+i);
        
        if ((c>=32) && (c<=125))
            sprintf(out_str++, "%c", c);
        else
            sprintf(out_str++, ".");
    }
}



/* UpdateDasmAndDump:
 *  Désassemble l'instruction suivante et met à jour
 *  si nécessaire le dump mémoire.
 */
static void UpdateDasmAndDump(int *pc)
{
    int i, q=*pc/DUMP_NBYTES;
    struct MC6809_DASM mc6809_dasm;

    textbackground(BLACK);
    textcolor(WHITE);

    /* on décale l'écran de désassemblage d'une ligne vers le haut */
    movetext(DASM_POS_X,DASM_POS_Y+2,80,50,DASM_POS_X,DASM_POS_Y+1);
    DeleteBox(DASM_POS_X,50,79,50);

    for (i=0; i<MC6809_DASM_FETCH_SIZE; i++)
        mc6809_dasm.fetch[i]=LOAD_BYTE(*pc+i);

    mc6809_dasm.addr = *pc;
    mc6809_dasm.mode = MC6809_DASM_ASM_MODE;
    *pc+=dasm6809_Disassemble(&mc6809_dasm);

    if (*pc>0xFFFF)
        *pc&=0xFFFF;

    gotoxy(DASM_POS_X,50);
    cputs(mc6809_dasm.str);

    if ((*pc/DUMP_NBYTES) != q)
    {
        /* on décale l'écran de dump d'une ligne vers le haut */
        movetext(DUMP_POS_X,DUMP_POS_Y+2,40,DUMP_POS_Y+DUMP_NLINES,DUMP_POS_X,DUMP_POS_Y+1);
        DeleteBox(DUMP_POS_X,DUMP_POS_Y+DUMP_NLINES,40,DUMP_POS_Y+DUMP_NLINES);

        DumpMemory(mc6809_dasm.str, q*DUMP_NBYTES, DUMP_NBYTES);
        gotoxy(DUMP_POS_X,DUMP_POS_Y+DUMP_NLINES);
        cputs(mc6809_dasm.str);
    }
}



/* EditMemory:
 *  Edite un octet de mémoire.
 */
static int EditMemory(void)
{
    int addr, val;

    ReadAddress(&addr,DIALOG_POS_X,DIALOG_POS_Y,is_fr?"Adresse: ":"Address: ");
    ReadByte(&val,DIALOG_POS_X,DIALOG_POS_Y+1,is_fr?"Nouvelle valeur : ":"New value : ");
    DeleteBox(DIALOG_POS_X,DIALOG_POS_Y,40,DIALOG_POS_Y+1);

    STORE_BYTE(addr, val);
    UpdateDasmAndDump(&addr);
    return addr;
}



/* LoadMemory:
 *  Lit une portion de la mémoire.
 */
static int LoadMemory(void)
{
    char file_name[32];
    int  addr, addr_orig, d;
    FILE *file;

    ReadFileName(file_name,DIALOG_POS_X,DIALOG_POS_Y,is_fr?"Nom du fichier: ":"File name: ");
    ReadAddress(&addr,DIALOG_POS_X,DIALOG_POS_Y+1,is_fr?"Adresse de d‚but: ":"Start address: ");

    addr_orig=addr;

    if ((file=fopen(file_name,"rb")) == NULL)
    {
        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+2);
        cprintf(is_fr?"Erreur !!!":"Error !!!");
    }
    else
    {
        while ((addr<0x10000) && ((d=fgetc(file))!=EOF))
        {
            STORE_BYTE(addr, d);
            addr++;
        }

        fclose(file);
        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+2);
        cprintf(is_fr?"Termin‚":"Finished");
    }

    gotoxy(DIALOG_POS_X,DIALOG_POS_Y+3);
    cprintf(is_fr?"Appuyer sur <espace>":"Press <space bar>");

    while (getch() != 32)
        ;

    DeleteBox(DIALOG_POS_X,DIALOG_POS_Y,40,DIALOG_POS_Y+3);

    UpdateDasmAndDump(&addr_orig);
    return addr_orig;
}



/* SaveMemory:
 *  Sauvegarde une portion de la mémoire.
 */
static void SaveMemory(void)
{
    char file_name[32];
    struct MC6809_DASM mc6809_dasm;
    int addr1,addr2,c,i;
    FILE *file;

    ReadFileName(file_name,DIALOG_POS_X,DIALOG_POS_Y,is_fr?"Nom du fichier: ":"File name: ");
    ReadAddress(&addr1,DIALOG_POS_X,DIALOG_POS_Y+1,is_fr?"Adresse de d‚but: ":"Start address: ");
    ReadAddress(&addr2,DIALOG_POS_X,DIALOG_POS_Y+2,is_fr?"Adresse de fin: ":"End address: ");

    gotoxy(DIALOG_POS_X, DIALOG_POS_Y+3);
    cprintf(is_fr?"Format: 1.binaire":"Format: 1.binary");
    gotoxy(DIALOG_POS_X, DIALOG_POS_Y+4);
    cprintf("        2.ASCII");
    gotoxy(DIALOG_POS_X, DIALOG_POS_Y+5);
    cprintf(is_fr?"        3.assembleur":"        3.assembler");

    do
        c=getch();
    while ((c<'1') || (c>'3'));

    if ((file=fopen(file_name,"wb")) == NULL)
    {
        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+6);
        cprintf(is_fr?"Erreur !!!":"Error !!!");
    }
    else
    {
        switch(c)
        {
            case '1':
                for (i=addr1; i<=addr2; i++)
                    fputc(LOAD_BYTE(i)&0xFF, file);
                break;

            case '2':
                do
                {
                    DumpMemory(mc6809_dasm.str, addr1, 8);
                    addr1+=8;
                    fprintf(file, "%s\r\n", mc6809_dasm.str);
                }
                while (addr1 < addr2);
                break;

            case '3':
                do
                {
                    for (i=0; i<MC6809_DASM_FETCH_SIZE; i++)
                        mc6809_dasm.fetch[i]=LOAD_BYTE(addr1+i);

                    mc6809_dasm.addr = addr1;
                    mc6809_dasm.mode = MC6809_DASM_BINASM_MODE;
                    addr1 += dasm6809_Disassemble(&mc6809_dasm);
                    fprintf(file, "%s\r\n", mc6809_dasm.str);
                }
                while (addr1 < addr2);
                break;
        }
                        
        fclose(file);
        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+6);
        cprintf(is_fr?"Termin‚":"Finished");
    }
                    
    gotoxy(DIALOG_POS_X,DIALOG_POS_Y+7);
    cprintf(is_fr?"Appuyer sur <espace>":"Press <space bar>");

    while (getch() != 32)
        ;

    DeleteBox(DIALOG_POS_X,DIALOG_POS_Y,40,DIALOG_POS_Y+7);
}



/* MemoryManager:
 *  Point d'entrée du module; affiche l'écran du gestionnaire
 *  de mémoire et lance la boucle principale.
 */
void MemoryManager(void)
{
    register int i;
             int c=0,hotspot=4,pc;
    struct MC6809_REGS regs;
    unsigned char *mapper_orig[16];

    /* on sauvegarde d'abord l'état du mapper */
    for (i=0; i<16; i++)
        mapper_orig[i]=mempager.segment[i];

    /* on lit ensuite l'état des banques de mémoire et du PC */
    VRAM[0]=mem.ram.bank[0]+0x2000;
    VRAM[1]=mem.ram.bank[0];
    current_bank[0]=mempager.cart.rom_page;
    current_bank[1]=mempager.screen.page;
    current_bank[2]=mempager.data.page;
    current_bank[3]=mempager.mon.page;
    mc6809_GetRegs(&regs);
    pc=regs.pc;

    /* on construit l'écran du gestionnaire */
    clrscr();

    /* les trois barres de titres */
    textbackground(BLUE);
    textcolor(LIGHTGRAY);
    cputs(map_line[0]);
    gotoxy(DUMP_POS_X-2,DUMP_POS_Y);
    cprintf(is_fr?"          Contenu de la m‚moire           ":"          Content of the memory           ");
    gotoxy(DASM_POS_X-2,DASM_POS_Y);
    cprintf(is_fr?"       D‚sassembleur MC6809E       ":"       MC6809E disassembler        ");

    /* la carte de la mémoire */
    textbackground(BLACK);
    textcolor(WHITE);
    gotoxy(1,2);
    for (i=1; i<9; i++)
        cputs(map_line[i]);

    /* le désassembleur */
    for (i=0; i<50-DASM_POS_Y; i++)
        UpdateDasmAndDump(&pc);

    /* les volets des banques */
    textcolor(LIGHTGREEN);
    gotoxy(1,10);
    cputs(map_line[9]);
    cputs(map_line[10]);
    for (i=0; i<4; i++)
    {
        gotoxy(hotspot_loc[i].x,hotspot_loc[i].y);
        cprintf("%2d",current_bank[i]);
    }

    /* le menu */
    textcolor(LIGHTCYAN);
    for (i=0; i<MENU_NLINE; i++)
    {
        gotoxy(MENU_POS_X,MENU_POS_Y+i);
        cputs(menu_line[i]);
    }

    SetHotspot(hotspot);

    while ((c != 'q') && (c != 'Q'))  /* boucle principale */
    {
        switch (c=getch())
        {
            case 9: /* TAB: circulation du curseur */
                if (++hotspot == 5)
                    hotspot=0;
                SetHotspot(hotspot);
                break;

            case '+': /* banque suivante ou instruction suivante */
                if (hotspot == 4)
                    UpdateDasmAndDump(&pc);
                else
                {
                    if (current_bank[hotspot] < max_bank[hotspot])
                    {
                        int mod=0;

                        if (hotspot==2)  /* espace données */
                            mod=2;
                            
                        current_bank[hotspot]++;
                        
                        for (i=0; i<bank_size[hotspot]; i++)
                            mempager.segment[bank_loc[hotspot]+i]=
                             mapper_table[hotspot][current_bank[hotspot]]+(i^mod)*0x1000;

                        SetHotspot(hotspot);
                    }
                }
                break;

            case '-': /* banque précédente */
                if (hotspot != 4)
                {
                    if (current_bank[hotspot] > 0)
                    {
                        int mod=0;

                        if (hotspot==2)  /* espace données */
                            mod=2;
                            
                        current_bank[hotspot]--;
                        
                        for (i=0; i<bank_size[hotspot]; i++)
                            mempager.segment[bank_loc[hotspot]+i]=
                             mapper_table[hotspot][current_bank[hotspot]]+(i^mod)*0x1000;

                        SetHotspot(hotspot);
                    }
                }
                break;

            case 'e': /* édition d'un octet de mémoire */
            case 'E':
                pc=EditMemory();
                hotspot=4;
                SetHotspot(hotspot);
                break;

            case 'a': /* saut à l'adresse voulue */
            case 'A':
                ReadAddress(&pc,DIALOG_POS_X,DIALOG_POS_Y,is_fr?"Nouvelle adresse: ":"New address: ");
                DeleteBox(DIALOG_POS_X,DIALOG_POS_Y,40,DIALOG_POS_Y);
                UpdateDasmAndDump(&pc);
                hotspot=4;
                SetHotspot(hotspot);
                break;

            case 'c': /* chargement d'une portion mémoire */
            case 'C':
                pc=LoadMemory();
                hotspot=4;
                SetHotspot(hotspot);
                break;

            case 's': /* sauvegarde d'une portion mémoire */
            case 'S':
                SaveMemory();
                break;
        } /* end of switch */
    }

    /* on restaure l'ancien mapper */
    for (i=0; i<16; i++)
        mempager.segment[i]=mapper_orig[i];
}

