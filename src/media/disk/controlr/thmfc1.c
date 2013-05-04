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
 *  Module     : media/disk/controlr/thmfc1.c
 *  Version    : 1.8.2
 *  Créé par   : Francois Mouret 24/05/2012
 *  Modifié par:
 *
 *  Contrôleur THMFC1.
 */

/*
 * CONTROLLER REGISTERS
 *
 * CMD0 (write - offset 0)
 *   b7       = 0
 *   b6       = 0
 *   b5       = 0 MFM (double density)
 *              1 FM  (single density)
 *   b4       = 1 synchronization detection wanted
 *   b3       = 0 inhibition of synchronisation of data separator for formatting
 *   b2       = 1 writing activated
 *   b1 -> b0 = intelligent operation code
 *                00 controller reset
 *                01 data write
 *                10 info read
 *                11 data read
 *
 * CMD1 (write - offset 1)
 *   b7       = compatibility bit
 *   b6 -> b5 = Size of sector for intelligent operation
 *                00 128 chars/sector
 *                01 256 chars/sector
 *                10 512 chars/sector
 *                11 1024 chars/sector
 *   b4       = 0 side 0
 *              1 side 1
 *   b3 -> b1 = Precompensation by step of 62,5ns
 *                000 0 ns
 *                001 62,5 ns
 *                010 125 ns
 *                011 187,5 ns
 *                100 250 ns
 *                101 312,5 ns
 *                110 375 ns
 *                111 437,5 ns
 *   b0       = 1 system inhibition when READY signal is off
 *
 * CMD2 (write - offset 2)
 *   b7       = 0
 *   b6       = 0 turn QDD motor on / floppy side 1
 *            = 1 turn QDD motor off / floppy side 0
 *   b5       = 0 move head outward (floppy)
 *            = 1 move head inward (floppy)
 *   b4       = 0 step by step command off (floppy)
 *            = 1 step by step command on (floppy)
 *   b3       = 0
 *   b2       = 0 motor command off (floppy)
 *            = 1 motor command on (floppy)
 *   b1 -> b0 = Selection of drive (QDD and floppy)
 *                      01 first controller
 *                      10 second controller
 *
 * STAT0 (read - offset 0)
 *   b7       = character clock
 *                 0 = WDATA not ready or RDATA not sent
 *                 1 = WDATA ready or RDATA sent (transmission completed)
 *   b6       = 0
 *   b5       = 0
 *   b4       = intelligent operation is going to be completed
 *   b3       = intelligent operation is completed
 *   b2       = 1 CRC error
 *   b1       = character clock for intelligent operation
 *   b0       = 0 synchronization not detected
 *            = 1 synchronization detected
 *
 * STAT1 (read - offset 1)
 *   b7       = 0
 *   b6       = 1 Detection of index (floppy) / disk inserted (QDD)
 *   b5       = disk change (floppy)
 *   b4       = inversed copy of motor command
 *   b3       = 1 detection of track 0 (floppy) / detection of a QDD (QDD)
 *   b2       = 1 write protection (floppy / QDD)
 *   b1       = 1 READY information (floppy / QDD)
 *   b0       = 0
 *
 * WDATA/RDATA (read/write - offset 3)
 *   b7 -> b0 = data bus
 *
 * WCLK (write - offset 4)
 *   b7 -> b0 = character clock
 *              Generally : $FF for datas
 *                          $0A for synchronization
 *
 * WSECT (read/write - offset 5)
 *   b7 -> b0 = sector number for intelligent operation
 *              This number will be compared with the sector number written
 *              in the info field during the automatic operations.
 *
 * WTRK (read/write- offset 6)
 *   b7 -> b0 = track number for intelligent operation
 *              This number will be compared with the track number written
 *              in the info field during the automatic operations.
 *
 * WCELL (write - offset 7)
 *   b7       = 0 modification of data separator characteristics
 *              1 normal working of the separator
 *   b6 -> b0 = separator counter (on if b7 to 0)
 *                 $3F  = FM
 *                 $1F  = MFM
 *
 *
 * ==========================================================================
 *
 * DESCRIPTION OF THE INTELLIGENT FUNCTIONS
 *
 * Address read
 *   Address reading begins with an initialization phase, after which the
 *   control unit carries out the following operations :
 *   1. Search for three successive synchronizing characters.
 *   2. Comparison of the next character with the FE value.
 *   3. Activation of the DREQ output.
 *   The CPU must reply to the control unit by reading the RDATA register :
 *   this enables the DREQ output to be reactivated so that another character
 *   can be read.
 *   The transfer of data must take place within 25 microseconds of the DREQ
 *   being activated. If not, the control unit calculates the 2 bytes of CRC
 *   corresponding to the N bytes of data transferred, and positions the CRCER
 *   bit (bit 2 of STAT0).
 *
 * Sector read
 *   After initialization, the control unit carries out the following
 *   operations :
 *    1. Search for three successive synchronizing characters.
 *    2. Comparison of the next character with the FE value.
 *    3. Comparison of the next character with the content of the WTRCK register
 *    4. Comparison of bit 0 of the following character with bit SIDE of CMD1.
 *    5. Comparison of the following character with the content of the WSECT
 *       register
 *    6. Comparison of bit 0 and 1 of the following character with LG0 and LG1
 *       of the CMD1 register.
 *    7. CRC control of the 4 characters in the identification zone (the control
 *       unit forms the CRC calculated with the CRC written on the diskette).
 *    8. The control unit waits for 27 characters.
 *    9. Search for one synchronizing character. The control unit must find the
 *       synchronizing character within a delay corresponding to 42 characters;
 *       if not, the search sequence is reinitialized.
 *   10. Activation of the DREQ output.
 *   11. The CPU replies by reading the RDATA register, thereby enabling the
 *       DREQ to be reactivated and the characters from the data field to be
 *       transferred.
 *
 * Sector write
 *   The writing operation begins with the search phase for the identifiers,
 *   which is the same as the first 7 stages of the previous paragraph. Then the
 *   control unit carries out the following operations :
 *    8. The control unit waits for the time it takes to read 22 characters.
 *    9. The write gate is activated, and then the control unit writes 12
 *       synchronizing characters bit 00 following by 1 byte of synchronizing
 *       character.
 *   10. Activation of the DREQ output.
 *   11. The CPU must reply by writing the item of data in the WDATA write
 *       register, thereby enabling the DREQ output to be reactivated and the
 *       characters from the data field to be transferred onto the diskette.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
#endif

#include "teo.h"
#include "errors.h"
#include "std.h"
#include "main.h"
#include "hardware.h"
#include "media/disk/controlr.h"
#include "media/disk.h"

#define CMD0_ENCODING         0x20
#define CMD0_DETECT_SYNCHRO   0x10
#define CMD0_NO_SEPARATOR     0x08
#define CMD0_WRITE            0x04
#define CMD0_OP_MASK          0x03
#define CMD0_OP_RESET         0x00
#define CMD0_OP_WRITE_SECTOR  0x01
#define CMD0_OP_READ_ADDRESS  0x02
#define CMD0_OP_READ_SECTOR   0x03

#define STAT0_DRQ             0x80
#define STAT0_LAST_OPERATION  0x10
#define STAT0_FINISHED        0x08
#define STAT0_CRC_ERROR       0x04
#define STAT0_DREQ            0x02
#define STAT0_SYNCHRO_ON      0x01

#define CMD1_SECTOR_SIZE_MASK 0x60
#define CMD1_AUTO_HEAD        0x10
#define CMD1_PRECOMP_MASK     0x0E

#define STAT1_FLOPPY_INDEX    0x40
#define STAT1_QDD_DISK        0x40
#define STAT1_DISK_CHANGE     0x20
#define STAT1_MOTOR_IMAGE     0x10
#define STAT1_FLOPPY_TRACK0   0x08
#define STAT1_IS_QDD          0x08
#define STAT1_WRITE_PROTECTED 0x04
#define STAT1_READY           0x02

#define CMD2_FLOPPY_HEAD      0x40
#define CMD2_QDD_MOTOR        0x40
#define CMD2_HEAD_DIRECTION   0x20
#define CMD2_STEP             0x10
#define CMD2_FLOPPY_MOTOR     0x04
#define CMD2_DRIVE            0x03

#define WCELL_SEPARATOR_ON    0x80
#define WCELL_SEPARATOR_MASK  0x7f


// #define DO_PRINTF 1

static mc6809_clock_t clock = 0;
static int track_written = 0;
static int byte_written = 0;
static int ctrl = 0;
static int pos;
static int track_i = 0;
static uint8 tmp_uint8 = 0;

static struct MC6809_REGS regs;


/* __________________ read/write track/sector functions ___________________ */


