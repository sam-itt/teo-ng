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
 *  Module     : disk.c
 *  Version    : 1.8.2
 *  Créé par   : Alexandre Pukall mai 1998
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 15/09/2006 26/01/2010 12/01/2012 25/04/2012
 *                               29/09/2012
 *               Samuel Devulder 05/02/2012 30/07/2011
 *
 *  Gestion du format SAP 2.0: lecture et écriture disquette.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <unistd.h>
   #include <dirent.h>
#endif

#include "media/disk.h"
#include "media/libsap.h"
#include "error.h"
#include "hardware.h"
#include "main.h"
#include "std.h"
#include "teo.h"

#ifdef UNIX_TOOL
#   define SLASH "/"
#else
#   define SLASH "\\"
#endif

/* paramètres physiques des lecteurs Thomson */
#define NBTRACK   80
#define NBSECT    16
#define SECTSIZE 256

#define SAPFS_NAME  "sapfs"

/* contrôleur de disquettes */
struct DISK_CTRL disk_ctrl;

/* type d'un lecteur */
typedef struct {
    enum {
        NO_DISK = 1,
        DIRECT_ACCESS,
        NORMAL_ACCESS
    } state;
    int mode;
    char *tmp;
} disk_t;

static disk_t disk[NBDRIVE];

/*****************************************/
/* émulation du contrôleur de disquettes */
/*****************************************/


void disk_ctrl_cmd0(int val)
{
    if (val==0x1B)
        disk_ctrl.prot=1;
    else if (val==0x18)
        disk_ctrl.prot=11;

    disk_ctrl.cmd0=val;

#ifdef DEBUG
    fprintf(stderr, "disk_ctrl.cmd0 = %02X\n", disk_ctrl.cmd0);
#endif
}



int disk_ctrl_stat0(void)
{
    switch (disk_ctrl.prot)
    {
        /* protection 0x1B (Avenger, Marche à l'ombre) */
        case 1:
            disk_ctrl.stat0=2;
            disk_ctrl.prot=2;
            break;

        case 2:
            disk_ctrl.stat0=0x80;
            disk_ctrl.rdata=0xFB;
            disk_ctrl.prot=3;
            break;

        case 3:
            disk_ctrl.rdata=0xF7;
            disk_ctrl.prot=0;
            break;

        /* protection 0x18 (Les Chevaliers) */
        case 11:
            disk_ctrl.stat0=0;
            disk_ctrl.prot=12;
            break;

        case 12:
            disk_ctrl.stat0=0x80;
            disk_ctrl.rdata=0xFE;
            disk_ctrl.prot=13;
            break;

        case 13:
            disk_ctrl.rdata=0x4F;
            disk_ctrl.prot=3;
            break;
    }

#ifdef DEBUG
    fprintf(stderr, "disk_ctrl.stat0 = %02X\n", disk_ctrl.stat0);
#endif

    return disk_ctrl.stat0;
}



int disk_ctrl_stat1(void)
{
    if (disk_ctrl.prot==11)
        disk_ctrl.stat1=0x40;

#ifdef DEBUG
    fprintf(stderr, "disk_ctrl.stat1 = %02X\n", disk_ctrl.stat1);
#endif

    return disk_ctrl.stat1;
}


#ifdef DEBUG

DISK_CTRL_SET_FUNC(cmd1)
DISK_CTRL_SET_FUNC(cmd2)
DISK_CTRL_SET_FUNC(wdata)
DISK_CTRL_SET_FUNC(wclk)
DISK_CTRL_SET_FUNC(wsect)
DISK_CTRL_SET_FUNC(wtrck)
DISK_CTRL_SET_FUNC(wcell)

int disk_ctrl_rdata(void)
{
    return disk_ctrl.rdata;
}

#endif


/***************************************/
/* gestion du format SAP               */
/***************************************/

#define SAP_HEADER_SIZE  66
#define SAP_SECT_SIZE   262
#define SAP_MAGIC_NUM  0xB3

