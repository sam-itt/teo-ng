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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou 1998
 *  Modifié par: Eric Botcazou 17/09/2001
 *               François Mouret 02/11/2012 20/10/2017
 *
 *  Gestion du clavier (et des manettes).
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stddef.h>
#endif

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
static volatile int j0_dir[2], j1_dir[2]; /* buffer direction des manettes */

#define SPECIAL_NULL   -1      /* key not mapped */
#define SPECIAL_KBD    0x1000  /* special key of main keyboard */
#define SPECIAL_PAD    0x2000  /* special key of numeric pad */
#define SPECIAL_UPC    0x4000  /* upper case if CAPS LOCK */
#define SPECIAL_KEY    0x8000  /* special key (not defined) */

#define TOKEY_SHIFT    0x80
#define TOKEY_CTRL     0x100
#define JOYSTICK_MASK  (0x7f|SPECIAL_KBD|SPECIAL_PAD|SPECIAL_UPC)

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
#define TOKEY_EXCLAMATION_MARK     0x39|SPECIAL_PAD  /* ! */
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
#define TOKEY_A_LOWER_CASE         0x2a|SPECIAL_KBD|SPECIAL_UPC  /* a */
#define TOKEY_A_UPPER_CASE         TOKEY_A_LOWER_CASE|TOKEY_SHIFT  /* A */
#define TOKEY_Z_LOWER_CASE         0x22|SPECIAL_KBD|SPECIAL_UPC  /* z */
#define TOKEY_Z_UPPER_CASE         TOKEY_Z_LOWER_CASE|TOKEY_SHIFT  /* Z */
#define TOKEY_E_LOWER_CASE         0x1a|SPECIAL_KBD|SPECIAL_UPC  /* e */
#define TOKEY_E_UPPER_CASE         TOKEY_E_LOWER_CASE|TOKEY_SHIFT  /* E */
#define TOKEY_R_LOWER_CASE         0x12|SPECIAL_UPC  /* r */
#define TOKEY_R_UPPER_CASE         TOKEY_R_LOWER_CASE|TOKEY_SHIFT  /* R */
#define TOKEY_T_LOWER_CASE         0x0a|SPECIAL_UPC  /* t */
#define TOKEY_T_UPPER_CASE         TOKEY_T_LOWER_CASE|TOKEY_SHIFT  /* T */
#define TOKEY_Y_LOWER_CASE         0x02|SPECIAL_UPC  /* y  */
#define TOKEY_Y_UPPER_CASE         TOKEY_Y_LOWER_CASE|TOKEY_SHIFT  /* Y */
#define TOKEY_U_LOWER_CASE         0x32|SPECIAL_UPC  /* u */
#define TOKEY_U_UPPER_CASE         TOKEY_U_LOWER_CASE|TOKEY_SHIFT  /* U */
#define TOKEY_I_LOWER_CASE         0x3a|SPECIAL_UPC|SPECIAL_PAD  /* i */
#define TOKEY_I_UPPER_CASE         TOKEY_I_LOWER_CASE|TOKEY_SHIFT  /* I */
#define TOKEY_O_LOWER_CASE         0x42|SPECIAL_UPC|SPECIAL_PAD  /* o */
#define TOKEY_O_UPPER_CASE         TOKEY_O_LOWER_CASE|TOKEY_SHIFT  /* O */
#define TOKEY_P_LOWER_CASE         0x4a|SPECIAL_UPC|SPECIAL_PAD  /* p */
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
#define TOKEY_Q_LOWER_CASE         0x2b|SPECIAL_KBD|SPECIAL_UPC  /* q */
#define TOKEY_Q_UPPER_CASE         TOKEY_Q_LOWER_CASE|TOKEY_SHIFT  /* Q */
#define TOKEY_S_LOWER_CASE         0x23|SPECIAL_KBD|SPECIAL_UPC  /* s */ 
#define TOKEY_S_UPPER_CASE         TOKEY_S_LOWER_CASE|TOKEY_SHIFT  /* S */
#define TOKEY_D_LOWER_CASE         0x1b|SPECIAL_KBD|SPECIAL_UPC  /* d */ 
#define TOKEY_D_UPPER_CASE         TOKEY_D_LOWER_CASE|TOKEY_SHIFT  /* D */
#define TOKEY_F_LOWER_CASE         0x13|SPECIAL_UPC  /* f */
#define TOKEY_F_UPPER_CASE         TOKEY_F_LOWER_CASE|TOKEY_SHIFT  /* F */
#define TOKEY_G_LOWER_CASE         0x0b|SPECIAL_UPC  /* g */
#define TOKEY_G_UPPER_CASE         TOKEY_G_LOWER_CASE|TOKEY_SHIFT  /* G */
#define TOKEY_H_LOWER_CASE         0x03|SPECIAL_UPC  /* h  */
#define TOKEY_H_UPPER_CASE         TOKEY_H_LOWER_CASE|TOKEY_SHIFT  /* H */
#define TOKEY_J_LOWER_CASE         0x33|SPECIAL_UPC  /* j */
#define TOKEY_J_UPPER_CASE         TOKEY_J_LOWER_CASE|TOKEY_SHIFT  /* J */
#define TOKEY_K_LOWER_CASE         0x3b|SPECIAL_UPC|SPECIAL_PAD  /* k */
#define TOKEY_K_UPPER_CASE         TOKEY_K_LOWER_CASE|TOKEY_SHIFT  /* K */
#define TOKEY_L_LOWER_CASE         0x43|SPECIAL_UPC|SPECIAL_PAD  /* l */
#define TOKEY_L_UPPER_CASE         TOKEY_L_LOWER_CASE|TOKEY_SHIFT  /* L */
#define TOKEY_M_LOWER_CASE         0x4b|SPECIAL_UPC|SPECIAL_PAD  /* m */
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
#define TOKEY_W_LOWER_CASE         0x2f|SPECIAL_KBD|SPECIAL_UPC  /* w */
#define TOKEY_W_UPPER_CASE         TOKEY_W_LOWER_CASE|TOKEY_SHIFT  /* W */
#define TOKEY_X_LOWER_CASE         0x27|SPECIAL_KBD|SPECIAL_UPC  /* x */
#define TOKEY_X_UPPER_CASE         TOKEY_X_LOWER_CASE|TOKEY_SHIFT  /* X */
#define TOKEY_C_LOWER_CASE         0x1f|SPECIAL_KBD|SPECIAL_UPC  /* c */
#define TOKEY_C_UPPER_CASE         TOKEY_C_LOWER_CASE|TOKEY_SHIFT  /* C */
#define TOKEY_V_LOWER_CASE         0x17|SPECIAL_UPC  /* v */
#define TOKEY_V_UPPER_CASE         TOKEY_V_LOWER_CASE|TOKEY_SHIFT  /* V */
#define TOKEY_B_LOWER_CASE         0x0f|SPECIAL_UPC  /* b */
#define TOKEY_B_UPPER_CASE         TOKEY_B_LOWER_CASE|TOKEY_SHIFT  /* B */
#define TOKEY_N_LOWER_CASE         0x07|SPECIAL_UPC  /* n */
#define TOKEY_N_UPPER_CASE         TOKEY_N_LOWER_CASE|TOKEY_SHIFT  /* N */
#define TOKEY_COMMA                0x37  /* , */
#define TOKEY_QUESTION_MARK        TOKEY_COMMA|TOKEY_SHIFT  /* ? */
#define TOKEY_SEMICOLON            0x3f|SPECIAL_PAD  /* ; */
#define TOKEY_DOT                  TOKEY_SEMICOLON|TOKEY_SHIFT  /* . */
#define TOKEY_COLON                0x47|SPECIAL_PAD  /* : */
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

