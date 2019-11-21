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
 *  Copyright (C) 1997-2019 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret, 
 *                          Samuel Cuella
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
 *  Module     : alleg/akeyboard.c
 *  Version    : 0.1
 *  Créé par   : Samuel Cuella 11/2018
 *  Modifié par: 
 *
 *  Allegro-based keyboard handler: This handler does NOT maps a physical PC key to a Thomson key (i.e where the PC 
 *  F1 key would be mapped to the physical F1 key on the virtual TO8 meaning users would have to press SHIFT+F1 to
 *  get F6 pressed, as on the TO8 (see keyboard.png)). Instead original authors have chosen a symbolic mapping where
 *  a PC key will emit a scancode with a possible modifier. This may, PC F6 will in fact emit TOKEY_F6|TOKEY_SHIFT.
 *  The goal is to make the virtual TO8 write on screen the symbols actually written to the user keyboard.
 *
 *  This handler does the conversion between Allegro KEY_* to TOKEY_. Mapping is done through a config file that is
 *  read at startup.
 *
 *  As the lower-level keyboard_Press functions take a TOKEY nothing stops from writting a handler that "physicaly" 
 *  maps keys from the host to the virtual TO8.
 *
 *  This handler also implements the keyboard-joystick emulation where a key will emit both a scancode and a joystick direction.
 *  Just like the previous implementation, this mode is toggled on/off using the numlock key (active=no jostick emulation). The 
 *  mapping is also read from a config file during startup 
 *
 *  Symbolic mapping from Allegro KEY_*
 *  to TOKEY_. T
 *      Reads symbolic mapping from config files
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
    int tokey; /*TOKEY_ mappng when no modifier(shift,altgr) is set*/
    int shift; /*KEY_* to TOKEY_ mappng when altgr is set*/
    int altgr; /*KEY_* to TOKEY_ mappng when shift is set*/
    int joycode; /*joystick direction to emulate for that key*/
}teo_kmap_t;

static teo_kmap_t keymap[KEY_MAX];


static volatile int jdir_buffer[2][2]; /*joysticks state buffer*/

void akeyboard_Handler(int key)
{
    int tokey;
    int release=key&0x80;

    key&=0x7F; /*What is that ? Why ? 7-bits set to 1*/

//    printf("Got key : %s\n",scancode_to_name(key));
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
//        joystick_verbose_debug_command(keymap[key].joycode);

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
        tokey = keymap[key].shift;
    }
    if(keyboard_hasFlag(TEO_KEY_F_ALTGR) ){
        tokey = keymap[key].altgr;
    }
    if(!tokey)
        tokey = keymap[key].tokey; 
    if(tokey){
        keyboard_Press_ng(tokey, release);
    }
}

int allegro_name_to_scancode(char *alkey)
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

void akeyboard_read_joystick_bindings(char *section, int jdx)
{

    char **bindings;
    int n_bindings;
    char *alkey;
    char *jdir, *jdir2;

    printf("Loading up joystick emulation key mappings\n");
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
    to_int = keyboard_tokey_to_int(tokey);

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

void akeyboard_init(void) 
{
    char **tokey;
    char **tokeys;
    const char *binding;
    char *b2,*modifier;

    memset(keymap,0,KEY_MAX*sizeof(teo_kmap_t));
    for(int i = 0; i < KEY_MAX; i++)
        keymap[i].joycode = -1;


    printf("Loading up key mappings\n");
    tokeys = keyboard_get_tokeys();
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
//    read_joystick_bindings("joyemu1", 1);
//    read_joystick_bindings("joyemu2", 2);

    jdir_buffer[0][0] =  jdir_buffer[0][1] = TEO_JOYSTICK_CENTER; 
    jdir_buffer[1][0] =  jdir_buffer[1][1] = TEO_JOYSTICK_CENTER; 
//    exit(0);
//    const char *get_config_string(const char *section, const char *name, const char *def);

}

