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
 *  Module     : image.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 30/11/2000
 *  Modifié par: François Mouret 26/01/2010 05/10/2012 02/11/2012
 *
 *  Gestion des fichier-images de l'état de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
#endif

#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "mc68xx/mc6821.h"
#include "media/disk.h"
#include "error.h"
#include "hardware.h"


/***************************************************************************************************
 *
 * Teo image file format specification (v1.1):
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
static const char format_id[FORMAT_ID_SIZE] = "TEO IMAGE FILE FORMAT ";
static const int format_ver = 0x100;


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
 * End of Teo image file format specification (v1.1)
 *
 ***************************************************************************************************/



/* fread_int8:
 *  Helper pour lire en big endian un entier 8-bit
 *  quel que soit son format natif.
 */
static void fread_int8(int *value, FILE *file)
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
static void fread_int16(int *value, FILE *file)
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
static void fread_int32(int *value, FILE *file)
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
static void fread_int64(unsigned long long int *value, FILE *file)
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
static void fwrite_int8(int val, FILE *file)
{
    unsigned char buffer[1];

    buffer[0] = (unsigned char) val;

    fwrite(buffer, 1, 1, file);
}



/* fwrite_int16:
 *  Helper pour écrire en big endian un entier 16-bit
 *  quel que soit son format natif.
 */
static void fwrite_int16(int val, FILE *file)
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
static void fwrite_int32(int val, FILE *file)
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
static void fwrite_int64(unsigned long long int val, FILE *file)
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
static void null_Loader(int chunk_id, int chunk_size, FILE *file)
{
    /* Oups! l'identifiant matériel est inconnu, il a dû être ajouté
       dans une version ultérieure du format; on saute les données */
    fseek(file, chunk_size, SEEK_CUR);

    (void) chunk_id;
}


#define FILL_GAP(from, to)               \
    if (from < to)                       \
        fseek(file, to-from, SEEK_CUR)


static void mc6809_Loader(int chunk_id, int chunk_size, FILE *file)
{
    struct MC6809_REGS regs;

    fread_int8(&regs.cc, file);
    fread_int8(&regs.dp, file);
    fread_int8(&regs.ar, file);
    fread_int8(&regs.br, file);
    fread_int16(&regs.xr, file);
    fread_int16(&regs.yr, file);
    fread_int16(&regs.ur, file);
    fread_int16(&regs.sr, file);
    fread_int16(&regs.pc, file);
    fread_int64(&regs.cpu_clock, file);
    fread_int64(&regs.cpu_timer, file);
    fread_int8(&mc6809_irq, file);
    FILL_GAP(31, chunk_size);

    mc6809_SetRegs(&regs, (1<<MC6809_REGS_MAX_FLAG)-1);
    (void) chunk_id;
}


static void mc6846_Loader(int chunk_id, int chunk_size, FILE *file)
{
    int tmp;

    fread_int8(&mc6846.csr, file);
    fread_int8(&mc6846.crc, file);
    fread_int8(&mc6846.ddrc, file);
    fread_int8(&mc6846.prc, file);

    /* backward compatibility */
    /* v1.0: fread_int8(&mc6846.w_mask, file); */
    fread_int8(&tmp, file);

    fread_int8(&mc6846.tcr, file);
    fread_int8(&mc6846.tmsb, file);
    fread_int8(&mc6846.tlsb, file);
    fread_int8(&mc6846.timer_ratio, file);
    fread_int64(&mc6846.timeout, file);
    FILL_GAP(17, chunk_size);
    (void) chunk_id;
}


static void mc6821_Loader(int chunk_id, int chunk_size, FILE *file)
{
    struct MC6821_PIA *pia;
    int tmp;

    if (chunk_id == hardware_id[HW_MC6821_SYS])
        pia = &pia_int;
    else if (chunk_id == hardware_id[HW_MC6821_GAME])
        pia = &pia_ext;
    else
    {
        null_Loader(chunk_id, chunk_size, file);
        return;
    }

    fread_int8(&pia->porta.cr, file);
    fread_int8(&pia->porta.ddr, file);
    fread_int8(&pia->porta.odr, file);

    /* backward compatibility */
    /* v1.0: fread_int8(&pia->porta.pdr, file);    */
    /*       fread_int8(&pia->porta.w_mask, file); */
    fread_int8(&tmp, file);
    fread_int8(&tmp, file);

    fread_int8(&pia->portb.cr, file);
    fread_int8(&pia->portb.ddr, file);
    fread_int8(&pia->portb.odr, file);

    /* backward compatibility */
    /* v1.0: fread_int8(&pia->portb.pdr, file);    */
    /*       fread_int8(&pia->portb.w_mask, file); */
    fread_int8(&tmp, file);
    fread_int8(&tmp, file);

    FILL_GAP(10, chunk_size);
}


