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
 *  Module     : alleg/agui/adisk.c
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *
 *  Gestion des disquettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "to8.h"

extern void PopupMessage(const char message[]);


/* Noms des fichiers utilisés comme disquettes. */
#define LABEL_LENGTH     20
static char disk_label[4][LABEL_LENGTH+1];

/* Masque de bits décrivant les lecteurs supportant la lecture directe. */
static int direct_disk;

/* Boîte de dialogue. */
static DIALOG diskdial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc, 20,   10,  280,  180,     0,     0,   0,    0,       0,    0,    NULL },
{ d_ctext_proc,     160,   20,    0,    0,     0,     0,   0,    0,       0,    0,    NULL },
#ifdef FRENCH_LANG
{ d_ctext_proc,     160,   30,    0,    0,     0,     0,   0,    0,       0,    0,    "Lecteurs de disquettes" },
#else
{ d_ctext_proc,     160,   30,    0,    0,     0,     0,   0,    0,       0,    0,    "Disk drives" },
#endif
  /* disk 0 */
{ d_text_proc,       30,   54,    0,    0,     0,     0,   0,    0,       0,    0,    "0:" },
{ d_button_proc,     47,   52,   15,   12,     0,     0,   0,    D_EXIT,  0,    0,    "x" },
{ d_textbox_proc,    64,   50,  150,   16,     0,     0,   0,    0,       0,    0,    disk_label[0] },
{ d_button_proc,    220,   50,   30,   16,     0,     0, '0',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,   50,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* disk 1 */
{ d_text_proc,       30,   78,    0,    0,     0,     0,   0,    0,       0,    0,    "1:" },
{ d_button_proc,     47,   76,   15,   12,     0,     0,   0,    D_EXIT,  0,    0,    "x" },
{ d_textbox_proc,    64,   74,  150,   16,     0,     0,   0,    0,       0,    0,    disk_label[1] },
{ d_button_proc,    220,   74,   30,   16,     0,     0, '1',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,   74,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* disk 2 */
{ d_text_proc,       30,  102,    0,    0,     0,     0,   0,    0,       0,    0,    "2:" },
{ d_button_proc,     47,  100,   15,   12,     0,     0,   0,    D_EXIT,  0,    0,    "x" },
{ d_textbox_proc,    64,   98,  150,   16,     0,     0,   0,    0,       0,    0,    disk_label[2] },
{ d_button_proc,    220,   98,   30,   16,     0,     0, '2',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,   98,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* disk 3 */
{ d_text_proc,       30,  126,    0,    0,     0,     0,   0,    0,       0,    0,    "3:" },
{ d_button_proc,     47,  124,   15,   12,     0,     0,   0,    D_EXIT,  0,    0,    "x" },
{ d_textbox_proc,    64,  122,  150,   16,     0,     0,   0,    0,       0,    0,    disk_label[3] },
{ d_button_proc,    220,  122,   30,   16,     0,     0, '3',    D_EXIT,  0,    0,    "..." },
{ d_check_proc,     260,  122,   15,   15,     0,     0,   0,    D_EXIT,  0,    0,    "" },
  /* direct disk */
#ifdef FRENCH_LANG
{ d_button_proc,     30,  146,  250,   16,     0,     0, 'd',    D_EXIT,  0,    0,    "Accès &direct" },
#else
{ d_button_proc,     30,  146,  250,   16,     0,     0, 'D',    D_EXIT,  0,    0,    "&Direct access" },
#endif
{ d_text_proc,      255,   40,    0,    0,     0,     0,   0,    0,       0,    0,    "prot." },
{ d_button_proc,     30,  170,  126,   16,     0,     0, 'o',    D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,      20,   10,    0,    0,     0,     0,   0,    0,       0,    0,    NULL },
{ NULL,               0,    0,    0,    0,     0,     0,   0,    0,       0,    0,    NULL }
};

#define DISKDIAL_EJECT0    4
#define DISKDIAL_LABEL0    5
#define DISKDIAL_BUTTON0   6
#define DISKDIAL_CHECK0    7

#define DISKDIAL_EJECT1    9
#define DISKDIAL_LABEL1    10
#define DISKDIAL_BUTTON1   11
#define DISKDIAL_CHECK1    12

#define DISKDIAL_EJECT2    14
#define DISKDIAL_LABEL2    15
#define DISKDIAL_BUTTON2   16
#define DISKDIAL_CHECK2    17

#define DISKDIAL_EJECT3    19
#define DISKDIAL_LABEL3    20
#define DISKDIAL_BUTTON3   21
#define DISKDIAL_CHECK3    22
#define DISKDIAL_DIRECT    23

#define DISKDIAL_OK        25


/* MenuDisk:
 *  Affiche le menu de gestion des lecteurs de disquettes.
 */
void MenuDisk(void)
{
    static int first=1;
    static char filename[MAX_PATH+1];
    int drive, ret, ret2;
    
    if (first)
    {
        /* La première fois on tente d'ouvrir le répertoire par défaut. */
        if ((strlen (gui->cass.file) == 0) && (file_exists("./disks", FA_DIREC, NULL)))
#ifdef DJGPP
	        (void)sprintf (filename, "./disks/");
#else
	        (void)snprintf (filename, MAX_PATH, "./disks/");
#endif
	
        centre_dialog(diskdial);

        for (drive=0; drive<4; drive++)
        {
#ifdef DJGPP
            (void)sprintf (disk_label[drive], "%s", get_filename(gui->disk[drive].file));
#else
            (void)snprintf (disk_label[drive], LABEL_LENGTH, "%s", get_filename(gui->disk[drive].file));
#endif
            if (disk_label[drive][0] == '\0')
#ifdef DJGPP
                (void)sprintf (disk_label[drive], "%s", is_fr?"(Aucun)":"(None)");
#else
                (void)snprintf (disk_label[drive], LABEL_LENGTH, "%s", is_fr?"(Aucun)":"(None)");
#endif
            if (gui->disk[drive].write_protect == TRUE)
                diskdial[DISKDIAL_CHECK0+5*drive].flags |= D_SELECTED;
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
                drive=(ret-4)/5;
#ifdef DJGPP
                (void)sprintf (disk_label[drive], "%s", is_fr?"(Aucun)":"(None)");
#else
                (void)snprintf (disk_label[drive], LABEL_LENGTH, "%s", is_fr?"(Aucun)":"(None)");
#endif
                to8_EjectDisk(drive);
                break;

            case DISKDIAL_BUTTON0:
            case DISKDIAL_BUTTON1:
            case DISKDIAL_BUTTON2:
            case DISKDIAL_BUTTON3:
                drive=(ret-6)/5;
            
                if (file_select_ex(is_fr?"Choisissez votre disquette:":"Choose your disk:", filename, "sap",
                                   MAX_PATH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    ret2=to8_LoadDisk(drive, filename);

                    if (ret2 == TO8_ERROR)
                        PopupMessage(to8_error_msg);
                    else
                    {
                        diskdial[DISKDIAL_EJECT0+5*drive].flags &= ~D_DISABLED;
#ifdef DJGPP
                        (void)sprintf (disk_label[drive], "%s", get_filename(filename));
                        (void)sprintf (gui->disk[drive].file, "%s", filename);
#else
                        (void)snprintf (disk_label[drive], LABEL_LENGTH, "%s", get_filename(filename));
                        (void)snprintf (gui->disk[drive].file, MAX_PATH, "%s", filename);
#endif
                        if ((ret2==TO8_READ_ONLY) && !(diskdial[ret+1].d2))
                        {
                            PopupMessage(is_fr?"Attention: écriture impossible.":"Warning: writing unavailable.");
                            diskdial[ret+1].flags|=D_SELECTED;
                            diskdial[ret+1].d2=1;
                        }
                    }
                }
                break;

            case DISKDIAL_CHECK0:
            case DISKDIAL_CHECK1:
            case DISKDIAL_CHECK2:
            case DISKDIAL_CHECK3:
                drive=(ret-7)/5;
            
                if (diskdial[ret].d2)
                {
                    if (to8_SetDiskMode(drive, TO8_READ_WRITE)==TO8_READ_ONLY)
                    {
                        PopupMessage(is_fr?"Ecriture impossible sur ce support.":"Writing unavailable on this device.");
                        diskdial[ret].flags|=D_SELECTED;
                        diskdial[ret].d2=1;
                    }
                    else
                    {
                        diskdial[ret].flags&=~D_SELECTED;
                        diskdial[ret].d2=0;
                    }
                }
                else
                {
                    to8_SetDiskMode(drive, TO8_READ_ONLY);
                    diskdial[ret].flags|=D_SELECTED;
                    diskdial[ret].d2=1;
                }
                break;

            case DISKDIAL_DIRECT:
                for (drive=0; drive<4; drive++)
                {
                    if (direct_disk & (1<<drive))
                    {
                        diskdial[DISKDIAL_EJECT0+5*drive].flags |= D_DISABLED;
#ifdef DJGPP
                        (void)sprintf(disk_label[drive], "%s", is_fr?"Accès Direct":"Direct Access");
#else
                        (void)snprintf(disk_label[drive], LABEL_LENGTH, "%s", is_fr?"Accès Direct":"Direct Access");
#endif
                        gui->disk[drive].file[0] = '\0';

                        if (to8_DirectSetDrive(drive) == TO8_READ_ONLY)
                        {
                            diskdial[DISKDIAL_CHECK0+5*drive].flags|=D_SELECTED;
                            diskdial[DISKDIAL_CHECK0+5*drive].d2=1;
                        }
                        else
                        {
                            diskdial[DISKDIAL_CHECK0+5*drive].flags&=~D_SELECTED;
                            diskdial[DISKDIAL_CHECK0+5*drive].d2=0;
                        }
                    }
                }
                break;

            case -1:  /* ESC */
            case DISKDIAL_OK:
                for (drive=0; drive<4; drive++)
                    gui->disk[drive].write_protect = (diskdial[DISKDIAL_CHECK0+5*drive].flags & D_SELECTED) ? TRUE : FALSE;
                return;
        }
    }
}


/* SetDiskGUIColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void SetDiskGUIColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(diskdial, fg_color, bg_color);
    diskdial[DISKDIAL_LABEL0].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL1].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL2].bg = bg_entry_color;
    diskdial[DISKDIAL_LABEL3].bg = bg_entry_color;
}



/* InitDiskGUI:
 *  Initialise le module interface utilisateur.
 */
void InitDiskGUI(int direct_disk_support, char *title)
{
    if (!direct_disk_support)
        diskdial[DISKDIAL_DIRECT].flags=D_HIDDEN;

    direct_disk = direct_disk_support;

    /* Définit le titre de la fenêtre */
    diskdial[1].dp = title;
}
