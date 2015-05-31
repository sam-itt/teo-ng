/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2015 François Mouret
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
 *  Module     : hfe.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of HFE format (MFM).
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>   /*****************************/
#endif

#include "defs.h"
#include "errors.h"
#include "std.h"

typedef struct picfileformatheader_
{
    unsigned char HEADERSIGNATURE[8];   // "HXCPICFE" 48 58 43 50 49 43 46 45
    unsigned char formatrevision;       // Revision 0 00
    unsigned char number_of_track;      // Number of track in the file 50
    unsigned char number_of_side;       // Number of valid side (Not used by the emulator) 01 
    unsigned char track_encoding;       // Encoding mode (Not used actually !) 00
    unsigned short bitRate;             // Bitrate in Kbit/s. Ex : 250=250000bits/s FA 00
    unsigned short floppyRPM;           // Rotation per minute (Not used by the emulator) 00 00
    unsigned char floppyinterfacemode;  // Floppy interface mode. (Please see the list above.) 07
    unsigned char dnu;                  // Free 01
    unsigned short track_list_offset;   // Offset of the track list LUT in block of 512 bytes (Ex: 1=0x200) 01 00
    unsigned char write_allowed;        // The Floppy image are write protected ? FF
    unsigned char single_step;          // 0xFF : Single Step - 0x00 Double Step mode
    unsigned char track0s0_altencoding; // 0x00 : Use an alternate track_encoding for track 0 Side 0
    unsigned char track0s0_encoding;    // alternate track_encoding for track 0 Side 0
    unsigned char track0s1_altencoding; // 0x00 : Use an alternate track_encoding for track 0 Side 1
    unsigned char track0s1_encoding;    // alternate track_encoding for track 0 Side 1
} picfileformatheader;

#define IBMPC_DD_FLOPPYMODE            0x00
#define IBMPC_HD_FLOPPYMODE            0x01
#define ATARIST_DD_FLOPPYMODE          0x02
#define ATARIST_HD_FLOPPYMODE          0x03
#define AMIGA_DD_FLOPPYMODE            0x04
#define AMIGA_HD_FLOPPYMODE            0x05
#define CPC_DD_FLOPPYMODE              0x06
#define GENERIC_SHUGGART_DD_FLOPPYMODE 0x07
#define IBMPC_ED_FLOPPYMODE            0x08
#define MSX2_DD_FLOPPYMODE             0x09

#define ISOIBM_MFM_ENCODING 0x00
#define AMIGA_MFM_ENCODING  0x01
#define ISOIBM_FM_ENCODING  0x02
#define EMU_FM_ENCODING     0x03
#define UNKNOWN_ENCODING    0xFF

typedef struct pictrack_
{
    unsigned short offset;    // Offset of the track data in block of 512 bytes (Ex: 2=0x400)
    unsigned short track_len; // Length of the track data in byte.
} pictrack;

static picfileformatheader hfe_hd;
static pictrack track_list[80];

#define MFM_SYNCHRO_WORD  0x2291      /* == 4489 */
#define CC90HFE_MARK      PROG_NAME"v"PROG_VERSION_STRING

static const char hfe_header[] = "HXCPICFE";
FILE *file = NULL;



