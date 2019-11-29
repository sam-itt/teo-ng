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
 *  Module     : keyboard.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 1998
 *  Modifié par: Eric Botcazou 17/09/2001
 *               François Mouret 02/11/2012 20/10/2017
 *               Samuel Cuella 10/2019
 *
 *  Gestion du clavier (et des manettes).
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stddef.h>
#endif
#include <string.h>

#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "mc68xx/mc6804.h"
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

static char *tokeys[]={"TOKEY_SHIFT","TOKEY_CTRL","TOKEY_Q_LOWER_CASE","TOKEY_B_LOWER_CASE","TOKEY_C_LOWER_CASE",
                       "TOKEY_D_LOWER_CASE","TOKEY_E_LOWER_CASE","TOKEY_F_LOWER_CASE","TOKEY_G_LOWER_CASE",
                       "TOKEY_H_LOWER_CASE","TOKEY_I_LOWER_CASE","TOKEY_J_LOWER_CASE","TOKEY_K_LOWER_CASE",
                       "TOKEY_L_LOWER_CASE","TOKEY_COMMA","TOKEY_N_LOWER_CASE","TOKEY_O_LOWER_CASE","TOKEY_P_LOWER_CASE",
                       "TOKEY_A_LOWER_CASE","TOKEY_R_LOWER_CASE","TOKEY_S_LOWER_CASE","TOKEY_T_LOWER_CASE",
                       "TOKEY_U_LOWER_CASE","TOKEY_V_LOWER_CASE","TOKEY_Z_LOWER_CASE","TOKEY_X_LOWER_CASE",
                       "TOKEY_Y_LOWER_CASE","TOKEY_W_LOWER_CASE","TOKEY_A_GRAVE_LOWER_CASE","TOKEY_AMPERSAND",
                       "TOKEY_E_ACUTE_LOWER_CASE","TOKEY_QUOTE","TOKEY_APOSTROPHE","TOKEY_OPEN_BRACKET","TOKEY_MINUS",
                       "TOKEY_E_GRAVE_LOWER_CASE","TOKEY_UNDERSCORE","TOKEY_C_CEDILLA_LOWER_CASE","TOKEY_PAD_0","TOKEY_PAD_1",
                       "TOKEY_PAD_2","TOKEY_PAD_3","TOKEY_PAD_4","TOKEY_PAD_5","TOKEY_PAD_6","TOKEY_PAD_7","TOKEY_PAD_8",
                       "TOKEY_PAD_9","TOKEY_F1","TOKEY_F2","TOKEY_F3","TOKEY_F4","TOKEY_F5","TOKEY_F6","TOKEY_F7","TOKEY_F8",
                       "TOKEY_F9","TOKEY_F10","TOKEY_CLOSE_BRACKET","TOKEY_EQUAL","TOKEY_DEL","TOKEY_STOP","TOKEY_CIRCUMFLEX",
                       "TOKEY_DOLLAR","TOKEY_ENT","TOKEY_M_LOWER_CASE","TOKEY_U_GRAVE_LOWER_CASE","TOKEY_LOWER_THAN","TOKEY_SEMICOLON",
                       "TOKEY_COLON","TOKEY_EXCLAMATION_MARK","TOKEY_SPACE","TOKEY_INS","TOKEY_EFF","TOKEY_HOME","TOKEY_ARROW_LEFT",
                       "TOKEY_ARROW_RIGHT","TOKEY_ARROW_UP","TOKEY_ARROW_DOWN","TOKEY_SLASH","TOKEY_ASTERISK","TOKEY_PLUS","TOKEY_DOT",
                       "TOKEY_PAD_ENT","TOKEY_ACC","TOKEY_CAPS_LOCK","TOKEY_Q_UPPER_CASE","TOKEY_B_UPPER_CASE","TOKEY_C_UPPER_CASE",
                       "TOKEY_D_UPPER_CASE","TOKEY_E_UPPER_CASE","TOKEY_F_UPPER_CASE","TOKEY_G_UPPER_CASE","TOKEY_H_UPPER_CASE","TOKEY_I_UPPER_CASE",
                       "TOKEY_J_UPPER_CASE","TOKEY_K_UPPER_CASE","TOKEY_L_UPPER_CASE","TOKEY_QUESTION_MARK","TOKEY_N_UPPER_CASE","TOKEY_O_UPPER_CASE",
                       "TOKEY_P_UPPER_CASE","TOKEY_A_UPPER_CASE","TOKEY_R_UPPER_CASE","TOKEY_S_UPPER_CASE","TOKEY_T_UPPER_CASE","TOKEY_U_UPPER_CASE",
                       "TOKEY_V_UPPER_CASE","TOKEY_Z_UPPER_CASE","TOKEY_X_UPPER_CASE","TOKEY_Y_UPPER_CASE","TOKEY_W_UPPER_CASE","TOKEY_0","TOKEY_1",
                       "TOKEY_2","TOKEY_3","TOKEY_4","TOKEY_5","TOKEY_6","TOKEY_7","TOKEY_8","TOKEY_9","TOKEY_DEGREE","TOKEY_M_UPPER_CASE",
                       "TOKEY_PERCENT","TOKEY_GREATER_THAN","TOKEY_RAZ","TOKEY_AT","TOKEY_NUMBER_SIGN","TOKEY_OPEN_BRACES","TOKEY_OPEN_SQUARE_BRACKET",
                       "TOKEY_BACKSLASH","TOKEY_CLOSE_SQUARE_BRACKET","TOKEY_CLOSE_BRACES",NULL};




