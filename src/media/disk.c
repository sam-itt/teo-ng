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
 *  Module     : media/disk.c
 *  Version    : 1.8.4
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 15/09/2006 26/01/2010 12/01/2012 25/04/2012
 *                               29/09/2012 31/08/2013 23/08/2015 31/07/2016
 *               Samuel Devulder 05/02/2012 30/07/2011
 *
 *  Gestion des disquettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <dirent.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "main.h"
#include "hardware.h"
#include "errors.h"
#include "media/disk.h"
#include "media/disk/sap.h"
#include "media/disk/hfe.h"
#include "media/disk/fd.h"
#include "media/disk/daccess.h"

#if 0
#define DO_PRINT  1      /* if output wanted */
#endif

struct DISK_SIDE disk[NBDRIVE];
int dkcurr = 0;
static int track_written = 0;



/* is_dd_floppy_sector:
 *  Check if DD floppy sector is Thomson standard.
 *  Returns sector number or -1 if failure.
 */
static int is_dd_floppy_sector (int track, uint8 *data, uint8 *clck)
{
    int i;
    int crc0, crc1;

    /* check pre-synchro field of info */
    for (i=0; i<12; i++)
        if ((data[i] != MFM_PRE_SYNC_DATA_VALUE)
         || (clck[i] >= SYNCHRO_CLOCK_MARK))
            return -1;

    /* check data field of info */
    if ((data[16] != (uint8)track)
     || (data[17] != MFM_HEAD_NUMBER)
     || (data[18] < 1)
     || (data[18] > 16)
     || (data[19] != MFM_SIZE_ID))
        return -1;

    /* check clock field of info */
    for (i=16; i<21; i++)
        if (clck[i] >= SYNCHRO_CLOCK_MARK)
            return -1;

    /* check crc */
    crc0 = disk_ComputeCrc (data+16, 4, MFM_CRC_INFO_INIT);
    crc1 = (int)((data[20]<<8) | data[21]);
    if (crc0 != crc1)
        return -1;

    /* check gap field of sector */
    for (i=22; i<44; i++)
        if ((clck[i]>=SYNCHRO_CLOCK_MARK)
         || (data[i] != MFM_GAP_DATA_VALUE))
            return -1;

    /* check pre-synchro field of sector */
    for (i=44; i<56; i++)
        if ((clck[i]>=SYNCHRO_CLOCK_MARK)
         || (data[i] != MFM_PRE_SYNC_DATA_VALUE))
            return -1;

    /* check synchro field of sector */
    for (i=56; i<59; i++)
        if ((clck[i]<SYNCHRO_CLOCK_MARK)
         || (data[i] != MFM_SYNCHRO_DATA_VALUE))
            return -1;

    /* check clock of sector field */
    for (i=59; i<318; i++)
        if (clck[i]>=SYNCHRO_CLOCK_MARK)
            return -1;

    /* check data of info field */
    if (data[59] != MFM_SECTOR_ID)
        return -1;

    /* check CRC */
    crc0 = disk_ComputeCrc (data+60, 256, MFM_CRC_DATA_INIT);
    crc1 = (int)((data[316]<<8) | data[317]);
    if (crc0 != crc1)
        return -1;

    /* return sector number */
    return (int)data[18];
}



/* is_sd_floppy_sector:
 *  Check if SD floppy info is Thomson standard.
 *  Returns sector number or -1 if failure.
 */
