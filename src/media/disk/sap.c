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
 *  Version    : 1.8.5
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 15/09/2006 26/01/2010 12/01/2012 25/04/2012
 *                               15/05/2012 23/08/2015 31/07/2016
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
#include "media/disk.h"

#if 0
#define DO_PRINT  1      /* if output wanted */
#endif 

#define SAP_HEADER_SIZE  66
#define SAP_MAGIC_NUM    0xB3
#define SAP_FORMAT       0
#define SAP_PROTECTION   1
#define SAP_TRACK        2
#define SAP_SECTOR       3
#define SAP_DATA         4
#define SAP_SD_CRC_HIGH  (SAP_DATA+TEO_DISK_SD_SECTOR_SIZE)
#define SAP_SD_CRC_LOW   (SAP_DATA+TEO_DISK_SD_SECTOR_SIZE+1)
#define SAP_SD_SECT_SIZE (SAP_DATA+TEO_DISK_SD_SECTOR_SIZE+2)
#define SAP_DD_CRC_HIGH  (SAP_DATA+TEO_DISK_DD_SECTOR_SIZE)
#define SAP_DD_CRC_LOW   (SAP_DATA+TEO_DISK_DD_SECTOR_SIZE+1)
#define SAP_DD_SECT_SIZE (SAP_DATA+TEO_DISK_DD_SECTOR_SIZE+2)

static const char sap_header[]="\1SYSTEME D'ARCHIVAGE PUKALL S.A.P. "
                               "(c) Alexandre PUKALL Avril 1998";
static char sap_read_header[SAP_HEADER_SIZE] = "";
static uint8 sap_sector[SAP_DD_SECT_SIZE];

/* table de calcul du CRC */
static short int crcpuk_temp;
static short int puktable[]={
   0x0000, 0x1081, 0x2102, 0x3183,
   0x4204, 0x5285, 0x6306, 0x7387,
   0x8408, 0x9489, 0xa50a, 0xb58b,
   0xc60c, 0xd68d, 0xe70e, 0xf78f
};



#ifdef DO_PRINT
/* display_track :
 *  Display a track
 */
static void display_track (char *message, int drive, int track)
{
    int i;

    printf ("---------------------------------------\n");
    printf ("SAP %s drive %d track %d\n", message, drive, track);
    for (i=0; i<disk[drive].track_size;i++)
    {
        if ((i&15) == 0)
            printf ("\n%04x   ", i);
        printf ("%02x%02x ",
            disk[drive].clck[i],
            disk[drive].data[i]);
    }
    printf ("\n\n");
}
#endif



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

    crc_pukall((short int)sap_sector[SAP_FORMAT]);
    crc_pukall((short int)sap_sector[SAP_PROTECTION]);
    crc_pukall((short int)sap_sector[SAP_TRACK]);
    crc_pukall((short int)sap_sector[SAP_SECTOR]);

    for (i=0; i<sector_size; i++)
       crc_pukall((short int)data_sector[i]);
}



#define FILE_SEEK (long int)(SAP_HEADER_SIZE \
                             + (track \
                              * TEO_DISK_SECTOR_PER_TRACK \
                              * (disk[drive].sector_size+6)))

static int sap_error (int drive, int error, FILE *file)
{
    if (file != NULL)
        fclose (file);
    return error_Message (error, teo.disk[drive].file);
}



/* read_fm_track:
 *  Read and convert the SAP track (simple density).
 */
static int read_fm_track (int drive, int track)
{
    FILE *file = NULL;
    int i;
    int pos;
    int crc;
    int sector;
    size_t secsiz = (size_t)SAP_SD_SECT_SIZE;

    /* initialize track */
    memset (disk[drive].data, FM_GAP_DATA_VALUE, TEO_DISK_TRACK_SIZE_MAX);
    memset (disk[drive].clck, DATA_CLOCK_MARK, TEO_DISK_TRACK_SIZE_MAX);

    /* read-open SAP file */
    if ((file = fopen (teo.disk[drive].file, "rb")) == NULL)
        return sap_error (drive, TEO_ERROR_DISK_NONE, file);

    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return sap_error (drive, TEO_ERROR_FILE_READ, file);

    /* read sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* read the sector */
        if (fread (sap_sector, 1, secsiz, file) != secsiz)
            return sap_error (drive, TEO_ERROR_FILE_READ, file);

        /* compute sector offset (interleave 7) */
        pos = SDSECTORPOS (sap_sector[SAP_SECTOR]);

        /* decode sector datas */
        for (i=0;i<TEO_DISK_SD_SECTOR_SIZE;i++)
            sap_sector[SAP_DATA+i] ^= SAP_MAGIC_NUM;

        /* create sector */
        disk_CreateSDFloppySector (
            sap_sector[SAP_TRACK],
            sap_sector[SAP_SECTOR],
            sap_sector+SAP_DATA,
            disk[drive].data+pos,
            disk[drive].clck+pos);

        /* check crc */
        do_crc (sap_sector,
                disk[drive].data+pos+38,
                TEO_DISK_SD_SECTOR_SIZE);

        crc = ((int)sap_sector[SAP_SD_CRC_HIGH] & 0xff) << 8;
        crc |= (int)sap_sector[SAP_SD_CRC_LOW] & 0xff;
        
        if (crc != ((int)crcpuk_temp & 0xffff))
            *(disk[drive].data+pos+167) += 1;  /* generate crc error */
    }
    fclose(file);
