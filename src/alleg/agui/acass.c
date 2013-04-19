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
 *  Module     : alleg/agui/acass.c
 *  Version    : 1.8.2
 *  Créé par   : Gilles Fétis 1998
 *  Modifié par: Jérémie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               François Mouret 12/08/2011 18/03/2012 25/04/2012
 *                               24/10/2012
 *
 *  Gestion des cassettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "media/cass.h"
#include "errors.h"
#include "std.h"
#include "teo.h"

/* Chemin du fichier de la cassette. */
static char filename[MAX_PATH+1] = "";

/* Affichage du compteur. */
#define COUNTER_MAX  999
static char k7_counter_str[4];

/* Horloge de la répétition automatique. */
static int k7_tick;

/* Références avant. */
static int updown_edit_proc(int msg, DIALOG *d, int c);
static int up_button_proc(int msg, DIALOG *d, int c);
static int down_button_proc(int msg, DIALOG *d, int c);

/* Boîte de dialogue. */
static DIALOG k7dial[]={
/* (dialog proc)      x    y    w    h   fg bg key flags   d1 d2  dp */
{ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,   0,  0,     0, 0, NULL },
#ifdef FRENCH_LANG
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "Lecteur de cassettes" },
#else
{ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, "Tape recorder" },
#endif
{ d_text_proc,        30,  44,   0,   0, 0, 0,   0,  0,     0, 0, "k7" },
{ d_button_proc,      47,  42,  15,  12, 0, 0,   0, D_EXIT, 0, 0, "x" },
{ d_textbox_proc,     64,  40, 150,  16, 0, 0,   0,  0,     0, 0, NULL },
{ d_button_proc,     220,  40,  30,  16, 0, 0, 'k', D_EXIT, 0, 0, "..." },
{ d_check_proc,      260,  40,  15,  15, 0, 0,   0, D_EXIT, 0, 1, "" },
{ d_text_proc,       255,  30,   0,   0, 0, 0,   0,  0,     0, 0, "prot." },
#ifdef FRENCH_LANG
{ d_text_proc,        30,  69,   0,   0, 0, 0,   0,  0,     0, 0, "Compteur:" },
#else
{ d_text_proc,        30,  69,   0,   0, 0, 0,   0,  0,     0, 0, "Counter:" },
#endif
{ d_box_proc,        106,  67,  36,  12, 0, 0,   0,  0,     0, 0, NULL },
{ updown_edit_proc,  108,  69,  32,  10, 0, 0,   0,  0,     3, 3, k7_counter_str },
{ up_button_proc,    145,  63,  19,  10, 0, 0,   0, D_EXIT, 0, 0, "+" },
{ down_button_proc,  145,  74,  19,  10, 0, 0,   0,  0,     0, 0, "-" },
#ifdef FRENCH_LANG
{ d_button_proc,     174,  65, 106,  16, 0, 0, 'r', D_EXIT, 0, 0, "&Rembobiner" },
#else
{ d_button_proc,     174,  65, 106,  16, 0, 0, 'r', D_EXIT, 0, 0, "&Rewind" },
#endif
{ d_button_proc,     210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" },
{ d_yield_proc,       20,  10,   0,   0, 0, 0,   0,  0,     0, 0, NULL },
{ NULL,                0,   0,   0,   0, 0, 0,   0,  0,     0, 0, NULL }
};

#define K7DIAL_EJECT    3
#define K7DIAL_LABEL    4
#define K7DIAL_BUTTON   5
#define K7DIAL_CHECK    6
#define K7DIAL_BOXEDIT  9
#define K7DIAL_EDIT     10
#define K7DIAL_REWIND   13
#define K7DIAL_OK       14


/* updown_edit_proc:
 *  Procédure du champ d'entrée du compteur.
 */
static int updown_edit_proc(int msg, DIALOG *d, int c)
{
    char *endp;
    int ret, counter;

    /* Seuls les chiffres nous intéressent. */
    if ((msg == MSG_UCHAR) && !uisdigit(c))
        return D_O_K;

    ret = d_edit_proc(msg, d, c);

    /* d_edit_proc peut ne rien faire à la réception de MSG_CHAR et attendre
       le MSG_UCHAR suivant. Par conséquent D_USED_CHAR est le bon signal. */
    if (ret == D_USED_CHAR)
    {
        counter = ustrtol(k7_counter_str, &endp, 10);
        if (endp == k7_counter_str)
            counter = 0;
        else
            counter = MID(0, counter, COUNTER_MAX);

        cass_SetCounter(counter);
    }

    return ret;
}


