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

    /*This is becoming useless has it's on its way to the higher level*/
    j0_dir[0] = j0_dir[1] = TEO_JOYSTICK_CENTER;
    j1_dir[0] = j1_dir[1] = TEO_JOYSTICK_CENTER;
}

/* Toogles appropriate flags in kb_state.
 * Avalable flags are: TEO_KEY_F_CTRL, TEO_KEY_F_ALTGR, 
 * TEO_KEY_F_NUMLOCK, TEO_KEY_F_CAPSLOCK, TEO_KEY_F_SHIFT
 */
/*TODO: replace release by "state"*/
/*Remembre that we are no more working with key presses, we only set/unset a flag
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


/* keyboard_Press:
 *  Prend en compte la frappe ou le relâchement d'une touche.
 *   key: scancode de la touche frappée/relachée (voir to8keys.h pour la liste).
 *   release: flag d'enfoncement/relâchement.
 */
void keyboard_Press (int key, int release)
{
    int code;
    int joycode;

    printf("Keypressed: %s makes (%c)\n",scancode_to_name(key), scancode_to_ascii(key));
    //printf("Keypressed: %s, TEO_KEY_RSHIFT is %d, TEO_KEY_LSHIFT is %d\n",scancode_to_name(key), TEO_KEY_RSHIFT, TEO_KEY_LSHIFT);
    //printf("name(TEO_KEY_RSHIFT): %s, name(TEO_KEY_LSHIFT): %d\n",scancode_to_name(TEO_KEY_RSHIFT), scancode_to_name(TEO_KEY_LSHIFT));
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
                elsec
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
                                          : kb_state|TEO_KEY_F_SHIFT;
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

            /* special key for joystick 0 (keyboard right) */
            if ((code & SPECIAL_KB0) != 0)
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
                    /* no break */
                }
            }
            else
            /* special key for joystick 1 (keyboard left) */
            if ((code & SPECIAL_KB1) != 0)
            {
                if ((njoy>0) && ((kb_state&TEO_KEY_F_NUMLOCK) == 0))
                {
                    switch (code & JOYSTICK_MASK)
                    {
                        case TOKEY_SEMICOLON :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_COLON :
                            joycode = TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_EXCLAMATION_MARK :
                            joycode = TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_K_LOWER_CASE :
                            joycode = TEO_JOYSTICK_LEFT;
                            break;

                        case TOKEY_L_LOWER_CASE :
                            joycode = TEO_JOYSTICK_CENTER;
                            break;

                        case TOKEY_M_LOWER_CASE :
                            joycode = TEO_JOYSTICK_RIGHT;
                            break;

                        case TOKEY_I_LOWER_CASE :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_O_LOWER_CASE :
                            joycode = TEO_JOYSTICK_DOWN;
                            break;

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
                    /* no break */
                }
            }
            else
            /* special key for joystick 0 (numeric pad) */
            if ((code & SPECIAL_PAD) != 0)
            {
                if ((njoy>0) && ((kb_state&TEO_KEY_F_NUMLOCK) == 0))
                {
                    switch (code & JOYSTICK_MASK)
                    {
                        case TOKEY_PAD_1 :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_PAD_2 :
                            joycode = TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_PAD_3 :
                            joycode = TEO_JOYSTICK_RIGHT|TEO_JOYSTICK_UP;
                            break;

                        case TOKEY_PAD_4 :
                            joycode = TEO_JOYSTICK_LEFT;
                            break;

                        case TOKEY_PAD_5 :
                            joycode = TEO_JOYSTICK_CENTER;
                            break;

                        case TOKEY_PAD_6 :
                            joycode = TEO_JOYSTICK_RIGHT;
                            break;

                        case TOKEY_PAD_7 :
                            joycode = TEO_JOYSTICK_LEFT|TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_PAD_8 :
                            joycode = TEO_JOYSTICK_DOWN;
                            break;

                        case TOKEY_PAD_9 :
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

