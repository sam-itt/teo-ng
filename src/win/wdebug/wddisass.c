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
 *  Modifié par: François Mouret 04/06/2015 15/07/2016
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
#include "debug.h"
#include "debug/debug.h"
#include "win/gui.h"

static HFONT hfont_normal = NULL;
static struct MC6809_REGS regs;



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
    char *text = NULL;

    /* set text and line positionning */
    mc6809_GetRegs(&regs);
    Edit_GetRect(hwnd, &rect);
    visible_line_count = (int)((rect.bottom-rect.top)/FIXED_WIDTH_FONT_HEIGHT);
    visible_line_first = (int)Edit_GetFirstVisibleLine(hwnd);
    ddisass_EditPositionning (regs.pc&0xffff,
                              visible_line_first,
                              visible_line_count);

    /* display and scroll the new text */
    if (debug.force_display == TRUE)
    {
        /* Get the new text */
        text = ddisass_GetText ("\r\n");

        if (text != NULL)
        {
            Edit_SetText (hwnd, text);
            free (text);
            Edit_Scroll (hwnd, debug.scroll, 0);
        }
    }
    if ((debug.force_display == TRUE) || (debug.force_scroll == TRUE))
    {
        visible_line_first = (int)Edit_GetFirstVisibleLine(hwnd);
        Edit_Scroll (hwnd, debug.scroll-visible_line_first, 0);
    }
    debug.force_display = FALSE;
    debug.force_scroll = FALSE;

    /* highlight selection */
    index = Edit_LineIndex (hwnd, debug.line);
    length = Edit_LineLength (hwnd, debug.line);
    Edit_SetSel (hwnd, index, index+length+2);
}



static void run_subroutine (HWND hwnd, int next_pc)
{
    int watch=0;
    int sr = -1;
    int ptr = -1;

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
            ?"L'exécution pas-à-pas a été interrompue car le\n" \
             "pointeur de retour vient d'être changé dans la pile."
            :"The single-stepping has been aborted because the\n" \
             "return pointer has just been overwritten in stack.");
    else
    if ((regs.sr == (sr+2)) && (regs.pc != next_pc))
        wgui_Warning (hwnd, is_fr
            ?"L'exécution pas-à-pas a été interrompue car le\n" \
             "pointeur de retour vient d'être dépilé avant le retour\n" \
             "du sous-programme."
            :"The single-stepping has been aborted because the\n" \
             "return pointer has just been pulled from stack before\n" \
             "the return from subroutine.");
    else
    if (watch > WATCH_MAX)
        wgui_Warning (hwnd, is_fr
            ?"L'exécution pas-à-pas a été interrompue à cause du\n" \
             "trop grand nombre d'instructions exécutées.\n" \
             "La sous-routine peut comporter une boucle infinie."
            :"The single-stepping has been aborted because of the\n" \
             "great number of executed instructions.\n" \
             "The subroutine could have an infinite loop.");
    else
    if (regs.sr != (sr+2))
        wgui_Warning (hwnd, is_fr
            ?"L'exécution pas-à-pas a été interrompue car\n" \
             "le pointeur de pile a changé."
            :"The single-stepping has been aborted because\n" \
             "the stack pointer has changed.");
}



