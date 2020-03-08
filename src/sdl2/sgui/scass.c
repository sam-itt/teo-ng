/*
  Original code from Hatari, adapted for Teo by Samuel Cuella

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Dialog to load/eject magnetic tape images 
*/
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <assert.h>

#include "dialog.h"
#include "file.h"
#include "sdlgui.h"

#include "media/cass.h"
#include "errors.h"
#include "std.h"
#include "teo.h"
#include "logsys.h"
#include "gettext.h"



#define DLGTPE_EJECT 4
#define DLGTPE_NAME 5
#define DLGTPE_BROWSE 6
#define DLGTPE_WP 7

#define DLGTPE_IDX_LESS 9
#define DLGTPE_IDX 10
#define DLGTPE_IDX_MORE 11
#define DLGTPE_REWIND 12

#define DLGDSK_OK 13
#define DLGTPE_LEN 15

/*Borrowed from Allegro4*/
#ifndef MID
/* Returns the median of x, y, z */
#define MID(x,y,z)   ((x) > (y) ? ((y) > (z) ? (y) : ((x) > (z) ?    \
                       (z) : (x))) : ((y) > (z) ? ((z) > (x) ? (z) : \
                       (x)): (y)))
#endif

static char sFile[FILENAME_MAX];

/*Tape counter*/
#define COUNTER_MAX  999
static char sTapeIdx[4];

/*Write protect*/
static bool wp_active;

//x,y base = 20,10
//w,h box !  280,180
static SGOBJ *_tapedlg = NULL;

static SGOBJ *scass_GetDialog(void)
{
    if(!_tapedlg){
        int i = 0;
        _tapedlg = malloc(sizeof(SGOBJ)*DLGTPE_LEN);

                                 /*type, f,s        x    y    w    h  text */
        _tapedlg[i++] = (SGOBJ){SGBOX,0,0,       0,  0,   50, 11, NULL };
        _tapedlg[i++] = (SGOBJ){SGTEXT,0,0,      18,  1,   11,   1, _("Tape drive") };

        /*Header*/
        _tapedlg[i++] = (SGOBJ){SGTEXT,0,0,     40,  2,   5, 1,  _("prot.") };

          /* disk 0 */
        _tapedlg[i++] = (SGOBJ){SGTEXT,0,0,      1,  4,   2,   1, "k7" };
        _tapedlg[i++] = (SGOBJ){SGBUTTON,0,0,    4,  4,   1,   1, "x" };
        _tapedlg[i++] = (SGOBJ){SGTEXT,0,0,      6,  4,  16,   1, sFile };
        _tapedlg[i++] = (SGOBJ){SGBUTTON,0,0,    35, 4,   3,   1, "..." };
        _tapedlg[i++] = (SGOBJ){SGCHECKBOX,0,0,  41, 4,   1,   1, "" };

          /* Counter block */
        _tapedlg[i++] = (SGOBJ){SGTEXT,0,0,      1,  6,  8,    1, _("Counter:") };
        _tapedlg[i++] = (SGOBJ){SGBUTTON,0,0,   18,  6,  1,    1, "\x04", SG_SHORTCUT_LEFT };
        _tapedlg[i++] = (SGOBJ){SGTEXT,0,0,     20,  6,  3,    1, sTapeIdx };
        _tapedlg[i++] = (SGOBJ){SGBUTTON,0,0,   24,  6,  1,    1, "\x03", SG_SHORTCUT_RIGHT };
        _tapedlg[i++] = (SGOBJ){SGBUTTON,0,0,   30,  6,  strlen(_("Rewind")),    1, _("Rewind") };

        _tapedlg[i++] = (SGOBJ){SGBUTTON,SG_DEFAULT,0,   37, 9, 10,  1,  "_OK" };
        _tapedlg[i++] = (SGOBJ){SGSTOP, 0, 0, 0,0, 0,0, NULL };

        assert(i <= DLGTPE_LEN);
    }
    return _tapedlg;
}



static void scass_Eject(char *dlgname)
{
    snprintf(dlgname, FILENAME_MAX-1, "%s", _("(None)"));
    cass_Eject();
}

/**
 * Let user browse given tape, insert tape if one selected.
 * @dlgname: pointer to where the text displayed on screen is stored
 * @diskid: id of the dialog object displaying the text (index the window
 * array)
 */
