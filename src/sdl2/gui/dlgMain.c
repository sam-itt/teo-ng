/*
  Original code from Hatari, adapted for Teo by Samuel Cuella

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  The main dialog.
*/
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include "dialog.h"
#include "sdlgui.h"
#include "screen.h"

#include "teo.h"
#include "gettext.h"

#define MAINDLG_WARM_RESET 2
#define MAINDLG_COLD_RESET 3
#define MAINDLG_FULL_RESET 4

#define MAINDLG_SETTINGS 5
#define MAINDLG_DISKS 6
#define MAINDLG_TAPE 7
#define MAINDLG_CARTRIDGE 8
#define MAINDLG_PRINTER 9

#define MAINDLG_QUIT 10
#define MAINDLG_ABOUT 11
#define MAINDLG_OK 12
#define MAINDLG_LEN 14 

static SGOBJ *_maindlg = NULL;

static SGOBJ *sgui_GetDialog(void)
{
    if(!_maindlg){
        int i = 0;
        _maindlg = malloc(sizeof(SGOBJ)*MAINDLG_LEN);

        _maindlg[i++] = (SGOBJ){ SGBOX, 0, 0,     0, 0, 37,21, NULL };
        _maindlg[i++] = (SGOBJ){ SGTEXT, 0, 0,   11, 1, 16,1, _("Control panel") };

        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0,  2, 3, 16,1, _("_Warm reset") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0, 19, 3, 16,1, _("_Cold reset") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0, 12, 5, 13,1, _("_Full reset") };

        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0,  2, 8,  33,1, _("_Settings") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0,  2, 10,  33,1, _("_Disk drives") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0,  2, 12, 33,1, _("_Tape drive") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0,  2, 14, 33,1, _("Ca_rtridge") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0,  2, 16, 33,1, _("_Printer") };

        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0,  2, 19, 10,1, _("_Quit") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, 0, 0, 13, 19, 11,1, _("A_bout") };
        _maindlg[i++] = (SGOBJ){ SGBUTTON, SG_DEFAULT|SG_CANCEL, 0, 25, 19, 10,1, "_OK" };
        _maindlg[i++] = (SGOBJ){ SGSTOP, 0, 0, 0,0, 0,0, NULL };

        assert(i <= MAINDLG_LEN);
    }
    return _maindlg;
}


/**
 * This functions sets up the actual font and then displays the main dialog.
 */
int Dialog_MainDlg(bool adjustableVolume, int da_mask)
{
	int retbut, response;
	bool bOldMouseVisibility;
	int nOldMouseX, nOldMouseY;
    SGOBJ *maindlg;

    maindlg = sgui_GetDialog();
    
	SDL_GetMouseState(&nOldMouseX, &nOldMouseY);
	bOldMouseVisibility = SDL_ShowCursor(SDL_QUERY);
	SDL_ShowCursor(SDL_ENABLE);

	SDLGui_CenterDlg(maindlg);

	do{
		retbut = SDLGui_DoDialog(maindlg, NULL, false);
		switch (retbut){
         /*Resets*/
		 case MAINDLG_WARM_RESET:
            teo.command=TEO_COMMAND_RESET;
            retbut = MAINDLG_OK;
            break;
		 case MAINDLG_COLD_RESET:
            teo.command=TEO_COMMAND_COLD_RESET;
            retbut = MAINDLG_OK;
            break;
		 case MAINDLG_FULL_RESET:
            response = DlgAlert_Query("All the memory will be cleared.");
            if(response == true){
                teo.command=TEO_COMMAND_FULL_RESET;
                return true;
            }
            break;
         /*Other dialogs*/
         case MAINDLG_SETTINGS:
            DlgSystem_Main(adjustableVolume); 
            break;
         case MAINDLG_DISKS:
            DlgDisks_Main(da_mask); 
            break;
         case MAINDLG_TAPE:
            DlgTape_Main();
            break;
         case MAINDLG_CARTRIDGE:
            DlgCart_Main();
            break;
         case MAINDLG_PRINTER:
            DlgPrinter_Main();
            break;
         /*Other actions*/
         case MAINDLG_QUIT:
            if(teo.command == TEO_COMMAND_COLD_RESET)
                teo_ColdReset();
            teo.command=TEO_COMMAND_QUIT;
            retbut = MAINDLG_OK;
//			bQuitProgram = true;
            break;
         case MAINDLG_ABOUT:
            Dialog_AboutDlg();
            break;
		}
	}
	while (retbut != MAINDLG_OK && retbut != SDLGUI_QUIT
	        && retbut != SDLGUI_ERROR && !bQuitProgram);

	SDL_ShowCursor(bOldMouseVisibility);
	//log_msgf(LOG_TRACE,"Main_WarpMouse(nOldMouseX, nOldMouseY, true);\n");

	return (retbut == MAINDLG_OK);
}