static int key_code[TEO_KEY_MAX] = {  /* see to8keys.h */
    SPECIAL_NULL,
    TOKEY_Q_LOWER_CASE,            /*  TEO_KEY_A          1    q Q    */
    TOKEY_B_LOWER_CASE,            /*  TEO_KEY_B          2    b B    */
    TOKEY_C_LOWER_CASE,            /*  TEO_KEY_C          3    c C    */
    TOKEY_D_LOWER_CASE,            /*  TEO_KEY_D          4    d D    */
    TOKEY_E_LOWER_CASE,            /*  TEO_KEY_E          5    e E    */
    TOKEY_F_LOWER_CASE,            /*  TEO_KEY_F          6    f F    */
    TOKEY_G_LOWER_CASE,            /*  TEO_KEY_G          7    g G    */
    TOKEY_H_LOWER_CASE,            /*  TEO_KEY_H          8    h H    */
    TOKEY_I_LOWER_CASE,            /*  TEO_KEY_I          9    i I    */
    TOKEY_J_LOWER_CASE,            /*  TEO_KEY_J          10   j J    */
    TOKEY_K_LOWER_CASE,            /*  TEO_KEY_K          11   k K    */
    TOKEY_L_LOWER_CASE,            /*  TEO_KEY_L          12   l L    */
    TOKEY_COMMA,                   /*  TEO_KEY_M          13   , ?    */
    TOKEY_N_LOWER_CASE,            /*  TEO_KEY_N          14   n N    */
    TOKEY_O_LOWER_CASE,            /*  TEO_KEY_O          15   o O    */
    TOKEY_P_LOWER_CASE,            /*  TEO_KEY_P          16   p P    */
    TOKEY_A_LOWER_CASE,            /*  TEO_KEY_Q          17   a A    */
    TOKEY_R_LOWER_CASE,            /*  TEO_KEY_R          18   r R    */
    TOKEY_S_LOWER_CASE,            /*  TEO_KEY_S          19   s S    */
    TOKEY_T_LOWER_CASE,            /*  TEO_KEY_T          20   t T    */
    TOKEY_U_LOWER_CASE,            /*  TEO_KEY_U          21   u U    */
    TOKEY_V_LOWER_CASE,            /*  TEO_KEY_V          22   v V    */
    TOKEY_Z_LOWER_CASE,            /*  TEO_KEY_W          23   z Z    */
    TOKEY_X_LOWER_CASE,            /*  TEO_KEY_X          24   x X    */
    TOKEY_Y_LOWER_CASE,            /*  TEO_KEY_Y          25   y Y    */
    TOKEY_W_LOWER_CASE,            /*  TEO_KEY_Z          26   w W    */
    TOKEY_A_GRAVE_LOWER_CASE,      /*  TEO_KEY_0          27   à 0 @  */
    TOKEY_AMPERSAND,               /*  TEO_KEY_1          28   & 1    */
    TOKEY_E_ACUTE_LOWER_CASE,      /*  TEO_KEY_2          29   é 2    */
    TOKEY_QUOTE,                   /*  TEO_KEY_3          30   " 3 #  */
    TOKEY_APOSTROPHE,              /*  TEO_KEY_4          31   ' 4 {  */
    TOKEY_OPEN_BRACKET,            /*  TEO_KEY_5          32   ( 5 [  */
    TOKEY_MINUS,                   /*  TEO_KEY_6          33   - 6    */
    TOKEY_E_GRAVE_LOWER_CASE,      /*  TEO_KEY_7          34   è 7    */
    TOKEY_UNDERSCORE,              /*  TEO_KEY_8          35   _ 8 \  */
    TOKEY_C_CEDILLA_LOWER_CASE,    /*  TEO_KEY_9          36   ç 9 ^  */
    TOKEY_PAD_0,                   /*  TEO_KEY_0_PAD      37   0 0 0  */
    TOKEY_PAD_1,                   /*  TEO_KEY_1_PAD      38   1 1 1  */
    TOKEY_PAD_2,                   /*  TEO_KEY_2_PAD      39   2 2 2  */
    TOKEY_PAD_3,                   /*  TEO_KEY_3_PAD      40   3 3 3  */
    TOKEY_PAD_4,                   /*  TEO_KEY_4_PAD      41   4 4 4  */
    TOKEY_PAD_5,                   /*  TEO_KEY_5_PAD      42   5 5 5  */
    TOKEY_PAD_6,                   /*  TEO_KEY_6_PAD      43   6 6 6  */
    TOKEY_PAD_7,                   /*  TEO_KEY_7_PAD      44   7 7 7  */
    TOKEY_PAD_8,                   /*  TEO_KEY_8_PAD      45   8 8 8  */
    TOKEY_PAD_9,                   /*  TEO_KEY_9_PAD      46   9 9 9  */
    TOKEY_F1,                      /*  TEO_KEY_F1         47          */
    TOKEY_F2,                      /*  TEO_KEY_F2         48          */
    TOKEY_F3,                      /*  TEO_KEY_F3         49          */
    TOKEY_F4,                      /*  TEO_KEY_F4         50          */
    TOKEY_F5,                      /*  TEO_KEY_F5         51          */
    TOKEY_F6,                      /*  TEO_KEY_F6         52          */
    TOKEY_F7,                      /*  TEO_KEY_F7         53          */
    TOKEY_F8,                      /*  TEO_KEY_F8         54          */
    TOKEY_F9,                      /*  TEO_KEY_F9         55          */
    TOKEY_F10,                     /*  TEO_KEY_F10        56          */
    SPECIAL_NULL,                  /*  TEO_KEY_F11        57          */
    SPECIAL_NULL,                  /*  TEO_KEY_F12        58          */
    SPECIAL_NULL,                  /*  TEO_KEY_ESC        59          */
    SPECIAL_NULL,                  /*  TEO_KEY_TILDE      60          */
    TOKEY_CLOSE_BRACKET,           /*  TEO_KEY_MINUS      61   ) ° ]  */
    TOKEY_EQUAL,                   /*  TEO_KEY_EQUALS     62   = + }  */
    TOKEY_DEL,                     /*  TEO_KEY_BACKSPACE  63          */
    TOKEY_STOP,                    /*  TEO_KEY_TAB        64          */
    TOKEY_CIRCUMFLEX,              /*  TEO_KEY_OPENBRACE  65   ^      */
    TOKEY_DOLLAR,                  /*  TEO_KEY_CLOSEBRACE 66   $      */
    TOKEY_ENT,                     /*  TEO_KEY_ENTER      67          */
    TOKEY_M_LOWER_CASE,            /*  TEO_KEY_COLON      68   m M    */
    TOKEY_U_GRAVE_LOWER_CASE,      /*  TEO_KEY_QUOTE      69   ù %    */
    TOKEY_AMPERSAND,               /*  TEO_KEY_BACKSLASH  70   * $    */
    TOKEY_LOWER_THAN,              /*  TEO_KEY_BACKSLASH2 71   < >    */
    TOKEY_SEMICOLON,               /*  TEO_KEY_COMMA      72   ; .    */
    TOKEY_COLON,                   /*  TEO_KEY_STOP       73   : /    */
    TOKEY_EXCLAMATION_MARK,        /*  TEO_KEY_SLASH      74   !      */
    TOKEY_SPACE,                   /*  TEO_KEY_SPACE      75          */
    TOKEY_INS,                     /*  TEO_KEY_INSERT     76          */
    TOKEY_EFF,                     /*  TEO_KEY_DEL        77          */
    TOKEY_HOME,                    /*  TEO_KEY_HOME       78          */
    SPECIAL_NULL,                  /*  TEO_KEY_END        79          */
    SPECIAL_NULL,                  /*  TEO_KEY_PGUP       80          */
    SPECIAL_NULL,                  /*  TEO_KEY_PGDN       81          */
    TOKEY_ARROW_LEFT,              /*  TEO_KEY_LEFT       82          */
    TOKEY_ARROW_RIGHT,             /*  TEO_KEY_RIGHT      83          */
    TOKEY_ARROW_UP,                /*  TEO_KEY_UP         84          */
    TOKEY_ARROW_DOWN,              /*  TEO_KEY_DOWN       85          */
    TOKEY_SLASH,                   /*  TEO_KEY_SLASH_PAD  86   / :    */
    TOKEY_ASTERISK,                /*  TEO_KEY_ASTERISK   87   *      */
    TOKEY_MINUS,                   /*  TEO_KEY_MINUS_PAD  88   -      */
    TOKEY_PLUS,                    /*  TEO_KEY_PLUS_PAD   89   +      */
    TOKEY_DOT,                     /*  TEO_KEY_DEL_PAD    90   .      */
    TOKEY_PAD_ENT,                 /*  TEO_KEY_ENTER_PAD  91          */
    SPECIAL_NULL,                  /*  TEO_KEY_PRTSCR     92          */
    SPECIAL_NULL,                  /*  TEO_KEY_PAUSE      93          */
    SPECIAL_NULL,                  /*  TEO_KEY_ABNT_C1    94          */
    SPECIAL_NULL,                  /*  TEO_KEY_YEN        95          */
    SPECIAL_NULL,                  /*  TEO_KEY_KANA       96          */
    SPECIAL_NULL,                  /*  TEO_KEY_CONVERT    97          */
    SPECIAL_NULL,                  /*  TEO_KEY_NOCONVERT  98          */
    SPECIAL_NULL,                  /*  TEO_KEY_AT         99          */
    SPECIAL_NULL,                  /*  TEO_KEY_CIRCUMFLEX 100         */
    SPECIAL_NULL,                  /*  TEO_KEY_COLON2     101         */
    SPECIAL_NULL,                  /*  TEO_KEY_KANJI      102         */
    SPECIAL_KEY,                   /*  TEO_KEY_LSHIFT     103         */
    SPECIAL_KEY,                   /*  TEO_KEY_RSHIFT     104         */
    SPECIAL_KEY,                   /*  TEO_KEY_LCONTROL   105         */
    SPECIAL_KEY,                   /*  TEO_KEY_RCONTROL   106         */
    TOKEY_ACC,                     /*  TEO_KEY_ALT        107         */
    SPECIAL_KEY,                   /*  TEO_KEY_ALTGR      108         */
    SPECIAL_NULL,                  /*  TEO_KEY_LWIN       109         */
    SPECIAL_NULL,                  /*  TEO_KEY_RWIN       110         */
    SPECIAL_NULL,                  /*  TEO_KEY_MENU       111         */
    SPECIAL_NULL,                  /*  TEO_KEY_SCRLOCK    112         */
    SPECIAL_KEY,                   /*  TEO_KEY_NUMLOCK    113         */
    TOKEY_CAPS_LOCK                /*  TEO_KEY_CAPSLOCK   114         */
};