static const char sap_header[]="\1SYSTEME D'ARCHIVAGE PUKALL S.A.P. "
                               "(c) Alexandre PUKALL Avril 1998";

/* type d'un secteur SAP */
typedef struct {
    unsigned char format;
    unsigned char protection;
    unsigned char track;
    unsigned char sector;
    unsigned char data[SECTSIZE];
    unsigned char crc1sect;
    unsigned char crc2sect;
} SAPSector_t;

/* table de calcul du CRC */
static short int crcpuk_temp;
static short int puktable[]={
   0x0000, 0x1081, 0x2102, 0x3183,
   0x4204, 0x5285, 0x6306, 0x7387,
   0x8408, 0x9489, 0xa50a, 0xb58b,
   0xc60c, 0xd68d, 0xe70e, 0xf78f
};



/* crc_pukall:
 *  Calcule le nouveau CRC à partir de la donnée c.
 */
static void crc_pukall(short int c)
{
    register short int index;

    index = (crcpuk_temp ^ c) & 0xf;
    crcpuk_temp = ((crcpuk_temp>>4) & 0xfff) ^ puktable[index];

    c >>= 4;

    index = (crcpuk_temp ^ c) & 0xf;
    crcpuk_temp = ((crcpuk_temp>>4) & 0xfff) ^ puktable[index];
}



/* do_crc:
 *  Calcule le CRC d'un secteur SAP.
 */
static void do_crc(SAPSector_t *sapsector)
{
    register int i;

    crcpuk_temp = 0xffff;

    crc_pukall(sapsector->format);
    crc_pukall(sapsector->protection);
    crc_pukall(sapsector->track);
    crc_pukall(sapsector->sector);

    for (i=0;i< SECTSIZE;i++)
       crc_pukall(sapsector->data[i]);
}



/* verify_sap_lect:
 *  Vérifie l'intégrité du secteur.
 */
static int verify_sap_lect(SAPSector_t *sapsector)
{
    do_crc(sapsector);

    if ((sapsector->crc1sect != ((crcpuk_temp>>8)&0xff)) || (sapsector->crc2sect != (crcpuk_temp&0xff)))
	return 8;

    if ((sapsector->format==4) || (sapsector->format==5))
	return 4;

    return 0;
}



/* verify_sap_ecri:
 *  Calcule le CRC du secteur.
 */
static int verify_sap_ecri(SAPSector_t *sapsector)
{
    if ((sapsector->protection==1) || (sapsector->protection==3))
  	return 1;
  
    if (sapsector->format==5)
	return 4;
  
    do_crc(sapsector);

    sapsector->crc1sect=(crcpuk_temp>>8)&0xff;
    sapsector->crc2sect=crcpuk_temp&0xff;

    return 0;
}



/* sap_get_sector:
 *  Lit un secteur sur le lecteur spécifié et retourne
 *  un code d'erreur moniteur TO8.
 */
static int sap_get_sector(int drive, SAPSector_t *sapsector)
{
    register int i;
    unsigned char buffer[SAP_SECT_SIZE];
    long pos;
    int err=0;
    char *fname;
    FILE *file;

    /* lecture du secteur dans le fichier */
    fname = (disk[drive].tmp == NULL) ? teo.disk[drive].file : disk[drive].tmp;
    if ((fname == NULL)
     || ((file=fopen(fname, "rb")) == NULL))
        return 4;
             
    pos = SAP_HEADER_SIZE + (sapsector->track*NBSECT+(sapsector->sector-1))*SAP_SECT_SIZE;

    fseek(file, pos, SEEK_SET);
    if (fread(buffer, sizeof(char), (size_t)SAP_SECT_SIZE, file) != SAP_SECT_SIZE) {
        fclose(file);
        return 4; }
    fclose(file);

    /* pour être portable on n'écrit pas directement dans une structure */
    sapsector->format=buffer[0];
    sapsector->protection=buffer[1];

    /* teste la différence d'info piste/secteur */
    if ((sapsector->track != buffer[2]) || (sapsector->sector != buffer[3]))
        err=4;

    sapsector->track=buffer[2];
    sapsector->sector=buffer[3];

    for (i=0;i<SECTSIZE;i++)
        sapsector->data[i]=buffer[4+i]^SAP_MAGIC_NUM;

    sapsector->crc1sect=buffer[4+i];
    sapsector->crc2sect=buffer[4+i+1];

    return err;
}