/* ------------------------------------------------------------------------- */

char **keyboard_get_tokeys(void)
{
    return tokeys;
}

/* keyboard_SetACK:
 *  Emule le contrôleur clavier MC6804.
 */
void keyboard_SetACK (int state)
{
    kb_state = mc6804_SetACK (&mc6846, state, kb_state);
}



/* keyboard_Reset:
 *  Remet à zéro le clavier et le port manette du TO8.
 */
void keyboard_Reset(int mask, int value)
{
    int i;

    mc6804_Init (&mc6846);      /* reset keyboard */
    pia_int.porta.idr &= 0xFE;  /* keytest à 0 */
    kb_data=SPECIAL_NULL;       /* donnée clavier à 0 */

    /* modification optionnelle des flags */
    for (i=0; i<TEO_KEY_F_MAX; i++)
        if (mask & (1<<i))
            kb_state = (kb_state & ~(1<<i)) | (value & (1<<i));

    if (kb_state & TEO_KEY_F_CAPSLOCK)
        mc6846.prc &= 0xF7;
    else
        mc6846.prc |= 8;

}

/* Toogles appropriate flags in kb_state.
 * Avalable flags are: TEO_KEY_F_CTRL, TEO_KEY_F_ALTGR, 
 * TEO_KEY_F_NUMLOCK, TEO_KEY_F_CAPSLOCK, TEO_KEY_F_SHIFT
 */
/*TODO: replace release by "state"*/
/*Remember that we are no more working with key presses, we only set/unset a flag
 * upon instructions of the upper layer that handles keypresses/releases
 * */
void keyboard_ToggleState(int flag, int release)
{

    switch(flag){
        case TEO_KEY_F_NUMLOCK: /*TODO: make the magic jostick key configurable*/
            if(!release){
                kb_state^=TEO_KEY_F_NUMLOCK;
                if (teo_SetKeyboardLed)
                    teo_SetKeyboardLed(kb_state);

                if (njoy)
                    joystick_Reset();
            }
            break;
        case TEO_KEY_F_CAPSLOCK:
            if (!release){
                kb_state^=TEO_KEY_F_CAPSLOCK;

                if (teo_SetKeyboardLed)
                    teo_SetKeyboardLed(kb_state);
            }
            break;
        default:
            kb_state = (release != 0) ? kb_state&~flag
                                      : kb_state|flag;
    }

}

int keyboard_hasFlag(int flag)
{
    return kb_state&flag;
}


/* keyboard_Press:
 *   handle key presses
 *   key: TOKEY_* Thomson scancode. See keyboard.h for full list.
 *   release: 0 means key is pressed, 1 released.
 */
void keyboard_Press_ng (int key, int release)
{
    int code;

    code = key;

    /* activate CNT if requested */
    if( (kb_state&TEO_KEY_F_CTRL) != 0 )
        code |= TOKEY_CTRL;

    /* key up */
    if (release != 0)
    {
        if (code == kb_data)
        {
            kb_data = SPECIAL_NULL;
        }
        pia_int.porta.idr &= 0xFE;  /* keytest set to 0 */
    }
    else
    /* key down */
    {
        kb_data = code;
        pia_int.porta.idr |= 1;     /* keytest set to 1 */
        /*This bitwise & with 0X1FF clears the SPECIAL_ flags added to the defines
         *these flages are defined by the emulator internal logic and have to meaning 
         *for the real machine so we have to clear them up before passing the scancode 
         *to the actual hardware emualtor.
         **/
        mc6804_SetScanCode (&mc6846, code & 0X1FF); 
    }

}

