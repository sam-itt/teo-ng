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
 *  Module     : win/keybint.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 17/09/2001
 *               François Mouret 01/11/2012 10/05/2014
 *
 *  Interface de gestion du clavier.
 */

#include <stdio.h> //printf
#ifndef SCAN_DEPEND
   #include <allegro.h>
   #include <allegro/internal/aintern.h>
#endif

#include "teo.h"
#include "to8keys.h"
#include "media/keyboard.h"
#include "media/joystick.h"

typedef struct{
    int tokey; /*TOKEY_ mappng when no modiufier(shift,altgr) is set*/
    int shift; /*KEY_* to TOKEY_ mappng when altgr is set*/
    int altgr; /*KEY_* to TOKEY_ mappng when shift is set*/
    int joycode; /*joystick direction to emulate for that key*/
}teo_kmap_t;

static teo_kmap_t keymap[KEY_MAX];

static char *tokeys[]={"TOKEY_SHIFT","TOKEY_CTRL","","TOKEY_Q_LOWER_CASE","TOKEY_B_LOWER_CASE","TOKEY_C_LOWER_CASE",
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



static volatile int jdir_buffer[2][2]; /* buffer direction des manettes */


static void verbose_debug_joystick_command(int value)
{
    printf("Joystick command for joystick %d. Raw value is: %d, meaning: ",TEO_JOYN(value),value);
    if(value&TEO_JOYSTICK_CENTER) printf("TEO_JOYSTICK_CENTER ");
    if(value&TEO_JOYSTICK_UP) printf("TEO_JOYSTICK_UP ");
    if(value&TEO_JOYSTICK_DOWN) printf("TEO_JOYSTICK_DOWN ");
    if(value&TEO_JOYSTICK_LEFT) printf("TEO_JOYSTICK_LEFT ");
    if(value&TEO_JOYSTICK_RIGHT) printf("TEO_JOYSTICK_RIGHT ");
    if(value&TEO_JOYSTICK_RIGHT) printf("TEO_JOYSTICK_RIGHT ");
    if(value&TEO_JOYSTICK_BUTTON_A) printf("TEO_JOYSTICK_BUTTON_A ");
    if(value&TEO_JOYSTICK_BUTTON_B) printf("TEO_JOYSTICK_BUTTON_B ");

    printf("\n");
}

/* KeyboardHandler:
 *  Handler des évènements bas-niveau clavier.
 */
static void KeyboardHandler_ng(int key)
{
    int tokey;
    int release=key&0x80;

   // printf("Key was: %d\n", key);
    key&=0x7F; /*What is that ? Why ? 7-bits set to 1*/
   // printf("Key is: %d\n", key);
//    printf("Keypressed: %s(%d)\n",scancode_to_name(key), key);
//    if(key == 115) key = TEO_KEY_LSHIFT;

    /*Special (emulator) keys handling:
     * do the emulator command and return.
     * The virtual TO8 won't see the key
     * */
    if(key == KEY_ESC && !release){
        teo.command=TEO_COMMAND_PANEL;
        return;
    }

    if(key == KEY_F11 && !release){
        teo.command=TEO_COMMAND_SCREENSHOT;
        return;
    }

    if(key == KEY_F12 && !release){
        teo.command=TEO_COMMAND_DEBUGGER;
        return;
    }

    /*Setting the flags on the virtual TO8 keyboard.
     * Actual scancodes are not sent to the virtual TO8
     * except for Capslock      
     **/
    if(key == KEY_LCONTROL){
        keyboard_ToggleState(TEO_KEY_F_CTRL, release);
        return;
    }

    if(key == KEY_ALTGR){
        keyboard_ToggleState(TEO_KEY_F_ALTGR, release);
        return;
    }

    if(key == KEY_NUMLOCK){
        keyboard_ToggleState(TEO_KEY_F_NUMLOCK, release);
        return;
    }

    if(key == KEY_LSHIFT || key == KEY_RSHIFT){
        keyboard_ToggleState(TEO_KEY_F_SHIFT, release);
        return;
    }

    if(key == KEY_CAPSLOCK && !release){ 
        keyboard_ToggleState(TEO_KEY_F_CAPSLOCK, release);
        /*No return ! The existing code wanted to a) set the kb_state flag
         * b) pass the code to the lower level*/
    }
    
    /*Special mode where joysticks are emulated using the keyboard*/
    if(!keyboard_hasFlag(TEO_KEY_F_NUMLOCK) && keymap[key].joycode != -1){
        int jdx; 
        int jdir;
        int astate,bstate;

//        printf("Magic key enabled(NUMLOCK off), interpreting %s(%d) as a joystick action\n",scancode_to_name(key),key);
//        verbose_debug_joystick_command(keymap[key].joycode);

        jdx = TEO_JOYN(keymap[key].joycode);
        astate = (keymap[key].joycode & TEO_JOYSTICK_BUTTON_A) ?  TEO_JOYSTICK_FIRE_ON : TEO_JOYSTICK_FIRE_OFF;
        bstate = (keymap[key].joycode & TEO_JOYSTICK_BUTTON_A) ?  TEO_JOYSTICK_FIRE_ON : TEO_JOYSTICK_FIRE_OFF;

        jdir = TEO_JOY_DIRECTIONS(keymap[key].joycode);

        if(keymap[key].joycode & TEO_JOYSTICK_BUTTON_A){
                joystick_Button (jdx-1, 0,
                                 (release != 0) ? TEO_JOYSTICK_FIRE_OFF
                                                : TEO_JOYSTICK_FIRE_ON);
        }
        if(keymap[key].joycode & TEO_JOYSTICK_BUTTON_B){
                joystick_Button (jdx-1, 1,
                                 (release != 0) ? TEO_JOYSTICK_FIRE_OFF
                                                : TEO_JOYSTICK_FIRE_ON);
        }

        if (release != 0){
            if (jdir == jdir_buffer[jdx-1][0]){
                jdir_buffer[jdx-1][0] = jdir_buffer[jdx-1][1];
                jdir_buffer[jdx-1][1]=TEO_JOYSTICK_CENTER;
            }else{
                if (jdir == jdir_buffer[jdx-1][1])
                    jdir_buffer[jdx-1][1]=TEO_JOYSTICK_CENTER;
            }
        }else{
            if (jdir != jdir_buffer[jdx-1][0]){
                jdir_buffer[jdx-1][1] = jdir_buffer[jdx-1][0];
                jdir_buffer[jdx-1][0]=jdir;
            }
        }
        joystick_Move (jdx-1, jdir_buffer[jdx-1][0]);

        /*Here we don't return as in the original code where
         * the thomson keycode were also passed to the virtual TO8
         * after having been interpreted as joystick directions.
         * A press on a joystick-bound key will move the virtual joystick
         * AND emit it's own scancode as if the key were pressed on the TO8.
         * Wether or not this is desirable is questionnable and could/should be
         * configurable.
         * */
    }

    /*Sending a scancode to the virtual TO8*/
    tokey = 0;
    if(keyboard_hasFlag(TEO_KEY_F_SHIFT) || 
       (keyboard_hasFlag(TEO_KEY_F_CAPSLOCK) && (keymap[key].tokey&SPECIAL_UPC) != 0) ){
//        printf("Got shift/capslock\n");
        tokey = keymap[key].shift;
    }
    if(keyboard_hasFlag(TEO_KEY_F_ALTGR) ){
//        printf("Got altgr\n");
        tokey = keymap[key].altgr;
    }
    if(!tokey)
        tokey = keymap[key].tokey; 
    if(tokey){
//        printf("Got allegro key %s(%d), sending %d\n",scancode_to_name(key),key, tokey);
        keyboard_Press_ng(tokey, release);
    }


}



/* KeyboardHandler:
 *  Handler des évènements bas-niveau clavier.
 */
static void KeyboardHandler(int key)
{
    int release=key&0x80;

   // printf("Key was: %d\n", key);
    key&=0x7F;
   // printf("Key is: %d\n", key);
//    printf("Keypressed: %s(%d)\n",scancode_to_name(key), key);
    if(key == 115) key = TEO_KEY_LSHIFT;

    switch (key)
    {
        case KEY_ESC:
            if (!release)
                teo.command=TEO_COMMAND_PANEL;
            break;

        case KEY_F11:
            if (!release)
                teo.command=TEO_COMMAND_SCREENSHOT;
            break;

        case KEY_F12:
            if (!release)
                teo.command=TEO_COMMAND_DEBUGGER;
            break;

        default:
            keyboard_Press(key, release);
    }
}


/* ------------------------------------------------------------------------- */


/* ukeybint_Install:
 *  Installe le module de gestion bas-niveau du clavier.
 */
void ukeybint_Install(void)
{
    int mask=(1<<TEO_KEY_F_MAX)-1, value=0;

    if (_key_shifts&KB_NUMLOCK_FLAG)
        value |= TEO_KEY_F_NUMLOCK;

    if (_key_shifts&KB_CAPSLOCK_FLAG)
        value |= TEO_KEY_F_CAPSLOCK;

    if (_key_shifts&KB_SHIFT_FLAG)
        value |= TEO_KEY_F_SHIFT;

    if (_key_shifts&KB_CTRL_FLAG)
        value |= TEO_KEY_F_CTRL;

    if (_key_shifts&KB_ALT_FLAG)
        value |= TEO_KEY_F_ALTGR;

    teo_InputReset(mask, value);

    //keyboard_lowlevel_callback=KeyboardHandler;
    keyboard_lowlevel_callback=KeyboardHandler_ng;
}



/* wkeybint_ShutDown:
 *  Désinstalle le module de gestion bas-niveau du clavier.
 */
void ukeybint_ShutDown(void)
{
    keyboard_lowlevel_callback=NULL;
}

static int allegro_name_to_scancode(char *alkey)
{
    if(strcmp(alkey,"KEY_A") == 0) return KEY_A;
    if(strcmp(alkey,"KEY_B") == 0) return KEY_B;
    if(strcmp(alkey,"KEY_C") == 0) return KEY_C;
    if(strcmp(alkey,"KEY_D") == 0) return KEY_D;
    if(strcmp(alkey,"KEY_E") == 0) return KEY_E;
    if(strcmp(alkey,"KEY_F") == 0) return KEY_F;
    if(strcmp(alkey,"KEY_G") == 0) return KEY_G;
    if(strcmp(alkey,"KEY_H") == 0) return KEY_H;
    if(strcmp(alkey,"KEY_I") == 0) return KEY_I;
    if(strcmp(alkey,"KEY_J") == 0) return KEY_J;
    if(strcmp(alkey,"KEY_K") == 0) return KEY_K;
    if(strcmp(alkey,"KEY_L") == 0) return KEY_L;
    if(strcmp(alkey,"KEY_M") == 0) return KEY_M;
    if(strcmp(alkey,"KEY_N") == 0) return KEY_N;
    if(strcmp(alkey,"KEY_O") == 0) return KEY_O;
    if(strcmp(alkey,"KEY_P") == 0) return KEY_P;
    if(strcmp(alkey,"KEY_Q") == 0) return KEY_Q;
    if(strcmp(alkey,"KEY_R") == 0) return KEY_R;
    if(strcmp(alkey,"KEY_S") == 0) return KEY_S;
    if(strcmp(alkey,"KEY_T") == 0) return KEY_T;
    if(strcmp(alkey,"KEY_U") == 0) return KEY_U;
    if(strcmp(alkey,"KEY_V") == 0) return KEY_V;
    if(strcmp(alkey,"KEY_W") == 0) return KEY_W;
    if(strcmp(alkey,"KEY_X") == 0) return KEY_X;
    if(strcmp(alkey,"KEY_Y") == 0) return KEY_Y;
    if(strcmp(alkey,"KEY_Z") == 0) return KEY_Z;
    if(strcmp(alkey,"KEY_0") == 0) return KEY_0;
    if(strcmp(alkey,"KEY_1") == 0) return KEY_1;
    if(strcmp(alkey,"KEY_2") == 0) return KEY_2;
    if(strcmp(alkey,"KEY_3") == 0) return KEY_3;
    if(strcmp(alkey,"KEY_4") == 0) return KEY_4;
    if(strcmp(alkey,"KEY_5") == 0) return KEY_5;
    if(strcmp(alkey,"KEY_6") == 0) return KEY_6;
    if(strcmp(alkey,"KEY_7") == 0) return KEY_7;
    if(strcmp(alkey,"KEY_8") == 0) return KEY_8;
    if(strcmp(alkey,"KEY_9") == 0) return KEY_9;
    if(strcmp(alkey,"KEY_0_PAD") == 0) return KEY_0_PAD;
    if(strcmp(alkey,"KEY_1_PAD") == 0) return KEY_1_PAD;
    if(strcmp(alkey,"KEY_2_PAD") == 0) return KEY_2_PAD;
    if(strcmp(alkey,"KEY_3_PAD") == 0) return KEY_3_PAD;
    if(strcmp(alkey,"KEY_4_PAD") == 0) return KEY_4_PAD;
    if(strcmp(alkey,"KEY_5_PAD") == 0) return KEY_5_PAD;
    if(strcmp(alkey,"KEY_6_PAD") == 0) return KEY_6_PAD;
    if(strcmp(alkey,"KEY_7_PAD") == 0) return KEY_7_PAD;
    if(strcmp(alkey,"KEY_8_PAD") == 0) return KEY_8_PAD;
    if(strcmp(alkey,"KEY_9_PAD") == 0) return KEY_9_PAD;
    if(strcmp(alkey,"KEY_F1") == 0) return KEY_F1;
    if(strcmp(alkey,"KEY_F2") == 0) return KEY_F2;
    if(strcmp(alkey,"KEY_F3") == 0) return KEY_F3;
    if(strcmp(alkey,"KEY_F4") == 0) return KEY_F4;
    if(strcmp(alkey,"KEY_F5") == 0) return KEY_F5;
    if(strcmp(alkey,"KEY_F6") == 0) return KEY_F6;
    if(strcmp(alkey,"KEY_F7") == 0) return KEY_F7;
    if(strcmp(alkey,"KEY_F8") == 0) return KEY_F8;
    if(strcmp(alkey,"KEY_F9") == 0) return KEY_F9;
    if(strcmp(alkey,"KEY_F10") == 0) return KEY_F10;
    if(strcmp(alkey,"KEY_F11") == 0) return KEY_F11;
    if(strcmp(alkey,"KEY_F12") == 0) return KEY_F12;
    if(strcmp(alkey,"KEY_ESC") == 0) return KEY_ESC;
    if(strcmp(alkey,"KEY_TILDE") == 0) return KEY_TILDE;
    if(strcmp(alkey,"KEY_MINUS") == 0) return KEY_MINUS;
    if(strcmp(alkey,"KEY_EQUALS") == 0) return KEY_EQUALS;
    if(strcmp(alkey,"KEY_BACKSPACE") == 0) return KEY_BACKSPACE;
    if(strcmp(alkey,"KEY_TAB") == 0) return KEY_TAB;
    if(strcmp(alkey,"KEY_OPENBRACE") == 0) return KEY_OPENBRACE;
    if(strcmp(alkey,"KEY_CLOSEBRACE") == 0) return KEY_CLOSEBRACE;
    if(strcmp(alkey,"KEY_ENTER") == 0) return KEY_ENTER;
    if(strcmp(alkey,"KEY_COLON") == 0) return KEY_COLON;
    if(strcmp(alkey,"KEY_QUOTE") == 0) return KEY_QUOTE;
    if(strcmp(alkey,"KEY_BACKSLASH") == 0) return KEY_BACKSLASH;
    if(strcmp(alkey,"KEY_BACKSLASH2") == 0) return KEY_BACKSLASH2;
    if(strcmp(alkey,"KEY_COMMA") == 0) return KEY_COMMA;
    if(strcmp(alkey,"KEY_STOP") == 0) return KEY_STOP;
    if(strcmp(alkey,"KEY_SLASH") == 0) return KEY_SLASH;
    if(strcmp(alkey,"KEY_SPACE") == 0) return KEY_SPACE;
    if(strcmp(alkey,"KEY_INSERT") == 0) return KEY_INSERT;
    if(strcmp(alkey,"KEY_DEL") == 0) return KEY_DEL;
    if(strcmp(alkey,"KEY_HOME") == 0) return KEY_HOME;
    if(strcmp(alkey,"KEY_END") == 0) return KEY_END;
    if(strcmp(alkey,"KEY_PGUP") == 0) return KEY_PGUP;
    if(strcmp(alkey,"KEY_PGDN") == 0) return KEY_PGDN;
    if(strcmp(alkey,"KEY_LEFT") == 0) return KEY_LEFT;
    if(strcmp(alkey,"KEY_RIGHT") == 0) return KEY_RIGHT;
    if(strcmp(alkey,"KEY_UP") == 0) return KEY_UP;
    if(strcmp(alkey,"KEY_DOWN") == 0) return KEY_DOWN;
    if(strcmp(alkey,"KEY_SLASH_PAD") == 0) return KEY_SLASH_PAD;
    if(strcmp(alkey,"KEY_ASTERISK") == 0) return KEY_ASTERISK;
    if(strcmp(alkey,"KEY_MINUS_PAD") == 0) return KEY_MINUS_PAD;
    if(strcmp(alkey,"KEY_PLUS_PAD") == 0) return KEY_PLUS_PAD;
    if(strcmp(alkey,"KEY_DEL_PAD") == 0) return KEY_DEL_PAD;
    if(strcmp(alkey,"KEY_ENTER_PAD") == 0) return KEY_ENTER_PAD;
    if(strcmp(alkey,"KEY_PRTSCR") == 0) return KEY_PRTSCR;
    if(strcmp(alkey,"KEY_PAUSE") == 0) return KEY_PAUSE;
    if(strcmp(alkey,"KEY_ABNT_C1") == 0) return KEY_ABNT_C1;
    if(strcmp(alkey,"KEY_YEN") == 0) return KEY_YEN;
    if(strcmp(alkey,"KEY_KANA") == 0) return KEY_KANA;
    if(strcmp(alkey,"KEY_CONVERT") == 0) return KEY_CONVERT;
    if(strcmp(alkey,"KEY_NOCONVERT") == 0) return KEY_NOCONVERT;
    if(strcmp(alkey,"KEY_AT") == 0) return KEY_AT;
    if(strcmp(alkey,"KEY_CIRCUMFLEX") == 0) return KEY_CIRCUMFLEX;
    if(strcmp(alkey,"KEY_COLON2") == 0) return KEY_COLON2;
    if(strcmp(alkey,"KEY_KANJI") == 0) return KEY_KANJI;
    if(strcmp(alkey,"KEY_EQUALS_PAD") == 0) return KEY_EQUALS_PAD;
    if(strcmp(alkey,"KEY_BACKQUOTE") == 0) return KEY_BACKQUOTE;
    if(strcmp(alkey,"KEY_SEMICOLON") == 0) return KEY_SEMICOLON;
    if(strcmp(alkey,"KEY_COMMAND") == 0) return KEY_COMMAND;
    if(strcmp(alkey,"KEY_UNKNOWN1") == 0) return KEY_UNKNOWN1;
    if(strcmp(alkey,"KEY_UNKNOWN2") == 0) return KEY_UNKNOWN2;
    if(strcmp(alkey,"KEY_UNKNOWN3") == 0) return KEY_UNKNOWN3;
    if(strcmp(alkey,"KEY_UNKNOWN4") == 0) return KEY_UNKNOWN4;
    if(strcmp(alkey,"KEY_UNKNOWN5") == 0) return KEY_UNKNOWN5;
    if(strcmp(alkey,"KEY_UNKNOWN6") == 0) return KEY_UNKNOWN6;
    if(strcmp(alkey,"KEY_UNKNOWN7") == 0) return KEY_UNKNOWN7;
    if(strcmp(alkey,"KEY_UNKNOWN8") == 0) return KEY_UNKNOWN8;
    if(strcmp(alkey,"KEY_MODIFIERS") == 0) return KEY_MODIFIERS;
    if(strcmp(alkey,"KEY_LSHIFT") == 0) return KEY_LSHIFT;
    if(strcmp(alkey,"KEY_RSHIFT") == 0) return KEY_RSHIFT;
    if(strcmp(alkey,"KEY_LCONTROL") == 0) return KEY_LCONTROL;
    if(strcmp(alkey,"KEY_RCONTROL") == 0) return KEY_RCONTROL;
    if(strcmp(alkey,"KEY_ALT") == 0) return KEY_ALT;
    if(strcmp(alkey,"KEY_ALTGR") == 0) return KEY_ALTGR;
    if(strcmp(alkey,"KEY_LWIN") == 0) return KEY_LWIN;
    if(strcmp(alkey,"KEY_RWIN") == 0) return KEY_RWIN;
    if(strcmp(alkey,"KEY_MENU") == 0) return KEY_MENU;
    if(strcmp(alkey,"KEY_SCRLOCK") == 0) return KEY_SCRLOCK;
    if(strcmp(alkey,"KEY_NUMLOCK") == 0) return KEY_NUMLOCK;
    if(strcmp(alkey,"KEY_CAPSLOCK") == 0) return KEY_CAPSLOCK;
}


static int tokey_to_int(char *tokey)
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
}