static void scass_Browse(char *dlgname, int diskid)
{
	char *selname, *zip_path;
	const char *tmpname;
    SGOBJ *tapedlg;

    tapedlg = scass_GetDialog();

	if (teo.memo.file)
	    tmpname = teo.cass.file;
	else
        tmpname = teo.default_folder ? teo.default_folder : std_getRootPath();

	selname = SDLGui_FileSelect(_("Tape image:"), tmpname, &zip_path, false);
	if (!selname)
		return;

	if(File_Exists(selname)){
        int rv;
        
        /* Will try to open filename (obeys teo.cass.write_protect)
         * and will set teo.cass.filename*/
        rv = cass_Load(selname); 
        if(rv < 0){
            DlgAlert_Notice(teo_error_msg);
        }else{
            snprintf(dlgname, FILENAME_MAX-1, "%s", std_BaseName(teo.cass.file));

            /*TODO: Integrate this set-default-from-last-file into a teo_ function*/
            if(!teo.default_folder){
                teo.default_folder = strdup(selname);
                std_CleanPath (teo.default_folder); /*CleanPath works like dirname(3)*/
            }
            
            /* cass_Load returned true and set teo.cass.write_protect 
             * while the toggle wasn't checked: Can't write but not 
             * because the user asked so 
             * */
            if( (rv > 0) && !wp_active ){
                DlgAlert_Notice(_("Warning: writing unavailable."));
                tapedlg[DLGTPE_WP].state |= SG_SELECTED;
                wp_active = 1;
            }
        }
	}
	else
	{
		dlgname[0] = '\0';
	}
	free(zip_path);
	free(selname);
}

static void scass_ToggleWriteProtection()
{
    bool wp_active = 0;
    SGOBJ *tapedlg;

    tapedlg = scass_GetDialog();
    if(wp_active){
        /* The write protection was active
         * before the click thus the "command"
         * is to make the protection inactive
         * */
        if (cass_SetProtection(false)==true){ /*Failure to do so*/
            DlgAlert_Notice(_("Writing unavailable on this device."));
            /* Forceful recheck to box the user has just unchecked */
            tapedlg[DLGTPE_WP].state |= SG_SELECTED;
            wp_active = true;          
        }else{
            /* This (first line) might be unnecessary
             * TODO: Remove and test
             * */
            tapedlg[DLGTPE_WP].state &= ~SG_SELECTED;
            wp_active = false;          
        }
    }else{
        /* The write protection was inactive
         * before the click thus the "command"
         * is to make the protection active
         * */
        cass_SetProtection(true);
        /* This (first line) might be unnecessary
         * TODO: Remove and test
         * */
        tapedlg[DLGTPE_WP].state |= SG_SELECTED;
        wp_active = true;          
    }
}

static void scass_ChangeCounter(bool down)
{
    int counter;

    counter = cass_GetCounter();

    if (down)
       counter--;
    else
       counter++;

    counter = MID(0, counter, COUNTER_MAX);
    cass_SetCounter(counter);

    sprintf(sTapeIdx, "%3d", counter);
}

/*-----------------------------------------------------------------------*/
/**
 * Show and process the "Tape" dialog
 */
void scass_Panel(void)
{
	int but;
    SGOBJ *tapedlg;

    tapedlg = scass_GetDialog();
	SDLGui_CenterDlg(tapedlg);

    /* Init values from Teo current config */

    /*Tape image filename*/
    log_msgf(LOG_TRACE,"Tape: %s\n",teo.cass.file );
    if(!teo.cass.file || *teo.cass.file == '\0')
        snprintf(sFile, FILENAME_MAX-1, "%s", _("(None)"));
    else
        snprintf(sFile, FILENAME_MAX-1, "%s", std_BaseName(teo.cass.file));

    /*Write protection*/
    tapedlg[DLGTPE_WP].state &= ~SG_SELECTED;
    wp_active = false;
    if(teo.cass.write_protect){ 
        tapedlg[DLGTPE_WP].state |= SG_SELECTED;
        wp_active = true;
    }

    /*Counter*/
	sprintf(sTapeIdx, "%3d", 0);



	/* Show the dialog: */
	do{
        but = SDLGui_DoDialog(tapedlg, NULL, false);
		switch(but){
         /* Choose a new disk*/
		 case DLGTPE_BROWSE:                        
			scass_Browse(sFile, DLGTPE_NAME);
			break;
		 case DLGTPE_EJECT:
            scass_Eject(sFile);
			break;
         case DLGTPE_WP:
            scass_ToggleWriteProtection();
            break;
		 case DLGTPE_IDX_MORE:
            scass_ChangeCounter(false);
			break;
		 case DLGTPE_IDX_LESS:
            scass_ChangeCounter(true);
			break;
         case DLGTPE_REWIND:
            cass_SetCounter(0);
			sprintf(sTapeIdx, "%3d", 0);
			break;
        }
    }while (but != DLGDSK_OK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );

    if(but != DLGDSK_OK) return;

    /*Writeback values to Teo config*/

    /*Write protection*/
    teo.cass.write_protect = get_state(tapedlg[DLGTPE_WP]);
    
    /* Other settings (filename, counter) have already been set 
     * by functions directly called from the dialog
     * */
}
