/*
  Hatari - dialog.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/

#ifndef TEO_SDL_DIALOG_H
#define TEO_SDL_DIALOG_H

#include <stdbool.h>
//#include "configuration.h"

/* prototypes for gui-sdl/dlg*.c functions: */
extern int Dialog_MainDlg(bool *bReset, bool *bLoadedSnapshot);

extern void DlgSystem_Main(void);
extern void DlgDisks_Main(void);
extern void DlgTape_Main(void);
extern void DlgCart_Main(void);
extern void DlgPrinter_Main(void);
extern void Dialog_AboutDlg(void);
#endif