static int is_sd_floppy_sector (int track, uint8 *data, uint8 *clck)
{
    int i;
    int crc0, crc1;

    /* check pre-synchro field */
    for (i=0; i<6; i++)
        if ((clck[i] >= SYNCHRO_CLOCK_MARK)
         || (data[i] != FM_PRE_SYNC_DATA_VALUE))
            return -1;

    /* check clock of info field */
    for (i=7; i<15; i++)
        if (clck[i] >= SYNCHRO_CLOCK_MARK)
            return -1;

    /* check data of info field */
    if ((data[7] != (uint8)track)
     || (data[8] != FM_HEAD_NUMBER)
     || (data[9] < 1)
     || (data[9] > 16)
     || (data[10] != FM_SIZE_ID))
        return -1;

    crc0 = disk_ComputeCrc (data+7, 4, FM_CRC_INFO_INIT);
    crc1 = (int)((data[11]<<8) | data[12]);
    if (crc1 != crc0)
        return -1;
     
    /* check gap field of sector */
    for (i=13; i<25; i++)
        if ((clck[i]>=SYNCHRO_CLOCK_MARK)
         || (data[i] != FM_GAP_DATA_VALUE))
            return -1;

    /* check pre-synchro field */
    for (i=25; i<31; i++)
        if ((clck[i]>=SYNCHRO_CLOCK_MARK)
         || (data[i] != FM_PRE_SYNC_DATA_VALUE))
            return -1;

    /* check synchro field */
    if ((clck[31]<SYNCHRO_CLOCK_MARK)
     || (data[31] != FM_SECTOR_ID))
            return -1;

    /* check clock of sector field */
    for (i=32; i<162; i++)
        if (clck[i]>=SYNCHRO_CLOCK_MARK)
            return -1;

    /* check CRC */
    crc0 = disk_ComputeCrc (data+32, 128, FM_CRC_DATA_INIT);
    crc1 = (int)((data[160]<<8) | data[161]);
    if (crc1 != crc0)
        return -1;

    /* return sector number */
    return (int)data[9];
}



/* disk_DiskVectorLast:
 *  Renvoit le pointeur sur le dernier élément de la stringlist.
 */
static struct DISK_VECTOR *disk_DiskVectorLast (struct DISK_VECTOR *p)
{
    while ((p!=NULL) && (p->next!=NULL))
        p=p->next;

    return p;
}



/* clear_track:
 *  Clear the current track so that the reading will generate an I/O error.
 */
static void clear_track (int drive)
{
    memset (disk[drive].data, 0x00, TEO_DISK_TRACK_SIZE_MAX);
    memset (disk[drive].clck, 0x00, TEO_DISK_TRACK_SIZE_MAX);
}



/* drv_alloc:
 *  Allocate a disk drive structure.
 */
static struct DISK_DRIVE *drv_alloc (void)
{
    struct DISK_DRIVE *drv;

    drv = malloc (sizeof(struct DISK_DRIVE));
    if (drv == NULL)
    {
        return NULL;
    }
    memset (drv, 0x00, sizeof (struct DISK_DRIVE));

    /* Reset loading */
    drv->track.curr = 0;
    drv->track.last = TEO_DISK_INVALID_NUMBER;

    return drv;
}



/* dkc_alloc:
 *  Allocate a disk controller structure.
 */
static struct DISK_CONTROLLER *dkc_alloc (void)
{
    struct DISK_CONTROLLER *dkc;

    dkc = malloc (sizeof(struct DISK_CONTROLLER));
    if (dkc == NULL)
    {
        return NULL;
    }
    memset (dkc, 0x00, sizeof (struct DISK_CONTROLLER));

    return dkc;
}



/* alloc_controller:
 *  Allocate a controller structure.
 */
static int alloc_controller (int controller)
{
    struct DISK_CONTROLLER *dkc;
    struct DISK_DRIVE *drv;

    dkc = dkc_alloc ();
    if (dkc == NULL)
    {
        return FALSE;
    }
    disk[controller*4+0].dkc = dkc;
    disk[controller*4+1].dkc = dkc;
    disk[controller*4+2].dkc = dkc;
    disk[controller*4+3].dkc = dkc;

    drv = drv_alloc ();
    if (drv == NULL)
    {
        return FALSE;
    }
    disk[controller*4+0].drv = drv;
    disk[controller*4+1].drv = drv;

    drv = drv_alloc ();
    if (drv == NULL)
    {
        return FALSE;
    }
    disk[controller*4+2].drv = drv;
    disk[controller*4+3].drv = drv;

    return TRUE;
}



/* free_controller:
 *  Free a controller memory.
 */
