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
 *  Module     : media/disk.h
 *  Version    : 1.8.2
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 24/10/2003
 *               François Mouret 05/10/2012
 *
 *  Gestion des disquettes.
 */


#ifndef MEDIA_DISK_H
#define MEDIA_DISK_H

#define TEO_DISK_SECTOR_SIZE_MAX  256
#define TEO_RAMDISK_SECTOR_SIZE   256
#define TEO_DISK_TRACK_NUMBER_MAX  80

// #define SECTOR_PER_TRACK         16

#define TEO_DISK_SECTOR_COUNT    16

struct DISK_VECTOR {
    char   *str;
    int    side;
    int    side_count;
    struct DISK_VECTOR *next;
};

enum {
    TEO_DISK_ACCESS_NONE = 1,
    TEO_DISK_ACCESS_DIRECT,
    TEO_DISK_ACCESS_SAP,
    TEO_DISK_ACCESS_HFE,
    TEO_DISK_ACCESS_FD
};

typedef struct {
    int   state;
    int   side_count;
    char  *tmp;
    int   format;
    int   (*WriteCtrlTrack) (const char filename [], struct DISK_INFO *info);
    int   (*ReadCtrlTrack) (const char filename [], struct DISK_INFO *info);
    int   (*ReadCtrlSector) (const char filename [], struct DISK_INFO *info);
    int   (*WriteCtrlSector) (const char filename [], int buffer, struct DISK_INFO *info);
    int   (*FormatCtrlTrack) (const char filename [], struct DISK_INFO *info);
    struct DISK_INFO *info;
} DISK_PARAMETER;

extern DISK_PARAMETER disk[NBDRIVE];

extern int   disk_Init (void);
extern void  disk_UnloadAll (void);
extern void  disk_FirstLoad (void);
extern int   disk_IsDisk (const char filename[]);
extern int   disk_SetProtection(int drive, int mode);
extern void  disk_Eject(int drive);

extern int   disk_IsSDFloppySector (int sector, struct DISK_INFO *info);
extern int   disk_IsDDFloppySector (int sector, struct DISK_INFO *info);
extern void  disk_CreateDDFloppySector (int track, int sector,
                           uint8 *sector_buffer, uint8 *data, uint8 *clock);
extern void  disk_CreateSDFloppySector (int track, int sector,
                           uint8 *sector_buffer, uint8 *data, uint8 *clock);
extern int   disk_AllocRawTracks (int track_size, struct DISK_INFO *info);
extern int   disk_ComputeCrc (uint8 *buffer, int length, int start_value);
extern int   disk_CheckFile (const char filename[], int mode);
extern int   disk_Check (const char filename[]);
extern int   disk_Load(int drive, const char filename[]);
extern int   disk_ReadCtrlTrack (int drive);
extern int   disk_WriteCtrlTrack (int drive);
extern void  disk_BuildSectorMap (int *sector_map, int factor);

extern int   disk_DiskVectorIndex (struct DISK_VECTOR *p, const char str[]);
extern int   disk_DiskVectorLength (struct DISK_VECTOR *p);
extern char *disk_DiskVectorText (struct DISK_VECTOR *p, int index);
extern struct DISK_VECTOR *disk_DiskVectorPtr (struct DISK_VECTOR *p, int index);
extern struct DISK_VECTOR *disk_DiskVectorAppend (struct DISK_VECTOR *p, const char str[], int side, int side_count);
extern void  disk_DiskVectorFree (struct DISK_VECTOR *p);

#endif

