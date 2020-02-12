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
 *  Module     : image.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 30/11/2000
 *  Modifié par: François Mouret 26/01/2010 05/10/2012 02/11/2012
 *                               20/09/2013 31/07/2016
 *
 *  Gestion des fichier-images de l'état de l'émulateur.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
#endif


#include "defs.h"
#include "teo.h"
#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "mc68xx/mc6821.h"
#include "media/disk.h"
#include "errors.h"
#include "std.h"
#include "hardware.h"


/***************************************************************************************************
 *
 * Teo image file format specification (v2.0):
 *
 *   endian   file offset(bytes)   field name    field size(bytes)           contents
 *    big             0             format_id          22              "TEO IMAGE FILE FORMAT " string
 *    big            22             format_ver          2                  version number 0x100
 *    big            24             thomson_id          2                 computer numeric id
 *    big            26                                 6                    reserved
 *
 *    big            32             chunk_id            2                 chunk numeric id
 *    big            34             chunk_size          2                 chunck size(bytes)
 *    big            36             chunk_data       xxxx                 chunk data
 *
 *    big          xxxx             chunk_id            2                 chunk numeric id
 *    big          xxxx             chunk_size          2                 chunck size(bytes)
 *    big          xxxx             chunk_data       xxxx                 chunk data
 *
 */

#define FORMAT_ID_SIZE  22
static FILE *file;
static const char format_id[FORMAT_ID_SIZE] = "TEO IMAGE FILE FORMAT ";
static const int format_ver = 0x201;
static int bank_range;

/* Computer numeric id table :
 */
struct THOMSON_ID {
    char name[8];
    int id;
};

enum {
    THOMSON_T9000,
    THOMSON_TO7,
    THOMSON_TO770,
    THOMSON_MO5,
    THOMSON_MO5NR,
    THOMSON_MO6,
    THOMSON_TO8,
    THOMSON_TO8D,
    THOMSON_TO9,
    THOMSON_TO9PLUS,
    THOMSON_MAX
}; 

static const struct THOMSON_ID thomson[THOMSON_MAX] = {
/*  Thomson model      id    */
    { "T9000",       0x100 },
    { "TO7",         0x110 },
    { "TO7-70",      0x120 },
    { "MO5",         0x200 },
    { "MO5NR",       0x201 },
    { "MO6",         0x210 },
    { "TO8",         0x300 },
    { "TO8D",        0x301 },
    { "TO9",         0x400 },
    { "TO9+",        0x410 }
};


/* Chunk numeric id table:
 *  Warning!!! id must be lesser than 0x2000 
 */
enum {
    HW_MC6809,
    HW_MC6846_SYS,
    HW_MC6821_SYS,
    HW_MC6821_GAME,
    HW_MODE_PAGE,
    HW_PAL_CHIP,
    HW_DISK_CTRL,
    HW_MEMORY_PAGER,
    HW_MEMORY_BANK,
    HW_MOTHERBOARD,
    HW_MAX
};

static const int hardware_id[HW_MAX] = {
/*  Hardware component          id  */
/*   MC6809E              */   0x100,
/*   MC6846 system        */   0x200,
/*   MC6821 system        */   0x300,
/*   MC6821 game          */   0x310,
/*   Gate Array mode page */   0x400,
/*   Palette chip         */   0x500,
/*   Disk controller      */   0x600,
/*   Memory Pager         */   0x700,
/*   Memory Bank          */   0x800,
/*   Motherboard          */   0x900
};  

/*
 * End of Teo image file format specification (v2.0)
 *
 ***************************************************************************************************/



/* fread_int8:
 *  Helper pour lire en big endian un entier 8-bit
 *  quel que soit son format natif.
 */
static void fread_int8(int *value)
{
    unsigned char buffer[1];

    if (fread(buffer, 1, 1, file) != 1)
        return;

    *value = (int) buffer[0];
}



/* fread_int16:
 *  Helper pour lire en big endian un entier 16-bit
 *  quel que soit son format natif.
 */
static void fread_int16(int *value)
{
    unsigned char buffer[2];

    if (fread(buffer, 1, 2, file) != 2)
        return;

    *value = (int) (buffer[0]<<8)+buffer[1];
}


