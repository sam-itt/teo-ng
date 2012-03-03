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
 *  Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : hardware.c
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 24/10/2003
 *               François Mouret 18/09/2006 02/02/2012
 *
 *  Emulation de l'environnement matériel du MC6809E:
 *	- carte mémoire
 *	- circuits d'entrée/sortie du système
 *	- circuits d'entrée/sortie des périphériques
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <time.h>
#endif

#include "mc68xx/mc6809.h"
#include "mc68xx/mc6821.h"
#include "mc68xx/mc6846.h"
#include "intern/defs.h"
#include "intern/disk.h"
#include "intern/errors.h"
#include "intern/hardware.h"
#include "intern/keyboard.h"
#include "intern/k7.h"
#include "intern/mouse.h"
#include "intern/printer.h"
#include "to8.h"


/* les composants métériels de l'émulateur */
struct MC6846_PIA mc6846;      /* PIA 6846 système         */
struct MC6821_PIA pia_int;     /* PIA 6821 système         */
struct MC6821_PIA pia_ext;     /* PIA 6821 musique et jeux */
struct GATE_ARRAY mode_page;   /* Gate Array mode page     */
struct EF9369 pal_chip;        /* circuit de palette vidéo */
struct MEMORY_PAGER mempager;  /* carte mémoire logique    */
struct MEMORY mem;             /* carte mémoire physique   */
struct MOTHERBOARD mb;         /* (pseudo) carte mère      */

mc6809_clock_t screen_clock;



/* Fonctions de commutation de l'espace mémoire:
 */
static void update_cart(void)
{
    static uint8 cart_garbage[32];

    if (mode_page.cart&0x20)
    {   /* l'espace cartouche est mappé sur la RAM */
        mempager.segment[0x0]=mem.ram.bank[mempager.cart.ram_page];
        mempager.segment[0x1]=mem.ram.bank[mempager.cart.ram_page]+0x1000;
        mempager.segment[0x2]=mem.ram.bank[mempager.cart.ram_page]+0x2000;
        mempager.segment[0x3]=mem.ram.bank[mempager.cart.ram_page]+0x3000;

        /* nécessaire pour Bob Morane SF !! */
        if (mode_page.system1&8)
            mc6846.prc|=4;
    }
    else if (mc6846.prc&4)
    {   /* l'espace cartouche est mappé sur la ROM interne */
        mempager.segment[0x0]=mem.rom.bank[mempager.cart.rom_page];
        mempager.segment[0x1]=mem.rom.bank[mempager.cart.rom_page]+0x1000;
        mempager.segment[0x2]=mem.rom.bank[mempager.cart.rom_page]+0x2000;
        mempager.segment[0x3]=mem.rom.bank[mempager.cart.rom_page]+0x3000;
    }
    else
    {   /* l'espace cartouche est mappé sur la cartouche Mémo7 */
        if (mem.cart.nbank==0)
            mempager.segment[0x0]=cart_garbage;
        else
        {
            mempager.segment[0x0]=mem.cart.bank[mempager.cart.page];
            mempager.segment[0x1]=mem.cart.bank[mempager.cart.page]+0x1000;
            mempager.segment[0x2]=mem.cart.bank[mempager.cart.page]+0x2000;
            mempager.segment[0x3]=mem.cart.bank[mempager.cart.page]+0x3000;
        }
    }
}


static void update_screen(void)
{
    mempager.segment[0x4]=mem.ram.bank[0]+(1-mempager.screen.page)*0x2000;
    mempager.segment[0x5]=mem.ram.bank[0]+(1-mempager.screen.page)*0x2000+0x1000;
}


static void update_system(void)
{
    mempager.segment[0x6]=mem.ram.bank[mempager.system.page];
    mempager.segment[0x7]=mem.ram.bank[mempager.system.page]+0x1000;
    mempager.segment[0x8]=mem.ram.bank[mempager.system.page]+0x2000;
    mempager.segment[0x9]=mem.ram.bank[mempager.system.page]+0x3000;
}


