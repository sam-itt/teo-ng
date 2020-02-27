/*
  Original code from Hatari, adapted for Teo by Samuel Cuella

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Dialog to load/eject memo7 cartidges 
*/
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <assert.h>

#include "dialog.h"

#include "file.h"
#include "sdlgui.h"

#include "media/memo.h"
#include "errors.h"
#include "std.h"
#include "teo.h"
#include "gettext.h"


#define DLGCART_EJECT 3
#define DLGCART_NAME 4
#define DLGCART_BROWSE 5
#define DLGCART_OK 6
#define DLGCART_LEN 8


static char sFile[FILENAME_MAX];

static SGOBJ *_cartdlg = NULL;

static SGOBJ *smemo_GetDialog(void)
{
    if(!_cartdlg){
        int i = 0;
        _cartdlg = malloc(sizeof(SGOBJ)*DLGCART_LEN);

                               /*type, f,s        x    y    w    h  text */
        _cartdlg[i++] = (SGOBJ){ SGBOX,0,0,       0,  0,   40, 16, NULL };
        _cartdlg[i++] = (SGOBJ){ SGTEXT,0,0,      18,  1,   11,   1, _("Cartridge drive") };

        _cartdlg[i++] = (SGOBJ){ SGTEXT,0,0,      1,  3,   2,   1, "m7" };
        _cartdlg[i++] = (SGOBJ){ SGBUTTON,0,0,    4,  3,   1,   1, "x" };
        _cartdlg[i++] = (SGOBJ){ SGTEXT,0,0,      6,  3,  16,   1, sFile };
        _cartdlg[i++] = (SGOBJ){ SGBUTTON,0,0,    35, 3,   3,   1, "..." };

        _cartdlg[i++] = (SGOBJ){ SGBUTTON,SG_DEFAULT,0,   28, 14, 10,  1,  "_OK" };
        _cartdlg[i++] = (SGOBJ){ SGSTOP, 0, 0, 0,0, 0,0, NULL };

        assert(i <= DLGCART_LEN);
    }
    return _cartdlg;
}


static void smemo_Eject(char *dlgname)
{
    snprintf(dlgname, FILENAME_MAX-1, "%s", "(None)");
    memo_Eject();
    teo.command=TEO_COMMAND_COLD_RESET;
}


static void smemo_Browse(char *dlgname)
{
	char *selname, *zip_path;
	const char *tmpname;

	if (teo.memo.file)
	    tmpname = teo.memo.file;
	else
        tmpname = teo.default_folder ? teo.default_folder : std_getRootPath();

	selname = SDLGui_FileSelect("Cartridge image:", tmpname, &zip_path, false);
	if (!selname)
		return;

	if (File_Exists(selname))
	{
        int rv;

        rv = memo_Load(selname);
        if(rv < 0){
            DlgAlert_Notice(teo_error_msg);
        }else{
            snprintf(dlgname, FILENAME_MAX-1, "%s", std_BaseName(teo.memo.label));
            /*TODO: Integrate this set-from-last-file into a teo_ function
             * see TODO file
             * */
            if(!teo.default_folder){
                teo.default_folder = strdup(selname);
                std_CleanPath (teo.default_folder); /*CleanPath works like dirname(3)*/
            }
            teo.command=TEO_COMMAND_COLD_RESET;
        }   
	}
	else
	{
		dlgname[0] = '\0';
	}
	free(zip_path);
	free(selname);
}



/*-----------------------------------------------------------------------*/
/**
 * Show and process the "Cartridge" dialog
 */
void smemo_Panel(void)
{
	int but;
    SGOBJ *cartdlg;

    cartdlg = smemo_GetDialog();
	SDLGui_CenterDlg(cartdlg);

    /*Current cartridge (if any)*/
    if(!teo.memo.label || *teo.memo.label == '\0')
        snprintf(sFile, FILENAME_MAX-1, "%s", "(None)");
    else
        snprintf(sFile, FILENAME_MAX-1, "%s", std_BaseName(teo.memo.label));

	/* Show the dialog: */
    do{
        but = SDLGui_DoDialog(cartdlg, NULL, false);
		switch(but){
         case DLGCART_BROWSE:
            smemo_Browse(sFile);
            break;
		 case DLGCART_EJECT:
            smemo_Eject(sFile);
			break;
        }
    }while (but != DLGCART_OK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );
}
