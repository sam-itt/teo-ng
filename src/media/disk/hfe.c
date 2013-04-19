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
 *  Module     : media/disk/hfe.c
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 08/11/2012
 *  Modifié par: 
 *
 *  Gestion du format HFE.
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
#include "media/disk/hfe.h"

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
static pictrack track_list[2][80];

#define MFM_SYNCHRO_WORD  0x2291      /* == 4489 */
#define FM_SYNCHRO_CLOCK  0x22002022  /* == 0xc7 */
#define FM_SYNCHRO_DATA   0x22222222  /* == 0xff */

static const char hfe_header[] = "HXCPICFE";

static int raw_to_fm[256] = {
    0x00000000, 0x00000080, 0x00000008, 0x00000088, 
    0x00008000, 0x00008080, 0x00008008, 0x00008088, 
    0x00000800, 0x00000880, 0x00000808, 0x00000888, 
    0x00008800, 0x00008880, 0x00008808, 0x00008888, 
    0x00800000, 0x00800080, 0x00800008, 0x00800088, 
    0x00808000, 0x00808080, 0x00808008, 0x00808088, 
    0x00800800, 0x00800880, 0x00800808, 0x00800888, 
    0x00808800, 0x00808880, 0x00808808, 0x00808888, 
    0x00080000, 0x00080080, 0x00080008, 0x00080088, 
    0x00088000, 0x00088080, 0x00088008, 0x00088088, 
    0x00080800, 0x00080880, 0x00080808, 0x00080888, 
    0x00088800, 0x00088880, 0x00088808, 0x00088888, 
    0x00880000, 0x00880080, 0x00880008, 0x00880088, 
    0x00888000, 0x00888080, 0x00888008, 0x00888088, 
    0x00880800, 0x00880880, 0x00880808, 0x00880888, 
    0x00888800, 0x00888880, 0x00888808, 0x00888888, 
    0x80000000, 0x80000080, 0x80000008, 0x80000088, 
    0x80008000, 0x80008080, 0x80008008, 0x80008088, 
    0x80000800, 0x80000880, 0x80000808, 0x80000888, 
    0x80008800, 0x80008880, 0x80008808, 0x80008888, 
    0x80800000, 0x80800080, 0x80800008, 0x80800088, 
    0x80808000, 0x80808080, 0x80808008, 0x80808088, 
    0x80800800, 0x80800880, 0x80800808, 0x80800888, 
    0x80808800, 0x80808880, 0x80808808, 0x80808888, 
    0x80080000, 0x80080080, 0x80080008, 0x80080088, 
    0x80088000, 0x80088080, 0x80088008, 0x80088088, 
    0x80080800, 0x80080880, 0x80080808, 0x80080888, 
    0x80088800, 0x80088880, 0x80088808, 0x80088888, 
    0x80880000, 0x80880080, 0x80880008, 0x80880088, 
    0x80888000, 0x80888080, 0x80888008, 0x80888088, 
    0x80880800, 0x80880880, 0x80880808, 0x80880888, 
    0x80888800, 0x80888880, 0x80888808, 0x80888888, 
    0x08000000, 0x08000080, 0x08000008, 0x08000088, 
    0x08008000, 0x08008080, 0x08008008, 0x08008088, 
    0x08000800, 0x08000880, 0x08000808, 0x08000888, 
    0x08008800, 0x08008880, 0x08008808, 0x08008888, 
    0x08800000, 0x08800080, 0x08800008, 0x08800088, 
    0x08808000, 0x08808080, 0x08808008, 0x08808088, 
    0x08800800, 0x08800880, 0x08800808, 0x08800888, 
    0x08808800, 0x08808880, 0x08808808, 0x08808888, 
    0x08080000, 0x08080080, 0x08080008, 0x08080088, 
    0x08088000, 0x08088080, 0x08088008, 0x08088088, 
    0x08080800, 0x08080880, 0x08080808, 0x08080888, 
    0x08088800, 0x08088880, 0x08088808, 0x08088888, 
    0x08880000, 0x08880080, 0x08880008, 0x08880088, 
    0x08888000, 0x08888080, 0x08888008, 0x08888088, 
    0x08880800, 0x08880880, 0x08880808, 0x08880888, 
    0x08888800, 0x08888880, 0x08888808, 0x08888888, 
    0x88000000, 0x88000080, 0x88000008, 0x88000088, 
    0x88008000, 0x88008080, 0x88008008, 0x88008088, 
    0x88000800, 0x88000880, 0x88000808, 0x88000888, 
    0x88008800, 0x88008880, 0x88008808, 0x88008888, 
    0x88800000, 0x88800080, 0x88800008, 0x88800088, 
    0x88808000, 0x88808080, 0x88808008, 0x88808088, 
    0x88800800, 0x88800880, 0x88800808, 0x88800888, 
    0x88808800, 0x88808880, 0x88808808, 0x88808888, 
    0x88080000, 0x88080080, 0x88080008, 0x88080088, 
    0x88088000, 0x88088080, 0x88088008, 0x88088088, 
    0x88080800, 0x88080880, 0x88080808, 0x88080888, 
    0x88088800, 0x88088880, 0x88088808, 0x88088888, 
    0x88880000, 0x88880080, 0x88880008, 0x88880088, 
    0x88888000, 0x88888080, 0x88888008, 0x88888088, 
    0x88880800, 0x88880880, 0x88880808, 0x88880888, 
    0x88888800, 0x88888880, 0x88888808, 0x88888888
};