/* clear_track:
 *  Clear the current track so that it will generate an I/O error
 */
static void clear_track (void)
{
    memset (dkc->info.data, 0x00, dkc->info.track_size);
    memset (dkc->info.data, 0x00, dkc->info.track_size);
}



/* write_update_track:
 *  Update the track if it has been written.
 */
static void write_update_track (void)
{
    int err = 0;

    if ((track_written != 0)
     && (dkc->info.track >= 0)
     && (dkc->info.track < dkc->info.track_count)
     && (disk[dkc->info.drive].WriteCtrlTrack != NULL))
    {
#ifdef DO_PRINTF
        printf ("writing track %d drive %d\n", dkc->info.track,
                                               dkc->info.drive);
        fflush (stdout);
#endif
        err = disk[dkc->info.drive].WriteCtrlTrack (
                            teo.disk[dkc->info.drive].file,
                            disk[dkc->info.drive].info);
        if (err < 0)
            main_DisplayMessage(teo_error_msg);
    }
    track_written = 0;
}



/* clear_write_flag:
 *  Clear the write_update flag.
 */
static void clear_write_flag (void)
{
    byte_written = 0;
}



/* write_update_timeout:
 *  Force to update the track if it has been written (no access left).
 */
static void write_update_timeout (void)
{
    static int counter = 0;

    if ((byte_written == 0) && (track_written != 0))
    {
        if ((dkc->wr0 & (CMD0_WRITE | CMD0_OP_MASK)) == 0)
        {
            if (++counter > 10) /* 1/5 second waiting */
            {
                write_update_track ();
                counter = 0;
            }
        }
    }
}