static void free_controller (int controller)
{
    std_free (disk[controller*4+0].dkc);
    std_free (disk[controller*4+0].drv);
    std_free (disk[controller*4+2].drv);
}    


/* ------------------------- Sector management ---------------------------- */


void disk_CreateDDFloppySector (int track, int sector, uint8 *sector_buffer,
                                uint8 *data, uint8 *clck)
{
    int crc;

    /* write start of sector field */
    memset (data, MFM_GAP_DATA_VALUE, 32);
    memset (data+32, MFM_PRE_SYNC_DATA_VALUE, 12);
    memset (data+44, MFM_SYNCHRO_DATA_VALUE, 3);
    memset (clck+44, SYNCHRO_CLOCK_MARK, 3);
    *(data+47) = MFM_INFO_ID;

    /* write sector field */
    *(data+48) = track;
    *(data+49) = MFM_HEAD_NUMBER;
    *(data+50) = sector;
    *(data+51) = MFM_SIZE_ID;
    crc = disk_ComputeCrc (data+48, 4, MFM_CRC_INFO_INIT);
    *(data+52) = (uint8)(crc>>8);
    *(data+53) = (uint8)crc;

    /* write start of sector field */
    memset (data+54, MFM_GAP_DATA_VALUE, 22);
    memset (data+76, MFM_PRE_SYNC_DATA_VALUE, 12);
    memset (data+88, MFM_SYNCHRO_DATA_VALUE, 3);
    memset (clck+88, SYNCHRO_CLOCK_MARK, 3);
    *(data+91) = MFM_SECTOR_ID;

    /* copy the sector */
    memcpy (data+92, sector_buffer, 256);
    
    /* update data crc */
    crc = disk_ComputeCrc (data+92, 256, MFM_CRC_DATA_INIT);
    *(data+348)   = (uint8)(crc>>8);
    *(data+349) = (uint8)crc;

    /* write sector gap */
    memset (data+350, MFM_GAP_DATA_VALUE, 12);
}



void disk_CreateSDFloppySector (int track, int sector, uint8 *sector_buffer,
                                uint8 *data, uint8 *clck)
{
    int crc;

    /* write start of sector field */
    memset (data, FM_GAP_DATA_VALUE, 16);
    memset (data+16, FM_PRE_SYNC_DATA_VALUE, 6);
    *(clck+22) = SYNCHRO_CLOCK_MARK;
    *(data+22) = FM_INFO_ID;

    /* write sector field */
    *(data+23) = track;
    *(data+24) = FM_HEAD_NUMBER;
    *(data+25) = sector;
    *(data+26) = FM_SIZE_ID;
    crc = disk_ComputeCrc (data+23, 4, FM_CRC_INFO_INIT);
    *(data+27) = (uint8)(crc>>8);
    *(data+28) = (uint8)crc;

    /* write start of sector field */
    memset (data+29, FM_GAP_DATA_VALUE, 12);
    memset (data+31, FM_PRE_SYNC_DATA_VALUE, 6);
    *(clck+37) = SYNCHRO_CLOCK_MARK;
    *(data+37) = FM_SECTOR_ID;

    /* copy the sector */
    memcpy (data+38, sector_buffer, 128);

    /* update data crc */
    crc = disk_ComputeCrc (data+38, 128, FM_CRC_DATA_INIT);
    *(data+166) = (uint8)(crc>>8);
    *(data+167) = (uint8)crc;

    /* write sector gap */
    memset (data+168, MFM_GAP_DATA_VALUE, 6);
}


/* --------------------------- Disk Controller ---------------------------- */


/* disk_Written:
 *  Activate the disk written flag.
 */
void disk_Written (void)
{
    track_written = 1;
}



/* disk_WriteTrack:
 *  Write the track onto disk if track memory has been written.
 */