static uint8 fm_to_raw[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03
};

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


/* my_fgetw:
 *  Helper pour lit un 16 bits little endian.
 */
static int hfe_fgetw (FILE *file)
{
    int a,b;

    b = fgetc(file)&0xff;
    a = fgetc(file)&0xff;

    return (a<<8)|b;
}



#if 0
static void hfe_fputw (int val, FILE *file)
{
    fputc (val, file);
    fputc (val>>8, file);
}
#endif



/* read_fm_track:
 *  Read and convert the FM track into raw format.
 */
static int read_fm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    uint8 *p;
    uint8 buf[256];

    /* clock field initialized to data */
    memset (info->clck, DATA_CLOCK_MARK, info->track_size);

    for (i=0; i<info->track_size; i++)
    {
        /* read the block */
        if ((i & 0x3f) == 0)
        {
            if (i != 0)
                if (fseek (file, 256, SEEK_CUR) != 0)
                     return TEO_ERROR_FILE_READ;

            if (fread (buf, 1, (size_t)256, file) != (size_t)256)
                return TEO_ERROR_FILE_READ;
        }

        /* update data value */
        p = buf+(i&0x3f)*4;
        info->data[i] = (fm_to_raw[p[0]] << 6)
                      | (fm_to_raw[p[1]] << 4)
                      | (fm_to_raw[p[2]] << 2)
                      | fm_to_raw[p[3]];

        /* update clock value if synchro word */
        if (((p[1]&0x22) == 0)
         && ((p[2]&0x02) == 0))
            info->clck[i] = SYNCHRO_CLOCK_MARK;
    }
    return 0;
}



/* read_mfm_track:
 *  Read and convert the MFM track into raw format.
 */
static int read_mfm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    uint8 *p;
    uint8 buf[256];

    /* clock field initialized to data */
    memset (info->clck, DATA_CLOCK_MARK, info->track_size);

    for (i=0; i<info->track_size; i++)
    {
        /* read the block */
        if ((i & 0x7f) == 0)
        {
            if (i != 0)
                if (fseek (file, 256, SEEK_CUR) != 0)
                     return TEO_ERROR_FILE_READ;

            if (fread (buf, 1, (size_t)256, file) != (size_t)256)
                return TEO_ERROR_FILE_READ;
        }

        /* update data value */
        p = buf+(i&0x7f)*2;
        info->data[i] = (mfm_to_raw[p[0]] << 4)
                       | mfm_to_raw[p[1]];
       
        /* update clock value if synchro word */
        if (((p[0]&0x1c) == 0)
         && ((p[1]&0x0e) == 0))
            info->clck[i] = SYNCHRO_CLOCK_MARK;
    }
    return 0;
}



/* read_ctrl_track:
 *  Open the file and read the HFE track.
 */
