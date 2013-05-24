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
 *  Module     : media/disk.c
 *  Version    : 1.8.3
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 15/09/2006 26/01/2010 12/01/2012 25/04/2012
 *                               29/09/2012
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
#include "media/disk/controlr.h"
#include "media/disk.h"
#include "media/disk/sap.h"
#include "media/disk/hfe.h"
#include "media/disk/fd.h"
#include "media/disk/daccess.h"

/* paramètres physiques des lecteurs Thomson */
#define NBTRACK   80
#define NBSECT    16
#define SECTSIZE 256

struct DISK_CONTROLLER *dkc;

DISK_PARAMETER disk[NBDRIVE];
struct DISK_CONTROLLER *thmfc1_controller;



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

    /* check synchro field of sector*/
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

    crc0 = disk_ComputeCrc (data+60, 256, MFM_CRC_DATA_INIT);
    crc1 = (int)((data[316]<<8) | data[317]);
    if (crc0 != crc1)
        return -1;

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

    crc0 = disk_ComputeCrc (data+32, 128, FM_CRC_DATA_INIT);
    crc1 = (int)((data[160]<<8) | data[161]);
    if (crc1 != crc0)
        return -1;

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


/* ------------------------------------------------------------------------- */


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



/* disk_ComputeCrc:
 *  Calcule le CRC Thomson d'une zone.
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



/* disk_BuildSectorMap:
 *  Construit la carte des secteurs d'une piste en fonction
 *  du facteur d'entrelacement.
 */
void disk_BuildSectorMap (int *sector_map, int factor)
{
    int sector;
    int i=0;

    /* mise à zéro de la table */
    memset(sector_map, 0, sizeof(int)*NBSECT);

    for (sector=1; sector<=NBSECT; sector++)
    {
        while (sector_map[i] != 0)
            i=(i+1)%NBSECT;

        sector_map[i]=sector;

        i=(i+factor)%NBSECT;
    }
}



/* disk_Eject:
 *  Ejecte l'archive du le lecteur spécifié.
 */
void disk_Eject(int drive)
{
    teo.disk[drive].file = std_free (teo.disk[drive].file);
    teo.disk[drive].side = 0;
    disk[drive].state = TEO_DISK_ACCESS_NONE;
    disk[drive].write_protect = FALSE;
    disk[drive].ReadCtrlTrack = NULL;
    disk[drive].WriteCtrlTrack = NULL;
    disk[drive].ReadCtrlSector = NULL;
    disk[drive].WriteCtrlSector = NULL;
    disk[drive].FormatCtrlTrack = NULL;
    disk[drive].IsWritable = NULL;
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
int disk_IsDDFloppySector (int sector, struct DISK_INFO *info)
{
    int i = 12;

    while (i <= (info->track_size - MFM_SECTOR_SIZE))
    {
        if ((info->data[i] == MFM_SYNCHRO_DATA_VALUE)
         && (info->clck[i] >= SYNCHRO_CLOCK_MARK)
         && (info->data[i+1] == MFM_SYNCHRO_DATA_VALUE)
         && (info->clck[i+1] >= SYNCHRO_CLOCK_MARK)
         && (info->data[i+2] == MFM_SYNCHRO_DATA_VALUE)
         && (info->clck[i+2] >= SYNCHRO_CLOCK_MARK)
         && (info->data[i-1] == MFM_PRE_SYNC_DATA_VALUE)
         && (info->clck[i-1] < SYNCHRO_CLOCK_MARK)
         && (info->data[i+3] == MFM_INFO_ID)
         && (info->clck[i+3] < SYNCHRO_CLOCK_MARK))
        {
            if (is_dd_floppy_sector (info->track,
                                           info->data+i-12,
                                           info->clck+i-12) == sector)
                return i;
            i += 256;    /* skip a little bit less than a sector */
        }
        i++;
    }
    return error_Message (TEO_ERROR_DISK_CONVERSION, NULL);
}



/* disk_IsSDFloppySector:
 *  Find the wanted SD sector info in raw track.
 */
int disk_IsSDFloppySector (int sector, struct DISK_INFO *info)
{
    int i = 6;

    while (i <= (info->track_size - FM_SECTOR_SIZE))
    {
        if ((info->data[i] == FM_INFO_ID)
         && (info->clck[i] >= SYNCHRO_CLOCK_MARK)
         && (info->data[i+1] == SYNCHRO_CLOCK_MARK)
         && (info->clck[i+1] < SYNCHRO_CLOCK_MARK)
         && (info->data[i-1] == FM_PRE_SYNC_DATA_VALUE)
         && (info->clck[i-1] < SYNCHRO_CLOCK_MARK))
        {
            if (is_sd_floppy_sector (info->track,
                                           info->data+i-6,
                                           info->clck+i-6) == sector)
                return i;
            i += 128;    /* skip a little bit less than a sector */
        }
        i++;
    }
    return error_Message (TEO_ERROR_DISK_CONVERSION, NULL);
}



int disk_Load (int drive, const char filename[])
{
    /* limite la valeur des faces */
    if ((teo.disk[drive].side < 0)
     || (teo.disk[drive].side > NBDRIVE))
        teo.disk[drive].side = 0;

    if ((sap_LoadDisk(drive, filename) < 0)
     && (hfe_LoadDisk(drive, filename) < 0)
     && (fd_LoadDisk(drive, filename) < 0))
        return TEO_ERROR;

    return 0;
    (void)drive;
}



/* disk_AllocRawTracks:
 *  Allocate the memory for the raw tracks.
 */
int disk_AllocRawTracks (int track_size, struct DISK_INFO *info)
{
    info->data = std_free (info->data);
    info->clck = std_free (info->clck);
    info->track_size = 0;

    info->data = calloc (track_size, 1);
    if (info->data == NULL)
        return error_Message (TEO_ERROR_ALLOC, NULL);
        
    info->clck = std_free (info->clck);
    info->clck = calloc (track_size, 1);
    if (info->clck == NULL)
    {
        info->data = std_free (info->data);
        return error_Message (TEO_ERROR_ALLOC, NULL);
    }
    info->track_size = track_size;
    return 0;
}



/* disk_FirstLoad:
 *  Premier chargement des disquettes.
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



int disk_IsDisk (const char filename[])
{
    if ((sap_IsSap (filename) < 0)
     && (hfe_IsHfe (filename) < 0)
     && (fd_IsFd (filename) < 0))
        return TEO_ERROR;

    return 0;
    (void)filename;
}



int disk_Init (void)
{
    int drive;

    thmfc1_controller = thmfc1_Alloc ();
    if (thmfc1_controller == NULL)
        return TEO_ERROR;

    for (drive=0; drive<NBDRIVE; drive++)
    {
        memset (&disk[drive], 0x00, sizeof (DISK_PARAMETER));
        disk[drive].state       = TEO_DISK_ACCESS_NONE;
        disk[drive].info        = &thmfc1_controller->info;
        disk[drive].info->drive = drive;
        disk[drive].info->track = -1;  /* force track to be loaded */
    }
    dkc = thmfc1_controller;

    return 0;
}



void disk_Free (void)
{
    thmfc1_Free (thmfc1_controller);
}

