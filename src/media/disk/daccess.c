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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : media/disk/daccess.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 21/01/2013
 *  Modifié par: François Mouret & Eric Botcazou 31/07/2016
 *
 *  Management of direct access disk.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
#endif

#include "defs.h"
#include "teo.h"
#include "errors.h"
#include "std.h"
#include "hardware.h"
#include "media/disk.h"



static int return_error (int err)
{
    switch (err)
    {
        case 0x01 :  /* Write Protected */
            return error_Message (TEO_ERROR_DISK_PROTECT, NULL);

        case 0x04 :   /* error on address */
        case 0x08 :   /* error on datas */
            return error_Message (TEO_ERROR_DISK_IO, NULL);

        case 0x10 :   /* drive not ready */
            return error_Message (TEO_ERROR_DISK_NONE, NULL);
    }
    return 0;
}



static void create_sector (int drive, uint8 *sector_buffer)
{
    int pos;

    /* clear track */
    memset (disk[drive].clck, DATA_CLOCK_MARK, disk[drive].track_size);
    memset (disk[drive].data, MFM_PROT_GAP_DATA_VALUE, disk[drive].track_size);

    /* compute sector offset (interleave 7) */
    pos = DDSECTORPOS (disk[drive].drv->sector);

    /* create sector */
    disk_CreateDDFloppySector (disk[drive].drv->track.curr,
                               disk[drive].drv->sector,
                               sector_buffer,
                               disk[drive].data+pos,
                               disk[drive].clck+pos);
}



/* read_mfm_sector:
 *  Read the direct access sector (double density).
 */
static int read_mfm_sector (int drive)
{
    int err;
    /* MSDOS: the BIOS needs a buffer of 512 bytes */
    uint8 sector_buffer[512];

    if (disk[drive].drv->track.curr >= disk[drive].track_count)
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    if (teo_DirectReadSector == NULL)
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    err = teo_DirectReadSector (
                drive,
                disk[drive].drv->track.curr,
                disk[drive].drv->sector,
                1,
                sector_buffer);

    create_sector (drive, sector_buffer);

    return return_error(err);
}



/* write_mfm_sector:
 *  Write a direct access sector (double density).
 */
static int write_mfm_sector (int drive, int buffer)
{
    int i;
    int err = 0;
    /* MSDOS: the BIOS needs a buffer of 512 bytes */
    uint8 sector_buffer[512];

    if (disk[drive].drv->track.curr >= disk[drive].track_count)
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    if (teo_DirectWriteSector == NULL)
        return error_Message (TEO_ERROR_FILE_WRITE, NULL);

    /* copy the sector datas */
    for (i=0; i<256; i++)
         sector_buffer[i]=LOAD_BYTE((buffer+i)&0xFFFF);

    /* MSDOS: necessary so that the sector could be read by a real TO8 */
    sector_buffer[256]=0;

    /* write the sector */
    err = teo_DirectWriteSector (
            drive,
            disk[drive].drv->track.curr,
            disk[drive].drv->sector,
            1,
            sector_buffer);

    create_sector (drive, sector_buffer);

    return return_error(err);
}



/* build_sector_map:
 *  Construit la carte des secteurs d'une piste en fonction
 *  du facteur d'entrelacement.
 */
static void build_sector_map (int *sector_map, int factor)
{
    int sector;
    int i=0;

    /* mise à zéro de la table */
    memset(sector_map, 0, sizeof(int)*TEO_DISK_SECTOR_PER_TRACK);

    for (sector=1; sector<=TEO_DISK_SECTOR_PER_TRACK; sector++)
    {
        while (sector_map[i] != 0)
            i=(i+1)%TEO_DISK_SECTOR_PER_TRACK;

        sector_map[i]=sector;

        i=(i+factor)%TEO_DISK_SECTOR_PER_TRACK;
    }
}



/* format_mfm_track:
 *  Format the direct access track (double density).
 */
static int format_mfm_track (int drive)
{
    int err = 0;
    int sect, pos;
    int sector_map[TEO_DISK_SECTOR_PER_TRACK];
    /* MSDOS: the BIOS needs a buffer of 512 bytes */
    unsigned char headers_table[512];

    if (disk[drive].drv->track.curr >= disk[drive].track_count)
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    if (teo_DirectFormatTrack == NULL)
        return error_Message (TEO_ERROR_FILE_WRITE, NULL);

    /* construction de la carte des secteurs pour chaque piste,
       à partir du facteur d'entrelacement situé en 0x604D */
    build_sector_map (sector_map, LOAD_BYTE(0x604D));

    /* construction de la table des headers */
    for (sect=1, pos=0; sect<=TEO_DISK_SECTOR_PER_TRACK; sect++, pos+=4)
    {
        headers_table[pos]   = disk[drive].drv->track.curr;
        headers_table[pos+1] = 0;
        headers_table[pos+2] = sector_map[sect-1];
        headers_table[pos+3] = 1;
    }

    /* formatage de la piste */
    err = teo_DirectFormatTrack (drive, disk[drive].drv->track.curr, headers_table);

    return return_error(err);
}



/* file_protection:
 *  Checks the direct access format and returns the open mode.
 */
static int file_protection (const char filename[], int protection)
{
    if (teo_DirectWriteSector == NULL)
    {
        protection = TRUE;
        if (teo_DirectReadSector == NULL)
            protection = error_Message (TEO_ERROR_FILE_FORMAT, filename);
    }
    return protection;
}


/* ------------------------------------------------------------------------- */


/* daccess_LoadDisk:
 *  Loads the direct access into the specified drive and
 *  forces the read-only mode if necessary.
 */
int daccess_LoadDisk (int drive, const char filename[])
{
    int protection;

    protection = file_protection (filename, teo.disk[drive].write_protect);

    if (protection >= 0)
    {
        /* write-update the track of the current drive */
        disk_WriteTrack ();

        disk[drive].sector_size = TEO_DISK_DD_SECTOR_SIZE;
        disk[drive].track_count = TEO_DISK_DD_TRACK_NUMBER;
        disk[drive].track_size = TEO_DISK_DD_TRACK_SIZE;
        disk[drive].byte_rate = TEO_DISK_DD_BYTE_RATE;
        disk[drive].drv->track.curr = 0;
        disk[drive].drv->track.last = TEO_DISK_INVALID_NUMBER;

        /* update parameters */
        teo.disk[drive].file = std_free (teo.disk[drive].file);
        teo.disk[drive].file = std_strdup_printf ("%s", filename);
        teo.disk[drive].write_protect = protection;
        disk[drive].state = TEO_DISK_ACCESS_DIRECT;
        disk[drive].write_protect = FALSE;
        disk[drive].ReadTrack = NULL;
        disk[drive].WriteTrack = NULL;
        disk[drive].ReadSector = read_mfm_sector;
        disk[drive].WriteSector = write_mfm_sector;
        disk[drive].FormatTrack = format_mfm_track;
        disk[drive].IsWritable = teo_DirectIsDiskWritable;
        disk[drive].side_count = 1;
    }
    return protection;
    (void)filename;
}