#if 0

/* fread_int32:
 *  Helper pour lire en big endian un entier 32-bit
 *  quel que soit son format natif.
 */
static void fread_int32(int *value)
{
    unsigned char buffer[4];

    if (fread(buffer, 1, 4, file) != 4)
        return;

    *value = (buffer[0]<<24)+(buffer[1]<<16)+(buffer[2]<<8)+buffer[3];
}

#endif


/* fread_int64:
 *  Helper pour lire en big endian un entier 64-bit
 *  quel que soit son format natif.
 */
static void fread_int64(unsigned long long int *value)
{
    unsigned char buffer[8];
    unsigned long long int high_dword, low_dword; 

    if (fread(buffer, 1, 8, file) != 8)
        return;

    high_dword = (buffer[0]<<24)+(buffer[1]<<16)+(buffer[2]<<8)+buffer[3];
    low_dword  = (buffer[4]<<24)+(buffer[5]<<16)+(buffer[6]<<8)+buffer[7];

    *value = (high_dword<<32) + low_dword;
}



/* fwrite_int8:
 *  Helper pour écrire en big endian un entier 8-bit
 *  quel que soit son format natif.
 */
static void fwrite_int8(int val)
{
    unsigned char buffer[1];

    buffer[0] = (unsigned char) val;

    fwrite(buffer, 1, 1, file);
}



/* fwrite_int16:
 *  Helper pour écrire en big endian un entier 16-bit
 *  quel que soit son format natif.
 */
static void fwrite_int16(int val)
{
    unsigned char buffer[2];

    buffer[0] = (unsigned char) (val>>8);
    buffer[1] = (unsigned char) val;

    fwrite(buffer, 1, 2, file);
}


#if 0

/* fwrite_int32:
 *  Helper pour écrire en big endian un entier 32-bit
 *  quel que soit son format natif.
 */
static void fwrite_int32(int val)
{
    unsigned char buffer[4];

    buffer[0] = (unsigned char) (val>>24);
    buffer[1] = (unsigned char) (val>>16);
    buffer[2] = (unsigned char) (val>>8);
    buffer[3] = (unsigned char) val;

    fwrite(buffer, 1, 4, file);
}

#endif


/* fwrite_int64:
 *  Helper pour écrire en big endian un entier 64-bit
 *  quel que soit son format natif.
 */
static void fwrite_int64(unsigned long long int val)
{
    unsigned char buffer[8];

    buffer[0] = (unsigned char) (val>>56);
    buffer[1] = (unsigned char) (val>>48);
    buffer[2] = (unsigned char) (val>>40);
    buffer[3] = (unsigned char) (val>>32);
    buffer[4] = (unsigned char) (val>>24);
    buffer[5] = (unsigned char) (val>>16);
    buffer[6] = (unsigned char) (val>>8);
    buffer[7] = (unsigned char) val;

    fwrite(buffer, 1, 8, file);
}



/* Loader:
 *  Charge et modifie l'état du composant matériel.
 */ 
static void null_Loader(int chunk_id, int chunk_size)
{
    /* Oups! l'identifiant matériel est inconnu, il a dû être ajouté
       dans une version ultérieure du format; on saute les données */
    fseek(file, chunk_size, SEEK_CUR);

    (void) chunk_id;
}


#define FILL_GAP(from, to)               \
    if (from < to)                       \
        fseek(file, to-from, SEEK_CUR)


static void mc6809_Loader(int chunk_id, int chunk_size)
{
    struct MC6809_REGS regs;

    fread_int8  (&regs.cc);
    fread_int8  (&regs.dp);
    fread_int8  (&regs.ar);
    fread_int8  (&regs.br);
    fread_int16 (&regs.xr);
    fread_int16 (&regs.yr);
    fread_int16 (&regs.ur);
    fread_int16 (&regs.sr);
    fread_int16 (&regs.pc);
    fread_int64 (&regs.cpu_clock);
    fread_int64 (&regs.cpu_timer);
    fread_int8  (&mc6809_irq);
    FILL_GAP    (31, chunk_size);

    mc6809_SetRegs(&regs, (1<<MC6809_REGS_MAX_FLAG)-1);
    (void) chunk_id;
}


