#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>

#include "defs.h"
#include "media/keyboard.h"
#include "sdl2/gfx.h"
#include "sdl2/sfront-bindings.h"

#define SPEED 1

typedef struct{
    SDL_Rect bounds;
    int tokey;
}TeoSDLVKey;


/*Line 1 (function keys) */
static TeoSDLVKey vk_line0[] = {
    {{14,14,46,16}, TOKEY_F1},
    {{62,14,46,16}, TOKEY_F2},
    {{109,14,46,16}, TOKEY_F3},
    {{157,14,46,16}, TOKEY_F4},
    {{205,14,46,16}, TOKEY_F5},
};


/*Second line, from @ to 9*/
static TeoSDLVKey vk_line1[] = {
    {{14,53,38,28}, TOKEY_NUMBER_SIGN},
    {{52,53,32,28}, TOKEY_ASTERISK},
    {{84,53,32,28}, TOKEY_E_ACUTE_LOWER_CASE},
    {{117,53,30,28}, TOKEY_QUOTE},
    {{149,53,30,28}, TOKEY_APOSTROPHE},
    {{180,53,30,28}, TOKEY_OPEN_BRACKET},
    {{212,53,30,28}, TOKEY_UNDERSCORE},
    {{244,53,30,28}, TOKEY_E_GRAVE_LOWER_CASE},
    {{276,53,30,28}, TOKEY_EXCLAMATION_MARK},
    {{308,53,30,28}, TOKEY_C_CEDILLA_LOWER_CASE},
    {{340,53,30,28}, TOKEY_A_GRAVE_LOWER_CASE},
    {{372,53,30,28}, TOKEY_CLOSE_BRACKET},
    {{404,53,30,28}, TOKEY_MINUS},
    {{436,53,30,28}, TOKEY_EQUAL},
    {{468,53,30,28}, TOKEY_ACC},
    {{500,53,62,28}, TOKEY_ARROW_UP},
    {{563,53,30,28}, TOKEY_PAD_7},
    {{596,53,30,28}, TOKEY_PAD_8},
    {{628,53,30,28}, TOKEY_PAD_9},
};


/*3rd line from stop to 9*/
static TeoSDLVKey vk_line2[] = {
    {{13,84,56,28}, TOKEY_STOP},
    {{69,84,30,28}, TOKEY_A_LOWER_CASE},
    {{101,84,30,28}, TOKEY_Z_LOWER_CASE},
    {{133,84,30,28}, TOKEY_E_LOWER_CASE},
    {{164,84,30,28}, TOKEY_R_LOWER_CASE},
    {{197,84,30,28}, TOKEY_T_LOWER_CASE},
    {{228,84,30,28}, TOKEY_Y_LOWER_CASE},
    {{260,84,30,28}, TOKEY_U_LOWER_CASE},
    {{292,84,30,28}, TOKEY_I_LOWER_CASE},
    {{324,84,30,28}, TOKEY_O_LOWER_CASE},
    {{356,84,30,28}, TOKEY_P_LOWER_CASE},
    {{388,84,30,28}, TOKEY_CIRCUMFLEX},
    {{420,84,46,28}, TOKEY_DOLLAR},
    {{467,84,32,58}, TOKEY_ENT}, /*Enter (yellow)*/
    {{500,84,30,28}, TOKEY_ARROW_LEFT},
    {{532,84,30,28}, TOKEY_ARROW_RIGHT},
    {{564,84,30,28}, TOKEY_PAD_4},
    {{596,84,30,28}, TOKEY_PAD_5},
    {{628,84,30,28}, TOKEY_PAD_6},
};


