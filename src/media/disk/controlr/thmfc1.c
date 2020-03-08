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
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.5
 *  Créé par   : Francois Mouret 24/05/2012
 *  Modifié par: François Mouret 31/08/2013 31/07/2016
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
#include "logsys.h"
#include "hardware.h"
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

#if 0
#define DO_PRINT  1      /* if output wanted */
static struct MC6809_REGS regs;
#endif

static int disk_led = FALSE;



/* reset_controller_regs
 *  Reset the controller registers
 */
static void reset_controller_regs (int drive)
{
    disk[drive].dkc->process = 0;
    disk[drive].dkc->process_cpt = 0;
    disk[drive].dkc->write_door = 0;
    disk[drive].dkc->auto_count = 0;
    disk[drive].dkc->rr0 = STAT0_DRQ;
    disk[drive].dkc->rr1 = STAT1_DISK_CHANGE;
}



/* driver_still_running :
 *  Check if driver is still running.
 */
static int still_running (void)
{
    return (mc6809_clock() >= disk[dkcurr].drv->motor_stop) ? FALSE : TRUE;
}



/* stop_motor :
 *  Stop the motor.
 */
static void stop_motor (void)
{
    reset_controller_regs (dkcurr);
    disk[dkcurr].drv->motor_start = mc6809_clock();
    disk[dkcurr].drv->motor_stop = mc6809_clock();
}



/* _________________________ read/write function __________________________ */


static void flush_bus (struct DISK_SIDE *dsd)
{
    int pos;

    if (mc6809_clock() >= dsd->drv->motor_stop)
        return;

    /* compute track position */
    pos = (((dsd->byte_rate
            * (mc6809_clock()-dsd->drv->motor_start))
             / TEO_CPU_FREQ)
              + dsd->drv->pos.last)
               % dsd->track_size;

    /* manage DRQ bit */
    if (pos == dsd->drv->pos.curr)
        return;
    dsd->drv->pos.curr = pos;
    dsd->dkc->rr0 |= STAT0_DRQ;

    /* return disk not inserted */
    if (disk[dkcurr].state == TEO_DISK_ACCESS_NONE)
        return;

    /* write data if requested */
    if (dsd->dkc->write_door != 0)
    {
        if (teo.disk[dkcurr].write_protect == FALSE)
        {
            dsd->data[pos] = dsd->dkc->rr3;
            if (dsd->dkc->rr4 == MFM_DATA_CLOCK_VALUE)
                dsd->clck[pos] = DATA_CLOCK_MARK_WRITE;
            else
                dsd->clck[pos] = SYNCHRO_CLOCK_MARK_WRITE;
            disk_Written();
        }
    }
    else
    /* read data if requested */
    {
        dsd->dkc->rr3 = dsd->data[pos];
        if (dsd->clck[pos] < SYNCHRO_CLOCK_MARK)
            dsd->dkc->rr4 = MFM_DATA_CLOCK_VALUE;
        else
            dsd->dkc->rr4 = MFM_SYNCHRO_CLOCK_VALUE;
    }
}


/* ______________________ processes for auto functions _____________________ */