/* update_track:
 *  Update the track if it has been written and load the new track.
 */
static void update_track (void)
{
    int err = 0;

    if ((dkc->drive != dkc->info.drive)
     || (dkc->track[ctrl] != dkc->info.track))
    {
        write_update_track ();
        if ((dkc->track[ctrl] >= 0)
         && (dkc->track[ctrl] < dkc->info.track_count)
         && (disk[dkc->drive].ReadCtrlTrack != NULL))
        {
            dkc->info.drive = dkc->drive;
            dkc->info.track = dkc->track[ctrl];
            if (disk[dkc->drive].format == 0)
            {
#ifdef DO_PRINTF
               printf ("reading track %d (WTRK:%d)[current:%d] drive %d\n",
                              dkc->track[ctrl], dkc->wr6,
                            dkc->info.track, dkc->info.drive);
               fflush (stdout);
#endif
               err = disk[dkc->info.drive].ReadCtrlTrack (
                                teo.disk[dkc->info.drive].file,
                                disk[dkc->info.drive].info);
               if (err < 0)
               {
                   main_DisplayMessage(teo_error_msg);
                   clear_track();
               }
            }
        }
    }
}



/* write_sector:
 *  Write a sector.
 */
static void write_sector (void)
{
    int err = 0;

    if (disk[dkc->info.drive].WriteCtrlSector != NULL)
    {
        write_update_track ();
        dkc->info.drive = dkc->drive;
        dkc->info.track = dkc->track[ctrl];
        dkc->info.sector = LOAD_BYTE(0x604C);
#ifdef DO_PRINTF
        printf ("writing sector %d track %d drive %d\n",
                          dkc->info.sector,
                          dkc->info.track,
                          dkc->info.drive);
        fflush (stdout);
#endif
        err = disk[dkc->info.drive].WriteCtrlSector (
                            teo.disk[dkc->info.drive].file,
                            LOAD_WORD(0x604F),
                            disk[dkc->info.drive].info);
        if (err != 0)
            main_DisplayMessage(teo_error_msg);
    }
}



/* read_sector:
 *  Load a sector.
 */
static void read_sector (void)
{
    int err = 0;

    if ((dkc->track[ctrl] >= 0)
     && (dkc->track[ctrl] < dkc->info.track_count)
     && (disk[dkc->drive].ReadCtrlSector != NULL))
    {
        write_update_track ();
        dkc->info.drive = dkc->drive;
        dkc->info.track = dkc->track[ctrl];
        dkc->info.sector = LOAD_BYTE(0x604C);
#ifdef DO_PRINTF
       printf ("reading sector %d track %d (WTRK:%d)[current:%d] drive %d\n",
                    dkc->info.sector,
                    dkc->track[ctrl], dkc->wr6,
                    dkc->info.track, dkc->info.drive);
       fflush (stdout);
#endif

       err = disk[dkc->info.drive].ReadCtrlSector (
                        teo.disk[dkc->info.drive].file,
                        disk[dkc->info.drive].info);
       if (err < 0)
       {
           main_DisplayMessage(teo_error_msg);
           clear_track();
       }
    }
}



/* format_track:
 *  Update the track if it has been formatted.
 */
