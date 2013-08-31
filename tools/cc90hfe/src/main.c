/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 François Mouret
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
 *  Module     : main.c
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 26/07/2013
 *
 *  Main functions.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <unistd.h>
   #include <stdlib.h>
   #ifdef DEBIAN_BUILD
      #include <sys/stat.h>
   #endif
#endif

#include "defs.h"
#include "std.h"
#include "ini.h"
#include "errors.h"
#include "option.h"
#include "encode.h"
#include "cc90.h"
#include "main.h"
#include "hfe.h"
#include "serial.h"

int is_fr;
struct DISK_INFO disk;
struct GUI_INFO gui;
int windowed_mode = 0;
FILE *fd_debug = NULL;

static int current_drive = 0;
static int archive_option = 0;
static int extract_option = 0;
static int install_option = 0;
static int version_option = 0;
static struct STRING_LIST *file_name_list = NULL;


static int main_RecordMessage (const char message[])
{
    error_msg = std_free (error_msg);
    error_msg = std_strdup_printf ("%s : %s", is_fr?"Erreur":"Error", message);
    return CC90HFE_ERROR;
}



static char *read_command_line(int argc, char *argv[])
{
    struct OPTION_ENTRY entries[] = {
        { "archive", 'a', OPTION_ARG_BOOL, &archive_option,
           is_fr?"Copie du Thomson vers un fichier HFE via CC90"
                :"Copy a Thomson disk onto a HFE file via CC90", NULL },
        { "extract", 'x', OPTION_ARG_BOOL, &extract_option,
           is_fr?"Copie d'un fichier HFE vers un Thomson via CC90"
                :"Copy an HFE file onto a Thomson disk via CC90", NULL },
        { "install", 'i', OPTION_ARG_BOOL, &install_option,
           is_fr?"Installe CC90 (avec INSTALL.BAS tournant sur le Thomson)"
                :"Install CC90 (with INSTALL.BAS running on the Thomson)", NULL },
        { "version", 'v', OPTION_ARG_BOOL, &version_option,
           is_fr?"Affiche la version du programme"
                :"Display the program version", NULL },
        { NULL, 0, 0, NULL, NULL, NULL }
    };
    return option_Parse (argc, argv, PROG_NAME, entries, &file_name_list);
}



static int console_check_file_name_list (void)
{
    int string_list_length = std_StringListLength (file_name_list);

    if (string_list_length == 0)
        return main_RecordMessage (is_fr?"Nom du fichier HFE manquant"
                                         :"Missing HFE file name");

    if (string_list_length > 1)
        return main_RecordMessage (is_fr?"Fichiers HFE trop nombreux"
                                         :"Too many HFE files");

    disk.file_name = std_free (disk.file_name);
    disk.file_name = std_strdup_printf ("%s", file_name_list->str);
    return 0;
}



/* console_exit_failure:
 *  Free all resources and exit with failure.
 */
static void console_exit_failure (void)
{
    printf ("%s\n", encode_String (error_msg));
    main_FreeAll ();
    exit (EXIT_FAILURE);
}


static void open_log (void)
{
    char *fname = NULL;

    fname = std_ApplicationPath (APPLICATION_DIR, "cc90hfe.log");
    fd_debug = fopen (fname, "wb");
    fname = std_free (fname);
}



static void close_log (void)
{
    std_fclose (fd_debug);
    fd_debug = NULL;
}


/* ------------------------------------------------------------------------- */


int main_ArchiveDisk (void)
{
    int err = 0;
    int track;

    if ((progress_on == TRUE)
     && ((err = hfe_WriteOpen(disk.file_name)) == 0))
    {
        open_log ();
        for (track=0;
             (track<disk.track_count)&&(progress_on!=0)&&(err==0);
             track++)
        {
             gui_ProgressUpdate (((track+1) * 100)/disk.track_count);
             if (progress_on == TRUE)
                if ((err = cc90_ReadTrack (current_drive, track)) == 0)
                    err = hfe_WriteTrack ();
        }
        close_log ();
        hfe_Close();
    }
    if (windowed_mode == 0)
        printf ("\n");

    progress_on = FALSE;
    
    return err;
}



int main_ExtractDisk (void)
{
    int err = 0;
    int track;

    if ((progress_on == TRUE)
     && ((err = hfe_ReadOpen(disk.file_name)) == 0))
    {
        for (track=0;
             (track<disk.track_count)&&(progress_on!=0)&&(err==0);
             track++)
        {
            gui_ProgressUpdate (((track+1) * 100)/disk.track_count);
            if (progress_on  == TRUE)
                if ((err = hfe_ReadTrack ()) == 0)
                    err = cc90_WriteTrack (current_drive, track);
        }
        hfe_Close();
    }
    if (windowed_mode == 0)
        printf ("\n");

    progress_on = FALSE;

    return err;
}



void main_ConsoleReadCommandLine (int argc, char *argv[])
{
    if (read_command_line (argc, argv) != NULL)
        console_exit_failure ();
}



void main_InitAll (void)
{
    windowed_mode = 0;

    /* init info structure */
    memset (&disk, 0x00, sizeof(struct DISK_INFO));
    disk.track_size  = MFM_TRACK_LENGTH>>2;
    disk.track_count = 80;
    memset (&gui, 0x00, sizeof(struct GUI_INFO));
    gui.timeout = SERIAL_TIME_OUT;
    gui.thomson_check = TRUE;

    /* load INI file */
    ini_Load ();
}        



void main_FreeAll (void)
{
    /* save INI file and free strings */
    ini_Save ();

    /* free string list */
    std_StringListFree (file_name_list);
    file_name_list = NULL;

    /* free file name */
    disk.file_name = std_free (disk.file_name);

    /* free error message */
    error_msg = std_free (error_msg);

    /* free tracks */
    disk.data[SIDE_0] = std_free (disk.data[SIDE_0]);
    disk.clck[SIDE_0] = std_free (disk.clck[SIDE_0]);
    disk.data[SIDE_1] = std_free (disk.data[SIDE_1]);
    disk.clck[SIDE_1] = std_free (disk.clck[SIDE_1]);
}



void main_Console (void)
{
    int err = 0;
    progress_on = 1;

    if (version_option != 0)
    {
        printf(PROG_NAME" version "PROG_VERSION_STRING"\n");
    }
    else
    if (install_option != 0)
    {
        if (cc90_Install () < 0)
            console_exit_failure ();
    }
    else
    if (archive_option != 0)
    {
        if (console_check_file_name_list() < 0)
            console_exit_failure ();
        if ((err = cc90_Open()) == 0)
            err = main_ArchiveDisk ();
        cc90_Close();
        if (err < 0)
            console_exit_failure ();
    }
    else
    if (extract_option != 0)
    {
        if (console_check_file_name_list() < 0)
            console_exit_failure ();
        if ((err = cc90_Open()) == 0)
            err = main_ExtractDisk ();
        cc90_Close();
        if (err < 0)
            console_exit_failure ();
    }
    else
    {
        main_RecordMessage (is_fr?"Argument Manquant":"Missing argument");
        console_exit_failure ();
    }
}
