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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 21/01/2013
 *  Modifié par: François Mouret 23/08/2015 31/07/2016
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
#include "media/disk.h"

#if 0
#define DO_PRINT  1      /* if output wanted */
#endif 

struct FD_LIST {
    size_t file_size;
    char suffix[4];
    int side_count;
    int track_count;
    int fat_size;
    int sector_size;
};

static struct FD_LIST fd_list[] = {
    {   51200, ".qd", 1, 25,  50, TEO_DISK_SD_SECTOR_SIZE },
    {   51200, ".qd", 1, 25,  50, TEO_DISK_SD_SECTOR_SIZE },
    {   81920, ".fd", 1, 40,  80, TEO_DISK_SD_SECTOR_SIZE },
    {  163840, ".fd", 1, 40,  80, TEO_DISK_DD_SECTOR_SIZE },
    {  327680, ".fd", 1, 80, 160, TEO_DISK_DD_SECTOR_SIZE },
    {  655360, ".fd", 2, 80, 160, TEO_DISK_DD_SECTOR_SIZE },
    {  983040, ".fd", 3, 80, 160, TEO_DISK_DD_SECTOR_SIZE },
    { 1310720, ".fd", 4, 80, 160, TEO_DISK_DD_SECTOR_SIZE }
};

static int fd_type = 3;
static uint8 sector_buffer[TEO_DISK_DD_SECTOR_SIZE];



#ifdef DO_PRINT
/* display_track :
 *  Display a track
 */
static void display_track (char *message, int drive, int track)
{
    int i;

    printf ("---------------------------------------\n");
    printf ("FD %s drive %d track %d\n", message, drive, track);
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



#define FILE_SEEK (long int)(((teo.disk[drive].side \
                             * disk[drive].track_count) \
                             + track) \
                             * TEO_DISK_SECTOR_PER_TRACK \
                             * disk[drive].sector_size)

static int fd_error (int drive, int error, FILE *file)
{
    if (file != NULL)
        fclose (file);
    return error_Message (error, teo.disk[drive].file);
}



/* read_fm_track:
 *  Read and convert the FD track (simple density).
 */
static int read_fm_track (int drive, int track)
{
    FILE *file;
    int pos;
    int sector;
    size_t secsiz = (size_t)TEO_DISK_SD_SECTOR_SIZE;

    /* initialize track */
    memset (disk[drive].data, FM_GAP_DATA_VALUE, TEO_DISK_TRACK_SIZE_MAX);
    memset (disk[drive].clck, DATA_CLOCK_MARK, TEO_DISK_TRACK_SIZE_MAX);

    /* read-open FD file */
    if ((file = fopen (teo.disk[drive].file, "rb")) == NULL)
        return fd_error (drive, TEO_ERROR_DISK_NONE, file);
    
    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return fd_error (drive, TEO_ERROR_FILE_READ, file);

    /* read sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* compute sector offset (interleave 7) */
        pos = SDSECTORPOS (sector);

        /* read the sector */
        if (fread (sector_buffer, 1, secsiz, file) != secsiz)
            return fd_error (drive, TEO_ERROR_FILE_READ, file);

        /* create sector */
        disk_CreateSDFloppySector (
            disk[drive].drv->track.curr,
            sector,
            sector_buffer,
            disk[drive].data+pos,
            disk[drive].clck+pos);
    }
    fclose (file);
#ifdef DO_PRINT
    display_track ("read_fm_track", drive, track);
#endif
    return 0;
}



/* read_mfm_track:
 *  Read and convert the FD track (double density).
 */
static int read_mfm_track (int drive, int track)
{
    FILE *file;
    int pos;
    int sector;
    size_t secsiz = (size_t)TEO_DISK_DD_SECTOR_SIZE;

    /* initialize track */
    memset (disk[drive].data, MFM_GAP_DATA_VALUE, TEO_DISK_TRACK_SIZE_MAX);
    memset (disk[drive].clck, DATA_CLOCK_MARK, TEO_DISK_TRACK_SIZE_MAX);

    /* read-open FD file */
    if ((file = fopen (teo.disk[drive].file, "rb")) == NULL)
        return fd_error (drive, TEO_ERROR_DISK_NONE, file);

    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return fd_error (drive, TEO_ERROR_FILE_READ, file);

    /* read sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* compute sector offset (interleave 7) */
        pos = DDSECTORPOS (sector);

        /* read the sector */
        if (fread (sector_buffer, 1, secsiz, file) != secsiz)
            return fd_error (drive, TEO_ERROR_FILE_READ, file);

        /* create sector */
        disk_CreateDDFloppySector (
            disk[drive].drv->track.curr,
            sector,
            sector_buffer,
            disk[drive].data+pos,
            disk[drive].clck+pos);
    }
    fclose (file);
