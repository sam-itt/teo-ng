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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2018 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Fran�ois Mouret
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
 *  Module     : alleg/agui.c
 *  Version    : 1.8.5
 *  Cr�� par   : Gilles F�tis 1998
 *  Modifi� par: J�r�mie GUILLAUME alias "JnO" 1998
 *               Eric Botcazou 28/10/2003
 *               Fran�ois Mouret 12/08/2011 18/03/2012 25/04/2012
 *               Samuel Cuella 02/2020
 *
 *  Panneau de contr�le de l'�mulateur.
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

#include "std.h"
#include "teo.h"
#include "alleg/sound.h"
#include "alleg/gfxdrv.h"
#include "alleg/gui.h"
#include "gettext.h"


/* Facteur de correction pour la taille des boutons. */
#define BUTTON_FIX 6

/* Bo�te de dialogue. */
static DIALOG mesgdial[]={
/*  dialog proc       x    y    w    h  fg bg  key  flags d1  d2 dp */
{ d_shadow_box_proc, 10,  10,   0,  50, 0, 0,   0,    0,   0, 0, NULL },
{ d_ctext_proc,      10,  20,   0,   0, 0, 0,   0,    0,   0, 0, NULL },
{ d_button_proc,    210,  40,  32,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" },
{ d_yield_proc,      10,  10,   0,   0, 0, 0,   0,    0,   0, 0, NULL },
{ NULL,               0,   0,   0,   0, 0, 0,   0,    0,   0, 0, NULL }
};

#define MESGDIAL_SHADOW   0
#define MESGDIAL_MESG     1
#define MESGDIAL_OK       2

#if 0
/* Bo�te de dialogue. */
static DIALOG questdial[]={
/*  dialog proc      x    y    w    h  fg bg  key  flags  d1  d2 dp */
{ d_shadow_box_proc, 10,  10,   0,  50, 0, 0,   0,   0,    0, 0, NULL },
{ d_ctext_proc,      10,  20,   0,   0, 0, 0,   0,   0,    0, 0, NULL },
#ifdef FRENCH_LANGUAGE
{ d_button_proc,     30,  40,  32,  16, 0, 0, 'o', D_EXIT, 0, 0, "&Oui" },
{ d_button_proc,    130,  40,  32,  16, 0, 0, 'n', D_EXIT, 0, 0, "&Non" },
#else
{ d_button_proc,     30,  40,  32,  16, 0, 0, 'y', D_EXIT, 0, 0, "&Yes" },
{ d_button_proc,    130,  40,  32,  16, 0, 0, 'n', D_EXIT, 0, 0, "&No" },
#endif
{ d_yield_proc,      10,  10,   0,   0, 0, 0,   0,   0,    0, 0, NULL },
{ NULL,               0,   0,   0,   0, 0, 0,   0,   0,    0, 0, NULL }
};

#define QUESTDIAL_SHADOW  0
#define QUESTDIAL_QUEST   1
#define QUESTDIAL_YES     2
#define QUESTDIAL_NO      3

#define FOCUS_YES  QUESTDIAL_YES
#define FOCUS_NO   QUESTDIAL_NO


/* PopupQuestion:
 *  Affiche une bo�te de dialogue contenant une question et deux boutons Oui/Non.
 */
static int PopupQuestion(const char question[], int focus)
{
    int esp;
    int ret;

    questdial[QUESTDIAL_SHADOW].w=strlen(question)*8+16;
    questdial[QUESTDIAL_QUEST].x=questdial[QUESTDIAL_SHADOW].x+questdial[QUESTDIAL_SHADOW].w/2;

    esp=(questdial[QUESTDIAL_SHADOW].w-2*(questdial[QUESTDIAL_YES].w+BUTTON_FIX))/3;
    questdial[QUESTDIAL_YES].x=questdial[QUESTDIAL_SHADOW].x+esp;
    questdial[QUESTDIAL_NO].x=questdial[QUESTDIAL_YES].x+(questdial[QUESTDIAL_YES].w+BUTTON_FIX)+esp;

    questdial[QUESTDIAL_QUEST].dp = stdstrdup_printf ("%s", question);

    centre_dialog(questdial);

    ret = (popup_dialog(questdial, focus) == QUESTDIAL_YES ? TRUE : FALSE);

    questdial[QUESTDIAL_QUEST].dp = std_free (questdial[QUESTDIAL_QUEST].dp);

    return ret;
}

/*
if (PopupQuestion(_("Do you really want to quit ?"), FOCUS_NO))
{
}
*/

#endif


#define CONTROLDIAL_WARMRESET 2
#define CONTROLDIAL_COLDRESET 3
#define CONTROLDIAL_FULLRESET 4
#define CONTROLDIAL_COMMAND   5
#define CONTROLDIAL_DISK      6
#define CONTROLDIAL_CASS      7
#define CONTROLDIAL_CART      8
#define CONTROLDIAL_PRINTER   9
#define CONTROLDIAL_QUIT      10
#define CONTROLDIAL_ABOUT     11
#define CONTROLDIAL_OK        12
#define CONTROLDIAL_LEN        15

static DIALOG *_controldial = NULL;
#define agui_GetDialog() ( _controldial ? _controldial: agui_AllocDialog())

/* ------------------------------------------------------------------------- */
static DIALOG *agui_AllocDialog(void)
{
    DIALOG *rv;
    int i;

    if(_controldial)
        return _controldial;

    rv = malloc(sizeof(DIALOG)*CONTROLDIAL_LEN);
    i = 0;

/*                        dialog proc       x    y    w    h  fg bg  key  flags  d1 d2  dp  */
    rv[i++] = (DIALOG){ d_shadow_box_proc,  20,  10, 280, 180, 0, 0,  0,    0,    0, 0, NULL };
    rv[i++] = (DIALOG){ d_ctext_proc,      160,  20,   0,   0, 0, 0,   0,  0,     0, 0, _("Control panel") };
    rv[i++] = (DIALOG){ d_button_proc,      30,  35, 128,  16, 0, 0, 'w', D_EXIT, 0, 0, _("&Warm reset") };
    rv[i++] = (DIALOG){ d_button_proc,     162,  35, 128,  16, 0, 0, 'c', D_EXIT, 0, 0, _("&Cold reset") };
    rv[i++] = (DIALOG){ d_button_proc,      93,  53, 128,  16, 0, 0, 'f', D_EXIT, 0, 0, _("&Full reset") };
    rv[i++] = (DIALOG){ d_button_proc,      30,  75, 260,  16, 0, 0, 's', D_EXIT, 0, 0, _("&Settings...") };
    rv[i++] = (DIALOG){ d_button_proc,      30,  93, 260,  16, 0, 0, 'd', D_EXIT, 0, 0, _("&Disk drives...") };
    rv[i++] = (DIALOG){ d_button_proc,      30, 111, 260,  16, 0, 0, 't', D_EXIT, 0, 0, _("&Tape drive...") };
    rv[i++] = (DIALOG){ d_button_proc,      30, 129, 260,  16, 0, 0, 'r', D_EXIT, 0, 0, _("Ca&rtridge drive...") };
    rv[i++] = (DIALOG){ d_button_proc,      30, 147, 260,  16, 0, 0, 'p', D_EXIT, 0, 0, _("Dot-matrix &printer...") };
    rv[i++] = (DIALOG){ d_button_proc,      30, 170,  80,  16, 0, 0, 'q', D_EXIT, 0, 0, _("&Quit") };
    rv[i++] = (DIALOG){ d_button_proc,     120, 170,  80,  16, 0, 0, 'a', D_EXIT, 0, 0, _("&About") };
    rv[i++] = (DIALOG){ d_button_proc,     210, 170,  80,  16, 0, 0, 'o', D_EXIT, 0, 0, "&OK" };
    rv[i++] = (DIALOG){ d_yield_proc,       20,  10,   0,   0, 0, 0,  0,    0,    0, 0, NULL };
    rv[i++] = (DIALOG){ NULL,                0,   0,   0,   0, 0, 0,  0,    0,    0, 0, NULL };

    assert(i <= CONTROLDIAL_LEN);

    _controldial = rv;

    return rv;
}


/* ControlPanel:
 *  Affiche le panneau de contr�le.
 */
void agui_Panel(void)
{
    int response;
    DIALOG *controldial;

    controldial = agui_GetDialog();
    clear_keybuf();
    centre_dialog(controldial);

    while (TRUE)
        switch (popup_dialog(controldial, CONTROLDIAL_OK))
        {
            case CONTROLDIAL_WARMRESET:
                teo.command=TEO_COMMAND_RESET;
                return;

            case CONTROLDIAL_COLDRESET:
                teo.command=TEO_COMMAND_COLD_RESET;
                return;

            case CONTROLDIAL_FULLRESET:
                response = alert (_("All the memory"),
                                  _("will be cleared."),
                                  NULL,
                                  "Ok",
                                  _("Cancel"), 0, 0);
                if (response == 1)
                {
                    teo.command=TEO_COMMAND_FULL_RESET;
                    return;
                }
                break;

            case CONTROLDIAL_COMMAND:
                asetting_Panel();
                break;

            case CONTROLDIAL_CART:
                amemo_Panel();
                break;

            case CONTROLDIAL_CASS:
                acass_Panel();
                break;

            case CONTROLDIAL_DISK:
                adisk_Panel();
                break;

            case CONTROLDIAL_PRINTER:
                aprinter_Panel();
                break;

            case CONTROLDIAL_ABOUT:
                aabout_Panel();
                break;

            case -1:  /* ESC */
            case CONTROLDIAL_OK:
                return;

            case CONTROLDIAL_QUIT:
                if (teo.command == TEO_COMMAND_COLD_RESET)
                    teo_ColdReset();
                teo.command=TEO_COMMAND_QUIT;
                return;
        }
}



/* SetGUIColors:
 *  Fixe les 3 couleurs de l'interface utilisateur.
 */
void agui_SetColors(int fg_color, int bg_color, int bg_entry_color)
{
    DIALOG *controldial;

    controldial = agui_GetDialog();
    set_dialog_color(mesgdial, fg_color, bg_color);
/*    set_dialog_color(questdial, fg_color, bg_color); */
    
    asetting_SetColors(fg_color, bg_color, bg_entry_color);
    adisk_SetColors   (fg_color, bg_color, bg_entry_color);
    acass_SetColors   (fg_color, bg_color, bg_entry_color);
    amemo_SetColors   (fg_color, bg_color, bg_entry_color);
    aprinter_SetColors(fg_color, bg_color, bg_entry_color);
    aabout_SetColors  (fg_color, bg_color, bg_entry_color);

    set_dialog_color(controldial, fg_color, bg_color);

    /* Pour tous les objets. */
    gui_fg_color = fg_color;
    gui_bg_color = bg_color;
    gui_mg_color = bg_color;
}



/* agui_Init:
 *  Initialise le module interface utilisateur.
 */
void agui_Init(char version_name[], int gfx_mode, int direct_disk_support)
{
    asetting_Init(gfx_mode);
    adisk_Init   (direct_disk_support);
    acass_Init   ();
    amemo_Init   ();
    aprinter_Init();
    aabout_Init  (version_name);
}



/* FreeGUI:
 *  Lib�re le module interface utilisateur.
 */
void agui_Free (void)
{
    asetting_Free ();
    adisk_Free ();
    acass_Free ();
    amemo_Free ();
    aprinter_Free ();
    aabout_Free ();
}



/* agui_PopupMessage:
 *  Affiche une bo�te de dialogue contenant le message et un bouton OK.
 */
#define STR_MAX_LENGTH 34
void agui_PopupMessage(const char message[])
{
    char *msg = NULL;
    char *msg1 = NULL;
    char *msg2 = NULL;
    char *msg3 = NULL;
    char *p;
    
    if (strchr (message, 0x0a) == NULL)
    {
        mesgdial[MESGDIAL_SHADOW].w=strlen(message)*8+16;
        mesgdial[MESGDIAL_MESG].x=mesgdial[MESGDIAL_SHADOW].x+mesgdial[MESGDIAL_SHADOW].w/2;
        mesgdial[MESGDIAL_OK].x=mesgdial[MESGDIAL_MESG].x-(mesgdial[MESGDIAL_OK].w+BUTTON_FIX)/2;

        mesgdial[MESGDIAL_MESG].dp = std_strdup_printf ("%s", message);

        centre_dialog(mesgdial);

        popup_dialog(mesgdial, MESGDIAL_OK);

        mesgdial[MESGDIAL_MESG].dp = std_free (mesgdial[MESGDIAL_MESG].dp);
    }
    else
    {
        msg = std_strdup_printf ("%s", message);
        p = strchr (msg, 0x0a);
        p[0] = '\0';
        msg1 = std_strdup_printf ("%s", msg);
        p++;
        if (strlen (p) <= STR_MAX_LENGTH)
        {
            msg2 = std_strdup_printf ("%s", p);
            msg3 = std_strdup_printf ("%s", " ");
        }
        else
        if (strlen (p) <= (STR_MAX_LENGTH*2))
        {
            msg3 = std_strdup_printf ("%s", p+STR_MAX_LENGTH);
            p[STR_MAX_LENGTH] = '\0';
            msg2 = std_strdup_printf ("%s", p);
        }
        else
        {
            msg3 = std_strdup_printf ("...%s", p + (strlen(p) - STR_MAX_LENGTH + 3));
            p[STR_MAX_LENGTH-3] = '\0';
            msg2 = std_strdup_printf ("%s...", p);
        }
        alert (msg1, msg2, msg3, "OK", NULL, 0, 0);
        msg = std_free (msg);
        msg1 = std_free (msg1);
        msg2 = std_free (msg2);
        msg3 = std_free (msg3);
    }
}