static int raw_to_mfm [256] = {
    0x5455, 0x5495, 0x5425, 0x54a5, 0x5449, 0x5489, 0x5429, 0x54a9,
    0x5452, 0x5492, 0x5422, 0x54a2, 0x544a, 0x548a, 0x542a, 0x54aa,
    0x9454, 0x9494, 0x9424, 0x94a4, 0x9448, 0x9488, 0x9428, 0x94a8,
    0x9452, 0x9492, 0x9422, 0x94a2, 0x944a, 0x948a, 0x942a, 0x94aa,
    0x2455, 0x2495, 0x2425, 0x24a5, 0x2449, 0x2489, 0x2429, 0x24a9,
    0x2452, 0x2492, 0x2422, 0x24a2, 0x244a, 0x248a, 0x242a, 0x24aa,
    0xa454, 0xa494, 0xa424, 0xa4a4, 0xa448, 0xa488, 0xa428, 0xa4a8,
    0xa452, 0xa492, 0xa422, 0xa4a2, 0xa44a, 0xa48a, 0xa42a, 0xa4aa,
    0x4855, 0x4895, 0x4825, 0x48a5, 0x4849, 0x4889, 0x4829, 0x48a9,
    0x4852, 0x4892, 0x4822, 0x48a2, 0x484a, 0x488a, 0x482a, 0x48aa,
    0x8854, 0x8894, 0x8824, 0x88a4, 0x8848, 0x8888, 0x8828, 0x88a8,
    0x8852, 0x8892, 0x8822, 0x88a2, 0x884a, 0x888a, 0x882a, 0x88aa,
    0x2855, 0x2895, 0x2825, 0x28a5, 0x2849, 0x2889, 0x2829, 0x28a9,
    0x2852, 0x2892, 0x2822, 0x28a2, 0x284a, 0x288a, 0x282a, 0x28aa,
    0xa854, 0xa894, 0xa824, 0xa8a4, 0xa848, 0xa888, 0xa828, 0xa8a8,
    0xa852, 0xa892, 0xa822, 0xa8a2, 0xa84a, 0xa88a, 0xa82a, 0xa8aa,
    0x5255, 0x5295, 0x5225, 0x52a5, 0x5249, 0x5289, 0x5229, 0x52a9,
    0x5252, 0x5292, 0x5222, 0x52a2, 0x524a, 0x528a, 0x522a, 0x52aa,
    0x9254, 0x9294, 0x9224, 0x92a4, 0x9248, 0x9288, 0x9228, 0x92a8,
    0x9252, 0x9292, 0x9222, 0x92a2, 0x924a, 0x928a, 0x922a, 0x92aa,
    0x2255, 0x2295, 0x2225, 0x22a5, 0x2249, 0x2289, 0x2229, 0x22a9,
    0x2252, 0x2292, 0x2222, 0x22a2, 0x224a, 0x228a, 0x222a, 0x22aa,
    0xa254, 0xa294, 0xa224, 0xa2a4, 0xa248, 0xa288, 0xa228, 0xa2a8,
    0xa252, 0xa292, 0xa222, 0xa2a2, 0xa24a, 0xa28a, 0xa22a, 0xa2aa,
    0x4a55, 0x4a95, 0x4a25, 0x4aa5, 0x4a49, 0x4a89, 0x4a29, 0x4aa9,
    0x4a52, 0x4a92, 0x4a22, 0x4aa2, 0x4a4a, 0x4a8a, 0x4a2a, 0x4aaa,
    0x8a54, 0x8a94, 0x8a24, 0x8aa4, 0x8a48, 0x8a88, 0x8a28, 0x8aa8,
    0x8a52, 0x8a92, 0x8a22, 0x8aa2, 0x8a4a, 0x8a8a, 0x8a2a, 0x8aaa,
    0x2a55, 0x2a95, 0x2a25, 0x2aa5, 0x2a49, 0x2a89, 0x2a29, 0x2aa9,
    0x2a52, 0x2a92, 0x2a22, 0x2aa2, 0x2a4a, 0x2a8a, 0x2a2a, 0x2aaa,
    0xaa54, 0xaa94, 0xaa24, 0xaaa4, 0xaa48, 0xaa88, 0xaa28, 0xaaa8,
    0xaa52, 0xaa92, 0xaa22, 0xaaa2, 0xaa4a, 0xaa8a, 0xaa2a, 0xaaaa
};

