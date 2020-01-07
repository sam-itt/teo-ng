/*
  Original code from Hatari, adapted for Teo

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  The main dialog.
*/

#include "dialog.h"
#include "sdlgui.h"
#include "screen.h"

#include "teo.h"

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


/* The main dialog: */
static SGOBJ maindlg[] =
{ 
	{ SGBOX, 0, 0,     0, 0, 37,21, NULL },
	{ SGTEXT, 0, 0,   11, 1, 16,1, "Control Panel" },

	{ SGBUTTON, 0, 0,  2, 3, 16,1, "_Warm reset" },
	{ SGBUTTON, 0, 0, 19, 3, 16,1, "_Cold reset" },
	{ SGBUTTON, 0, 0, 12, 5, 13,1, "_Full reset" },

	{ SGBUTTON, 0, 0,  2, 8,  33,1, "_Settings" },
	{ SGBUTTON, 0, 0,  2, 10,  33,1, "_Disk drives" },
	{ SGBUTTON, 0, 0,  2, 12, 33,1, "_Tape drive" },
	{ SGBUTTON, 0, 0,  2, 14, 33,1, "Ca_rtridge" },
	{ SGBUTTON, 0, 0,  2, 16, 33,1, "_Printer" },

	{ SGBUTTON, SG_CANCEL, 0,  2, 19, 10,1, "_Quit" },
	{ SGBUTTON, 0, 0, 13, 19, 11,1, "A_bout" },
	{ SGBUTTON, SG_DEFAULT, 0, 25, 19, 10,1, "_OK" },
	{ SGSTOP, 0, 0, 0,0, 0,0, NULL }
};


/**
 * This functions sets up the actual font and then displays the main dialog.
 */
int Dialog_MainDlg(bool adjustableVolume, int da_mask)
{
	int retbut, response;
	bool bOldMouseVisibility;
	int nOldMouseX, nOldMouseY;

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
            break;
		 case MAINDLG_COLD_RESET:
            teo.command=TEO_COMMAND_COLD_RESET;
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
	//printf("Main_WarpMouse(nOldMouseX, nOldMouseY, true);\n");

	return (retbut == MAINDLG_OK);
}
