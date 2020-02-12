/*
  Original code from Hatari, adapted for Teo by Samuel Cuella

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  This is the Printer settings dialog
*/
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <assert.h>

#include "dialog.h"
#include "sdlgui.h"
#include "file.h"

#include "std.h"
#include "teo.h"
#include "media/printer.h"
#include "gettext.h"


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
#define DLGPRN_LEN 23

static SGOBJ *_printerdlg = NULL;

static SGOBJ *sprinter_GetDialog(void)
{
    if(!_printerdlg){
        int i = 0;
        _printerdlg = malloc(sizeof(SGOBJ)*DLGPRN_LEN);

        _printerdlg[i++] = (SGOBJ){ SGBOX,0,0,       0,0,  46,24, NULL };
        _printerdlg[i++] = (SGOBJ){ SGTEXT,0,0,     17, 1, 15,1, _("Dot-matrix printer") };

        _printerdlg[i++] = (SGOBJ){ SGTEXT,0,0,      1,  3,   10,   1, _("Save into:") };
        _printerdlg[i++] = (SGOBJ){ SGTEXT,0,0,     12,  3,  16,   1, sFile };
        _printerdlg[i++] = (SGOBJ){ SGBUTTON,0,0,   34,  3,    3,   1, "..." };

        /*Printer type*/
        _printerdlg[i++] = (SGOBJ){ SGBOX,0,0,       2,  5, 19, 9, NULL };
        _printerdlg[i++] = (SGOBJ){ SGTEXT,0,0,      3,  5, 7, 1, _("Printer") };
        _printerdlg[i++] = (SGOBJ){ SGRADIOBUT,0,0,  3,  7, 8,  1, "PR90-042" };
        _printerdlg[i++] = (SGOBJ){ SGRADIOBUT,0,0,  3,  8, 8,  1, "PR90-055" };
        _printerdlg[i++] = (SGOBJ){ SGRADIOBUT,0,0,  3,  9, 8,  1, "PR90-582" };
        _printerdlg[i++] = (SGOBJ){ SGRADIOBUT,0,0,  3, 10, 8,  1, "PR90-600" };
        _printerdlg[i++] = (SGOBJ){ SGRADIOBUT,0,0,  3, 11, 8,  1, "PR90-612" };

        /*Ouput*/
        _printerdlg[i++] = (SGOBJ){ SGBOX,0,0,      24, 5, 19,9, NULL };
        _printerdlg[i++] = (SGOBJ){ SGTEXT,0,0,     25, 5, 12,1, _("Output mode") };
        _printerdlg[i++] = (SGOBJ){ SGCHECKBOX,0,0, 25, 7, 3, 1, _("Raw") };
        _printerdlg[i++] = (SGOBJ){ SGCHECKBOX,0,0, 25, 9, 4, 1, _("Text") };
        _printerdlg[i++] = (SGOBJ){ SGCHECKBOX,0,0, 25, 11, 8, 1, _("Graphic") };

        /*Settings*/
        _printerdlg[i++] = (SGOBJ){ SGBOX,0,0,      2, 15, 42, 5, NULL };
        _printerdlg[i++] = (SGOBJ){ SGTEXT,0,0,     3, 15,  9, 1, _("Settings") };
        _printerdlg[i++] = (SGOBJ){ SGCHECKBOX,0,0, 3, 17, 14, 1, _("Double spacing") };
        _printerdlg[i++] = (SGOBJ){ SGCHECKBOX,0,0, 3, 18, 12, 1, _("HQ Printing") };


        _printerdlg[i++] = (SGOBJ){ SGBUTTON,SG_DEFAULT,0,   34, 22, 10, 1,  "_OK" };

        _printerdlg[i++] = (SGOBJ){ SGSTOP, 0, 0, 0,0, 0,0, NULL };
        assert(i <= DLGPRN_LEN);
    }
    return _printerdlg;
}


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
	char *selname;
    int i;

	selname = SDLGui_FileSelect("Select directory:", confname, NULL, false);
	if(selname){
        /*Removes any trailing DIR_SEPARATOR (slash on Unix, backslash on Win32)*/
        i = strlen(selname) - 1;
        if (i > 0)
            while ((i >= 0) && (selname[i] == DIR_SEPARATOR))
                selname[i--] = '\0';

        /*Set full path in the config strcut*/
        teo.lprt.folder = std_free(teo.lprt.folder);
        teo.lprt.folder = selname;

        /*Displays only the last part in the dialog box*/
        snprintf(sFile, FILENAME_MAX-1, "%s", std_BaseName(selname));

		return true;
	}
	return false;

}