/* sap_put_sector:
 *  Ecrit un secteur sur le lecteur spécifié et retourne
 *  un code d'erreur moniteur TO8.
 */
static int sap_put_sector(int drive, SAPSector_t *sapsector)
{
   register int i;
   unsigned char buffer[SAP_SECT_SIZE];
   long pos;
   char *fname;
   FILE *file;

   buffer[0]=sapsector->format;
   buffer[1]=sapsector->protection;
   buffer[2]=sapsector->track;
   buffer[3]=sapsector->sector;

   for (i=0;i<SECTSIZE;i++)
       buffer[4+i]=sapsector->data[i]^SAP_MAGIC_NUM;

   buffer[4+i]=sapsector->crc1sect;
   buffer[4+i+1]=sapsector->crc2sect;
				
   /* écriture du secteur dans le fichier */
    fname = (disk[drive].tmp == NULL) ? teo.disk[drive].file : disk[drive].tmp;
    if ((fname == NULL)
     || (file=fopen(fname,"rb+")) == NULL)
       return 4;

   pos = SAP_HEADER_SIZE + (sapsector->track*NBSECT+(sapsector->sector-1))*SAP_SECT_SIZE;

   fseek(file, pos, SEEK_SET);
   fwrite(buffer, sizeof(char), (size_t)SAP_SECT_SIZE, file);
   fclose(file);

   return 0;
}



/* sap_format_track:
 *  Formate une piste sur le lecteur spécifié et retourne
 *  un code d'erreur moniteur TO8.
 */
static int sap_format_track(int drive, int track, unsigned char filler_byte)
{
    int i, sect;
    int err=0;
    SAPSector_t sapsector;

    sapsector.format = 0;
    sapsector.protection = 0;

    for (i=0; i<SECTSIZE; i++)
        sapsector.data[i]=filler_byte;

    for (sect=1; (sect<NBSECT+1)&&(err==0); sect++)
    {
        sapsector.track = track;
        sapsector.sector = sect;

        err=verify_sap_ecri(&sapsector);
        /* err est toujours égal à 0 ... */
            
        if (err==0)
            err=sap_put_sector(drive, &sapsector);
    }

    return err;
}



/* SapErrorMessage:
 *  Returns the error message corresponding to the specified SAP error.
 */
static int SapErrorMessage(int sap_err, const char more[])
{
   switch (sap_err) {
      case SAP_EBADF : return error_Message (TEO_ERROR_VALID_SAP, more);
      case SAP_EFBIG : return error_Message (TEO_ERROR_FILE_TOO_LARGE, more);
      case SAP_ENFILE: return error_Message (TEO_ERROR_FILE_EMPTY, more);
      case SAP_ENOENT: return error_Message (TEO_ERROR_FILE_NOT_FOUND, more);
      case SAP_ENOSPC: return error_Message (TEO_ERROR_DIRECTORY_FULL, more);
      case SAP_EPERM : return error_Message (TEO_ERROR_DISK_CREATE, more);
      default        : return error_Message (0, more); /* Unknown error */
   }
}



/* ExtractFile:
 *  Extracts one or more files from the specified archive.
 */
static int disk_ExtractSAP(const char sap_name[])
{
   int format;
   int err = 0;
   sapID id;
   const char all_files[] = "*";

   if ((id = sap_OpenArchive(sap_name, &format)) == SAP_ERROR)
      return SapErrorMessage (sap_errno, sap_name);

   if (sap_ExtractFile(id, all_files) == 0)
       err = SapErrorMessage (sap_errno, sap_name);

   sap_CloseArchive(id);

   return err;
}



