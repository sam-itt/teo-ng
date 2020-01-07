/*
  Original code from Hatari, adapted for Teo

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Show information about the program and its license.
*/
#include "config.h"

#include "dialog.h"
#include "sdlgui.h"

static char aboutstr[] = PACKAGE_STRING;

/* The "About"-dialog: */
static SGOBJ aboutdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 43,25, NULL },
	{ SGTEXT, 0, 0, 13,1, 13,1, aboutstr },
	{ SGTEXT, 0, 0, 13,2, 14,1, "=============" },
	{ SGTEXT, 0, 0, 1,4, 42,1, "Authors: Gilles Fetis - Eric Botcazou" },
	{ SGTEXT, 0, 0, 1,5, 42,1, "Alex Pukall - Jeremie Guillaume" },
	{ SGTEXT, 0, 0, 1,6, 42,1, "Francois Mouret - Samuel Devulder" },
	{ SGTEXT, 0, 0, 1,7, 42,1, "Samuel Cuella" },


	{ SGTEXT, 0, 0, 1,9,  42,1, "Github: https://github.com/sam-itt/teo-ng" },
	{ SGTEXT, 0, 0, 1,11, 42,1, "This program is free software; you can" },
	{ SGTEXT, 0, 0, 1,12, 42,1, "redistribute it and/or modify it under" },
	{ SGTEXT, 0, 0, 1,13, 42,1, "the terms of the GNU General Public" },
	{ SGTEXT, 0, 0, 1,14, 42,1, "License as published by the Free Soft-" },
	{ SGTEXT, 0, 0, 1,15, 42,1, "ware Foundation version 2" },

	{ SGTEXT, 0, 0, 1,17, 42,1, "This program is distributed in the" },
	{ SGTEXT, 0, 0, 1,18, 42,1, "hope that it will be useful, but" },
	{ SGTEXT, 0, 0, 1,19, 42,1, "WITHOUT ANY WARRANTY. See the GNU Ge-" },
	{ SGTEXT, 0, 0, 1,20, 42,1, "neral Public License for more details." },
	{ SGBUTTON, SG_DEFAULT, 0, 16,23, 8,1, "OK" },
	{ SGSTOP, 0, 0, 0,0, 0,0, NULL }
};


/*-----------------------------------------------------------------------*/
/**
 * Show the "about" dialog:
 */
void Dialog_AboutDlg(void)
{
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