static int key_shift_code[TEO_KEY_MAX]={  /* see to8keys.h */
    SPECIAL_NULL,
    TOKEY_Q_UPPER_CASE,            /*  TEO_KEY_A          1    q Q    */
    TOKEY_B_UPPER_CASE,            /*  TEO_KEY_B          2    b B    */
    TOKEY_C_UPPER_CASE,            /*  TEO_KEY_C          3    c C    */
    TOKEY_D_UPPER_CASE,            /*  TEO_KEY_D          4    d D    */
    TOKEY_E_UPPER_CASE,            /*  TEO_KEY_E          5    e E    */
    TOKEY_F_UPPER_CASE,            /*  TEO_KEY_F          6    f F    */
    TOKEY_G_UPPER_CASE,            /*  TEO_KEY_G          7    g G    */
    TOKEY_H_UPPER_CASE,            /*  TEO_KEY_H          8    h H    */
    TOKEY_I_UPPER_CASE,            /*  TEO_KEY_I          9    i I    */
    TOKEY_J_UPPER_CASE,            /*  TEO_KEY_J          10   j J    */
    TOKEY_K_UPPER_CASE,            /*  TEO_KEY_K          11   k K    */
    TOKEY_L_UPPER_CASE,            /*  TEO_KEY_L          12   l L    */
    TOKEY_QUESTION_MARK,           /*  TEO_KEY_M          13   , ?    */
    TOKEY_N_UPPER_CASE,            /*  TEO_KEY_N          14   n N    */
    TOKEY_O_UPPER_CASE,            /*  TEO_KEY_O          15   o O    */
    TOKEY_P_UPPER_CASE,            /*  TEO_KEY_P          16   p P    */
    TOKEY_A_UPPER_CASE,            /*  TEO_KEY_Q          17   a A    */
    TOKEY_R_UPPER_CASE,            /*  TEO_KEY_R          18   r R    */
    TOKEY_S_UPPER_CASE,            /*  TEO_KEY_S          19   s S    */
    TOKEY_T_UPPER_CASE,            /*  TEO_KEY_T          20   t T    */
    TOKEY_U_UPPER_CASE,            /*  TEO_KEY_U          21   u U    */
    TOKEY_V_UPPER_CASE,            /*  TEO_KEY_V          22   v V    */
    TOKEY_Z_UPPER_CASE,            /*  TEO_KEY_W          23   z Z    */
    TOKEY_X_UPPER_CASE,            /*  TEO_KEY_X          24   x X    */
    TOKEY_Y_UPPER_CASE,            /*  TEO_KEY_Y          25   y Y    */
    TOKEY_W_UPPER_CASE,            /*  TEO_KEY_Z          26   w W    */
    TOKEY_0,                       /*  TEO_KEY_0          27   à 0 @  */
    TOKEY_1,                       /*  TEO_KEY_1          28   & 1    */
    TOKEY_2,                       /*  TEO_KEY_2          29   é 2    */
    TOKEY_3,                       /*  TEO_KEY_3          30   " 3 #  */
    TOKEY_4,                       /*  TEO_KEY_4          31   ' 4 {  */
    TOKEY_5,                       /*  TEO_KEY_5          32   ( 5 [  */
    TOKEY_6,                       /*  TEO_KEY_6          33   - 6    */
    TOKEY_7,                       /*  TEO_KEY_7          34   è 7    */
    TOKEY_8,                       /*  TEO_KEY_8          35   _ 8 \  */
    TOKEY_9,                       /*  TEO_KEY_9          36   ç 9 ^  */
    TOKEY_PAD_0,                   /*  TEO_KEY_0_PAD      37   0 0 0  */
    TOKEY_PAD_1,                   /*  TEO_KEY_1_PAD      38   1 1 1  */
    TOKEY_PAD_2,                   /*  TEO_KEY_2_PAD      39   2 2 2  */
    TOKEY_PAD_3,                   /*  TEO_KEY_3_PAD      40   3 3 3  */
    TOKEY_PAD_4,                   /*  TEO_KEY_4_PAD      41   4 4 4  */
    TOKEY_PAD_5,                   /*  TEO_KEY_5_PAD      42   5 5 5  */
    TOKEY_PAD_6,                   /*  TEO_KEY_6_PAD      43   6 6 6  */
    TOKEY_PAD_7,                   /*  TEO_KEY_7_PAD      44   7 7 7  */
    TOKEY_PAD_8,                   /*  TEO_KEY_8_PAD      45   8 8 8  */
    TOKEY_PAD_9,                   /*  TEO_KEY_9_PAD      46   9 9 9  */
    TOKEY_F1,                      /*  TEO_KEY_F1         47          */
    TOKEY_F2,                      /*  TEO_KEY_F2         48          */
    TOKEY_F3,                      /*  TEO_KEY_F3         49          */
    TOKEY_F4,                      /*  TEO_KEY_F4         50          */
    TOKEY_F5,                      /*  TEO_KEY_F5         51          */
    TOKEY_F6,                      /*  TEO_KEY_F6         52          */
    TOKEY_F7,                      /*  TEO_KEY_F7         53          */
    TOKEY_F8,                      /*  TEO_KEY_F8         54          */
    TOKEY_F9,                      /*  TEO_KEY_F9         55          */
    TOKEY_F10,                     /*  TEO_KEY_F10        56          */
    SPECIAL_NULL,                  /*  TEO_KEY_F11        57          */
    SPECIAL_NULL,                  /*  TEO_KEY_F12        58          */
    SPECIAL_NULL,                  /*  TEO_KEY_ESC        59          */
    SPECIAL_NULL,                  /*  TEO_KEY_TILDE      60          */
    TOKEY_DEGREE,                  /*  TEO_KEY_MINUS      61   ) ° ]  */
    TOKEY_PLUS,                    /*  TEO_KEY_EQUALS     62   = + }  */
    TOKEY_DEL,                     /*  TEO_KEY_BACKSPACE  63          */
    TOKEY_STOP,                    /*  TEO_KEY_TAB        64          */
    SPECIAL_NULL,                  /*  TEO_KEY_OPENBRACE  65   ^      */
    SPECIAL_NULL,                  /*  TEO_KEY_CLOSEBRACE 66   $      */
    TOKEY_ENT,                     /*  TEO_KEY_ENTER      67          */
    TOKEY_M_UPPER_CASE,            /*  TEO_KEY_COLON      68   m M    */
    TOKEY_PERCENT,                 /*  TEO_KEY_QUOTE      69   ù %    */
    TOKEY_AMPERSAND,               /*  TEO_KEY_BACKSLASH  70   * $    */
    TOKEY_GREATER_THAN,            /*  TEO_KEY_BACKSLASH2 71   < >    */
    TOKEY_DOT,                     /*  TEO_KEY_COMMA      72   ; .    */
    TOKEY_SLASH,                   /*  TEO_KEY_STOP       73   : /    */
    SPECIAL_NULL,                  /*  TEO_KEY_SLASH      74   !      */
    TOKEY_SPACE,                   /*  TEO_KEY_SPACE      75          */
    TOKEY_INS,                     /*  TEO_KEY_INSERT     76          */
    TOKEY_EFF,                     /*  TEO_KEY_DEL        77          */
    TOKEY_RAZ,                     /*  TEO_KEY_HOME       78          */
    SPECIAL_NULL,                  /*  TEO_KEY_END        79          */
    SPECIAL_NULL,                  /*  TEO_KEY_PGUP       80          */
    SPECIAL_NULL,                  /*  TEO_KEY_PGDN       81          */
    TOKEY_ARROW_LEFT,              /*  TEO_KEY_LEFT       82          */
    TOKEY_ARROW_RIGHT,             /*  TEO_KEY_RIGHT      83          */
    TOKEY_ARROW_UP,                /*  TEO_KEY_UP         84          */
    TOKEY_ARROW_DOWN,              /*  TEO_KEY_DOWN       85          */
    TOKEY_SLASH,                   /*  TEO_KEY_SLASH_PAD  86   / :    */
    TOKEY_ASTERISK,                /*  TEO_KEY_ASTERISK   87   *      */
    TOKEY_MINUS,                   /*  TEO_KEY_MINUS_PAD  88   -      */
    TOKEY_PLUS,                    /*  TEO_KEY_PLUS_PAD   89   +      */
    TOKEY_DOT,                     /*  TEO_KEY_DEL_PAD    90   .      */
    TOKEY_PAD_ENT,                 /*  TEO_KEY_ENTER_PAD  91          */
    SPECIAL_NULL,                  /*  TEO_KEY_PRTSCR     92          */
    SPECIAL_NULL,                  /*  TEO_KEY_PAUSE      93          */
    SPECIAL_NULL,                  /*  TEO_KEY_ABNT_C1    94          */
    SPECIAL_NULL,                  /*  TEO_KEY_YEN        95          */
    SPECIAL_NULL,                  /*  TEO_KEY_KANA       96          */
    SPECIAL_NULL,                  /*  TEO_KEY_CONVERT    97          */
    SPECIAL_NULL,                  /*  TEO_KEY_NOCONVERT  98          */
    SPECIAL_NULL,                  /*  TEO_KEY_AT         99          */
    SPECIAL_NULL,                  /*  TEO_KEY_CIRCUMFLEX 100         */
    SPECIAL_NULL,                  /*  TEO_KEY_COLON2     101         */
    SPECIAL_NULL,                  /*  TEO_KEY_KANJI      102         */
    SPECIAL_KEY,                   /*  TEO_KEY_LSHIFT     103         */
    SPECIAL_KEY,                   /*  TEO_KEY_RSHIFT     104         */
    SPECIAL_KEY,                   /*  TEO_KEY_LCONTROL   105         */
    SPECIAL_KEY,                   /*  TEO_KEY_RCONTROL   106         */
    TOKEY_ACC,                     /*  TEO_KEY_ALT        107         */
    SPECIAL_KEY,                   /*  TEO_KEY_ALTGR      108         */
    SPECIAL_NULL,                  /*  TEO_KEY_LWIN       109         */
    SPECIAL_NULL,                  /*  TEO_KEY_RWIN       110         */
    SPECIAL_NULL,                  /*  TEO_KEY_MENU       111         */
    SPECIAL_NULL,                  /*  TEO_KEY_SCRLOCK    112         */
    SPECIAL_KEY,                   /*  TEO_KEY_NUMLOCK    113         */
    TOKEY_CAPS_LOCK                /*  TEO_KEY_CAPSLOCK   114         */
};