static void mc6846_Loader(int chunk_id, int chunk_size)
{
    int tmp;

    fread_int8  (&mc6846.csr);
    fread_int8  (&mc6846.crc);
    fread_int8  (&mc6846.ddrc);
    fread_int8  (&mc6846.prc);

    /* backward compatibility */
    /* v1.0: fread_int8(&mc6846.w_mask, file); */
    fread_int8  (&tmp);

    fread_int8  (&mc6846.tcr);
    fread_int8  (&mc6846.tmsb);
    fread_int8  (&mc6846.tlsb);
    fread_int8  (&mc6846.timer_ratio);
    fread_int64 (&mc6846.timeout);
    FILL_GAP    (17, chunk_size);
    (void) chunk_id;
}


static void mc6821_Loader(int chunk_id, int chunk_size)
{
    struct MC6821_PIA *pia;
    int tmp;

    if (chunk_id == hardware_id[HW_MC6821_SYS])
        pia = &pia_int;
    else if (chunk_id == hardware_id[HW_MC6821_GAME])
        pia = &pia_ext;
    else
    {
        null_Loader(chunk_id, chunk_size);
        return;
    }

    fread_int8  (&pia->porta.cr);
    fread_int8  (&pia->porta.ddr);
    fread_int8  (&pia->porta.odr);

    /* backward compatibility */
    /* v1.0: fread_int8(&pia->porta.pdr, file);    */
    /*       fread_int8(&pia->porta.w_mask, file); */
    fread_int8  (&tmp);
    fread_int8  (&tmp);

    fread_int8  (&pia->portb.cr);
    fread_int8  (&pia->portb.ddr);
    fread_int8  (&pia->portb.odr);

    /* backward compatibility */
    /* v1.0: fread_int8(&pia->portb.pdr, file);    */
    /*       fread_int8(&pia->portb.w_mask, file); */
    fread_int8  (&tmp);
    fread_int8  (&tmp);

    FILL_GAP    (10, chunk_size);
}


static void modepage_Loader(int chunk_id, int chunk_size)
{
    fread_int8 (&mode_page.p_data);
    fread_int8 (&mode_page.p_addr);
    fread_int8 (&mode_page.lgamod);
    fread_int8 (&mode_page.system1);
    fread_int8 (&mode_page.system2);
    fread_int8 (&mode_page.commut);
    fread_int8 (&mode_page.ram_data);
    fread_int8 (&mode_page.cart);
    fread_int8 (&mode_page.lp1);
    fread_int8 (&mode_page.lp2);
    fread_int8 (&mode_page.lp3);
    fread_int8 (&mode_page.lp4);
    FILL_GAP   (12, chunk_size);

    if (teo_SetBorderColor)
        teo_SetBorderColor(mode_page.lgamod, mode_page.system2&0xF);

    (void) chunk_id;
}


static void palchip_Loader(int chunk_id, int chunk_size)
{
    int i;

    for (i=0; i<16; i++)
    {
        fread_int8 (&pal_chip.color[i].gr);
        fread_int8 (&pal_chip.color[i].b);
        pal_chip.update(i);
    }

    FILL_GAP(32, chunk_size);
    (void) chunk_id;
}


