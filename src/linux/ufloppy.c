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
 *  Module     : linux/disk.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 29/07/2000
 *  Modifié par: Eric Botcazou 05/11/2003
 *               Gilles Fétis 07/09/2011
 *               François Mouret 08/09/2011 28/12/2012
 *
 *  Lecture directe des disquettes Thomson.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <errno.h>
   #include <sys/ioctl.h>
   #include <sys/stat.h>
   #include <linux/fd.h>
   #include <linux/fdreg.h>
#endif

#include "teo.h"
#include "linux/sound.h"


#define DISK_RETRY    5
#define RESET_RETRY   5

static int fd[2] = {-1, -1};
static int drive_type[2];

#define SET_NO_MULTITRACK(lval)  (lval &= ~0x80)
#define IS_5_INCHES(drive) ((drive_type[drive]>0) && (drive_type[drive]<3))



/* reset_floppy:
 *  Réinitialise le lecteur de disquettes.
 */
static void reset_floppy(int drive)
{
    struct floppy_struct fd_prm;

    fd_prm.sect    = 8;
    fd_prm.head    = 2;
    fd_prm.track   = (IS_5_INCHES(drive) ? 40 : 80);
    fd_prm.size    = fd_prm.head * fd_prm.track * fd_prm.sect;
    fd_prm.stretch = 0;
    fd_prm.gap     = 0x1B;
    fd_prm.rate    = 0x3A;
    fd_prm.spec1   = 0xDF;
    fd_prm.fmt_gap = 0x2C;

    ioctl(fd[drive], FDSETPRM, &fd_prm);
}



/* open_floppy:
 *  Obtient le descripteur de fichier pour le lecteur de disquettes.
 */
static int open_floppy(int drive)
{
    char dev_str[16];

    snprintf(dev_str, sizeof(dev_str), "/dev/fd%d", drive);

    if ((fd[drive]=open(dev_str, O_RDWR | O_NDELAY))<0)
    {
        teo_DirectReadSector = NULL;
        teo_DirectWriteSector = NULL;
        teo_DirectFormatTrack = NULL;
        return 0;
    }

    reset_floppy(drive);
    return 1;
}



/* execute_command:
 *  Exécute la commande spécifiée via l'appel ioctl() FDRAWCMD.
 */
static int execute_command(int drive, struct floppy_raw_cmd *fd_cmd)
{
    int i, ret=-1;

    if (fd[drive]<0 && !open_floppy(drive))
        return 0x10;  /* lecteur non prêt */

    for (i=0; (i<DISK_RETRY)&&(ret!=0); i++)
    {
        if (i) reset_floppy(drive);
        ret=ioctl(fd[drive], FDRAWCMD, fd_cmd);
    }

    if (ret<0)
        return 0x10;  /* lecteur non prêt */

#ifdef DEBUG
    printf("fd_cmd reply: ");
    for (i=0; i<3; i++)
        printf("ST%d=%02x ", i, fd_cmd->reply[i]);
    printf("\n");
#endif

    switch (fd_cmd->reply[1])  /* ST1 */
    {
        case 0x01:  /* Missing Address Mark */
            return 0x04;   /* erreur sur l'adresse */

        case 0x02:  /* Write Protected */
            return 0x01;   /* disk protégé en écriture */

        case 0x04:  /* No Data - unreadable */
            return 0x08;   /* erreur sur les données */

        case 0x20:  /* CRC error in data or addr */
            if (fd_cmd->reply[2]==0x20)
                return 0x08;   /* erreur sur les données */
            else
                return 0x04;   /* erreur sur l'adresse */

        default:
            return 0;  /* OK */
    }
}



/* disk_command:
 *  Exécute la commande spécifiée (avec reset pour disquettes fatiguées)
 */
static int disk_command(int drive, struct floppy_raw_cmd *fd_cmd)
{
    int i,ret=-1;

    for (i=0;(i<RESET_RETRY)&&(ret!=0);i++)
    {
        if (i) reset_floppy(drive);
        ret = execute_command (drive,fd_cmd);
    }
    
    return ret;
}



/* read_sector:
 *  Lit le secteur spécifié sur la disquette.
 */
static int read_sector(int drive, int track, int sector, int nsects, unsigned char data[])
{
    struct floppy_raw_cmd fd_cmd;
    int pc_drive = drive/2;

    /* paramètres de commande */
    fd_cmd.flags  = FD_RAW_READ | FD_RAW_INTR | FD_RAW_NEED_SEEK;
    fd_cmd.data   = data;
    fd_cmd.length = 256*nsects; /* SECTOR_SIZE */
    fd_cmd.rate   = IS_5_INCHES(pc_drive) ? 1 : 2;
    fd_cmd.track  = IS_5_INCHES(pc_drive) ? track*2 : track;

    fd_cmd.cmd[0] = FD_READ;
    fd_cmd.cmd[1] = (drive%2) << 2;
    fd_cmd.cmd[2] = track;
    fd_cmd.cmd[3] = 0;  /* head */
    fd_cmd.cmd[4] = sector;
    fd_cmd.cmd[5] = 1;  /* SECTOR_SIZE >> 8 */
    fd_cmd.cmd[6] = 16; /* num sectors      */
    fd_cmd.cmd[7] = 0x1B;
    fd_cmd.cmd[8] = 0xFF;
    fd_cmd.cmd_count = 9;

    SET_NO_MULTITRACK(fd_cmd.cmd[0]);

    return disk_command(pc_drive, &fd_cmd);
}



