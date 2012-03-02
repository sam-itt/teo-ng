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
 *  Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume
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
 *  Module     : dos/disk.c
 *  Version    : 1.8.1
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 03/11/2003
 *
 *  Lecture directe des disquettes Thomson.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stddef.h>
   #include <bios.h>
   #include <dpmi.h>
   #include <sys/movedata.h>
#endif

#include "to8.h"


#define DPT_SIZE      11
#define DISK_RETRY     3

struct floppy_cmd {
    int cmd;
    int head;
    int track;
    int sector;
    int nsects;
    void *buffer;
};

static int drive_type[2];
static int dpt_addr = 0;
static unsigned char pc_dpt[DPT_SIZE];



/* SetDiskParameters:
 *  Initialise les paramètres disquette pour le format Thomson.
 */
static void SetDiskParameters(void)
{
    unsigned char to_dpt[DPT_SIZE];

    /* on récupère le vecteur 0x1E du BIOS qui pointe sur la table
       des paramètres de la disquette */
    _dosmemgetl(0x1E*4, 1, &dpt_addr);

    /* on sauvegarde les paramètres originaux */
    _dosmemgetb(dpt_addr, DPT_SIZE, pc_dpt);

    /* on fixe les nouveaux paramètres */
    to_dpt[0x0]=0xDF;  /* spec1                      */
    to_dpt[0x1]=0x02;  /* spec2                      */
    to_dpt[0x2]=0x25;  /* motor turn off delay       */
    to_dpt[0x3]=0x01;  /* 256 bytes/sector           */
    to_dpt[0x4]=0x10;  /* 16 sectors/track           */
    to_dpt[0x5]=0x1B;  /* gap between sectors        */
    to_dpt[0x6]=0xFF;  /* data length (ignored)      */
    to_dpt[0x7]=0x2C;  /* gap length when formatting */
    to_dpt[0x8]=0xE5;  /* filler byte                */
    to_dpt[0x9]=0x0F;  /* head settle time           */
    to_dpt[0xA]=0x08;  /* motor start time           */
    _dosmemputb(to_dpt, DPT_SIZE, dpt_addr);

    /* reset */
    biosdisk(0, 0, 0, 0, 1, 0, NULL);
}



/* ExecCommand:
 *  Exécute la commande spécifiée (via l'interruption 13h du BIOS).
 */
static int ExecCommand(int drive, struct floppy_cmd *fd_cmd)
{
    int i;
    int ret=0x10;

    if (!dpt_addr)
        SetDiskParameters();

    for (i=0; i<DISK_RETRY; i++)
    {
        ret=biosdisk(fd_cmd->cmd, drive, fd_cmd->head, fd_cmd->track,
                                  fd_cmd->sector, fd_cmd->nsects, fd_cmd->buffer);

        if (ret==0)  /* commande OK? */
            break;

        if ((i>1) && (ret==0x11))  /* commande non OK mais corrigée par ctrl? */
        {
            ret=0;
            break;
        }

        /* reset du lecteur */
        biosdisk(0, 0, 0, 0, 1, 0, NULL);
    }

    switch (ret)
    {
        case 0x02:  /* address mark not found */
            return 0x04;   /* erreur sur l'adresse */

        case 0x03:  /* disk write-protected */
            return 0x01;   /* disk protégé en écriture */

        case 0x04:  /* sector not found */
        case 0x07:  /* drive parameter activity failed */
        case 0x10:  /* data read (CRC or ECC) error */
        case 0x0A:  /* bad sector flag detected */
        case 0x0B:  /* bad track flag detected */
            return 0x08;   /* erreur sur les données */

        case 0x06:  /* floppy disk removed */
        case 0x80:  /* disk timed out or failed to respond */
            return 0x10;   /* lecteur non prêt */

        default:
            return 0;  /* OK */
    }
}



/* ReadSector:
 *  Lit le secteur spécifié sur la disquette.
 */
