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
 *  Module     : keyboard.c
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou 1998
 *  Modifié par: Eric Botcazou 17/09/2001
 *               François Mouret 02/11/2012
 *
 *  Gestion du clavier (et des manettes).
 */


#ifndef SCAN_DEPEND
   #include <stddef.h>
#endif

#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "media/joystick.h"
#include "media/keyboard.h"  /* MacOS */
#include "errors.h"
#include "hardware.h"
#include "teo.h"
#include "to8keys.h"


static volatile int kb_state; /* contient l'état des touches et leds du clavier PC:
                                 - SHIFT (gauche et droit confondus)
                                 - ALTGR
                                 - NUMLOCK */
static volatile int kb_data;

static int njoy;
static volatile int j0_dir[2], j1_dir[2]; /* buffer direction des manettes */

static unsigned char key_code[TEO_KEY_MAX]={
  0,253, 16,253,253,253, 20, 12,  4, 59, 52, 60, 68, 56,  8, 67,
 75,253, 19,253, 11, 51, 24,253,253,  3,253, 74, 42, 34, 26, 18,
 10,  2, 50, 58, 66, 31,254,254,254,254,254,254,254,254,254, 33,
  1,  9, 17, 25,161,129,137,145,153,  0,  0,  0,  0, 77, 13,151,
 49, 78, 61, 71, 76, 70,189,208, 64, 72, 69, 53, 15, 23,  7,  0,
  0,  0, 14,  6,  5, 62,200,189,  2,141,192, 55,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,255,255,255,255, 21,255,  0,  0,  0,
  0,255, 65};
 /* key_code[] convertit le scancode d'une touche PC (voir to8keys.h
    pour la liste des scancodes) en la valeur que produit le clavier
    TO8 pour la même touche; il y a quatres valeurs particulières :
     -   0 désigne une touche non mappée sur le TO8,
     - 253 désigne une touche du bloc clavier gauche du PC
     - 254 désigne une touche du clavier numérique du PC,
     - 255 désigne une touche spéciale du PC (shift, altgr, ...) mais n'a pas
       d'utilisation dans le traitement */

static unsigned char key_altgr_code[TEO_KEY_MAX]={
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,169,  0,  0, 41,173,
 45,  0,  0,197,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 63,191};
 /* key_altgr_code[] convertit le scancode d'une touche PC pressée en même
    temps que Altgr en sa valeur clavier TO8; nécessaire car certains signes
    du clavier TO8 sont en Altgr+touche sur le PC (#, @, ...) */

static unsigned char key_lpd_code[26][2]={
    {44, TEO_JOYSTICK_LEFT},
    {0, 0},
    {32, TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_UP},
    {28, TEO_JOYSTICK_RIGHT},
    {27, TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_DOWN},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {43, TEO_JOYSTICK_LEFT|TEO_JOYSTICK_DOWN},
    {0, 0},
    {36, TEO_JOYSTICK_CENTER},
    {0, 0},
    {0, 0},
    {0, 0},
    {35, TEO_JOYSTICK_DOWN},
    {40, TEO_JOYSTICK_UP},
    {0, 0},
    {48, TEO_JOYSTICK_LEFT|TEO_JOYSTICK_UP}
};
 /* key_lpd_code[] convertit le scancode d'une touche du bloc clavier gauche
    du PC en sa valeur clavier TO8 ou en sa valeur manette TO8 */

static unsigned char key_pad_code[9][2]={
    {22, TEO_JOYSTICK_LEFT|TEO_JOYSTICK_UP},
    {38, TEO_JOYSTICK_UP},
    {79, TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_UP},
    {30, TEO_JOYSTICK_LEFT},
    {46, TEO_JOYSTICK_CENTER},
    {47, TEO_JOYSTICK_RIGHT},
    {29, TEO_JOYSTICK_LEFT|TEO_JOYSTICK_DOWN},
    {37, TEO_JOYSTICK_DOWN},
    {54, TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_DOWN}
};
 /* key_pad_code[] convertit le scancode d'une touche du clavier numérique PC
    en sa valeur clavier TO8 ou en sa valeur manette TO8 */

