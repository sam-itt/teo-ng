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
 *  Module     : intern/keyboard.h
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 1998
 *  Modifié par: Eric Botcazou 14/02/2001
 *               François Mouret 01/11/2012
 *               Samuel Cuella 10/2019
 *
 *  Gestion du clavier (et des manettes).
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H

#define SPECIAL_NULL   -1      /* key not mapped */
#define SPECIAL_KB0    0x0800  /* special key for joystick 0 (keyboard right) */
#define SPECIAL_KB1    0x1000  /* special key for joystick 1 (keyboard left) */
#define SPECIAL_PAD    0x2000  /* special key for joystick 0 (numeric pad) */
#define SPECIAL_UPC    0x4000  /* upper case if CAPS LOCK */
#define SPECIAL_KEY    0x8000  /* special key (not defined) */

#define TOKEY_SHIFT    0x80
#define TOKEY_CTRL     0x100
#define JOYSTICK_MASK  (0x7f|SPECIAL_KB0|SPECIAL_KB1|SPECIAL_PAD|SPECIAL_UPC)

/* List of Thomson scancodes */
/* 1st row */
#define TOKEY_F1                   0x20  /* F1 */
#define TOKEY_F6                   TOKEY_F1|TOKEY_SHIFT  /* F6 */
#define TOKEY_F2                   0x00  /* F2 */
#define TOKEY_F7                   TOKEY_F2|TOKEY_SHIFT  /* F7 */
#define TOKEY_F3                   0x08  /* F3 */
#define TOKEY_F8                   TOKEY_F3|TOKEY_SHIFT  /* F8 */
#define TOKEY_F4                   0x10  /* F4 */
#define TOKEY_F9                   TOKEY_F4|TOKEY_SHIFT  /* F9 */
#define TOKEY_F5                   0x18  /* F5 */
#define TOKEY_F10                  TOKEY_F5|TOKEY_SHIFT  /* F10 */
/* 2nd row */
#define TOKEY_NUMBER_SIGN          0x28  /* # */
#define TOKEY_AT                   TOKEY_NUMBER_SIGN|TOKEY_SHIFT  /* @ */
#define TOKEY_ASTERISK             0x29  /* * */
#define TOKEY_1                    TOKEY_ASTERISK|TOKEY_SHIFT  /* 1 */
#define TOKEY_E_ACUTE_LOWER_CASE   0x21  /* é */
#define TOKEY_2                    TOKEY_E_ACUTE_LOWER_CASE|TOKEY_SHIFT  /* 2 */
#define TOKEY_QUOTE                0x19  /* " */
#define TOKEY_3                    TOKEY_QUOTE|TOKEY_SHIFT  /* 3 */
#define TOKEY_APOSTROPHE           0x11  /* ' */
#define TOKEY_4                    TOKEY_APOSTROPHE|TOKEY_SHIFT  /* 4 */
#define TOKEY_OPEN_BRACKET         0x09  /* ( */
#define TOKEY_5                    TOKEY_OPEN_BRACKET|TOKEY_SHIFT  /* 5 */
#define TOKEY_UNDERSCORE           0x01  /* _  */
#define TOKEY_6                    TOKEY_UNDERSCORE|TOKEY_SHIFT  /* 6 */
#define TOKEY_E_GRAVE_LOWER_CASE   0x31  /* è */
#define TOKEY_7                    TOKEY_E_GRAVE_LOWER_CASE|TOKEY_SHIFT  /* 7 */
#define TOKEY_EXCLAMATION_MARK     0x39|SPECIAL_KB1  /* ! */
#define TOKEY_8                    TOKEY_EXCLAMATION_MARK|TOKEY_SHIFT  /* 8 */
#define TOKEY_C_CEDILLA_LOWER_CASE 0x41  /* ç */
#define TOKEY_9                    TOKEY_C_CEDILLA_LOWER_CASE|TOKEY_SHIFT  /* 9 */
#define TOKEY_A_GRAVE_LOWER_CASE   0x49  /* à */
#define TOKEY_0                    TOKEY_A_GRAVE_LOWER_CASE|TOKEY_SHIFT  /* 0 */
#define TOKEY_CLOSE_BRACKET        0x4c  /* ) */
#define TOKEY_DEGREE               TOKEY_CLOSE_BRACKET|TOKEY_SHIFT  /* ° */
#define TOKEY_MINUS                0x44  /* - */
#define TOKEY_BACKSLASH            TOKEY_MINUS|TOKEY_SHIFT  /* \ */
#define TOKEY_EQUAL                0x0c  /* = */
#define TOKEY_PLUS                 TOKEY_EQUAL|TOKEY_SHIFT  /* + */
#define TOKEY_ACC                  0x14  /* ACC */
#define TOKEY_ARROW_UP             0x04  /* arrow up */
#define TOKEY_PAD_7                0x1c|SPECIAL_PAD  /* 7 numeric pad */
#define TOKEY_PAD_8                0x24|SPECIAL_PAD  /* 8 numeric pad */
#define TOKEY_PAD_9                0x35|SPECIAL_PAD  /* 9 numeric pad */
/* 3rd row */
#define TOKEY_STOP                 0x30  /* STOP */
#define TOKEY_A_LOWER_CASE         0x2a|SPECIAL_KB0|SPECIAL_UPC  /* a */
#define TOKEY_A_UPPER_CASE         TOKEY_A_LOWER_CASE|TOKEY_SHIFT  /* A */
#define TOKEY_Z_LOWER_CASE         0x22|SPECIAL_KB0|SPECIAL_UPC  /* z */
#define TOKEY_Z_UPPER_CASE         TOKEY_Z_LOWER_CASE|TOKEY_SHIFT  /* Z */
#define TOKEY_E_LOWER_CASE         0x1a|SPECIAL_KB0|SPECIAL_UPC  /* e */
#define TOKEY_E_UPPER_CASE         TOKEY_E_LOWER_CASE|TOKEY_SHIFT  /* E */
#define TOKEY_R_LOWER_CASE         0x12|SPECIAL_UPC  /* r */
#define TOKEY_R_UPPER_CASE         TOKEY_R_LOWER_CASE|TOKEY_SHIFT  /* R */
#define TOKEY_T_LOWER_CASE         0x0a|SPECIAL_UPC  /* t */
#define TOKEY_T_UPPER_CASE         TOKEY_T_LOWER_CASE|TOKEY_SHIFT  /* T */
#define TOKEY_Y_LOWER_CASE         0x02|SPECIAL_UPC  /* y  */
#define TOKEY_Y_UPPER_CASE         TOKEY_Y_LOWER_CASE|TOKEY_SHIFT  /* Y */
#define TOKEY_U_LOWER_CASE         0x32|SPECIAL_UPC  /* u */
#define TOKEY_U_UPPER_CASE         TOKEY_U_LOWER_CASE|TOKEY_SHIFT  /* U */
#define TOKEY_I_LOWER_CASE         0x3a|SPECIAL_UPC|SPECIAL_KB1  /* i */
#define TOKEY_I_UPPER_CASE         TOKEY_I_LOWER_CASE|TOKEY_SHIFT  /* I */
#define TOKEY_O_LOWER_CASE         0x42|SPECIAL_UPC|SPECIAL_KB1  /* o */
#define TOKEY_O_UPPER_CASE         TOKEY_O_LOWER_CASE|TOKEY_SHIFT  /* O */
#define TOKEY_P_LOWER_CASE         0x4a|SPECIAL_UPC|SPECIAL_KB1  /* p */
#define TOKEY_P_UPPER_CASE         TOKEY_P_LOWER_CASE|TOKEY_SHIFT  /* P */
#define TOKEY_CIRCUMFLEX           0x4d  /* ^ */
#define TOKEY_DIAERESIS            TOKEY_CIRCUMFLEX|TOKEY_SHIFT  /* " */
#define TOKEY_DOLLAR               0x3c  /* $ */
#define TOKEY_AMPERSAND            TOKEY_DOLLAR|TOKEY_SHIFT  /* & */
#define TOKEY_ENT                  0x46  /* ENT */
#define TOKEY_ARROW_RIGHT          0x05  /* arrow right */
#define TOKEY_ARROW_LEFT           0x0d  /* arrow left */
#define TOKEY_PAD_4                0x1d|SPECIAL_PAD  /* 4 numeric pad */
#define TOKEY_PAD_5                0x2d|SPECIAL_PAD  /* 5 numeric pad */
#define TOKEY_PAD_6                0x2e|SPECIAL_PAD  /* 6 numeric pad */
/* 4th row */
#define TOKEY_OPEN_SQUARE_BRACKET  0x2c  /* [ */
#define TOKEY_OPEN_BRACES          TOKEY_OPEN_SQUARE_BRACKET|TOKEY_SHIFT  /* [ */
#define TOKEY_Q_LOWER_CASE         0x2b|SPECIAL_KB0|SPECIAL_UPC  /* q */
#define TOKEY_Q_UPPER_CASE         TOKEY_Q_LOWER_CASE|TOKEY_SHIFT  /* Q */
#define TOKEY_S_LOWER_CASE         0x23|SPECIAL_KB0|SPECIAL_UPC  /* s */ 
#define TOKEY_S_UPPER_CASE         TOKEY_S_LOWER_CASE|TOKEY_SHIFT  /* S */
#define TOKEY_D_LOWER_CASE         0x1b|SPECIAL_KB0|SPECIAL_UPC  /* d */ 
#define TOKEY_D_UPPER_CASE         TOKEY_D_LOWER_CASE|TOKEY_SHIFT  /* D */
#define TOKEY_F_LOWER_CASE         0x13|SPECIAL_UPC  /* f */
#define TOKEY_F_UPPER_CASE         TOKEY_F_LOWER_CASE|TOKEY_SHIFT  /* F */
#define TOKEY_G_LOWER_CASE         0x0b|SPECIAL_UPC  /* g */
#define TOKEY_G_UPPER_CASE         TOKEY_G_LOWER_CASE|TOKEY_SHIFT  /* G */
#define TOKEY_H_LOWER_CASE         0x03|SPECIAL_UPC  /* h  */
#define TOKEY_H_UPPER_CASE         TOKEY_H_LOWER_CASE|TOKEY_SHIFT  /* H */
#define TOKEY_J_LOWER_CASE         0x33|SPECIAL_UPC  /* j */
#define TOKEY_J_UPPER_CASE         TOKEY_J_LOWER_CASE|TOKEY_SHIFT  /* J */
#define TOKEY_K_LOWER_CASE         0x3b|SPECIAL_UPC|SPECIAL_KB1  /* k */
#define TOKEY_K_UPPER_CASE         TOKEY_K_LOWER_CASE|TOKEY_SHIFT  /* K */
#define TOKEY_L_LOWER_CASE         0x43|SPECIAL_UPC|SPECIAL_KB1  /* l */
#define TOKEY_L_UPPER_CASE         TOKEY_L_LOWER_CASE|TOKEY_SHIFT  /* L */
#define TOKEY_M_LOWER_CASE         0x4b|SPECIAL_UPC|SPECIAL_KB1  /* m */
#define TOKEY_M_UPPER_CASE         TOKEY_M_LOWER_CASE|TOKEY_SHIFT  /* M */
#define TOKEY_U_GRAVE_LOWER_CASE   0x45  /* ù */
#define TOKEY_PERCENT              TOKEY_U_GRAVE_LOWER_CASE|TOKEY_SHIFT  /* % */
#define TOKEY_CLOSE_SQUARE_BRACKET 0x3e  /* ] */
#define TOKEY_CLOSE_BRACES         TOKEY_CLOSE_SQUARE_BRACKET|TOKEY_SHIFT  /* ] */
#define TOKEY_ARROW_DOWN           0x3d  /* arrow down */
#define TOKEY_PAD_1                0x15|SPECIAL_PAD  /* 1 numeric pad */
#define TOKEY_PAD_2                0x25|SPECIAL_PAD  /* 2 numeric pad */
#define TOKEY_PAD_3                0x4e|SPECIAL_PAD  /* 3 numeric pad */
/* 5th row */
#define TOKEY_CAPS_LOCK            0x40  /* CAPS LOCK */
#define TOKEY_W_LOWER_CASE         0x2f|SPECIAL_KB0|SPECIAL_UPC  /* w */
#define TOKEY_W_UPPER_CASE         TOKEY_W_LOWER_CASE|TOKEY_SHIFT  /* W */
#define TOKEY_X_LOWER_CASE         0x27|SPECIAL_KB0|SPECIAL_UPC  /* x */
#define TOKEY_X_UPPER_CASE         TOKEY_X_LOWER_CASE|TOKEY_SHIFT  /* X */
#define TOKEY_C_LOWER_CASE         0x1f|SPECIAL_KB0|SPECIAL_UPC  /* c */
#define TOKEY_C_UPPER_CASE         TOKEY_C_LOWER_CASE|TOKEY_SHIFT  /* C */
#define TOKEY_V_LOWER_CASE         0x17|SPECIAL_UPC  /* v */
#define TOKEY_V_UPPER_CASE         TOKEY_V_LOWER_CASE|TOKEY_SHIFT  /* V */
#define TOKEY_B_LOWER_CASE         0x0f|SPECIAL_UPC  /* b */
#define TOKEY_B_UPPER_CASE         TOKEY_B_LOWER_CASE|TOKEY_SHIFT  /* B */
#define TOKEY_N_LOWER_CASE         0x07|SPECIAL_UPC  /* n */
#define TOKEY_N_UPPER_CASE         TOKEY_N_LOWER_CASE|TOKEY_SHIFT  /* N */
#define TOKEY_COMMA                0x37  /* , */
#define TOKEY_QUESTION_MARK        TOKEY_COMMA|TOKEY_SHIFT  /* ? */
#define TOKEY_SEMICOLON            0x3f|SPECIAL_KB1  /* ; */
#define TOKEY_DOT                  TOKEY_SEMICOLON|TOKEY_SHIFT  /* . */
#define TOKEY_COLON                0x47|SPECIAL_KB1  /* : */
#define TOKEY_SLASH                TOKEY_COLON|TOKEY_SHIFT  /* / */
#define TOKEY_GREATER_THAN         0x4f  /* > */
#define TOKEY_LOWER_THAN           TOKEY_GREATER_THAN|TOKEY_SHIFT  /* < */
#define TOKEY_HOME                 0x06  /* HOME */
#define TOKEY_RAZ                  TOKEY_HOME|TOKEY_SHIFT  /* RAZ */
#define TOKEY_INS                  0x0e  /* INS */
#define TOKEY_EFF                  0x16  /* EFF */
#define TOKEY_DEL                  TOKEY_EFF|TOKEY_SHIFT  /* DEL */
#define TOKEY_PAD_0                0x1e  /* 0 numeric pad */
#define TOKEY_PAD_DOT              0x26  /* . numeric pad */
#define TOKEY_PAD_ENT              0x36  /* ENT numeric pad */
/* 6th row */
#define TOKEY_SPACE                0x34  /* SPACE */


extern int   keyboard_Init(int num_joy);
extern void  keyboard_Reset(int mask, int value);
extern void  keyboard_Press(int key, int release);
extern void  keyboard_Press_ng(int key, int release);
extern void  keyboard_SetACK(int state);

int keyboard_hasFlag(int flag);
void keyboard_ToggleState(int flag, int release);
int keyboard_tokey_to_int(char *tokey);

#endif