static int mfm_to_raw [256] = {
    0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08,
    0x04, 0x04, 0x0c, 0x0c, 0x04, 0x04, 0x0c, 0x0c,
    0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08,
    0x04, 0x04, 0x0c, 0x0c, 0x04, 0x04, 0x0c, 0x0c,
    0x02, 0x02, 0x0a, 0x0a, 0x02, 0x02, 0x0a, 0x0a,
    0x06, 0x06, 0x0e, 0x0e, 0x06, 0x06, 0x0e, 0x0e,
    0x02, 0x02, 0x0a, 0x0a, 0x02, 0x02, 0x0a, 0x0a,
    0x06, 0x06, 0x0e, 0x0e, 0x06, 0x06, 0x0e, 0x0e,
    0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08,
    0x04, 0x04, 0x0c, 0x0c, 0x04, 0x04, 0x0c, 0x0c,
    0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08,
    0x04, 0x04, 0x0c, 0x0c, 0x04, 0x04, 0x0c, 0x0c,
    0x02, 0x02, 0x0a, 0x0a, 0x02, 0x02, 0x0a, 0x0a,
    0x06, 0x06, 0x0e, 0x0e, 0x06, 0x06, 0x0e, 0x0e,
    0x02, 0x02, 0x0a, 0x0a, 0x02, 0x02, 0x0a, 0x0a,
    0x06, 0x06, 0x0e, 0x0e, 0x06, 0x06, 0x0e, 0x0e,
    0x01, 0x01, 0x09, 0x09, 0x01, 0x01, 0x09, 0x09,
    0x05, 0x05, 0x0d, 0x0d, 0x05, 0x05, 0x0d, 0x0d,
    0x01, 0x01, 0x09, 0x09, 0x01, 0x01, 0x09, 0x09,
    0x05, 0x05, 0x0d, 0x0d, 0x05, 0x05, 0x0d, 0x0d,
    0x03, 0x03, 0x0b, 0x0b, 0x03, 0x03, 0x0b, 0x0b,
    0x07, 0x07, 0x0f, 0x0f, 0x07, 0x07, 0x0f, 0x0f,
    0x03, 0x03, 0x0b, 0x0b, 0x03, 0x03, 0x0b, 0x0b,
    0x07, 0x07, 0x0f, 0x0f, 0x07, 0x07, 0x0f, 0x0f,
    0x01, 0x01, 0x09, 0x09, 0x01, 0x01, 0x09, 0x09,
    0x05, 0x05, 0x0d, 0x0d, 0x05, 0x05, 0x0d, 0x0d,
    0x01, 0x01, 0x09, 0x09, 0x01, 0x01, 0x09, 0x09,
    0x05, 0x05, 0x0d, 0x0d, 0x05, 0x05, 0x0d, 0x0d,
    0x03, 0x03, 0x0b, 0x0b, 0x03, 0x03, 0x0b, 0x0b,
    0x07, 0x07, 0x0f, 0x0f, 0x07, 0x07, 0x0f, 0x0f,
    0x03, 0x03, 0x0b, 0x0b, 0x03, 0x03, 0x0b, 0x0b,
    0x07, 0x07, 0x0f, 0x0f, 0x07, 0x07, 0x0f, 0x0f
};



/* hfe_fgetw:
 *  Helper to read a 16 bits little endian.
 */
static int hfe_fgetw (FILE *file)
{
    int a,b;

    b = fgetc(file)&0xff;
    a = fgetc(file)&0xff;

    return (a<<8)|b;
}



/* hfe_putw:
 *  Helper to write a 16 bits little endian.
 */
static void hfe_fputw (int val, FILE *file)
{
    fputc (val, file);
    fputc (val>>8, file);
}



static int format_error (const char filename[])
{
    (void)fclose(file);
    file = NULL;
    return error_Message (CC90HFE_ERROR_FILE_FORMAT, filename);
}    



static void init_hfe_block (uint8 *buf)
{
    memset (buf, 0x00, 512);
    memcpy (buf+256-strlen(CC90HFE_MARK), CC90HFE_MARK, strlen(CC90HFE_MARK));
    memcpy (buf+512-strlen(CC90HFE_MARK), CC90HFE_MARK, strlen(CC90HFE_MARK));
}
 

/* ------------------------------------------------------------------------- */