/* disk_CreateAndFillSAP:
 *  Remplit une nouvelle archive SAP avec le contenu d'un répertoire.
 */
static int disk_CreateAndFillSAP (const char sap_name[], const char dir_name[],
                                  int dformat, int capacity)
{
    struct dirent *entry;
    DIR *dir;
    char *path_name = NULL;
    char *file_name = NULL;
    int err = 0;
    sapID id;
    
    if ((id = sap_CreateArchive(sap_name, dformat)) == SAP_ERROR)
        return SapErrorMessage (sap_errno, sap_name);        

    if ((err = sap_FormatArchive (id, capacity)) == SAP_ERROR)
        err = SapErrorMessage (sap_errno, sap_name);
    
    path_name = std_strdup_printf ("%s", dir_name);
    if (path_name != NULL)
    {
        std_CleanPath (path_name);
        dir = opendir(path_name);
        if (dir != NULL)
        {
            /* add every entry in turn */
            while ((err == 0) && ((entry = readdir(dir)) != NULL))
            {
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                {
                    file_name = std_strdup_printf ("%s%s%s", path_name, SLASH, entry->d_name);
                    if (sap_AddFile (id, file_name) == 0)
                        err = SapErrorMessage (sap_errno, entry->d_name);
                    file_name = std_free (file_name);
                }
            }
            closedir(dir);
        } else err = error_Message (TEO_ERROR_DISK_CREATE, sap_name);
        path_name = std_free (path_name);
    } else err = error_Message (TEO_ERROR_DISK_CREATE, sap_name);

    sap_CloseArchive(id);

    return err;
}


/**************************************/
/* émulation du Disk Operating System */
/**************************************/


/* ResetDiskCtrl:
 *  Initialise le contrôleur de disquettes.
 */
void ResetDiskCtrl(int *cc)
{
    *cc&=0xFE;
    STORE_BYTE(0x604E,'D');
}



/* ReadSector:
 *  Lit un secteur et modifie le registre d'état.
 */
void ReadSector(int *cc)
{
    register int i;
    int err = 0;
    int drive = (LOAD_BYTE(0x6048) == 1) ? 1 : LOAD_BYTE(0x6049);  /* Pour le lancement du boot sur le lecteur 1 */
    int dest_data = LOAD_WORD(0x604F);
    SAPSector_t sapsector;

    /* Standard C: l'initialisation de structures avec des expressions non constantes n'est pas supportée */
    sapsector.format = 0;
    sapsector.protection = 0;
    sapsector.track = LOAD_WORD(0x604A);
    sapsector.sector = LOAD_BYTE(0x604C);

#ifdef DEBUG
    fprintf(stderr, "ReadSector(): drive  = %d\n"
                    "              track  = %d\n"
                    "              sector = %d\n", drive, sapsector.track, sapsector.sector);
#endif

    if ((drive<0) || (drive>NBDRIVE) || (sapsector.track>=NBTRACK) || (sapsector.sector>NBSECT))
    {
        STORE_BYTE(0x604E, 0x10);
        *cc|=1;
        return;
    }

    if (to8_SetDiskLed)
        to8_SetDiskLed(TRUE);

    switch (disk[drive].state)
    {
        case NO_DISK:
        default:
#ifdef DEBUG
            fprintf(stderr, "ReadSector(): no disk in drive %d\n"
                            "               track  = %d\n"
                            "               sector = %d\n", drive, sapsector.track, sapsector.sector);
#endif
            err=0x10;
            break;

        case DIRECT_ACCESS:
            if (to8_DirectReadSector)
            {
                /* MSDOS: le BIOS a besoin d'un buffer de 512 octets */
                unsigned char direct_buffer[512];

                err=to8_DirectReadSector(drive, sapsector.track, sapsector.sector, 1, direct_buffer);
                if (err==0)
                    for (i=0;i<SECTSIZE;i++)
                        /* On utilise StoreByte plutôt que STORE_BYTE pour limiter l'accès en mémoire */
                        hardware_StoreByte(((dest_data+i)&0xFFFF), direct_buffer[i]);
            }
            else
                err=0x10;
            break;

        case NORMAL_ACCESS:
            err=sap_get_sector(drive, &sapsector);

            if (err==0)
                err=verify_sap_lect(&sapsector);

            if (err==0)
                for (i=0;i<SECTSIZE;i++)
                    /* On utilise StoreByte plutôt que STORE_BYTE pour limiter l'accès en mémoire */
                    hardware_StoreByte(((dest_data+i)%0xFFFF), sapsector.data[i]);
            break;
    }

    if (err==0)
    {
        STORE_BYTE(0x604E, 0);
        *cc&=0xFE;
    }
    else
    {
        STORE_BYTE(0x604E, err);
        *cc|=1;
    }

    if (to8_SetDiskLed)
        to8_SetDiskLed(FALSE);
}