/*-----------------------------------------------------------------------*/
/**
 * Show and process the "Printer" dialog
 */
void DlgPrinter_Main(void)
{
	int i;
    int but;
    int selected_printer_idx;
    SGOBJ *printerdlg;

    printerdlg = sprinter_GetDialog();
	SDLGui_CenterDlg(printerdlg);

    /*Output folder*/
    if (teo.lprt.folder == NULL){
        teo.lprt.folder = get_current_dir_name(); /*TODO: Move this in the core*/
    }
    snprintf(sFile, FILENAME_MAX-1, "%s", teo.lprt.folder);
    std_CleanPath(sFile);

    /*Double spaced*/
    printerdlg[DLGPRN_DOUBLE_SPACED].state &= ~SG_SELECTED;
    if (teo.lprt.dip)
        printerdlg[DLGPRN_DOUBLE_SPACED].state |= SG_SELECTED;

    /*High quality*/
    printerdlg[DLGPRN_HQ].state &= ~SG_SELECTED;
    if (teo.lprt.nlq)
        printerdlg[DLGPRN_HQ].state |= SG_SELECTED;

    /*Output settings*/
    printerdlg[DLGPRN_RAW].state &= ~SG_SELECTED;
    if (teo.lprt.raw_output)
        printerdlg[DLGPRN_RAW].state |= SG_SELECTED;

    printerdlg[DLGPRN_TEXT].state &= ~SG_SELECTED;
    if (teo.lprt.txt_output)
        printerdlg[DLGPRN_TEXT].state |= SG_SELECTED;

    printerdlg[DLGPRN_GRAPHIC].state &= ~SG_SELECTED;
    if (teo.lprt.gfx_output)
        printerdlg[DLGPRN_GRAPHIC].state |= SG_SELECTED;

    /*Printer selection*/
    printerdlg[DLGPRN_PR042].state &= ~SG_SELECTED;
    printerdlg[DLGPRN_PR055].state &= ~SG_SELECTED;
    printerdlg[DLGPRN_PR582].state &= ~SG_SELECTED;
    printerdlg[DLGPRN_PR600].state &= ~SG_SELECTED;
    printerdlg[DLGPRN_PR612].state &= ~SG_SELECTED;

    selected_printer_idx = 0;
    for (i=0; i<PRINTER_NUMBER; i++){
        if (teo.lprt.number == printer_code_list[i].number)
            selected_printer_idx = i;
    }
    printerdlg[DLGPRN_PR042+selected_printer_idx].state |= SG_SELECTED;

	do{
        but = SDLGui_DoDialog(printerdlg, NULL, false);
		switch(but){
            case DLGPRN_BROWSE:
			    DlgPrinter_BrowseDir(sFile, sRealFile, printerdlg[DLGPRN_DIRECTORY].w);
                break;
        }
    }while (but != DLGPRN_OK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );

    if(but != DLGPRN_OK) return;

    /*Adjust config to selected values*/

    /*Selected printer*/
    for(int i = DLGPRN_PR042; i <= DLGPRN_PR612; i++){
        if(get_state(printerdlg[i])){
            teo.lprt.number = printer_code_list[i-DLGPRN_PR042].number;
        }
    }
    
    /*Double interval*/
    teo.lprt.dip = get_state(printerdlg[DLGPRN_DOUBLE_SPACED]);
    
    /*Quality*/
    teo.lprt.nlq = get_state(printerdlg[DLGPRN_HQ]);

    /*Output type*/ 
    teo.lprt.raw_output = get_state(printerdlg[DLGPRN_RAW]);
    teo.lprt.txt_output = get_state(printerdlg[DLGPRN_TEXT]);
    teo.lprt.gfx_output = get_state(printerdlg[DLGPRN_GRAPHIC]);

}
