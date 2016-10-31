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
 *  Version    : 1.8.4
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 24/10/2003
 *               François Mouret 05/10/2012 31/08/2013 31/07/2016
 *
 *  Gestion des disquettes.
 */


#ifndef MEDIA_DISK_H
#define MEDIA_DISK_H

#define MFM_GAP_DATA_VALUE      0x4e
#define MFM_PRE_SYNC_DATA_VALUE 0x00
#define MFM_SYNCHRO_DATA_VALUE  0xa1
#define MFM_SYNCHRO_CLOCK_VALUE 0x0a
#define MFM_INFO_ID             0xfe
#define MFM_SECTOR_ID           0xfb
#define MFM_HEAD_NUMBER         0x00
#define MFM_SIZE_ID             0x01
#define MFM_DATA_CLOCK_VALUE    0xff
#define MFM_SECTOR_SIZE         362
#define MFM_CRC_INFO_INIT       0xb230
#define MFM_CRC_DATA_INIT       0xe295

#define FM_GAP_DATA_VALUE       0xff
#define FM_PRE_SYNC_DATA_VALUE  0x00
#define FM_SYNCHRO_CLOCK_VALUE  0xc7
#define FM_INFO_ID              0xfe
#define FM_SECTOR_ID            0xfb
#define FM_HEAD_NUMBER          0x00
#define FM_SIZE_ID              0x00
#define FM_DATA_CLOCK_VALUE     0xff
#define FM_SECTOR_SIZE          174
#define FM_CRC_INFO_INIT        0xef21
#define FM_CRC_DATA_INIT        0xbf84

#define SYNCHRO_CLOCK_MARK       0x80
#define SYNCHRO_CLOCK_MARK_WRITE 0x81
#define DATA_CLOCK_MARK          0x00
#define DATA_CLOCK_MARK_WRITE    0x01

/* return position of sector in track memory (interleave 7) */
#define DDSECTORPOS(sector)  (((((sector) - 1) * 7) % 16) * MFM_SECTOR_SIZE)
#define SDSECTORPOS(sector)  (((((sector) - 1) * 7) % 16) * FM_SECTOR_SIZE)

#define TEO_DISK_DD_BYTE_RATE     (250000/8)
#define TEO_DISK_DD_SECTOR_SIZE   256
#define TEO_DISK_DD_TRACK_NUMBER  80
#define TEO_DISK_DD_TRACK_SIZE    (0x61b0/4)

#define TEO_DISK_SD_BYTE_RATE     (125000/8)
#define TEO_DISK_SD_SECTOR_SIZE   128
#define TEO_DISK_SD_TRACK_NUMBER  40
#define TEO_DISK_SD_TRACK_SIZE    (0x61b0/8)

#define TEO_DISK_TRACK_SIZE_MAX   TEO_DISK_DD_TRACK_SIZE
#define TEO_DISK_SECTOR_SIZE_MAX  TEO_DISK_DD_SECTOR_SIZE
#define TEO_RAMDISK_SECTOR_SIZE   TEO_DISK_DD_SECTOR_SIZE
#define TEO_DISK_TRACK_NUMBER_MAX TEO_DISK_DD_TRACK_NUMBER

#define TEO_DISK_SECTOR_PER_TRACK 16

#define TEO_DISK_INVALID_NUMBER  0xffff


struct DISK_VECTOR {
    char   *str;
    int    side;
    int    side_count;
    int    write_protect;
    struct DISK_VECTOR *next;
};


enum {
    TEO_DISK_ACCESS_NONE = 1,
    TEO_DISK_ACCESS_DIRECT,
    TEO_DISK_ACCESS_SAP,
    TEO_DISK_ACCESS_HFE,
    TEO_DISK_ACCESS_FD
};


struct CURRENT_AND_LAST {
    int    curr;
    int    last;
};


struct DISK_DRIVE {
    int    sector;
    struct CURRENT_AND_LAST track;
    struct CURRENT_AND_LAST pos;
    mc6809_clock_t motor_start;
    mc6809_clock_t motor_stop;
};