static void update_data(void)
{
    if (mode_page.system1&0x10)
        mempager.data.page = mempager.data.reg_page;
    else
        mempager.data.page = mempager.data.pia_page;

    mempager.segment[0xA]=mem.ram.bank[mempager.data.page]+0x2000;
    mempager.segment[0xB]=mem.ram.bank[mempager.data.page]+0x3000;
    mempager.segment[0xC]=mem.ram.bank[mempager.data.page];
    mempager.segment[0xD]=mem.ram.bank[mempager.data.page]+0x1000;
}


static void update_mon(void)
{
    mempager.segment[0xE]=mem.mon.bank[mempager.mon.page];
    mempager.segment[0xF]=mem.mon.bank[mempager.mon.page]+0x1000;
}



/* update_color:
 *  Met à jour la couleur correspond à l'index spécifié.
 */
static void update_color(int index)
{
    static int gamma[16]={0,100,127,147,163,179,191,203,215,223,231,239,243,247,251,255};

    to8_SetColor(index, gamma[ pal_chip.color[index].gr&0xF],
                        gamma[(pal_chip.color[index].gr&0xF0)>>4],
                        gamma[ pal_chip.color[index].b&0xF]);
}



/* SetDeviceRegister:
 *  Dépose un octet dans le registre du périphérique et
 *  modifie en conséquence son état.
 */