static void diskctrl_Loader(int chunk_id, int chunk_size)
{
    int drive;

    fread_int16  (&dkcurr);

    for (drive=0; drive<NBDRIVE; drive++)
    {
        /* controller registers */
        if ((drive&3) == 0)
        {
            fread_int8  (&disk[drive].dkc->rr0);
            fread_int8  (&disk[drive].dkc->rr1);
            fread_int8  (&disk[drive].dkc->rr2);
            fread_int8  (&disk[drive].dkc->rr3);
            fread_int8  (&disk[drive].dkc->rr4);
            fread_int8  (&disk[drive].dkc->rr5);
            fread_int8  (&disk[drive].dkc->rr6);
            fread_int8  (&disk[drive].dkc->rr7);
            fread_int8  (&disk[drive].dkc->rr8);
            fread_int8  (&disk[drive].dkc->rr9);
            fread_int8  (&disk[drive].dkc->wr0);
            fread_int8  (&disk[drive].dkc->wr1);
            fread_int8  (&disk[drive].dkc->wr2);
            fread_int8  (&disk[drive].dkc->wr3);
            fread_int8  (&disk[drive].dkc->wr4);
            fread_int8  (&disk[drive].dkc->wr5);
            fread_int8  (&disk[drive].dkc->wr6);
            fread_int8  (&disk[drive].dkc->wr7);
            fread_int8  (&disk[drive].dkc->wr8);
            fread_int8  (&disk[drive].dkc->wr9);
            fread_int8  (&disk[drive].dkc->crc);
            fread_int8  (&disk[drive].dkc->write_door);
            fread_int8  (&disk[drive].dkc->process);
            fread_int8  (&disk[drive].dkc->process_cpt);
            fread_int16 (&disk[drive].dkc->auto_count);
        }

        /* drive registers */
        if ((drive&1) == 0)
        {
            fread_int16 (&disk[drive].drv->sector);
            fread_int16 (&disk[drive].drv->track.curr);
            fread_int16 (&disk[drive].drv->track.last);
            fread_int16 (&disk[drive].drv->pos.curr);
            fread_int16 (&disk[drive].drv->pos.last);
            fread_int64 (&disk[drive].drv->motor_start);
            fread_int64 (&disk[drive].drv->motor_stop);
        }
    }
    disk_FillAllTracks ();
    FILL_GAP    (128, chunk_size);
    (void) chunk_id;
}


static void mempager_Loader(int chunk_id, int chunk_size)
{
    fread_int8  (&mempager.cart.page);
    fread_int8  (&mempager.cart.rom_page);
    fread_int8  (&mempager.cart.ram_page);
    mempager.cart.update();

    fread_int8  (&mempager.screen.page);
    fread_int8  (&mempager.screen.vram_page);
    mempager.screen.update();

    fread_int8  (&mempager.system.page);
    mempager.system.update();

    fread_int8  (&mempager.data.page);
    fread_int8  (&mempager.data.reg_page);
    fread_int8  (&mempager.data.pia_page);
    mempager.data.update();

    fread_int8  (&mempager.mon.page);
    mempager.mon.update();

    FILL_GAP    (10, chunk_size);
    (void) chunk_id;
}


static void membank_Loader(int chunk_id, int chunk_size)
{
    int bank = chunk_id&0xFF;
    int begin, size=chunk_size-2;

    fread_int16 (&begin);

    if (fread(mem.ram.bank[bank]+begin, 1, size, file) != (size_t)size)
        return;
    bank_range++;
}


static void mb_Loader(int chunk_id, int chunk_size)
{
    fread_int64 (&mb.exact_clock);
    fread_int8  (&mb.direct_screen_mode);
    FILL_GAP    (9, chunk_size);
    (void) chunk_id;
}


static void (*Loader[32])(int, int) = {
    null_Loader,     mc6809_Loader,   mc6846_Loader,   mc6821_Loader,
    modepage_Loader, palchip_Loader,  diskctrl_Loader, mempager_Loader,
    membank_Loader,  mb_Loader,       null_Loader,     null_Loader,
    null_Loader,     null_Loader,     null_Loader,     null_Loader,
    null_Loader,     null_Loader,     null_Loader,     null_Loader,
    null_Loader,     null_Loader,     null_Loader,     null_Loader,
    null_Loader,     null_Loader,     null_Loader,     null_Loader,
    null_Loader,     null_Loader,     null_Loader,     null_Loader
};



/* Saver:
 *  Sauvegarde l'état du composant matériel.
 */
static void mc6809_Saver(int chunk_id)
{
    struct MC6809_REGS regs;
    mc6809_GetRegs(&regs);
    fwrite_int16 (31);
    fwrite_int8  (regs.cc);
    fwrite_int8  (regs.dp);
    fwrite_int8  (regs.ar);
    fwrite_int8  (regs.br);
    fwrite_int16 (regs.xr);
    fwrite_int16 (regs.yr);
    fwrite_int16 (regs.ur);
    fwrite_int16 (regs.sr);
    fwrite_int16 (regs.pc);
    fwrite_int64 (regs.cpu_clock);
    fwrite_int64 (regs.cpu_timer);
    fwrite_int8  (mc6809_irq);
    (void) chunk_id;
}