/* write_sector:
 *  Ecrit le secteur spécifié sur la disquette.
 */
static int write_sector(int drive, int track, int sector, int nsects, const unsigned char data[])
{
    struct floppy_raw_cmd fd_cmd;
    int pc_drive = drive/2;

    /* paramètres de commande */
    fd_cmd.flags  = FD_RAW_WRITE | FD_RAW_INTR | FD_RAW_NEED_SEEK;
    fd_cmd.data   = (unsigned char *)data;
    fd_cmd.length = 256*nsects; /* SECTOR_SIZE */
    fd_cmd.rate   = IS_5_INCHES(pc_drive) ? 1 : 2;
    fd_cmd.track  = IS_5_INCHES(pc_drive) ? track*2 : track;

    fd_cmd.cmd[0] = FD_WRITE;
    fd_cmd.cmd[1] = (drive%2) << 2;
    fd_cmd.cmd[2] = track;
    fd_cmd.cmd[3] = 0;  /* head */
    fd_cmd.cmd[4] = sector;
    fd_cmd.cmd[5] = 1;  /* SECTOR_SIZE >> 8 */
    fd_cmd.cmd[6] = 16; /* num sectors      */
    fd_cmd.cmd[7] = 0x1B;
    fd_cmd.cmd[8] = 0xFF;
    fd_cmd.cmd_count = 9;

    SET_NO_MULTITRACK(fd_cmd.cmd[0]);

    return disk_command(pc_drive, &fd_cmd);
}



/* format_track:
 *  Formate la piste en utilisant la table des headers spécifiée.
 */
static int format_track(int drive, int track, const unsigned char header_table[])
{
    struct floppy_raw_cmd fd_cmd;
    int pc_drive = drive/2;

    /* paramètres de commande */
    fd_cmd.flags  = FD_RAW_WRITE | FD_RAW_INTR | FD_RAW_NEED_SEEK;
    fd_cmd.data   = (unsigned char *)header_table;
    fd_cmd.length = 64;
    fd_cmd.rate   = IS_5_INCHES(pc_drive) ? 1 : 2;
    fd_cmd.track  = IS_5_INCHES(pc_drive) ? track*2 : track;

    fd_cmd.cmd[0] = FD_FORMAT;
    fd_cmd.cmd[1] = (drive%2) << 2;
    fd_cmd.cmd[2] = 1;     /* SECTOR_SIZE >> 8 */
    fd_cmd.cmd[3] = 16;    /* num sectors      */
    fd_cmd.cmd[4] = 0x2C;  /* fmt_gap          */
    fd_cmd.cmd[5] = 0xE5;  /* filler_byte      */
    fd_cmd.cmd_count = 6;

    SET_NO_MULTITRACK(fd_cmd.cmd[0]);  /* nop */

    return disk_command(pc_drive, &fd_cmd);
}


/* ------------------------------------------------------------------------- */


/* ufloppy_Init:
 *  Initialise le module de lecture directe.
 */
int ufloppy_Init (int to_drive_type[4], int enable_write)
{
    struct floppy_drive_params fd_params;
    char dev_str[16];
    int i, num_drives = 0;

    teo_DirectReadSector  = NULL;
    teo_DirectWriteSector = NULL;
    teo_DirectFormatTrack = NULL;

    for (i=0; i<2; i++)
    {
        /* get drive type */
        snprintf(dev_str, sizeof(dev_str), "/dev/fd%d", i);

        if ((fd[i]=open(dev_str,  O_RDWR | O_NDELAY))<0)
        {
            if (errno != ENOENT)
                perror(dev_str);
            drive_type[i] = 0;

            to_drive_type[2*i] = 0;
            to_drive_type[2*i+1] = 0;
        }
        else
        {
            ioctl(fd[i], FDGETDRVPRM, &fd_params);
            close(fd[i]);
            fd[i] = -1;

            /* drive type: 1 (5"25 - 360 kb)
             *             2 (5"25 - 1.2 Mb)
             *             3 (3"5  - 720 kb)
             *             4 (3"5 - 1.44 Mb)
             *             5 (3"5 - 2.88 Mb)
             *             6 (3"5 - 2.88 Mb)
             */
            if (fd_params.cmos > 6)
            {
                drive_type[i] = 0;

                to_drive_type[2*i] = 0;
                to_drive_type[2*i+1] = 0;
            }
            else
            {
                drive_type[i] = fd_params.cmos;

                to_drive_type[2*i] = fd_params.cmos;
                to_drive_type[2*i+1] = fd_params.cmos;
            }

            num_drives++;
        }
    }

    teo_DirectReadSector = read_sector;

    if (enable_write)
    {
        teo_DirectWriteSector = write_sector;
        teo_DirectFormatTrack = format_track;
    }

    return num_drives;
}



/* ufloppy_Exit:
 *  Met au repos le module de lecture directe.
 */
void ufloppy_Exit (void)
{
    int i;

    teo_DirectReadSector  = NULL;
    teo_DirectWriteSector = NULL;
    teo_DirectFormatTrack = NULL;

    for (i=0; i<2; i++)
    {
        if (fd[i] >= 0)
        {
            close(fd[i]);
            fd[i] = -1;
        }
    }
}