/* hfe_ReadTrack:
 *  Read and convert the MFM track into raw format.
 */
int hfe_ReadTrack (void)
{
    int i;
    int err = 0;
    int size = 256;
    int side;
    uint8 buf[512];

    disk.data[SIDE_0] = std_free (disk.data[SIDE_0]);
    disk.clck[SIDE_0] = std_free (disk.clck[SIDE_0]);
    disk.data[SIDE_1] = std_free (disk.data[SIDE_1]);
    disk.clck[SIDE_1] = std_free (disk.clck[SIDE_1]);

    if (((disk.data[SIDE_0] = malloc (disk.track_size)) != NULL)
     && ((disk.clck[SIDE_0] = malloc (disk.track_size)) != NULL)
     && ((disk.data[SIDE_1] = malloc (disk.track_size)) != NULL)
     && ((disk.clck[SIDE_1] = malloc (disk.track_size)) != NULL))
    {
        /* clock field initialized to data */
        memset (disk.clck[SIDE_0], DATA_CLOCK_MARK, disk.track_size);
        memset (disk.clck[SIDE_1], DATA_CLOCK_MARK, disk.track_size);

        for (i=0; i<disk.track_size; i++)
        {
            /* read the block */
            if (size == 256)
            {
                if (fread (buf, 1, (size_t)512, file) != (size_t)512)
                    return error_Message (CC90HFE_ERROR_FILE_READ, NULL);
                size = 0;
            }

            for (side=SIDE_0; side<=SIDE_1; side++)
            {
                /* update data value */
                disk.data[side][i] = (mfm_to_raw[buf[size+(side*256)]] << 4)
                                    | mfm_to_raw[buf[size+(side*256)+1]];
       
                /* update clock value if synchro word */
                if (((buf[size+(side*256)]&0x1c) == 0)
                 && ((buf[size+(side*256)+1]&0x0e) == 0))
                    disk.clck[side][i] = SYNCHRO_CLOCK_MARK;
            }
            size += 2;
        }
    }
    else
        err = error_Message (CC90HFE_ERROR_ALLOC, NULL);

    return err;
}



/* hfe_WriteTrack:
 *  Convert the raw track into the MFM format and write it.
 */
int hfe_WriteTrack (void)
{
    int i;
    int val;
    int last_val[2] = { 0, 0 };
    int size = 0;
    int side = 0;
    uint8 buf[512];

    init_hfe_block (buf);
    
    for (i=0; i<disk.track_size; i++)
    {
        for (side=SIDE_0; side<=SIDE_1; side++)
        {
            /* get word value */
            val = (disk.clck[side][i] >= SYNCHRO_CLOCK_MARK)
                           ? MFM_SYNCHRO_WORD
                           : raw_to_mfm[disk.data[side][i]];

            /* set first clock bit if necessary */
            if (((last_val[side] & 0x80) | (val & 0x0200)) == 0)
                val |= 0x0100;

            /* record data track */
            buf[size+(side<<8)] = (uint8)(val>>8);
            buf[size+(side<<8)+1] = (uint8)val;
            last_val[side] = val;
        }
        size += 2;
        if ((size == 256) || (i == (disk.track_size - 1)))
        {
            /* write the block if full or last */
            if (fwrite (buf, 1, (size_t)512, file) != (size_t)512)
                return error_Message (CC90HFE_ERROR_FILE_WRITE, NULL);
            size = 0;
            init_hfe_block (buf);
        }
    }
    return 0;
}



/* hfe_ReadOpen:
 *  Open a HFE file in read mode.
 */