static int key_altgr_code[TEO_KEY_MAX]={  /* see to8keys.h */
    SPECIAL_NULL,
    TOKEY_Q_LOWER_CASE,            /*  TEO_KEY_A          1    q Q    */
    TOKEY_B_LOWER_CASE,            /*  TEO_KEY_B          2    b B    */
    TOKEY_C_LOWER_CASE,            /*  TEO_KEY_C          3    c C    */
    TOKEY_D_LOWER_CASE,            /*  TEO_KEY_D          4    d D    */
    TOKEY_E_LOWER_CASE,            /*  TEO_KEY_E          5    e E    */
    TOKEY_F_LOWER_CASE,            /*  TEO_KEY_F          6    f F    */
    TOKEY_G_LOWER_CASE,            /*  TEO_KEY_G          7    g G    */
    TOKEY_H_LOWER_CASE,            /*  TEO_KEY_H          8    h H    */
    TOKEY_I_LOWER_CASE,            /*  TEO_KEY_I          9    i I    */
    TOKEY_J_LOWER_CASE,            /*  TEO_KEY_J          10   j J    */
    TOKEY_K_LOWER_CASE,            /*  TEO_KEY_K          11   k K    */
    TOKEY_L_LOWER_CASE,            /*  TEO_KEY_L          12   l L    */
    TOKEY_COMMA,                   /*  TEO_KEY_M          13   , ?    */
    TOKEY_N_LOWER_CASE,            /*  TEO_KEY_N          14   n N    */
    TOKEY_O_LOWER_CASE,            /*  TEO_KEY_O          15   o O    */
    TOKEY_P_LOWER_CASE,            /*  TEO_KEY_P          16   p P    */
    TOKEY_A_LOWER_CASE,            /*  TEO_KEY_Q          17   a A    */
    TOKEY_R_LOWER_CASE,            /*  TEO_KEY_R          18   r R    */
    TOKEY_S_LOWER_CASE,            /*  TEO_KEY_S          19   s S    */
    TOKEY_T_LOWER_CASE,            /*  TEO_KEY_T          20   t T    */
    TOKEY_U_LOWER_CASE,            /*  TEO_KEY_U          21   u U    */
    TOKEY_V_LOWER_CASE,            /*  TEO_KEY_V          22   v V    */
    TOKEY_Z_LOWER_CASE,            /*  TEO_KEY_W          23   z Z    */
    TOKEY_X_LOWER_CASE,            /*  TEO_KEY_X          24   x X    */
    TOKEY_Y_LOWER_CASE,            /*  TEO_KEY_Y          25   y Y    */
    TOKEY_W_LOWER_CASE,            /*  TEO_KEY_Z          26   w W    */
    TOKEY_AT,                      /*  TEO_KEY_0          27   à 0 @  */
    SPECIAL_NULL,                  /*  TEO_KEY_1          28   & 1    */
    SPECIAL_NULL,                  /*  TEO_KEY_2          29   é 2    */
    TOKEY_NUMBER_SIGN,             /*  TEO_KEY_3          30   " 3 #  */
    TOKEY_OPEN_BRACES,             /*  TEO_KEY_4          31   ' 4 {  */
    TOKEY_OPEN_SQUARE_BRACKET,     /*  TEO_KEY_5          32   ( 5 [  */
    SPECIAL_NULL,                  /*  TEO_KEY_6          33   - 6    */
    SPECIAL_NULL,                  /*  TEO_KEY_7          34   è 7    */
    TOKEY_BACKSLASH,               /*  TEO_KEY_8          35   _ 8 \  */
    TOKEY_CIRCUMFLEX,              /*  TEO_KEY_9          36   ç 9 ^  */
    TOKEY_PAD_0,                   /*  TEO_KEY_0_PAD      37   0 0 0  */
    TOKEY_PAD_1,                   /*  TEO_KEY_1_PAD      38   1 1 1  */
    TOKEY_PAD_2,                   /*  TEO_KEY_2_PAD      39   2 2 2  */
    TOKEY_PAD_3,                   /*  TEO_KEY_3_PAD      40   3 3 3  */
    TOKEY_PAD_4,                   /*  TEO_KEY_4_PAD      41   4 4 4  */
    TOKEY_PAD_5,                   /*  TEO_KEY_5_PAD      42   5 5 5  */
    TOKEY_PAD_6,                   /*  TEO_KEY_6_PAD      43   6 6 6  */
    TOKEY_PAD_7,                   /*  TEO_KEY_7_PAD      44   7 7 7  */
    TOKEY_PAD_8,                   /*  TEO_KEY_8_PAD      45   8 8 8  */
    TOKEY_PAD_9,                   /*  TEO_KEY_9_PAD      46   9 9 9  */
    TOKEY_F1,                      /*  TEO_KEY_F1         47          */
    TOKEY_F2,                      /*  TEO_KEY_F2         48          */
    TOKEY_F3,                      /*  TEO_KEY_F3         49          */
    TOKEY_F4,                      /*  TEO_KEY_F4         50          */
    TOKEY_F5,                      /*  TEO_KEY_F5         51          */
    TOKEY_F6,                      /*  TEO_KEY_F6         52          */
    TOKEY_F7,                      /*  TEO_KEY_F7         53          */
    TOKEY_F8,                      /*  TEO_KEY_F8         54          */
    TOKEY_F9,                      /*  TEO_KEY_F9         55          */
    TOKEY_F10,                     /*  TEO_KEY_F10        56          */
    SPECIAL_NULL,                  /*  TEO_KEY_F11        57          */
    SPECIAL_NULL,                  /*  TEO_KEY_F12        58          */
    SPECIAL_NULL,                  /*  TEO_KEY_ESC        59          */
    SPECIAL_NULL,                  /*  TEO_KEY_TILDE      60          */
    TOKEY_CLOSE_SQUARE_BRACKET,    /*  TEO_KEY_MINUS      61   ) ° ]  */
    TOKEY_CLOSE_BRACES,            /*  TEO_KEY_EQUALS     62   = + }  */
    SPECIAL_NULL,                  /*  TEO_KEY_BACKSPACE  63          */
    TOKEY_STOP,                    /*  TEO_KEY_TAB        64          */
    SPECIAL_NULL,                  /*  TEO_KEY_OPENBRACE  65   ^      */
    SPECIAL_NULL,                  /*  TEO_KEY_CLOSEBRACE 66   $      */
    SPECIAL_NULL,                  /*  TEO_KEY_ENTER      67          */
    SPECIAL_NULL,                  /*  TEO_KEY_COLON      68   m M    */
    SPECIAL_NULL,                  /*  TEO_KEY_QUOTE      69   ù %    */
    SPECIAL_NULL,                  /*  TEO_KEY_BACKSLASH  70   * $    */
    SPECIAL_NULL,                  /*  TEO_KEY_BACKSLASH2 71   < >    */
    SPECIAL_NULL,                  /*  TEO_KEY_COMMA      72   ; .    */
    SPECIAL_NULL,                  /*  TEO_KEY_STOP       73   : /    */
    SPECIAL_NULL,                  /*  TEO_KEY_SLASH      74   !      */
    SPECIAL_NULL,                  /*  TEO_KEY_SPACE      75          */
    SPECIAL_NULL,                  /*  TEO_KEY_INSERT     76          */
    SPECIAL_NULL,                  /*  TEO_KEY_DEL        77          */
    SPECIAL_NULL,                  /*  TEO_KEY_HOME       78          */
    SPECIAL_NULL,                  /*  TEO_KEY_END        79          */
    SPECIAL_NULL,                  /*  TEO_KEY_PGUP       80          */
    SPECIAL_NULL,                  /*  TEO_KEY_PGDN       81          */
    TOKEY_ARROW_LEFT,              /*  TEO_KEY_LEFT       82          */
    TOKEY_ARROW_RIGHT,             /*  TEO_KEY_RIGHT      83          */
    TOKEY_ARROW_UP,                /*  TEO_KEY_UP         84          */
    TOKEY_ARROW_DOWN,              /*  TEO_KEY_DOWN       85          */
    TOKEY_SLASH,                   /*  TEO_KEY_SLASH_PAD  86   / :    */
    TOKEY_ASTERISK,                /*  TEO_KEY_ASTERISK   87   *      */
    TOKEY_MINUS,                   /*  TEO_KEY_MINUS_PAD  88   -      */
    TOKEY_PLUS,                    /*  TEO_KEY_PLUS_PAD   89   +      */
    TOKEY_DOT,                     /*  TEO_KEY_DEL_PAD    90   .      */
    TOKEY_PAD_ENT,                 /*  TEO_KEY_ENTER_PAD  91          */
    SPECIAL_NULL,                  /*  TEO_KEY_PRTSCR     92          */
    SPECIAL_NULL,                  /*  TEO_KEY_PAUSE      93          */
    SPECIAL_NULL,                  /*  TEO_KEY_ABNT_C1    94          */
    SPECIAL_NULL,                  /*  TEO_KEY_YEN        95          */
    SPECIAL_NULL,                  /*  TEO_KEY_KANA       96          */
    SPECIAL_NULL,                  /*  TEO_KEY_CONVERT    97          */
    SPECIAL_NULL,                  /*  TEO_KEY_NOCONVERT  98          */
    SPECIAL_NULL,                  /*  TEO_KEY_AT         99          */
    SPECIAL_NULL,                  /*  TEO_KEY_CIRCUMFLEX 100         */
    SPECIAL_NULL,                  /*  TEO_KEY_COLON2     101         */
    SPECIAL_NULL,                  /*  TEO_KEY_KANJI      102         */
    SPECIAL_KEY,                   /*  TEO_KEY_LSHIFT     103         */
    SPECIAL_KEY,                   /*  TEO_KEY_RSHIFT     104         */
    SPECIAL_KEY,                   /*  TEO_KEY_LCONTROL   105         */
    SPECIAL_KEY,                   /*  TEO_KEY_RCONTROL   106         */
    TOKEY_ACC,                     /*  TEO_KEY_ALT        107         */
    SPECIAL_KEY,                   /*  TEO_KEY_ALTGR      108         */
    SPECIAL_NULL,                  /*  TEO_KEY_LWIN       109         */
    SPECIAL_NULL,                  /*  TEO_KEY_RWIN       110         */
    SPECIAL_NULL,                  /*  TEO_KEY_MENU       111         */
    SPECIAL_NULL,                  /*  TEO_KEY_SCRLOCK    112         */
    SPECIAL_KEY,                   /*  TEO_KEY_NUMLOCK    113         */
    TOKEY_CAPS_LOCK                /*  TEO_KEY_CAPSLOCK   114         */
};