static void SetDeviceRegister(int addr, int val)
{
    static int new_lsb;
    int index;

    switch (addr)
    {
        /* PIA 6846 système */
        case 0xE7C1:
            mc6846_WriteCommand(&mc6846, val);
            break;

        case 0xE7C2:
            mc6846.ddrc=val;
            break;

        case 0xE7C3:
            mc6846_WriteData(&mc6846, val);

            /* bit 0: sélection demi-page VRAM */
            mempager.screen.page = (mc6846.prc&1 ? 1 : 0);
            mempager.screen.update();

            /* bit 2: commutation page cartouche */
            mempager.cart.update();

            /* bit 4: sélection page moniteur */
            mempager.mon.page = (mc6846.prc&0x10 ? 1 : 0);
            mempager.mon.update();

            /* bit 5: ACK clavier */
            mc6804_SetACK(mc6846.prc&0x20);
            break;

        case 0xE7C5:
            mc6846_SetTcr(&mc6846, val);
            break;

        case 0xE7C6:
            mc6846_SetTmsb(&mc6846, val);
            break;

        case 0xE7C7:
            mc6846_SetTlsb(&mc6846, val);
            break;

        /* PIA 6821 système */
        case 0xE7C8:
            mc6821_WriteData(&pia_int.porta, val);

            /* écriture sur le port donnée de l'imprimante */
            pr90612_WriteData(0xFE, mc6821_ReadPort(&pia_int.porta));
            break;

        case 0xE7C9:
            mc6821_WriteData(&pia_int.portb, val);

            /* écriture sur le port donnée de l'imprimante */
            pr90612_WriteData(0x01, mc6821_ReadPort(&pia_int.portb));

            /* écriture sur le STROBE de l'imprimante */
            pr90612_SetStrobe(mc6821_ReadPort(&pia_int.portb)&0x02);

            switch (mc6821_ReadPort(&pia_int.portb)&0xF8)
            {
                case 0xF0:  /* DDRB 0x0F */
                    mempager.data.pia_page=2;
                    break;

                case 0xE8:  /* DDRB 0x17 */
                    mempager.data.pia_page=3;
                    break;

                case 0x18:  /* DDRB 0xE7 */
                    mempager.data.pia_page=4;
                    break;

                case 0x58:  /* DDRB 0xA7 */
                    mempager.data.pia_page=5;
                    break;

                case 0x98:  /* DDRB 0x67 */
                    mempager.data.pia_page=6;
                    break;

                case 0xD8:  /* DDRB 0x27 */
                    mempager.data.pia_page=7;
                    break;

/* le TO7-70 et le TO9 ont une correspondance subtilement différente: 
   les cas 0x58 et 0x98 sont interchangés !!!
   (voir Chip: le Crépuscule du Naja routines 8029 et 8085 et le manuel
    technique TO8/9/9+ page 45) */
            }

            mempager.data.update();
            break;

        case 0xE7CA:
            mc6821_WriteCommand(&pia_int.porta, val);
#ifdef LEP_Motor
            if ((val&0x30) == 0x30)
                LEP_Motor(pia_int.porta.cr&8 ? OFF : ON);
#endif
            break;

        case 0xE7CB:
            mc6821_WriteCommand(&pia_int.portb, val);
            break;

        /* PIA 6821 musique et jeux */
        case 0xE7CC:
            mc6821_WriteData(&pia_ext.porta, val);
            break;

        case 0xE7CD:
            mc6821_WriteData(&pia_ext.portb, val);
            if (!(mc6846.crc&8))  /* MUTE son inactif */
                if ((mc6821_ReadCommand(&pia_ext.portb)&4) != 0) /* donnée port B */
                    to8_PutSoundByte(mc6809_clock(), (mc6821_ReadPort(&pia_ext.portb)&0x3F)<<2);
            break;

        case 0xE7CE:
            mc6821_WriteCommand(&pia_ext.porta, val);
            break;

        case 0xE7CF:
            mc6821_WriteCommand(&pia_ext.portb, val);
            break;

        /* Gate Array lecteur de disquettes */
        case 0xE7D0:
            disk_ctrl_cmd0(val);
            break;

        case 0xE7D1:
            disk_ctrl_cmd1(val);
            break;

        case 0xE7D2:
            disk_ctrl_cmd2(val);
            break;

        case 0xE7D3:
            disk_ctrl_wdata(val);
            break;

        case 0xE7D4:
            disk_ctrl_wclk(val);
            break;

        case 0xE7D5:
            disk_ctrl_wsect(val);
            break;

        case 0xE7D6:
            disk_ctrl_wtrck(val);
            break;

        case 0xE7D7:
            disk_ctrl_wcell(val);
            break;

        /* Gate Array mode page */
        case 0xE7DA:
            index = (mode_page.p_addr>>1);

            if (mode_page.p_addr&1)
            {
                val = 0xE0|(val&0xF);

                if ((pal_chip.color[index].b != val) || new_lsb)
                {
                    pal_chip.color[index].b = val;
                    pal_chip.update(index);
                    new_lsb = FALSE;
                    to8_new_video_params = TRUE;
                }
            }
            else if (pal_chip.color[index].gr != val)
            {
                pal_chip.color[index].gr = val;
                new_lsb = TRUE;
            }

            mode_page.p_addr=(mode_page.p_addr+1)&0x1F;
            break;

        case 0xE7DB:
            mode_page.p_addr=val&0x1F;
            break;

        case 0xE7DC:
            if (val != mode_page.lgamod)
            {
                mode_page.lgamod=val;
                to8_new_video_params=TRUE;
            }
            break;

        case 0xE7DD:
            if (mempager.screen.vram_page != (val>>6)) /* commutation de la VRAM */
            {
               mempager.screen.vram_page=(val>>6);
               to8_new_video_params=TRUE;
            }
            if ( ((mode_page.system2&0xF) != (val&0xF)) && to8_SetBorderColor)
                to8_SetBorderColor(mode_page.lgamod, val&0xF);

            mode_page.system2=val;
            break;

        case 0xE7E5:
            mode_page.ram_data=val;
 
            /* bit 0-4: sélection page de l'espace données */
            mempager.data.reg_page=val&0x1F;
            mempager.data.update();
            break;

        case 0xE7E6:  /* commutation de l'espace cartouche */
            mode_page.cart=val;
            mempager.cart.ram_page=val&0x1F;
            mempager.cart.update();
            break;

        case 0xE7E7:
            mode_page.system1=(val&0x5F)+(mode_page.system1&0xA0);

            /* bit 4: mode de commutation de l'espace données */
            mempager.data.update();
            break;

    } /* end of switch */
}


#ifdef DEBUG  /* la fonction n'est pas inlinée */

void DrawGPL(int addr)
{ 
    int pt,col;

    if (addr>=0x1F40)
        return;

    pt =mem.ram.bank[mempager.screen.vram_page][addr];
    col=mem.ram.bank[mempager.screen.vram_page][addr+0x2000];

    to8_DrawGPL(mode_page.lgamod, addr, pt, col);
}

#endif

/* GetRealValue_lp4:
 *  Récupère la valeur ajustée de mode_page.lp4
 */
