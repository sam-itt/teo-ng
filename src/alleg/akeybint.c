#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h> 
#include <allegro.h>
/*TODO: Check whether _key_shifts can be replaced
 * by key_shifts to avoind including internal headers
 * */
#include <allegro/internal/aintern.h> 

#include "teo.h"
#include "to8keys.h"
#include "alleg/akeyboard.h"


/**
 * Toggle physical host keyboard leds
 */
static void akeybint_SetKeyboardLed(int state)
{
    int flags = 0;

    if (state&TEO_KEY_F_NUMLOCK)
        flags |= KB_NUMLOCK_FLAG;

    if (state&TEO_KEY_F_CAPSLOCK)
        flags |= KB_CAPSLOCK_FLAG;
    
    /* Actually set leds */
    set_leds(flags);
}


/**   
 * Installs the "low-level callback" back-called by Allegero
 */
void akeybint_Install(void)
{
#if PLATFORM_UNIX || PLATFORM_WIN32
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
#elif PLATFORM_MSDOS
    static int first=1;
    int mask=0, value=0;
  
    if (first)
    {
        mask |= (TEO_KEY_F_NUMLOCK | TEO_KEY_F_CAPSLOCK);

        if (_key_shifts&KB_NUMLOCK_FLAG)
            value |= TEO_KEY_F_NUMLOCK;

        if (_key_shifts&KB_CAPSLOCK_FLAG)
            value |= TEO_KEY_F_CAPSLOCK;

        /* Now the leds are ours */
        key_led_flag = FALSE;
        first=0;
    }
       
    mask |= (TEO_KEY_F_SHIFT | TEO_KEY_F_CTRL | TEO_KEY_F_ALTGR);

    if (_key_shifts&KB_SHIFT_FLAG)
        value |= TEO_KEY_F_SHIFT;

    if (_key_shifts&KB_CTRL_FLAG)
        value |= TEO_KEY_F_CTRL;

    if (_key_shifts&KB_ALT_FLAG)
        value |= TEO_KEY_F_ALTGR;
#else
#error Neither PLATFORM_{DOS,UNIX,WIN32} is defined. Cannot build.
#endif
    teo_InputReset(mask, value);
    keyboard_lowlevel_callback=akeyboard_Handler;
}



/**
 * Removes the handler. Allegro won't call it anymore
 */
void akeybint_ShutDown(void)
{
    keyboard_lowlevel_callback=NULL;
}

/* 
 * Inits the module
 */
void akeybint_Init(void)  
{
#if PLATFORM_MSDOS
    teo_SetKeyboardLed = akeybint_SetKeyboardLed;
#endif //PLATFORM_MSDOS
    akeyboard_Init();
}