/* ------------------------------------------------------------------------- */


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

    j0_dir[0] = j0_dir[1] = TEO_JOYSTICK_CENTER;
    j1_dir[0] = j1_dir[1] = TEO_JOYSTICK_CENTER;
}



/* keyboard_Press:
 *  Prend en compte la frappe ou le relâchement d'une touche.
 *   key: scancode de la touche frappée/relachée (voir to8keys.h pour la liste).
 *   release: flag d'enfoncement/relâchement.
 */
void keyboard_Press (int key, int release)
{
    int code;
    int joycode;

    switch (key)
    {
        case TEO_KEY_LCONTROL:  /* le contrôle gauche émule la touche CNT
                                et le bouton joystick 1 (NUMLOCK éteint) */
            if ((njoy>1) && !(kb_state&TEO_KEY_F_NUMLOCK))
                joystick_Button (njoy-1, 0,
                                 (release != 0) ? TEO_JOYSTICK_FIRE_OFF
                                                : TEO_JOYSTICK_FIRE_ON);

            kb_state = (release != 0) ? kb_state&~TEO_KEY_F_CTRL
                                      : kb_state|TEO_KEY_F_CTRL;
            break;

        case TEO_KEY_RCONTROL:  /* le contrôle droit émule le bouton joystick
                                0 ou 1 en mode manette (NUMLOCK éteint) */
            if ((njoy>0) && !(kb_state&TEO_KEY_F_NUMLOCK))
                joystick_Button (TEO_NJOYSTICKS-njoy, 0,
                                 (release != 0) ? TEO_JOYSTICK_FIRE_OFF
                                                : TEO_JOYSTICK_FIRE_ON);

            break;

        case TEO_KEY_ALTGR:
            kb_state = (release != 0) ? kb_state&~TEO_KEY_F_ALTGR
                                      : kb_state|TEO_KEY_F_ALTGR;
            break;

        case TEO_KEY_NUMLOCK:
            if (release == 0)
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
                joystick_Button (TEO_NJOYSTICKS-njoy, 0,
                                 (release != 0) ? TEO_JOYSTICK_FIRE_OFF
                                                : TEO_JOYSTICK_FIRE_ON);
            }
            else
            {
#ifdef TEO_DOUBLE_CAPSLOCK
                if (kb_state&TEO_KEY_F_CAPSLOCK)
                    key = TEO_KEY_CAPSLOCK;
                else
                {
                    kb_state = (release != 0) ? kb_state&~TEO_KEY_F_SHIFT
                                              : kb_state|TEO_KEY_F_SHIFT;
                    break;
                }
#else
                kb_state = (release != 0) ? kb_state&~TEO_KEY_F_SHIFT
                                          : kb_state|TEO_KEY_F_SHIFT;
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
                kb_state = (release != 0) ? kb_state&~TEO_KEY_F_SHIFT
                                          : kb_state|TEO_KEY_F_SHIFT);
                break;
            }
