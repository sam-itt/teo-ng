#include <stdio.h>
#include <assert.h>
/*

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Dialog for setting various system options
*/
//const char DlgTape_fileid[] = "Teo-ng dlgDisks.c : " __DATE__ " " __TIME__;

//#include "main.h"
//#include "configuration.h"
#include "dialog.h"

#include "file.h"
#include "sdlgui.h"




#define DLGTPE_EJECT 4
#define DLGTPE_NAME 5
#define DLGTPE_BROWSE 6
#define DLGTPE_WP 7

#define DLGTPE_IDX_LESS 9
#define DLGTPE_IDX 10
#define DLGTPE_IDX_MORE 11
#define DLGTPE_REWIND 12

#define DLGDSK_OK 13
static char sFile[FILENAME_MAX];
static char sTapeIdx[4];


//x,y base = 20,10
//w,h box !  280,180
static SGOBJ tapedlg[]={
/*type, f,s        x    y    w    h  text */
{ SGBOX,0,0,       0,  0,   50, 11, NULL },
{ SGTEXT,0,0,      18,  1,   11,   1, "Tape drive" },

/*Header*/
{ SGTEXT,0,0,     40,  2,   5, 1,  "prot." },

  /* disk 0 */
{ SGTEXT,0,0,      1,  4,   2,   1, "k7" },
{ SGBUTTON,0,0,    4,  4,   1,   1, "x" },
{ SGTEXT,0,0,      6,  4,  16,   1, sFile },
{ SGBUTTON,0,0,    35, 4,   3,   1, "..." },
{ SGCHECKBOX,0,0,  41, 4,   1,   1, "" },

  /* Counter block */
{ SGTEXT,0,0,      1,  6,  8,    1, "Counter:" },
{ SGBUTTON,0,0,   18,  6,  1,    1, "\x04", SG_SHORTCUT_LEFT },
{ SGTEXT,0,0,     20,  6,  3,    1, sTapeIdx },
{ SGBUTTON,0,0,   24,  6,  1,    1, "\x03", SG_SHORTCUT_RIGHT },
{ SGBUTTON,0,0,   30,  6,  6,    1, "Rewind" },

{ SGBUTTON,SG_DEFAULT,0,   37, 9, 10,  1,  "_OK" },
{ SGSTOP, 0, 0, 0,0, 0,0, NULL }
};



static void DlgTape_Eject(char *dlgname)
{
    /*TODO: Teo eject here*/
    dlgname[0] = '\0';
}

/**
 * Let user browse given tape, insert tape if one selected.
 * @dlgname: pointer to where the text displayed on screen is stored
 * @diskid: id of the dialog object displaying the text (index the window
 * array)
 */
static void DlgTape_Browse(char *dlgname, int diskid)
{
	char *selname, *zip_path;
	const char *tmpname, *realname;

//	if (ConfigureParams.DiskImage.szDiskFileName[drive][0])
//		tmpname = printf("ConfigureParams.DiskImage.szDiskFileName[drive];\n");
//	else
//		tmpname = printf("ConfigureParams.DiskImage.szDiskImageDirectory;\n");
		tmpname = "/home";

	selname = SDLGui_FileSelect("Tape image:", tmpname, &zip_path, false);
	if (!selname)
		return;

	if (File_Exists(selname))
	{
        realname = strdup(selname);
        /*TODO: TEO Set floppy HERE*/
		printf("realname = Floppy_SetDiskFileName(drive, selname, zip_path);\n");
		if (realname)
			File_ShrinkName(dlgname, realname, tapedlg[diskid].w);
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
 * Show and process the "Tape" dialog
 */
void DlgTape_Main(void)
{
	int i;
	int but;
    int index;

//    *sFileOne = '\0';
    strncpy(sFile, "5axe.fd", FILENAME_MAX);
    index = 0;
	sprintf(sTapeIdx, "%3i", index);

	SDLGui_CenterDlg(tapedlg);

    /*Here get disks files from TEO config and put them in
     * also check/uncheck write protection
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
        but = SDLGui_DoDialog(tapedlg, NULL, false);
//        printf("But is : %d\n",but);
		switch(but){
         /* Choose a new disk*/
		 case DLGTPE_BROWSE:                        
			DlgTape_Browse(sFile, DLGTPE_NAME);
            tapedlg[DLGTPE_WP].state |= SG_SELECTED;
			break;
		 case DLGTPE_EJECT:
            DlgTape_Eject(sFile);
            tapedlg[DLGTPE_EJECT].state &= ~SG_SELECTED;
			break;
		 case DLGTPE_IDX_MORE:
            if(index < 100)
                index++;
			sprintf(sTapeIdx, "%3i", index);
			break;
		 case DLGTPE_IDX_LESS:
            if(index > 0)
                index--;
			sprintf(sTapeIdx, "%3i", index);
			break;
         case DLGTPE_REWIND:
            index = 0;
			sprintf(sTapeIdx, "%3i", index);
			break;
        }
    }while (but != DLGDSK_OK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );
}