static void auto_check_synchro (struct DISK_SIDE *dsd)
{
    if (dsd->dkc->rr4 != MFM_DATA_CLOCK_VALUE)
    {
#ifdef DO_PRINT
        log_msgf(LOG_TRACE,"-----------------------------------\n");
        log_msgf(LOG_TRACE,"%02x auto_check_synchro\n", dsd->dkc->rr3);
#endif
        dsd->dkc->process++;
    }
    else
        dsd->dkc->process = 0;

    dsd->dkc->process_cpt = 0;
    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_info_id (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_check_info_id\n", dsd->dkc->rr3);
#endif
    if (dsd->dkc->rr3 == MFM_INFO_ID)
    {
        dsd->dkc->crc = MFM_CRC_INFO_INIT;
        dsd->dkc->process++;
    }
    else
        dsd->dkc->process = 0;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_read_address_wait_for_id (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_read_address_wait_for_id\n", dsd->dkc->rr3);
#endif
    if (dsd->dkc->rr3 == MFM_INFO_ID)
    {
        dsd->dkc->crc = MFM_CRC_INFO_INIT;
        dsd->dkc->auto_count = 4;
        dsd->dkc->rr0 |= STAT0_DREQ;
        dsd->dkc->process++;
    }
    else
        dsd->dkc->process = 0;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_track (struct DISK_SIDE *dsd)
{
    uint8 tmp_uint8 = (uint8)dsd->dkc->rr3;

#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_check_track\n", dsd->dkc->rr3);
#endif
    if (dsd->dkc->rr3 == dsd->dkc->wr6)
        dsd->dkc->process++;
    else
        dsd->dkc->process = 0;

    dsd->dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dsd->dkc->crc);

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_head (struct DISK_SIDE *dsd)
{
    uint8 tmp_uint8 = dsd->dkc->rr3;

#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_check_head\n", dsd->dkc->rr3);
#endif
    if (dsd->dkc->rr3 == ((dsd->dkc->wr1 & CMD1_AUTO_HEAD) >> 4))
        dsd->dkc->process++;
    else
        dsd->dkc->process = 0;

    dsd->dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dsd->dkc->crc);

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_sector (struct DISK_SIDE *dsd)
{
    uint8 tmp_uint8 = (uint8)dsd->dkc->rr3;

#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_check_sector\n", dsd->dkc->rr3);
#endif
    if (dsd->dkc->rr3 == dsd->dkc->wr5)
        dsd->dkc->process++;
    else
        dsd->dkc->process = 0;

    dsd->dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dsd->dkc->crc);

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_size (struct DISK_SIDE *dsd)
{
    uint8 tmp_uint8 = (uint8)dsd->dkc->rr3;

#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_check_size\n", dsd->dkc->rr3);
#endif
    if (dsd->dkc->rr3 == ((dsd->dkc->wr1 & CMD1_SECTOR_SIZE_MASK) >> 5))
        dsd->dkc->process++;
    else
        dsd->dkc->process = 0;

    dsd->dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dsd->dkc->crc);

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_high_rewind (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"auto_check_crc_high_rewind\n");
#endif
    if (dsd->dkc->rr3 == (uint8)(dsd->dkc->crc>>8))
        dsd->dkc->process++;
    else
        dsd->dkc->process = 0;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_low_rewind (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"auto_check_crc_low_rewind\n");
#endif
    if (dsd->dkc->rr3 == (uint8)dsd->dkc->crc)
        dsd->dkc->process++;
    else
        dsd->dkc->process = 0;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_high (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"auto_check_crc_high\n");
#endif
    if (dsd->dkc->rr3 != (uint8)(dsd->dkc->crc>>8))
        dsd->dkc->rr0 |= STAT0_CRC_ERROR;
    
    dsd->dkc->process++;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_check_crc_low (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"auto_check_crc_low\n");
#endif
    if (dsd->dkc->rr3 != (uint8)dsd->dkc->crc)
        dsd->dkc->rr0 |= STAT0_CRC_ERROR;
    
    dsd->dkc->process++;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_skip_27_datas (struct DISK_SIDE *dsd)
{
    if (++dsd->dkc->process_cpt == 27)
    {
#ifdef DO_PRINT
        log_msgf(LOG_TRACE,"auto_skip_27_datas\n");
#endif
        dsd->dkc->process++;
        dsd->dkc->process_cpt = 0;
    }

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_read_wait_42 (struct DISK_SIDE *dsd)
{
    if (++dsd->dkc->process_cpt == 42)
    {
#ifdef DO_PRINT
        log_msgf(LOG_TRACE,"auto_read_wait_42");
#endif
        dsd->dkc->process = 0;
        dsd->dkc->process_cpt = 0;
    }

    if (dsd->dkc->rr4 != MFM_DATA_CLOCK_VALUE)
    {
        dsd->dkc->process_cpt = 0;
        dsd->dkc->rr0 |= STAT0_DREQ;
        dsd->dkc->process++;
    }

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_read_wait_sector_id (struct DISK_SIDE *dsd)
{
    if (dsd->dkc->rr3 == MFM_SECTOR_ID)
    {
#ifdef DO_PRINT
        log_msgf(LOG_TRACE,"auto_read_wait_sector_id\n");
#endif
        dsd->dkc->crc = MFM_CRC_DATA_INIT;
        dsd->dkc->auto_count = 128<<((dsd->dkc->wr1&CMD1_SECTOR_SIZE_MASK)>>5);
        dsd->dkc->process++;
    }
}



static void auto_count_data (struct DISK_SIDE *dsd)
{
    uint8 tmp_uint8 = (uint8)dsd->dkc->rr3;
#ifdef DO_PRINT
    int x, y;
    static int pos = 0;
    static uint8 sector[1024];
#endif

    dsd->dkc->crc = disk_ComputeCrc (&tmp_uint8, 1, dsd->dkc->crc);

#ifdef DO_PRINT
    sector[pos++] = tmp_uint8;
#endif 

    if (--dsd->dkc->auto_count == 0)
    {
#ifdef DO_PRINT
        for (y=0; y<pos; y+=16)
        {
            log_msgf(LOG_TRACE,"%04d  ", y);

            for (x=0; x<16; x++)
            {
                log_msgf(LOG_TRACE,"%02x ", sector[y+x]);
            }
            log_msgf(LOG_TRACE," ");
            for (x=0; x<16; x++)
            {
                if ((sector[y+x] > 32) && (sector[y+x] < 0x7e))
                    log_msgf(LOG_TRACE,"%c", sector[y+x]);
                else
                    log_msgf(LOG_TRACE,".");
            }
            log_msgf(LOG_TRACE,"\n");
        }
        pos = 0;
#endif
        dsd->dkc->process++;
    }
}



static void auto_skip_22_datas (struct DISK_SIDE *dsd)
{
    if (++dsd->dkc->process_cpt == 22)
    {
#ifdef DO_PRINT
        log_msgf(LOG_TRACE,"auto_skip_22_datas\n");
#endif
        dsd->dkc->rr3 = MFM_PRE_SYNC_DATA_VALUE;
        dsd->dkc->rr4 = MFM_DATA_CLOCK_VALUE;
        dsd->dkc->write_door = 1;
        dsd->dkc->process++;
        dsd->dkc->process_cpt = 0;
    }
    
    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_pre_synchro (struct DISK_SIDE *dsd)
{
    if (++dsd->dkc->process_cpt == 12)
    {
#ifdef DO_PRINT
        log_msgf(LOG_TRACE,"auto_write_pre_synchro\n");
#endif
        dsd->dkc->rr3 = MFM_SYNCHRO_DATA_VALUE;
        dsd->dkc->rr4 = MFM_SYNCHRO_CLOCK_VALUE;
        dsd->dkc->process++;
    }
    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_ready (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"auto_write_ready\n");
#endif
    dsd->dkc->rr0 |= STAT0_DREQ;
    dsd->dkc->process++;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_wait_sector_id (struct DISK_SIDE *dsd)
{
    if (dsd->dkc->rr3 == MFM_SECTOR_ID)
    {
#ifdef DO_PRINT
        log_msgf(LOG_TRACE,"auto_write_wait_sector\n");
#endif
        dsd->dkc->rr4 = MFM_DATA_CLOCK_VALUE;
        dsd->clck[dsd->drv->pos.curr] = DATA_CLOCK_MARK_WRITE;
        dsd->dkc->crc = MFM_CRC_DATA_INIT;
        dsd->dkc->auto_count = 128<<((dsd->dkc->wr1&CMD1_SECTOR_SIZE_MASK)>>5);
        dsd->dkc->process++;
    }
}



static void auto_write_crc_high (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_write_crc_high\n", (int)dsd->dkc->rr3&0xff);
#endif
    dsd->dkc->rr3 = (uint8)(dsd->dkc->crc>>8);
    dsd->dkc->rr4 = MFM_DATA_CLOCK_VALUE;
    dsd->dkc->process++;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_write_crc_low (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"%02x auto_write_crc_low\n", (int)dsd->dkc->rr3&0xff);
#endif
    dsd->dkc->rr3 = (uint8)dsd->dkc->crc;
    dsd->dkc->rr4 = MFM_DATA_CLOCK_VALUE;
    dsd->dkc->process++;

    dsd->dkc->rr0 &= ~STAT0_DRQ;
}



static void auto_finished (struct DISK_SIDE *dsd)
{
#ifdef DO_PRINT
    log_msgf(LOG_TRACE,"auto_finished\n");
#endif
    dsd->dkc->rr0 |= STAT0_FINISHED | STAT0_DRQ;
    dsd->dkc->process = 0;
    dsd->dkc->process_cpt = 0;
    dsd->dkc->write_door = 0;
}



static void (*auto_read[])(struct DISK_SIDE *dsd) = {
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



static void (*auto_write[])(struct DISK_SIDE *dsd) = {
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



static void (*auto_read_address[])(struct DISK_SIDE *dsd) = {
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
    struct DISK_SIDE *dsd = &disk[dkcurr];

    disk_ReadTrack (dkcurr);

    flush_bus (dsd);

    /* only if disk inserted and motor on */
    if ((disk[dkcurr].state != TEO_DISK_ACCESS_NONE)
     && (mc6809_clock() < dsd->drv->motor_stop))
    {
        if ((dsd->dkc->rr0 & STAT0_DRQ) != 0)
        {
            if ((dsd->dkc->rr0 & STAT0_FINISHED) == 0)
            {
                switch (dsd->dkc->wr0 & CMD0_OP_MASK)
                {
                    case CMD0_OP_READ_ADDRESS :
                        ((*(auto_read_address[dsd->dkc->process]))(dsd));
                        break;

                    case CMD0_OP_READ_SECTOR :
                        ((*(auto_read[dsd->dkc->process]))(dsd));
                        break;

                    case CMD0_OP_WRITE_SECTOR :
                        ((*(auto_write[dsd->dkc->process]))(dsd));
                        break;
                }
            }
            dsd->dkc->rr0 &= ~STAT0_SYNCHRO_ON;
            if ((dsd->dkc->wr0 & CMD0_DETECT_SYNCHRO) != 0)
                if (dsd->dkc->rr4 != MFM_DATA_CLOCK_VALUE)
                    dsd->dkc->rr0 |= STAT0_SYNCHRO_ON;
        }
    }
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg0=$%02x\n", regs.pc, disk[dkcurr].dkc->rr0);
#endif
    return dsd->dkc->rr0;
}



/* get_reg1:
 *  Get value of STAT1 register.
 */
static int get_reg1 (void)
{
    /* clear all flags */
    disk[dkcurr].dkc->rr1 &= ~(STAT1_WRITE_PROTECTED
                             | STAT1_FLOPPY_INDEX
                             | STAT1_FLOPPY_TRACK0
                             | STAT1_READY);

    flush_bus(&disk[dkcurr]);

    /* only if disk inserted */
    switch (disk[dkcurr].state)
    {
        case TEO_DISK_ACCESS_NONE :
            break;

        /* special management for SAP files.
         A call to a SAP sector whose number is greater than
         16 (like in microbac_maths.sap or microbac_phys.sap)
         must generate a "No Disk" error (see version 1.8.1 of
         Teo and older). */
        case TEO_DISK_ACCESS_SAP :
            if ((LOAD_WORD(0x604A) < 80)
             && (LOAD_BYTE(0x604C) <= TEO_DISK_SECTOR_PER_TRACK))
                disk[dkcurr].dkc->rr1 |= STAT1_READY;
            break;

        /* special management for direct access */
        case TEO_DISK_ACCESS_DIRECT :
            if (teo_DirectReadSector != NULL)
                disk[dkcurr].dkc->rr1 |= STAT1_READY;
            break;
        
        default :
            disk[dkcurr].dkc->rr1 |= STAT1_READY;
            break;
    }

    /* only if motor on */
    if (mc6809_clock() < disk[dkcurr].drv->motor_stop)
    {
        /* manage disk protection */
        if ((teo.disk[dkcurr].write_protect == TRUE)
         || (disk[dkcurr].state == TEO_DISK_ACCESS_NONE))
        {
            disk[dkcurr].dkc->rr1 |= STAT1_WRITE_PROTECTED;
        }

        /* manage track 0 detection */
        if (disk[dkcurr].drv->track.curr == 0)
        {
            disk[dkcurr].dkc->rr1 |= STAT1_FLOPPY_TRACK0;
        }
    }

    /* manage index detection */
    if ((disk[dkcurr].drv->pos.curr == (disk[dkcurr].track_size-1))
     || (disk[dkcurr].drv->pos.curr < 5))
    {
        disk[dkcurr].dkc->rr1 |= STAT1_FLOPPY_INDEX;
    }

#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg1=$%02x\n", regs.pc, disk[dkcurr].dkc->rr1);
#endif
    return disk[dkcurr].dkc->rr1;
}



/* get_reg2:
 *  Get value of CMD2 register.
 */
static int get_reg2 (void)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg2=$%02x\n", regs.pc, disk[dkcurr].dkc->rr2);
#endif
    return disk[dkcurr].dkc->rr2;
}



/* get_reg3:
 *  Get value of RDATA register.
 */
static int get_reg3 (void)
{
    /* Update rr0 if motor on */
    if (mc6809_clock() < disk[dkcurr].drv->motor_stop)
        disk[dkcurr].dkc->rr0 &= ~STAT0_DRQ;

#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg3=$%02x\n", regs.pc, disk[dkcurr].dkc->rr3);
#endif
    return disk[dkcurr].dkc->rr3;
}



/* get_reg4:
 *  Get value of CLCK register (always returns value of rr3).
 */
static int get_reg4 (void)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg4=$%02x\n", regs.pc, disk[dkcurr].dkc->rr3);
#endif
    return disk[dkcurr].dkc->rr3;
}



/* get_reg5:
 *  Get value of WSECT register.
 *  Same register for input and output
 */
static int get_reg5 (void)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg5=$%02x\n", regs.pc, disk[dkcurr].dkc->rr5);
#endif
    return disk[dkcurr].dkc->rr5;
}



/* get_reg6:
 *  Get value of WTRCK register.
 *  Same register for input and output
 */
static int get_reg6 (void)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg6=$%02x\n", regs.pc, disk[dkcurr].dkc->rr6);
#endif
    return disk[dkcurr].dkc->rr6;
}



/* get_reg7:
 *  Get value of WCELL register.
 *  Same register for input and output
 */
static int get_reg7 (void)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg7=$%02x\n", regs.pc, disk[dkcurr].dkc->rr7);
#endif
    return disk[dkcurr].dkc->rr7;
}



/* get_reg8:
 *  Get value of register offset 8 (not used).
 *  Returns STAT0.
 */
static int get_reg8 (void)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg8=$%02x\n", regs.pc, disk[dkcurr].dkc->rr0);
#endif
    return disk[dkcurr].dkc->rr0;
}



/* get_reg9:
 *  Get value of register offset 9 (not used).
 *  Returns STAT1.
 */
static int get_reg9 (void)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X get_reg9=$%02x\n", regs.pc, disk[dkcurr].dkc->rr1);
#endif
    return disk[dkcurr].dkc->rr1;
}



/* set_reg0:
 *  Set CMD0 register.
 */
static void set_reg0 (int val)
{
    disk_ReadTrack (dkcurr);

#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg0=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->wr0 = val;

    reset_controller_regs (dkcurr);

    if ((val & CMD0_OP_MASK) == CMD0_OP_RESET)
    {
        /* activate write mode if requested */
        disk[dkcurr].dkc->write_door = (val & CMD0_WRITE) >> 2;
    }

    if (disk[dkcurr].state == TEO_DISK_ACCESS_DIRECT)
    {
        switch (val)
        {
            /* special management for direct access.
               Reading/writing a sector and formatting a track are
               immediately done */
            case 0x19 :   /* write sector */
                disk_WriteSector (dkcurr);
                break;
 
            case 0x1b :   /* read sector */
                disk_ReadSector (dkcurr);
                break;
 
            case 0x04 :   /* format track */
                disk_FormatTrack (dkcurr);
                break;
        }
    }
}



/* set_reg1:
 *  Set CMD1 register.
 */
static void set_reg1 (int val)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg1=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->wr1 = val;
}



/* disk_led_off:
 *  Switch the floppy led off.
 */
static void disk_led_off (void)
{
    if ((teo_SetDiskLed != NULL) && (disk_led == TRUE))
    {
        teo_SetDiskLed (FALSE);
        disk_led = FALSE;
    }
}



/* floppy_motor_on:
 *  Activate the floppy motor.
 */
static void floppy_motor_on (void)
{
    /* switch led on */
    if ((teo_SetDiskLed != NULL) && (disk_led == FALSE))
    {
        teo_SetDiskLed (TRUE);
        disk_led = TRUE;
    }

    /* activate floppy motor */
    if (mc6809_clock() >= disk[dkcurr].drv->motor_stop)
    {
        /* adjust last position */
        disk[dkcurr].drv->pos.last =
             ((disk[dkcurr].byte_rate * 2)
             + disk[dkcurr].drv->pos.last)
             % disk[dkcurr].track_size;
        disk[dkcurr].drv->motor_start = mc6809_clock();
    }
    /* 2 seconds more */
    disk[dkcurr].drv->motor_stop = mc6809_clock() + (TEO_CPU_FREQ*2);
}



static void select_drive (int drive)
{
    if (drive != dkcurr)
        disk_ReadTrack (drive);

    /* writing a valid drive code in CMD2 reactivate the motor */
    floppy_motor_on ();

}



/* set_reg2:
 *  Set CMD2 register.
 */
static void set_reg2 (int val)
{
    int side;
    int prev_val = disk[dkcurr].dkc->wr2;

#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg2=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->wr2 = val;

    /* drive selection */
    side = ((val & CMD2_FLOPPY_HEAD) == 0) ? 1 : 0;
    switch (val & CMD2_DRIVE)
    {
        case 0x02:
            select_drive (2+side);
            break;

        case 0x00 :
            disk_led_off ();
            return;
                
        default :
            select_drive (0+side);
            break;
    }

    /* activate motor */
    if (((prev_val ^ val) & CMD2_FLOPPY_MOTOR) != 0)
    {
        if ((val & CMD2_FLOPPY_MOTOR) != 0)
            floppy_motor_on ();
    }

    /* manage head moving */
    /* no track loaded until moving stops */
    if (((prev_val ^ val) & CMD2_STEP) != 0)
    {
        if (((val & CMD2_STEP) != 0)
         && (disk[dkcurr].drv->track.curr < disk[dkcurr].track_count))
        {
            if ((val & CMD2_HEAD_DIRECTION) != 0)
            {
                disk[dkcurr].drv->track.curr++;
            }
            else
            {
                if (disk[dkcurr].drv->track.curr > 0)
                {
                    disk[dkcurr].drv->track.curr--;
                }
            }
#ifdef DO_PRINT
            log_msgf(LOG_TRACE,"move head to track %d\n", disk[dkcurr].drv->track.curr);
#endif
        }
    }
}



/* set_reg3:
 *  Set WDATA register.
 *  Same register for input and output
 */
static void set_reg3 (int val)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg3=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->rr3 = val;

    /* Update rr0 if motor on */
    if (mc6809_clock() < disk[dkcurr].drv->motor_stop)
        disk[dkcurr].dkc->rr0 &= ~STAT0_DRQ;
}