enum {
    KB_INIT,
    KB_MAJ,
    KB_MIN
};


/* DoRequest:
 *  Exécute une requête du CPU.
 */
static void DoRequest(int req)
{
    if (teo_SetKeyboardLed)
        switch (req)
        {
            case KB_INIT:
            case KB_MAJ:
                kb_state |= TEO_KEY_F_CAPSLOCK;
                teo_SetKeyboardLed(kb_state);
            break;

            case KB_MIN:
                kb_state &= ~TEO_KEY_F_CAPSLOCK;
                teo_SetKeyboardLed(kb_state);
            break;
        }
    else  /* on re-synchronise */
    {
        if (kb_state&TEO_KEY_F_CAPSLOCK)
            mc6846.prc &= 0xF7;
        else
            mc6846.prc |= 8;
    }
}


/* ------------------------------------------------------------------------- */


#define KB_INIT_DELAY 670  /* cycles CPU */
#define KB_MAJ_DELAY 1300  /* cycles CPU */
#define KB_MIN_DELAY 1900  /* cycles CPU */

/* keyboard_SetACK:
 *  Emule le contrôleur clavier MC6804.
 */
void keyboard_SetACK(int state)
{
    static mc6809_clock_t start_time;
    int delay;

    if (state)
    {
        if (start_time)
        {
            delay = mc6809_clock() - start_time;

            if (delay < KB_INIT_DELAY/2)
                ;
            else if (delay < (KB_INIT_DELAY+KB_MAJ_DELAY)/2)
                DoRequest(KB_INIT);
            else if (delay < (KB_MAJ_DELAY+KB_MIN_DELAY)/2)
                DoRequest(KB_MAJ);
            else
                DoRequest(KB_MIN);

            mc6846_SetCP1(&mc6846, 1);
            start_time=0;
        }
    }
    else
    {
        if (!(mc6846.crc&0x80))  /* CP1 à 0? */
        {
            if (kb_data == 0)
            {
                /* if no key pressed, simulates a CAPS LOCK
                   key pressed to go out the keyboard routine
                   and not write the code into the keyboard
                   buffer */
                mem.mon.bank[1][0x10F8] = 0x40;
                mem.mon.bank[1][0x1125] = 0;
            }
            else
            {
                mem.mon.bank[1][0x10F8] = kb_data;
                mem.mon.bank[1][0x1125] = (kb_state&TEO_KEY_F_CTRL ? 1 : 0);
            }
        }
        else
            mc6846_SetCP1(&mc6846, 0);

        start_time = mc6809_clock();
    }
}



/* keyboard_Reset:
 *  Remet à zéro le clavier et le port manette du TO8.
 */
void keyboard_Reset(int mask, int value)
{
    int i;

    mc6846_SetCP1(&mc6846, 1);  /* DATAS clavier à 1 */
    pia_int.porta.idr &= 0xFE;  /* keytest à 0 */
    kb_data=0;                  /* donnée clavier à 0 */

    /* modification optionnelle des flags */
    for (i=0; i<TEO_KEY_F_MAX; i++)
        if (mask & (1<<i))
            kb_state = (kb_state & ~(1<<i)) | (value & (1<<i));

    if (kb_state & TEO_KEY_F_CAPSLOCK)
        mc6846.prc &= 0xF7;
    else
        mc6846.prc |= 8;

    j0_dir[0] = j0_dir[1] = TEO_JOYSTICK_CENTER;
    j1_dir[0] = j1_dir[1] = TEO_JOYSTICK_CENTER;
}



/* keyboard_Press:
 *  Prend en compte la frappe ou le relâchement d'une touche.
 *   key: scancode de la touche frappée/relachée (voir to8keys.h pour la liste).
 *   release: flag d'enfoncement/relâchement.
 */
