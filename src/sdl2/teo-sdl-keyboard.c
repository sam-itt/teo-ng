#include "sdl2/sdl-keyboard.h"
#include "sdl2/teo-sdl-mouse.h"
#include "sdl2/teo-sdl-joystick.h"

#include "teo.h"
#include "to8keys.h"
#include "media/keyboard.h"
#include "media/joystick.h"
#include "ini.h"


typedef struct{
    int tokey; /*TOKEY_ mappng when no modifier(shift,altgr) is set*/
    int shift; /*SDL_SCANCODE_* to TOKEY_ mappng when shift is set*/
    int altgr; /*SDL_SCANCODE_* to TOKEY_ mappng when altgr is set*/
    int joycode; /*joystick direction to emulate for that key*/
}teo_kmap_t;

static teo_kmap_t keymap[SDL_NUM_SCANCODES];


static volatile int jdir_buffer[2][2]; /*joysticks state buffer*/

static SDL_Scancode teoSDL_KeyboardSDLTextToScancode(char *code);
static char *teoSDL_KeyboardSDLScancodeToText(SDL_Scancode code);

/*TODO: Move me out*/
int teoSDL_EventHandler(void)
{
    SDL_Event event;

    while(SDL_PollEvent(&event) == 1){
        switch(event.type){
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYUP:
                teoSDL_KeyboardHandler(event.key.keysym.scancode, event.key.keysym.sym, 1);
                break;
            case SDL_KEYDOWN:
                teoSDL_KeyboardHandler(event.key.keysym.scancode, event.key.keysym.sym, 0);
                break;
            case SDL_MOUSEMOTION:
                teoSDL_MouseMove(&(event.motion)); 
                break;
            case SDL_MOUSEBUTTONDOWN:
                teoSDL_MouseButton(&(event.button));
                break;
            case SDL_MOUSEBUTTONUP:
                teoSDL_MouseButton(&(event.button));
                break;
            case SDL_JOYAXISMOTION:
                teoSDL_JoystickMove(&(event.jaxis));
                break;
            case SDL_JOYBUTTONDOWN:
                teoSDL_JoystickButton(&(event.jbutton));
                break;
            case SDL_JOYBUTTONUP:
                teoSDL_JoystickButton(&(event.jbutton));
                break;
        }
    }
    return 1;
}


