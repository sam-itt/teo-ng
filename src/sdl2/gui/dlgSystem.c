/*
  Original code from Hatari, adapted for Teo

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Dialog for setting various options
*/

#include "dialog.h"
#include "sdlgui.h"

#include "teo.h"

static char sSoundVolume[4];

#define DLGSET_SPD_EXACT   3
#define DLGSET_SPD_FAST    4
#define DLGSET_SOUND       5
#define DLGSET_VOL_LESS    6
#define DLGSET_VOL_TXT     7
#define DLGSET_VOL_MORE    8 
#define DLGSET_MEM_256     11
#define DLGSET_MEM_512     12
#define DLGSET_INTL_VID    13
#define DLGSET_EXIT        14 


static SGOBJ systemdlg[] =
{
/*x and y are relative to the upper left corner of the dialog */
/*Type, flags, state, x,y, w, h,text, shortcut*/
/*    type ,fl, st,x,y, w, h, text*/
	{ SGBOX, 0, 0, 0,0, 50,13, NULL },
	{ SGTEXT, 0, 0, 18,1, 14,1, "System options" },

	{ SGTEXT,     0, 0,  3,3, 6,1, "Speed:" }, /*Width in chars*/
	{ SGRADIOBUT, 0, 0, 19,3, 4,1, "_exact" },
	{ SGRADIOBUT, 0, 0, 35,3, 4,1, "_fast" },

	{ SGCHECKBOX, 0, 0, 6, 5, 5,1, " _Sound" },
	{ SGBUTTON,   0, 0, 18,5, 1,1, "\x04", SG_SHORTCUT_LEFT },
	{ SGTEXT,     0, 0, 20,5, 3,1, sSoundVolume },
	{ SGBUTTON,   0, 0, 24,5, 1,1, "\x03", SG_SHORTCUT_RIGHT },
	{ SGTEXT,     0, 0, 26,5,12,1, "%" },

	{ SGTEXT,     0, 0,  3,7, 6,1, "Memory:" }, /*Width in chars*/
	{ SGRADIOBUT, 0, 0, 19,7, 4,1, "_256k" },
	{ SGRADIOBUT, 0, 0, 35,7, 4,1, "_512k" },

	{ SGCHECKBOX, 0, 0, 6, 9, 17,1, "Interlaced _video" },

    { SGBUTTON, SG_DEFAULT, 0, 16,11, 20,1, "Back to main menu" },
	{ SGSTOP, 0, 0, 0,0, 0,0, NULL }
};


/*-----------------------------------------------------------------------*/
/**
 * Show and process the "System" dialog
 */
void DlgSystem_Main(bool adjustableVolume)
{
	int but;
    int volume;
    Uint8 mem_size;

    
	SDLGui_CenterDlg(systemdlg);

	/* Set up speed */
    systemdlg[DLGSET_SPD_EXACT].state &= ~SG_SELECTED;
    systemdlg[DLGSET_SPD_FAST].state &= ~SG_SELECTED;

    if(teo.setting.exact_speed)
        systemdlg[DLGSET_SPD_EXACT].state |= SG_SELECTED;
    else 
        systemdlg[DLGSET_SPD_FAST].state |= SG_SELECTED;

    /*Sound*/
    systemdlg[DLGSET_SOUND].state &= ~SG_SELECTED;
    volume = 100;
	sprintf(sSoundVolume, "%3i", volume);

    if(teo.setting.sound_enabled)
        systemdlg[DLGSET_SOUND].state |= SG_SELECTED;


    /*Memory*/
    systemdlg[DLGSET_MEM_256].state &= ~SG_SELECTED;
    systemdlg[DLGSET_MEM_512].state &= ~SG_SELECTED;

    if(teo.setting.bank_range == 32)
        systemdlg[DLGSET_MEM_512].state |= SG_SELECTED;
    else
        systemdlg[DLGSET_MEM_256].state |= SG_SELECTED;

    /*Video*/
    systemdlg[DLGSET_INTL_VID].state &= ~SG_SELECTED;

    if(teo.setting.interlaced_video)
        systemdlg[DLGSET_INTL_VID].state |= SG_SELECTED;

	/* Show the dialog: */
    do{
	    but = SDLGui_DoDialog(systemdlg, NULL, false);
		switch(but){
		 case DLGSET_VOL_MORE:
            if(adjustableVolume){
                if(volume < 100)
                    volume++;
			    sprintf(sSoundVolume, "%3i", volume);
            }
			break;
		 case DLGSET_VOL_LESS:
            if(adjustableVolume){
                if(volume > 0)
                    volume--;
			    sprintf(sSoundVolume, "%3i", volume);
            }
			break;
        }
    }while (but != DLGSET_EXIT && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );


	/* Read values from dialog: */


    /*Speed*/
    printf("Speed: ");
    if(systemdlg[DLGSET_SPD_EXACT].state & SG_SELECTED)
        printf("exact\n");
    else if(systemdlg[DLGSET_SPD_FAST].state & SG_SELECTED)
        printf("fast\n");
    else
        printf("undefined\n");
    teo.setting.exact_speed = get_state(systemdlg[DLGSET_SPD_EXACT]);

    /*Sound*/
    printf("Sound: %s\n", systemdlg[DLGSET_SOUND].state & SG_SELECTED ? "on" : "off");
    printf("Volume: %s%%\n", sSoundVolume);
    teo.setting.sound_enabled = get_state(systemdlg[DLGSET_SOUND]);


    /*Memory*/
    printf("Memory: ");
    if(systemdlg[DLGSET_MEM_256].state & SG_SELECTED)
        printf("256k\n");
    else if(systemdlg[DLGSET_MEM_512].state & SG_SELECTED)
        printf("512k\n");
    else
        printf("undefined\n");
    mem_size = get_state(systemdlg[DLGSET_MEM_512]) ? 32 : 16;
    if (mem_size != teo.setting.bank_range)
        teo.command = TEO_COMMAND_COLD_RESET;
    teo.setting.bank_range = mem_size;

    /*Video*/
    printf("Interlaced video: %s\n", systemdlg[DLGSET_INTL_VID].state & SG_SELECTED ? "on" : "off");
    teo.setting.interlaced_video = get_state(systemdlg[DLGSET_INTL_VID]);

}