/* WriteSector:
 *  Ecrit un secteur et modifie le registre d'état.
 */
void WriteSector(int *cc)
{
    register int i;
    int err = 0;
    int drive = LOAD_BYTE(0x6049);
    int source_data = LOAD_WORD(0x604F);
    SAPSector_t sapsector;

    /* Standard C: l'initialisation de structures avec des expressions non constantes n'est pas supportée */
    sapsector.format = 0;
    sapsector.protection = 0;
    sapsector.track = LOAD_WORD(0x604A);
    sapsector.sector = LOAD_BYTE(0x604C);

#ifdef DEBUG
    fprintf(stderr, "WriteSector(): drive  = %d\n"
                    "               track  = %d\n"
                    "               sector = %d\n", drive, sapsector.track, sapsector.sector);
#endif

    if ((drive<0) || (drive>NBDRIVE) || (sapsector.track>=NBTRACK) || (sapsector.sector>NBSECT))
    {
        STORE_BYTE(0x604E, 0x10);
        *cc|=1;
        return;
    }

    if (disk[drive].mode == TO8_READ_ONLY)
    {
        STORE_BYTE(0x604E, 1);
        *cc|=1;
        return;
    }

    if (to8_SetDiskLed)
        to8_SetDiskLed(TRUE);

    switch (disk[drive].state)
    {
        case NO_DISK:
        default:
#ifdef DEBUG
            fprintf(stderr, "WriteSector(): no disk in drive %d\n"
                            "                track  = %d\n"
                            "                sector = %d\n", drive, sapsector.track, sapsector.sector);
#endif
            err=0x10;
            break;

        case DIRECT_ACCESS:
            if (to8_DirectWriteSector)
            {
                /* MSDOS: le BIOS a besoin d'un buffer de 512 octets */
                unsigned char direct_buffer[512];

                for (i=0; i<SECTSIZE; i++)
                    direct_buffer[i]=LOAD_BYTE((source_data+i)&0xFFFF);

                /* MSDOS: nécessaire pour que le secteur soit lu par un TO8 réel */
                direct_buffer[SECTSIZE]=0;

                err=to8_DirectWriteSector(drive, sapsector.track, sapsector.sector, 1, direct_buffer);
            }
            else if (to8_DirectReadSector)
                err=0x01;  /* disque protégé en écriture */
            else
                err=0x10;
            break;

        case NORMAL_ACCESS:
            for (i=0; i<SECTSIZE; i++)
                sapsector.data[i]=LOAD_BYTE((source_data+i)&0xFFFF);

            err=verify_sap_ecri(&sapsector);
            /* err est toujours égal à 0 ... */
            
            if (err==0)
                err=sap_put_sector(drive, &sapsector);
            break;
    }

    if (err==0)
    {
        STORE_BYTE(0x604E, 0);
        *cc&=0xFE;
    }
    else
    {
        STORE_BYTE(0x604E, err);
        *cc|=1;
    }

    if (to8_SetDiskLed)
        to8_SetDiskLed(FALSE);
}



/* DiskNop:
 *  Ne fait rien en elle-même, se contente de modifier le registre d'état.
 */