static inline int GetRealValue_lp4(void)
{
    mc6809_clock_t spot_pos, spot_point;
    int lp4;

    /* Positionne d'office b7 et b5 à 1 */
    lp4=mode_page.lp4|0xA0;

    /* Position du spot dans l'écran */
    spot_pos=mc6809_clock()-screen_clock;

    /* Ajoute 1 cycle si ligne impaire:
        Pour assurer la "fébrilité" du spot ?
        Chinese Stack ne fonctionne bien qu'avec un ajout
        de 1 cycle pour les lignes impaires, et le raster de
        couleurs de HCL en est parfaitement centré */
    if ((spot_pos/FULL_LINE_CYCLES)&1)
        spot_pos+=1;
    
    /* Positionne b7 à 0 si le spot est au dessus ou au dessous de l'écran affichable */
    spot_point=spot_pos/FULL_LINE_CYCLES;    /* Numéro de ligne */
    if ((spot_point<TOP_SHADOW_LINES+TOP_BORDER_LINES)
        || (spot_point>=TOP_SHADOW_LINES+TOP_BORDER_LINES+WINDOW_LINES))
        lp4&=0x7f;

    /* Positionne b5 à 0 si le spot est à droite ou à gauche de l'écran affichable */
    spot_point=spot_pos%FULL_LINE_CYCLES;    /* Numéro de colonne */
    if ((spot_point<LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES)
        || (spot_point>=LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES+WINDOW_LINE_CYCLES))
        lp4&=0xDF;

    return lp4;
}


/* StoreByte:
 *  Ecrit un octet en mémoire.
 */
void StoreByte(int addr, int val)
{
    /* masque de commutation des pages de la cartouche */
    static const int page_mask[]={0, 0x0, 0x1, 0x2, 0x2};
    
    int msq=addr>>12;

    switch (msq)
    {
        case 0x0: /* espace cartouche */
        case 0x1:
        case 0x2:
        case 0x3:
            if (mode_page.cart&0x20)  /* l'espace est mappé sur la RAM */
            {
                if (mode_page.cart&0x40) /* l'écriture est autorisée */
                {
                    STORE_BYTE(addr, val);

                    if ((mempager.screen.vram_page==mempager.cart.ram_page) && mb.direct_screen_mode)
                        DrawGPL(addr&0x1FFF);
                }
            }
            else if (addr<=0x1FFF)  /* commutation par latchage */
            {
                if (mc6846.prc&4)
                    mempager.cart.rom_page = addr&3;
                else
                    mempager.cart.page = addr&page_mask[mem.cart.nbank];
                    
                mempager.cart.update();
            }
            break;

        case 0x4: /* espace écran */
        case 0x5:
            STORE_BYTE(addr, val);

            if ((mempager.screen.vram_page==0) && mb.direct_screen_mode)
                DrawGPL(addr&0x1FFF);

            break;

        case 0x6: /* espace système non commutable */
        case 0x7:
        case 0x8:
        case 0x9:
            STORE_BYTE(addr, val);

            if ((mempager.screen.vram_page==1) && mb.direct_screen_mode)
                DrawGPL(addr&0x1FFF);

            break;

        case 0xA: /* espace données */
        case 0xB:
        case 0xC:
        case 0xD:
            STORE_BYTE(addr, val);

            if ((mempager.screen.vram_page==mempager.data.page) && mb.direct_screen_mode)
                DrawGPL(addr&0x1FFF);

            break;

        case 0xE:
        case 0xF:
            if ((0xE7C0<=addr) && (addr<=0xE7FF))
                SetDeviceRegister(addr,val);

            break;
    }
}



/* StoreWord:
 *  Ecrit deux octets en mémoire.
 */
static void StoreWord(int addr, int val)
{
    StoreByte(addr, (val>>8));
    StoreByte(addr+1, val&0xFF);
}



/* LoadByte:
 *  Lit un octet en mémoire.
 */
