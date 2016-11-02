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
 *  Copyright (C) 1997-2016 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : debug/ddisass.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 04/06/2015 14/07/2016
 *
 *  Débogueur 6809 - Affichage des mnémoniques.
 */


#ifndef SCAN_DEPEND
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"
#include "debug.h"
#include "debug/debug.h"

// static struct MC6809_REGS regs;
struct MC6809_DASM mc6809_dasm;
// static char *text = NULL;


/* remove_address:
 *  Remove an address in the list.
 */
static void remove_address (int pos)
{
    if ((pos+1) < (DASM_NLINES+1))
    {
        /* addresses list down if inside list */
        memmove (&debug.address[pos],
                 &debug.address[pos+1],
                 (DASM_NLINES-pos)*sizeof(int));
    }
    debug.address[DASM_NLINES] = ddisass_GetNextAddress (
                                     debug.address[DASM_NLINES-1]);
}



/* insert_address:
 *  Insert an address in the list.
 */
static void insert_address (int pos, int address)
{
    if ((pos+1) > DASM_NLINES)
    {
        /* addresses list up if end of list */
        memmove (&debug.address[0],
                 &debug.address[1],
                 DASM_NLINES*sizeof(int));
        debug.address[DASM_NLINES-1] = address;
        debug.address[DASM_NLINES] = (address+1)&0xffff;
    }
    else
    {
        /* just insert address */
        memmove (&debug.address[pos+1],
                 &debug.address[pos],
                 (DASM_NLINES+1-(pos+1))*sizeof(int));
        debug.address[pos] = address;
    }
}



/* update_fcbs:
 *  Insert fcb lines if address is in between.
 */
static void update_fcbs (int address)
{
    int i;
    int pos;
    int interval;
    int prev_addr;

    for (i=0; i<DASM_NLINES; i++)
    {
        /* address is in between -> creation of FCB's */
        if ((address > (debug.address[i]&0xffff))
         && (address < (debug.address[i+1]&0xffff)))
        {
            /* create the FCB's */
            prev_addr = debug.address[i]&0xffff;
            interval = (address - prev_addr) & 0xffff;
            remove_address (i);
            
            for (pos=0; pos<interval; pos++)
            {
                insert_address (
                    i+pos,
                    ((prev_addr+pos)&0xffff) | DASM_LIST_FCB);
            }
            pos = i+interval;
            insert_address (pos, address);
            /* adjust the addresses after */
            pos++;
            while (pos<DASM_NLINES)
            {
                address = ddisass_GetNextAddress (address);
                while (address > (debug.address[pos]&0xffff))
                {
                    remove_address (pos);
                }

                if (address < debug.address[pos])
                {
                    insert_address (pos, address);
                    pos++;
                }
                else
                    break;
            }
            break;
        }
        else
        /* address is a FCB -> remove FCB's */
        if ((address == (debug.address[i]&0xffff))
         && ((debug.address[i]&DASM_LIST_FCB) != 0))
        {
            while ((debug.address[i]&DASM_LIST_FCB) != 0)
            {
                remove_address (i);
            }
            debug.address[i] = address;
            break;
        }
    }
}



/* update_address_list:
 *  Rebuild list if address is outside the range.
 */
static void update_address_list (int address)
{
    int i;
    int *address_copy;

    address_copy = malloc((DASM_NLINES+1)*sizeof(int));
    if (address_copy == NULL)
        return;

    /* address < edit first line */
    if (address < (debug.address[0]&0xffff))
    {
        memmove (&address_copy[0],
                 &debug.address[0],
                 (DASM_NLINES+1)*sizeof(int));

        for (i=0; i<DASM_NLINES+1; i++)
        {
            if (address == (address_copy[0]&0xffff))
            {
                /* copy rest of addresses list and exit */
                memmove (&debug.address[i],
                         &address_copy[0],
                         (DASM_NLINES+1-i)*sizeof(int));
                break;
            }
            else
            {
                /* continue to load new adress */
                debug.address[i] = address;
                address = ddisass_GetNextAddress (address);
            }
        }
    }
    else
    /* address > edit last line */
    if (address > (debug.address[DASM_NLINES-1]&0xffff))
    {
        while (address > (debug.address[DASM_NLINES-1]&0xffff))
        {
            memmove (&debug.address[0],
                     &debug.address[1],
                     DASM_NLINES*sizeof(int));
            debug.address[DASM_NLINES-1] = ddisass_GetNextAddress (
                                               debug.address[DASM_NLINES-2]);
            debug.address[DASM_NLINES] = ddisass_GetNextAddress (
                                             debug.address[DASM_NLINES-1]);
        }
    }
    address_copy = std_free(address_copy);
}



/* check_changes:
 *  Check if dump or address list has changed and force display.
 */
static void check_changes (void)
{
    int i;
    int end;

    /* dump current memory */
//    start = debug.address[0]&0xffff;
    end = debug.address[DASM_NLINES]&0xffff;
    for (i=0; (debug.address[i]&0xffff)!=end; i++)
        debug.dump[i] = mc6809_interface.LoadByte(debug.address[0]+i);

    /* check if dump has changed */
    if (debug.dump_last_size > 0)
    {
        if (debug.dump_last_size < i)
            debug.force_display = TRUE;
        if (memcmp (debug.dump, debug.dump_last, debug.dump_last_size) != 0)
            debug.force_display = TRUE;
    }
    debug.dump_last_size = i;

    /* update last address list */
    memcpy (debug.address_last, debug.address, (DASM_NLINES+1)*sizeof(int));

    /* check if address list has changed */
    if (memcmp (debug.address_last,
                debug.address,
                (DASM_NLINES+1)*sizeof(int)) != 0)
        debug.force_display = TRUE;

    /* update last dump list */
    memcpy (debug.dump_last, debug.dump, (DASM_NLINES*5));
}