static void modepage_Loader(int chunk_id, int chunk_size, FILE *file)
{
    fread_int8(&mode_page.p_data, file);
    fread_int8(&mode_page.p_addr, file);
    fread_int8(&mode_page.lgamod, file);
    fread_int8(&mode_page.system1, file);
    fread_int8(&mode_page.system2, file);
    fread_int8(&mode_page.commut, file);
    fread_int8(&mode_page.ram_data, file);
    fread_int8(&mode_page.cart, file);
    fread_int8(&mode_page.lp1, file);
    fread_int8(&mode_page.lp2, file);
    fread_int8(&mode_page.lp3, file);
    fread_int8(&mode_page.lp4, file);
    FILL_GAP(12, chunk_size);

    if (to8_SetBorderColor)
        to8_SetBorderColor(mode_page.lgamod, mode_page.system2&0xF);

    (void) chunk_id;
}


static void palchip_Loader(int chunk_id, int chunk_size, FILE *file)
{
    int i;

    for (i=0; i<16; i++)
    {
        fread_int8(&pal_chip.color[i].gr, file);
        fread_int8(&pal_chip.color[i].b, file);
        pal_chip.update(i);
    }

    FILL_GAP(32, chunk_size);
    (void) chunk_id;
}


static void diskctrl_Loader(int chunk_id, int chunk_size, FILE *file)
{
    fread_int8(&disk_ctrl.cmd0, file);
    fread_int8(&disk_ctrl.cmd1, file);
    fread_int8(&disk_ctrl.cmd2, file);
    fread_int8(&disk_ctrl.stat0, file);
    fread_int8(&disk_ctrl.stat1, file);
    fread_int8(&disk_ctrl.wdata, file);
    fread_int8(&disk_ctrl.rdata, file);
    fread_int8(&disk_ctrl.wclk, file);
    fread_int8(&disk_ctrl.wsect, file);
    fread_int8(&disk_ctrl.wtrck, file);
    fread_int8(&disk_ctrl.wcell, file);
    fread_int16(&disk_ctrl.prot, file);
    FILL_GAP(13, chunk_size);
    (void) chunk_id;
}


static void mempager_Loader(int chunk_id, int chunk_size, FILE *file)
{
    fread_int8(&mempager.cart.page, file);
    fread_int8(&mempager.cart.rom_page, file);
    fread_int8(&mempager.cart.ram_page, file);
    mempager.cart.update();

    fread_int8(&mempager.screen.page, file);
    fread_int8(&mempager.screen.vram_page, file);
    mempager.screen.update();

    fread_int8(&mempager.system.page, file);
    mempager.system.update();

    fread_int8(&mempager.data.page, file);
    fread_int8(&mempager.data.reg_page, file);
    fread_int8(&mempager.data.pia_page, file);
    mempager.data.update();

    fread_int8(&mempager.mon.page, file);
    mempager.mon.update();

    FILL_GAP(10, chunk_size);
    (void) chunk_id;
}


static void membank_Loader(int chunk_id, int chunk_size, FILE *file)
{
    int bank = chunk_id&0xFF;
    int begin, end, size=chunk_size-2;

    fread_int16(&begin, file);

    if (begin)
        memset(mem.ram.bank[bank], 0, begin);

    if (fread(mem.ram.bank[bank]+begin, 1, size, file) != (size_t)size)
        return;

    end = begin+size;

    if (end < mem.ram.size)
        memset(mem.ram.bank[bank]+end, 0, mem.ram.size - end);
}


static void mb_Loader(int chunk_id, int chunk_size, FILE *file)
{
    fread_int64(&mb.exact_clock, file);
    fread_int8(&mb.direct_screen_mode, file);
    FILL_GAP(9, chunk_size);
    (void) chunk_id;
}


