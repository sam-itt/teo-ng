#include <stdio.h>
#include <assert.h>
/*

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Dialog for setting various system options
*/
//const char DlgDisks_fileid[] = "Teo-ng dlgDisks.c : " __DATE__ " " __TIME__;

//#include "main.h"
//#include "configuration.h"
#include "dialog.h"

#include "file.h"
#include "sdlgui.h"

#define DLGCART_EJECT 3
#define DLGCART_NAME 4
#define DLGCART_BROWSE 5
#define DLGCART_OK 6


static char sFile[FILENAME_MAX];


//x,y base = 20,10
//w,h box !  280,180
static SGOBJ cartdlg[]={
/*type, f,s        x    y    w    h  text */
{ SGBOX,0,0,       0,  0,   40, 16, NULL },
{ SGTEXT,0,0,      18,  1,   11,   1, "Cartridge drive" },

{ SGTEXT,0,0,      1,  3,   2,   1, "m7" },
{ SGBUTTON,0,0,    4,  3,   1,   1, "x" },
{ SGTEXT,0,0,      6,  3,  16,   1, sFile },
{ SGBUTTON,0,0,    35, 3,   3,   1, "..." },

{ SGBUTTON,SG_DEFAULT,0,   28, 14, 10,  1,  "_OK" },
{ SGSTOP, 0, 0, 0,0, 0,0, NULL }
};



static void DlgCart_Eject(char *dlgname)
{
    /*TODO: Teo eject here*/

    dlgname[0] = '\0';
}


static void DlgCart_Browse(char *dlgname, int diskid)
{
	char *selname, *zip_path;
	const char *tmpname, *realname;

//	if (ConfigureParams.DiskImage.szDiskFileName[drive][0])
//		tmpname = printf("ConfigureParams.DiskImage.szDiskFileName[drive];\n");
//	else
//		tmpname = printf("ConfigureParams.DiskImage.szDiskImageDirectory;\n");
		tmpname = "/home";

	selname = SDLGui_FileSelect("Cartridge image:", tmpname, &zip_path, false);
	if (!selname)
		return;

	if (File_Exists(selname))
	{
        realname = strdup(selname);
        /*TODO: TEO Set floppy HERE*/
		printf("realname = Floppy_SetDiskFileName(drive, selname, zip_path);\n");
		if (realname)
			File_ShrinkName(dlgname, realname, cartdlg[diskid].w);
	}
	else
	{
        /*TODO: TEO Clear floppy HERE*/
		printf("Floppy_SetDiskFileNameNone(drive);\n");
		dlgname[0] = '\0';
	}
	free(zip_path);
	free(selname);
}



/*-----------------------------------------------------------------------*/
/**
 * Show and process the "Cartridge" dialog
 */
void DlgCart_Main(void)
{
	int i;
	int but;
    int volume;

//    *sFileOne = '\0';
    strncpy(sFile, "5axe.fd", FILENAME_MAX);
    
	SDLGui_CenterDlg(cartdlg);

    /*Here get cartridge files from TEO config and put it in
     *
     * */
//	/* Set up speed */
//    systemdlg[DLGSET_SPD_EXACT].state &= ~SG_SELECTED;
//    systemdlg[DLGSET_SPD_FAST].state &= ~SG_SELECTED;
//
//    systemdlg[DLGSET_SPD_EXACT].state |= SG_SELECTED;
//
//    /*Sound*/
//    systemdlg[DLGSET_SOUND].state &= ~SG_SELECTED;
//    volume = 100;
//	sprintf(sSoundVolume, "%3i", volume);
//
//    systemdlg[DLGSET_SOUND].state |= SG_SELECTED;
//
//
//    /*Memory*/
//    systemdlg[DLGSET_MEM_256].state &= ~SG_SELECTED;
//    systemdlg[DLGSET_MEM_512].state &= ~SG_SELECTED;
//
//    systemdlg[DLGSET_MEM_512].state |= SG_SELECTED;
//
//    /*Video*/
//    systemdlg[DLGSET_INTL_VID].state &= ~SG_SELECTED;

	/* Show the dialog: */
	do{
        but = SDLGui_DoDialog(cartdlg, NULL, false);
		switch(but){
         case DLGCART_BROWSE:
            DlgCart_Browse(sFile, DLGCART_NAME);
            break;
		 case DLGCART_EJECT:
            DlgCart_Eject(sFile);
			break;
        }
    }while (but != DLGCART_OK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );
}