static int LoadByte(int addr)
{
    int index;

    if ((0xE7C0<=addr) && (addr<=0xE7FF))
        switch (addr)
        {
            /* PIA 6846 système */
            case 0xE7C0:
            case 0xE7C4:
                return mc6846.csr;

            case 0xE7C1:
                return mc6846.crc;

            case 0xE7C2:
                return mc6846.ddrc;

            case 0xE7C3:
                return mc6846.prc;

            case 0xE7C5:
                return mc6846_tcr(&mc6846);

            case 0xE7C6:
                return mc6846_tmsb(&mc6846);

            case 0xE7C7:
                return mc6846_tlsb(&mc6846);

            /* PIA 6821 système */
            case 0xE7C8:
                return mc6821_ReadData(&pia_int.porta);

            case 0xE7C9:
                return mc6821_ReadData(&pia_int.portb);

            case 0xE7CA:
		return mc6821_ReadCommand(&pia_int.porta);

            case 0xE7CB:
		return mc6821_ReadCommand(&pia_int.portb);

            /* PIA 6821 musique et jeux */
            case 0xE7CC:
                return mc6821_ReadData(&pia_ext.porta);

            case 0xE7CD:
                return mc6821_ReadData(&pia_ext.portb);

            case 0xE7CE:
		return mc6821_ReadCommand(&pia_ext.porta);

            case 0xE7CF:
		return mc6821_ReadCommand(&pia_ext.portb);

            /* Gate Array lecteur de disquettes */
            case 0xE7D0:
                return disk_ctrl_stat0();

            case 0xE7D1:
                return disk_ctrl_stat1();

            case 0xE7D2:
                return 0;

            case 0xE7D3:
                return disk_ctrl_rdata();

            case 0xE7D4:
            case 0xE7D5:
            case 0xE7D6:
            case 0xE7D7:
            case 0xE7D8:
                return 0;

            /* Gate Array mode page */
            case 0xE7DA:
                index = (mode_page.p_addr>>1);

                if (mode_page.p_addr&1)
                    mode_page.p_data = pal_chip.color[index].b;
                else
                    mode_page.p_data = pal_chip.color[index].gr;

                mode_page.p_addr = (mode_page.p_addr+1)&0x1F;
                return mode_page.p_data;

            case 0xE7DB:
                return mode_page.p_addr;

            case 0xE7DC:
            case 0xE7DD:
            case 0xE7DE:
            case 0xE7DF:
            case 0xE7E0:
            case 0xE7E1:
            case 0xE7E2:
            case 0xE7E3: 
                return 0xCC;

            case 0xE7E4:
                return (mode_page.commut&1 ? mode_page.lp1 : mode_page.system2&0xC0);

            case 0xE7E5:
                /* Retourne le numéro de banque courante même en mode PIA */
                return (mode_page.commut&1 ? mode_page.lp2 : mempager.data.page&0x1F);

            case 0xE7E6:
                return (mode_page.commut&1 ? mode_page.lp3 : mode_page.cart);

            case 0xE7E7:
                return (/*mode_page.lp4*/ GetRealValue_lp4()&0xFE) | (mode_page.commut&1);
        }

    return LOAD_BYTE(addr);
}



/* LoadWord:
 *  Lit deux octets en mémoire.
 */
static int LoadWord(int addr)
{
    if ((0xE7C0-1<=addr) && (addr<=0xE7FF))
        return (LoadByte(addr)<<8)+LoadByte(addr+1);
    else
        return LOAD_WORD(addr);
}



/* FetchInstr:
 *  Remplit le buffer de fetch du CPU.
 */
static void FetchInstr(int addr, unsigned char fetch_buffer[])
{
    register int i;

    for (i=0; i<MC6809_FETCH_BUFFER_SIZE; i++)
        fetch_buffer[i]=LOAD_BYTE(addr+i);
}



/* BiosCall:
 *  Appel aux routines du BIOS de gestion des périphériques.
 */