/* update_counter:
 *  Met à jour le compteur et son affichage.
 */
static void update_counter(int *counter)
{
    *counter = MID(0, *counter, COUNTER_MAX);
    cass_SetCounter(*counter);

    k7dial[K7DIAL_EDIT].d2 = uszprintf(k7_counter_str, sizeof(k7_counter_str), "%d", *counter);
    scare_mouse();
    object_message(k7dial+K7DIAL_EDIT, MSG_DRAW, 0);
    unscare_mouse();
}


/* updown_button_proc:
 *  Corps de procédure des boutons UP et DOWN.
 */
static int updown_button_proc(int msg, DIALOG *d, int c, int down)
{
    int previous_tick, step, counter;

    if (msg == MSG_CLICK)
    {
        /* Enfonce le bouton. */
        d->flags |= D_SELECTED;
        scare_mouse();
        object_message(d, MSG_DRAW, 0);
        unscare_mouse();

        counter = cass_GetCounter();

        if (down)
           counter--;
        else
           counter++;

        update_counter(&counter);

        /* Gestion de la répétition automatique. */
        k7_tick = 0;
        previous_tick = 1;
        while (gui_mouse_b())
        {
            if (k7_tick > previous_tick)
            {
                if (k7_tick < 5)
                    step = 1;
                else if (k7_tick < 15)
                    step = 10;
                else
                    step = 50;
 
                if (down)
                   counter -= step;
                else
                   counter += step;

                update_counter(&counter);

                previous_tick = k7_tick;
            }

   	    /* Poursuit l'animation des autres objets. */
	    broadcast_dialog_message(MSG_IDLE, 0);
        }

        /* Relâche le bouton. */
        d->flags &= ~D_SELECTED;
        scare_mouse();
        object_message(d, MSG_DRAW, 0);
        unscare_mouse();

        return D_O_K;
    }
  
    return d_button_proc(msg, d, c);
}


/* up_button_proc:
 *  Procédure du boutons UP.
 */
static int up_button_proc(int msg, DIALOG *d, int c)
{
    return updown_button_proc(msg, d, c, FALSE);
}


/* down_button_proc:
 *  Procédure des boutons DOWN.
 */
static int down_button_proc(int msg, DIALOG *d, int c)
{
    return updown_button_proc(msg, d, c, TRUE);
}


/* k7_ticker:
 *  Mécanisme de l'horloge de la répétition automatique.
 */
static void k7_ticker(void)
{
    k7_tick++;
}



/* init_filename:
 *  Initialise le répertoire d'appel pour le fileselect.
 */
static void init_filename(void)
{
    *filename = '\0';

    if (teo.cass.file != NULL)
        (void)std_snprintf (filename, MAX_PATH-1, "%s", teo.cass.file);
    else
    if (teo.default_folder != NULL)
        (void)std_snprintf (filename, MAX_PATH-1, "%s\\", teo.default_folder);
    else
    if (file_exists(".\\k7", FA_DIREC, NULL))
        (void)std_snprintf (filename, MAX_PATH-1, "%s", ".\\k7\\");
}


/* ------------------------------------------------------------------------- */


/* acass_Panel:
 *  Affiche le menu de gestion du lecteur de cassettes.
 */