void keyboard_Press(int key, int release)
{
    unsigned char code;

    switch (key)
    {
        case TEO_KEY_LCONTROL:  /* le contrôle gauche émule la touche CNT
                                et le bouton joystick 1 (NUMLOCK éteint) */
            if ((njoy>1) && !(kb_state&TEO_KEY_F_NUMLOCK))
                joystick_Button(njoy-1, 0, release ? TEO_JOYSTICK_FIRE_OFF : TEO_JOYSTICK_FIRE_ON);

            kb_state=(release ? kb_state&~TEO_KEY_F_CTRL : kb_state|TEO_KEY_F_CTRL);
            break;

        case TEO_KEY_RCONTROL:  /* le contrôle droit émule le bouton joystick 0 ou 1
                                en mode manette (NUMLOCK éteint) */
            if ((njoy>0) && !(kb_state&TEO_KEY_F_NUMLOCK))
                joystick_Button(TEO_NJOYSTICKS-njoy, 0, release ? TEO_JOYSTICK_FIRE_OFF : TEO_JOYSTICK_FIRE_ON);

            break;

        case TEO_KEY_ALTGR:
            kb_state=(release ? kb_state&~TEO_KEY_F_ALTGR : kb_state|TEO_KEY_F_ALTGR);
            break;

        case TEO_KEY_NUMLOCK:
            if (!release)
            {
                kb_state^=TEO_KEY_F_NUMLOCK;

                if (teo_SetKeyboardLed)
                    teo_SetKeyboardLed(kb_state);

                if (njoy)
                    joystick_Reset();
            }
            break;

        case TEO_KEY_RSHIFT:
            if ((njoy>0) && !(kb_state&TEO_KEY_F_NUMLOCK))
            {
                joystick_Button(TEO_NJOYSTICKS-njoy, 0, release ? TEO_JOYSTICK_FIRE_OFF : TEO_JOYSTICK_FIRE_ON);
            }
            else
            {
#ifdef TEO_DOUBLE_CAPSLOCK
                if (kb_state&TEO_KEY_F_CAPSLOCK)
                    key = TEO_KEY_CAPSLOCK;
                else
                {
                    kb_state=(release ? kb_state&~TEO_KEY_F_SHIFT : kb_state|TEO_KEY_F_SHIFT);
                    break;
                }
#else
                kb_state=(release ? kb_state&~TEO_KEY_F_SHIFT : kb_state|TEO_KEY_F_SHIFT);
                break;
#endif
            }
            break;

        case TEO_KEY_LSHIFT:
#ifdef TEO_DOUBLE_CAPSLOCK
        case TEO_KEY_CAPSLOCK:
            if ( ((key == TEO_KEY_CAPSLOCK) && !(kb_state&TEO_KEY_F_CAPSLOCK)) ||
                 ((key == TEO_KEY_LSHIFT) && (kb_state&TEO_KEY_F_CAPSLOCK)) )
                key = TEO_KEY_CAPSLOCK;
            else
            {
                kb_state=(release ? kb_state&~TEO_KEY_F_SHIFT : kb_state|TEO_KEY_F_SHIFT);
                break;
            }
#else
            kb_state=(release ? kb_state&~TEO_KEY_F_SHIFT : kb_state|TEO_KEY_F_SHIFT);
            break;

        case TEO_KEY_CAPSLOCK:
#endif
            if (!release)
            {
                kb_state^=TEO_KEY_F_CAPSLOCK;

                if (teo_SetKeyboardLed)
                    teo_SetKeyboardLed(kb_state);
            }
	    /* pas de break, la gestion du CapsLock est assurée par la
               routine logicielle du moniteur */

        default:
            code=key_code[key];

            if (code==253)  /* touche du bloc gauche */
            {
                if ((njoy>1) && !(kb_state&TEO_KEY_F_NUMLOCK))
		{
		    code=key_lpd_code[key-TEO_KEY_A][1];

                    if (release)
                    {
                        if (code == j1_dir[0])
                        {
                            j1_dir[0]=j1_dir[1];
                            j1_dir[1]=TEO_JOYSTICK_CENTER;
                        }
                        else if (code == j1_dir[1])
                            j1_dir[1]=TEO_JOYSTICK_CENTER;
                    }
                    else if (code != j1_dir[0])
                    {
                        j1_dir[1]=j1_dir[0];
                        j1_dir[0]=code;
                    }

                    joystick_Move(njoy-1, j1_dir[0]);
                }

		code=key_lpd_code[key-TEO_KEY_A][0];
                /* pas de break */
            }
            else if (code==254)  /* touche du pave numérique */
            {
                if (kb_state&TEO_KEY_F_NUMLOCK)
                    code=key_pad_code[key-TEO_KEY_1_PAD][0];
                else if (njoy>0) /* mode manette */
                {
                    code=key_pad_code[key-TEO_KEY_1_PAD][1];

                    if (release)
                    {
                        if (code == j0_dir[0])
                        {
                            j0_dir[0]=j0_dir[1];
                            j0_dir[1]=TEO_JOYSTICK_CENTER;
                        }
                        else if (code == j0_dir[1])
                            j0_dir[1]=TEO_JOYSTICK_CENTER;
                    }
                    else if (code != j0_dir[0])
                    {
                        j0_dir[1]=j0_dir[0];
                        j0_dir[0]=code;
                    }

                    joystick_Move(TEO_NJOYSTICKS-njoy, j0_dir[0]);
                    break; /* fin du traitement pour le pavé numérique en mode manette */
                }
            }
            else if ((kb_state&TEO_KEY_F_ALTGR) && key_altgr_code[key])
                     code=key_altgr_code[key];
             /* on remplace le code donné par key_code[] par celui donné par
                key_altgr_code[] si ce dernier est non nul et si AltGr est pressée */

            if (code--)  /* touche mappée */
            {
                /* deux cas où le bit 7 est mis à 1 */
                if (kb_state&TEO_KEY_F_SHIFT)
                    code^=0x80;

                if ((code==64) && (kb_state&TEO_KEY_F_CAPSLOCK))
                    code|=0x80;

                if (release)  /* touche relâchée */
                {
                    if (code==kb_data)
                    {
                        kb_data=0;
			pia_int.porta.idr &= 0xFE;  /* keytest à 0 */
                        mc6846_SetCP1(&mc6846, 1);
                    }
                }
                else  /* touche appuyée */
                {
                    kb_data=code;
                    pia_int.porta.idr |= 1;     /* keytest à 1 */
                    mc6846_SetCP1(&mc6846, 0);
                }
            }
            break;

    } /* end of switch */
}

