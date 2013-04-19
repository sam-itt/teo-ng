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
 *  Module     : media/disk/daccess.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 21/01/2013
 *  Modifié par: 
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
#include "media/disk/controlr.h"
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



static void create_sector (uint8 *sector_buffer, struct DISK_INFO *info)
{
    int pos;

    /* clear track */
    memset (info->clck, DATA_CLOCK_MARK, info->track_size);
    memset (info->data, MFM_GAP_DATA_VALUE, info->track_size);

    /* compute sector offset (interleave 7) */
    pos = DDSECTORPOS (info->sector);

    /* create sector */
    disk_CreateDDFloppySector (info->track,
                               info->sector,
                               sector_buffer,
                               info->data+pos,
                               info->clck+pos);
}



/* read_mfm_sector:
 *  Read the direct access sector (double density).
 */
static int read_mfm_sector (const char filename [], struct DISK_INFO *info)
{
    int err;
    /* MSDOS: the BIOS needs a buffer of 512 bytes */
    uint8 sector_buffer[512];

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    if (teo_DirectReadSector == NULL)
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    err = disk_AllocRawTracks (TEO_DISK_MFM_TRACK_SIZE, info);
    if (err != 0)
        return err;

    err = teo_DirectReadSector (info->drive, info->track, info->sector,
                                1, sector_buffer);

    create_sector (sector_buffer, info);

    return return_error(err);
}



/* write_mfm_sector:
 *  Write a direct access sector (double density).
 */
static int write_mfm_sector (const char filename [], int buffer,
                             struct DISK_INFO *info)
{
    int i;
    int err = 0;
    /* MSDOS: the BIOS needs a buffer of 512 bytes */
    uint8 sector_buffer[512];

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    if (teo_DirectWriteSector == NULL)
        return error_Message (TEO_ERROR_FILE_WRITE, NULL);

    /* copy the sector datas */
    for (i=0; i<256; i++)
         sector_buffer[i]=LOAD_BYTE((buffer+i)&0xFFFF);

    /* MSDOS: necessary so that the sector could be read by a real TO8 */
    sector_buffer[256]=0;

    /* write the sector */
    err = teo_DirectWriteSector (info->drive, info->track, info->sector,
                                 1, sector_buffer);

    create_sector (sector_buffer, info);

    return return_error(err);
}



/* format_mfm_track:
 *  Format the direct access track (double density).
 */
#define NBSECT    16
static int format_mfm_track (const char filename [], struct DISK_INFO *info)
{
    int err = 0;
    int sect, pos;
    int sector_map[NBSECT];
    /* MSDOS: the BIOS needs a buffer of 512 bytes */
    unsigned char headers_table[512];

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, NULL);

    if (teo_DirectFormatTrack == NULL)
        return error_Message (TEO_ERROR_FILE_WRITE, NULL);

    /* construction de la carte des secteurs pour chaque piste,
       à partir du facteur d'entrelacement situé en 0x604D */
    disk_BuildSectorMap (sector_map, LOAD_BYTE(0x604D));

    /* construction de la table des headers */
    for (sect=1, pos=0; sect<=NBSECT; sect++, pos+=4)
    {
        headers_table[pos]   = info->track;
        headers_table[pos+1] = 0;
        headers_table[pos+2] = sector_map[sect-1];
        headers_table[pos+3] = 1;
    }

    /* formatage de la piste */
    err = teo_DirectFormatTrack (info->drive, info->track, headers_table);

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
    (void)filename;
}


/* ------------------------------------------------------------------------- */


/* fd_IsFd:
 *  Check if the file is a FD/QD file.
 */
int fd_IsDirectAccess (const char filename[])
{
    return file_protection (filename, FALSE);
}



/* daccess_LoadDisk:
 *  Loads the FD archive into the specified drive and
 *  forces the read-only mode if necessary.
 */
int daccess_LoadDisk (int drive, const char filename[])
{
    int protection;

    protection = file_protection (filename, teo.disk[drive].write_protect);

    if (protection >= 0)
    {
        disk[drive].info->sector_size  = 256;
        disk[drive].info->fat_size     = 180;
        disk[drive].info->track_count  = 80;
        disk[drive].info->byte_rate    = 250000/8;
        disk[drive].info->track = -1;  /* force track to be loaded */

        /* update parameters */
        teo.disk[drive].file = std_free (teo.disk[drive].file);
        teo.disk[drive].write_protect = protection;
        disk[drive].state = TEO_DISK_ACCESS_DIRECT;
        disk[drive].ReadCtrlTrack = NULL;
        disk[drive].WriteCtrlTrack = NULL;
        disk[drive].ReadCtrlSector = read_mfm_sector;
        disk[drive].WriteCtrlSector = write_mfm_sector;
        disk[drive].FormatCtrlTrack = format_mfm_track;
        disk[drive].side_count = 2;
    }
    return protection;
    (void)filename;
}