void acass_Panel(void)
{
    static int first=1;
    int ret;
    char *name = NULL;

    if (first)
    {
        /* La première fois on tente d'ouvrir le répertoire par défaut. */
        init_filename();

        centre_dialog(k7dial);

        k7dial[K7DIAL_LABEL].dp = std_strdup_printf ("%s", get_filename(filename));
        name = (char *)k7dial[K7DIAL_LABEL].dp;
        if ((name == NULL) || (name[0] == '\0'))
        {
            k7dial[K7DIAL_LABEL].dp = std_free (k7dial[K7DIAL_LABEL].dp);
            k7dial[K7DIAL_LABEL].dp = std_strdup_printf ("%s", is_fr?"(Aucun)":"(None)");
        }
        (void)cass_SetProtection(FALSE);
        k7dial[K7DIAL_CHECK].d2=0;
        if (teo.cass.write_protect)
        {
            k7dial[K7DIAL_CHECK].flags |= D_SELECTED;
            k7dial[K7DIAL_CHECK].d2=1;
            (void)cass_SetProtection(TRUE);
        }
        first=0;
    }

    clear_keybuf();
    install_int_ex(k7_ticker, BPS_TO_TIMER(2));

    while (TRUE)
    {
        k7dial[K7DIAL_EDIT].d2 = uszprintf(k7_counter_str, sizeof(k7_counter_str), "%d", cass_GetCounter());

        switch (popup_dialog(k7dial, K7DIAL_OK))
        {
            case K7DIAL_EJECT:
                k7dial[K7DIAL_LABEL].dp = std_free (k7dial[K7DIAL_LABEL].dp);
                k7dial[K7DIAL_LABEL].dp = std_strdup_printf ("%s", is_fr?"(Aucun)":"(None)");
                cass_Eject();
                break;

            case K7DIAL_BUTTON:
                init_filename();
                std_CleanPath (filename);
                strcat (filename, "\\");
                if (file_select_ex(is_fr?"Choisissez votre cassette:":"Choose your tape:", filename, "k7", 
                                   MAX_PATH, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
                {
                    ret=cass_Load(filename);

                    if (ret < 0)
                        agui_PopupMessage(teo_error_msg);
                    else
                    {
                        k7dial[K7DIAL_LABEL].dp = std_free (k7dial[K7DIAL_LABEL].dp);
                        k7dial[K7DIAL_LABEL].dp = std_strdup_printf ("%s", get_filename(filename));
                        teo.default_folder = std_free (teo.default_folder);
                        teo.default_folder = std_strdup_printf ("%s", filename);
                        std_CleanPath (teo.default_folder);

                        if ((ret==TRUE) && !(k7dial[6].d2))
                        {
                            agui_PopupMessage(is_fr?"Attention: écriture impossible."
                                                   :"Warning: writing unavailable.");
                            k7dial[K7DIAL_CHECK].flags|=D_SELECTED;
                            k7dial[K7DIAL_CHECK].d2=1;
                        }
                    }
                }
                break;

            case K7DIAL_CHECK:
                if (k7dial[K7DIAL_CHECK].d2)
                {
                    if (cass_SetProtection(FALSE)==TRUE)
                    {
                        agui_PopupMessage(is_fr?"Ecriture impossible sur ce support."
                                               :"Writing unavailable on this device.");
                        k7dial[K7DIAL_CHECK].flags|=D_SELECTED;
                        k7dial[K7DIAL_CHECK].d2=1;
                    }
                    else
                    {
                        k7dial[K7DIAL_CHECK].flags&=~D_SELECTED;
                        k7dial[K7DIAL_CHECK].d2=0;
                    }
                }
                else
                {
                    cass_SetProtection(TRUE);
                    k7dial[K7DIAL_CHECK].flags|=D_SELECTED;
                    k7dial[K7DIAL_CHECK].d2=1;
                }
                break;

            case K7DIAL_REWIND:
                cass_SetCounter(0);
                break;

            case -1:  /* ESC */
            case K7DIAL_OK:
                teo.cass.write_protect = (k7dial[K7DIAL_CHECK].flags & D_SELECTED) ? TRUE : FALSE;
                remove_int(k7_ticker);
                return;
        }
    }
}



/* acass_SetColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void acass_SetColors(int fg_color, int bg_color, int bg_entry_color)
{
    set_dialog_color(k7dial, fg_color, bg_color);
    k7dial[K7DIAL_LABEL].bg = bg_entry_color;
    k7dial[K7DIAL_BOXEDIT].bg = bg_entry_color;
    k7dial[K7DIAL_EDIT].bg = bg_entry_color;
}


/* acass_Init:
 *  Initialise le module interface utilisateur.
 */
void acass_Init(void)
{
}



/* acass_Free:
 *  Libère le module interface utilisateur.
 */
void acass_Free(void)
{
    k7dial[K7DIAL_LABEL].dp = std_free (k7dial[K7DIAL_LABEL].dp);
}