static void mc6846_Saver(int chunk_id)
{
    fwrite_int16 (17);
    fwrite_int8  (mc6846.csr);
    fwrite_int8  (mc6846.crc);
    fwrite_int8  (mc6846.ddrc);
    fwrite_int8  (mc6846.prc);

    /* backward compatibility */
    /* v1.0: fwrite_int8(mc6846.w_mask, file); */
    fwrite_int8  (0x3D);

    fwrite_int8  (mc6846.tcr);
    fwrite_int8  (mc6846.tmsb);
    fwrite_int8  (mc6846.tlsb);
    fwrite_int8  (mc6846.timer_ratio);
    fwrite_int64 (mc6846.timeout);
    (void) chunk_id;
}


static void mc6821_Saver(int chunk_id)
{
    struct MC6821_PIA *pia;
    int porta_w_mask, portb_w_mask, mask;

    if (chunk_id == hardware_id[HW_MC6821_SYS])
    {
        pia = &pia_int;
        /* backward compatibility */
        porta_w_mask = 0xFE;
        portb_w_mask = 0xFF;
    }
    else
    {
        pia = &pia_ext;
        /* backward compatibility */
        porta_w_mask = 0x0;
        portb_w_mask = 0x3F;
    }

    fwrite_int16 (10);
    fwrite_int8  (pia->porta.cr);
    fwrite_int8  (pia->porta.ddr);
    fwrite_int8  (pia->porta.odr);

    /* backward compatibility */
    /* v1.0: fwrite_int8(pia->porta.pdr, file);    */
    /*       fwrite_int8(pia->porta.w_mask, file); */
    mask = pia->porta.ddr & porta_w_mask;
    fwrite_int8  ((pia->porta.idr&(mask^0xFF)) | (pia->porta.odr&mask));
    fwrite_int8  (porta_w_mask);

    fwrite_int8  (pia->portb.cr);
    fwrite_int8  (pia->portb.ddr);
    fwrite_int8  (pia->portb.odr);

    /* backward compatibility */
    /* v1.0: fwrite_int8(pia->portb.pdr, file);    */
    /*       fwrite_int8(pia->portb.w_mask, file); */
    mask = pia->portb.ddr & portb_w_mask;
    fwrite_int8  ((pia->portb.idr&(mask^0xFF)) | (pia->portb.odr&mask));
    fwrite_int8  (portb_w_mask);
}


static void modepage_Saver(int chunk_id)
{
    fwrite_int16 (12);
    fwrite_int8  (mode_page.p_data);
    fwrite_int8  (mode_page.p_addr);
    fwrite_int8  (mode_page.lgamod);
    fwrite_int8  (mode_page.system1);
    fwrite_int8  (mode_page.system2);
    fwrite_int8  (mode_page.commut);
    fwrite_int8  (mode_page.ram_data);
    fwrite_int8  (mode_page.cart);
    fwrite_int8  (mode_page.lp1);
    fwrite_int8  (mode_page.lp2);
    fwrite_int8  (mode_page.lp3);
    fwrite_int8  (mode_page.lp4);
    (void) chunk_id;
}


static void palchip_Saver(int chunk_id)
{
    int i;

    fwrite_int16 (32);

    for (i=0; i<16; i++)
    {
        fwrite_int8  (pal_chip.color[i].gr);
        fwrite_int8  (pal_chip.color[i].b);
    }

    (void) chunk_id;
}