#ifdef DO_PRINT
    display_track ("read_fm_track", drive, track);
#endif
    return 0;
}



/* read_mfm_track:
 *  Read and convert the SAP track (double density).
 */
static int read_mfm_track (int drive, int track)
{
    FILE *file = NULL;
    int i;
    int pos;
    int crc;
    int sector;
    size_t secsiz = (size_t)SAP_DD_SECT_SIZE;

    /* initialize track */
    memset (disk[drive].data, MFM_PROT_GAP_DATA_VALUE, TEO_DISK_TRACK_SIZE_MAX);
    memset (disk[drive].clck, DATA_CLOCK_MARK, TEO_DISK_TRACK_SIZE_MAX);

    /* read-open SAP file */
    if ((file = fopen (teo.disk[drive].file, "rb")) == NULL)
        return sap_error (drive, TEO_ERROR_DISK_NONE, file);

    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return sap_error (drive, TEO_ERROR_FILE_READ, file);

    /* read sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* read the sector */
        if (fread (sap_sector, 1, secsiz, file) != secsiz)
            return sap_error (drive, TEO_ERROR_FILE_READ, file);

        /* compute sector offset (interleave 7) */
        pos = DDSECTORPOS (sap_sector[SAP_SECTOR]);

        /* decode sector datas */
        for (i=0;i<TEO_DISK_DD_SECTOR_SIZE;i++)
            sap_sector[SAP_DATA+i] ^= SAP_MAGIC_NUM;

        /* create sector */
        disk_CreateDDFloppySector (
            sap_sector[SAP_TRACK],
            sap_sector[SAP_SECTOR],
            sap_sector+SAP_DATA,
            disk[drive].data+pos,
            disk[drive].clck+pos);

        /* check crc */
        do_crc (sap_sector,
                disk[drive].data+pos+92,
                TEO_DISK_DD_SECTOR_SIZE);

        crc = ((int)sap_sector[SAP_DD_CRC_HIGH] & 0xff) << 8;
        crc |= (int)sap_sector[SAP_DD_CRC_LOW] & 0xff;

        if (crc != ((int)crcpuk_temp & 0xffff))
            *(disk[drive].data+pos+349) += 1;  /* generate crc error */
    }
    fclose(file);
#ifdef DO_PRINT
    display_track ("read_mfm_track", drive, track);
#endif
    return 0;
}



/* write_fm_track:
 *  Convert and write the SAP track (simple density).
 */
static int write_fm_track (int drive, int track)
{
    FILE *file = NULL;
    int i = 0;
    int pos;
    int sector;
    size_t secsiz = (size_t)SAP_SD_SECT_SIZE;
    uint8 *secptr;

    /* write-open SAP file */
    if ((file = fopen (teo.disk[drive].file, "rb+")) == NULL)
        return sap_error (drive, TEO_ERROR_DISK_NONE, file);

    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return sap_error (drive, TEO_ERROR_FILE_WRITE, file);

    for (sector=1; sector<=16; sector++)
    {
        if ((pos = disk_IsSDFloppySector (track, sector)) >= 0)
        {
            secptr = &disk[drive].data[pos];

            /* init sap sector header */
            sap_sector[SAP_FORMAT]     = 0x00;
            sap_sector[SAP_PROTECTION] = 0x00;
            sap_sector[SAP_TRACK]      = track;
            sap_sector[SAP_SECTOR]     = sector;

            /* update special format */
            if (secptr[156] == 0xf7)
                sap_sector[SAP_FORMAT] = 0x04;

            /* compute crc */
            do_crc (sap_sector, secptr+26, TEO_DISK_SD_SECTOR_SIZE);

            /* encode sector datas */
            for (i=0;i<TEO_DISK_SD_SECTOR_SIZE;i++)
                sap_sector[SAP_DATA+i] = secptr[26+i]^SAP_MAGIC_NUM;

            /* update crc */
            sap_sector[SAP_SD_CRC_HIGH] = (uint8)(crcpuk_temp>>8);
            sap_sector[SAP_SD_CRC_LOW]  = (uint8)crcpuk_temp;

            /* write the sector */
            if (fwrite (sap_sector, 1, secsiz, file) != secsiz)
                return sap_error (drive, TEO_ERROR_FILE_WRITE, file);
        }
    }
    fclose(file);
#ifdef DO_PRINT
    display_track ("write_fm_track", drive, track);
#endif
    return 0;
}