static int read_ctrl_track (const char filename [], struct DISK_INFO *info)
{
    int err = 0;
    FILE *file = NULL;
    long int file_seek;

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, filename);

    file_seek = (
        track_list[teo.disk[info->drive].side>>1][info->track].offset << 9)
        + ((teo.disk[info->drive].side & 1) << 8);

    if ((file = fopen (filename, "rb")) == NULL)
        return error_Message (TEO_ERROR_DISK_NONE, filename);

    if (fseek (file, file_seek, SEEK_SET) != 0)
        err = error_Message (TEO_ERROR_FILE_READ, filename);

    if (err == 0)
    {
        if (info->sector_size == 256)
        {
            err = disk_AllocRawTracks (
                      track_list[teo.disk[info->drive].side>>1][info->track].track_len>>2,
                      info);
            if (err == 0)
                err = read_mfm_track (file, info);
        }
        else
        {
            err = disk_AllocRawTracks (
                      track_list[teo.disk[info->drive].side>>1][info->track].track_len>>3,
                      info);
            if (err == 0)
                err = read_fm_track (file, info);
        }
    }

    (void)fclose(file);
    return err;
}



/* write_fm_track:
 *  Convert the raw track into the FM format and write it.
 */
static int write_fm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    int val;
    int size = 0;
    uint8 buf[256];

    for (i=0; i<info->track_size; i++)
    {
        /* get word value */
        val = raw_to_fm[info->data[i]]
             | ((info->clck[i] >= SYNCHRO_CLOCK_MARK) ? FM_SYNCHRO_CLOCK
                                                      : FM_SYNCHRO_DATA);
        /* record data track */
        buf[size++] = (uint8)(val >> 24);
        buf[size++] = (uint8)(val >> 16);
        buf[size++] = (uint8)(val >> 8);
        buf[size++] = (uint8)val;
        
        if ((size == 256) || (i == (info->track_size - 1)))
        {
            /* write the block if full or last */
            if (fwrite (buf, 1, (size_t)size, file) != (size_t)size)
                return error_Message (TEO_ERROR_FILE_WRITE, NULL);

            /* seek if not last block */
            if (i != (info->track_size - 1))
                if (fseek (file, 256, SEEK_CUR) != 0)
                    return error_Message (TEO_ERROR_FILE_WRITE, NULL);
            size = 0;
        }
    }
    return 0;
}



/* write_mfm_track:
 *  Convert the raw track into the MFM format and write it.
 */
static int write_mfm_track (FILE *file, struct DISK_INFO *info)
{
    int i;
    int val;
    int last_val = 0x0000;
    int size = 0;
    uint8 buf[256];

    for (i=0; i<info->track_size; i++)
    {
        /* get word value */
        val = (info->clck[i] >= SYNCHRO_CLOCK_MARK) ? MFM_SYNCHRO_WORD
                                                    : raw_to_mfm[info->data[i]];

        /* set first clock bit if necessary */
        if (((last_val & 0x80) | (val & 0x0200)) == 0)
            val |= 0x0100;

        /* record data track */
        buf[size++] = (uint8)(val>>8);
        buf[size++] = (uint8)val;

        if ((size == 256) || (i == (info->track_size - 1)))
        {
            /* write the block if full or last */
            if (fwrite (buf, 1, (size_t)size, file) != (size_t)size)
                return error_Message (TEO_ERROR_FILE_WRITE, NULL);

            /* seek if not last block */
            if (i != (info->track_size - 1))
                if (fseek (file, 256, SEEK_CUR) != 0)
                    return error_Message (TEO_ERROR_FILE_WRITE, NULL);
            size = 0;
        }
        last_val = val;
    }
    return 0;
}



static int write_ctrl_track (const char filename [], struct DISK_INFO *info)
{
    FILE *file;
    int err = 0;
    long int file_seek;

    if ((info->track < 0) || (info->track >= info->track_count))
        return error_Message (TEO_ERROR_DISK_IO, filename);

    file_seek = (
        track_list[teo.disk[info->drive].side>>1][info->track].offset << 9)
        + ((teo.disk[info->drive].side & 1) << 8);

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

    (void)fclose (file);
    return err;
}



