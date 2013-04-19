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
 *  Module     : media/disk/fd.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 21/01/2013
 *  Modifié par: 
 *
 *  Gestion du format raw (FD, QD).
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


struct FD_LIST {
    size_t file_size;
    char suffix[4];
    int side_count;
    int track_count;
    int fat_size;
    int sector_size;
};

static struct FD_LIST fd_list[] = {
    {   51200, ".qd", 1, 25,  50, 128 },
    {   51200, ".qd", 1, 25,  50, 128 },
    {   81920, ".fd", 1, 40,  80, 128 },
    {  163840, ".fd", 1, 40,  80, 256 },
    {  327680, ".fd", 1, 80, 160, 256 },
    {  655360, ".fd", 2, 80, 160, 256 },
    {  983040, ".fd", 3, 80, 160, 256 },
    { 1310720, ".fd", 4, 80, 160, 256 }
};

static int fd_type = 3;


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
 *  Read and convert the FD track (simple density).
 */
static int read_fm_track (FILE *file, struct DISK_INFO *info)
{
    int pos;
    int sector;
    uint8 sector_buffer[128];

    /* initialize track */
    memset (info->clck, DATA_CLOCK_MARK, info->track_size);

    /* read sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* compute sector offset (interleave 7) */
        pos = SDSECTORPOS (sector);

        /* read the sector */
        if (fread (sector_buffer, 1, (size_t)128, file) != (size_t)128)
            return error_Message (TEO_ERROR_DISK_IO, NULL);  /* error on sector */

        /* create sector */
        disk_CreateSDFloppySector (info->track,
                                   sector,
                                   sector_buffer,
                                   info->data+pos,
                                   info->clck+pos);
    }
    pos = 16 * FM_SECTOR_SIZE;
    if (pos < info->track_size)
        memset (info->data+pos, FM_GAP_DATA_VALUE, info->track_size-pos);

    return 0;
}



/* read_mfm_track:
 *  Read and convert the FD track (double density).
 */
static int read_mfm_track (FILE *file, struct DISK_INFO *info)
{
    int pos;
    int sector;
    uint8 sector_buffer[256];

    /* initialize track */
    memset (info->clck, DATA_CLOCK_MARK, info->track_size);

    /* read sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* compute sector offset (interleave 7) */
        pos = DDSECTORPOS (sector);

        /* read the sector */
        if (fread (sector_buffer, 1, (size_t)256, file) != (size_t)256)
            return error_Message (TEO_ERROR_DISK_IO, NULL);  /* error on sector */

        /* create sector */
        disk_CreateDDFloppySector (info->track,
                                   sector,
                                   sector_buffer,
                                   info->data+pos,
                                   info->clck+pos);
    }
    pos = 16 * MFM_SECTOR_SIZE;
    if (pos < info->track_size)
        memset (info->data+pos, MFM_GAP_DATA_VALUE, info->track_size-pos);

    return 0;
}



/* read_ctrl_track:
 *  Open the file and read the FD track.
 */
static int read_ctrl_track (const char filename [], struct DISK_INFO *info)
{
    int err = 0;
    FILE *file = NULL;
    long int file_seek = ((teo.disk[info->drive].side * info->track_count) + info->track)
                         * TEO_DISK_SECTOR_COUNT
                         * info->sector_size;

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, filename);

    if ((file = fopen (filename, "rb")) == NULL)
        return error_Message (TEO_ERROR_DISK_NONE, filename);

    if (fseek (file, file_seek, SEEK_SET) != 0)
        err = error_Message (TEO_ERROR_FILE_READ, filename);

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
 *  Convert and write the FD track (simple density).
 */
static int write_fm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    int sector;

    for (sector=1; sector<=16; sector++)
    {
        if ((i = disk_IsSDFloppySector (sector, info)) < 0)
            return TEO_ERROR;

        /* write the sector */
        if (fwrite (info->data+i+26, 1, (size_t)128, file) != (size_t)128)
            return error_Message (TEO_ERROR_DISK_IO, NULL);
    }
    return 0;
}



/* write_mfm_track:
 *  Convert and save the FD track (double density).
 */
static int write_mfm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    int sector;

    for (sector=1; sector<=16; sector++)
    {
        if ((i = disk_IsDDFloppySector (sector, info)) < 0)
            return TEO_ERROR;

        /* write the sector */
        if (fwrite (info->data+i+48, 1, (size_t)256, file) != (size_t)256)
            return error_Message (TEO_ERROR_DISK_IO, NULL);  /* error on sector */
    }
    return 0;
}



/* write_ctrl_track:
 *  Open the file and write the FD track.
 */
static int write_ctrl_track (const char filename [], struct DISK_INFO *info)
{
    int err = 0;
    FILE *file = NULL;
    long int file_seek = ((teo.disk[info->drive].side * info->track_count) + info->track)
                         * TEO_DISK_SECTOR_COUNT
                         * info->sector_size;

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



/* file_protection:
 *  Checks the FD format and returns the open mode.
 */
static int file_protection (const char filename[], int protection)
{
    FILE *file = NULL;
    size_t file_size;
    size_t name_length;
    
    protection = disk_CheckFile(filename, protection);

    if (protection >= 0)
    {
        /* check size of file */
        file = fopen (filename, "rb");
        fseek (file, 0, SEEK_END);
        file_size = ftell (file);
        (void)fclose(file);

        for (fd_type=0; file_size != fd_list[fd_type].file_size; fd_type++)
           if (fd_type == 7)
               return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        /* check name of file */
        name_length = strlen(filename);
        if (name_length < 3)
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        if (strcmp(filename+name_length-3, fd_list[fd_type].suffix) != 0)
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);
    }
    return protection;
}


/* ------------------------------------------------------------------------- */


/* fd_IsFd:
 *  Check if the file is a FD/QD file.
 */
int fd_IsFd (const char filename[])
{
    return file_protection (filename, FALSE);
}



/* fd_LoadDisk:
 *  Loads the FD archive into the specified drive and
 *  forces the read-only mode if necessary.
 */
int fd_LoadDisk(int drive, const char filename[])
{
    int protection;

    protection = file_protection (filename, teo.disk[drive].write_protect);

    if (protection >= 0)
    {
        /* check size of file */
        if (fd_list[fd_type].fat_size!= 160)
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        switch (fd_list[fd_type].sector_size)
        {
            case 128 :
                disk[drive].info->byte_rate = 125000/8;
                break;

            case 256 :
                disk[drive].info->byte_rate = 250000/8;
                break;
        }
        disk[drive].info->sector_size  = fd_list[fd_type].sector_size;
        disk[drive].info->fat_size     = fd_list[fd_type].fat_size;
        disk[drive].info->track_count  = fd_list[fd_type].track_count;
        disk[drive].info->track = -1;  /* force track to be loaded */

        /* update parameters */
        teo.disk[drive].file = std_free (teo.disk[drive].file);
        teo.disk[drive].file = std_strdup_printf ("%s", filename);
        teo.disk[drive].write_protect = protection;
        disk[drive].state = TEO_DISK_ACCESS_FD;
        disk[drive].ReadCtrlTrack = read_ctrl_track;
        disk[drive].WriteCtrlTrack = write_ctrl_track;
        disk[drive].ReadCtrlSector = NULL;
        disk[drive].WriteCtrlSector = NULL;
        disk[drive].FormatCtrlTrack = NULL;
        disk[drive].side_count = fd_list[fd_type].side_count;
    }
    return protection;
}