static void diskctrl_Saver(int chunk_id)
{
    int drive;

    fwrite_int16 (128);

    fwrite_int16  (dkcurr);

    for (drive=0; drive<NBDRIVE; drive++)
    {
        /* controller registers */
        if ((drive&3) == 0)
        {
            fwrite_int8  (disk[drive].dkc->rr0);
            fwrite_int8  (disk[drive].dkc->rr1);
            fwrite_int8  (disk[drive].dkc->rr2);
            fwrite_int8  (disk[drive].dkc->rr3);
            fwrite_int8  (disk[drive].dkc->rr4);
            fwrite_int8  (disk[drive].dkc->rr5);
            fwrite_int8  (disk[drive].dkc->rr6);
            fwrite_int8  (disk[drive].dkc->rr7);
            fwrite_int8  (disk[drive].dkc->rr8);
            fwrite_int8  (disk[drive].dkc->rr9);
            fwrite_int8  (disk[drive].dkc->wr0);
            fwrite_int8  (disk[drive].dkc->wr1);
            fwrite_int8  (disk[drive].dkc->wr2);
            fwrite_int8  (disk[drive].dkc->wr3);
            fwrite_int8  (disk[drive].dkc->wr4);
            fwrite_int8  (disk[drive].dkc->wr5);
            fwrite_int8  (disk[drive].dkc->wr6);
            fwrite_int8  (disk[drive].dkc->wr7);
            fwrite_int8  (disk[drive].dkc->wr8);
            fwrite_int8  (disk[drive].dkc->wr9);
            fwrite_int8  (disk[drive].dkc->crc);
            fwrite_int8  (disk[drive].dkc->write_door);
            fwrite_int8  (disk[drive].dkc->process);
            fwrite_int8  (disk[drive].dkc->process_cpt);
            fwrite_int16 (disk[drive].dkc->auto_count);
        }

        /* drive registers */
        if ((drive&1) == 0)
        {
            fwrite_int16 (disk[drive].drv->sector);
            fwrite_int16 (disk[drive].drv->track.curr);
            fwrite_int16 (disk[drive].drv->track.last);
            fwrite_int16 (disk[drive].drv->pos.curr);
            fwrite_int16 (disk[drive].drv->pos.last);
            fwrite_int64 (disk[drive].drv->motor_start);
            fwrite_int64 (disk[drive].drv->motor_stop);
        }
    }
    (void) chunk_id;
}


static void mempager_Saver(int chunk_id)
{
    fwrite_int16 (10);
    fwrite_int8  (mempager.cart.page);
    fwrite_int8  (mempager.cart.rom_page);
    fwrite_int8  (mempager.cart.ram_page);
    fwrite_int8  (mempager.screen.page);
    fwrite_int8  (mempager.screen.vram_page);
    fwrite_int8  (mempager.system.page);
    fwrite_int8  (mempager.data.page);
    fwrite_int8  (mempager.data.reg_page);
    fwrite_int8  (mempager.data.pia_page);
    fwrite_int8  (mempager.mon.page);
    (void) chunk_id;
}



static uint8 mbto8d (int offset)
{
    return (uint8)((((offset+0x80)&0x180) < 0x100) ? 0x00 : 0xff);
}



static void membank_Saver(int chunk_id)
{
    int bank = chunk_id&0xFF;
    int begin = 0, end = mem.ram.size, size;

    while ((begin < end) && (mem.ram.bank[bank][begin] == mbto8d(begin)))
        begin++;

    while ((end > begin) && (mem.ram.bank[bank][end-1] == mbto8d(end)))
        end--;

    size = end-begin;

    fwrite_int16 (2+size);
    fwrite_int16 (begin);
    fwrite(mem.ram.bank[bank]+begin, 1, size, file);
}    


static void mb_Saver(int chunk_id)
{
    fwrite_int16 (9);
    fwrite_int64 (mb.exact_clock);
    fwrite_int8  (mb.direct_screen_mode);
    (void) chunk_id;
}


static void (*Saver[32])(int) = {
    NULL,           mc6809_Saver,   mc6846_Saver,   mc6821_Saver,
    modepage_Saver, palchip_Saver,  diskctrl_Saver, mempager_Saver,
    membank_Saver,  mb_Saver,       NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
};


/* TODO: Have a std_FileOpen that merges this and ini.c:file_open
 * and handles the various search paths
 *
 * */
