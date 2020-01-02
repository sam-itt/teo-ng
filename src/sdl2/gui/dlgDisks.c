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




#define DLGDSK_0_EJECT 5
#define DLGDSK_0_NAME 6
#define DLGDSK_0_SIDE 7
#define DLGDSK_0_BROWSE 8
#define DLGDSK_0_WPROT 9

#define DLGDSK_1_EJECT 11
#define DLGDSK_1_NAME 12
#define DLGDSK_1_SIDE 13
#define DLGDSK_1_BROWSE 14
#define DLGDSK_1_WPROT 15

#define DLGDSK_2_EJECT 17
#define DLGDSK_2_NAME 18
#define DLGDSK_2_SIDE 19
#define DLGDSK_2_BROWSE 20
#define DLGDSK_2_WPROT 21

#define DLGDSK_3_EJECT 23
#define DLGDSK_3_NAME 24
#define DLGDSK_3_SIDE 25
#define DLGDSK_3_BROWSE 26
#define DLGDSK_3_WPROT 27

#define DLGDSK_DIRECT_ACCESS 28
#define DLGDSK_OK 29

#define MAX_FLOPPYDRIVES 4

static char sFiles[4][FILENAME_MAX];


//x,y base = 20,10
//w,h box !  280,180
static SGOBJ diskdlg[]={
/*type, f,s        x    y    w    h  text */
{ SGBOX,0,0,       0,  0,   50, 16, NULL },
{ SGTEXT,0,0,      18,  1,   11,   1, "Disk drives" },

/*Header*/
{ SGTEXT,0,0,     30,  2,   4, 1,  "side" },
{ SGTEXT,0,0,     40,  2,   5, 1,  "prot." },

  /* disk 0 */
{ SGTEXT,0,0,      1,  4,   2,   1, "0:" },
{ SGBUTTON,0,0,    4,  4,   1,   1, "x" },
{ SGTEXT,0,0,      6,  4,  16,   1, sFiles[0] },
{ SGBUTTON,0,0,    32, 4,   1,   1, "0" },
{ SGBUTTON,0,0,    35, 4,   3,   1, "..." },
{ SGCHECKBOX,0,0,  41, 4,   1,   1, "" },
  /* disk 1 */
{ SGTEXT,0,0,      1,  6,   2,   1, "1:" },
{ SGBUTTON,0,0,    4,  6,   1,   1, "x" },
{ SGTEXT,0,0,      6,  6,  16,   1, sFiles[1] },
{ SGBUTTON,0,0,    32, 6,   1,   1, "0" },
{ SGBUTTON,0,0,    35, 6,   3,   1, "..." },
{ SGCHECKBOX,0,0,  41, 6,   1,   1, "" },
  /* disk 2 */
{ SGTEXT,0,0,      1,  8,   2,   1, "2:" },
{ SGBUTTON,0,0,    4,  8,   1,   1, "x" },
{ SGTEXT,0,0,      6,  8,  16,   1, sFiles[2] },
{ SGBUTTON,0,0,    32, 8,   1,   1, "0" },
{ SGBUTTON,0,0,    35, 8,   3,   1, "..." },
{ SGCHECKBOX,0,0,  41, 8,   1,   1, "" },

  /* disk 3 */
{ SGTEXT,0,0,      1,  10,   2,   1, "3:" },
{ SGBUTTON,0,0,    4,  10,   1,   1, "x" },
{ SGTEXT,0,0,      6,  10,  16,   1, sFiles[3] },
{ SGBUTTON,0,0,    32, 10,   1,   1, "0" },
{ SGBUTTON,0,0,    35, 10,   3,   1, "..." },
{ SGCHECKBOX,0,0,  41, 10,   1,   1, "" },

  /* direct disk */
{ SGBUTTON,0,0,     1, 12,  46,  1, "_Direct access" },

{ SGBUTTON,SG_DEFAULT,0,   37, 14, 10,  1,  "_OK" },
{ SGSTOP, 0, 0, 0,0, 0,0, NULL }
};



static void DlgDisks_EjectDisk(char *dlgname, int drive)
{
	assert(drive >= 0 && drive < MAX_FLOPPYDRIVES);
    /*TODO: Teo eject here*/

    dlgname[0] = '\0';
}