/* set_reg4:
 *  Set CLCK register.
 *  Same register for input and output
 */
static void set_reg4 (int val)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg4=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->rr4 = val;

    /* Update rr0 if motor on */
    if (mc6809_clock() < disk[dkcurr].drv->motor_stop)
        disk[dkcurr].dkc->rr0 &= ~STAT0_DRQ;
}



/* set_reg5:
 *  Set WSECT register.
 *  Same register for input and output
 */
static void set_reg5 (int val)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg5=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->wr5 = val;
}



/* set_reg6:
 *  Set WTRCK register.
 *  Same register for input and output
 */
static void set_reg6 (int val)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg6=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->wr6 = val;
}



/* set_reg7:
 *  Set WCELL register.
 *  Same register for input and output
 */
static void set_reg7 (int val)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_reg7=$%02x\n", regs.pc, val);
#endif
    disk[dkcurr].dkc->wr7 = val;
}



/* set_nop:
 *  Set remain registers.
 */
static void set_nop (int val)
{
#ifdef DO_PRINT
    mc6809_GetRegs(&regs);
    log_msgf(LOG_TRACE,"%04X set_nop=$%02x\n", regs.pc, val);
#endif
    (void)val;
}


/* ------------------------------------------------------------------------- */


/* thmfc1_Init:
 *  Init a THMFC1 controller unit.
 */