static int BiosCall(struct MC6809_REGS *regs)
{
    time_t x;
    struct tm *t;

//    printf ("%x ", regs->pc); fflush (stdout);

    switch (regs->pc)
    {
        case 0x25D4:  /* routine d'affichage de la date */
            time(&x);
            t = gmtime(&x);
            STORE_BYTE(0x607C,t->tm_mday);
            STORE_BYTE(0x607D,t->tm_mon+1);
            STORE_BYTE(0x607E,t->tm_year%100);
            return 0x10; /* LDY immédiat */

        case 0x315A:  /* routine de sélection souris/crayon optique */
            to8_SetPointer( LOAD_BYTE(0x6074)&0x80 ? TO8_MOUSE: TO8_LIGHTPEN);
            ResetMouse();
            return 0x8E; /* LDX immédiat */

        case 0x357A:  /* page de modification palette: fin */
            mode_page.lgamod=TO8_COL40;
            return 0x7D; /* TST étendu */

        case 0x3686:  /* page de modification palette: début */
            mode_page.lgamod=TO8_PALETTE;
            return 0x86; /* LDA immédiat */

        case 0x337E:  /* routine GETL crayon optique   */
        case 0x3F97:  /* routine GETL crayon optique 2 */
	    GetLightpen(&regs->xr, &regs->yr, &regs->cc);
            break;

        case 0xFA5A:  /* routine CASS */
            DoK7Stuff(&regs->br,&regs->cc);
            break;

 	/* Contrôleur de disquettes */
        case 0xE0FF:
            ResetDiskCtrl(&regs->cc);
            break;

        case 0xE188:
            WriteSector(&regs->cc);
            break;

        case 0xE3A8:
            ReadSector(&regs->cc);
            break;

        case 0xE4C9:
            FormatDrive(&regs->cc);
            break;

        case 0xE135:        
        case 0xE45B:
        case 0xE47B:
            DiskNop(&regs->cc);
            break;

    } /* end of switch */
    return 0x12;  /* NOP */
}



/* InitHardware:
 *  Initialise la carte mémoire de l'émulateur.
 */
void InitHardware(void)
{
    register int i;

    struct MC6809_INTERFACE interface={ FetchInstr,
                                        LoadByte,
                                        LoadWord,
                                        StoreByte,
                                        StoreWord,
                                        BiosCall };
    mc6809_Init(&interface);

    /* circuit de palette vidéo */
    pal_chip.update = update_color;

    /* définition de la carte mémoire physique */
    /* pas de cartouche */
    mem.cart.nbank = 0;
    mem.cart.size = 0x4000;

    /* 64ko de ROM */
    mem.rom.nbank = 4;
    mem.rom.size = 0x4000;
    for (i=0; i<mem.rom.nbank; i++)
        mem.rom.bank[i] = NULL;

#ifdef DEBIAN_BUILD
    strcpy(mem.rom.filename[0], "/usr/share/teo/b512_b0.rom");
    strcpy(mem.rom.filename[1], "/usr/share/teo/b512_b1.rom");
    strcpy(mem.rom.filename[2], "/usr/share/teo/basic1.rom");
    strcpy(mem.rom.filename[3], "/usr/share/teo/fichier.rom");
#else
    strcpy(mem.rom.filename[0], "b512_b0.rom");
    strcpy(mem.rom.filename[1], "b512_b1.rom");
    strcpy(mem.rom.filename[2], "basic1.rom");
    strcpy(mem.rom.filename[3], "fichier.rom");
#endif

    /* 512ko de RAM */
    mem.ram.nbank = 32;            
    mem.ram.size = 0x4000;
    for (i=0; i<mem.ram.nbank; i++)
        mem.ram.bank[i] = NULL;

    /* 16ko de ROM moniteur */
    mem.mon.nbank = 2;
    mem.mon.size = 0x2000;
    for (i=0; i<mem.mon.nbank; i++)
        mem.mon.bank[i] = NULL;

#ifdef DEBIAN_BUILD
    strcpy(mem.mon.filename[0], "/usr/share/teo/to8mon1.rom");
    strcpy(mem.mon.filename[1], "/usr/share/teo/to8mon2.rom");
#else
    strcpy(mem.mon.filename[0], "to8mon1.rom");
    strcpy(mem.mon.filename[1], "to8mon2.rom");
#endif

    /* définition de la carte mémoire logique */
    mempager.cart.update = update_cart;
    mempager.screen.update = update_screen;
    mempager.system.update = update_system;
    mempager.data.update = update_data;
    mempager.mon.update = update_mon;

    /* carte mère */
    mb.exact_clock = 0;
    mb.direct_screen_mode = TRUE;

    LOCK_VARIABLE(mc6809_irq);
    LOCK_VARIABLE(mc6846);
    LOCK_VARIABLE(pia_int);
    LOCK_VARIABLE(pia_ext);
    LOCK_VARIABLE(mode_page);
    LOCK_VARIABLE(pal_chip);
    LOCK_VARIABLE(mempager);
    LOCK_VARIABLE(mem);
    LOCK_VARIABLE(screen_clock);
}