struct DISK_CONTROLLER {
    /* internal registers (read only) */
    int    rr0;   /* $E7D0/$A7D0 */
    int    rr1;   /* $E7D1/$A7D1 */
    int    rr2;   /* $E7D2/$A7D2 */
    int    rr3;   /* $E7D3/$A7D3 */
    int    rr4;   /* $E7D4/$A7D4 */
    int    rr5;   /* $E7D5/$A7D5 */
    int    rr6;   /* $E7D6/$A7D6 */
    int    rr7;   /* $E7D7/$A7D7 */
    int    rr8;   /* $E7D8/$A7D8 */
    int    rr9;   /* $E7D9/$A7D9 */
    /* getters */
    int    (*GetReg0) (void);
    int    (*GetReg1) (void);
    int    (*GetReg2) (void);
    int    (*GetReg3) (void);
    int    (*GetReg4) (void);
    int    (*GetReg5) (void);
    int    (*GetReg6) (void);
    int    (*GetReg7) (void);
    int    (*GetReg8) (void);
    int    (*GetReg9) (void);
    /* internal registers (write only)*/
    int    wr0;   /* $E7D0/$A7D0 */
    int    wr1;   /* $E7D1/$A7D1 */
    int    wr2;   /* $E7D2/$A7D2 */
    int    wr3;   /* $E7D3/$A7D3 */
    int    wr4;   /* $E7D4/$A7D4 */
    int    wr5;   /* $E7D5/$A7D5 */
    int    wr6;   /* $E7D6/$A7D6 */
    int    wr7;   /* $E7D7/$A7D7 */
    int    wr8;   /* $E7D8/$A7D8 */
    int    wr9;   /* $E7D9/$A7D9 */
    /* setters */
    void   (*SetReg0) (int val);
    void   (*SetReg1) (int val);
    void   (*SetReg2) (int val);
    void   (*SetReg3) (int val);
    void   (*SetReg4) (int val);
    void   (*SetReg5) (int val);
    void   (*SetReg6) (int val);
    void   (*SetReg7) (int val);
    void   (*SetReg8) (int val);
    void   (*SetReg9) (int val);
    /* functions */
    int    (*StillRunning) (void);
    void   (*StopMotor) (void);
    /* automatic operations */
    int    crc;
    int    write_door;
    int    process;
    int    process_cpt;
    int    auto_count;
};


struct DISK_SIDE {
    int    byte_rate;
    int    sector_size;
    int    track_size;
    int    track_count;
    int    drive;
    int    state;
    int    side_count;
    int    write_protect;
    int    (*WriteTrack) (int drive, int track);
    int    (*ReadTrack) (int drive, int track);
    int    (*ReadSector) (int drive);
    int    (*WriteSector) (int drive, int buffer);
    int    (*FormatTrack) (int drive);
    int    (*IsWritable) (int drive);
    struct DISK_DRIVE *drv;
    struct DISK_CONTROLLER *dkc;
    uint8  *data;
    uint8  *clck;
};

extern struct DISK_SIDE disk[NBDRIVE];
extern int dkcurr; /* current disk */

extern int   disk_Init (void);
extern void  disk_Free (void);
extern void  disk_FirstLoad (void);
extern int   disk_Protection(int drive, int protection);
extern void  disk_Eject(int drive);
extern int   disk_IsSDFloppySector (int track, int sector);
extern int   disk_IsDDFloppySector (int track, int sector);
extern void  disk_CreateDDFloppySector (int track, int sector,
                           uint8 *sector_buffer, uint8 *data, uint8 *clock);
extern void  disk_CreateSDFloppySector (int track, int sector,
                           uint8 *sector_buffer, uint8 *data, uint8 *clock);
extern int   disk_ComputeCrc (uint8 *buffer, int length, int start_value);
extern int   disk_CheckFile (const char filename[], int mode);
extern int   disk_Load(int drive, const char filename[]);
extern void  disk_FillAllTracks (void);

/* GUI vectors */
extern int   disk_DiskVectorIndex (struct DISK_VECTOR *p, const char str[]);
extern int   disk_DiskVectorLength (struct DISK_VECTOR *p);
extern char *disk_DiskVectorText (struct DISK_VECTOR *p, int index);
extern struct DISK_VECTOR *disk_DiskVectorPtr (struct DISK_VECTOR *p, int index);
extern struct DISK_VECTOR *disk_DiskVectorAppend (struct DISK_VECTOR *p, const char str[], int side, int side_count, int write_protect);
extern void  disk_DiskVectorFree (struct DISK_VECTOR *p);

/* Controller */
extern void  disk_WriteTrack (void);
extern void  disk_ReadTrack (int drive);
extern void  disk_WriteSector (int drive);
extern void  disk_ReadSector (int drive);
extern int   disk_FormatTrack (int drive);
extern void  disk_Written (void);
extern void  disk_WriteTimeout (void);

/* Controller specific */
extern void thmfc1_Init (int controller);

#endif