static int format_track (void)
{
    int err = 0;

    if ((dkc->drive != dkc->info.drive)
     || (dkc->track[ctrl] != dkc->info.track))
    {
        write_update_track ();
        if ((dkc->info.track >= 0)
         && (dkc->info.track < dkc->info.track_count)
         && (disk[dkc->info.drive].FormatCtrlTrack != NULL))
        {
            dkc->info.drive = dkc->drive;
            dkc->info.track = dkc->track[ctrl];
#ifdef DO_PRINTF
            printf ("formatting track %d drive %d\n", dkc->info.track,
                                               dkc->info.drive);
            fflush (stdout);
#endif
            err = disk[dkc->info.drive].FormatCtrlTrack (
                            teo.disk[dkc->info.drive].file,
                            disk[dkc->info.drive].info);
            if (err < 0)
                main_DisplayMessage(teo_error_msg);
        }
    }
    return err;
}



/* _________________________ read/write function __________________________ */


static void flush_bus (void)
{
    /* return if motor off or track not defined */
    if ((dkc->motor_clock[ctrl] != 0L)
     || (dkc->info.track_size == 0))
        return;

    /* compute track position */
    clock = mc6809_clock();
    pos = (((dkc->info.byte_rate * (clock-dkc->motor_clock[ctrl])) / TEO_CPU_FREQ)
             + dkc->last_pos[ctrl]) % dkc->info.track_size;

    /* manage DRQ bit */
    if (pos != track_i)
    {
        dkc->rr0 |= STAT0_DRQ;
        track_i = pos;

        /* write data if requested */
        if (dkc->write_door != 0)
        {
            dkc->info.data[pos] = dkc->rr3;
            if (dkc->rr4 == MFM_DATA_CLOCK_VALUE)
                dkc->info.clck[pos] = DATA_CLOCK_MARK_WRITE;
            else
                dkc->info.clck[pos] = SYNCHRO_CLOCK_MARK_WRITE;
            track_written = 1;
            byte_written = 1;
        }
        else
        /* read data if requested */
        {
            dkc->rr3 = dkc->info.data[pos];
            if (dkc->info.clck[pos] < SYNCHRO_CLOCK_MARK)
                dkc->rr4 = MFM_DATA_CLOCK_VALUE;
            else
                dkc->rr4 = MFM_SYNCHRO_CLOCK_VALUE;
        }
    }
}


/* ______________________ processes for auto functions _____________________ */