/*4th line from cont to 3*/
static TeoSDLVKey vk_line3[] = {
    {{14,114,30,28}, TOKEY_CTRL},
    {{46,114,30,28}, TOKEY_OPEN_SQUARE_BRACKET},
    {{77,114,30,28}, TOKEY_Q_LOWER_CASE},
    {{109,114,30,28}, TOKEY_S_LOWER_CASE},
    {{141,114,30,28}, TOKEY_D_LOWER_CASE},
    {{172,114,30,28}, TOKEY_F_LOWER_CASE},
    {{204,114,30,28}, TOKEY_G_LOWER_CASE},
    {{236,114,30,28}, TOKEY_H_LOWER_CASE},
    {{268,114,30,28}, TOKEY_J_LOWER_CASE},
    {{300,114,30,28}, TOKEY_K_LOWER_CASE},
    {{332,114,30,28}, TOKEY_L_LOWER_CASE},
    {{364,114,30,28}, TOKEY_M_LOWER_CASE},
    {{396,114,30,28}, TOKEY_U_GRAVE_LOWER_CASE},
    {{428,114,38,28}, TOKEY_CLOSE_SQUARE_BRACKET},
    {{467,84,32,58},  TOKEY_ENT}, /*Yellow Enter again*/
    {{500,114,62,28}, TOKEY_ARROW_DOWN},
    {{564,114,30,28}, TOKEY_PAD_1},
    {{596,114,30,28}, TOKEY_PAD_2},
    {{627,114,30,28}, TOKEY_PAD_3},
};

/*5th line from caps lock to keypad enter*/
static TeoSDLVKey vk_line4[] = {
    {{15,144,30,28}, TOKEY_CAPS_LOCK},
    {{45,144,46,28}, TOKEY_SHIFT},
    {{92,144,32,28}, TOKEY_W_LOWER_CASE},
    {{125,144,30,28}, TOKEY_X_LOWER_CASE},
    {{157,144,30,28}, TOKEY_C_LOWER_CASE},
    {{188,144,30,28}, TOKEY_V_LOWER_CASE},
    {{220,144,30,28}, TOKEY_B_LOWER_CASE},
    {{252,144,30,28}, TOKEY_N_LOWER_CASE},
    {{284,144,30,28}, TOKEY_COMMA},
    {{316,144,30,28}, TOKEY_SEMICOLON},
    {{348,144,30,28}, TOKEY_COLON},
    {{380,144,30,28}, TOKEY_GREATER_THAN},
    {{411,144,48,28}, TOKEY_SHIFT},
    {{459,144,40,28}, TOKEY_HOME},
    {{499,144,32,28}, TOKEY_INS},
    {{532,144,30,28}, TOKEY_EFF},
    {{563,144,30,28}, TOKEY_PAD_0},
    {{597,144,30,28}, TOKEY_PAD_DOT},
    {{628,144,30,28}, TOKEY_PAD_ENT}
};


/*6th line (space)*/
static TeoSDLVKey vk_line5[] = {
    {{123,175,256,28},TOKEY_SPACE}
};

static TeoSDLVKey *vkbd[]={
    vk_line0,
    vk_line1,
    vk_line2,
    vk_line3,
    vk_line4,
    vk_line5,
};

static SDL_Rect caps_led = {20,171,18,2};
static int vx,vy;
static int row,col;
static TeoSDLVKey *current_key = NULL;

/**
 * NULL when non-shifted.
 * When shifted, points to the shift key
 * responsible for the last shift
 **/
static TeoSDLVKey *shifted = NULL; 
static bool caps_locked = false;
static SDL_Surface *kbdSurface;
static SDL_Surface *kbdBuffer;
static SDL_Rect dst;
static Uint32 box_color;
static SDL_Surface *screenSurface;


static void teoSDL_VKbdPressKey(bool release);
static int teoSDL_VKbdKeyRowSize(int row);
static void teoSDL_VKbdHandleKeyboard(SDL_KeyboardEvent *event, SDL_Rect *rect);
static void teoSDL_VKbdHandleJoystick(SDL_Event *e);