/* write_mfm_track:
 *  Convert and write the SAP track (double density).
 */
static int write_mfm_track (int drive, int track)
{
    FILE *file = NULL;
    int i = 0;
    int pos;
    int sector;
    size_t secsiz = (size_t)SAP_DD_SECT_SIZE;
    uint8 *secptr;

    /* write-open SAP file */
    if ((file = fopen (teo.disk[drive].file, "rb+")) == NULL)
        return sap_error (drive, TEO_ERROR_DISK_NONE, file);

    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return sap_error (drive, TEO_ERROR_FILE_WRITE, file);

    for (sector=1; sector<=16; sector++)
    {
        if ((pos = disk_IsDDFloppySector (track, sector)) >= 0)
        {
            secptr = &disk[drive].data[pos];

            /* init sap sector header */
            sap_sector[SAP_FORMAT]     = 0x00;
            sap_sector[SAP_PROTECTION] = 0x00;
            sap_sector[SAP_TRACK]      = track;
            sap_sector[SAP_SECTOR]     = sector;

            /* update special format */
            if (secptr[306] == 0xf7)
                sap_sector[SAP_FORMAT] = 0x04;

            /* compute crc */
            do_crc (sap_sector, secptr+48, TEO_DISK_DD_SECTOR_SIZE);

            /* encode sector datas */
            for (i=0;i<TEO_DISK_DD_SECTOR_SIZE;i++)
                sap_sector[SAP_DATA+i] = secptr[48+i]^SAP_MAGIC_NUM;

            /* update crc */
            sap_sector[SAP_DD_CRC_HIGH] = (uint8)(crcpuk_temp>>8);
            sap_sector[SAP_DD_CRC_LOW]  = (uint8)crcpuk_temp;

            /* write the sector */
            if (fwrite (sap_sector, 1, secsiz, file) != secsiz)
                return sap_error (drive, TEO_ERROR_FILE_WRITE, file);
        }
    }
    fclose(file);
#ifdef DO_PRINT
    display_track ("write_mfm_track", drive, track);
#endif
    return 0;
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

    if (protection >= 0)
    {
        /* write-update the track of the current drive */
        disk_WriteTrack ();

        teo.disk[drive].file = std_free (teo.disk[drive].file);
        teo.disk[drive].file = std_strdup_printf ("%s", filename);
        teo.disk[drive].write_protect = protection;
        switch (*sap_read_header)
        {
            case '\1' :
                disk[drive].sector_size = TEO_DISK_DD_SECTOR_SIZE;
                disk[drive].track_count = TEO_DISK_DD_TRACK_NUMBER;
                disk[drive].track_size = TEO_DISK_DD_TRACK_SIZE;
                disk[drive].byte_rate = TEO_DISK_DD_BYTE_RATE;
                disk[drive].ReadTrack = read_mfm_track;
                disk[drive].WriteTrack = write_mfm_track;
                break;
                     
            case '\2' :
                disk[drive].sector_size = TEO_DISK_SD_SECTOR_SIZE;
                disk[drive].track_count = TEO_DISK_SD_TRACK_NUMBER;
                disk[drive].track_size = TEO_DISK_SD_TRACK_SIZE;
                disk[drive].byte_rate = TEO_DISK_SD_BYTE_RATE;
                disk[drive].ReadTrack = read_fm_track;
                disk[drive].WriteTrack = write_fm_track;
                break;
        }
        disk[drive].state = TEO_DISK_ACCESS_SAP;
        disk[drive].write_protect = FALSE;
        disk[drive].ReadSector = NULL;
        disk[drive].WriteSector = NULL;
        disk[drive].FormatTrack = NULL;
        disk[drive].IsWritable = NULL;
        disk[drive].side_count = 1;
    }
    return protection;
}