static int ReadSector(int drive, int track, int sector, int nsects, unsigned char data[])
{
    struct floppy_cmd fd_cmd;

    fd_cmd.cmd    = 2;
    fd_cmd.head   = drive%2;
    fd_cmd.track  = track;
    fd_cmd.sector = sector;
    fd_cmd.nsects = nsects;
    fd_cmd.buffer = data;

    return ExecCommand(drive/2, &fd_cmd);
}



/* WriteSector:
 *  Ecrit le secteur spécifié sur la disquette.
 */
static int WriteSector(int drive, int track, int sector, int nsects, const unsigned char data[])
{
    struct floppy_cmd fd_cmd;

    fd_cmd.cmd    = 3;
    fd_cmd.head   = drive%2;
    fd_cmd.track  = track;
    fd_cmd.sector = sector;
    fd_cmd.nsects = nsects;
    fd_cmd.buffer = (void *)data;

    return ExecCommand(drive/2, &fd_cmd);
}



/* FormatTrack:
 *  Formate la piste en utilisant la table des headers spécifiée.
 */
static int FormatTrack(int drive, int track, const unsigned char header_table[])
{
    struct floppy_cmd fd_cmd;
    int format_type = 0;

    /* sélection du format de formatage */
    switch (drive_type[drive/2])
    {
        case 1:
            /* 320/360 kb in 360 kb drive */
            format_type = 1;
            break;

        case 2:
            /* 320/360 kb in 1.2 Mb drive */
            format_type = 2;
            break;

        case 3:
        case 4:
        case 5:
        case 6:
            /* 720 kb in 720 kb/1.44 Mb/2.88 Mb drive */
            format_type = 4;
            break;
    }

    biosdisk(23, drive/2, 0, 0, 1, format_type, NULL);

    fd_cmd.cmd    = 5;
    fd_cmd.head   = drive%2;
    fd_cmd.track  = track;
    fd_cmd.sector = 1;
    fd_cmd.nsects = 16;
    fd_cmd.buffer = (void *)header_table;

    return ExecCommand(drive/2, &fd_cmd);
}



/* InitDirectDisk:
 *  Initialise le module de lecture directe.
 */
int InitDirectDisk(int to_drive_type[4], int enable_write)
{
    __dpmi_regs r;
    int i, num_drives = 0;

    for (i=0; i<2; i++)
    {
        /* get drive parameters (int 13h, function 08h) */
        r.h.ah = 0x08;
        r.h.dl = i;

        __dpmi_int(0x13, &r);

        if (r.x.flags&1)  /* CF set? */
        {
            drive_type[i] = 0;

            to_drive_type[2*i] = 0;
            to_drive_type[2*i+1] = 0;
        }
        else
        {
            /* drive type: 1 (5"25 - 360 kb)
             *             2 (5"25 - 1.2 Mb)
             *             3 (3"5  - 720 kb)
             *             4 (3"5 - 1.44 Mb)
             *             5 (3"5 - 2.88 Mb)
             *             6 (3"5 - 2.88 Mb)
             */
            if (r.h.bl > 6)
            {
               drive_type[i] = 0;

               to_drive_type[2*i] = 0;
               to_drive_type[2*i+1] = 0;
            }
            else
            {
               drive_type[i] = r.h.bl;

               to_drive_type[2*i] = r.h.bl;
               to_drive_type[2*i+1] = 0;  /* face 1 unsupported */
            }

            num_drives++;
        }
    }

    to8_DirectReadSector = ReadSector;

    if (enable_write)
    {
        to8_DirectWriteSector = WriteSector;
        to8_DirectFormatTrack = FormatTrack;
    }

    return num_drives;
}



/* ExitDirectDisk:
 *  Met au repos le module de lecture directe.
 */
void ExitDirectDisk(void)
{
    to8_DirectReadSector = NULL;
    to8_DirectWriteSector = NULL;
    to8_DirectFormatTrack = NULL;

    if (dpt_addr)
    {
        /* restaure les paramètres originaux */
        _dosmemputb(pc_dpt, DPT_SIZE, dpt_addr);

        /* reset */
        biosdisk(0, 0, 0, 0, 1, 0, NULL);

        dpt_addr = 0;
    }
}