void disk_WriteTrack (void)
{
    if ((disk[dkcurr].WriteTrack != NULL)
     && (track_written != 0)
     && (disk[dkcurr].drv->track.last < disk[dkcurr].track_count))
    {
#ifdef DO_PRINT
        printf ("disk_WriteTrack drive %d track %d\n",
            dkcurr,
            disk[dkcurr].drv->track.last);
        fflush (stdout);
#endif
        (void)disk[dkcurr].WriteTrack (dkcurr, disk[dkcurr].drv->track.last);
    }
    disk[dkcurr].drv->track.last = disk[dkcurr].drv->track.curr;
    track_written = 0;
}



/* disk_WriteTimeout:
 *  Force to update the track if it has been written (no access left).
 */
void disk_WriteTimeout (void)
{
    if (disk[dkcurr].dkc->StillRunning () == FALSE)
    {
        if (track_written != 0)
        {
#ifdef DO_PRINT
            printf ("disk_WriteTimeout drive %d track %d (%d)\n",
                dkcurr,
                disk[dkcurr].drv->track.curr,
                disk[dkcurr].drv->track.last);
            fflush (stdout);
#endif
            disk_WriteTrack ();
        }
        disk[dkcurr].drv->track.last = TEO_DISK_INVALID_NUMBER;
    }
}



/* disk_ReadTrack:
 *  Update the track if it has been written and load the new track.
 */
void disk_ReadTrack (int drive)
{
    if ((dkcurr != drive)
     || (disk[drive].drv->track.curr != disk[drive].drv->track.last))
    {
        disk_WriteTrack ();

        if (disk[drive].ReadTrack != NULL)
        {
#ifdef DO_PRINT
            printf ("disk_ReadTrack drive %d track %d (%d)\n",
                drive,
                disk[drive].drv->track.curr,
                disk[drive].drv->track.last);
            fflush (stdout);
#endif
            (void)disk[drive].ReadTrack (drive, disk[drive].drv->track.curr);
        }
    }
    /* Reset loading */
    disk[drive].drv->track.last = disk[drive].drv->track.curr;
    dkcurr = drive;
}



/* disk_WriteSector:
 *  Write a sector (direct access only).
 */
void disk_WriteSector (int drive)
{
    if (disk[drive].WriteSector != NULL)
    {
        disk_WriteTrack ();

        disk[drive].drv->sector = LOAD_BYTE(0x604C);
#ifdef DO_PRINT
        printf ("disk_WriteSector drive %d track %d sector %d\n",
             drive,
             disk[drive].drv->track.curr,
             disk[drive].drv->sector);
        fflush (stdout);
#endif
        (void)disk[dkcurr].WriteSector (drive, LOAD_WORD(0x604F));
    }
    /* Reset loading */
    disk[drive].drv->track.last = disk[drive].drv->track.curr;
    dkcurr = drive;
}



/* disk_ReadSector:
 *  Load a sector (direct access only).
 */
void disk_ReadSector (int drive)
{
    int err = 0;

    if (disk[drive].ReadSector != NULL)
    {
        disk_WriteTrack ();
    
        disk[drive].drv->sector = LOAD_BYTE(0x604C);
#ifdef DO_PRINT
        printf ("disk_ReadSector drive %d track %d (%d) sector %d\n",
                drive,
                disk[drive].drv->track.curr,
                disk[drive].drv->track.last,
                disk[drive].drv->sector);
        fflush (stdout);
#endif
        err = disk[drive].ReadSector (drive);
        if (err < 0)
        {
            clear_track(drive);
        }
    }
    /* Reset loading */
    disk[drive].drv->track.last = disk[drive].drv->track.curr;
    dkcurr = drive;
}



/* disk_FormatTrack:
 *  Update the track if it has been formatted (direct access only).
 */
int disk_FormatTrack (int drive)
{
    int err = 0;

    if (disk[drive].FormatTrack != NULL)
    {
        disk_WriteTrack ();

#ifdef DO_PRINT
        printf ("disk_FormatTrack track %d drive %d\n",
                disk[drive].drv->track.last,
                drive);
        fflush (stdout);
#endif
        err = disk[drive].FormatTrack (drive);
    }
    disk[drive].drv->track.last = disk[drive].drv->track.curr;
    dkcurr = drive;
    return err;
}


/* ---------------------------- Disk Vectors ------------------------------ */


