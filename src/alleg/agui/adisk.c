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
 *  Module     : alleg/agui/adisk.c
 *  Version    : 1.8.3
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               24/10/2012
 *
 *  Gestion des disquettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "defs.h"
#include "teo.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "media/disk/controlr.h"
#include "media/disk.h"
#include "media/disk/daccess.h"
#include "errors.h"
#include "std.h"
#include "teo.h"

/* Chemin du fichier de la disquette. */
static char filename[MAX_PATH+1] = "";

/* Masque de bits décrivant les lecteurs supportant la lecture directe. */
static int direct_disk = 0;

/* Boîte de dialogue. */
static DIALOG diskdial[]={
/*  dialog proc       x    y    w    h  fg bg  key flags  d1  d2 dp */
{ d_shadow_box_proc, 20,  10, 280, 180, 0, 0,   0,   0,    0, 0, NULL },
#ifdef FRENCH_LANGUAGE
{ d_ctext_proc,     160,  20,   0,   0, 0, 0,   0,   0,    0, 0, "Lecteurs de disquettes" },
#else
{ d_ctext_proc,     160,  20,   0,   0, 0, 0,   0,   0,    0, 0, "Disk drives" },
#endif
  /* disk 0 */
{ d_text_proc,       30,  44,   0,   0, 0, 0,   0,   0,    0, 0, "0:" },
{ d_button_proc,     47,  42,  15,  12, 0, 0,   0, D_EXIT, 0, 0, "x" },
{ d_textbox_proc,    64,  40, 130,  16, 0, 0,   0,   0,    0, 0, NULL },
{ d_button_proc,    202,  40,  20,  16, 0, 0,   0, D_EXIT, 0, 0, NULL },
{ d_button_proc,    228,  40,  30,  16, 0, 0, '0', D_EXIT, 0, 0, "..." },
{ d_check_proc,     260,  40,  15,  15, 0, 0,   0, D_EXIT, 0, 0, "" },
  /* disk 1 */
{ d_text_proc,       30,  68,   0,   0, 0, 0,   0,   0,    0, 0, "1:" },
{ d_button_proc,     47,  66,  15,  12, 0, 0,   0, D_EXIT, 0, 0, "x" },
{ d_textbox_proc,    64,  64, 130,  16, 0, 0,   0,   0,    0, 0, NULL },
{ d_button_proc,    202,  64,  20,  16, 0, 0,   0, D_EXIT, 0, 0, NULL },
{ d_button_proc,    228,  64,  30,  16, 0, 0, '1', D_EXIT, 0, 0, "..." },
{ d_check_proc,     260,  64,  15,  15, 0, 0,   0, D_EXIT, 0, 0, "" },
  /* disk 2 */
{ d_text_proc,       30,  92,   0,   0, 0, 0,   0,   0,    0, 0, "2:" },
{ d_button_proc,     47,  90,  15,  12, 0, 0,   0, D_EXIT, 0, 0, "x" },
{ d_textbox_proc,    64,  88, 130,  16, 0, 0,   0,   0,    0, 0, NULL },
{ d_button_proc,    202,  88,  20,  16, 0, 0,   0, D_EXIT, 0, 0, NULL },
{ d_button_proc,    228,  88,  30,  16, 0, 0, '2', D_EXIT, 0, 0, "..." },
{ d_check_proc,     260,  88,  15,  15, 0, 0,   0, D_EXIT, 0, 0, "" },
  /* disk 3 */
{ d_text_proc,       30, 116,   0,   0, 0, 0,   0,   0,    0, 0, "3:" },
{ d_button_proc,     47, 114,  15,  12, 0, 0,   0, D_EXIT, 0, 0, "x" },
{ d_textbox_proc,    64, 112, 130,  16, 0, 0,   0,   0,    0, 0, NULL },
{ d_button_proc,    202, 112,  20,  16, 0, 0,   0, D_EXIT, 0, 0, NULL },
{ d_button_proc,    228, 112,  30,  16, 0, 0, '3', D_EXIT, 0, 0, "..." },
{ d_check_proc,     260, 112,  15,  15, 0, 0,   0, D_EXIT, 0, 0, "" },
  /* direct disk */
#ifdef FRENCH_LANGUAGE
{ d_button_proc,     30, 136, 250,  16, 0, 0, 'd', D_EXIT, 0, 0, "Accès &direct" },
{ d_text_proc,      198,  30,   0,   0, 0, 0,   0,   0,    0, 0, "face" },
#else
{ d_button_proc,     30, 146, 250,  16, 0, 0, 'd', D_EXIT, 0, 0, "&Direct access" },
{ d_text_proc,      198,  30,   0,   0, 0, 0,   0,   0,    0, 0, "side" },
#endif
{ d_text_proc,      255,  30,   0,   0, 0, 0,   0,   0,    0, 0, "prot." },
{ d_button_proc,    210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" },
{ d_yield_proc,      20,  10,   0,   0, 0, 0,   0,   0,    0, 0, NULL },
{ NULL,               0,   0,   0,   0, 0, 0,   0,   0,    0, 0, NULL }
};

#define DISKDIAL_EJECT0   3
#define DISKDIAL_LABEL0   4
#define DISKDIAL_SIDE0    5
#define DISKDIAL_BUTTON0  6
#define DISKDIAL_CHECK0   7

#define DISKDIAL_EJECT1   9
#define DISKDIAL_LABEL1   10
#define DISKDIAL_SIDE1    11
#define DISKDIAL_BUTTON1  12
#define DISKDIAL_CHECK1   13

#define DISKDIAL_EJECT2   15
#define DISKDIAL_LABEL2   16
#define DISKDIAL_SIDE2    17
#define DISKDIAL_BUTTON2  18
#define DISKDIAL_CHECK2   19

#define DISKDIAL_EJECT3   21
#define DISKDIAL_LABEL3   22
#define DISKDIAL_SIDE3    23
#define DISKDIAL_BUTTON3  24
#define DISKDIAL_CHECK3   25

#define DISKDIAL_DIRECT   26

#define DISKDIAL_OK       29



/* update_params:
 *  Sauve les paramètres d'un disque.
 */
static void update_params (int dial_nbr, int drive)
{
    int state = diskdial[dial_nbr].flags & D_SELECTED;

    diskdial[dial_nbr].d2 = (state) ? 1 : 0;
    teo.disk[drive].write_protect = (state) ? TRUE : FALSE;
}



/* update_side_button:
 *  Update the button for side.
 */
static void update_side_button(int drive)
{
    int dial_nbr = DISKDIAL_SIDE0+(DISKDIAL_SIDE1-DISKDIAL_SIDE0)*drive;

    if (teo.disk[drive].side >= disk[drive].side_count)
        teo.disk[drive].side = disk[drive].side_count - 1;
    diskdial[dial_nbr].dp = std_free (diskdial[dial_nbr].dp);
    diskdial[dial_nbr].dp = std_strdup_printf ("%d", teo.disk[drive].side);
}



/* init_filename:
 *  Initialise le répertoire d'appel pour le fileselect.
 */
static void init_filename(int drive)
{
    *filename = '\0';

    if (teo.disk[drive].file != NULL)
        (void)std_snprintf (filename, MAX_PATH-1, "%s", teo.disk[drive].file);
    else
    if (teo.default_folder != NULL)
        (void)std_snprintf (filename, MAX_PATH-1, "%s\\", teo.default_folder);
    else
      if (file_exists(".\\disks", FA_DIREC, NULL))
        (void)std_snprintf (filename, MAX_PATH-1, "%s", ".\\disks\\");
}


/* ------------------------------------------------------------------------- */


/* adisk_Panel:
 *  Affiche le menu de gestion des lecteurs de disquettes.
 */
void adisk_Panel(void)
{
    static int first=1;
    int drive, ret, ret2;
    int dial_nbr;
    char *name = NULL;
    
    if (first)
    {
        centre_dialog(diskdial);

        for (drive=0; drive<4; drive++)
        {
            init_filename(drive);

            /* init disk name */
            dial_nbr = DISKDIAL_LABEL0+(DISKDIAL_LABEL1-DISKDIAL_LABEL0)*drive;
            if ((filename != NULL) && (*filename != '\0'))
                diskdial[dial_nbr].dp = std_strdup_printf ("%s" , get_filename(filename));
            name = (char *)diskdial[dial_nbr].dp;
            if ((name == NULL) || (*name == '\0'))
            {
                diskdial[dial_nbr].dp = std_free (diskdial[dial_nbr].dp);
                diskdial[dial_nbr].dp = std_strdup_printf ("%s", is_fr?"(Aucun)":"(None)");
                teo.disk[drive].side = 0;
                disk[drive].side_count = 1;
            }

            /* init disk protection */
            dial_nbr = DISKDIAL_CHECK0+(DISKDIAL_CHECK1-DISKDIAL_CHECK0)*drive;
            if (teo.disk[drive].write_protect)
                diskdial[dial_nbr].flags |= D_SELECTED;
            update_params (dial_nbr, drive);

            /* init disk side */
            update_side_button(drive);
        }
	    first=0;
    }

    clear_keybuf();

    while (TRUE)
    {
        ret=popup_dialog(diskdial, DISKDIAL_OK);
        
        switch (ret)
        {
            case DISKDIAL_EJECT0:
            case DISKDIAL_EJECT1:
            case DISKDIAL_EJECT2:
            case DISKDIAL_EJECT3:
                drive=(ret-DISKDIAL_EJECT0)/(DISKDIAL_EJECT1-DISKDIAL_EJECT0);
                dial_nbr = DISKDIAL_LABEL0+(DISKDIAL_LABEL1-DISKDIAL_LABEL0)*drive;
                diskdial[dial_nbr].dp = std_free (diskdial[dial_nbr].dp);
                diskdial[dial_nbr].dp = std_strdup_printf ("%s", is_fr?"(Aucun)":"(None)");
                disk_Eject(drive);
                break;

            case DISKDIAL_BUTTON0:
            case DISKDIAL_BUTTON1:
            case DISKDIAL_BUTTON2:
            case DISKDIAL_BUTTON3:
                drive=(ret-DISKDIAL_BUTTON0)/(DISKDIAL_BUTTON1-DISKDIAL_BUTTON0);
                init_filename(drive);
                std_CleanPath (filename);
                strcat (filename, "\\");
                if (file_select_ex(is_fr?"Choisissez votre disquette:":"Choose your disk:",
                                   filename, "sap;hfe;fd;qd", MAX_PATH,
                                   OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    ret2=disk_Load (drive, filename);

                    if (ret2 < 0)
                        agui_PopupMessage(teo_error_msg);
                    else
                    {
                        dial_nbr = DISKDIAL_EJECT0+(DISKDIAL_EJECT1-DISKDIAL_EJECT0)*drive;
                        diskdial[dial_nbr].flags &= ~D_DISABLED;

                        dial_nbr = DISKDIAL_LABEL0+(DISKDIAL_LABEL1-DISKDIAL_LABEL0)*drive;
                        diskdial[dial_nbr].dp = std_free (diskdial[dial_nbr].dp);
                        diskdial[dial_nbr].dp = std_strdup_printf ("%s", get_filename(filename));
                        teo.default_folder = std_free (teo.default_folder);
                        teo.default_folder = std_strdup_printf ("%s", filename);
                        std_CleanPath (teo.default_folder);

                        dial_nbr = DISKDIAL_CHECK0+(DISKDIAL_CHECK1-DISKDIAL_CHECK0)*drive;
                        diskdial[dial_nbr].flags &= ~D_SELECTED;
                        if (ret2==TRUE)
                        {
                            agui_PopupMessage(is_fr?"Attention: écriture impossible."
                                                   :"Warning: writing unavailable.");
                            diskdial[dial_nbr].flags|=D_SELECTED;
                        }
                        update_params (dial_nbr, drive);
                        update_side_button(drive);
                    }
                }
                break;

            case DISKDIAL_SIDE0:
            case DISKDIAL_SIDE1:
            case DISKDIAL_SIDE2:
            case DISKDIAL_SIDE3:
                drive=(ret-DISKDIAL_SIDE0)/(DISKDIAL_SIDE1-DISKDIAL_SIDE0);
                teo.disk[drive].side++;
                if (teo.disk[drive].side >= disk[drive].side_count)
                    teo.disk[drive].side = 0;
                disk_ControllerWriteUpdateTrack();
                disk[drive].info->track = -1;
                update_side_button(drive);
                break;

            case DISKDIAL_CHECK0:
            case DISKDIAL_CHECK1:
            case DISKDIAL_CHECK2:
            case DISKDIAL_CHECK3:
                drive=(ret-DISKDIAL_CHECK0)/(DISKDIAL_CHECK1-DISKDIAL_CHECK0);
            
                diskdial[ret].flags|=D_SELECTED;
                if (diskdial[ret].d2)
                {
                    diskdial[ret].flags&=~D_SELECTED;
                    if (disk_Protection(drive, FALSE) == TRUE)
                    {
                        agui_PopupMessage(is_fr?"Ecriture impossible sur ce support."
                                               :"Writing unavailable on this device.");
                        diskdial[ret].flags|=D_SELECTED;
                    }
                }
                update_params (ret, drive);
                break;

            case DISKDIAL_DIRECT:
                for (drive=0; drive<4; drive++)
                {
                    if (direct_disk & (1<<drive))
                    {
                        dial_nbr = DISKDIAL_EJECT0+(DISKDIAL_EJECT1-DISKDIAL_EJECT0)*drive;
                        diskdial[dial_nbr].flags |= D_DISABLED;

                        dial_nbr = DISKDIAL_LABEL0+(DISKDIAL_LABEL1-DISKDIAL_LABEL0)*drive;
                        diskdial[dial_nbr].dp = std_free (diskdial[dial_nbr].dp);
                        diskdial[dial_nbr].dp = std_strdup_printf ("%s",
                                                    is_fr?"Accès Direct":"Direct Access");
                        teo.disk[drive].file = std_free (teo.disk[drive].file);

                        dial_nbr = DISKDIAL_CHECK0+(DISKDIAL_CHECK1-DISKDIAL_CHECK0)*drive;

                        diskdial[dial_nbr].flags&=~D_SELECTED;
                        if (daccess_LoadDisk (drive, "") == TRUE)
                            diskdial[dial_nbr].flags|=D_SELECTED;
                        update_params (dial_nbr, drive);
                    }
                }
                break;

            case -1:  /* ESC */
            case DISKDIAL_OK:
                return;
        }
    }
}


/* adisk_SetColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void adisk_SetColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(diskdial, fg_color, bg_color);
    diskdial[DISKDIAL_LABEL0].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL1].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL2].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL3].bg = bg_entry_color;
    diskdial[DISKDIAL_SIDE0].bg = bg_entry_color;
    diskdial[DISKDIAL_SIDE1].bg = bg_entry_color;
    diskdial[DISKDIAL_SIDE2].bg = bg_entry_color;
    diskdial[DISKDIAL_SIDE3].bg = bg_entry_color;
}



/* adisk_Init:
 *  Initialise le module interface utilisateur.
 */
void adisk_Init(int direct_disk_support)
{
    if (!direct_disk_support)
        diskdial[DISKDIAL_DIRECT].flags=D_HIDDEN;

    direct_disk = direct_disk_support;
}



/* adisk_Free:
 *  Libère le module interface utilisateur.
 */
void adisk_Free(void)
{
    int drive;
    int dial_nbr;

    for (drive=0; drive<NBDRIVE; drive++)
    {
        dial_nbr = DISKDIAL_LABEL0+(DISKDIAL_LABEL1-DISKDIAL_LABEL0)*drive;
        diskdial[dial_nbr].dp = std_free (diskdial[dial_nbr].dp);

        dial_nbr = DISKDIAL_SIDE0+(DISKDIAL_SIDE1-DISKDIAL_SIDE0)*drive;
        diskdial[dial_nbr].dp = std_free (diskdial[dial_nbr].dp);
    }
}
