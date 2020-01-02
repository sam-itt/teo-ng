/*
  Hatari - dlgCpu.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  This is the CPU settings dialog
*/
//const char DlgCpu_fileid[] = "Hatari dlgCpu.c : " __DATE__ " " __TIME__;

//#include "main.h"
//#include "configuration.h"
#include "dialog.h"
#include "sdlgui.h"
#include "file.h"

static char sFile[FILENAME_MAX];
static char sRealFile[FILENAME_MAX];

#define DLGPRN_DIRECTORY 3
#define DLGPRN_BROWSE 4


#define DLGPRN_PR042 7
#define DLGPRN_PR055 8
#define DLGPRN_PR582 9
#define DLGPRN_PR600 10
#define DLGPRN_PR612 11

#define DLGPRN_RAW 14
#define DLGPRN_TEXT 15
#define DLGPRN_GRAPHIC 16

#define DLGPRN_DOUBLE_SPACED 19
#define DLGPRN_HQ 20

#define DLGPRN_OK 21

static SGOBJ printerdlg[] ={
{ SGBOX,0,0,       0,0,  46,24, NULL },
{ SGTEXT,0,0,     17, 1, 15,1, "Needle printer" },

{ SGTEXT,0,0,      1,  3,   10,   1, "Save into:" },
{ SGTEXT,0,0,     12,  3,  16,   1, sFile },
{ SGBUTTON,0,0,   34,  3,    3,   1, "..." },

/*Printer type*/
{ SGBOX,0,0,       2,  5, 19, 9, NULL },
{ SGTEXT,0,0,      3,  5, 7, 1, "Printer" },
{ SGRADIOBUT,0,0,  3,  7, 8,  1, "PR90-042" },
{ SGRADIOBUT,0,0,  3,  8, 8,  1, "PR90-055" },
{ SGRADIOBUT,0,0,  3,  9, 8,  1, "PR90-582" },
{ SGRADIOBUT,0,0,  3, 10, 8,  1, "PR90-600" },
{ SGRADIOBUT,0,0,  3, 11, 8,  1, "PR90-612" },

/*Ouput*/
{ SGBOX,0,0,      24, 5, 19,9, NULL },
{ SGTEXT,0,0,     25, 5, 12,1, "Output mode" },
{ SGCHECKBOX,0,0, 25, 7, 3, 1, "Raw" },
{ SGCHECKBOX,0,0, 25, 9, 4, 1, "Text" },
{ SGCHECKBOX,0,0, 25, 11, 8, 1, "Graphic" },

/*Settings*/
{ SGBOX,0,0,      2, 15, 42, 5, NULL },
{ SGTEXT,0,0,     3, 15,  9, 1, "Settings" },
{ SGCHECKBOX,0,0, 3, 17, 14, 1, "Double-spaced" },
{ SGCHECKBOX,0,0, 3, 18, 12, 1, "HQ Printing" },


{ SGBUTTON,SG_DEFAULT,0,   34, 22, 10, 1,  "_OK" },

{ SGSTOP, 0, 0, 0,0, 0,0, NULL }
};


/**
 * Let user browse given directory, set directory if one selected.
 * return false if none selected, otherwise return true.
 * @dlgname: name to be used for display in the dialog box
 * @confname: real filename with path
 * @maxlen: space available for display in the dialog box
 *
 */
static bool DlgPrinter_BrowseDir(char *dlgname, char *confname, int maxlen)
{
	char *str, *selname;

	selname = SDLGui_FileSelect("Select directory:", confname, NULL, false);
	if (selname)
	{
		strcpy(confname, selname);
		free(selname);

		str = strrchr(confname, DIR_SEPARATOR);
		if (str != NULL)
			str[1] = 0;
		File_CleanFileName(confname);
		File_ShrinkName(dlgname, confname, maxlen);
		return true;
	}
	return false;
}


/*-----------------------------------------------------------------------*/
/**
 * Show and process the "CPU" dialog
 */
void DlgPrinter_Main(void)
{
	int i;
    int but;

    strncpy(sFile, "5axe.fd", FILENAME_MAX);
	SDLGui_CenterDlg(printerdlg);
	do{
        but = SDLGui_DoDialog(printerdlg, NULL, false);
		switch(but){
            case DLGPRN_BROWSE:
			    DlgPrinter_BrowseDir(sFile, sRealFile, printerdlg[DLGPRN_DIRECTORY].w);
                break;
        }
    }while (but != DLGPRN_OK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );

}