void teoSDL_KeyboardHandler(SDL_Scancode key, SDL_Keycode ksym, Uint8 release)
{
    int tokey;

/*    if(release)
        printf("Got key_release : %s\n",teoSDL_KeyboardSDLScancodeToText(key));
    else
        printf("Got key_pressed : %s\n",teoSDL_KeyboardSDLScancodeToText(key));*/
    /*Special (emulator) keys handling:
     * do the emulator command and return.
     * The virtual TO8 won't see the key
     * */
    if(ksym == SDLK_ESCAPE && !release){
        teo.command=TEO_COMMAND_PANEL;
        return;
    }

    if(ksym == SDLK_F11 && !release){
        teo.command=TEO_COMMAND_SCREENSHOT;
        return;
    }

    if(ksym == SDLK_F12 && !release){
        teo.command=TEO_COMMAND_DEBUGGER;
        return;
    }

    /*Setting the flags on the virtual TO8 keyboard.
     * Actual scancodes are not sent to the virtual TO8
     * except for Capslock      
     **/

    /*TODO: Use the keymap file*/
    if(ksym == SDLK_LCTRL){
        keyboard_ToggleState(TEO_KEY_F_CTRL, release);
        return;
    }

    if(ksym == SDLK_RALT){
        keyboard_ToggleState(TEO_KEY_F_ALTGR, release);
        return;
    }

    if(ksym == SDLK_NUMLOCKCLEAR){
        keyboard_ToggleState(TEO_KEY_F_NUMLOCK, release);
        return;
    }

    if(ksym == SDLK_LSHIFT || ksym == SDLK_RSHIFT){
        keyboard_ToggleState(TEO_KEY_F_SHIFT, release);
        return;
    }

    if(ksym == SDLK_CAPSLOCK && !release){ 
        keyboard_ToggleState(TEO_KEY_F_CAPSLOCK, release);
        /*No return ! The existing code wanted to a) set the kb_state flag
         * b) pass the code to the lower level*/
    }
    
    /*Special mode where joysticks are emulated using the keyboard*/
    if(!keyboard_hasFlag(TEO_KEY_F_NUMLOCK) && keymap[key].joycode != -1){
        int jdx; 
        int jdir;

//        printf("Magic key enabled(NUMLOCK off), interpreting %s(%d) as a joystick action\n",scancode_to_name(key),key);
//        joystick_VerboseDebugCommand(keymap[key].joycode);

        jdx = TEO_JOYN(keymap[key].joycode);
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

static void teoSDL_KeyboardRegisterJoystickBinding(char *sdl_scode, int jdx, char *jdir, char *jdir2)
{

    SDL_Scancode code;
    int jd_int;

    code = teoSDL_KeyboardSDLTextToScancode(sdl_scode);
    jd_int = joystick_SymbolToInt(jdir);
    if(jdir2)
        jd_int |= joystick_SymbolToInt(jdir2);

//    printf("SDL key %s(%d) will produce %s + %s (%d)\n",sdl_scode,code,jdir,jdir2,jd_int);
    jd_int |= ((jdx == 1) ? TEO_JOY1 : TEO_JOY2); 
    keymap[code].joycode = jd_int;
//    printf("keymap[%d].joycode = %d\n",code,keymap[code].joycode);
}


static Uint8 teoSDL_KeyboardReadJoystickBindings(ini_t *key_file, char *section, int jdx)
{

//    char **bindings;
    int n_bindings;
    char *sdl_sym;
    char *jdir, *jdir2;

    printf("Loading up joystick emulation key mappings\n");
    n_bindings = ini_count_keys(key_file, section);
    if(!n_bindings) return -1;

    for(int i = 0;  i < n_bindings; i++){
        sdl_sym = (char *)ini_get_key(key_file, section, i); 
        jdir = (char *)ini_get(key_file, section, sdl_sym);
        if(!jdir) continue;
        
        /* ini_get returns pointer to actual file data
         * we must duplicate it in case we need to alter it
         * by replacing + by \0 as follows 
         * */
        jdir = strdup(jdir); 

//        printf("Key %s will emit %s\n", sdl_sym, jdir);

        jdir2 = strchr(jdir,'+');
        if(jdir2){
            *jdir2 = '\0';
            jdir2++;
        }

        teoSDL_KeyboardRegisterJoystickBinding(sdl_sym, jdx, (char*)jdir, jdir2);
        free(jdir);
    }
    return 1;
}





static void teoSDL_KeyboardRegisterBinding(char *sdl_scode, char *tokey, char *modifier)
{
    SDL_Scancode code;
    int to_int;

    code = teoSDL_KeyboardSDLTextToScancode(sdl_scode);
    to_int = keyboard_TokeyToInt(tokey);

    if(!modifier){
//        printf("SDL key %s(%d) will produce %s(%d)\n",sdl_scode,code,tokey,to_int);
        keymap[code].tokey = to_int;
        return;
    }
    if(strcmp(modifier,"SHIFT") == 0){
//        printf("SDL key %s(%d) + SHIFT will produce %s(%d)\n",sdl_scode,code,tokey,to_int);
        keymap[code].shift = to_int;
        return;
    }
    if(strcmp(modifier,"ALTGR") == 0){
//        printf("SDL key %s(%d) + ALTGR will produce %s(%d)\n",sdl_scode,code,tokey,to_int);
        keymap[code].altgr = to_int;
        return;
    }
}



void teoSDL_KeyboardLoadKeybindings(char *filename)
{
    //Uint8 rv;
    char *binding, *b2;
    char *modifier;

    char **tokey;
    char **tokeys;

    ini_t *key_file;

    key_file = ini_load(filename);
    if(!key_file){
        printf("Error loading key file: %s\n",filename);
        return;
    }

    printf("Loading up key mappings\n");
    tokeys = keyboard_GetTokeys();
    for(tokey = tokeys; *tokey != NULL; tokey++){
//        printf("Resolving mapping for emulator definition %s... ", *tokey);
        binding = (char *)ini_get(key_file, "keymapping", *tokey);
//        printf("got %s\n", binding);
        if(!binding) continue;
        /*ini_get returns static char *, while we need to modify it*/
        binding = strdup(binding); 

        modifier = strchr(binding,'+');
        if(modifier){
            *modifier = '\0';
            modifier++;
        }


        b2 = strchr(binding,',');
        if(b2){
            *b2 = '\0';
            b2++;
            teoSDL_KeyboardRegisterBinding(b2, *tokey, modifier);
        }
        teoSDL_KeyboardRegisterBinding(binding, *tokey, modifier);
        free(binding);
    }
    teoSDL_KeyboardReadJoystickBindings(key_file, "joyemu1", 1);
    teoSDL_KeyboardReadJoystickBindings(key_file, "joyemu2", 2);
    ini_free(key_file);

}

static SDL_Scancode teoSDL_KeyboardSDLTextToScancode(char *code)
{
    if (!strcmp(code,"SDL_SCANCODE_A")) return SDL_SCANCODE_A;
    if (!strcmp(code,"SDL_SCANCODE_B")) return SDL_SCANCODE_B;
    if (!strcmp(code,"SDL_SCANCODE_C")) return SDL_SCANCODE_C;
    if (!strcmp(code,"SDL_SCANCODE_D")) return SDL_SCANCODE_D;
    if (!strcmp(code,"SDL_SCANCODE_E")) return SDL_SCANCODE_E;
    if (!strcmp(code,"SDL_SCANCODE_F")) return SDL_SCANCODE_F;
    if (!strcmp(code,"SDL_SCANCODE_G")) return SDL_SCANCODE_G;
    if (!strcmp(code,"SDL_SCANCODE_H")) return SDL_SCANCODE_H;
    if (!strcmp(code,"SDL_SCANCODE_I")) return SDL_SCANCODE_I;
    if (!strcmp(code,"SDL_SCANCODE_J")) return SDL_SCANCODE_J;
    if (!strcmp(code,"SDL_SCANCODE_K")) return SDL_SCANCODE_K;
    if (!strcmp(code,"SDL_SCANCODE_L")) return SDL_SCANCODE_L;
    if (!strcmp(code,"SDL_SCANCODE_M")) return SDL_SCANCODE_M;
    if (!strcmp(code,"SDL_SCANCODE_N")) return SDL_SCANCODE_N;
    if (!strcmp(code,"SDL_SCANCODE_O")) return SDL_SCANCODE_O;
    if (!strcmp(code,"SDL_SCANCODE_P")) return SDL_SCANCODE_P;
    if (!strcmp(code,"SDL_SCANCODE_Q")) return SDL_SCANCODE_Q;
    if (!strcmp(code,"SDL_SCANCODE_R")) return SDL_SCANCODE_R;
    if (!strcmp(code,"SDL_SCANCODE_S")) return SDL_SCANCODE_S;
    if (!strcmp(code,"SDL_SCANCODE_T")) return SDL_SCANCODE_T;
    if (!strcmp(code,"SDL_SCANCODE_U")) return SDL_SCANCODE_U;
    if (!strcmp(code,"SDL_SCANCODE_V")) return SDL_SCANCODE_V;
    if (!strcmp(code,"SDL_SCANCODE_W")) return SDL_SCANCODE_W;
    if (!strcmp(code,"SDL_SCANCODE_X")) return SDL_SCANCODE_X;
    if (!strcmp(code,"SDL_SCANCODE_Y")) return SDL_SCANCODE_Y;
    if (!strcmp(code,"SDL_SCANCODE_Z")) return SDL_SCANCODE_Z;
    if (!strcmp(code,"SDL_SCANCODE_1")) return SDL_SCANCODE_1;
    if (!strcmp(code,"SDL_SCANCODE_2")) return SDL_SCANCODE_2;
    if (!strcmp(code,"SDL_SCANCODE_3")) return SDL_SCANCODE_3;
    if (!strcmp(code,"SDL_SCANCODE_4")) return SDL_SCANCODE_4;
    if (!strcmp(code,"SDL_SCANCODE_5")) return SDL_SCANCODE_5;
    if (!strcmp(code,"SDL_SCANCODE_6")) return SDL_SCANCODE_6;
    if (!strcmp(code,"SDL_SCANCODE_7")) return SDL_SCANCODE_7;
    if (!strcmp(code,"SDL_SCANCODE_8")) return SDL_SCANCODE_8;
    if (!strcmp(code,"SDL_SCANCODE_9")) return SDL_SCANCODE_9;
    if (!strcmp(code,"SDL_SCANCODE_0")) return SDL_SCANCODE_0;
    if (!strcmp(code,"SDL_SCANCODE_RETURN")) return SDL_SCANCODE_RETURN;
    if (!strcmp(code,"SDL_SCANCODE_ESCAPE")) return SDL_SCANCODE_ESCAPE;
    if (!strcmp(code,"SDL_SCANCODE_BACKSPACE")) return SDL_SCANCODE_BACKSPACE;
    if (!strcmp(code,"SDL_SCANCODE_TAB")) return SDL_SCANCODE_TAB;
    if (!strcmp(code,"SDL_SCANCODE_SPACE")) return SDL_SCANCODE_SPACE;
    if (!strcmp(code,"SDL_SCANCODE_MINUS")) return SDL_SCANCODE_MINUS;
    if (!strcmp(code,"SDL_SCANCODE_EQUALS")) return SDL_SCANCODE_EQUALS;
    if (!strcmp(code,"SDL_SCANCODE_LEFTBRACKET")) return SDL_SCANCODE_LEFTBRACKET;
    if (!strcmp(code,"SDL_SCANCODE_RIGHTBRACKET")) return SDL_SCANCODE_RIGHTBRACKET;
    if (!strcmp(code,"SDL_SCANCODE_BACKSLASH")) return SDL_SCANCODE_BACKSLASH;
    if (!strcmp(code,"SDL_SCANCODE_NONUSHASH")) return SDL_SCANCODE_NONUSHASH;
    if (!strcmp(code,"SDL_SCANCODE_SEMICOLON")) return SDL_SCANCODE_SEMICOLON;
    if (!strcmp(code,"SDL_SCANCODE_APOSTROPHE")) return SDL_SCANCODE_APOSTROPHE;
    if (!strcmp(code,"SDL_SCANCODE_GRAVE")) return SDL_SCANCODE_GRAVE;
    if (!strcmp(code,"SDL_SCANCODE_COMMA")) return SDL_SCANCODE_COMMA;
    if (!strcmp(code,"SDL_SCANCODE_PERIOD")) return SDL_SCANCODE_PERIOD;
    if (!strcmp(code,"SDL_SCANCODE_SLASH")) return SDL_SCANCODE_SLASH;
    if (!strcmp(code,"SDL_SCANCODE_CAPSLOCK")) return SDL_SCANCODE_CAPSLOCK;
    if (!strcmp(code,"SDL_SCANCODE_F1")) return SDL_SCANCODE_F1;
    if (!strcmp(code,"SDL_SCANCODE_F2")) return SDL_SCANCODE_F2;
    if (!strcmp(code,"SDL_SCANCODE_F3")) return SDL_SCANCODE_F3;
    if (!strcmp(code,"SDL_SCANCODE_F4")) return SDL_SCANCODE_F4;
    if (!strcmp(code,"SDL_SCANCODE_F5")) return SDL_SCANCODE_F5;
    if (!strcmp(code,"SDL_SCANCODE_F6")) return SDL_SCANCODE_F6;
    if (!strcmp(code,"SDL_SCANCODE_F7")) return SDL_SCANCODE_F7;
    if (!strcmp(code,"SDL_SCANCODE_F8")) return SDL_SCANCODE_F8;
    if (!strcmp(code,"SDL_SCANCODE_F9")) return SDL_SCANCODE_F9;
    if (!strcmp(code,"SDL_SCANCODE_F10")) return SDL_SCANCODE_F10;
    if (!strcmp(code,"SDL_SCANCODE_F11")) return SDL_SCANCODE_F11;
    if (!strcmp(code,"SDL_SCANCODE_F12")) return SDL_SCANCODE_F12;
    if (!strcmp(code,"SDL_SCANCODE_PRINTSCREEN")) return SDL_SCANCODE_PRINTSCREEN;
    if (!strcmp(code,"SDL_SCANCODE_SCROLLLOCK")) return SDL_SCANCODE_SCROLLLOCK;
    if (!strcmp(code,"SDL_SCANCODE_PAUSE")) return SDL_SCANCODE_PAUSE;
    if (!strcmp(code,"SDL_SCANCODE_INSERT")) return SDL_SCANCODE_INSERT;
    if (!strcmp(code,"SDL_SCANCODE_HOME")) return SDL_SCANCODE_HOME;
    if (!strcmp(code,"SDL_SCANCODE_PAGEUP")) return SDL_SCANCODE_PAGEUP;
    if (!strcmp(code,"SDL_SCANCODE_DELETE")) return SDL_SCANCODE_DELETE;
    if (!strcmp(code,"SDL_SCANCODE_END")) return SDL_SCANCODE_END;
    if (!strcmp(code,"SDL_SCANCODE_PAGEDOWN")) return SDL_SCANCODE_PAGEDOWN;
    if (!strcmp(code,"SDL_SCANCODE_RIGHT")) return SDL_SCANCODE_RIGHT;
    if (!strcmp(code,"SDL_SCANCODE_LEFT")) return SDL_SCANCODE_LEFT;
    if (!strcmp(code,"SDL_SCANCODE_DOWN")) return SDL_SCANCODE_DOWN;
    if (!strcmp(code,"SDL_SCANCODE_UP")) return SDL_SCANCODE_UP;
    if (!strcmp(code,"SDL_SCANCODE_NUMLOCKCLEAR")) return SDL_SCANCODE_NUMLOCKCLEAR;
    if (!strcmp(code,"SDL_SCANCODE_KP_DIVIDE")) return SDL_SCANCODE_KP_DIVIDE;
    if (!strcmp(code,"SDL_SCANCODE_KP_MULTIPLY")) return SDL_SCANCODE_KP_MULTIPLY;
    if (!strcmp(code,"SDL_SCANCODE_KP_MINUS")) return SDL_SCANCODE_KP_MINUS;
    if (!strcmp(code,"SDL_SCANCODE_KP_PLUS")) return SDL_SCANCODE_KP_PLUS;
    if (!strcmp(code,"SDL_SCANCODE_KP_ENTER")) return SDL_SCANCODE_KP_ENTER;
    if (!strcmp(code,"SDL_SCANCODE_KP_1")) return SDL_SCANCODE_KP_1;
    if (!strcmp(code,"SDL_SCANCODE_KP_2")) return SDL_SCANCODE_KP_2;
    if (!strcmp(code,"SDL_SCANCODE_KP_3")) return SDL_SCANCODE_KP_3;
    if (!strcmp(code,"SDL_SCANCODE_KP_4")) return SDL_SCANCODE_KP_4;
    if (!strcmp(code,"SDL_SCANCODE_KP_5")) return SDL_SCANCODE_KP_5;
    if (!strcmp(code,"SDL_SCANCODE_KP_6")) return SDL_SCANCODE_KP_6;
    if (!strcmp(code,"SDL_SCANCODE_KP_7")) return SDL_SCANCODE_KP_7;
    if (!strcmp(code,"SDL_SCANCODE_KP_8")) return SDL_SCANCODE_KP_8;
    if (!strcmp(code,"SDL_SCANCODE_KP_9")) return SDL_SCANCODE_KP_9;
    if (!strcmp(code,"SDL_SCANCODE_KP_0")) return SDL_SCANCODE_KP_0;
    if (!strcmp(code,"SDL_SCANCODE_KP_PERIOD")) return SDL_SCANCODE_KP_PERIOD;
    if (!strcmp(code,"SDL_SCANCODE_NONUSBACKSLASH")) return SDL_SCANCODE_NONUSBACKSLASH;
    if (!strcmp(code,"SDL_SCANCODE_APPLICATION")) return SDL_SCANCODE_APPLICATION;
    if (!strcmp(code,"SDL_SCANCODE_POWER")) return SDL_SCANCODE_POWER;
    if (!strcmp(code,"SDL_SCANCODE_KP_EQUALS")) return SDL_SCANCODE_KP_EQUALS;
    if (!strcmp(code,"SDL_SCANCODE_F13")) return SDL_SCANCODE_F13;
    if (!strcmp(code,"SDL_SCANCODE_F14")) return SDL_SCANCODE_F14;
    if (!strcmp(code,"SDL_SCANCODE_F15")) return SDL_SCANCODE_F15;
    if (!strcmp(code,"SDL_SCANCODE_F16")) return SDL_SCANCODE_F16;
    if (!strcmp(code,"SDL_SCANCODE_F17")) return SDL_SCANCODE_F17;
    if (!strcmp(code,"SDL_SCANCODE_F18")) return SDL_SCANCODE_F18;
    if (!strcmp(code,"SDL_SCANCODE_F19")) return SDL_SCANCODE_F19;
    if (!strcmp(code,"SDL_SCANCODE_F20")) return SDL_SCANCODE_F20;
    if (!strcmp(code,"SDL_SCANCODE_F21")) return SDL_SCANCODE_F21;
    if (!strcmp(code,"SDL_SCANCODE_F22")) return SDL_SCANCODE_F22;
    if (!strcmp(code,"SDL_SCANCODE_F23")) return SDL_SCANCODE_F23;
    if (!strcmp(code,"SDL_SCANCODE_F24")) return SDL_SCANCODE_F24;
    if (!strcmp(code,"SDL_SCANCODE_EXECUTE")) return SDL_SCANCODE_EXECUTE;
    if (!strcmp(code,"SDL_SCANCODE_HELP")) return SDL_SCANCODE_HELP;
    if (!strcmp(code,"SDL_SCANCODE_MENU")) return SDL_SCANCODE_MENU;
    if (!strcmp(code,"SDL_SCANCODE_SELECT")) return SDL_SCANCODE_SELECT;
    if (!strcmp(code,"SDL_SCANCODE_STOP")) return SDL_SCANCODE_STOP;
    if (!strcmp(code,"SDL_SCANCODE_AGAIN")) return SDL_SCANCODE_AGAIN;
    if (!strcmp(code,"SDL_SCANCODE_UNDO")) return SDL_SCANCODE_UNDO;
    if (!strcmp(code,"SDL_SCANCODE_CUT")) return SDL_SCANCODE_CUT;
    if (!strcmp(code,"SDL_SCANCODE_COPY")) return SDL_SCANCODE_COPY;
    if (!strcmp(code,"SDL_SCANCODE_PASTE")) return SDL_SCANCODE_PASTE;
    if (!strcmp(code,"SDL_SCANCODE_FIND")) return SDL_SCANCODE_FIND;
    if (!strcmp(code,"SDL_SCANCODE_MUTE")) return SDL_SCANCODE_MUTE;
    if (!strcmp(code,"SDL_SCANCODE_VOLUMEUP")) return SDL_SCANCODE_VOLUMEUP;
    if (!strcmp(code,"SDL_SCANCODE_VOLUMEDOWN")) return SDL_SCANCODE_VOLUMEDOWN;
    if (!strcmp(code,"SDL_SCANCODE_KP_COMMA")) return SDL_SCANCODE_KP_COMMA;
    if (!strcmp(code,"SDL_SCANCODE_KP_EQUALSAS400")) return SDL_SCANCODE_KP_EQUALSAS400;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL1")) return SDL_SCANCODE_INTERNATIONAL1;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL2")) return SDL_SCANCODE_INTERNATIONAL2;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL3")) return SDL_SCANCODE_INTERNATIONAL3;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL4")) return SDL_SCANCODE_INTERNATIONAL4;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL5")) return SDL_SCANCODE_INTERNATIONAL5;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL6")) return SDL_SCANCODE_INTERNATIONAL6;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL7")) return SDL_SCANCODE_INTERNATIONAL7;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL8")) return SDL_SCANCODE_INTERNATIONAL8;
    if (!strcmp(code,"SDL_SCANCODE_INTERNATIONAL9")) return SDL_SCANCODE_INTERNATIONAL9;
    if (!strcmp(code,"SDL_SCANCODE_LANG1")) return SDL_SCANCODE_LANG1;
    if (!strcmp(code,"SDL_SCANCODE_LANG2")) return SDL_SCANCODE_LANG2;
    if (!strcmp(code,"SDL_SCANCODE_LANG3")) return SDL_SCANCODE_LANG3;
    if (!strcmp(code,"SDL_SCANCODE_LANG4")) return SDL_SCANCODE_LANG4;
    if (!strcmp(code,"SDL_SCANCODE_LANG5")) return SDL_SCANCODE_LANG5;
    if (!strcmp(code,"SDL_SCANCODE_LANG6")) return SDL_SCANCODE_LANG6;
    if (!strcmp(code,"SDL_SCANCODE_LANG7")) return SDL_SCANCODE_LANG7;
    if (!strcmp(code,"SDL_SCANCODE_LANG8")) return SDL_SCANCODE_LANG8;
    if (!strcmp(code,"SDL_SCANCODE_LANG9")) return SDL_SCANCODE_LANG9;
    if (!strcmp(code,"SDL_SCANCODE_ALTERASE")) return SDL_SCANCODE_ALTERASE;
    if (!strcmp(code,"SDL_SCANCODE_SYSREQ")) return SDL_SCANCODE_SYSREQ;
    if (!strcmp(code,"SDL_SCANCODE_CANCEL")) return SDL_SCANCODE_CANCEL;
    if (!strcmp(code,"SDL_SCANCODE_CLEAR")) return SDL_SCANCODE_CLEAR;
    if (!strcmp(code,"SDL_SCANCODE_PRIOR")) return SDL_SCANCODE_PRIOR;
    if (!strcmp(code,"SDL_SCANCODE_RETURN2")) return SDL_SCANCODE_RETURN2;
    if (!strcmp(code,"SDL_SCANCODE_SEPARATOR")) return SDL_SCANCODE_SEPARATOR;
    if (!strcmp(code,"SDL_SCANCODE_OUT")) return SDL_SCANCODE_OUT;
    if (!strcmp(code,"SDL_SCANCODE_OPER")) return SDL_SCANCODE_OPER;
    if (!strcmp(code,"SDL_SCANCODE_CLEARAGAIN")) return SDL_SCANCODE_CLEARAGAIN;
    if (!strcmp(code,"SDL_SCANCODE_CRSEL")) return SDL_SCANCODE_CRSEL;
    if (!strcmp(code,"SDL_SCANCODE_EXSEL")) return SDL_SCANCODE_EXSEL;
    if (!strcmp(code,"SDL_SCANCODE_KP_00")) return SDL_SCANCODE_KP_00;
    if (!strcmp(code,"SDL_SCANCODE_KP_000")) return SDL_SCANCODE_KP_000;
    if (!strcmp(code,"SDL_SCANCODE_THOUSANDSSEPARATOR")) return SDL_SCANCODE_THOUSANDSSEPARATOR;
    if (!strcmp(code,"SDL_SCANCODE_DECIMALSEPARATOR")) return SDL_SCANCODE_DECIMALSEPARATOR;
    if (!strcmp(code,"SDL_SCANCODE_CURRENCYUNIT")) return SDL_SCANCODE_CURRENCYUNIT;
    if (!strcmp(code,"SDL_SCANCODE_CURRENCYSUBUNIT")) return SDL_SCANCODE_CURRENCYSUBUNIT;
    if (!strcmp(code,"SDL_SCANCODE_KP_LEFTPAREN")) return SDL_SCANCODE_KP_LEFTPAREN;
    if (!strcmp(code,"SDL_SCANCODE_KP_RIGHTPAREN")) return SDL_SCANCODE_KP_RIGHTPAREN;
    if (!strcmp(code,"SDL_SCANCODE_KP_LEFTBRACE")) return SDL_SCANCODE_KP_LEFTBRACE;
    if (!strcmp(code,"SDL_SCANCODE_KP_RIGHTBRACE")) return SDL_SCANCODE_KP_RIGHTBRACE;
    if (!strcmp(code,"SDL_SCANCODE_KP_TAB")) return SDL_SCANCODE_KP_TAB;
    if (!strcmp(code,"SDL_SCANCODE_KP_BACKSPACE")) return SDL_SCANCODE_KP_BACKSPACE;
    if (!strcmp(code,"SDL_SCANCODE_KP_A")) return SDL_SCANCODE_KP_A;
    if (!strcmp(code,"SDL_SCANCODE_KP_B")) return SDL_SCANCODE_KP_B;
    if (!strcmp(code,"SDL_SCANCODE_KP_C")) return SDL_SCANCODE_KP_C;
    if (!strcmp(code,"SDL_SCANCODE_KP_D")) return SDL_SCANCODE_KP_D;
    if (!strcmp(code,"SDL_SCANCODE_KP_E")) return SDL_SCANCODE_KP_E;
    if (!strcmp(code,"SDL_SCANCODE_KP_F")) return SDL_SCANCODE_KP_F;
    if (!strcmp(code,"SDL_SCANCODE_KP_XOR")) return SDL_SCANCODE_KP_XOR;
    if (!strcmp(code,"SDL_SCANCODE_KP_POWER")) return SDL_SCANCODE_KP_POWER;
    if (!strcmp(code,"SDL_SCANCODE_KP_PERCENT")) return SDL_SCANCODE_KP_PERCENT;
    if (!strcmp(code,"SDL_SCANCODE_KP_LESS")) return SDL_SCANCODE_KP_LESS;
    if (!strcmp(code,"SDL_SCANCODE_KP_GREATER")) return SDL_SCANCODE_KP_GREATER;
    if (!strcmp(code,"SDL_SCANCODE_KP_AMPERSAND")) return SDL_SCANCODE_KP_AMPERSAND;
    if (!strcmp(code,"SDL_SCANCODE_KP_DBLAMPERSAND")) return SDL_SCANCODE_KP_DBLAMPERSAND;
    if (!strcmp(code,"SDL_SCANCODE_KP_VERTICALBAR")) return SDL_SCANCODE_KP_VERTICALBAR;
    if (!strcmp(code,"SDL_SCANCODE_KP_DBLVERTICALBAR")) return SDL_SCANCODE_KP_DBLVERTICALBAR;
    if (!strcmp(code,"SDL_SCANCODE_KP_COLON")) return SDL_SCANCODE_KP_COLON;
    if (!strcmp(code,"SDL_SCANCODE_KP_HASH")) return SDL_SCANCODE_KP_HASH;
    if (!strcmp(code,"SDL_SCANCODE_KP_SPACE")) return SDL_SCANCODE_KP_SPACE;
    if (!strcmp(code,"SDL_SCANCODE_KP_AT")) return SDL_SCANCODE_KP_AT;
    if (!strcmp(code,"SDL_SCANCODE_KP_EXCLAM")) return SDL_SCANCODE_KP_EXCLAM;
    if (!strcmp(code,"SDL_SCANCODE_KP_MEMSTORE")) return SDL_SCANCODE_KP_MEMSTORE;
    if (!strcmp(code,"SDL_SCANCODE_KP_MEMRECALL")) return SDL_SCANCODE_KP_MEMRECALL;
    if (!strcmp(code,"SDL_SCANCODE_KP_MEMCLEAR")) return SDL_SCANCODE_KP_MEMCLEAR;
    if (!strcmp(code,"SDL_SCANCODE_KP_MEMADD")) return SDL_SCANCODE_KP_MEMADD;
    if (!strcmp(code,"SDL_SCANCODE_KP_MEMSUBTRACT")) return SDL_SCANCODE_KP_MEMSUBTRACT;
    if (!strcmp(code,"SDL_SCANCODE_KP_MEMMULTIPLY")) return SDL_SCANCODE_KP_MEMMULTIPLY;
    if (!strcmp(code,"SDL_SCANCODE_KP_MEMDIVIDE")) return SDL_SCANCODE_KP_MEMDIVIDE;
    if (!strcmp(code,"SDL_SCANCODE_KP_PLUSMINUS")) return SDL_SCANCODE_KP_PLUSMINUS;
    if (!strcmp(code,"SDL_SCANCODE_KP_CLEAR")) return SDL_SCANCODE_KP_CLEAR;
    if (!strcmp(code,"SDL_SCANCODE_KP_CLEARENTRY")) return SDL_SCANCODE_KP_CLEARENTRY;
    if (!strcmp(code,"SDL_SCANCODE_KP_BINARY")) return SDL_SCANCODE_KP_BINARY;
    if (!strcmp(code,"SDL_SCANCODE_KP_OCTAL")) return SDL_SCANCODE_KP_OCTAL;
    if (!strcmp(code,"SDL_SCANCODE_KP_DECIMAL")) return SDL_SCANCODE_KP_DECIMAL;
    if (!strcmp(code,"SDL_SCANCODE_KP_HEXADECIMAL")) return SDL_SCANCODE_KP_HEXADECIMAL;
    if (!strcmp(code,"SDL_SCANCODE_LCTRL")) return SDL_SCANCODE_LCTRL;
    if (!strcmp(code,"SDL_SCANCODE_LSHIFT")) return SDL_SCANCODE_LSHIFT;
    if (!strcmp(code,"SDL_SCANCODE_LALT")) return SDL_SCANCODE_LALT;
    if (!strcmp(code,"SDL_SCANCODE_LGUI")) return SDL_SCANCODE_LGUI;
    if (!strcmp(code,"SDL_SCANCODE_RCTRL")) return SDL_SCANCODE_RCTRL;
    if (!strcmp(code,"SDL_SCANCODE_RSHIFT")) return SDL_SCANCODE_RSHIFT;
    if (!strcmp(code,"SDL_SCANCODE_RALT")) return SDL_SCANCODE_RALT;
    if (!strcmp(code,"SDL_SCANCODE_RGUI")) return SDL_SCANCODE_RGUI;
    if (!strcmp(code,"SDL_SCANCODE_MODE")) return SDL_SCANCODE_MODE;
    if (!strcmp(code,"SDL_SCANCODE_AUDIONEXT")) return SDL_SCANCODE_AUDIONEXT;
    if (!strcmp(code,"SDL_SCANCODE_AUDIOPREV")) return SDL_SCANCODE_AUDIOPREV;
    if (!strcmp(code,"SDL_SCANCODE_AUDIOSTOP")) return SDL_SCANCODE_AUDIOSTOP;
    if (!strcmp(code,"SDL_SCANCODE_AUDIOPLAY")) return SDL_SCANCODE_AUDIOPLAY;
    if (!strcmp(code,"SDL_SCANCODE_AUDIOMUTE")) return SDL_SCANCODE_AUDIOMUTE;
    if (!strcmp(code,"SDL_SCANCODE_MEDIASELECT")) return SDL_SCANCODE_MEDIASELECT;
    if (!strcmp(code,"SDL_SCANCODE_WWW")) return SDL_SCANCODE_WWW;
    if (!strcmp(code,"SDL_SCANCODE_MAIL")) return SDL_SCANCODE_MAIL;
    if (!strcmp(code,"SDL_SCANCODE_CALCULATOR")) return SDL_SCANCODE_CALCULATOR;
    if (!strcmp(code,"SDL_SCANCODE_COMPUTER")) return SDL_SCANCODE_COMPUTER;
    if (!strcmp(code,"SDL_SCANCODE_AC_SEARCH")) return SDL_SCANCODE_AC_SEARCH;
    if (!strcmp(code,"SDL_SCANCODE_AC_HOME")) return SDL_SCANCODE_AC_HOME;
    if (!strcmp(code,"SDL_SCANCODE_AC_BACK")) return SDL_SCANCODE_AC_BACK;
    if (!strcmp(code,"SDL_SCANCODE_AC_FORWARD")) return SDL_SCANCODE_AC_FORWARD;
    if (!strcmp(code,"SDL_SCANCODE_AC_STOP")) return SDL_SCANCODE_AC_STOP;
    if (!strcmp(code,"SDL_SCANCODE_AC_REFRESH")) return SDL_SCANCODE_AC_REFRESH;
    if (!strcmp(code,"SDL_SCANCODE_AC_BOOKMARKS")) return SDL_SCANCODE_AC_BOOKMARKS;
    if (!strcmp(code,"SDL_SCANCODE_BRIGHTNESSDOWN")) return SDL_SCANCODE_BRIGHTNESSDOWN;
    if (!strcmp(code,"SDL_SCANCODE_BRIGHTNESSUP")) return SDL_SCANCODE_BRIGHTNESSUP;
    if (!strcmp(code,"SDL_SCANCODE_DISPLAYSWITCH")) return SDL_SCANCODE_DISPLAYSWITCH;
    if (!strcmp(code,"SDL_SCANCODE_KBDILLUMTOGGLE")) return SDL_SCANCODE_KBDILLUMTOGGLE;
    if (!strcmp(code,"SDL_SCANCODE_KBDILLUMDOWN")) return SDL_SCANCODE_KBDILLUMDOWN;
    if (!strcmp(code,"SDL_SCANCODE_KBDILLUMUP")) return SDL_SCANCODE_KBDILLUMUP;
    if (!strcmp(code,"SDL_SCANCODE_EJECT")) return SDL_SCANCODE_EJECT;
    if (!strcmp(code,"SDL_SCANCODE_SLEEP")) return SDL_SCANCODE_SLEEP;
    if (!strcmp(code,"SDL_SCANCODE_APP1")) return SDL_SCANCODE_APP1;
    if (!strcmp(code,"SDL_SCANCODE_APP2")) return SDL_SCANCODE_APP2;
    if (!strcmp(code,"SDL_SCANCODE_AUDIOREWIND")) return SDL_SCANCODE_AUDIOREWIND;
    if (!strcmp(code,"SDL_SCANCODE_AUDIOFASTFORWARD")) return SDL_SCANCODE_AUDIOFASTFORWARD;

    return SDL_SCANCODE_UNKNOWN;
}