static FILE *file_open (const char filename[], const char mode[])
{
    char *name = NULL;
    char *data_dir = NULL;

    /*TODO: Check root path*/
    if(!std_IsAbsolutePath(filename)){
        data_dir = std_getUserDataDir();
        if(data_dir){
            name = std_PathAppend(data_dir, filename);
            std_Debug("%s: Datadir found, using %s as image file.\n", __FUNCTION__, name);
        }else{
            std_Debug("%s: Datadir NOT FOUND (this shouldn't happen). Falling back to current directory for %s\n", __FUNCTION__, name);
        }
    }

    /* Use the filename as-is if filaname is a root path or std_getUserDataDir() fails*/
    file = fopen(name ? name : filename, mode);
    if(name)
        std_free (name);
    std_free(data_dir);
    return file;
}


/* ------------------------------------------------------------------------- */


/* image_Load:
 *  Charge une image et modifie l'état de l'émulateur.
 */
int image_Load(const char filename[])
{
    char buffer[FORMAT_ID_SIZE];
    int value, chunk_id, chunk_size;

    /* open the image file */
    if (file_open(filename, "rb") == NULL)
        return error_Message(TEO_ERROR_FILE_OPEN, NULL);

    /* lecture de l'entête */
    if (fread(buffer, 1, FORMAT_ID_SIZE, file) != FORMAT_ID_SIZE)  /* format_id */
    {
        fclose(file);
        return error_Message(TEO_ERROR_FILE_OPEN, NULL);
    }
    
    if (strncmp(buffer, format_id, FORMAT_ID_SIZE))
    {
        fclose(file);
        return error_Message(TEO_ERROR_FILE_FORMAT, NULL);
    }

    /* check format version */
    fread_int16 (&value);
    if (value != format_ver)
    {
        fclose(file);
        return error_Message(TEO_ERROR_UNSUPPORTED_MODEL, NULL);
    }

    /* check thomson id */
    fread_int16 (&value);
    if ((value != thomson[THOMSON_TO8].id) && (value != thomson[THOMSON_TO8D].id))
    {
        fclose(file);
        return error_Message(TEO_ERROR_UNSUPPORTED_MODEL, NULL);
    }

    fseek(file, 6, SEEK_CUR);  /* reserved */

    bank_range = 0;

    while (TRUE)
    {
        fread_int16 (&chunk_id);

        if (feof(file))
            break;

        fread_int16 (&chunk_size);

        if (chunk_id >= 0x2000)
        {
            fclose(file);
            return error_Message(TEO_ERROR_FILE_FORMAT, NULL);
        }

        Loader[chunk_id>>8](chunk_id, chunk_size);
    } 

    fclose(file);

    if ((bank_range != 16) && (bank_range != 32))
        return error_Message(TEO_ERROR_FILE_FORMAT, NULL);
    teo.setting.bank_range = bank_range;

    teo_new_video_params = TRUE;

    return 0;
}



/* Image_Save:
 *  Sauvegarde une image de l'état de l'émulateur.
 */
int image_Save(const char filename[])
{
    int chunk_id, i;

    /* open the image file */
    if (file_open(filename, "wb") == NULL)
        return error_Message(TEO_ERROR_FILE_OPEN, NULL);

    /* écriture de l'entête */
    fwrite(format_id, 1, FORMAT_ID_SIZE, file); 
    fwrite_int16 (format_ver);
    fwrite_int16 (thomson[THOMSON_TO8D].id);

    fwrite_int16 (0); /* reserved */
    fwrite_int16 (0); /* reserved */
    fwrite_int16 (0); /* reserved */

    /* sauvegarde de l'état des composants matériels */
    for (i=0; i<HW_MAX; i++)
        if (i != HW_MEMORY_BANK)
        {
            chunk_id = hardware_id[i];
            fwrite_int16 (chunk_id);
            Saver[chunk_id>>8](chunk_id);
        }

    /* sauvegarde des banques mémoire */
    chunk_id = hardware_id[HW_MEMORY_BANK];

    for (i=0; i<teo.setting.bank_range; i++)
    {
        fwrite_int16(chunk_id+i);
        Saver[chunk_id>>8](chunk_id+i);
    }

    fclose(file);
    return 0;
}