static int file_mode_error (int error, const char filename[], FILE *file)
{
    fclose (file);
    return error_Message (error, filename);
}



/* file_mode:
 *  Loads the HFE header and returns the open mode.
 */
static int file_protection (const char filename[], int protection)
{
    FILE *file;
    
    protection = disk_CheckFile(filename, protection);

    if (protection >= 0)
    {
        /* check size of file */
        file = fopen (filename, "rb");
        if ((file == NULL)
         || (fseek (file, 0, SEEK_END) != 0)
         || (ftell (file) < 512*20))
            return file_mode_error (TEO_ERROR_FILE_FORMAT, filename, file);

        /* load header */
        fseek (file, 0, SEEK_SET);
        if (fread (hfe_hd.HEADERSIGNATURE, 1, 8, file) != 8)
            return file_mode_error (TEO_ERROR_FILE_READ, filename, file);

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
        (void)fclose (file);

        if (memcmp (hfe_hd.HEADERSIGNATURE, hfe_header, 8) != 0)
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);

        /* check formatrevision */
        if (hfe_hd.formatrevision != 0)
            return file_mode_error (TEO_ERROR_FILE_FORMAT, filename, file);
        
        /* check number_of_track */
        if (hfe_hd.number_of_track > TEO_DISK_TRACK_NUMBER_MAX)
            return error_Message (TEO_ERROR_FILE_FORMAT, filename);
    }
    return protection;
}


/* ------------------------------------------------------------------------- */


/* hfe_IsHfe:
 *  Check if the file is a HFE file.
 */
int hfe_IsHfe (const char filename[])
{
    return file_protection (filename, FALSE);
}



/* hfe_LoadDisk:
 *  Loads the HFE archive into the specified drive and
 *  forces the read-only mode if necessary.
 */
int hfe_LoadDisk(int drive, const char filename[])
{
    int i = 0;
    FILE *file;
    int  protection;

    protection = file_protection (filename, teo.disk[drive].write_protect);

    if (protection >= 0)
    {
        switch (hfe_hd.track_encoding)
        {
            case ISOIBM_FM_ENCODING :
            case EMU_FM_ENCODING :
                     return error_Message (TEO_ERROR_FILE_FORMAT, filename);
                     /*
                     disk[drive].info->sector_size  = 128;
                     disk[drive].info->byte_rate    = 125000/8;
                     */
                     break;

            case ISOIBM_MFM_ENCODING :
            case AMIGA_MFM_ENCODING :
            case UNKNOWN_ENCODING :
                     disk[drive].info->sector_size  = 256;
                     disk[drive].info->byte_rate    = 250000/8;
                     break;
            default :
                return error_Message (TEO_ERROR_FILE_FORMAT, filename);
        }
        disk[drive].info->fat_size    = hfe_hd.number_of_track*2;
        disk[drive].info->track_count = hfe_hd.number_of_track;
        disk[drive].info->track       = -1;  /* force track to be loaded */

        /* load track list */
        file = fopen (filename, "rb");
        (void)fseek (file, hfe_hd.track_list_offset<<9, SEEK_SET);
        for (i=0; i<hfe_hd.number_of_track; i++)
        {
            track_list[drive>>1][i].offset = hfe_fgetw (file);
            track_list[drive>>1][i].track_len = hfe_fgetw (file);
        }
        (void)fclose(file);

        /* check if write allowed disk */
        if (hfe_hd.write_allowed != 0xff)
            protection = TRUE;

        /* update parameters */
        teo.disk[drive].file = std_free (teo.disk[drive].file);
        teo.disk[drive].file = std_strdup_printf ("%s", filename);
        teo.disk[drive].write_protect = protection;
        disk[drive].state = TEO_DISK_ACCESS_HFE;
        disk[drive].ReadCtrlTrack = read_ctrl_track;
        disk[drive].WriteCtrlTrack = write_ctrl_track;
        disk[drive].ReadCtrlSector = NULL;
        disk[drive].WriteCtrlSector = NULL;
        disk[drive].FormatCtrlTrack = NULL;
        disk[drive].side_count = 2;
    }

    return protection;
}

