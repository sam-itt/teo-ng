/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2016 Yves Charriau, François Mouret
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
 *  Module     : windows/gui/extract.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  About callback.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <windows.h>
   #include <windowsx.h>
   #include <shellapi.h>
   #include <commctrl.h>
#endif

#include "defs.h"
#include "main.h"
#include "std.h"
#include "errors.h"
#include "encode.h"
#include "win/gui.h"
#include "win/progress.h"
#include "win/resource.h"


/* ------------------------------------------------------------------------- */


void extract_Prog (void)
{
    int ret;

    ret = gui_OpenFile (OFN_HIDEREADONLY
                      | OFN_NOCHANGEDIR
                      | OFN_FILEMUSTEXIST,
                      is_fr?"Choisissez une disquette:"
                           :"Choose a disk:");
    if (ret == FALSE)
        return;
        
    ret = gui_InformationDialog (
                   is_fr?"Introduisez une disquette dans le lecteur " \
                         "du Thomson et clickez sur OK pour commencer " \
                         "l'écriture."
                        :"Insert a disk in the Thomson drive and click " \
                         "OK to start the writing.");
    if (ret == IDCANCEL)
        return;

    gui_EnableButtons (FALSE);
    gui_SetProgressText (is_fr?"Ecriture des pistes...":"Writing tracks...");

    progress_Run (main_ExtractDisk);
}

