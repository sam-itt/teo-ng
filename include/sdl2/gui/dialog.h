/*
  Original code from Hatari, adapted for Teo

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/
#ifndef TEO_SDL_DIALOG_H
#define TEO_SDL_DIALOG_H

#include <stdbool.h>

/* prototypes for gui-sdl/dlg*.c functions: */
extern int sgui_Panel(bool adjustableVolume, int da_mask);

extern void ssetting_Panel(bool adjustableVolume);
extern void sdisk_Panel(int da_mask);
extern void scass_Panel(void);
extern void smemo_Panel(void);
extern void sprinter_Panel(void);
extern void sabout_Panel(void);


int DlgAlert_Query(const char *text);
int DlgAlert_Notice(const char *text);
#endif