static int joystick_symbol_to_int(char *symbol)
{
    if(strcmp(symbol,"TEO_JOYSTICK_CENTER") == 0) return TEO_JOYSTICK_CENTER;
    if(strcmp(symbol,"TEO_JOYSTICK_LEFT") == 0) return TEO_JOYSTICK_LEFT;
    if(strcmp(symbol,"TEO_JOYSTICK_RIGHT") == 0) return TEO_JOYSTICK_RIGHT;
    if(strcmp(symbol,"TEO_JOYSTICK_UP") == 0) return TEO_JOYSTICK_UP;
    if(strcmp(symbol,"TEO_JOYSTICK_DOWN") == 0) return TEO_JOYSTICK_DOWN;
    if(strcmp(symbol,"TEO_JOYSTICK_BUTTON_A") == 0) return TEO_JOYSTICK_BUTTON_A;
    if(strcmp(symbol,"TEO_JOYSTICK_BUTTON_B") == 0) return TEO_JOYSTICK_BUTTON_B;
    return -1;
}


static void register_joystick_binding(char *allegro_key, int jdx, char *jdir, char *jdir2)
{
    int a_int, jd_int;

    a_int = allegro_name_to_scancode(allegro_key);
    jd_int = joystick_symbol_to_int(jdir);
    if(jdir2)
        jd_int |= joystick_symbol_to_int(jdir2);

    printf("Allegro key %s(%d) will produce %s + %s (%d)\n",allegro_key,a_int,jdir,jdir2,jd_int);
    jd_int |= ((jdx == 1) ? TEO_JOY1 : TEO_JOY2); 
    keymap[a_int].joycode = jd_int;
    printf("keymap[%d].joycode = %d\n",a_int,keymap[a_int].joycode);
}