static int teoSDL_VKbdKeyRowSize(int row)
{
    switch(row){
        case 0:
            return sizeof(vk_line0)/sizeof(TeoSDLVKey);
        break;
        case 1:
            return sizeof(vk_line1)/sizeof(TeoSDLVKey);
        break;
        case 2:
            return sizeof(vk_line2)/sizeof(TeoSDLVKey);
        break;
        case 3:
            return sizeof(vk_line3)/sizeof(TeoSDLVKey);
        break;
        case 4:
            return sizeof(vk_line4)/sizeof(TeoSDLVKey);
        break;
        case 5:
            return sizeof(vk_line5)/sizeof(TeoSDLVKey);
        break;
        default:
            return 0;
    }
    return 0; // never reached
}

static void teoSDL_VKbdPressKey(bool release)
{
    if(release){
        keyboard_Press_ng(current_key->tokey, release);
        current_key = NULL;
        return;
    }

    current_key = &(vkbd[row][col]);
    if(row == 4){ /*Handling shift keys*/
        if(col == 1 || col == 12){
            if(!shifted) 
                shifted = current_key;/*current key is either left or right shift*/ 
        }
        if(col == 0)
            caps_locked = !caps_locked;
    }
    if(current_key->tokey != TOKEY_SHIFT){
        if(shifted){
            keyboard_Press_ng(current_key->tokey|TOKEY_SHIFT, 0);
            shifted = NULL;
        }else{
            keyboard_Press_ng(current_key->tokey, 0);
        }
   }
}


static void teoSDL_VKbdHandleJoystick(SDL_Event *e)
{
    if(e->type == SDL_JOYAXISMOTION){
        SDL_JoyAxisEvent *event;

        event = (SDL_JoyAxisEvent *)e;

        if(event->axis == VKB_VAXIS){
            if(event->value < 0){
                vy = -1 * SPEED;
            }else if(event->value > 0){
                vy = SPEED;
            }else{
                vy = 0;
            }
        }

        if(event->axis == VKB_HAXIS){
            if(event->value < 0){
                vx = -1 * SPEED;
            }else if(event->value > 0){
                vx = SPEED;
            }else{
                vx = 0;
            }
        }
        return;
    }

    if(e->type == SDL_JOYBUTTONDOWN || e->type == SDL_JOYBUTTONUP){
        SDL_JoyButtonEvent *event;
        event = (SDL_JoyButtonEvent *)e;

        if(event->button == VKB_BTN){
            teoSDL_VKbdPressKey((event->state == SDL_RELEASED));
        }
        return;
    }

    if(e->type == SDL_JOYHATMOTION){
        SDL_JoyHatEvent *event;

        event = (SDL_JoyHatEvent *)e;
        switch(event->value){
            case SDL_HAT_LEFTUP:
                vx = -1 * SPEED;
                vy = -1 * SPEED;
                break;
            case SDL_HAT_LEFT:
                vx = -1 * SPEED;
                vy = 0;
                break;
            case SDL_HAT_LEFTDOWN:
                vx = -1 * SPEED;
                vy = SPEED;
                break;
            case SDL_HAT_UP:
                vx = 0;
                vy = -1 * SPEED;
                break;
            case SDL_HAT_CENTERED:
                vx = 0;
                vy = 0;
                break;
            case SDL_HAT_DOWN:
                vx = 0;
                vy = SPEED;
                break;
            case SDL_HAT_RIGHTUP:
                vx = SPEED;
                vy = -1 * SPEED;
                break;
            case SDL_HAT_RIGHT:
                vx = SPEED;
                vy = 0;
                break;
            case SDL_HAT_RIGHTDOWN:
                vx = SPEED;
                vy = SPEED;
                break;
        }
        return;
    }
}

