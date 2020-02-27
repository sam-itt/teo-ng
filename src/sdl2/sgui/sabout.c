/*
  Original code from Hatari, adapted for Teo by Samuel Cuella

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Show information about the program and its license.
*/
#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <assert.h>

#include "dialog.h"
#include "sdlgui.h"
#include "gettext.h"
#include "std.h" 

static char aboutstr[] = PACKAGE_STRING;

/* The "About" dialog: */
#define ABOUTDLG_LEN 19
static SGOBJ *_aboutdlg = NULL;

/*-----------------------------------------------------------------------*/
static SGOBJ *sabout_GetDialog(void)
{

    if(!_aboutdlg){
        int i = 0;
        char *tmp;

        tmp = std_strdup_printf("%s %s", _("Authors:"), "Gilles Fetis - Eric Botcazou");

        _aboutdlg = malloc(sizeof(SGOBJ)*ABOUTDLG_LEN);

        _aboutdlg[i++] = (SGOBJ){ SGBOX, 0, 0, 0,0, 43,25, NULL };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 13,1, 13,1, aboutstr };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 13,2, 14,1, "=============" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,4, 42,1, tmp };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,5, 42,1, "Alex Pukall - Jeremie Guillaume" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,6, 42,1, "Francois Mouret - Samuel Devulder" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,7, 42,1, "Samuel Cuella" };


        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,9,  42,1, "Github: https://github.com/sam-itt/teo-ng" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,11, 42,1, "This program is free software; you can" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,12, 42,1, "redistribute it and/or modify it under" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,13, 42,1, "the terms of the GNU General Public" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,14, 42,1, "License as published by the Free Soft-" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,15, 42,1, "ware Foundation version 2" };

        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,17, 42,1, "This program is distributed in the" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,18, 42,1, "hope that it will be useful, but" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,19, 42,1, "WITHOUT ANY WARRANTY. See the GNU Ge-" };
        _aboutdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 1,20, 42,1, "neral Public License for more details." };
        _aboutdlg[i++] = (SGOBJ){ SGBUTTON, SG_DEFAULT, 0, 16,23, 8,1, "OK" };
        _aboutdlg[i++] = (SGOBJ){ SGSTOP, 0, 0, 0,0, 0,0, NULL };

        assert(i <= ABOUTDLG_LEN);
    }
    return _aboutdlg;
}



/**
 * Show the "about" dialog:
 */
void sabout_Panel(void)
{
    SGOBJ *aboutdlg;

    aboutdlg = sabout_GetDialog();
	if ((int)strlen(aboutstr) > aboutdlg[0].w)
	{
		/* Shorten the name if it is too long */
		char *p = strrchr(aboutstr, '(');
		if (p)
			*(p-1) = 0;
	}
	/* Center the program name title string */
	aboutdlg[1].x = (aboutdlg[0].w - strlen(aboutstr)) / 2;
    aboutdlg[2].x = aboutdlg[1].x;

	SDLGui_CenterDlg(aboutdlg);
	SDLGui_DoDialog(aboutdlg, NULL,false);
}