static char *teoSDL_KeyboardSDLScancodeToText(SDL_Scancode code)
{
    if (code == SDL_SCANCODE_A) return "SDL_SCANCODE_A";
    if (code == SDL_SCANCODE_B) return "SDL_SCANCODE_B";
    if (code == SDL_SCANCODE_C) return "SDL_SCANCODE_C";
    if (code == SDL_SCANCODE_D) return "SDL_SCANCODE_D";
    if (code == SDL_SCANCODE_E) return "SDL_SCANCODE_E";
    if (code == SDL_SCANCODE_F) return "SDL_SCANCODE_F";
    if (code == SDL_SCANCODE_G) return "SDL_SCANCODE_G";
    if (code == SDL_SCANCODE_H) return "SDL_SCANCODE_H";
    if (code == SDL_SCANCODE_I) return "SDL_SCANCODE_I";
    if (code == SDL_SCANCODE_J) return "SDL_SCANCODE_J";
    if (code == SDL_SCANCODE_K) return "SDL_SCANCODE_K";
    if (code == SDL_SCANCODE_L) return "SDL_SCANCODE_L";
    if (code == SDL_SCANCODE_M) return "SDL_SCANCODE_M";
    if (code == SDL_SCANCODE_N) return "SDL_SCANCODE_N";
    if (code == SDL_SCANCODE_O) return "SDL_SCANCODE_O";
    if (code == SDL_SCANCODE_P) return "SDL_SCANCODE_P";
    if (code == SDL_SCANCODE_Q) return "SDL_SCANCODE_Q";
    if (code == SDL_SCANCODE_R) return "SDL_SCANCODE_R";
    if (code == SDL_SCANCODE_S) return "SDL_SCANCODE_S";
    if (code == SDL_SCANCODE_T) return "SDL_SCANCODE_T";
    if (code == SDL_SCANCODE_U) return "SDL_SCANCODE_U";
    if (code == SDL_SCANCODE_V) return "SDL_SCANCODE_V";
    if (code == SDL_SCANCODE_W) return "SDL_SCANCODE_W";
    if (code == SDL_SCANCODE_X) return "SDL_SCANCODE_X";
    if (code == SDL_SCANCODE_Y) return "SDL_SCANCODE_Y";
    if (code == SDL_SCANCODE_Z) return "SDL_SCANCODE_Z";
    if (code == SDL_SCANCODE_1) return "SDL_SCANCODE_1";
    if (code == SDL_SCANCODE_2) return "SDL_SCANCODE_2";
    if (code == SDL_SCANCODE_3) return "SDL_SCANCODE_3";
    if (code == SDL_SCANCODE_4) return "SDL_SCANCODE_4";
    if (code == SDL_SCANCODE_5) return "SDL_SCANCODE_5";
    if (code == SDL_SCANCODE_6) return "SDL_SCANCODE_6";
    if (code == SDL_SCANCODE_7) return "SDL_SCANCODE_7";
    if (code == SDL_SCANCODE_8) return "SDL_SCANCODE_8";
    if (code == SDL_SCANCODE_9) return "SDL_SCANCODE_9";
    if (code == SDL_SCANCODE_0) return "SDL_SCANCODE_0";
    if (code == SDL_SCANCODE_RETURN) return "SDL_SCANCODE_RETURN";
    if (code == SDL_SCANCODE_ESCAPE) return "SDL_SCANCODE_ESCAPE";
    if (code == SDL_SCANCODE_BACKSPACE) return "SDL_SCANCODE_BACKSPACE";
    if (code == SDL_SCANCODE_TAB) return "SDL_SCANCODE_TAB";
    if (code == SDL_SCANCODE_SPACE) return "SDL_SCANCODE_SPACE";
    if (code == SDL_SCANCODE_MINUS) return "SDL_SCANCODE_MINUS";
    if (code == SDL_SCANCODE_EQUALS) return "SDL_SCANCODE_EQUALS";
    if (code == SDL_SCANCODE_LEFTBRACKET) return "SDL_SCANCODE_LEFTBRACKET";
    if (code == SDL_SCANCODE_RIGHTBRACKET) return "SDL_SCANCODE_RIGHTBRACKET";
    if (code == SDL_SCANCODE_BACKSLASH) return "SDL_SCANCODE_BACKSLASH";
    if (code == SDL_SCANCODE_NONUSHASH) return "SDL_SCANCODE_NONUSHASH";
    if (code == SDL_SCANCODE_SEMICOLON) return "SDL_SCANCODE_SEMICOLON";
    if (code == SDL_SCANCODE_APOSTROPHE) return "SDL_SCANCODE_APOSTROPHE";
    if (code == SDL_SCANCODE_GRAVE) return "SDL_SCANCODE_GRAVE";
    if (code == SDL_SCANCODE_COMMA) return "SDL_SCANCODE_COMMA";
    if (code == SDL_SCANCODE_PERIOD) return "SDL_SCANCODE_PERIOD";
    if (code == SDL_SCANCODE_SLASH) return "SDL_SCANCODE_SLASH";
    if (code == SDL_SCANCODE_CAPSLOCK) return "SDL_SCANCODE_CAPSLOCK";
    if (code == SDL_SCANCODE_F1) return "SDL_SCANCODE_F1";
    if (code == SDL_SCANCODE_F2) return "SDL_SCANCODE_F2";
    if (code == SDL_SCANCODE_F3) return "SDL_SCANCODE_F3";
    if (code == SDL_SCANCODE_F4) return "SDL_SCANCODE_F4";
    if (code == SDL_SCANCODE_F5) return "SDL_SCANCODE_F5";
    if (code == SDL_SCANCODE_F6) return "SDL_SCANCODE_F6";
    if (code == SDL_SCANCODE_F7) return "SDL_SCANCODE_F7";
    if (code == SDL_SCANCODE_F8) return "SDL_SCANCODE_F8";
    if (code == SDL_SCANCODE_F9) return "SDL_SCANCODE_F9";
    if (code == SDL_SCANCODE_F10) return "SDL_SCANCODE_F10";
    if (code == SDL_SCANCODE_F11) return "SDL_SCANCODE_F11";
    if (code == SDL_SCANCODE_F12) return "SDL_SCANCODE_F12";
    if (code == SDL_SCANCODE_PRINTSCREEN) return "SDL_SCANCODE_PRINTSCREEN";
    if (code == SDL_SCANCODE_SCROLLLOCK) return "SDL_SCANCODE_SCROLLLOCK";
    if (code == SDL_SCANCODE_PAUSE) return "SDL_SCANCODE_PAUSE";
    if (code == SDL_SCANCODE_INSERT) return "SDL_SCANCODE_INSERT";
    if (code == SDL_SCANCODE_HOME) return "SDL_SCANCODE_HOME";
    if (code == SDL_SCANCODE_PAGEUP) return "SDL_SCANCODE_PAGEUP";
    if (code == SDL_SCANCODE_DELETE) return "SDL_SCANCODE_DELETE";
    if (code == SDL_SCANCODE_END) return "SDL_SCANCODE_END";
    if (code == SDL_SCANCODE_PAGEDOWN) return "SDL_SCANCODE_PAGEDOWN";
    if (code == SDL_SCANCODE_RIGHT) return "SDL_SCANCODE_RIGHT";
    if (code == SDL_SCANCODE_LEFT) return "SDL_SCANCODE_LEFT";
    if (code == SDL_SCANCODE_DOWN) return "SDL_SCANCODE_DOWN";
    if (code == SDL_SCANCODE_UP) return "SDL_SCANCODE_UP";
    if (code == SDL_SCANCODE_NUMLOCKCLEAR) return "SDL_SCANCODE_NUMLOCKCLEAR";
    if (code == SDL_SCANCODE_KP_DIVIDE) return "SDL_SCANCODE_KP_DIVIDE";
    if (code == SDL_SCANCODE_KP_MULTIPLY) return "SDL_SCANCODE_KP_MULTIPLY";
    if (code == SDL_SCANCODE_KP_MINUS) return "SDL_SCANCODE_KP_MINUS";
    if (code == SDL_SCANCODE_KP_PLUS) return "SDL_SCANCODE_KP_PLUS";
    if (code == SDL_SCANCODE_KP_ENTER) return "SDL_SCANCODE_KP_ENTER";
    if (code == SDL_SCANCODE_KP_1) return "SDL_SCANCODE_KP_1";
    if (code == SDL_SCANCODE_KP_2) return "SDL_SCANCODE_KP_2";
    if (code == SDL_SCANCODE_KP_3) return "SDL_SCANCODE_KP_3";
    if (code == SDL_SCANCODE_KP_4) return "SDL_SCANCODE_KP_4";
    if (code == SDL_SCANCODE_KP_5) return "SDL_SCANCODE_KP_5";
    if (code == SDL_SCANCODE_KP_6) return "SDL_SCANCODE_KP_6";
    if (code == SDL_SCANCODE_KP_7) return "SDL_SCANCODE_KP_7";
    if (code == SDL_SCANCODE_KP_8) return "SDL_SCANCODE_KP_8";
    if (code == SDL_SCANCODE_KP_9) return "SDL_SCANCODE_KP_9";
    if (code == SDL_SCANCODE_KP_0) return "SDL_SCANCODE_KP_0";
    if (code == SDL_SCANCODE_KP_PERIOD) return "SDL_SCANCODE_KP_PERIOD";
    if (code == SDL_SCANCODE_NONUSBACKSLASH) return "SDL_SCANCODE_NONUSBACKSLASH";
    if (code == SDL_SCANCODE_APPLICATION) return "SDL_SCANCODE_APPLICATION";
    if (code == SDL_SCANCODE_POWER) return "SDL_SCANCODE_POWER";
    if (code == SDL_SCANCODE_KP_EQUALS) return "SDL_SCANCODE_KP_EQUALS";
    if (code == SDL_SCANCODE_F13) return "SDL_SCANCODE_F13";
    if (code == SDL_SCANCODE_F14) return "SDL_SCANCODE_F14";
    if (code == SDL_SCANCODE_F15) return "SDL_SCANCODE_F15";
    if (code == SDL_SCANCODE_F16) return "SDL_SCANCODE_F16";
    if (code == SDL_SCANCODE_F17) return "SDL_SCANCODE_F17";
    if (code == SDL_SCANCODE_F18) return "SDL_SCANCODE_F18";
    if (code == SDL_SCANCODE_F19) return "SDL_SCANCODE_F19";
    if (code == SDL_SCANCODE_F20) return "SDL_SCANCODE_F20";
    if (code == SDL_SCANCODE_F21) return "SDL_SCANCODE_F21";
    if (code == SDL_SCANCODE_F22) return "SDL_SCANCODE_F22";
    if (code == SDL_SCANCODE_F23) return "SDL_SCANCODE_F23";
    if (code == SDL_SCANCODE_F24) return "SDL_SCANCODE_F24";
    if (code == SDL_SCANCODE_EXECUTE) return "SDL_SCANCODE_EXECUTE";
    if (code == SDL_SCANCODE_HELP) return "SDL_SCANCODE_HELP";
    if (code == SDL_SCANCODE_MENU) return "SDL_SCANCODE_MENU";
    if (code == SDL_SCANCODE_SELECT) return "SDL_SCANCODE_SELECT";
    if (code == SDL_SCANCODE_STOP) return "SDL_SCANCODE_STOP";
    if (code == SDL_SCANCODE_AGAIN) return "SDL_SCANCODE_AGAIN";
    if (code == SDL_SCANCODE_UNDO) return "SDL_SCANCODE_UNDO";
    if (code == SDL_SCANCODE_CUT) return "SDL_SCANCODE_CUT";
    if (code == SDL_SCANCODE_COPY) return "SDL_SCANCODE_COPY";
    if (code == SDL_SCANCODE_PASTE) return "SDL_SCANCODE_PASTE";
    if (code == SDL_SCANCODE_FIND) return "SDL_SCANCODE_FIND";
    if (code == SDL_SCANCODE_MUTE) return "SDL_SCANCODE_MUTE";
    if (code == SDL_SCANCODE_VOLUMEUP) return "SDL_SCANCODE_VOLUMEUP";
    if (code == SDL_SCANCODE_VOLUMEDOWN) return "SDL_SCANCODE_VOLUMEDOWN";
    if (code == SDL_SCANCODE_KP_COMMA) return "SDL_SCANCODE_KP_COMMA";
    if (code == SDL_SCANCODE_KP_EQUALSAS400) return "SDL_SCANCODE_KP_EQUALSAS400";
    if (code == SDL_SCANCODE_INTERNATIONAL1) return "SDL_SCANCODE_INTERNATIONAL1";
    if (code == SDL_SCANCODE_INTERNATIONAL2) return "SDL_SCANCODE_INTERNATIONAL2";
    if (code == SDL_SCANCODE_INTERNATIONAL3) return "SDL_SCANCODE_INTERNATIONAL3";
    if (code == SDL_SCANCODE_INTERNATIONAL4) return "SDL_SCANCODE_INTERNATIONAL4";
    if (code == SDL_SCANCODE_INTERNATIONAL5) return "SDL_SCANCODE_INTERNATIONAL5";
    if (code == SDL_SCANCODE_INTERNATIONAL6) return "SDL_SCANCODE_INTERNATIONAL6";
    if (code == SDL_SCANCODE_INTERNATIONAL7) return "SDL_SCANCODE_INTERNATIONAL7";
    if (code == SDL_SCANCODE_INTERNATIONAL8) return "SDL_SCANCODE_INTERNATIONAL8";
    if (code == SDL_SCANCODE_INTERNATIONAL9) return "SDL_SCANCODE_INTERNATIONAL9";
    if (code == SDL_SCANCODE_LANG1) return "SDL_SCANCODE_LANG1";
    if (code == SDL_SCANCODE_LANG2) return "SDL_SCANCODE_LANG2";
    if (code == SDL_SCANCODE_LANG3) return "SDL_SCANCODE_LANG3";
    if (code == SDL_SCANCODE_LANG4) return "SDL_SCANCODE_LANG4";
    if (code == SDL_SCANCODE_LANG5) return "SDL_SCANCODE_LANG5";
    if (code == SDL_SCANCODE_LANG6) return "SDL_SCANCODE_LANG6";
    if (code == SDL_SCANCODE_LANG7) return "SDL_SCANCODE_LANG7";
    if (code == SDL_SCANCODE_LANG8) return "SDL_SCANCODE_LANG8";
    if (code == SDL_SCANCODE_LANG9) return "SDL_SCANCODE_LANG9";
    if (code == SDL_SCANCODE_ALTERASE) return "SDL_SCANCODE_ALTERASE";
    if (code == SDL_SCANCODE_SYSREQ) return "SDL_SCANCODE_SYSREQ";
    if (code == SDL_SCANCODE_CANCEL) return "SDL_SCANCODE_CANCEL";
    if (code == SDL_SCANCODE_CLEAR) return "SDL_SCANCODE_CLEAR";
    if (code == SDL_SCANCODE_PRIOR) return "SDL_SCANCODE_PRIOR";
    if (code == SDL_SCANCODE_RETURN2) return "SDL_SCANCODE_RETURN2";
    if (code == SDL_SCANCODE_SEPARATOR) return "SDL_SCANCODE_SEPARATOR";
    if (code == SDL_SCANCODE_OUT) return "SDL_SCANCODE_OUT";
    if (code == SDL_SCANCODE_OPER) return "SDL_SCANCODE_OPER";
    if (code == SDL_SCANCODE_CLEARAGAIN) return "SDL_SCANCODE_CLEARAGAIN";
    if (code == SDL_SCANCODE_CRSEL) return "SDL_SCANCODE_CRSEL";
    if (code == SDL_SCANCODE_EXSEL) return "SDL_SCANCODE_EXSEL";
    if (code == SDL_SCANCODE_KP_00) return "SDL_SCANCODE_KP_00";
    if (code == SDL_SCANCODE_KP_000) return "SDL_SCANCODE_KP_000";
    if (code == SDL_SCANCODE_THOUSANDSSEPARATOR) return "SDL_SCANCODE_THOUSANDSSEPARATOR";
    if (code == SDL_SCANCODE_DECIMALSEPARATOR) return "SDL_SCANCODE_DECIMALSEPARATOR";
    if (code == SDL_SCANCODE_CURRENCYUNIT) return "SDL_SCANCODE_CURRENCYUNIT";
    if (code == SDL_SCANCODE_CURRENCYSUBUNIT) return "SDL_SCANCODE_CURRENCYSUBUNIT";
    if (code == SDL_SCANCODE_KP_LEFTPAREN) return "SDL_SCANCODE_KP_LEFTPAREN";
    if (code == SDL_SCANCODE_KP_RIGHTPAREN) return "SDL_SCANCODE_KP_RIGHTPAREN";
    if (code == SDL_SCANCODE_KP_LEFTBRACE) return "SDL_SCANCODE_KP_LEFTBRACE";
    if (code == SDL_SCANCODE_KP_RIGHTBRACE) return "SDL_SCANCODE_KP_RIGHTBRACE";
    if (code == SDL_SCANCODE_KP_TAB) return "SDL_SCANCODE_KP_TAB";
    if (code == SDL_SCANCODE_KP_BACKSPACE) return "SDL_SCANCODE_KP_BACKSPACE";
    if (code == SDL_SCANCODE_KP_A) return "SDL_SCANCODE_KP_A";
    if (code == SDL_SCANCODE_KP_B) return "SDL_SCANCODE_KP_B";
    if (code == SDL_SCANCODE_KP_C) return "SDL_SCANCODE_KP_C";
    if (code == SDL_SCANCODE_KP_D) return "SDL_SCANCODE_KP_D";
    if (code == SDL_SCANCODE_KP_E) return "SDL_SCANCODE_KP_E";
    if (code == SDL_SCANCODE_KP_F) return "SDL_SCANCODE_KP_F";
    if (code == SDL_SCANCODE_KP_XOR) return "SDL_SCANCODE_KP_XOR";
    if (code == SDL_SCANCODE_KP_POWER) return "SDL_SCANCODE_KP_POWER";
    if (code == SDL_SCANCODE_KP_PERCENT) return "SDL_SCANCODE_KP_PERCENT";
    if (code == SDL_SCANCODE_KP_LESS) return "SDL_SCANCODE_KP_LESS";
    if (code == SDL_SCANCODE_KP_GREATER) return "SDL_SCANCODE_KP_GREATER";
    if (code == SDL_SCANCODE_KP_AMPERSAND) return "SDL_SCANCODE_KP_AMPERSAND";
    if (code == SDL_SCANCODE_KP_DBLAMPERSAND) return "SDL_SCANCODE_KP_DBLAMPERSAND";
    if (code == SDL_SCANCODE_KP_VERTICALBAR) return "SDL_SCANCODE_KP_VERTICALBAR";
    if (code == SDL_SCANCODE_KP_DBLVERTICALBAR) return "SDL_SCANCODE_KP_DBLVERTICALBAR";
    if (code == SDL_SCANCODE_KP_COLON) return "SDL_SCANCODE_KP_COLON";
    if (code == SDL_SCANCODE_KP_HASH) return "SDL_SCANCODE_KP_HASH";
    if (code == SDL_SCANCODE_KP_SPACE) return "SDL_SCANCODE_KP_SPACE";
    if (code == SDL_SCANCODE_KP_AT) return "SDL_SCANCODE_KP_AT";
    if (code == SDL_SCANCODE_KP_EXCLAM) return "SDL_SCANCODE_KP_EXCLAM";
    if (code == SDL_SCANCODE_KP_MEMSTORE) return "SDL_SCANCODE_KP_MEMSTORE";
    if (code == SDL_SCANCODE_KP_MEMRECALL) return "SDL_SCANCODE_KP_MEMRECALL";
    if (code == SDL_SCANCODE_KP_MEMCLEAR) return "SDL_SCANCODE_KP_MEMCLEAR";
    if (code == SDL_SCANCODE_KP_MEMADD) return "SDL_SCANCODE_KP_MEMADD";
    if (code == SDL_SCANCODE_KP_MEMSUBTRACT) return "SDL_SCANCODE_KP_MEMSUBTRACT";
    if (code == SDL_SCANCODE_KP_MEMMULTIPLY) return "SDL_SCANCODE_KP_MEMMULTIPLY";
    if (code == SDL_SCANCODE_KP_MEMDIVIDE) return "SDL_SCANCODE_KP_MEMDIVIDE";
    if (code == SDL_SCANCODE_KP_PLUSMINUS) return "SDL_SCANCODE_KP_PLUSMINUS";
    if (code == SDL_SCANCODE_KP_CLEAR) return "SDL_SCANCODE_KP_CLEAR";
    if (code == SDL_SCANCODE_KP_CLEARENTRY) return "SDL_SCANCODE_KP_CLEARENTRY";
    if (code == SDL_SCANCODE_KP_BINARY) return "SDL_SCANCODE_KP_BINARY";
    if (code == SDL_SCANCODE_KP_OCTAL) return "SDL_SCANCODE_KP_OCTAL";
    if (code == SDL_SCANCODE_KP_DECIMAL) return "SDL_SCANCODE_KP_DECIMAL";
    if (code == SDL_SCANCODE_KP_HEXADECIMAL) return "SDL_SCANCODE_KP_HEXADECIMAL";
    if (code == SDL_SCANCODE_LCTRL) return "SDL_SCANCODE_LCTRL";
    if (code == SDL_SCANCODE_LSHIFT) return "SDL_SCANCODE_LSHIFT";
    if (code == SDL_SCANCODE_LALT) return "SDL_SCANCODE_LALT";
    if (code == SDL_SCANCODE_LGUI) return "SDL_SCANCODE_LGUI";
    if (code == SDL_SCANCODE_RCTRL) return "SDL_SCANCODE_RCTRL";
    if (code == SDL_SCANCODE_RSHIFT) return "SDL_SCANCODE_RSHIFT";
    if (code == SDL_SCANCODE_RALT) return "SDL_SCANCODE_RALT";
    if (code == SDL_SCANCODE_RGUI) return "SDL_SCANCODE_RGUI";
    if (code == SDL_SCANCODE_MODE) return "SDL_SCANCODE_MODE";
    if (code == SDL_SCANCODE_AUDIONEXT) return "SDL_SCANCODE_AUDIONEXT";
    if (code == SDL_SCANCODE_AUDIOPREV) return "SDL_SCANCODE_AUDIOPREV";
    if (code == SDL_SCANCODE_AUDIOSTOP) return "SDL_SCANCODE_AUDIOSTOP";
    if (code == SDL_SCANCODE_AUDIOPLAY) return "SDL_SCANCODE_AUDIOPLAY";
    if (code == SDL_SCANCODE_AUDIOMUTE) return "SDL_SCANCODE_AUDIOMUTE";
    if (code == SDL_SCANCODE_MEDIASELECT) return "SDL_SCANCODE_MEDIASELECT";
    if (code == SDL_SCANCODE_WWW) return "SDL_SCANCODE_WWW";
    if (code == SDL_SCANCODE_MAIL) return "SDL_SCANCODE_MAIL";
    if (code == SDL_SCANCODE_CALCULATOR) return "SDL_SCANCODE_CALCULATOR";
    if (code == SDL_SCANCODE_COMPUTER) return "SDL_SCANCODE_COMPUTER";
    if (code == SDL_SCANCODE_AC_SEARCH) return "SDL_SCANCODE_AC_SEARCH";
    if (code == SDL_SCANCODE_AC_HOME) return "SDL_SCANCODE_AC_HOME";
    if (code == SDL_SCANCODE_AC_BACK) return "SDL_SCANCODE_AC_BACK";
    if (code == SDL_SCANCODE_AC_FORWARD) return "SDL_SCANCODE_AC_FORWARD";
    if (code == SDL_SCANCODE_AC_STOP) return "SDL_SCANCODE_AC_STOP";
    if (code == SDL_SCANCODE_AC_REFRESH) return "SDL_SCANCODE_AC_REFRESH";
    if (code == SDL_SCANCODE_AC_BOOKMARKS) return "SDL_SCANCODE_AC_BOOKMARKS";
    if (code == SDL_SCANCODE_BRIGHTNESSDOWN) return "SDL_SCANCODE_BRIGHTNESSDOWN";
    if (code == SDL_SCANCODE_BRIGHTNESSUP) return "SDL_SCANCODE_BRIGHTNESSUP";
    if (code == SDL_SCANCODE_DISPLAYSWITCH) return "SDL_SCANCODE_DISPLAYSWITCH";
    if (code == SDL_SCANCODE_KBDILLUMTOGGLE) return "SDL_SCANCODE_KBDILLUMTOGGLE";
    if (code == SDL_SCANCODE_KBDILLUMDOWN) return "SDL_SCANCODE_KBDILLUMDOWN";
    if (code == SDL_SCANCODE_KBDILLUMUP) return "SDL_SCANCODE_KBDILLUMUP";
    if (code == SDL_SCANCODE_EJECT) return "SDL_SCANCODE_EJECT";
    if (code == SDL_SCANCODE_SLEEP) return "SDL_SCANCODE_SLEEP";
    if (code == SDL_SCANCODE_APP1) return "SDL_SCANCODE_APP1";
    if (code == SDL_SCANCODE_APP2) return "SDL_SCANCODE_APP2";
    if (code == SDL_SCANCODE_AUDIOREWIND) return "SDL_SCANCODE_AUDIOREWIND";
    if (code == SDL_SCANCODE_AUDIOFASTFORWARD) return "SDL_SCANCODE_AUDIOFASTFORWARD";

    return "SDL_SCANCODE_UNKNOWN";
}