#else
            kb_state = (release != 0) ? kb_state&~TEO_KEY_F_SHIFT
                                      : kb_state|TEO_KEY_F_SHIFT;
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
            if (((kb_state&TEO_KEY_F_SHIFT) != 0)
             || (((kb_state&TEO_KEY_F_CAPSLOCK) != 0)
              && ((key_code[key]&SPECIAL_UPC) != 0)))
            {
                /* everything is upper case if SHIFT */
                /* only letters are upper case if CAPS LOCK */
                code = key_shift_code[key];
            }
            else
            {
                code = key_code[key];
            }

            /* special key of the main keyboard */
            if ((code & SPECIAL_KBD) != 0)
            {
                if ((njoy>1) && ((kb_state&TEO_KEY_F_NUMLOCK) == 0))
                {
                    switch (code & JOYSTICK_MASK)
                    {
                        case TOKEY_A_LOWER_CASE :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_Z_LOWER_CASE :
                            joycode = TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_E_LOWER_CASE :
                            joycode = TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_Q_LOWER_CASE :
                            joycode = TEO_JOYSTICK_LEFT;
                            break;

                        case TOKEY_S_LOWER_CASE :
                            joycode = TEO_JOYSTICK_CENTER;
                            break;

                        case TOKEY_D_LOWER_CASE :
                            joycode = TEO_JOYSTICK_RIGHT;
                            break;

                        case TOKEY_W_LOWER_CASE :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_X_LOWER_CASE :
                            joycode = TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_C_LOWER_CASE :
                            joycode = TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_UP;
                            break;

                        default :
                            joycode = TEO_JOYSTICK_CENTER;
                            break;
                    }

                    if (release != 0)
                    {
                        if (joycode == j1_dir[0])
                        {
                            j1_dir[0]=j1_dir[1];
                            j1_dir[1]=TEO_JOYSTICK_CENTER;
                        }
                        else
                        if (joycode == j1_dir[1])
                        {
                            j1_dir[1]=TEO_JOYSTICK_CENTER;
                        }
                    }
                    else
                    if (joycode != j1_dir[0])
                    {
                        j1_dir[1]=j1_dir[0];
                        j1_dir[0]=joycode;
                    }

                    joystick_Move (njoy-1, j1_dir[0]);
                    /* pas de break */
                }
            }
            else
            /* special key of the numeric pad */
            if ((code & SPECIAL_PAD) != 0)
            {
                if ((njoy>0) && ((kb_state&TEO_KEY_F_NUMLOCK) == 0))
                {
                    switch (code & JOYSTICK_MASK)
                    {
                        case TOKEY_PAD_1 :
                        case TOKEY_SEMICOLON :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_PAD_2 :
                        case TOKEY_COLON :
                            joycode = TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_PAD_3 :
                        case TOKEY_EXCLAMATION_MARK :
                            joycode = TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_PAD_4 :
                        case TOKEY_K_LOWER_CASE :
                            joycode = TEO_JOYSTICK_LEFT;
                            break;

                        case TOKEY_PAD_5 :
                        case TOKEY_L_LOWER_CASE :
                            joycode = TEO_JOYSTICK_CENTER;
                            break;

                        case TOKEY_PAD_6 :
                        case TOKEY_M_LOWER_CASE :
                            joycode = TEO_JOYSTICK_RIGHT;
                            break;

                        case TOKEY_PAD_7 :
                        case TOKEY_I_LOWER_CASE :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_PAD_8 :
                        case TOKEY_O_LOWER_CASE :
                            joycode = TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_PAD_9 :
                        case TOKEY_P_LOWER_CASE :
                            joycode = TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_DOWN;
                            break;

                        default :
                            joycode = TEO_JOYSTICK_CENTER;
                            break;
                    }

                    if (release != 0)
                    {
                        if (joycode == j0_dir[0])
                        {
                            j0_dir[0]=j0_dir[1];
                            j0_dir[1]=TEO_JOYSTICK_CENTER;
                        }
                        else
                        if (joycode == j0_dir[1])
                        {
                            j0_dir[1]=TEO_JOYSTICK_CENTER;
                        }
                    }
                    else
                    if (joycode != j0_dir[0])
                    {
                        j0_dir[1]=j0_dir[0];
                        j0_dir[0]=joycode;
                    }

                    joystick_Move (TEO_NJOYSTICKS-njoy, j0_dir[0]);
                    break; /* fin du traitement pour le pavé numérique en mode manette */
                }
            }
            else
            /* on remplace le code donné par key_code[] par celui donné par
               key_altgr_code[] si ce dernier est non nul et si AltGr est
               pressée */
            if (((kb_state&TEO_KEY_F_ALTGR) != 0)
             && (key_altgr_code[key] != SPECIAL_NULL))
            {
                code = key_altgr_code[key];
            }

            /* mapped key */
            if (code != SPECIAL_NULL)
            {
                /* activate CNT if requested */
                if ((kb_state&TEO_KEY_F_CTRL) != 0)
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
                    mc6804_SetScanCode (&mc6846, code & 0X1FF);
                }
            }
            break;

    } /* end of switch */
}

END_OF_FUNCTION(keyboard_Press)


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
    LOCK_VARIABLE(key_code);
    LOCK_VARIABLE(key_shift_code);
    LOCK_VARIABLE(key_altgr_code);
    LOCK_VARIABLE(j0_dir);
    LOCK_VARIABLE(j1_dir);
    LOCK_FUNCTION(keyboard_Press);

    return 0;
}