static void read_joystick_bindings(char *section, int jdx)
{

    char **bindings;
    int n_bindings;
    char *alkey;
    char *jdir, *jdir2;

    printf("Loading up joystick emulation key mappings\n");
    override_config_file("/home/samuel/teo/github/teo/teo-keymap-joystick.ini");
    bindings = NULL;
    //Todo: #define section name
    n_bindings = list_config_entries(section, (const char ***)&bindings);
    for(int i = 0;  i < n_bindings; i++){
        alkey = bindings[i]; 
        jdir = (char*)get_config_string(section, alkey, NULL);
        if(!jdir) continue;
        printf("Key %s will emit %s\n", alkey, jdir);

        jdir2 = strchr(jdir,'+');
        if(jdir2){
            *jdir2 = '\0';
            jdir2++;
        }

        register_joystick_binding(alkey, jdx, (char*)jdir, jdir2);
    }
}

static void register_binding(char *allegro_key, char *tokey, char *modifier)
{
    int a_int, to_int;

    a_int = allegro_name_to_scancode(allegro_key);
    to_int = tokey_to_int(tokey);

    if(!modifier){
        printf("Allegro key %s(%d) will produce %s(%d)\n",allegro_key,a_int,tokey,to_int);
        keymap[a_int].tokey = to_int;
        return;
    }
    if(strcmp(modifier,"SHIFT") == 0){
        printf("Allegro key %s(%d) + SHIFT will produce %s(%d)\n",allegro_key,a_int,tokey,to_int);
        keymap[a_int].shift = to_int;
        return;
    }
    if(strcmp(modifier,"ALTGR") == 0){
        printf("Allegro key %s(%d) + ALTGR will produce %s(%d)\n",allegro_key,a_int,tokey,to_int);
        keymap[a_int].altgr = to_int;
        return;
    }
}

