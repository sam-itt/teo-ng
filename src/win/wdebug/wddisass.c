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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : win/wdebug/wddisass.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 04/06/2015
 *
 *  Débogueur 6809 - Affichage des mnémoniques.
 */


#ifndef SCAN_DEPEND
    #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"
#include "win/gui.h"

#define DASM_NLINES 150
#define DASM_LIST_FCB  0x10000
#define DASM_LIST_BLANK  0x20000

#define DASM_LINE_LENGTH  54
#define DASM_LINE_LENGTH_STRING  "54"


static HFONT hfont_normal = NULL;

static struct MC6809_REGS regs;
static struct MC6809_DASM mc6809_dasm;

static int dasm_force_display = FALSE;
static int dasm_scroll = 0;
static int dasm_force_scroll = FALSE;
static int dasm_line = 0;

static int *dasm_address = NULL;
static int *dasm_address_last = NULL;
static char *dasm_dump = NULL;
static char *dasm_dump_last = NULL;
static int dasm_dump_last_size = 0;

static char *text = NULL;



/* get_next_address:
 *  Get the next address.
 */
static int get_next_address (int address)
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



/* get_text:
 *  Get the disassembly text.
 */
static void get_text (char *p)
{
    int i;
    int pos;
    int byte;

    p[0] = '\0';

    for (i=0,pos=0; pos<DASM_NLINES; i++)
    {
        for (byte=0; byte<MC6809_DASM_FETCH_SIZE; byte++)
            mc6809_dasm.fetch[byte]=LOAD_BYTE((dasm_address[i]&0xffff)+byte);

        if (i > 0)
            p += sprintf(p, "\r\n");

        if ((dasm_address[i] & DASM_LIST_FCB) != 0)
        {
            pos++;
            p += sprintf(p, "%04X  %02X              FCB    $%02X",
                         dasm_address[i]&0xffff,
                         mc6809_dasm.fetch[0],
                         mc6809_dasm.fetch[0]);
        }
        else
        if ((dasm_address[i] & DASM_LIST_BLANK) == 0)
        {
            pos++;
            mc6809_dasm.addr = dasm_address[i];
            mc6809_dasm.mode = MC6809_DASM_BINASM_MODE;
            (void)dasm6809_Disassemble (&mc6809_dasm);
            p += sprintf(p, "%-"DASM_LINE_LENGTH_STRING"s", mc6809_dasm.str);
        }
    }
}



/* remove_address:
 *  Remove an address in the list.
 */
static void remove_address (int pos)
{
    if ((pos+1) < (DASM_NLINES+1))
    {
        /* addresses list down if inside list */
        memmove (&dasm_address[pos],
                 &dasm_address[pos+1],
                 (DASM_NLINES-pos)*sizeof(int));
    }
    dasm_address[DASM_NLINES] = get_next_address (dasm_address[DASM_NLINES-1]);
}



/* insert_address:
 *  Insert an address in the list.
 */