/* disk_DiskVectorIndex:
 *  Renvoit l'index de l'élément du DISK_VECTOR.
 */
int disk_DiskVectorIndex (struct DISK_VECTOR *p, const char str[])
{
    int index;

    for (index=0; p!=NULL; p=p->next,index++)
        if (p->str!=NULL)
            if (strcmp (p->str, str) == 0)
                break;
    return (p==NULL)?-1:index;
}



/* disk_DiskVectorLength:
 *  Renvoit le nombre d'éléments du DISK_VECTOR.
 */
int disk_DiskVectorLength (struct DISK_VECTOR *p)
{
    int i = 0;

    while (p!=NULL)
    {
        i++;
        p=p->next;
    }

    return i;
}



/*  disk_DiskVectorPtr:
 *  Renvoit le pointeur de l'élément DISK_VECTOR.
 */
struct DISK_VECTOR *disk_DiskVectorPtr (struct DISK_VECTOR *p, int index)
{
    for (;index>0;index--)
        if (p!=NULL)
            p=p->next;
    return p;
}



/*  disk_DiskVectorText:
 *  Renvoit le pointeur du texte de l'élément du DISK_VECTOR.
 */
char *disk_DiskVectorText (struct DISK_VECTOR *p, int index)
{
    p = disk_DiskVectorPtr (p, index);
    return (p!=NULL)?p->str:NULL;
}



/* disk_DiskVectorAppend:
 *  Ajoute un élément au DISK_VECTOR.
 */
struct DISK_VECTOR *disk_DiskVectorAppend (struct DISK_VECTOR *p,
                      const char str[], int side, int side_count,
                      int write_protect)
{
    struct DISK_VECTOR *last_str = disk_DiskVectorLast (p);
    struct DISK_VECTOR *new_str = NULL;
    
    if (str != NULL)
    {
        new_str = calloc (1, sizeof (struct DISK_VECTOR));

        if (new_str!=NULL)
        {
            new_str->str = std_strdup_printf ("%s", str);
            new_str->side = side;
            new_str->side_count = side_count;
            new_str->write_protect = write_protect;
        }

        if ((last_str!=NULL) && (last_str->str!=NULL))
            last_str->next=new_str;
    }
    return (p==NULL)?new_str:p;
}



/* std_StringListFree:
 *  Libère la mémoire du DISK_VECTOR.
 */
void disk_DiskVectorFree (struct DISK_VECTOR *p)
{
    struct DISK_VECTOR *next;

    while (p!=NULL)
    {
        next=p->next;
        if (p->str!=NULL)
            free (p->str);
        free (p);
        p=next;
    }
}


/* -------------------------- Low level functions ------------------------- */


/* disk_ComputeCrc:
 *  Compute the Thomson CRC of a memory range.
 */
int disk_ComputeCrc (uint8 *buffer, int length, int start_value)
{
    int i;
    int crc_high = (start_value >> 8) & 0xff;
    int crc_low  = start_value & 0xff;
    int c;

    for (i=0; i<length; i++)
    {
        c = (crc_high ^ (int)*(buffer++)) & 0xff;
        c ^= (c >> 4);
        crc_low ^= (c >> 3);
        crc_high = ((c << 4) ^ crc_low) & 0xff;
        crc_low = ((c << 5) ^ c) & 0xff;
    }
    return (crc_high << 8) | crc_low;
}



/* disk_CheckFile:
 *  Teste la présence du fichier et le mode d'accès.
 */
int disk_CheckFile (const char filename[], int protection)
{
    FILE *file;

    if ((file = fopen (filename, "rb+")) == NULL)
    {
        protection = TRUE;
        if ((file = fopen (filename, "rb")) == NULL)
            protection = error_Message(TEO_ERROR_FILE_OPEN, filename);
    }
    file = std_fclose (file);

    return protection;
}



/* disk_Protection:
 *  Fixe le mode d'accès à la disquette.
 *  (lecture seule ou lecture/écriture)
 */ 