END_OF_FUNCTION(keyboard_Press_ng)


#define NMODS 10

/* keyboard_Init:
 *  Initialize the keyboard
 */
int keyboard_Init(int num_joy)
{
    if ((num_joy<0) || (num_joy>TEO_NJOYSTICKS))
       return error_Message(TEO_ERROR_JOYSTICK_NUM, NULL);

    njoy = num_joy;

    LOCK_VARIABLE(kb_state);
    LOCK_FUNCTION(keyboard_Press_ng);

    return 0;
}


//int tokey_to_int(char *tokey)
int keyboard_tokey_to_int(char *tokey)
{
    if(strcmp(tokey,"TOKEY_SHIFT") == 0) return TOKEY_SHIFT;
    if(strcmp(tokey,"TOKEY_CTRL") == 0) return TOKEY_CTRL;
    if(strcmp(tokey,"TOKEY_Q_LOWER_CASE") == 0) return TOKEY_Q_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_B_LOWER_CASE") == 0) return TOKEY_B_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_C_LOWER_CASE") == 0) return TOKEY_C_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_D_LOWER_CASE") == 0) return TOKEY_D_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_E_LOWER_CASE") == 0) return TOKEY_E_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_F_LOWER_CASE") == 0) return TOKEY_F_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_G_LOWER_CASE") == 0) return TOKEY_G_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_H_LOWER_CASE") == 0) return TOKEY_H_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_I_LOWER_CASE") == 0) return TOKEY_I_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_J_LOWER_CASE") == 0) return TOKEY_J_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_K_LOWER_CASE") == 0) return TOKEY_K_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_L_LOWER_CASE") == 0) return TOKEY_L_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_COMMA") == 0) return TOKEY_COMMA;
    if(strcmp(tokey,"TOKEY_N_LOWER_CASE") == 0) return TOKEY_N_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_O_LOWER_CASE") == 0) return TOKEY_O_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_P_LOWER_CASE") == 0) return TOKEY_P_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_A_LOWER_CASE") == 0) return TOKEY_A_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_R_LOWER_CASE") == 0) return TOKEY_R_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_S_LOWER_CASE") == 0) return TOKEY_S_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_T_LOWER_CASE") == 0) return TOKEY_T_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_U_LOWER_CASE") == 0) return TOKEY_U_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_V_LOWER_CASE") == 0) return TOKEY_V_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_Z_LOWER_CASE") == 0) return TOKEY_Z_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_X_LOWER_CASE") == 0) return TOKEY_X_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_Y_LOWER_CASE") == 0) return TOKEY_Y_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_W_LOWER_CASE") == 0) return TOKEY_W_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_A_GRAVE_LOWER_CASE") == 0) return TOKEY_A_GRAVE_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_AMPERSAND") == 0) return TOKEY_AMPERSAND;
    if(strcmp(tokey,"TOKEY_E_ACUTE_LOWER_CASE") == 0) return TOKEY_E_ACUTE_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_QUOTE") == 0) return TOKEY_QUOTE;
    if(strcmp(tokey,"TOKEY_APOSTROPHE") == 0) return TOKEY_APOSTROPHE;
    if(strcmp(tokey,"TOKEY_OPEN_BRACKET") == 0) return TOKEY_OPEN_BRACKET;
    if(strcmp(tokey,"TOKEY_MINUS") == 0) return TOKEY_MINUS;
    if(strcmp(tokey,"TOKEY_E_GRAVE_LOWER_CASE") == 0) return TOKEY_E_GRAVE_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_UNDERSCORE") == 0) return TOKEY_UNDERSCORE;
    if(strcmp(tokey,"TOKEY_C_CEDILLA_LOWER_CASE") == 0) return TOKEY_C_CEDILLA_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_PAD_0") == 0) return TOKEY_PAD_0;
    if(strcmp(tokey,"TOKEY_PAD_1") == 0) return TOKEY_PAD_1;
    if(strcmp(tokey,"TOKEY_PAD_2") == 0) return TOKEY_PAD_2;
    if(strcmp(tokey,"TOKEY_PAD_3") == 0) return TOKEY_PAD_3;
    if(strcmp(tokey,"TOKEY_PAD_4") == 0) return TOKEY_PAD_4;
    if(strcmp(tokey,"TOKEY_PAD_5") == 0) return TOKEY_PAD_5;
    if(strcmp(tokey,"TOKEY_PAD_6") == 0) return TOKEY_PAD_6;
    if(strcmp(tokey,"TOKEY_PAD_7") == 0) return TOKEY_PAD_7;
    if(strcmp(tokey,"TOKEY_PAD_8") == 0) return TOKEY_PAD_8;
    if(strcmp(tokey,"TOKEY_PAD_9") == 0) return TOKEY_PAD_9;
    if(strcmp(tokey,"TOKEY_F1") == 0) return TOKEY_F1;
    if(strcmp(tokey,"TOKEY_F2") == 0) return TOKEY_F2;
    if(strcmp(tokey,"TOKEY_F3") == 0) return TOKEY_F3;
    if(strcmp(tokey,"TOKEY_F4") == 0) return TOKEY_F4;
    if(strcmp(tokey,"TOKEY_F5") == 0) return TOKEY_F5;
    if(strcmp(tokey,"TOKEY_F6") == 0) return TOKEY_F6;
    if(strcmp(tokey,"TOKEY_F7") == 0) return TOKEY_F7;
    if(strcmp(tokey,"TOKEY_F8") == 0) return TOKEY_F8;
    if(strcmp(tokey,"TOKEY_F9") == 0) return TOKEY_F9;
    if(strcmp(tokey,"TOKEY_F10") == 0) return TOKEY_F10;
    if(strcmp(tokey,"TOKEY_CLOSE_BRACKET") == 0) return TOKEY_CLOSE_BRACKET;
    if(strcmp(tokey,"TOKEY_EQUAL") == 0) return TOKEY_EQUAL;
    if(strcmp(tokey,"TOKEY_DEL") == 0) return TOKEY_DEL;
    if(strcmp(tokey,"TOKEY_STOP") == 0) return TOKEY_STOP;
    if(strcmp(tokey,"TOKEY_CIRCUMFLEX") == 0) return TOKEY_CIRCUMFLEX;
    if(strcmp(tokey,"TOKEY_DOLLAR") == 0) return TOKEY_DOLLAR;
    if(strcmp(tokey,"TOKEY_ENT") == 0) return TOKEY_ENT;
    if(strcmp(tokey,"TOKEY_M_LOWER_CASE") == 0) return TOKEY_M_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_U_GRAVE_LOWER_CASE") == 0) return TOKEY_U_GRAVE_LOWER_CASE;
    if(strcmp(tokey,"TOKEY_LOWER_THAN") == 0) return TOKEY_LOWER_THAN;
    if(strcmp(tokey,"TOKEY_SEMICOLON") == 0) return TOKEY_SEMICOLON;
    if(strcmp(tokey,"TOKEY_COLON") == 0) return TOKEY_COLON;
    if(strcmp(tokey,"TOKEY_EXCLAMATION_MARK") == 0) return TOKEY_EXCLAMATION_MARK;
    if(strcmp(tokey,"TOKEY_SPACE") == 0) return TOKEY_SPACE;
    if(strcmp(tokey,"TOKEY_INS") == 0) return TOKEY_INS;
    if(strcmp(tokey,"TOKEY_EFF") == 0) return TOKEY_EFF;
    if(strcmp(tokey,"TOKEY_HOME") == 0) return TOKEY_HOME;
    if(strcmp(tokey,"TOKEY_ARROW_LEFT") == 0) return TOKEY_ARROW_LEFT;
    if(strcmp(tokey,"TOKEY_ARROW_RIGHT") == 0) return TOKEY_ARROW_RIGHT;
    if(strcmp(tokey,"TOKEY_ARROW_UP") == 0) return TOKEY_ARROW_UP;
    if(strcmp(tokey,"TOKEY_ARROW_DOWN") == 0) return TOKEY_ARROW_DOWN;
    if(strcmp(tokey,"TOKEY_SLASH") == 0) return TOKEY_SLASH;
    if(strcmp(tokey,"TOKEY_ASTERISK") == 0) return TOKEY_ASTERISK;
    if(strcmp(tokey,"TOKEY_PLUS") == 0) return TOKEY_PLUS;
    if(strcmp(tokey,"TOKEY_DOT") == 0) return TOKEY_DOT;
    if(strcmp(tokey,"TOKEY_PAD_ENT") == 0) return TOKEY_PAD_ENT;
    if(strcmp(tokey,"TOKEY_ACC") == 0) return TOKEY_ACC;
    if(strcmp(tokey,"TOKEY_CAPS_LOCK") == 0) return TOKEY_CAPS_LOCK;
    if(strcmp(tokey,"TOKEY_Q_UPPER_CASE") == 0) return TOKEY_Q_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_B_UPPER_CASE") == 0) return TOKEY_B_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_C_UPPER_CASE") == 0) return TOKEY_C_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_D_UPPER_CASE") == 0) return TOKEY_D_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_E_UPPER_CASE") == 0) return TOKEY_E_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_F_UPPER_CASE") == 0) return TOKEY_F_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_G_UPPER_CASE") == 0) return TOKEY_G_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_H_UPPER_CASE") == 0) return TOKEY_H_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_I_UPPER_CASE") == 0) return TOKEY_I_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_J_UPPER_CASE") == 0) return TOKEY_J_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_K_UPPER_CASE") == 0) return TOKEY_K_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_L_UPPER_CASE") == 0) return TOKEY_L_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_QUESTION_MARK") == 0) return TOKEY_QUESTION_MARK;
    if(strcmp(tokey,"TOKEY_N_UPPER_CASE") == 0) return TOKEY_N_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_O_UPPER_CASE") == 0) return TOKEY_O_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_P_UPPER_CASE") == 0) return TOKEY_P_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_A_UPPER_CASE") == 0) return TOKEY_A_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_R_UPPER_CASE") == 0) return TOKEY_R_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_S_UPPER_CASE") == 0) return TOKEY_S_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_T_UPPER_CASE") == 0) return TOKEY_T_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_U_UPPER_CASE") == 0) return TOKEY_U_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_V_UPPER_CASE") == 0) return TOKEY_V_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_Z_UPPER_CASE") == 0) return TOKEY_Z_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_X_UPPER_CASE") == 0) return TOKEY_X_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_Y_UPPER_CASE") == 0) return TOKEY_Y_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_W_UPPER_CASE") == 0) return TOKEY_W_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_0") == 0) return TOKEY_0;
    if(strcmp(tokey,"TOKEY_1") == 0) return TOKEY_1;
    if(strcmp(tokey,"TOKEY_2") == 0) return TOKEY_2;
    if(strcmp(tokey,"TOKEY_3") == 0) return TOKEY_3;
    if(strcmp(tokey,"TOKEY_4") == 0) return TOKEY_4;
    if(strcmp(tokey,"TOKEY_5") == 0) return TOKEY_5;
    if(strcmp(tokey,"TOKEY_6") == 0) return TOKEY_6;
    if(strcmp(tokey,"TOKEY_7") == 0) return TOKEY_7;
    if(strcmp(tokey,"TOKEY_8") == 0) return TOKEY_8;
    if(strcmp(tokey,"TOKEY_9") == 0) return TOKEY_9;
    if(strcmp(tokey,"TOKEY_DEGREE") == 0) return TOKEY_DEGREE;
    if(strcmp(tokey,"TOKEY_M_UPPER_CASE") == 0) return TOKEY_M_UPPER_CASE;
    if(strcmp(tokey,"TOKEY_PERCENT") == 0) return TOKEY_PERCENT;
    if(strcmp(tokey,"TOKEY_GREATER_THAN") == 0) return TOKEY_GREATER_THAN;
    if(strcmp(tokey,"TOKEY_RAZ") == 0) return TOKEY_RAZ;
    if(strcmp(tokey,"TOKEY_AT") == 0) return TOKEY_AT;
    if(strcmp(tokey,"TOKEY_NUMBER_SIGN") == 0) return TOKEY_NUMBER_SIGN;
    if(strcmp(tokey,"TOKEY_OPEN_BRACES") == 0) return TOKEY_OPEN_BRACES;
    if(strcmp(tokey,"TOKEY_OPEN_SQUARE_BRACKET") == 0) return TOKEY_OPEN_SQUARE_BRACKET;
    if(strcmp(tokey,"TOKEY_BACKSLASH") == 0) return TOKEY_BACKSLASH;
    if(strcmp(tokey,"TOKEY_CLOSE_SQUARE_BRACKET") == 0) return TOKEY_CLOSE_SQUARE_BRACKET;
    if(strcmp(tokey,"TOKEY_CLOSE_BRACES") == 0) return TOKEY_CLOSE_BRACES;

    return SPECIAL_NULL;
}

