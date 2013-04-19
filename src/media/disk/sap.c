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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret,
 *                          Samuel Devulder
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
 *  Module     : media/disk/sap.c
 *  Version    : 1.8.2
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 15/09/2006 26/01/2010 12/01/2012 25/04/2012
 *                               15/05/2012
 *               Samuel Devulder 05/02/2012
 *
 *  Gestion du format SAP 2.0: lecture et écriture disquette.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
#endif

#include "defs.h"
#include "teo.h"
#include "errors.h"
#include "std.h"
#include "media/disk/controlr.h"
#include "media/disk.h"


#define SAP_HEADER_SIZE  66
#define SAP_DD_SECT_SIZE 262
#define SAP_SD_SECT_SIZE 134
#define SAP_MAGIC_NUM    0xB3

#define SAPSECTOR_FORMAT      0
#define SAPSECTOR_PROTECTION  1
#define SAPSECTOR_TRACK       2
#define SAPSECTOR_SECTOR      3
#define SAPSECTOR_DATA        4


static const char sap_header[]="\1SYSTEME D'ARCHIVAGE PUKALL S.A.P. "
                               "(c) Alexandre PUKALL Avril 1998";

static char sap_read_header[SAP_HEADER_SIZE] = "";

#if 0
/* type d'un secteur SAP */
typedef struct {
    char format;
    char protection;
    char track;
    char sector;
} SAP_SECTOR_HEADER;

static SAP_SECTOR_HEADER sap_hd;
#endif

/* table de calcul du CRC */
static short int crcpuk_temp;
static short int puktable[]={
   0x0000, 0x1081, 0x2102, 0x3183,
   0x4204, 0x5285, 0x6306, 0x7387,
   0x8408, 0x9489, 0xa50a, 0xb58b,
   0xc60c, 0xd68d, 0xe70e, 0xf78f
};



/* crc_pukall:
 *  Calcule le nouveau CRC à partir de la donnée c.
 */
static void crc_pukall(short int c)
{
    register short int index;

    index = (crcpuk_temp ^ c) & 0xf;
    crcpuk_temp = ((crcpuk_temp>>4) & 0xfff) ^ puktable[index];

    c >>= 4;

    index = (crcpuk_temp ^ c) & 0xf;
    crcpuk_temp = ((crcpuk_temp>>4) & 0xfff) ^ puktable[index];
}



/* do_crc:
 *  Calcule le CRC d'un secteur SAP.
 */
static void do_crc(uint8 *sap_sector, uint8 *data_sector, int sector_size)
{
    register int i;

    crcpuk_temp = 0xffff;

    crc_pukall((short int)sap_sector[SAPSECTOR_FORMAT]);
    crc_pukall((short int)sap_sector[SAPSECTOR_PROTECTION]);
    crc_pukall((short int)sap_sector[SAPSECTOR_TRACK]);
    crc_pukall((short int)sap_sector[SAPSECTOR_SECTOR]);

    for (i=0; i<sector_size; i++)
       crc_pukall((short int)data_sector[i]);
}


/*
static void display_track (struct DISK_INFO *info)
{
    int i;

    for (i=0; i<info->track_size;i++)
    {
        if ((i&15) == 0)
            printf ("\n%04x   ", i);
        printf ("%02x%02x ", info->clck[i], info->data[i]);
    }
    printf ("\n\n");
}
*/


/* read_fm_track:
 *  Read and convert the SAP track (simple density).
 */
static int read_fm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    int pos;
    int crc;
    int sector;
    uint8 sap_sector[SAP_DD_SECT_SIZE];

    /* initialize track */
    memset (info->data, FM_GAP_DATA_VALUE, info->track_size);
    memset (info->clck, DATA_CLOCK_MARK, info->track_size);

    /* read sectors */    
    for (sector=0; sector<16; sector++)
    {
        /* read the sector */
        if (fread (sap_sector, 1, (size_t)SAP_SD_SECT_SIZE,
                                   file) != (size_t)SAP_SD_SECT_SIZE)
            return error_Message (TEO_ERROR_DISK_IO, NULL);

        /* compute sector offset (interleave 7) */
        pos = SDSECTORPOS (sap_sector[SAPSECTOR_SECTOR]);

        /* decode sector datas */
        for (i=0;i<128;i++)
            sap_sector[SAPSECTOR_DATA+i] ^= SAP_MAGIC_NUM;

        /* create sector */
        disk_CreateSDFloppySector (sap_sector[SAPSECTOR_TRACK],
                                   sap_sector[SAPSECTOR_SECTOR],
                                   sap_sector+SAPSECTOR_DATA,
                                   info->data+pos,
                                   info->clck+pos);

        /* check crc */
        do_crc (sap_sector, info->data+pos+38, 128);
        crc = (int)((*(sap_sector+132) << 8) | *(sap_sector+133));
        if (crc != ((int)crcpuk_temp & 0xffff))
            *(info->data+pos+167) += 1;  /* generate crc error */
    }
    pos = 16 * FM_SECTOR_SIZE;
    if (pos < info->track_size)
        memset (info->data+pos, FM_GAP_DATA_VALUE, info->track_size-pos);

    return 0;
}