/* wkeybint_Init:
 *  Initialise le module de gestion bas-niveau clavier.
 */
void ukeybint_Init(void) 
{
    char **tokey;
    const char *binding;
    char *b2,*modifier;

//    memset(keymap_altgr,0,KEY_MAX*sizeof(int));
//    memset(keymap_shift,0,KEY_MAX*sizeof(int));
    memset(keymap,0,KEY_MAX*sizeof(teo_kmap_t));
    for(int i = 0; i < KEY_MAX; i++)
        keymap[i].joycode = -1;


    printf("Loading up key mappings\n");
    override_config_file("/home/samuel/teo/github/teo/teo-keymap-final.ini");
    for(tokey = tokeys; *tokey != NULL; tokey++){
        printf("Resolving mapping for emualtor definition %s... ", *tokey);
        //Todo: #define section name
        binding = get_config_string("keymapping",*tokey, NULL);
        printf("got %s\n", binding);
        if(!binding) continue;

        modifier = strchr(binding,'+');
        if(modifier){
            *modifier = '\0';
            modifier++;
        }

        b2 = strchr(binding,',');
        if(b2){
            *b2 = '\0';
            b2++;
            register_binding(b2, *tokey, modifier);
        }
        register_binding((char*)binding, *tokey, modifier);
    }

    read_joystick_bindings("joyemu1", 1);
    read_joystick_bindings("joyemu2", 2);

    jdir_buffer[0][0] =  jdir_buffer[0][1] = TEO_JOYSTICK_CENTER; 
    jdir_buffer[1][0] =  jdir_buffer[1][1] = TEO_JOYSTICK_CENTER; 
//    exit(0);
//    const char *get_config_string(const char *section, const char *name, const char *def);

}