int disk_Protection (int drive, int protection)
{
    switch (disk[drive].state)
    {
        case TEO_DISK_ACCESS_NONE:
            break;

        case TEO_DISK_ACCESS_DIRECT:
            if (!teo_DirectWriteSector)
                protection = TRUE;  
            break;

        default:
            if (teo.disk[drive].write_protect != protection)
                protection = disk_CheckFile(teo.disk[drive].file, protection);
            break;
    }

    if (protection >= 0)
    {
        /* manage disk protection by function */
        if ((disk[drive].IsWritable != NULL)
         && (disk[drive].IsWritable (drive) == 0))
            protection = TRUE;

        /* manage disk protection by flag */
        if (disk[drive].write_protect == TRUE)
            protection = TRUE;
    }

    return protection;
}



/* disk_IsDDFloppyInfo:
 *  Find the wanted DD sector info in raw track.
 */
int disk_IsDDFloppySector (int track, int sector)
{
    int i = 12;

    while (i <= (disk[dkcurr].track_size - MFM_SECTOR_SIZE))
    {
        if ((disk[dkcurr].data[i] == MFM_SYNCHRO_DATA_VALUE)
         && (disk[dkcurr].clck[i] >= SYNCHRO_CLOCK_MARK)
         && (disk[dkcurr].data[i+1] == MFM_SYNCHRO_DATA_VALUE)
         && (disk[dkcurr].clck[i+1] >= SYNCHRO_CLOCK_MARK)
         && (disk[dkcurr].data[i+2] == MFM_SYNCHRO_DATA_VALUE)
         && (disk[dkcurr].clck[i+2] >= SYNCHRO_CLOCK_MARK)
         && (disk[dkcurr].data[i-1] == MFM_PRE_SYNC_DATA_VALUE)
         && (disk[dkcurr].clck[i-1] < SYNCHRO_CLOCK_MARK)
         && (disk[dkcurr].data[i+3] == MFM_INFO_ID)
         && (disk[dkcurr].clck[i+3] < SYNCHRO_CLOCK_MARK))
        {
            if (is_dd_floppy_sector (track,
                                     disk[dkcurr].data+i-12,
                                     disk[dkcurr].clck+i-12) == sector)
                return i;
            i += 256;    /* skip a little bit less than a sector */
        }
        i++;
    }
    return -1;
}



/* disk_IsSDFloppySector:
 *  Find the wanted SD sector info in raw track.
 */
int disk_IsSDFloppySector (int track, int sector)
{
    int i = 6;

    while (i <= (disk[dkcurr].track_size - FM_SECTOR_SIZE))
    {
        if ((disk[dkcurr].data[i] == FM_INFO_ID)
         && (disk[dkcurr].clck[i] >= SYNCHRO_CLOCK_MARK)
         && (disk[dkcurr].data[i+1] == SYNCHRO_CLOCK_MARK)
         && (disk[dkcurr].clck[i+1] < SYNCHRO_CLOCK_MARK)
         && (disk[dkcurr].data[i-1] == FM_PRE_SYNC_DATA_VALUE)
         && (disk[dkcurr].clck[i-1] < SYNCHRO_CLOCK_MARK))
        {
            if (is_sd_floppy_sector (
                track,
                disk[dkcurr].data+i-6,
                disk[dkcurr].clck+i-6) == sector)
                return i;
            i += 128;    /* skip a little bit less than a sector */
        }
        i++;
    }
    return -1;
}



/* disk_Eject:
 *  Ejecte l'archive du le lecteur spécifié.
 */
void disk_Eject(int drive)
{
    /* update last write eventually */
    disk_WriteTrack ();

    teo.disk[drive].file = std_free (teo.disk[drive].file);
    teo.disk[drive].side = 0;
    disk[drive].state = TEO_DISK_ACCESS_NONE;
    disk[drive].write_protect = FALSE;
    disk[drive].ReadTrack = NULL;
    disk[drive].WriteTrack = NULL;
    disk[drive].ReadSector = NULL;
    disk[drive].WriteSector = NULL;
    disk[drive].FormatTrack = NULL;
    disk[drive].IsWritable = NULL;
    disk[drive].drv->track.last = TEO_DISK_INVALID_NUMBER;
}