/* read_mfm_track:
 *  Read and convert the SAP track (double density).
 */
static int read_mfm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    int pos;
    int crc;
    int sector;
    uint8 sap_sector[SAP_DD_SECT_SIZE];

    /* initialize track */
    memset (info->clck, DATA_CLOCK_MARK, info->track_size);

    /* read sectors */    
    for (sector=0; sector<16; sector++)
    {
        /* read the sector */
        if (fread (sap_sector, 1, (size_t)SAP_DD_SECT_SIZE,
                                file) != (size_t)SAP_DD_SECT_SIZE)
            return error_Message (TEO_ERROR_DISK_IO, NULL);

        /* compute sector offset (interleave 7) */
        pos = DDSECTORPOS (sap_sector[SAPSECTOR_SECTOR]);

        /* decode sector datas */
        for (i=0;i<256;i++)
            sap_sector[SAPSECTOR_DATA+i] ^= SAP_MAGIC_NUM;

        /* create sector */
        disk_CreateDDFloppySector (sap_sector[SAPSECTOR_TRACK],
                                   sap_sector[SAPSECTOR_SECTOR],
                                   sap_sector+SAPSECTOR_DATA,
                                   info->data+pos,
                                   info->clck+pos);

        /* check crc */
        do_crc (sap_sector, info->data+pos+92, 256);
        crc = (int)((*(sap_sector+260) << 8) | *(sap_sector+261));
        if (crc != ((int)crcpuk_temp & 0xffff))
            *(info->data+pos+349) += 1;  /* generate crc error */
    }
    pos = 16 * MFM_SECTOR_SIZE;
    if (pos < info->track_size)
        memset (info->data+pos, MFM_GAP_DATA_VALUE, info->track_size-pos);

    return 0;
}



/* read_ctrl_track:
 *  Open the file and read the SAP track.
 */
static int read_ctrl_track (const char filename [], struct DISK_INFO *info)
{
    int err = 0;
    FILE *file = NULL;
    long int file_seek = SAP_HEADER_SIZE
                         + info->track
                         * TEO_DISK_SECTOR_COUNT
                         * (info->sector_size+6);

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, filename);

    if ((file = fopen (filename, "rb")) == NULL)
        return error_Message (TEO_ERROR_DISK_NONE, filename);

    if (fseek (file, file_seek, SEEK_SET) != 0)
        err = error_Message (TEO_ERROR_FILE_READ, filename);

    if (err == 0)
        err = disk_AllocRawTracks (TEO_DISK_MFM_TRACK_SIZE, info);

    if (err == 0)
    {
        if (info->sector_size == 256)
        {
            err = disk_AllocRawTracks (TEO_DISK_MFM_TRACK_SIZE, info);
            if (err == 0)
                err = read_mfm_track (file, info);
        }
        else
        {
            err = disk_AllocRawTracks (TEO_DISK_MFM_TRACK_SIZE>>1, info);
            if (err == 0)
                err = read_fm_track (file, info);
        }
    }

    (void)fclose(file);
    return err;   
}



/* write_fm_track:
 *  Convert and write the SAP track (simple density).
 */
static int write_fm_track (FILE *file, struct DISK_INFO *info)
{
    int i = 0;
    int pos;
    int sector;
    uint8 sap_sector[SAP_DD_SECT_SIZE];

    for (sector=1; sector<=16; sector++)
    {
        if ((pos = disk_IsSDFloppySector (sector, info)) < 0)
            return TEO_ERROR;

        /* init sap sector header */
        sap_sector[SAPSECTOR_FORMAT]     = 0x00;
        sap_sector[SAPSECTOR_PROTECTION] = 0x00;
        sap_sector[SAPSECTOR_TRACK]      = info->track;
        sap_sector[SAPSECTOR_SECTOR]     = sector;

        /* update special format */
        if (info->data[pos+156] == 0xf7)
            sap_sector[SAPSECTOR_FORMAT] = 0x04;

        /* compute crc */
        do_crc (sap_sector, info->data+pos+26, 128);

        /* encode sector datas */
        for (i=0;i<128;i++)
            sap_sector[SAPSECTOR_DATA+i] = info->data[pos+26+i]^SAP_MAGIC_NUM;

        /* update crc */
        sap_sector[132] = (uint8)(crcpuk_temp>>8);
        sap_sector[133] = (uint8)crcpuk_temp;

        /* write the sector */
        if (fwrite (sap_sector, 1, (size_t)SAP_SD_SECT_SIZE,
                                    file) != (size_t)SAP_SD_SECT_SIZE)
            return error_Message (TEO_ERROR_DISK_IO, NULL);
    }
    return 0;
}



/* write_mfm_track:
 *  Convert and write the SAP track (double density).
 */
