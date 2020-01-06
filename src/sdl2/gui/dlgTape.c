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

#include "media/cass.h"
#include "errors.h"
#include "std.h"
#include "teo.h"



#define DLGTPE_EJECT 4
#define DLGTPE_NAME 5
#define DLGTPE_BROWSE 6
#define DLGTPE_WP 7

#define DLGTPE_IDX_LESS 9
#define DLGTPE_IDX 10
#define DLGTPE_IDX_MORE 11
#define DLGTPE_REWIND 12

#define DLGDSK_OK 13

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
    snprintf(dlgname, FILENAME_MAX-1, "%s", "(None)");
    cass_Eject();
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
	const char *tmpname;

	if (teo.memo.file)
	    tmpname = teo.cass.file;
	else
        tmpname = teo.default_folder ? teo.default_folder : "/";

	selname = SDLGui_FileSelect("Tape image:", tmpname, &zip_path, false);
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
                DlgAlert_Notice("Warning: writing unavailable.");
                tapedlg[DLGTPE_WP].state |= SG_SELECTED;
                wp_active = 1;
            }
        }
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

static void DlgTape_ToggleWriteProtection()
{
    bool wp_active = 0;
    if(wp_active){
        /* The write protection was active
         * before the click thus the "command"
         * is to make the protection inactive
         * */
        if (cass_SetProtection(false)==true){ /*Failure to do so*/
            DlgAlert_Notice(is_fr?"Ecriture impossible sur ce support."
                                   :"Writing unavailable on this device.");
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

static void DlgTape_ChangeCounter(bool down)
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
void DlgTape_Main(void)
{
	int but;


	SDLGui_CenterDlg(tapedlg);

    /* Init values from Teo current config
     * 
     * */

    /*Tape image filename*/
    printf("Tape: %s\n",teo.cass.file );
    if(!teo.cass.file || *teo.cass.file == '\0')
        snprintf(sFile, FILENAME_MAX-1, "%s", "(None)");
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
//        printf("But is : %d\n",but);
		switch(but){
         /* Choose a new disk*/
		 case DLGTPE_BROWSE:                        
			DlgTape_Browse(sFile, DLGTPE_NAME);
			break;
		 case DLGTPE_EJECT:
            DlgTape_Eject(sFile);
			break;
         case DLGTPE_WP:
            DlgTape_ToggleWriteProtection();
            break;
		 case DLGTPE_IDX_MORE:
            DlgTape_ChangeCounter(false);
			break;
		 case DLGTPE_IDX_LESS:
            DlgTape_ChangeCounter(true);
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
     * by the functions
     * */
}
