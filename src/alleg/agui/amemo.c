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
 *  Module     : alleg/agui/amemo.c
 *  Version    : 1.8.2
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               24/10/2012
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
#include "alleg/gui.h"
#include "media/memo.h"
#include "errors.h"
#include "std.h"
#include "teo.h"

/* Chemin du fichier de la cartouche. */
static char filename[MAX_PATH+1] = "";

/* Boîte de dialogue. */
static DIALOG m7dial[]={
/*  dialog proc        x    y    w    h  fg bg  key flags  d1 d2  dp */
{ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,   0,    0, 0, NULL },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,   0,    0, 0, "Lecteur de cartouches" },
#else
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,   0,    0, 0, "Cartridge reader" },
#endif
{ d_text_proc,        30,  44,   0,   0, 0, 0,   0,   0,    0, 0, "m7" },
{ d_button_proc,      47,  42,  15,  12, 0, 0,   0, D_EXIT, 0, 0, "x" },
{ d_textbox_proc,     64,  40, 191,  16, 0, 0,   0,   0,    0, 0, NULL },
{ d_button_proc,     260,  40,  30,  16, 0, 0, 'm', D_EXIT, 0, 0, "..." },
{ d_button_proc,     210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" },
{ d_yield_proc,       20,  10,   0,   0, 0, 0,   0,   0,    0, 0, NULL },
{ NULL,                0,   0,   0,   0, 0, 0,   0,   0,    0, 0, NULL }
};

#define M7DIAL_EJECT   3
#define M7DIAL_LABEL   4
#define M7DIAL_BUTTON  5
#define M7DIAL_OK      6


/* init_filename:
 *  Initialise le répertoire d'appel pour le fileselect.
 */
static void init_filename(void)
{
    *filename = '\0';

    if (teo.memo.file != NULL)
        (void)std_snprintf (filename, MAX_PATH-1, "%s", teo.memo.file);
    else
    if (teo.default_folder != NULL)
        (void)std_snprintf (filename, MAX_PATH-1,"%s\\", teo.default_folder);
    else
    if (file_exists(".\\memo7", FA_DIREC, NULL))
        (void)std_snprintf (filename, MAX_PATH-1, "%s", ".\\memo7\\");
}


/* ------------------------------------------------------------------------- */


/* amemo_Panel:
 *  Affiche le menu de gestion du lecteur de cartouches.
 */
void amemo_Panel(void)
{
    static int first=1;
    int ret;
    char *name;

    if (first)
    {
        /* La première fois on tente d'ouvrir le répertoire par défaut. */
        init_filename();
        centre_dialog(m7dial);
        m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", teo.memo.label);
        name = (char *)m7dial[M7DIAL_LABEL].dp;
        if ((name == NULL) || (*name == '\0'))
        {
            m7dial[M7DIAL_LABEL].dp = std_free (m7dial[M7DIAL_LABEL].dp);
            m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", is_fr?"(Aucun)":"(None)");
        }
        first=0;
    }

    clear_keybuf();

    while (TRUE)
    {
        ret=popup_dialog(m7dial, M7DIAL_OK);

        switch (ret)
        {
            case M7DIAL_EJECT:
                m7dial[M7DIAL_LABEL].dp = std_free (m7dial[M7DIAL_LABEL].dp);
                m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", is_fr?"(Aucun)":"(None)");
                memo_Eject();
                teo.command=TEO_COMMAND_COLD_RESET;
                break;

            case M7DIAL_BUTTON:
                init_filename();
                std_CleanPath (filename);
                strcat (filename, "\\");
                if (file_select_ex(is_fr?"Choisissez votre cartouche:":"Choose your cartridge", filename, "m7",
                                   MAX_PATH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    if (memo_Load(filename) < 0)
                        agui_PopupMessage(teo_error_msg);
                    else
                    {
                        m7dial[M7DIAL_LABEL].dp = std_free (m7dial[M7DIAL_LABEL].dp);
                        m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", teo.memo.label);
                        teo.default_folder = std_free (teo.default_folder);
                        teo.default_folder = std_strdup_printf ("%s", filename);
                        std_CleanPath (teo.default_folder);
                        teo.command=TEO_COMMAND_COLD_RESET;
                    }   
                }
                break;

            case -1:  /* ESC */
            case M7DIAL_OK:
                return;
        }
    }
}



/* amemo_SetColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void amemo_SetColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(m7dial, fg_color, bg_color);
    m7dial[M7DIAL_LABEL].bg = bg_entry_color;
}



/* amemo_Init:
 *  Initialise le module interface utilisateur.
 */
void amemo_Init(void)
{
    if (teo.memo.label != NULL)
        m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", teo.memo.label);
}


/* amemo_Free:
 *  Libère le module interface utilisateur.
 */
void amemo_Free(void)
{
    m7dial[M7DIAL_LABEL].dp = std_free (m7dial[M7DIAL_LABEL].dp);
}