static int write_mfm_track (FILE *file, struct DISK_INFO *info)
{
    int i = 0;
    int pos;
    int sector;
    uint8 sap_sector[SAP_DD_SECT_SIZE];

    for (sector=1; sector<=16; sector++)
    {
        if ((pos = disk_IsDDFloppySector (sector, info)) < 0)
            return TEO_ERROR;

        /* init sap sector header */
        sap_sector[SAPSECTOR_FORMAT]     = 0x00;
        sap_sector[SAPSECTOR_PROTECTION] = 0x00;
        sap_sector[SAPSECTOR_TRACK]      = info->track;
        sap_sector[SAPSECTOR_SECTOR]     = sector;

        /* update special format */
        if (info->data[pos+306] == 0xf7)
            sap_sector[SAPSECTOR_FORMAT] = 0x04;

        /* compute crc */
        do_crc (sap_sector, info->data+pos+48, 256);

        /* encode sector datas */
        for (i=0;i<256;i++)
            sap_sector[SAPSECTOR_DATA+i] = info->data[48+pos+i]^SAP_MAGIC_NUM;

        /* update crc */
        sap_sector[260] = (uint8)(crcpuk_temp>>8);
        sap_sector[261] = (uint8)crcpuk_temp;

        /* write the sector */
        if (fwrite (sap_sector, 1, (size_t)SAP_DD_SECT_SIZE,
                                    file) != (size_t)SAP_DD_SECT_SIZE)
            return error_Message (TEO_ERROR_DISK_IO, NULL);
    }
    return 0;
}




/* write_ctrl_track:
 *  Open the file and write the SAP track.
 */
static int write_ctrl_track (const char filename [], struct DISK_INFO *info)
{
    int err = 0;
    FILE *file = NULL;
    long int file_seek = SAP_HEADER_SIZE
                         + info->track
                         * TEO_DISK_SECTOR_COUNT
                         * (info->sector_size+6);

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, filename);

    if ((file = fopen (filename, "rb+")) == NULL)
        return error_Message (TEO_ERROR_DISK_NONE, filename);

    if (fseek (file, file_seek, SEEK_SET) != 0)
        err = error_Message (TEO_ERROR_FILE_WRITE, filename);

    if (err == 0)
    {
        if (info->sector_size == 256)
            err = write_mfm_track (file, info);
        else
            err = write_fm_track (file, info);
    }

    (void)fclose(file);
    return err;   
}



/* file_mode:
 *  Loads the SAP header and returns the open mode.
 */
static int file_protection (const char filename[], int protection)
{
    FILE *file = NULL;
    size_t length;
    
    protection = disk_CheckFile(filename, protection);

    if (protection >= 0)
    {
        file=fopen(filename, "rb");

        /* on vérifie le header */
        length = fread(sap_read_header, sizeof(char), SAP_HEADER_SIZE, file);
        fclose(file);

        if ((length != SAP_HEADER_SIZE)
         || (strncmp(sap_read_header+1, sap_header+1, SAP_HEADER_SIZE-1) != 0))
             return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        if ((*sap_read_header != '\1') && (*sap_read_header != '\2'))
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);
    }
    return protection;
}


/* ------------------------------------------------------------------------- */


/* sap_IsSap:
 *  Check if the file is a SAP file.
 */
int sap_IsSap (const char filename[])
{
    return file_protection (filename, TRUE);
}



/* sap_LoadDisk:
 *  Loads the SAP archive into the specified drive and
 *  forces the read-only mode if necessary.
 */
int sap_LoadDisk(int drive, const char filename[])
{
    int protection = file_protection (filename, teo.disk[drive].write_protect);

    if (protection < 0)
        return TEO_ERROR;

    switch (*sap_read_header)
    {
        case '\1' : disk[drive].info->sector_size  = 256;
                    disk[drive].info->fat_size     = 160;
                    disk[drive].info->track_count  = 80;
                    disk[drive].info->byte_rate    = 250000/8;
                    break;
                     
        case '\2' : return error_Message (TEO_ERROR_FILE_FORMAT, filename);
/*
                    disk[drive].info->sector_size  = 128;
                    disk[drive].info->fat_size     = 80;
                    disk[drive].info->track_count  = 40;
                    disk[drive].info->byte_rate    = 125000/8;
*/
                    break;
    }
    disk[drive].info->track = -1;  /* force track to be loaded */

    /* update parameters */
    teo.disk[drive].file = std_free (teo.disk[drive].file);
    teo.disk[drive].file = std_strdup_printf ("%s", filename);
    teo.disk[drive].write_protect = protection;
    disk[drive].state = TEO_DISK_ACCESS_SAP;
    disk[drive].ReadCtrlTrack = read_ctrl_track;
    disk[drive].WriteCtrlTrack = write_ctrl_track;
    disk[drive].ReadCtrlSector = NULL;
    disk[drive].WriteCtrlSector = NULL;
    disk[drive].FormatCtrlTrack = NULL;
    disk[drive].side_count = 1;

    return protection;
}

