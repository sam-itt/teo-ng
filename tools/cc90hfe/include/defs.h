/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2018 Yves Charriau, François Mouret
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
 *  Module     : defs.h
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 26/07/2013
 *
 *  Definitions.
 */

#ifndef DEFS_H
#define DEFS_H 1

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef NULL
#define NULL  0
#endif

#ifdef UNIX_TOOL
#   define SYSTEMSLASH "/"
#else
#   define SYSTEMSLASH "\\"
#endif

#define APPLICATION_DIR    "teo"
#define PROG_NAME          "CC90HFE"
#define PROG_VERSION_MAJOR "0"
#define PROG_VERSION_MINOR "7"
#define PROG_VERSION_MICRO "0"
#define PROG_VERSION_STRING PROG_VERSION_MAJOR"."PROG_VERSION_MINOR

#define PROG_CREATION_MONTH "April"
#define PROG_CREATION_YEAR "2018"
#define PROG_WEB_SITE       "http://sourceforge.net/projects/teoemulator/"
#define PROG_WEB_FORUM      "http://www.logicielsmoto.com/"
#define PROG_DESCRIPTION    "Serial transfers for CC90-232"

#define MFM_GAP_DATA_VALUE       0x4e
#define MFM_PRE_SYNC_DATA_VALUE  0x00
#define MFM_SYNCHRO_DATA_VALUE   0xa1
#define MFM_INFO_ID              0xfe
#define MFM_SECTOR_ID            0xfb
#define MFM_CRC_DATA_INIT        0xe295

#define MFM_TRACK_LENGTH         0x61b0

#define SYNCHRO_CLOCK_MARK  0x80
#define DATA_CLOCK_MARK     0x00

#define SIDE_0  0
#define SIDE_1  1

#ifndef MAX_PATH
#   define MAX_PATH 300
#endif

extern int is_fr;

typedef unsigned char uint8;  /* unité de mémoire */

struct DISK_INFO {
    int    track_size;
    int    track_count;
    int    pack_size;
    int    sector_size;
    uint8  *data[2];
    uint8  *clck[2];
    char   *file_name;
};
extern struct DISK_INFO disk;

struct GUI_INFO {
    int   window_x;
    int   window_y;
    char  *default_folder;
    char  *archive_folder;
    char  *extract_folder;
    int   read_retry_max;
    int   not_thomson_side[2];
    char  *port_name;
    int   timeout;
};
extern struct GUI_INFO gui;

extern int is_fr;

extern void main_free_all (void);


#endif