static void teoSDL_VKbdHandleKeyboard(SDL_KeyboardEvent *event, SDL_Rect *rect)
{
    switch(event->keysym.sym){
        /*Move*/
        case SDLK_LEFT:
            if(event->state == SDL_PRESSED)
                vx = -1*SPEED;
            else
                vx = 0;
            break;
        case SDLK_RIGHT:
            if(event->state == SDL_PRESSED)
                vx = 1*SPEED;
            else
                vx = 0;
            break;
        case SDLK_UP:
            if(event->state == SDL_PRESSED)
                vy = -1*SPEED;
            else
                vy = 0;
            break;
        case SDLK_DOWN:
            if(event->state == SDL_PRESSED)
                vy = 1*SPEED;
            else
                vy = 0;
            break;
        /*Print*/
        case SDLK_RETURN:
            teoSDL_VKbdPressKey((event->state == SDL_RELEASED));
            break;
    }
}

void teoSDL_VKbdInit(void)
{
    kbdSurface = SDL_LoadBMP("keyboard.bmp");
    if(!kbdSurface){
        printf("Error: %s\n",SDL_GetError());
        exit(-1);
    }
    kbdBuffer = SDL_CreateRGBSurface(kbdSurface->flags, kbdSurface->w, kbdSurface->h, 
                      kbdSurface->format->BitsPerPixel, kbdSurface->format->Rmask,
                      kbdSurface->format->Gmask, kbdSurface->format->Bmask,
                      kbdSurface->format->Amask);
    if(!kbdBuffer){
        printf("Error: %s\n",SDL_GetError());
        exit(-1);
    }
}

void teoSDL_VKbdShutdown(void)
{
    SDL_FreeSurface(kbdBuffer);   
    SDL_FreeSurface(kbdSurface);   
}


void teoSDL_VKbdSetScreen(SDL_Surface *screen)
{
    screenSurface = screen;

    dst.x=0;
    dst.y = (screenSurface->h - kbdSurface->h) - 1;
//    dst.w=kbdSurface->w;
//    dst.h=kbdSurface->h;
    dst.w=screenSurface->w;
    dst.h=kbdSurface->h;

    box_color = SDL_MapRGB(screenSurface->format, 0xFF, 0x00, 0x00);
}

void teoSDL_VKbdSetWindow(SDL_Window *win)
{
//    sdlWindow = win;

    teoSDL_VKbdSetScreen(SDL_GetWindowSurface(win));
}


void teoSDL_VKbdUpdate(void)
{
    SDL_Rect *box;

    col += vx;
    row += vy;

    /* Disable auto-repeat, it's way too fast
     * TODO: Investigate why
     * */
    vx = 0;
    vy = 0;

    col = MAX(col, 0);
    row = MAX(row, 0);
    row = MIN(row, 6-1);
    col = MIN(col, teoSDL_VKbdKeyRowSize(row)-1);

    box = &(vkbd[row][col].bounds);
    SDL_BlitSurface(kbdSurface,NULL, kbdBuffer, NULL);
    rectangleRGBA(kbdBuffer, box->x, box->y, box->x+box->w, box->y+box->h, 0xFF,0,0,0xFF);
    if(shifted)
        rectangleRGBA(kbdBuffer, shifted->bounds.x, shifted->bounds.y, 
                      shifted->bounds.x+shifted->bounds.w, 
                      shifted->bounds.y+shifted->bounds.h, 0xFF,0,0,0xFF);
    if(caps_locked)
        SDL_FillRect(kbdBuffer, &caps_led, box_color);

    SDL_BlitScaled(kbdBuffer, NULL, screenSurface, &dst);
}

#if 0
int teoSDL_VKbdGetPressedKey(void)
{
    if(current_key){
        if(shifted){
            return current_key->tokey|TOKEY_SHIFT;
        }
        return current_key->tokey
    }
    return 0;
}
#endif

void teoSDL_VKbdProcessEvent(SDL_Event *event)
{
    switch(event->type){
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            teoSDL_VKbdHandleKeyboard(&(event->key), NULL);
            break;
        case SDL_JOYAXISMOTION:
        case SDL_JOYHATMOTION:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            teoSDL_VKbdHandleJoystick(event);
    }
}
