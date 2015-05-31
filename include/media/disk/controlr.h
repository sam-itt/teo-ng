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
 *  Module     : media/disk/controlr/thmfc1.h
 *  Version    : 1.8.3
 *  Créé par   : François Mouret 20/11/2012
 *  Modifié par: François Mouret 31/08/2013
 *
 *  Controller.
 */


#ifndef MEDIA_DISK_CONTROLR_H
#define MEDIA_DISK_CONTROLR_H

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

#define TEO_DISK_MFM_TRACK_SIZE  (0x61b0/4)

/* return position of sector in track memory (interleave 7) */
#define DDSECTORPOS(sector)  (((((sector) - 1) * 7) % 16) * MFM_SECTOR_SIZE)
#define SDSECTORPOS(sector)  (((((sector) - 1) * 7) % 16) * FM_SECTOR_SIZE)

struct DISK_INFO {
    int    track_size;
    uint8  *data;
    uint8  *clck;
    int    drive;
    int    side;
    int    track;
    int    sector;
    int    byte_rate;
    int    sector_size;
    int    fat_size;
    int    track_count;
};

struct DISK_CONTROLLER_SIDING {
    int    sector;
    int    track;
    int    last_pos;
    mc6809_clock_t motor_clock;
    mc6809_clock_t motor_stop;
};

struct DISK_CONTROLLER {
    /* internal registers (read only) */
    int    rr0;
    int    rr1;
    int    rr2;
    int    rr3;
    int    rr4;
    int    rr5;
    int    rr6;
    int    rr7;
    int    rr8;
    int    rr9;
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
    /* internal registers (write only) */
    int    wr0;
    int    wr1;
    int    wr2;
    int    wr3;
    int    wr4;
    int    wr5;
    int    wr6;
    int    wr7;
    int    wr8;
    int    wr9;
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
    int    (*StillWriting) (void);
    /* extra registers */
    int    ctrl;
    int    drive;
    int    crc;
    int    write_door;
    int    process;
    int    process_cpt;
    int    auto_count;
    mc6809_clock_t read_address_clock;
    int    sector[2];
    int    track[2];
    int    last_pos[2];
    mc6809_clock_t motor_clock[2];
    mc6809_clock_t motor_stop[2];
    struct DISK_INFO info;
};

extern struct DISK_CONTROLLER *dkc;

extern struct DISK_CONTROLLER *thmfc1_Alloc (void);
extern void thmfc1_Free (struct DISK_CONTROLLER *thmfc1);
extern void thmfc1_Update (void);

#endif