static void (*Loader[32])(int, int, FILE *) = {
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
static void mc6809_Saver(int chunk_id, FILE *file)
{
    struct MC6809_REGS regs;
    mc6809_GetRegs(&regs);
    fwrite_int16(31, file);
    fwrite_int8(regs.cc, file);
    fwrite_int8(regs.dp, file);
    fwrite_int8(regs.ar, file);
    fwrite_int8(regs.br, file);
    fwrite_int16(regs.xr, file);
    fwrite_int16(regs.yr, file);
    fwrite_int16(regs.ur, file);
    fwrite_int16(regs.sr, file);
    fwrite_int16(regs.pc, file);
    fwrite_int64(regs.cpu_clock, file);
    fwrite_int64(regs.cpu_timer, file);
    fwrite_int8(mc6809_irq, file);
    (void) chunk_id;
}


static void mc6846_Saver(int chunk_id, FILE *file)
{
    fwrite_int16(17, file);
    fwrite_int8(mc6846.csr, file);
    fwrite_int8(mc6846.crc, file);
    fwrite_int8(mc6846.ddrc, file);
    fwrite_int8(mc6846.prc, file);

    /* backward compatibility */
    /* v1.0: fwrite_int8(mc6846.w_mask, file); */
    fwrite_int8(0x3D, file);

    fwrite_int8(mc6846.tcr, file);
    fwrite_int8(mc6846.tmsb, file);
    fwrite_int8(mc6846.tlsb, file);
    fwrite_int8(mc6846.timer_ratio, file);
    fwrite_int64(mc6846.timeout, file);
    (void) chunk_id;
}


static void mc6821_Saver(int chunk_id, FILE *file)
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

    fwrite_int16(10, file);
    fwrite_int8(pia->porta.cr, file);
    fwrite_int8(pia->porta.ddr, file);
    fwrite_int8(pia->porta.odr, file);

    /* backward compatibility */
    /* v1.0: fwrite_int8(pia->porta.pdr, file);    */
    /*       fwrite_int8(pia->porta.w_mask, file); */
    mask = pia->porta.ddr & porta_w_mask;
    fwrite_int8((pia->porta.idr&(mask^0xFF)) | (pia->porta.odr&mask), file);
    fwrite_int8(porta_w_mask, file);

    fwrite_int8(pia->portb.cr, file);
    fwrite_int8(pia->portb.ddr, file);
    fwrite_int8(pia->portb.odr, file);

    /* backward compatibility */
    /* v1.0: fwrite_int8(pia->portb.pdr, file);    */
    /*       fwrite_int8(pia->portb.w_mask, file); */
    mask = pia->portb.ddr & portb_w_mask;
    fwrite_int8((pia->portb.idr&(mask^0xFF)) | (pia->portb.odr&mask), file);
    fwrite_int8(portb_w_mask, file);
}


static void modepage_Saver(int chunk_id, FILE *file)
{
    fwrite_int16(12, file);
    fwrite_int8(mode_page.p_data, file);
    fwrite_int8(mode_page.p_addr, file);
    fwrite_int8(mode_page.lgamod, file);
    fwrite_int8(mode_page.system1, file);
    fwrite_int8(mode_page.system2, file);
    fwrite_int8(mode_page.commut, file);
    fwrite_int8(mode_page.ram_data, file);
    fwrite_int8(mode_page.cart, file);
    fwrite_int8(mode_page.lp1, file);
    fwrite_int8(mode_page.lp2, file);
    fwrite_int8(mode_page.lp3, file);
    fwrite_int8(mode_page.lp4, file);
    (void) chunk_id;
}


static void palchip_Saver(int chunk_id, FILE *file)
{
    int i;

    fwrite_int16(32, file);

    for (i=0; i<16; i++)
    {
        fwrite_int8(pal_chip.color[i].gr, file);
        fwrite_int8(pal_chip.color[i].b, file);
    }

    (void) chunk_id;
}


static void diskctrl_Saver(int chunk_id, FILE *file)
{
    fwrite_int16(13, file);
    fwrite_int8(disk_ctrl.cmd0, file);
    fwrite_int8(disk_ctrl.cmd1, file);
    fwrite_int8(disk_ctrl.cmd2, file);
    fwrite_int8(disk_ctrl.stat0, file);
    fwrite_int8(disk_ctrl.stat1, file);
    fwrite_int8(disk_ctrl.wdata, file);
    fwrite_int8(disk_ctrl.rdata, file);
    fwrite_int8(disk_ctrl.wclk, file);
    fwrite_int8(disk_ctrl.wsect, file);
    fwrite_int8(disk_ctrl.wtrck, file);
    fwrite_int8(disk_ctrl.wcell, file);
    fwrite_int16(disk_ctrl.prot, file);
    (void) chunk_id;
}


static void mempager_Saver(int chunk_id, FILE *file)
{
    fwrite_int16(10, file);
    fwrite_int8(mempager.cart.page       , file);
    fwrite_int8(mempager.cart.rom_page   , file);
    fwrite_int8(mempager.cart.ram_page   , file);
    fwrite_int8(mempager.screen.page     , file);
    fwrite_int8(mempager.screen.vram_page, file);
    fwrite_int8(mempager.system.page     , file);
    fwrite_int8(mempager.data.page       , file);
    fwrite_int8(mempager.data.reg_page   , file);
    fwrite_int8(mempager.data.pia_page   , file);
    fwrite_int8(mempager.mon.page        , file);
    (void) chunk_id;
}