/**
 * Let user browse given disk, insert disk if one selected.
 * @dlgname: pointer to where the text displayed on screen is stored
 * @drive: drive index
 * @diskid: id of the dialog object displaying the text (index the window
 * array)
 */
static void DlgDisks_BrowseDisk(char *dlgname, int drive, int diskid)
{
	char *selname, *zip_path;
	const char *tmpname, *realname;

	assert(drive >= 0 && drive < MAX_FLOPPYDRIVES);
//	if (ConfigureParams.DiskImage.szDiskFileName[drive][0])
//		tmpname = printf("ConfigureParams.DiskImage.szDiskFileName[drive];\n");
//	else
//		tmpname = printf("ConfigureParams.DiskImage.szDiskImageDirectory;\n");
		tmpname = "/home";

	selname = SDLGui_FileSelect("Floppy image:", tmpname, &zip_path, false);
	if (!selname)
		return;

	if (File_Exists(selname))
	{
        realname = strdup(selname);
        /*TODO: TEO Set floppy HERE*/
		printf("realname = Floppy_SetDiskFileName(drive, selname, zip_path);\n");
		if (realname)
			File_ShrinkName(dlgname, realname, diskdlg[diskid].w);
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
 * Show and process the "Disks" dialog
 */
void DlgDisks_Main(void)
{
	int i;
	int but;
    int volume;

//    *sFileOne = '\0';
    strncpy(sFiles[0], "5axe.fd", FILENAME_MAX);
    strncpy(sFiles[1], "6axe.fd", FILENAME_MAX);
    strncpy(sFiles[2], "7axe.fd", FILENAME_MAX);
    strncpy(sFiles[3], "8axe.fd", FILENAME_MAX);
    
	SDLGui_CenterDlg(diskdlg);

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
        but = SDLGui_DoDialog(diskdlg, NULL, false);
//        printf("But is : %d\n",but);
		switch(but){
         /* Choose a new disk*/
		 case DLGDSK_0_BROWSE:                        
			DlgDisks_BrowseDisk(sFiles[0], 0, DLGDSK_0_NAME);
            diskdlg[DLGDSK_0_WPROT].state |= SG_SELECTED;
			break;
		 case DLGDSK_1_BROWSE:                     
			DlgDisks_BrowseDisk(sFiles[1], 1, DLGDSK_1_NAME);
            diskdlg[DLGDSK_1_WPROT].state |= SG_SELECTED;
			break;
		 case DLGDSK_2_BROWSE:                    
			DlgDisks_BrowseDisk(sFiles[2], 2, DLGDSK_2_NAME);
            diskdlg[DLGDSK_2_WPROT].state |= SG_SELECTED;
			break;
		 case DLGDSK_3_BROWSE:                   
			DlgDisks_BrowseDisk(sFiles[3], 3, DLGDSK_3_NAME);
            diskdlg[DLGDSK_3_WPROT].state |= SG_SELECTED;
			break;
		 case DLGDSK_0_EJECT:
            DlgDisks_EjectDisk(sFiles[0], 0);
            diskdlg[DLGDSK_0_WPROT].state &= ~SG_SELECTED;
			break;
		 case DLGDSK_1_EJECT:
            DlgDisks_EjectDisk(sFiles[1], 1);
            diskdlg[DLGDSK_1_WPROT].state &= ~SG_SELECTED;
			break;
		 case DLGDSK_2_EJECT:
            DlgDisks_EjectDisk(sFiles[2], 2);
            diskdlg[DLGDSK_2_WPROT].state &= ~SG_SELECTED;
			break;
		 case DLGDSK_3_EJECT:
            DlgDisks_EjectDisk(sFiles[3], 3);
            diskdlg[DLGDSK_3_WPROT].state &= ~SG_SELECTED;
			break;


//		 case DLGSET_VOL_MORE:
//            if(volume < 100)
//                volume++;
//			sprintf(sSoundVolume, "%3i", volume);
//			break;
//		 case DLGSET_VOL_LESS:
//            if(volume > 0)
//                volume--;
//			sprintf(sSoundVolume, "%3i", volume);
//			break;
        }
    }while (but != DLGDSK_OK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );
}
