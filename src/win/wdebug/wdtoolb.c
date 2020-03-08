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
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : win/wdebug/wdstatus.c
 *  Version    : 1.8.5
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: Samuel Cuella 02/2020 
 *
 *  Débogueur 6809 - Gestion de la barre d'outils.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
    #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "win/gui.h"
#include "gettext.h"

typedef struct button{
    int message_id;
    char *text;
    char image_name[20];
}button_t; 

static HIMAGELIST imgl;


/* wdtoolb_Init:
 *  Initialize the tool bar.
 */
#define NUM_TOOLBAR_BUTTONS 4
void wdtoolb_Init (HWND hDlg)
{
    int i;
    int count;
    HWND hwnd;
    TBBUTTON tbButton;
    HICON imgh;

    static button_t *button = NULL;

    if(!button){
        button = (button_t*)malloc(sizeof(button_t)*NUM_TOOLBAR_BUTTONS);
        button[0] = (button_t){IDM_DEBUG_BUTTON_STEP,      _(" Step "),     "step_ico"};
        button[1] = (button_t){IDM_DEBUG_BUTTON_STEP_OVER, _(" Step over "),"stepover_ico"};
        button[2] = (button_t){IDM_DEBUG_BUTTON_RUN,       _(" Run "),      "run_ico"};
        button[3] = (button_t){IDM_DEBUG_BUTTON_LEAVE,     _(" Leave "),    "leave_ico"};
    }

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_TOOL_BAR);

    /* create the image list */
    imgl = ImageList_Create (20, 20, ILC_COLOR32 | ILC_MASK,
                             NUM_TOOLBAR_BUTTONS, 0);

    /* Set the image list */
    SendMessage(hwnd, TB_SETIMAGELIST, (WPARAM)0, (LPARAM)imgl);

    SendMessage(hwnd, CCM_SETVERSION, (WPARAM)1, 0); 
    SendMessage(hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

    /* add icon collection */
    for (i=0; i<NUM_TOOLBAR_BUTTONS; i++)
    {
        if (button[i].image_name[0] != '\0')
        {
            imgh = LoadImage (prog_inst, button[i].image_name,
                              IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
            ImageList_AddIcon (imgl, imgh);
            DestroyIcon(imgh);
        }
    }

    /* create toolbar buttons */
    count = 0;
    for (i=0; i<NUM_TOOLBAR_BUTTONS; i++)
    {
        tbButton.dwData    = -1;
        tbButton.iString   = (INT_PTR)button[i].text;
        tbButton.fsState   = TBSTATE_ENABLED;

        if (button[i].text[0] == '\0')
        {
            tbButton.iBitmap   = 16;
            tbButton.fsStyle = (BYTE)(TBSTYLE_SEP);
            tbButton.idCommand = -1;
        }
        else
        {
            tbButton.iBitmap   = MAKELONG (count++, 0);
            tbButton.fsStyle   = (BYTE)(TBSTYLE_WRAPABLE | TBSTYLE_FLAT);
            tbButton.idCommand = button[i].message_id;
        }
        SendMessage(hwnd, TB_INSERTBUTTON, (WPARAM)i, (LPARAM)&tbButton);
    }
}



/* display_toolbar_button_tooltip:
 *  Display the toolbar button tooltip.
 */
void wdtoolb_DisplayTooltips (LPARAM lParam)
{
    LPTOOLTIPTEXT lpttt;
    UINT_PTR idButton;

    lpttt = (LPTOOLTIPTEXT)lParam;
    lpttt->hinst = prog_inst;
    idButton = lpttt->hdr.idFrom;

    switch (idButton) 
    {
        case IDM_DEBUG_BUTTON_STEP :
            lpttt->lpszText = _("Execute the machine code step by step");
            break;
                
        case IDM_DEBUG_BUTTON_STEP_OVER:
            lpttt->lpszText = _("Execute the machine code step by step\nbut don't jump to sub-programs");
            break;
    }
}