static void membank_Saver(int chunk_id, FILE *file)
{
    int bank = chunk_id&0xFF;
    int begin = 0, end = mem.ram.size, size;

    while ((begin < mem.ram.size) && (mem.ram.bank[bank][begin] == 0))
        begin++;

    while ((end>begin) && (mem.ram.bank[bank][end-1] == 0))
        end--;

    size = end-begin;

    fwrite_int16(2+size, file);
    fwrite_int16(begin, file);
    fwrite(mem.ram.bank[bank]+begin, 1, size, file);
}    


static void mb_Saver(int chunk_id, FILE *file)
{
    fwrite_int16(9, file);
    fwrite_int64(mb.exact_clock, file);
    fwrite_int8(mb.direct_screen_mode, file);
    (void) chunk_id;
}


static void (*Saver[32])(int, FILE *) = {
    NULL,           mc6809_Saver,   mc6846_Saver,   mc6821_Saver,
    modepage_Saver, palchip_Saver,  diskctrl_Saver, mempager_Saver,
    membank_Saver,  mb_Saver,       NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
    NULL,           NULL,           NULL,           NULL,
};


/* ------------------------------------------------------------------------- */


/* image_Load:
 *  Charge une image et modifie l'état de l'émulateur.
 */
int image_Load(const char filename[])
{
    FILE *file;
    char buffer[FORMAT_ID_SIZE];
    int value, chunk_id, chunk_size;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";

    (void)snprintf (fname, MAX_PATH, "%s/.teo/%s", getenv("HOME"), filename);
    if ((file = fopen(fname, "rb")) == NULL)
        return error_Message(TO8_CANNOT_OPEN_FILE, NULL);
#else
    if ((file = fopen(filename, "rb")) == NULL)
        return error_Message(TO8_CANNOT_OPEN_FILE, NULL);
#endif
    /* lecture de l'entête */
    if (fread(buffer, 1, FORMAT_ID_SIZE, file) != FORMAT_ID_SIZE)  /* format_id */
    {
        fclose(file);
        return error_Message(TO8_CANNOT_OPEN_FILE, NULL);
    }
    
    if (strncmp(buffer, format_id, FORMAT_ID_SIZE))
    {
        fclose(file);
        return error_Message(TO8_BAD_FILE_FORMAT, NULL);
    }

    fread_int16(&value, file);  /* format_ver */
    /* pour le moment, la version du format ne sert à rien... */

    fread_int16(&value, file);  /* thomson_id */

    if ((value != thomson[THOMSON_TO8].id) && (value != thomson[THOMSON_TO8D].id))
    {
        fclose(file);
        return error_Message(TO8_UNSUPPORTED_MODEL, NULL);
    }

    fseek(file, 6, SEEK_CUR);  /* reserved */

    while (TRUE)
    {
        fread_int16(&chunk_id, file);

        if (feof(file))
            break;

        fread_int16(&chunk_size, file);

        if (chunk_id >= 0x2000)
        {
            fclose(file);
            return error_Message(TO8_BAD_FILE_FORMAT, NULL);
        }

        Loader[chunk_id>>8](chunk_id, chunk_size, file);
    } 

    fclose(file);

    to8_new_video_params = TRUE;

    return 0;
}



/* Image_Save:
 *  Sauvegarde une image de l'état de l'émulateur.
 */
int image_Save(const char filename[])
{
    FILE *file;
    int chunk_id, i;
#ifdef DEBIAN_BUILD
    char fname[MAX_PATH+1] = "";

    (void)snprintf (fname, MAX_PATH, "%s/.teo/%s", getenv("HOME"), filename);
    if ((file = fopen(fname, "wb")) == NULL)
        return error_Message(TO8_CANNOT_OPEN_FILE, NULL);
#else
    if ((file = fopen(filename, "wb")) == NULL)
        return error_Message(TO8_CANNOT_OPEN_FILE, NULL);
#endif
    /* écriture de l'entête */
    fwrite(format_id, 1, FORMAT_ID_SIZE, file); 
    fwrite_int16(format_ver, file);
    fwrite_int16(thomson[THOMSON_TO8D].id, file);

    fwrite_int16(0, file); /* reserved */
    fwrite_int16(0, file); /* reserved */
    fwrite_int16(0, file); /* reserved */

    /* sauvegarde de l'état des composants matériels */
    for (i=0; i<HW_MAX; i++)
        if (i != HW_MEMORY_BANK)
        {
            chunk_id = hardware_id[i];
            fwrite_int16(chunk_id, file);
            Saver[chunk_id>>8](chunk_id, file);
        }

    /* sauvegarde des banques mémoire */
    chunk_id = hardware_id[HW_MEMORY_BANK];

    for (i=0; i<mem.ram.nbank; i++)
    {
        fwrite_int16(chunk_id+i, file);
        Saver[chunk_id>>8](chunk_id+i, file);
    }

    fclose(file);
    return 0;
}