void DiskNop(int *cc)
{
    int drive = LOAD_BYTE(0x6049);

    if ((drive<0) || (drive>NBDRIVE))
    {
        STORE_BYTE(0x604E, 0x10);
        *cc|=1;
        return;
    }

    if (disk[drive].state == NO_DISK)
    {
        STORE_BYTE(0x604E, 0x10);
        *cc|=1;
        return;
    }
    else
    {
        STORE_BYTE(0x604E, 0);
        *cc&=0xFE;
        return;
    }
}



/* BuildSectorMap:
 *  Construit la carte des secteurs d'une piste en fonction
 *  du facteur d'entrelacement.
 */
static void BuildSectorMap(int *sector_map, int factor)
{
    int sect, loc=0;

    /* mise à zéro de la table */
    memset(sector_map, 0, sizeof(int)*NBSECT);

    for (sect=1; sect<=NBSECT; sect++)
    {
        while (sector_map[loc] != 0)
            loc=(loc+1)%NBSECT;

        sector_map[loc]=sect;

        loc=(loc+factor)%NBSECT;
    }
}


/* octet de remplissage des pistes formatées du TO8 */
#define FILLER_BYTE 0xE5


/* FormatDrive:
 *  Formate un lecteur et modifie le registre d'état.
 */
void FormatDrive(int *cc)
{
    int err = 0;
    int drive = LOAD_BYTE(0x6049), track, sect, pos;
    int sector_map[NBSECT];
    /* MSDOS: le BIOS a besoin d'un buffer de 512 octets */
    unsigned char headers_table[512];

    if ((drive<0) || (drive>NBDRIVE))
    {
        STORE_BYTE(0x604E, 0x10);
        *cc|=1;
        return;
    }

    STORE_BYTE(0x6048, LOAD_BYTE(0x6048)&0x80);

    if (disk[drive].mode == TO8_READ_ONLY)
    {
        STORE_BYTE(0x604E, 1);
        *cc|=1;
        return;
    }

    /* construction de la carte des secteurs pour chaque piste,
       à partir du facteur d'entrelacement situé en 0x604D */
    BuildSectorMap(sector_map, LOAD_BYTE(0x604D));

    /* formatage des pistes */
    for (track=0; track<NBTRACK; track++)
    { 
	/* construction de la table des headers */
	for (sect=1, pos=0; sect<=NBSECT; sect++, pos+=4)
	{
	    headers_table[pos]   = track;
	    headers_table[pos+1] = 0;
	    headers_table[pos+2] = sector_map[sect-1];
	    headers_table[pos+3] = 1;
	}

        switch (disk[drive].state)
        {
            case NO_DISK:
            default:
                err=0x10;
                break;

            case DIRECT_ACCESS:
                if (to8_DirectFormatTrack)
                    err=to8_DirectFormatTrack(drive, track, headers_table);
                else if (to8_DirectReadSector)
                    err=0x01;  /* disque protégé en écriture */
                else
                    err=0x10;
                break;

            case NORMAL_ACCESS:
                err=sap_format_track(drive, track, FILLER_BYTE);
                break;
        }

        if (err)
            break;
    }

    if (err==0)
    {
        STORE_BYTE(0x604E, 0);
        *cc&=0xFE;
    }
    else
    {
        STORE_BYTE(0x604E, err);
        *cc|=1;
    }

    /* la construction de la piste 20 contenant le répertoire
       et la FAT est assurée par le TO8 lui-même */ 
}



/* CheckFile:
 *  Teste la présence et le mode d'accès du fichier.
 */
static int CheckFile(const char filename[], int mode)
{
    if (access (filename, F_OK) == 0)
        mode = (access (filename, W_OK) == 0) ? mode : TO8_READ_ONLY;
    else
        mode = error_Message (TEO_ERROR_FILE_OPEN, filename);

    return mode;
}



/* ------------------------------------------------------------------------- */


/* disk_SetDirect:
 *  Déclare le lecteur spécifié en accès direct et force
 *  le mode lecture seule pour le premier accès.
 */