static void exit_loop (HWND hwnd, int next_pc)
{
    int watch=0;

    do
    {
        mc6809_StepExec();
        mc6809_GetRegs(&regs);
    } while (((watch++)<WATCH_MAX) && (regs.pc != next_pc));

    if (watch > WATCH_MAX)
        wgui_Warning (hwnd, is_fr
            ?"L'exécution pas-à-pas a été interrompue à cause du\n" \
             "trop grand nombre d'instructions exécutées.\n" \
             "Il pourrait s'agir d'une boucle infinie."
            :"The single-stepping has been aborted because of the\n" \
             "great number of executed instructions.\n" \
             "It could be an infinite loop.");
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

    debug.address = malloc((DASM_NLINES+1)*sizeof(int)*2);
    if (debug.address == NULL)
        return;

    debug.address_last = malloc((DASM_NLINES+1)*sizeof(int)*2);
    if (debug.address_last == NULL)
        return;

    debug.dump = malloc(DASM_NLINES*5);
    if (debug.dump == NULL)
        return;

    debug.dump_last = malloc(DASM_NLINES*5);
    if (debug.dump_last == NULL)
        return;

    mc6809_GetRegs(&regs);
    address = regs.pc&0xffff;
    for (i=0; i<DASM_NLINES+1; i++)
    {
        debug.address[i] = address;
        address = ddisass_GetNextAddress (address);
    }
    debug.force_display = TRUE;  /* force text display and scroll */
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
void wddisass_DoStepOver (HWND hwnd)
{
    int pc;
    int next_pc;
    int offset;

    mc6809_GetRegs(&regs);
    pc = regs.pc;
    next_pc = ddisass_GetNextAddress (pc);

    switch (mc6809_dasm.fetch[0])
    {
        /* Jump to subroutine */
        case 0x9d : /* JSR direct */
        case 0xad : /* JSR indexed */
        case 0xbd : /* JSR extended */
        case 0x8d : /* BSR */
        case 0x17 : /* LBSR */
            run_subroutine (hwnd, next_pc);
            break;

        /* Short branches */
        case 0x22 : /* BHI */
        case 0x23 : /* BLS */
        case 0x24 : /* BCC */
        case 0x25 : /* BCS */
        case 0x26 : /* BNE */
        case 0x27 : /* BEQ */
        case 0x28 : /* BVC */
        case 0x29 : /* BVS */
        case 0x2a : /* BPL */
        case 0x2b : /* BMI */
        case 0x2c : /* BGE */
        case 0x2d : /* BLT */
        case 0x2e : /* BGT */
        case 0x2f : /* BLE */
            /* Only backward branch */
            offset = mc6809_dasm.fetch[1]&0xff;
            if ((offset <= 0xfd) && (offset >= 0x80))
            {
                exit_loop (hwnd, next_pc);
            }
            else
                mc6809_StepExec();
            break;

        /* Long branches */
        case 0x10 : /* Long branch */
            switch (mc6809_dasm.fetch[1])
            {
                case 0x22 : /* LBHI */
                case 0x23 : /* LBLS */
                case 0x24 : /* LBCC */
                case 0x25 : /* LBCS */
                case 0x26 : /* LBNE */
                case 0x27 : /* LBEQ */
                case 0x28 : /* LBVC */
                case 0x29 : /* LBVS */
                case 0x2a : /* LBPL */
                case 0x2b : /* LBMI */
                case 0x2c : /* LBGE */
                case 0x2d : /* LBLT */
                case 0x2e : /* LBGT */
                case 0x2f : /* LBLE */
                    /* Only backward branch */
                    offset = (mc6809_dasm.fetch[2]&0xff)<<8;
                    offset |= mc6809_dasm.fetch[3]&0xff;
                    if ((offset <= 0xfffb) && (offset >= 0x8000))
                    {
                        exit_loop (hwnd, next_pc);
                    }
                    else
                        mc6809_StepExec();
                    break;

                /* Others */
                default :
                    mc6809_StepExec();
                    break;
            }
            break;

        /* Others */
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
    char *text;

    if ((debug.address != NULL)
     && (debug.address_last != NULL)
     && (debug.dump != NULL)
     && (debug.dump_last != NULL))
    {
        text = ddisass_GetText ("\r\n");
        if (text != NULL)
        {
            debug_display_dasm(hDlg, text);
            text = std_free(text);
        }
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

    debug.address = std_free(debug.address);
    debug.address_last = std_free(debug.address_last);
    debug.dump = std_free(debug.dump);
    debug.dump_last = std_free(debug.dump_last);
    (void)hDlg;
}