static void insert_address (int pos, int address)
{
    if ((pos+1) > DASM_NLINES)
    {
        /* addresses list up if end of list */
        memmove (&dasm_address[0],
                 &dasm_address[1],
                 DASM_NLINES*sizeof(int));
        dasm_address[DASM_NLINES-1] = address;
        dasm_address[DASM_NLINES] = (address+1)&0xffff;
    }
    else
    {
        /* just insert address */
        memmove (&dasm_address[pos+1],
                 &dasm_address[pos],
                 (DASM_NLINES+1-(pos+1))*sizeof(int));
        dasm_address[pos] = address;
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
        if ((address > (dasm_address[i]&0xffff))
         && (address < (dasm_address[i+1]&0xffff)))
        {
            /* create the FCB's */
            prev_addr = dasm_address[i]&0xffff;
            interval = (address - prev_addr) & 0xffff;
            remove_address (i);
            
            for (pos=0; pos<interval; pos++)
            {
                insert_address (i+pos, ((prev_addr+pos)&0xffff) | DASM_LIST_FCB);
            }
            pos = i+interval;
            insert_address (pos, address);
            /* adjust the addresses after */
            pos++;
            while (pos<DASM_NLINES)
            {
                address = get_next_address (address);
                while (address > (dasm_address[pos]&0xffff))
                {
                    remove_address (pos);
                }

                if (address < dasm_address[pos])
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
        if ((address == (dasm_address[i]&0xffff))
         && ((dasm_address[i]&DASM_LIST_FCB) != 0))
        {
            while ((dasm_address[i]&DASM_LIST_FCB) != 0)
            {
                remove_address (i);
            }
            dasm_address[i] = address;
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
    int *dasm_address_copy;

    dasm_address_copy = malloc((DASM_NLINES+1)*sizeof(int));
    if (dasm_address_copy == NULL)
        return;

    /* address < edit first line */
    if (address < (dasm_address[0]&0xffff))
    {
        memmove (&dasm_address_copy[0],
                 &dasm_address[0],
                 (DASM_NLINES+1)*sizeof(int));

        for (i=0; i<DASM_NLINES+1; i++)
        {
            if (address == (dasm_address_copy[0]&0xffff))
            {
                /* copy rest of addresses list and exit */
                memmove (&dasm_address[i],
                         &dasm_address_copy[0],
                         (DASM_NLINES+1-i)*sizeof(int));
                break;
            }
            else
            {
                /* continue to load new adress */
                dasm_address[i] = address;
                address = get_next_address (address);
            }
        }
    }
    else
    /* address > edit last line */
    if (address > (dasm_address[DASM_NLINES-1]&0xffff))
    {
        while (address > (dasm_address[DASM_NLINES-1]&0xffff))
        {
            memmove (&dasm_address[0],
                     &dasm_address[1],
                     DASM_NLINES*sizeof(int));
            dasm_address[DASM_NLINES-1] = get_next_address (dasm_address[DASM_NLINES-2]);
            dasm_address[DASM_NLINES] = get_next_address (dasm_address[DASM_NLINES-1]);
        }
    }
    dasm_address_copy = std_free(dasm_address_copy);
}



/* check_changes:
 *  Check if dump or address list has changed and force display.
 */
static void check_changes (void)
{
    int i;
    int start;
    int end;

    /* dump current memory */
    start = dasm_address[0]&0xffff;
    end = dasm_address[DASM_NLINES]&0xffff;
    for (i=0; (dasm_address[i]&0xffff)!=end; i++)
        dasm_dump[i] = mc6809_interface.LoadByte(dasm_address[0]+i);

    /* check if dump has changed */
    if (dasm_dump_last_size > 0)
    {
        if (dasm_dump_last_size < i)
            dasm_force_display = TRUE;
        if (memcmp (dasm_dump, dasm_dump_last, dasm_dump_last_size) != 0)
            dasm_force_display = TRUE;
    }
    dasm_dump_last_size = i;

    /* update last address list */
    memcpy (dasm_address_last, dasm_address, (DASM_NLINES+1)*sizeof(int));

    /* check if address list has changed */
    if (memcmp (dasm_address_last,
                dasm_address,
                (DASM_NLINES+1)*sizeof(int)) != 0)
        dasm_force_display = TRUE;

    /* update last dump list */
    memcpy (dasm_dump_last, dasm_dump, (DASM_NLINES*5));
}



/* remove_blanks:
 *  Remove the blank lines in the address list */
static void remove_blanks (void)
{
    int i;
    int pos;

    /*  the list skipping the blanks */
    for (i=0,pos=0; pos<(DASM_NLINES+1); i++)
        if ((dasm_address[i]&DASM_LIST_BLANK) == 0)
            dasm_address[pos++] = dasm_address[i];
}



/* add_blanks:
 *  Add the blank lines in the address list */
static void add_blanks (void)
{
    int i;
    int pos;
    int d0;
    int d1;
    int *dasm_address_copy = NULL;

    dasm_address_copy = malloc((DASM_NLINES+1)*sizeof(int));
    if (dasm_address_copy == NULL)
        return;

    memmove (dasm_address_copy, dasm_address, (DASM_NLINES+1)*sizeof(int));

    for (i=0,pos=0; pos<(DASM_NLINES+1); pos++)
    {
        d0 = mc6809_interface.LoadByte(dasm_address_copy[pos]);
        d1 = mc6809_interface.LoadByte(dasm_address_copy[pos]+1);
        dasm_address[i++] = dasm_address_copy[pos];

        switch (d0)
        {
            case 0x0e :  /* JMP direct */
            case 0x6e :  /* JMP indexed */
            case 0x7e :  /* JMP extended */
            case 0x3b :  /* RTI */
            case 0x39 :  /* RTS */
                dasm_address[i++] = DASM_LIST_BLANK;
                break;

            case 0x35 :  /* PULS */
            case 0x37 :  /* PULU */
                if ((d1 & 0x80) != 0)    /* pull PC */
                    dasm_address[i++] = DASM_LIST_BLANK;
                break;
        
            case 0x1f :  /* TFR */
                if ((d1 & 0x0f) == 0x05) /* TFR r,PC */
                    dasm_address[i++] = DASM_LIST_BLANK;
                break;
        
            case 0x1e :  /* EXG */
                if (((d1 & 0x0f) == 0x05)  /* EXG r,PC */
                 || ((d1 & 0xf0) == 0x50)) /* EXG PC,r */
                    dasm_address[i++] = DASM_LIST_BLANK;
                break;
        }
    }
    dasm_address_copy = std_free(dasm_address_copy);
}



/* edit_positionning:
 *  Positionning of the text in the edit control.
 */
static void edit_positionning (int address, int vlfirst, int vlcount)
{
    int i;

    /* update the new lists */
    remove_blanks();
    update_address_list(address);
    update_fcbs(address);
    check_changes();
    add_blanks();

    /* reset first line if display forced */
    if (dasm_force_display == TRUE)
        vlfirst = 0;

    /* find the line in the list */
    for (i=0,dasm_line=0; i<DASM_NLINES; dasm_line++)
    {
        if (address == (dasm_address[dasm_line]&(0xffff|DASM_LIST_BLANK)))
            break;
        if ((dasm_address[dasm_line]&DASM_LIST_BLANK) == 0)
            i++;
    }

    /* address < first visible line */
    if (dasm_line < vlfirst)
    {
        dasm_force_scroll = TRUE;
        dasm_scroll = dasm_line;
    }
    else
    /* address inside visible window */
    if (dasm_line < vlfirst+vlcount)
    {
        dasm_scroll = vlfirst;
    }
    else
    /* address > last visible line */
    {
        dasm_force_scroll = TRUE;
        dasm_scroll = dasm_line-vlcount+1;
    }
}



/* debug_display_dasm:
 *  Display the disassembling.
 */
static void debug_display_dasm (HWND hDlg, char *ptxt)
{ 
    int index;
    int length;
    RECT rect;
    int visible_line_count;
    int visible_line_first;
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_DASM_EDIT);

    /* set text and line positionning */
    mc6809_GetRegs(&regs);
    Edit_GetRect(hwnd, &rect);
    visible_line_count = (int)((rect.bottom-rect.top)/FIXED_WIDTH_FONT_HEIGHT);
    visible_line_first = (int)Edit_GetFirstVisibleLine(hwnd);
    edit_positionning (regs.pc&0xffff, visible_line_first, visible_line_count);

    /* display and scroll the new text */
    if (dasm_force_display == TRUE)
    {
        get_text(ptxt);
        Edit_SetText (hwnd, ptxt);
        Edit_Scroll (hwnd, dasm_scroll, 0);
    }
    if ((dasm_force_display == TRUE) || (dasm_force_scroll == TRUE))
    {
        visible_line_first = (int)Edit_GetFirstVisibleLine(hwnd);
        Edit_Scroll (hwnd, dasm_scroll-visible_line_first, 0);
    }
    dasm_force_display = FALSE;
    dasm_force_scroll = FALSE;

    /* highlight selection */
    index = Edit_LineIndex (hwnd, dasm_line);
    length = Edit_LineLength (hwnd, dasm_line);
    Edit_SetSel (hwnd, index, index+length+2);
}


/* ------------------------------------------------------------------------- */


/* disass_init:
 *  Initialize the disassembly.
 */
void wddisass_Init (HWND hDlg)
{
    int i;
    int address;
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_DASM_EDIT);

    hfont_normal = wdebug_GetNormalFixedWidthHfont();
    if (hfont_normal != NULL)
        SetWindowFont(hwnd, hfont_normal, TRUE);

    dasm_address = malloc((DASM_NLINES+1)*sizeof(int)*2);
    if (dasm_address == NULL)
        return;

    dasm_address_last = malloc((DASM_NLINES+1)*sizeof(int)*2);
    if (dasm_address_last == NULL)
        return;

    dasm_dump = malloc(DASM_NLINES*5);
    if (dasm_dump == NULL)
        return;

    dasm_dump_last = malloc(DASM_NLINES*5);
    if (dasm_dump_last == NULL)
        return;

    mc6809_GetRegs(&regs);
    address = regs.pc&0xffff;
    for (i=0; i<DASM_NLINES+1; i++)
    {
        dasm_address[i] = address;
        address = get_next_address (address);
    }
    dasm_force_display = TRUE;  /* force text display and scroll */
}



/* wddisass_DoStep:
 *  Execute the instructions step by step.
 */
void wddisass_DoStep (void)
{
    mc6809_StepExec();
}



/* wddisass_DoStepOver:
 *  Execute the instructions step by step skipping the jumps to subroutine.
 */
#define WATCH_MAX  40000
void wddisass_DoStepOver (HWND hwnd)
{
    int pc;
    int next_pc;
    int sr = -1;
    int ptr = -1;
    int watch=0;

    mc6809_GetRegs(&regs);
    pc = regs.pc;
    next_pc = get_next_address (pc);

    switch (mc6809_dasm.fetch[0])
    {
        case 0x9d : /* JSR direct */
        case 0xad : /* JSR indexed */
        case 0xbd : /* JSR extended */
        case 0x8d : /* BSR */
        case 0x17 : /* LBSR */
            do
            {
                mc6809_StepExec();
                mc6809_GetRegs(&regs);
                if (sr == -1)
                {
                    sr = regs.sr&0xffff;
                    ptr = (LOAD_BYTE(sr)<<8)|LOAD_BYTE(sr+1);
                }
            } while (((watch++)<WATCH_MAX)
                  && (regs.pc != next_pc)
                  && (regs.sr != (sr+2))
                  && (ptr == ((LOAD_BYTE(sr)<<8)|LOAD_BYTE(sr+1))));

            if (ptr != ((LOAD_BYTE(sr)<<8)|LOAD_BYTE(sr+1)))
                wgui_Warning (hwnd, is_fr
                    ?"L'exécution pas-à-pas a été interrompue car le \n"\
                     "pointeur de retour vient d'être changé dans la pile."
                    :"The single-stepping has been aborted because the \n"\
                     "return pointer has just been overwritten in stack."
                );
            else
            if ((regs.sr == (sr+2)) && (regs.pc != next_pc))
                wgui_Warning (hwnd, is_fr
                    ?"L'exécution pas-à-pas a été interrompue car le\n"\
                     "pointeur de retour vient d'être dépilé avant le retour\n"\
                     "du sous-programme."
                    :"The single-stepping has been aborted because the \n" \
                     "return pointer has just been pulled from stack before\n" \
                     "the return from subroutine."
                );
            else
            if (watch > WATCH_MAX)
                wgui_Warning (hwnd, is_fr
                    ?"L'exécution pas-à-pas a été interrompue à cause du \n"\
                     "trop grand nombre d'instructions exécutées.\n" \
                     "La sous-routine peut comporter une boucle infinie."
                    :"The single-stepping has been aborted because of the \n"\
                     "great number of executed instructions.\n" \
                     "The subroutine could have an infinite loop."
                );
            else
            if (regs.sr != (sr+2))
                wgui_Warning (hwnd, is_fr
                    ?"L'exécution pas-à-pas a été interrompue car le \n"\
                     "le pointeur de pile a changé."
                    :"The single-stepping has been aborted because the \n"\
                     "the stack pointer has changed."
                );
            break;

        default :
            mc6809_StepExec();
            break;
    }
}



/* wddisass_Display:
 *  Display the disassembly.
 */
void wddisass_Display(HWND hDlg)
{ 
    text = malloc ((DASM_LINE_LENGTH+2+2)*DASM_NLINES);
    if ((text != NULL)
     && (dasm_address != NULL)
     && (dasm_address_last != NULL)
     && (dasm_dump != NULL)
     && (dasm_dump_last != NULL))
    {
        debug_display_dasm(hDlg, text);
        text = std_free(text);
    }
}



/* wddisass_Exit:
 *  Exit the disassembly area.
 */
void wddisass_Exit(HWND hDlg)
{
    if (hfont_normal != NULL)
    {
        (void)DeleteObject((HGDIOBJ)hfont_normal);
        hfont_normal = NULL;
    }

    text = std_free(text);
    dasm_address = std_free(dasm_address);
    dasm_address_last = std_free(dasm_address_last);
    dasm_dump = std_free(dasm_dump);
    dasm_dump_last = std_free(dasm_dump_last);
    (void)hDlg;
}