int disk_SetDirect(int drive)
{ 
    if (disk[drive].state != DIRECT_ACCESS)
    {
        disk[drive].state = DIRECT_ACCESS;
            
        /* premier accès en lecture seule */
        disk[drive].mode = TO8_READ_ONLY;
    }

    return disk[drive].mode;
}



/* disk_SetVirtual:
 *  Déclare le lecteur spécifié en accès virtuel
 */
int disk_SetVirtual(int drive)
{ 
    if ((disk[drive].state != NORMAL_ACCESS)
     && (disk[drive].state != NO_DISK))
    {
        disk[drive].state = (teo.disk[drive].file == NULL) ? NO_DISK : NORMAL_ACCESS;
            
        /* premier accès en lecture seule */
        disk[drive].mode=TO8_READ_WRITE;
    }

    return disk[drive].mode;
}



/*
 *  Libère une disquette temporaire créé avec un répertoire.
 */
void disk_UnloadDir (int drive)
{
    int err = 0;
    char *cmd = NULL;
    
    /* nettoyage des fichiers d'auto-conversion directory->sap. */
    if ((access (SAPFS_NAME, F_OK) == 0)
     && (disk[drive].tmp != NULL))
    {
        main_SysExec(cmd, teo.disk[drive].file);
        err = disk_ExtractSAP(disk[drive].tmp);
        err = err;
        cmd = std_free (cmd);
        main_RmFile (disk[drive].tmp);
        disk[drive].tmp = std_free(disk[drive].tmp);
    }
    teo.disk[drive].file = std_free(teo.disk[drive].file);
}



/* disk_Eject:
 *  Ejecte l'archive SAP du le lecteur spécifié.
 */
void disk_Eject(int drive)
{
    disk_UnloadDir (drive);
    disk[drive].state = NO_DISK;
}



/*
 *  Libère toutes les disquettes temporaires créés avec un répertoire.
 */
void disk_UnloadAll (void)
{
    disk_UnloadDir (0);
    disk_UnloadDir (1);
    disk_UnloadDir (2);
    disk_UnloadDir (3);
}



/* disk_Check:
 *  Vérifie si le fichier est une disquette SAP.
 */
int disk_Check (const char filename[])
{
    int err = TRUE;
    FILE *file = NULL;
    size_t length;
    char header[SAP_HEADER_SIZE] = "";

    if (access (filename, F_OK) < 0)
        return TEO_ERROR_FILE_NOT_FOUND;

    if (std_IsFile (filename) == TRUE)
    {
        file=fopen(filename, "rb");

        /* on vérifie le header */ 
        length = fread(header, sizeof(char), SAP_HEADER_SIZE, file);
        fclose(file);

        if ((length != SAP_HEADER_SIZE)
         || (strncmp(header, sap_header, SAP_HEADER_SIZE) != 0))
             err = TEO_ERROR_FILE_FORMAT;
    }
    else
    if (std_IsDir (filename) != TRUE)
        err = TEO_ERROR_FILE_FORMAT;

    return err;
}



/* disk_Load:
 *  Charge l'archive SAP dans le lecteur spécifié et
 *  force si nécessaire le mode lecture seule.
 */
int disk_Load(int drive, const char filename[])
{
    int err = 0;
    int mode;
    char tmp[MAX_PATH+1] = "";
    
    mode = CheckFile(filename, disk[drive].mode);

    if (mode >= 0)
    {
        /* Conversion d'un dossier en fichier disquette temporaire. */
        if (std_IsDir (filename) == TRUE)
        {
            if (access(SAPFS_NAME, F_OK) != 0)
                return error_Message (TEO_ERROR_DISK_CREATE, SAPFS_NAME);

            if (main_TmpFile (tmp, MAX_PATH) == NULL)
                return error_Message (TEO_ERROR_DISK_CREATE, NULL);

            if (disk_CreateAndFillSAP (filename, tmp, SAP_FORMAT2, SAP_TRK80) < 0)
                return TEO_ERROR;

            disk_UnloadDir (drive);
            
            disk[drive].tmp = std_free (disk[drive].tmp);
            disk[drive].tmp = std_strdup_printf ("%s", tmp);
        }
        else
        {
            if ((err = disk_Check (filename)) < 0)
                return error_Message(err, filename);

            disk_UnloadDir (drive);
        }

        teo.disk[drive].file = std_free (teo.disk[drive].file);
        teo.disk[drive].file = std_strdup_printf ("%s", filename);
        disk[drive].state = NORMAL_ACCESS;
        disk[drive].mode = mode;
    }

    return mode;
}



