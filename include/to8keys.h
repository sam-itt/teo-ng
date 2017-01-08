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
 *  Module     : key.h
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou
 *  Modifié par: Eric Botcazou 13/02/2001
 *               François Mouret 21/04/2013
 *
 *  scancodes des touches passés par le handler d'Allegro 3.9.38 WIP.
 */


#ifndef TO8KEYS_H
#define TO8KEYS_H 1

/* List of Teo key flags */
#define TEO_KEY_F_NONE          0
#define TEO_KEY_F_SHIFT         (1<<0)
#define TEO_KEY_F_CTRL          (1<<1)
#define TEO_KEY_F_ALTGR         (1<<2)
#define TEO_KEY_F_NUMLOCK       (1<<3)
#define TEO_KEY_F_CAPSLOCK      (1<<4)
#define TEO_KEY_F_MAX           5

/* List of Teo scancodes.
 * comment: <code> <code+shift> <code+altgr> <code+altgr+shift> */
#define TEO_KEY_A           1     /* q Q q Q */
#define TEO_KEY_B           2     /* b B b B */
#define TEO_KEY_C           3     /* c C c C */
#define TEO_KEY_D           4     /* d D d D */
#define TEO_KEY_E           5     /* e E e E */
#define TEO_KEY_F           6     /* f F f F */
#define TEO_KEY_G           7     /* g G g G */
#define TEO_KEY_H           8     /* h H h H */
#define TEO_KEY_I           9     /* i I i I */
#define TEO_KEY_J           10    /* j J j J */
#define TEO_KEY_K           11    /* k K k K */
#define TEO_KEY_L           12    /* l L l L */
#define TEO_KEY_M           13    /* , ? , ? */
#define TEO_KEY_N           14    /* n N n N */
#define TEO_KEY_O           15    /* o O o O */
#define TEO_KEY_P           16    /* p P p P */
#define TEO_KEY_Q           17    /* a A a A */
#define TEO_KEY_R           18    /* r R r R */
#define TEO_KEY_S           19    /* s S s S */
#define TEO_KEY_T           20    /* t T t T */
#define TEO_KEY_U           21    /* u U u U */
#define TEO_KEY_V           22    /* v V v V */
#define TEO_KEY_W           23    /* z Z z Z */
#define TEO_KEY_X           24    /* x X x X */
#define TEO_KEY_Y           25    /* y Y y Y */
#define TEO_KEY_Z           26    /* w W w W */
#define TEO_KEY_0           27    /* à 0 @ # */
#define TEO_KEY_1           28    /* & 1 & 1 */
#define TEO_KEY_2           29    /* é 2 é 2 */
#define TEO_KEY_3           30    /* " 3 # @ */
#define TEO_KEY_4           31    /* ' 4 { [ */
#define TEO_KEY_5           32    /* ( 5 [ { */
#define TEO_KEY_6           33    /* - 6 - 6 */
#define TEO_KEY_7           34    /* è 7 è 7 */
#define TEO_KEY_8           35    /* _ 8 \ ! */
#define TEO_KEY_9           36    /* ç 9 ç 9 */
#define TEO_KEY_0_PAD       37    /* 0 0 0 0 */
#define TEO_KEY_1_PAD       38    /* 1 1 1 1 */
#define TEO_KEY_2_PAD       39    /* 2 2 2 2 */
#define TEO_KEY_3_PAD       40    /* 3 3 3 3 */
#define TEO_KEY_4_PAD       41    /* 4 4 4 4 */
#define TEO_KEY_5_PAD       42    /* 5 5 5 5 */
#define TEO_KEY_6_PAD       43    /* 6 6 6 6 */
#define TEO_KEY_7_PAD       44    /* 7 7 7 7 */
#define TEO_KEY_8_PAD       45    /* 8 8 8 8 */
#define TEO_KEY_9_PAD       46    /* 9 9 9 9 */
#define TEO_KEY_F1          47
#define TEO_KEY_F2          48
#define TEO_KEY_F3          49
#define TEO_KEY_F4          50
#define TEO_KEY_F5          51
#define TEO_KEY_F6          52
#define TEO_KEY_F7          53
#define TEO_KEY_F8          54
#define TEO_KEY_F9          55
#define TEO_KEY_F10         56
#define TEO_KEY_F11         57
#define TEO_KEY_F12         58
#define TEO_KEY_ESC         59
#define TEO_KEY_TILDE       60
#define TEO_KEY_MINUS       61    /* ) ° ] } */
#define TEO_KEY_EQUALS      62    /* = + } ] */
#define TEO_KEY_BACKSPACE   63
#define TEO_KEY_TAB         64
#define TEO_KEY_OPENBRACE   65    /* ^ " ^ " */
#define TEO_KEY_CLOSEBRACE  66    /* $ * $ * */
#define TEO_KEY_ENTER       67
#define TEO_KEY_COLON       68    /* m M m M */
#define TEO_KEY_QUOTE       69    /* ù % ù % */
#define TEO_KEY_BACKSLASH   70    /* * $ * $ */
#define TEO_KEY_BACKSLASH2  71    /* < > < > */
#define TEO_KEY_COMMA       72    /* ; . ; . */
#define TEO_KEY_STOP        73    /* : / : / */
#define TEO_KEY_SLASH       74    /* ! \ ! \ */
#define TEO_KEY_SPACE       75
#define TEO_KEY_INSERT      76
#define TEO_KEY_DEL         77
#define TEO_KEY_HOME        78
#define TEO_KEY_END         79
#define TEO_KEY_PGUP        80
#define TEO_KEY_PGDN        81
#define TEO_KEY_LEFT        82
#define TEO_KEY_RIGHT       83
#define TEO_KEY_UP          84
#define TEO_KEY_DOWN        85
#define TEO_KEY_SLASH_PAD   86    /* / : / : */
#define TEO_KEY_ASTERISK    87    /* * $ * $ */
#define TEO_KEY_MINUS_PAD   88    /* - 6 - 6 */
#define TEO_KEY_PLUS_PAD    89    /* + = + = */
#define TEO_KEY_DEL_PAD     90    /* . ; . ; */
#define TEO_KEY_ENTER_PAD   91
#define TEO_KEY_PRTSCR      92
#define TEO_KEY_PAUSE       93
#define TEO_KEY_ABNT_C1     94
#define TEO_KEY_YEN         95
#define TEO_KEY_KANA        96
#define TEO_KEY_CONVERT     97
#define TEO_KEY_NOCONVERT   98
#define TEO_KEY_AT          99
#define TEO_KEY_CIRCUMFLEX  100
#define TEO_KEY_COLON2      101
#define TEO_KEY_KANJI       102

#define TEO_KEY_MODIFIERS   103

#define TEO_KEY_LSHIFT      103
#define TEO_KEY_RSHIFT      104
#define TEO_KEY_LCONTROL    105
#define TEO_KEY_RCONTROL    106
#define TEO_KEY_ALT         107
#define TEO_KEY_ALTGR       108
#define TEO_KEY_LWIN        109
#define TEO_KEY_RWIN        110
#define TEO_KEY_MENU        111
#define TEO_KEY_SCRLOCK     112
#define TEO_KEY_NUMLOCK     113
#define TEO_KEY_CAPSLOCK    114

#define TEO_KEY_MAX         115

#endif

