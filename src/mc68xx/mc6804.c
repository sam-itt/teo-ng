/*
 *  Module d'émulation des micro-circuits Motorola MC68xx:
 *    - microprocesseur MC6809E
 *    - PIA MC6846
 *    - PIA MC6821
 *
 *  Copyright (C) 1996 Sylvain Huet, 1999 Eric Botcazou, 2017 François Mouret.
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
 *  Module     : mc68xx/mc6804.c
 *  Version    : 1.0
 *  Créé par   : François Mouret (& Eric Botcazou) 21/08/2017
 *  Modifié par:
 *
 *  Emulation du Motorola MC6804.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
#endif

#include "defs.h"
#include "teo.h"
#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "mc68xx/mc6804.h"
#include "to8keys.h"

#define CPU_TO_TIME(a)  ((((a)*TEO_CYCLES_PER_FRAME) \
                                    +(TEO_MICROSECONDS_PER_FRAME/2)) \
                                    /TEO_MICROSECONDS_PER_FRAME)

#define MC6804_LENGTH_START CPU_TO_TIME(100)
#define MC6804_LENGTH_ONE   CPU_TO_TIME(56)
#define MC6804_LENGTH_ZERO  CPU_TO_TIME(38)
#define MC6804_LENGTH_GAP   CPU_TO_TIME(80)
#define MC6804_LENGTH_ALL   (MC6804_LENGTH_START+ \
                             (MC6804_LENGTH_ONE+MC6804_LENGTH_GAP)*9)

#define MC6804_DELAY_START  CPU_TO_TIME(96)
#define MC6804_DELAY_INIT   CPU_TO_TIME(670)
#define MC6804_DELAY_UPPER  CPU_TO_TIME(1300)
#define MC6804_DELAY_LOWER  CPU_TO_TIME(1900)
#define MC6804_DELAY_MAX    CPU_TO_TIME(2500)

#define MC6804_MIN_DELAY_INIT   (MC6804_DELAY_START + MC6804_DELAY_INIT - 300)
#define MC6804_MIN_DELAY_UPPER  (MC6804_DELAY_START + MC6804_DELAY_UPPER - 300)
#define MC6804_MIN_DELAY_LOWER  (MC6804_DELAY_START + MC6804_DELAY_LOWER - 300)
#define MC6804_MIN_DELAY_MAX    (MC6804_DELAY_START + MC6804_DELAY_MAX - 300)
#define KB_LIST_MAX  (1+(9*2))

enum {
    MC6804_COMMAND_INIT = 0,
    MC6804_COMMAND_MAJ,
    MC6804_COMMAND_MIN
};

struct KB_LIST {
    int pos;
    int val;
};

static int keyboard_call = FALSE;
static mc6809_clock_t clock_start = 0L;
static struct KB_LIST kb_list[KB_LIST_MAX];



/* mc6804_SetCP1:
 *  Set CP1 status
 */
static void mc6804_SetCP1 (struct MC6846_PIA *mc6846, int state)
{
    if ( (mc6846->crc&0x80) != (state<<7) )
    {
        mc6846->crc=(mc6846->crc&0x7F) | (state<<7);
    }

    if  ((mc6846->crc&2) == (state<<1))
        mc6846->csr|=0x02;   /* CSR1 bit set to 1 */
    else
        mc6846->csr&=0xFD;

    MAKE_CSR();
}


/* ------------------------------------------------------------------------- */


/* mc6804_SetACK:
 *  Set status of ACK and return new state of kb_state
 */
int mc6804_SetACK (struct MC6846_PIA *mc6846, int state, int kb_state)
{
    int delay;

    /* activate PCR */
    if (state == 0)
    {
        /* update clock for read and write */
        if (clock_start == 0L)
        {
            clock_start = mc6809_clock();
            mc6804_SetCP1(mc6846, 0);
        }
    }
    /* deactivate PCR */
    else
    {
        if (keyboard_call == FALSE)
        {
            if (clock_start != 0L)
            {
                delay = mc6809_clock()-clock_start;

                if (delay > MC6804_DELAY_START)
                {
                    /* Upper case for :
                       - MC6804_DELAY_INIT
                       - MC6804_DELAY_UPPER
                       - MC6804_DELAY_MAX */
                    if ((delay < MC6804_MIN_DELAY_LOWER)
                     || (delay > MC6804_MIN_DELAY_MAX))
                    {
                        kb_state |= TEO_KEY_F_CAPSLOCK;
                    }
                    else
                    /* lower case for :
                       - MC6804_DELAY_LOWER */
                    {
                        kb_state &= ~TEO_KEY_F_CAPSLOCK;
                    }

                    if (teo_SetKeyboardLed != NULL)
                    {
                        teo_SetKeyboardLed (kb_state);
                    }
                    else  /* on re-synchronise */
                    {
                        if ((kb_state & TEO_KEY_F_CAPSLOCK) != 0)
                        {
                            mc6846->prc &= 0xF7;
                        }
                        else
                        {
                            mc6846->prc |= 8;
                        }
                    }
                }
            }
        }
        keyboard_call = FALSE;
        clock_start = 0L;
        mc6846->crc &= 0xfd;
        mc6804_SetCP1(mc6846, 1);
    }

    return kb_state;
}



/* mc6804_SetScanCode:
 *  Set the scancode
 */
void mc6804_SetScanCode (struct MC6846_PIA *mc6846, int code)
{
    int i = 0;
    int mask;

    /* set start */
    kb_list[i].pos = MC6804_LENGTH_START;
    kb_list[i++].val = 0;

    /* set scancode */
    for (mask=0x100; mask>0 ; mask>>=1)
    {
        if ((code & mask) != 0)
        {
            kb_list[i].pos = kb_list[i-1].pos + MC6804_LENGTH_ONE;
            kb_list[i++].val = 1;
        }
        else
        {
            kb_list[i].pos = kb_list[i-1].pos + MC6804_LENGTH_ZERO;
            kb_list[i++].val = 1;
        }

        kb_list[i].pos = kb_list[i-1].pos + MC6804_LENGTH_GAP;
        kb_list[i++].val = 0;
    }

    keyboard_call = TRUE;
    clock_start = 0L;
    mc6846->crc |= 0x82;
    mc6846->csr |= 0x80;
    mc6804_SetCP1 (mc6846, 1);
}



/* mc6804_UpdateCP1:
 *  Update the CP1 status
 */
void mc6804_UpdateCP1 (struct MC6846_PIA *mc6846)
{
    int i;
    mc6809_clock_t clock_time;

    if (clock_start != 0L)
    {
        if (keyboard_call == FALSE)
        {
            mc6804_SetCP1 (mc6846, 0);
        }
        else
        {
            clock_time = mc6809_clock()-clock_start;

            for (i=0; i<(KB_LIST_MAX-1); i++)
            {
                if ((int)(clock_time) < kb_list[i].pos)
                {
                    break;
                }
            }
            mc6804_SetCP1 (mc6846, kb_list[i].val);
        }
    }
}



/* mc6804_Init:
 *  Initialize the 6804
 */
void mc6804_Init (struct MC6846_PIA *mc6846)
{
    memset (kb_list, 1, sizeof(struct KB_LIST)*KB_LIST_MAX);
    mc6804_SetCP1 (mc6846, 1);
}

