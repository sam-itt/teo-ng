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
 *  Module     : alleg/agui/amemo.c
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *
 *  Gestion des cartouches.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "intern/gui.h"
#include "to8.h"

extern void PopupMessage(const char message[]);

/* Chemin du fichier de la cartouche. */
static char filename[MAX_PATH+1] = "";

/* Nom du fichier utilisé comme cartouche. */
static char m7_label[TO8_MEMO7_LABEL_LENGTH+1];

/* Boîte de dialogue. */
static DIALOG m7dial[]={
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)   (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,     0,     0,    0,    0,       0,    0,    NULL },
{ d_ctext_proc,      160,  20,    0,    0,     0,     0,    0,    0,       0,    0,    NULL },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Lecteur de cartouches" },
#else
{ d_ctext_proc,      160,  30,    0,    0,     0,     0,    0,    0,       0,    0,    "Cartridge reader" },
#endif
{ d_text_proc,        30,  54,    0,    0,     0,     0,    0,    0,       0,    0,    "m7" },
{ d_button_proc,      47,  52,   15,   12,     0,     0,    0,    D_EXIT,  0,    0,    "x" },
{ d_textbox_proc,     64,  50,  191,   16,     0,     0,    0,    0,       0,    0,    m7_label },
{ d_button_proc,     260,  50,   30,   16,     0,     0,  'm',    D_EXIT,  0,    0,    "..." },
{ d_button_proc,      30, 170,  126,   16,     0,     0,  'o',    D_EXIT,  0,    0,    "&OK" },
{ d_yield_proc,       20,  10,    0,    0,     0,     0,    0,    0,       0,    0,    NULL },
{ NULL,                0,   0,    0,    0,     0,     0,    0,    0,       0,    0,    NULL }
};

#define M7DIAL_EJECT   4
#define M7DIAL_LABEL   5
#define M7DIAL_BUTTON  6
#define M7DIAL_OK      7


/* init_filename:
 *  Initialise le répertoire d'appel pour le fileselect.
 */
static void init_filename(void)
{
    *filename = '\0';

    if (strlen (gui->memo.file) != 0)
#ifdef DJGPP
        (void)sprintf (filename, "%s", gui->memo.file);
#else
        (void)snprintf (filename, MAX_PATH, "%s", gui->memo.file);
#endif
    else
    if (strlen (gui->default_folder) != 0)
#ifdef DJGPP
        (void)sprintf (filename, "%s\\", gui->default_folder);
#else
        (void)snprintf (filename, MAX_PATH, "%s\\", gui->default_folder);
#endif
    else
      if (file_exists(".\\memo7", FA_DIREC, NULL))
//    if (access(def_folder, F_OK) == 0)
#ifdef DJGPP
        (void)sprintf (filename, "%s", ".\\memo7\\");
#else
        (void)snprintf (filename, MAX_PATH, "%s", ".\\memo7\\");
#endif
}



/* MenuMemo7:
 *  Affiche le menu de gestion du lecteur de cartouches.
 */
void MenuMemo7(void)
{
    static int first=1;
    int ret;

    if (first)
    {
        /* La première fois on tente d'ouvrir le répertoire par défaut. */
        init_filename();
        centre_dialog(m7dial);
#ifdef DJGPP
        (void)sprintf (m7_label, "%s", gui->memo.label);
#else
        (void)snprintf (m7_label, TO8_MEMO7_LABEL_LENGTH, "%s", gui->memo.label);
#endif
        if (m7_label[0] == '\0')
#ifdef DJGPP
            (void)sprintf (m7_label, "%s", is_fr?"(Aucun)":"(None)");
#else
            (void)snprintf (m7_label, TO8_MEMO7_LABEL_LENGTH, "%s", is_fr?"(Aucun)":"(None)");
#endif

	first=0;
    }

    clear_keybuf();

    while (TRUE)
    {
        ret=popup_dialog(m7dial, M7DIAL_OK);

        switch (ret)
        {
            case M7DIAL_EJECT:
#ifdef DJGPP
                (void)sprintf (m7_label, "%s", is_fr?"(Aucun)":"(None)");
#else
                (void)snprintf (m7_label, TO8_MEMO7_LABEL_LENGTH, "%s", is_fr?"(Aucun)":"(None)");
#endif
                to8_EjectMemo7();
                teo.command=COLD_RESET;
                break;

            case M7DIAL_BUTTON:
                init_filename();
                gui_CleanPath (filename);
                strcat (filename, "\\");
                if (file_select_ex(is_fr?"Choisissez votre cartouche:":"Choose your cartridge", filename, "m7",
                                   MAX_PATH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    if (to8_LoadMemo7(filename) == TO8_ERROR)
                        PopupMessage(to8_error_msg);
                    else
                    {
#ifdef DJGPP
                        (void)sprintf (m7_label, "%s", gui->memo.label);
                        (void)sprintf (gui->default_folder, "%s", filename);
#else
                        (void)snprintf (m7_label, TO8_MEMO7_LABEL_LENGTH, "%s", gui->memo.label);
                        (void)snprintf (gui->default_folder, MAX_PATH, "%s", filename);
#endif
                        gui_CleanPath (gui->default_folder);
                        teo.command=COLD_RESET;
                    }   
                }
                break;

            case -1:  /* ESC */
            case M7DIAL_OK:
                return;
        }
    }
}



/* SetMemoGUIColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void SetMemoGUIColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(m7dial, fg_color, bg_color);
    m7dial[M7DIAL_LABEL].bg = bg_entry_color;
}



/* InitMemoGUI:
 *  Initialise le module interface utilisateur.
 */
void InitMemoGUI(char *title)
{
    if (strlen(gui->memo.file))
#ifdef DJGPP
        (void)sprintf (m7_label, "%s", gui->memo.file);
#else
        (void)snprintf (m7_label, TO8_MEMO7_LABEL_LENGTH, "%s", gui->memo.file);
#endif
    /* Définit le titre de la fenêtre */
    m7dial[1].dp = title;
}