/* disk_Load:
 *  Load a disk.
 */
int disk_Load (int drive, const char filename[])
{
    if ((sap_LoadDisk(drive, filename) < 0)
     && (hfe_LoadDisk(drive, filename) < 0)
     && (fd_LoadDisk(drive, filename) < 0))
        return TEO_ERROR;

    return 0;
    (void)drive;
}



/* disk_FirstLoad:
 *  First load of disks.
 */
void disk_FirstLoad (void)
{
    int drive;
    char *name = NULL;

    for (drive=0; drive<NBDRIVE; drive++)
    {
        /* charge la disquette */
        if ((teo.disk[drive].file != NULL)
         && (*teo.disk[drive].file != '\0'))
        {
            name = teo.disk[drive].file;
            teo.disk[drive].file = NULL;
            if (disk_Load (drive, name) < 0)
            {
                main_DisplayMessage (teo_error_msg);
                disk_Eject(drive);
            }
            name = std_free (name);
        }
    }
}



/* disk_FillAllTracks:
 *  Load the tracks at beginning.
 */
void disk_FillAllTracks (void)
{
    int drive;

    for (drive=0; drive<NBDRIVE; drive++)
    {
        disk[drive].drv->track.last = disk[drive].drv->track.curr;
        if ((disk[drive].ReadTrack != NULL)
         && (disk[drive].drv->track.curr < disk[drive].track_count))
        {
            if ((disk[drive].state == TEO_DISK_ACCESS_SAP)
             || (disk[drive].state == TEO_DISK_ACCESS_HFE)
             || (disk[drive].state == TEO_DISK_ACCESS_FD))
            {
                disk[drive].ReadTrack (drive, disk[drive].drv->track.curr);
            }
        }
    }
}
            
        

/* disk_Free:
 *  Free disk structures.
 */
void disk_Free (void)
{
    int i;

    /* update last write eventually */
    disk_WriteTrack();

    /* free track buffers */
    std_free (disk[0].data);
    std_free (disk[0].clck);
    
    /* free controller */
    for (i=0; i<(NBDRIVE/4); i++)
    {
        free_controller (i);
    }
}    



/* disk_Init:
 *  Allocate and initialize disk structures.
 */
int disk_Init (void)
{
    int i;
    uint8 *data;
    uint8 *clck;

    /* clear main structure */
    memset (&disk, 0x00, sizeof(struct DISK_SIDE)*NBDRIVE);

    /* allocate data track */
    data = malloc (TEO_DISK_TRACK_SIZE_MAX);
    if (data == NULL)
        return error_Message (TEO_ERROR_ALLOC, NULL);
    memset (data, 0x00, TEO_DISK_TRACK_SIZE_MAX);

    /* allocate clock track */
    clck = malloc (TEO_DISK_TRACK_SIZE_MAX);
    if (clck == NULL)
        return error_Message (TEO_ERROR_ALLOC, NULL);
    memset (clck, 0x00, TEO_DISK_TRACK_SIZE_MAX);

    /* initialize variables */
    for (i=0; i<NBDRIVE; i++)
    {
        disk[i].byte_rate = TEO_DISK_DD_BYTE_RATE;
        disk[i].sector_size = TEO_DISK_DD_SECTOR_SIZE;
        disk[i].track_size = TEO_DISK_DD_TRACK_SIZE;
        disk[i].track_count = TEO_DISK_DD_TRACK_NUMBER;
        disk[i].drive = i;
        disk[i].state = TEO_DISK_ACCESS_NONE;
        disk[i].side_count = 1;
        disk[i].write_protect = TRUE;
        disk[i].data = data;
        disk[i].clck = clck;
    }

    /* allocate and initialize controller */
    for (i=0; i<(NBDRIVE/4); i++)
    {
        if (alloc_controller (i) == FALSE)
            return error_Message (TEO_ERROR_ALLOC, NULL);

        thmfc1_Init (i);
    }
    return 0;
}    

