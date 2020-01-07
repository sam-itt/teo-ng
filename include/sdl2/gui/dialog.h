/*
  Original code from Hatari, adapted for Teo

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/
#ifndef TEO_SDL_DIALOG_H
#define TEO_SDL_DIALOG_H

#include <stdbool.h>

/* prototypes for gui-sdl/dlg*.c functions: */
extern int Dialog_MainDlg(bool adjustableVolume, int da_mask);

extern void DlgSystem_Main(bool adjustableVolume);
extern void DlgDisks_Main(int da_mask);
extern void DlgTape_Main(void);
extern void DlgCart_Main(void);
extern void DlgPrinter_Main(void);
extern void Dialog_AboutDlg(void);


int DlgAlert_Query(const char *text);
int DlgAlert_Notice(const char *text);
#endif