/* disk_FirstLoad:
 *  Premier chargement des disquettes
 */
void disk_FirstLoad (void)
{
    int drive;
    char *name = NULL;

    for (drive=0; drive<NBDRIVE; drive++)
    {
        disk[drive].tmp = NULL;
        if (teo.disk[drive].file !=NULL)
        {
            name = std_strdup_printf ("%s", teo.disk[drive].file);
            teo.disk[drive].file = std_free (teo.disk[drive].file);
            if (name != NULL)
                if (disk_Load (drive, name) < 0)
                    main_DisplayMessage (teo_error_msg);
            name = std_free (name);
        }
    }
}



/* disk_SetMode:
 *  Fixe le mode d'accès à la disquette.
 *  (lecture seule ou lecture écriture)
 */ 
int disk_SetMode(int drive, int mode)
{
    int ret = NO_DISK;

    if (disk[drive].mode == mode)
        ret = disk[drive].mode;
    else
    {
        switch (disk[drive].state)
        {
            case NO_DISK:
                ret = disk[drive].mode = mode;
                break;

            case DIRECT_ACCESS:
                if ((mode == TO8_READ_WRITE) && !to8_DirectWriteSector)
                    mode = TO8_READ_ONLY;  
                  ret = disk[drive].mode = mode;
                break;

            case NORMAL_ACCESS:
            default:
                ret=CheckFile(teo.disk[drive].file, mode);
                if (ret >= 0)
                    disk[drive].mode = ret;
                break;
        }

    }
    return ret;
}


/* disk_Init:
 *  Initialise le module et met en place les trappes.
 */
void disk_Init(void)
{
    int drive;
        
    /* trap reset du contrôleur disk -> ResetDiskCtrl() */
    mem.mon.bank[0][0x00FE]=TO8_TRAP_CODE;
    mem.mon.bank[0][0x00FF]=0x39;

    /* trap écriture d'un secteur -> WriteSector() */
    mem.mon.bank[0][0x0187]=TO8_TRAP_CODE;
    mem.mon.bank[0][0x0188]=0x39;
    /* E177 conduit toujours à ce trap */
    mem.mon.bank[0][0x0180]=0x20; /* BRA */

    /* trap lecture d'un secteur -> ReadSector() */
    mem.mon.bank[0][0x03A7]=TO8_TRAP_CODE;
    mem.mon.bank[0][0x03A8]=0x39;

    /* trap formatage lecteur -> FormatDrive() + BRA >E515 */
    mem.mon.bank[0][0x04C8]=TO8_TRAP_CODE;
    mem.mon.bank[0][0x04C9]=0x20;
    mem.mon.bank[0][0x04CA]=0x4A;

    /* trap recherche piste 0 -> DiskNop() */
    mem.mon.bank[0][0x0134]=TO8_TRAP_CODE;
    mem.mon.bank[0][0x0135]=0x39;

    /* trap attente ready -> DiskNop() */
    mem.mon.bank[0][0x045A]=TO8_TRAP_CODE;
    mem.mon.bank[0][0x045B]=0x39;

    /* trap recherche piste effective -> DiskNop() */
    /* E452 conduit toujours à ce trap */
    mem.mon.bank[0][0x047A]=TO8_TRAP_CODE;
    mem.mon.bank[0][0x047B]=0x39;

    for (drive=0; drive<NBDRIVE; drive++)
    {
        disk[drive].state=NO_DISK;
        disk[drive].mode=TO8_READ_WRITE;
    }
}

