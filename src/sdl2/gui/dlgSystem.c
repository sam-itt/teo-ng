/*
  Original code from Hatari, adapted for Teo by Samuel Cuella

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Dialog for setting various options
*/
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include "dialog.h"
#include "sdlgui.h"

#include "teo.h"
#include "gettext.h"

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
#define DLGSET_LEN         16

static SGOBJ *_systemdlg = NULL;

static SGOBJ *ssetting_GetDialog(void)
{
    if(!_systemdlg){
        int i = 0;
        _systemdlg = malloc(sizeof(SGOBJ)*DLGSET_LEN);

        /*x and y are relative to the upper left corner of the dialog */
        /*Type, flags, state, x,y, w, h,text, shortcut*/
        /*    type ,fl, st,x,y, w, h, text*/
        _systemdlg[i++] = (SGOBJ){ SGBOX, 0, 0, 0,0, 50,13, NULL };
        _systemdlg[i++] = (SGOBJ){ SGTEXT, 0, 0, 18,1, 14,1, _("Settings") };

        _systemdlg[i++] = (SGOBJ){ SGTEXT,     0, 0,  3,3, 6,1, _("Speed:") }; /*Width in chars*/
        _systemdlg[i++] = (SGOBJ){ SGRADIOBUT, 0, 0, 19,3, 4,1, _("_exact") };
        _systemdlg[i++] = (SGOBJ){ SGRADIOBUT, 0, 0, 35,3, 4,1, _("_fast") };

        _systemdlg[i++] = (SGOBJ){ SGCHECKBOX, 0, 0, 6, 5, 5,1, _(" _Sound") };
        _systemdlg[i++] = (SGOBJ){ SGBUTTON,   0, 0, 18,5, 1,1, "\x04", SG_SHORTCUT_LEFT };
        _systemdlg[i++] = (SGOBJ){ SGTEXT,     0, 0, 20,5, 3,1, sSoundVolume };
        _systemdlg[i++] = (SGOBJ){ SGBUTTON,   0, 0, 24,5, 1,1, "\x03", SG_SHORTCUT_RIGHT };
        _systemdlg[i++] = (SGOBJ){ SGTEXT,     0, 0, 26,5,12,1, "%" };

        _systemdlg[i++] = (SGOBJ){ SGTEXT,     0, 0,  3,7, 6,1, _("Memory:") }; /*Width in chars*/
        _systemdlg[i++] = (SGOBJ){ SGRADIOBUT, 0, 0, 19,7, 4,1, "_256k" };
        _systemdlg[i++] = (SGOBJ){ SGRADIOBUT, 0, 0, 35,7, 4,1, "_512k" };

        _systemdlg[i++] = (SGOBJ){ SGCHECKBOX, 0, 0, 6, 9, 17,1, _("Interlaced _video") };

        _systemdlg[i++] = (SGOBJ){ SGBUTTON, SG_DEFAULT, 0, 16,11, 20,1, _("Back to main menu") };
        _systemdlg[i++] = (SGOBJ){ SGSTOP, 0, 0, 0,0, 0,0, NULL };

        assert(i <= DLGSET_LEN);
    }
    return _systemdlg;
}


/*-----------------------------------------------------------------------*/
/**
 * Show and process the "System" dialog
 */
void DlgSystem_Main(bool adjustableVolume)
{
	int but;
    int volume;
    Uint8 mem_size;
    SGOBJ *systemdlg;

    systemdlg = ssetting_GetDialog();
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