END_OF_FUNCTION(keyboard_Press)


#define NMODS 10

/* keyboard_Init:
 *  Effectue quelques modifications mineures de la routine de
 *  lecture clavier TO8 (adresse: 0xF08E bank 1).
 */
int keyboard_Init(int num_joy)
{
    int addr[NMODS]={0x0A5,0x0A6,0x0A7,0x0F7,0x124,0x277,0x2DA,0x24F,0x292,0x287};
    unsigned char val[NMODS]={0x7E,0xF0,0xE5,0x86,0x86,0x26,0x2A,0x2D,0x21,0x5F};
    int i;

    if ((num_joy<0) || (num_joy>TEO_NJOYSTICKS))
       return error_Message(TEO_ERROR_JOYSTICK_NUM, NULL);

    njoy = num_joy;

    /* on modifie la routine clavier */
    for (i=0; i<NMODS; i++)
        mem.mon.bank[1][0x1000+addr[i]] = val[i];

    LOCK_VARIABLE(kb_state);
    LOCK_VARIABLE(key_code);
    LOCK_VARIABLE(key_lpd_code);
    LOCK_VARIABLE(key_pad_code);
    LOCK_VARIABLE(key_altgr_code);
    LOCK_VARIABLE(j0_dir);
    LOCK_VARIABLE(j1_dir);
    LOCK_FUNCTION(keyboard_Press);

    return 0;
}