void thmfc1_Init (int controller)
{
    /* initialize controller registers */
    reset_controller_regs (controller*4);

    /* setters */
    disk[controller*4].dkc->SetReg0 = set_reg0;
    disk[controller*4].dkc->SetReg1 = set_reg1;
    disk[controller*4].dkc->SetReg2 = set_reg2;
    disk[controller*4].dkc->SetReg3 = set_reg3;
    disk[controller*4].dkc->SetReg4 = set_reg4;
    disk[controller*4].dkc->SetReg5 = set_reg5;
    disk[controller*4].dkc->SetReg6 = set_reg6;
    disk[controller*4].dkc->SetReg7 = set_reg7;
    disk[controller*4].dkc->SetReg8 = set_nop;
    disk[controller*4].dkc->SetReg9 = set_nop;

    /* getters */
    disk[controller*4].dkc->GetReg0 = get_reg0;
    disk[controller*4].dkc->GetReg1 = get_reg1;
    disk[controller*4].dkc->GetReg2 = get_reg2;
    disk[controller*4].dkc->GetReg3 = get_reg3;
    disk[controller*4].dkc->GetReg4 = get_reg4;
    disk[controller*4].dkc->GetReg5 = get_reg5;
    disk[controller*4].dkc->GetReg6 = get_reg6;
    disk[controller*4].dkc->GetReg7 = get_reg7;
    disk[controller*4].dkc->GetReg8 = get_reg8;
    disk[controller*4].dkc->GetReg9 = get_reg9;

    /* functions */
    disk[controller*4].dkc->StillRunning = still_running;
    disk[controller*4].dkc->StopMotor = stop_motor;
}

