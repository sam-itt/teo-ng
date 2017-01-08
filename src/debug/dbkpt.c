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
 *  Module     : debug/dbkpt.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 16/11/2015 08/03/2016 14/07/2016
 *
 *  Débogueur 6809 - Gestion des breakpoints.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"


/* ------------------------------------------------------------------------- */

/* dbkpt_BreakPoint:
 *  Check if there is breakpoint.
 */
int dbkpt_BreakPoint(int pc)
{
    int i;

    for (i=0; i<MAX_BREAKPOINTS; i++)
        if (teo.debug.breakpoint[i]==pc)
            return 1;
    return 0;
}



/* wdbkpt_TraceOn:
 *  Turn the trace on.
 */
void dbkpt_TraceOn(void)
{
    teo_DebugBreakPoint = dbkpt_BreakPoint;
}



/* wdbkpt_TraceOff:
 *  Turn the trace off.
 */
void dbkpt_TraceOff(void)
{
    teo_DebugBreakPoint = NULL;
}



/* wdbkpt_Exit:
 *  Exit the breakpoints.
 */
void dbkpt_Exit (void)
{
}