/* remove_blanks:
 *  Remove the blank lines in the address list */
static void remove_blanks (void)
{
    int i;
    int pos;

    /*  the list skipping the blanks */
    for (i=0,pos=0; pos<(DASM_NLINES+1); i++)
        if ((debug.address[i]&DASM_LIST_BLANK) == 0)
            debug.address[pos++] = debug.address[i];
}



/* add_blanks:
 *  Add the blank lines in the address list */
static void add_blanks (void)
{
    int i;
    int pos;
    int d0;
    int d1;
    int *address_copy = NULL;

    address_copy = malloc((DASM_NLINES+1)*sizeof(int));
    if (address_copy == NULL)
        return;

    memmove (address_copy, debug.address, (DASM_NLINES+1)*sizeof(int));

    for (i=0,pos=0; pos<(DASM_NLINES+1); pos++)
    {
        d0 = mc6809_interface.LoadByte(address_copy[pos]);
        d1 = mc6809_interface.LoadByte(address_copy[pos]+1);
        debug.address[i++] = address_copy[pos];

        switch (d0)
        {
            case 0x0e :  /* JMP direct */
            case 0x6e :  /* JMP indexed */
            case 0x7e :  /* JMP extended */
            case 0x3b :  /* RTI */
            case 0x39 :  /* RTS */
                debug.address[i++] = DASM_LIST_BLANK;
                break;

            case 0x35 :  /* PULS */
            case 0x37 :  /* PULU */
                if ((d1 & 0x80) != 0)    /* pull PC */
                    debug.address[i++] = DASM_LIST_BLANK;
                break;
        
            case 0x1f :  /* TFR */
                if ((d1 & 0x0f) == 0x05) /* TFR r,PC */
                    debug.address[i++] = DASM_LIST_BLANK;
                break;
        
            case 0x1e :  /* EXG */
                if (((d1 & 0x0f) == 0x05)  /* EXG r,PC */
                 || ((d1 & 0xf0) == 0x50)) /* EXG PC,r */
                    debug.address[i++] = DASM_LIST_BLANK;
                break;
        }
    }
    address_copy = std_free(address_copy);
}


/* ------------------------------------------------------------------------- */


/* get_next_address:
 *  Get the next address.
 */
int ddisass_GetNextAddress (int address)
{
    int i;

    for (i=0; i<MC6809_DASM_FETCH_SIZE; i++)
        mc6809_dasm.fetch[i]=LOAD_BYTE((address&0xffff)+i);

    mc6809_dasm.addr = address;
    mc6809_dasm.mode = MC6809_DASM_BINASM_MODE;
    
    address += dasm6809_Disassemble(&mc6809_dasm);
    address &= 0xffff;

    return address;
}



/* ddisass_GetText:
 *  Get the disassembly text.
 */
char *ddisass_GetText (char *special_cr)
{
    int i;
    int pos;
    int byte;
    char *text;
    char *p = NULL;

    text = malloc ((DASM_LINE_LENGTH+2+2)*DASM_NLINES);
    if (text != NULL)
    {
        p = text;
        *p = '\0';

        for (i=0,pos=0; pos<DASM_NLINES; i++)
        {
            for (byte=0; byte<MC6809_DASM_FETCH_SIZE; byte++)
                mc6809_dasm.fetch[byte]=LOAD_BYTE((debug.address[i]&0xffff)+byte);

            if (i > 0)
                p += sprintf(p, "%s", special_cr);

            if ((debug.address[i] & DASM_LIST_FCB) != 0)
            {
                pos++;
                p += sprintf(p, "%04X  %02X              FCB    $%02X",
                             debug.address[i]&0xffff,
                             mc6809_dasm.fetch[0],
                             mc6809_dasm.fetch[0]);
            }
            else
            if ((debug.address[i] & DASM_LIST_BLANK) == 0)
            {
                pos++;
                mc6809_dasm.addr = debug.address[i];
                mc6809_dasm.mode = MC6809_DASM_BINASM_MODE;
                (void)dasm6809_Disassemble (&mc6809_dasm);
                p += sprintf(p, "%-"DASM_LINE_LENGTH_STRING"s", mc6809_dasm.str);
            }
        }
    }
    p += sprintf(p, "%s", special_cr);
    return text;
}



/* ddisass_EditPositionning:
 *  Positionning of the text in the edit control.
 */
void ddisass_EditPositionning (int address, int vlfirst, int vlcount)
{
    int i;

    /* update the new lists */
    remove_blanks();
    update_address_list(address);
    update_fcbs(address);
    check_changes();
    add_blanks();

    /* reset first line if display forced */
    if (debug.force_display == TRUE)
        vlfirst = 0;

    /* find the line in the list */
    for (i=0,debug.line=0; i<DASM_NLINES; debug.line++)
    {
        if (address == (debug.address[debug.line]&(0xffff|DASM_LIST_BLANK)))
            break;
        if ((debug.address[debug.line]&DASM_LIST_BLANK) == 0)
            i++;
    }

    /* address < first visible line */
    if (debug.line < vlfirst)
    {
        debug.force_scroll = TRUE;
        debug.scroll = debug.line;
    }
    else
    /* address inside visible window */
    if (debug.line < vlfirst+vlcount)
    {
        debug.scroll = vlfirst;
    }
    else
    /* address > last visible line */
    {
        debug.force_scroll = TRUE;
        debug.scroll = debug.line-vlcount+1;
    }
}



/* ddisass_DoStep:
 *  Execute the instructions step by step.
 */
void ddisass_DoStep (void)
{
    mc6809_StepExec();
}

