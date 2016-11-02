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
 *  Module     : debug/debug.h
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 30/07/2011
 *               François Mouret 21/03/2012 21/05/2016 14/07/2016
 *
 *  Primitives pour le débogueur GUI .
 */


#ifndef DEBUG_DEBUG_H
#define DEBUG_DEBUG_H

#define WATCH_MAX  100000
#define FILENT_LENGTH  127

#define DASM_LIST_FCB  0x10000
#define DASM_LIST_BLANK  0x20000

#define DASM_LINE_LENGTH  54
#define DASM_LINE_LENGTH_STRING  "54"
#undef DASM_NLINES
#define DASM_NLINES 150

extern struct MC6809_DEBUG debug;

/* disassembly */
extern char  *ddisass_GetText (char *special_cr);
extern int   ddisass_GetNextAddress (int address);
extern void  ddisass_EditPositionning (int address, int vlfirst, int vlcount);
extern void  ddisass_DoStep (void);
/* extras registers */
extern char  *dreg_GetText (char *special_cr);
/* memory */
extern int   dmem_GetJumpAddress (void);
extern char  *dmem_GetText (int address, uint8 *addr_ptr, char *special_cr);
extern int   dmem_GetStepAddress(void);
extern uint8 *dmem_GetDisplayPointer(void);
/* accumulators */
extern void dacc_GetDumpFor16Bits (char *p, int address);
/* breakpoints */
extern void  dbkpt_TraceOn(void);
extern void  dbkpt_TraceOff(void);
/* status bar */
/* tool bar */

#endif