int hfe_ReadOpen (const char filename[])
{
    int i;
    size_t file_size;

    /* check size of file */
    file_size = std_FileSize (filename);
    if (file_size != 3922*512)
        return error_Message (CC90HFE_ERROR_FILE_FORMAT, filename);

    /* load header */
    file = fopen (filename, "rb");
    if (file == NULL)
        return error_Message (CC90HFE_ERROR_FILE_OPEN, filename);
    
    if (fread (hfe_hd.HEADERSIGNATURE, 1, 8, file) != 8)
        return format_error (filename);
    hfe_hd.formatrevision      = (unsigned char)fgetc(file);
    hfe_hd.number_of_track     = (unsigned char)fgetc(file);
    hfe_hd.number_of_side      = (unsigned char)fgetc(file);
    hfe_hd.track_encoding      = (unsigned char)fgetc(file);
    hfe_hd.bitRate             = (unsigned short)hfe_fgetw (file);
    hfe_hd.floppyRPM           = (unsigned short)hfe_fgetw (file);
    hfe_hd.floppyinterfacemode = (unsigned char)fgetc(file);
    hfe_hd.dnu                 = (unsigned char)fgetc(file);
    hfe_hd.track_list_offset   = (unsigned short)hfe_fgetw (file);
    hfe_hd.write_allowed       = (unsigned char)fgetc(file);

    /* check header */
    if ((memcmp (hfe_hd.HEADERSIGNATURE, hfe_header, 8) != 0)
     || (hfe_hd.formatrevision != 0)
     || (hfe_hd.number_of_track > 80)
     || (hfe_hd.number_of_side > 2)
     || (hfe_hd.track_encoding != ISOIBM_MFM_ENCODING)
     || (hfe_hd.track_list_offset != 1))
        return format_error (filename);

    /* load track list */
    if (fseek (file, 512, SEEK_SET) != 0)
        return format_error (filename);
    for (i=0; i<80; i++)
    {
        track_list[i].offset    = hfe_fgetw (file);
        track_list[i].track_len = hfe_fgetw (file);
    }

    if (fseek (file, 512*2, SEEK_SET) != 0)
        return format_error (filename);

    return 0;
}



/* hfe_WriteOpen:
 *  Open a HFE file in write mode.
 */
int hfe_WriteOpen (const char filename[])
{
    int i;
    int track_offset;
    int track_length;

    file = fopen (filename, "wb");
    if (file == NULL)
        return error_Message (CC90HFE_ERROR_FILE_OPEN, filename);


    /* write header */
    (void)fwrite ("HXCPICFE", 8, 1, file); /* HEADERSIGNATURE      */
    fputc     (0x00, file);                /* formatrevision       */
    fputc     (0x50, file);                /* number_of_track      */
    fputc     (0x02, file);                /* number_of_side       */
    fputc     (ISOIBM_MFM_ENCODING, file); /* track_encoding       */
    hfe_fputw (250, file);                 /* bitRate              */
    hfe_fputw (0, file);                   /* floppyRPM            */
    fputc     (GENERIC_SHUGGART_DD_FLOPPYMODE, file); /* floppyinterfacemode  */
    fputc     (0x01, file);                /* dnu                  */
    hfe_fputw (0x01, file);                /* track_list_offset    */
    fputc     (0xFF, file);                /* write_allowed        */
    fputc     (0xFF, file);                /* single_step          */
    fputc     (0xFF, file);                /* track0s0_altencoding */
    fputc     (0x00, file);                /* track0s0_encoding    */
    fputc     (0xFF, file);                /* track0s1_altencoding */
    fputc     (0x00, file);                /* track0s1_encoding    */

    /* skip to next block */
    for (i=26;i<512;i++)
        fputc (0xff, file);  /* gap */

    /* write track list */
    track_offset = 2;
    track_length = MFM_TRACK_LENGTH;
    for (i=0; i<80; i++)
    {
        hfe_fputw (track_offset, file);
        hfe_fputw (MFM_TRACK_LENGTH, file);
        track_offset+= ((track_length + 511) / 512);
    }

    /* skip to next block */
    for (i=80*4;i<512;i++)
        fputc (0xff, file);  /* gap */

    return 0;
}


/* hfe_Close:
 *  Close the HFE file.
 */
void hfe_Close (void)
{
    if (file != NULL)
        file = std_fclose (file);
}