#ifdef DO_PRINT
    display_track ("read_mfm_track", drive, track);
#endif
    return 0;
}



/* write_fm_track:
 *  Convert and write the FD track (simple density).
 */
static int write_fm_track (int drive, int track)
{
    FILE *file;
    int i;
    int sector;
    size_t secsiz = (size_t)TEO_DISK_SD_SECTOR_SIZE;

    /* write-open FD file */
    if ((file = fopen (teo.disk[drive].file, "rb+")) == NULL)
        return fd_error (drive, TEO_ERROR_DISK_NONE, file);
    
    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return fd_error (drive, TEO_ERROR_FILE_WRITE, file);

    /* write sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* write the sector */
        if ((i = disk_IsSDFloppySector (track, sector)) >= 0)
            if (fwrite (disk[drive].data+i+26, 1, secsiz, file) != secsiz)
                return fd_error (drive, TEO_ERROR_FILE_WRITE, file);
    }
    fclose (file);
#ifdef DO_PRINT
    display_track ("write_fm_track", drive, track);
#endif
    return 0;
}



/* write_mfm_track:
 *  Convert and save the FD track (double density).
 */
static int write_mfm_track (int drive, int track)
{
    FILE *file;
    int i;
    int sector;
    size_t secsiz = (size_t)TEO_DISK_DD_SECTOR_SIZE;

    /* write-open FD file */
    if ((file = fopen (teo.disk[drive].file, "rb+")) == NULL)
        return fd_error (drive, TEO_ERROR_DISK_NONE, file);
    
    if (fseek (file, FILE_SEEK, SEEK_SET) != 0)
        return fd_error (drive, TEO_ERROR_FILE_WRITE, file);

    /* write sectors */    
    for (sector=1; sector<=16; sector++)
    {
        /* write the sector */
        if ((i = disk_IsDDFloppySector (track, sector)) >= 0)
            if (fwrite (disk[drive].data+i+48, 1, secsiz, file) != secsiz)
                return fd_error (drive, TEO_ERROR_FILE_WRITE, file);
    }
    fclose (file);
#ifdef DO_PRINT
    display_track ("write_mfm_track", drive, track);
#endif
    return 0;
}



/* file_protection:
 *  Checks the FD format and returns the open mode.
 */
static int file_protection (const char filename[], int protection)
{
    size_t file_size;
    size_t name_length;
    
    protection = disk_CheckFile(filename, protection);

    if (protection >= 0)
    {
        /* check size of file */
        file_size = std_FileSize(filename);
        for (fd_type=0; file_size != fd_list[fd_type].file_size; fd_type++)
           if (fd_type == 7)
               return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        /* check name of file */
        name_length = strlen(filename);
        if (name_length < 3)
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        if (strcmp(filename+name_length-3, fd_list[fd_type].suffix) != 0)
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        /* check number_of_tracks */
        if (fd_list[fd_type].track_count != TEO_DISK_TRACK_NUMBER_MAX)
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
int fd_LoadDisk (int drive, const char filename[])
{
    int protection;

    protection = file_protection (filename, teo.disk[drive].write_protect);

    if (protection >= 0)
    {
        /* write-update the track of the current drive */
        disk_WriteTrack ();

        teo.disk[drive].file = std_free (teo.disk[drive].file);
        teo.disk[drive].file = std_strdup_printf ("%s", filename);
        teo.disk[drive].write_protect = protection;
        switch (fd_list[fd_type].sector_size)
        {
            case TEO_DISK_SD_SECTOR_SIZE :
                disk[drive].byte_rate = TEO_DISK_SD_BYTE_RATE;
                disk[drive].track_size = TEO_DISK_SD_TRACK_SIZE;
                disk[drive].ReadTrack = read_fm_track;
                disk[drive].WriteTrack = write_fm_track;
                break;

            case TEO_DISK_DD_SECTOR_SIZE :
                disk[drive].byte_rate = TEO_DISK_DD_BYTE_RATE;
                disk[drive].track_size = TEO_DISK_DD_TRACK_SIZE;
                disk[drive].ReadTrack = read_mfm_track;
                disk[drive].WriteTrack = write_mfm_track;
                break;
        }
        disk[drive].sector_size  = fd_list[fd_type].sector_size;
        disk[drive].track_count  = fd_list[fd_type].track_count;
        disk[drive].drv->track.curr = 0;
        disk[drive].drv->track.last = TEO_DISK_INVALID_NUMBER;
        disk[drive].state = TEO_DISK_ACCESS_FD;
        disk[drive].write_protect = FALSE;
        disk[drive].ReadSector = NULL;
        disk[drive].WriteSector = NULL;
        disk[drive].FormatTrack = NULL;
        disk[drive].IsWritable = NULL;
        disk[drive].side_count = fd_list[fd_type].side_count;
    }
    return protection;
}

