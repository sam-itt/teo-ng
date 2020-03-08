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
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               24/10/2012
 *               Samuel Cuella 02/2020
 *
 *  Gestion des cartouches.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif
#include <assert.h>

#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "media/memo.h"
#include "errors.h"
#include "std.h"
#include "teo.h"
#include "gettext.h"

/* Chemin du fichier de la cartouche. */
static char filename[MAX_PATH+1] = "";


#define M7DIAL_EJECT   3
#define M7DIAL_LABEL   4
#define M7DIAL_BUTTON  5
#define M7DIAL_OK      6
#define M7DIAL_LEN     9

static DIALOG *_m7dial = NULL;
#define amemo_GetDialog() ( _m7dial ? _m7dial : amemo_AllocDialog())

/* ------------------------------------------------------------------------- */

static DIALOG *amemo_AllocDialog(void)
{
    DIALOG *rv;
    int i;

    if(_m7dial)
        return _m7dial;

    rv = malloc(sizeof(DIALOG)*M7DIAL_LEN);
    i = 0;

/*                       dialog proc        x    y    w    h  fg bg  key flags  d1 d2  dp */
    rv[i++] = (DIALOG){ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,   0,    0, 0, NULL };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,   0,    0, 0, _("Cartridge drive") };
    rv[i++] = (DIALOG){ d_text_proc,        30,  44,   0,   0, 0, 0,   0,   0,    0, 0, "m7" };
    rv[i++] = (DIALOG){ d_button_proc,      47,  42,  15,  12, 0, 0,   0, D_EXIT, 0, 0, "x" };
    rv[i++] = (DIALOG){ d_textbox_proc,     64,  40, 191,  16, 0, 0,   0,   0,    0, 0, NULL };
    rv[i++] = (DIALOG){ d_button_proc,     260,  40,  30,  16, 0, 0, 'm', D_EXIT, 0, 0, "..." };
    rv[i++] = (DIALOG){ d_button_proc,     210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" };
    rv[i++] = (DIALOG){ d_yield_proc,       20,  10,   0,   0, 0, 0,   0,   0,    0, 0, NULL };
    rv[i++] = (DIALOG){ NULL,                0,   0,   0,   0, 0, 0,   0,   0,    0, 0, NULL };

    assert(i <= M7DIAL_LEN);

    _m7dial = rv;
    return rv;
}


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




/* amemo_Panel:
 *  Affiche le menu de gestion du lecteur de cartouches.
 */
void amemo_Panel(void)
{
    static int first=1;
    int ret;
    char *name;
    DIALOG *m7dial;

    m7dial = amemo_GetDialog();
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
            m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", _("(None)"));
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
                m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", _("(None)"));
                memo_Eject();
                teo.command=TEO_COMMAND_COLD_RESET;
                break;

            case M7DIAL_BUTTON:
                init_filename();
                std_CleanPath (filename);
                strcat (filename, "\\");
                if (file_select_ex(_("Select a cartridge"), filename, "m7",
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
    DIALOG *m7dial;

    m7dial = amemo_GetDialog();
    set_dialog_color(m7dial, fg_color, bg_color);
    m7dial[M7DIAL_LABEL].bg = bg_entry_color;
}



/* amemo_Init:
 *  Initialise le module interface utilisateur.
 */
void amemo_Init(void)
{
    DIALOG *m7dial;

    m7dial = amemo_GetDialog();
    if (teo.memo.label != NULL)
        m7dial[M7DIAL_LABEL].dp = std_strdup_printf ("%s", teo.memo.label);
}


/* amemo_Free:
 *  Libère le module interface utilisateur.
 */
void amemo_Free(void)
{
    DIALOG *m7dial;

    m7dial = amemo_GetDialog();
    m7dial[M7DIAL_LABEL].dp = std_free (m7dial[M7DIAL_LABEL].dp);
}