static void auto_check_synchro (void)
{
    if (dkc->rr4 != MFM_DATA_CLOCK_VALUE)
        dkc->process++;
    else
        dkc->process = 0;

    dkc->process_cpt = 0;
    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_info_id (void)
{
    if (dkc->rr3 == MFM_INFO_ID)
    {
        dkc->crc = MFM_CRC_INFO_INIT;
        dkc->process++;
    }
    else
        dkc->process = 0;
    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_read_address_wait_for_id (void)
{
    if (dkc->rr3 == MFM_INFO_ID)
    {
        dkc->crc = MFM_CRC_INFO_INIT;
        dkc->auto_count = 4;
        dkc->rr0 |= STAT0_DREQ;
        dkc->process++;
    }
    else
        dkc->process = 0;
    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_track (void)
{
    if (dkc->rr3 == dkc->wr6)
        dkc->process++;
    else
        dkc->process = 0;

    tmp_uint8 = (uint8)dkc->rr3;
    dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dkc->crc);

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_head (void)
{
    if (dkc->rr3 == ((dkc->wr1 & CMD1_AUTO_HEAD) >> 4))
        dkc->process++;
    else
        dkc->process = 0;

    tmp_uint8 =(uint8) dkc->rr3;
    dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dkc->crc);

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_sector (void)
{
    if (dkc->rr3 == dkc->wr5)
        dkc->process++;
    else
        dkc->process = 0;

    tmp_uint8 = (uint8)dkc->rr3;
    dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dkc->crc);

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_size (void)
{
    if (dkc->rr3 == ((dkc->wr1 & CMD1_SECTOR_SIZE_MASK) >> 5))
        dkc->process++;
    else
        dkc->process = 0;

    tmp_uint8 = (uint8)dkc->rr3;
    dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dkc->crc);

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_high_rewind (void)
{

    if (dkc->rr3 == (uint8)(dkc->crc>>8))
        dkc->process++;
    else
        dkc->process = 0;

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_low_rewind (void)
{
    if (dkc->rr3 == (uint8)dkc->crc)
        dkc->process++;
    else
        dkc->process = 0;

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_high (void)
{
    if (dkc->rr3 != (uint8)(dkc->crc>>8))
        dkc->rr0 |= STAT0_CRC_ERROR;
    
    dkc->process++;

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_low (void)
{
    if (dkc->rr3 != (uint8)dkc->crc)
        dkc->rr0 |= STAT0_CRC_ERROR;
    
    dkc->process++;

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_skip_27_datas (void)
{
    if (++dkc->process_cpt == 27)
    {
        dkc->process++;
        dkc->process_cpt = 0;
    }

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_read_wait_42 (void)
{
    if (++dkc->process_cpt == 43)
    {
        dkc->process = 0;
        dkc->process_cpt = 0;
    }

    if (dkc->rr4 != MFM_DATA_CLOCK_VALUE)
    {
        dkc->process_cpt = 0;
        dkc->rr0 |= STAT0_DREQ;
        dkc->process++;
    }

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_read_wait_sector_id (void)
{
    if (dkc->rr3 == MFM_SECTOR_ID)
    {
        dkc->crc = MFM_CRC_DATA_INIT;
        dkc->auto_count = 128 << ((dkc->wr1 & CMD1_SECTOR_SIZE_MASK) >> 5);
        dkc->process++;
    }
}



static void auto_count_data (void)
{
    tmp_uint8 = (uint8)dkc->rr3;
    dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dkc->crc);
    /* TODO  dkc->read_address_clock = clock; */

    if (--dkc->auto_count == 0)
        dkc->process++;
}



static void auto_skip_22_datas (void)
{
    if (++dkc->process_cpt == 22)
    {
        dkc->rr3 = MFM_PRE_SYNC_DATA_VALUE;
        dkc->rr4 = MFM_DATA_CLOCK_VALUE;
        dkc->write_door = 1;
        dkc->process++;
        dkc->process_cpt = 0;
    }
    
    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_pre_synchro (void)
{
    if (++dkc->process_cpt == 12)
    {
        dkc->rr3 = MFM_SYNCHRO_DATA_VALUE;
        dkc->rr4 = MFM_SYNCHRO_CLOCK_VALUE;
        dkc->process++;
    }
    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_ready (void)
{
    dkc->rr0 |= STAT0_DREQ;
    dkc->process++;

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_wait_sector_id (void)
{
    if (dkc->rr3 == MFM_SECTOR_ID)
    {
        dkc->rr4 = MFM_DATA_CLOCK_VALUE;
        dkc->info.clck[pos] = DATA_CLOCK_MARK_WRITE;
        dkc->crc = MFM_CRC_DATA_INIT;
        dkc->auto_count = 128 << ((dkc->wr1 & CMD1_SECTOR_SIZE_MASK) >> 5);
        dkc->process++;
    }
}



static void auto_write_crc_high (void)
{
    dkc->rr3 = (uint8)(dkc->crc>>8);
    dkc->rr4 = MFM_DATA_CLOCK_VALUE;
    dkc->process++;

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_crc_low (void)
{
    dkc->rr3 = (uint8)dkc->crc;
    dkc->rr4 = MFM_DATA_CLOCK_VALUE;
    dkc->process++;

    dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_finished (void)
{
    dkc->rr0 |= STAT0_FINISHED | STAT0_DRQ;
    dkc->process = 0;
    dkc->process_cpt = 0;
    dkc->write_door = 0;
}



static void (*auto_read_process[])(void) = {
    auto_check_synchro,
    auto_check_synchro,
    auto_check_synchro,
    auto_check_info_id,
    auto_check_track,
    auto_check_head,
    auto_check_sector,
    auto_check_size,
    auto_check_crc_high_rewind,
    auto_check_crc_low_rewind,
    auto_skip_27_datas,
    auto_read_wait_42,
    auto_read_wait_sector_id,
    auto_count_data,
    auto_check_crc_high,
    auto_check_crc_low,
    auto_finished
};



static void (*auto_write_process[])(void) = {
    auto_check_synchro,
    auto_check_synchro,
    auto_check_synchro,
    auto_check_info_id,
    auto_check_track,
    auto_check_head,
    auto_check_sector,
    auto_check_size,
    auto_check_crc_high_rewind,
    auto_check_crc_low_rewind,
    auto_skip_22_datas,
    auto_write_pre_synchro,
    auto_write_ready,
    auto_write_wait_sector_id,
    auto_count_data,
    auto_write_crc_high,
    auto_write_crc_low,
    auto_finished
};



static void (*auto_read_address_process[])(void) = {
    auto_check_synchro,
    auto_check_synchro,
    auto_check_synchro,
    auto_read_address_wait_for_id,
    auto_count_data,
    auto_check_crc_high,
    auto_check_crc_low,
    auto_finished
};


/* __________________________ registers management _________________________ */


/* get_reg0:
 *  Get value of STAT0 register.
 */
static int get_reg0 (void)
{
    update_track();

    /* only if motor on */
    if (dkc->motor_clock[ctrl] == 0L)
    {
        flush_bus ();

        if ((dkc->rr0 & STAT0_DRQ) != 0)
        {
            switch (dkc->wr0 & CMD0_OP_MASK)
            {
                case CMD0_OP_READ_ADDRESS :
                    if ((dkc->rr0 & STAT0_FINISHED) == 0)
                        ((*(auto_read_address_process[dkc->process]))());
                    break;

                case CMD0_OP_READ_SECTOR :
                    if ((dkc->rr0 & STAT0_FINISHED) == 0)
                        ((*(auto_read_process[dkc->process]))());
                    break;

                case CMD0_OP_WRITE_SECTOR :
                    if ((dkc->rr0 & STAT0_FINISHED) == 0)
                        ((*(auto_write_process[dkc->process]))());
                    break;
            }
            dkc->rr0 &= ~STAT0_SYNCHRO_ON;
            if ((dkc->wr0 & CMD0_DETECT_SYNCHRO) != 0)
                if (dkc->rr4 != MFM_DATA_CLOCK_VALUE)
                    dkc->rr0 |= STAT0_SYNCHRO_ON;
        }
    }
    return dkc->rr0;
}



/* get_reg1:
 *  Get value of STAT1 register.
 */
static int get_reg1 (void)
{
    /* only if motor on */
    if (dkc->motor_clock[ctrl] == 0L)
    {
        /* manage disk protection */
        dkc->rr1 &= ~STAT1_WRITE_PROTECTED;
        if (disk_Protection (dkc->drive, FALSE) == TRUE)
            dkc->rr1 |= STAT1_WRITE_PROTECTED;

        /* manage index detection */
        flush_bus();
        dkc->rr1 &= ~STAT1_FLOPPY_INDEX;
        if ((track_i == dkc->info.track_size) || (track_i < 5))
            dkc->rr1 |= STAT1_FLOPPY_INDEX;
    }
    return dkc->rr1;
}



/* get_reg2:
 *  Get value of CMD2 register.
 */
static int get_reg2 (void)
{
    return dkc->rr2;
}



/* get_reg3:
 *  Get value of RDATA register.
 */
static int get_reg3 (void)
{
    /* Update rr0 if motor on */
    if (dkc->motor_clock[ctrl] == 0L)
        dkc->rr0 &= ~STAT0_DRQ;

    return dkc->rr3;
}



/* get_reg4:
 *  Get value of CLCK register (always returns value of rr3).
 */
static int get_reg4 (void)
{
    return dkc->rr3;
}



/* get_reg5:
 *  Get value of WSECT register.
 *  Same register for input and output
 */
static int get_reg5 (void)
{
    return dkc->rr5;
}



/* get_reg6:
 *  Get value of WTRCK register.
 *  Same register for input and output
 */
static int get_reg6 (void)
{
    return dkc->rr6;
}



/* get_reg7:
 *  Get value of WCELL register.
 *  Same register for input and output
 */
static int get_reg7 (void)
{
    return dkc->rr7;
}



/* get_reg8:
 *  Get value of register offset 8 (not used).
 *  Returns STAT0.
 */
static int get_reg8 (void)
{
    return dkc->rr0;
}



/* get_reg9:
 *  Get value of register offset 9 (not used).
 *  Returns STAT1.
 */
static int get_reg9 (void)
{
    return dkc->rr1;
}



static void reset_controller_regs (struct DISK_CONTROLLER *controller)
{
    controller->process = 0;
    controller->process_cpt = 0;
    controller->write_door = 0;
    controller->auto_count = 0;
    controller->rr0 = STAT0_DRQ;
    controller->rr1 = STAT1_DISK_CHANGE;
}



/* set_reg0:
 *  Set CMD0 register.
 */
static void set_reg0 (int val)
{
    update_track();

    dkc->wr0 = val;

    reset_controller_regs (dkc);

    if ((val & CMD0_OP_MASK) == CMD0_OP_RESET)
    {
        /* activate write mode if requested */
        dkc->write_door = (val & CMD0_WRITE) >> 2;
    }

    switch (disk[dkc->drive].state)
    {
        /* special management for SAP files.
           Some programs need to have the post-sector bytes (at least the first
           4 bytes) set to 0xF7 (for example :  Avenger, Marche à l'ombre) */
        case TEO_DISK_ACCESS_SAP :
            if ((val == 0x1b) && (disk[dkc->drive].info->sector_size == 256))
            {
                mc6809_GetRegs(&regs);
                if ((regs.pc < 0xe004) && (dkc->wr5 >= 0) && (dkc->wr5 <= 16))
                    memset (disk[dkc->drive].info->data
                             +DDSECTORPOS(dkc->wr5)+MFM_SECTOR_SIZE-12,
                            0xf7, 12);
            }
            break;

        /* special management for direct access.
           Reading/writing a sector and formatting a track are
           immediately done */
        case TEO_DISK_ACCESS_DIRECT :
            mc6809_GetRegs(&regs);
            if (regs.pc > 0xe004)
            {
                switch (val)
                {
                    case 0x19 :   /* write sector */
                        write_sector ();
                        break;

                    case 0x1b :   /* read sector */
                        read_sector ();
                        break;

                    case 0x04 :   /* format track */
                        format_track ();
                        break;
                }
            }
            break;
    }
}



/* set_reg1:
 *  Set CMD1 register.
 */
static void set_reg1 (int val)
{
    dkc->wr1 = val;
}



/* floppy_motor_off:
 *  Stop the floppy motor.
 */
static void floppy_motor_off (void)
{
    /* only if motor on */
    if (dkc->motor_clock[ctrl] == 0L)
    {
        if (teo_SetDiskLed)
            teo_SetDiskLed(FALSE);

        /* the floppy motor is considered to be actually stopped
           only 2 seconds after the register has been written */
        dkc->motor_clock[ctrl] = mc6809_clock();

        dkc->rr1 &= ~STAT1_READY;
    }
}



/* floppy_motor_on:
 *  Activate the floppy motor.
 */
static void floppy_motor_on (void)
{
    switch (disk[dkc->drive].state)
    {
        /* stop motor if no disk inserted */
        case TEO_DISK_ACCESS_NONE :
            floppy_motor_off ();
            return;
        
        /* special management for SAP files.
         A call to a SAP sector whose number is greater than
         16 (like in microbac_maths.sap or microbac_phys.sap)
         must generate a "No Disk" error (see version 1.8.1 of
         Teo and older). */
        case TEO_DISK_ACCESS_SAP :
            if ((LOAD_WORD(0x604A) > 80) || (LOAD_BYTE(0x604C) > 16))
            {
                floppy_motor_off ();
                return;
            }
            break;

        /* special management for direct access */
        case TEO_DISK_ACCESS_DIRECT :
            if (teo_DirectReadSector == NULL)
            {
                floppy_motor_off ();
                return;
            }
            break;
    }

    /* switch on the disk led */
    if (teo_SetDiskLed)
        teo_SetDiskLed(TRUE);

    /* manage track position */
    clock = mc6809_clock();
    if ((dkc->motor_clock[ctrl] != 0L)
      && (clock > (dkc->motor_clock[ctrl]+(TEO_CPU_FREQ*2))))
    {
        /* add 2 seconds of rotation to last position */
        if (dkc->info.track_size != 0)
            dkc->last_pos[ctrl] = ((dkc->info.byte_rate * 2)
                                      + dkc->last_pos[ctrl])
                                        % dkc->info.track_size;
    }
    dkc->motor_clock[ctrl] = 0L;
    dkc->rr1 |= STAT1_READY;
}



/* set_reg2:
 *  Set CMD2 register.
 */
static void set_reg2 (int val)
{
    int prev_val = dkc->wr2;

    dkc->wr2 = val;

    /* selection of controller and side */
    if (val & 0x03)
    {
        switch (val & 0x43)
        {
            case 0x41: dkc->drive = 0; break;
            case 0x01: dkc->drive = 1; break;
            case 0x42: dkc->drive = 2; break;
            case 0x02: dkc->drive = 3; break;
        }
        ctrl = dkc->drive>>1;
        if (dkc->drive != dkc->info.drive)
            update_track();
        /* writing a valid drive code in CMD2 reactivate the motor */
        floppy_motor_on ();
    }
    else
    /* stop motor */
    if ((val == 0x00) || (val == 0x40))
    {
        floppy_motor_off ();
        return;
    }

    /* activate motor */
    if (((prev_val ^ val) & CMD2_FLOPPY_MOTOR) != 0)
    {
        if (((val & CMD2_FLOPPY_MOTOR) != 0)
         && (disk[dkc->drive].state > TEO_DISK_ACCESS_DIRECT))
            floppy_motor_on ();
    }
        
    /* manage head moving */
    /* no track loaded until moving stops */
    if (((prev_val ^ val) & CMD2_STEP) != 0)
    {
        if ((val & CMD2_STEP) != 0)
        {
            if ((val & CMD2_HEAD_DIRECTION) != 0)
            {
                if (dkc->track[ctrl] < dkc->info.track_count)
                {
                    dkc->track[ctrl]++;
                    write_update_track();
                }
                dkc->rr1 &= ~STAT1_FLOPPY_TRACK0;
            }
            else
            {
                if (dkc->track[ctrl] > 0)
                {
                    dkc->track[ctrl]--;
                    write_update_track();
                }
                else
                    dkc->rr1 |= STAT1_FLOPPY_TRACK0;
            }
        }
    }
    else
        update_track ();
}



/* set_reg3:
 *  Set WDATA register.
 *  Same register for input and output
 */
static void set_reg3 (int val)
{
    dkc->rr3 = val;

    /* Update rr0 if motor on */
    if (dkc->motor_clock[ctrl] == 0L)
        dkc->rr0 &= ~STAT0_DRQ;
}



/* set_reg4:
 *  Set CLCK register.
 *  Same register for input and output
 */
static void set_reg4 (int val)
{
    dkc->rr4 = val;

    /* Update rr0 if motor on */
    if (dkc->motor_clock[ctrl] == 0L)
        dkc->rr0 &= ~STAT0_DRQ;
}



/* set_reg5:
 *  Set WSECT register.
 *  Same register for input and output
 */
static void set_reg5 (int val)
{
    dkc->wr5 = val;
}



/* set_reg6:
 *  Set WTRCK register.
 *  Same register for input and output
 */
static void set_reg6 (int val)
{
    dkc->wr6 = val;
}



/* set_reg7:
 *  Set WCELL register.
 *  Same register for input and output
 */
static void set_reg7 (int val)
{
    dkc->wr7 = val;
}



/* set_nop:
 *  Set remain registers.
 */
static void set_nop (int val)
{
    (void)val;
}


/* ------------------------------------------------------------------------- */


void thmfc1_Update (void)
{
    ctrl = dkc->drive>>1;
}


/* thmfc1_Alloc:
 *  Allocate a THMFC1 controller.
 */
struct DISK_CONTROLLER *thmfc1_Alloc (void)
{
    static struct DISK_CONTROLLER *thmfc1 = NULL;

    /* allocate memory space */
    thmfc1 = calloc (sizeof(struct DISK_CONTROLLER), 1);
    if (thmfc1 == NULL)
    {
        (void)error_Message (TEO_ERROR_ALLOC, NULL);
        return NULL;
    }

    /* initialize controller registers */
    reset_controller_regs (thmfc1);

    /* setters */
    thmfc1->SetReg0 = set_reg0;
    thmfc1->SetReg1 = set_reg1;
    thmfc1->SetReg2 = set_reg2;
    thmfc1->SetReg3 = set_reg3;
    thmfc1->SetReg4 = set_reg4;
    thmfc1->SetReg5 = set_reg5;
    thmfc1->SetReg6 = set_reg6;
    thmfc1->SetReg7 = set_reg7;
    thmfc1->SetReg8 = set_nop;
    thmfc1->SetReg9 = set_nop;

    /* getters */
    thmfc1->GetReg0 = get_reg0;
    thmfc1->GetReg1 = get_reg1;
    thmfc1->GetReg2 = get_reg2;
    thmfc1->GetReg3 = get_reg3;
    thmfc1->GetReg4 = get_reg4;
    thmfc1->GetReg5 = get_reg5;
    thmfc1->GetReg6 = get_reg6;
    thmfc1->GetReg7 = get_reg7;
    thmfc1->GetReg8 = get_reg8;
    thmfc1->GetReg9 = get_reg9;

    /* functions */
    thmfc1->ClearWriteFlag = clear_write_flag;
    thmfc1->WriteUpdateTimeout = write_update_timeout;
    thmfc1->WriteUpdateTrack = write_update_track;
    
    return thmfc1;
}



/* thmfc1_Free:
 *  Free the memory for a THMFC1 controller.
 */
void thmfc1_Free (struct DISK_CONTROLLER *thmfc1)
{
    if (thmfc1 != NULL)
        std_free (thmfc1->info.data);

    std_free (thmfc1);
}

