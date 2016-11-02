/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2016 Yves Charriau, François Mouret
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  Module     : windows/gui/install.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  About callback.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <windows.h>
   #include <windowsx.h>
   #include <shellapi.h>
   #include <commctrl.h>
   #include <sys/stat.h>
#endif

#include "defs.h"
#include "main.h"
#include "std.h"
#include "encode.h"
#include "errors.h"
#include "cc90.h"
#include "win/gui.h"
#include "win/resource.h"

/* ------------------------------------------------------------------------- */


static char program_text[] = 
    "0 'SAVE\"INSTALL.BAS\",A\r\n" \
    "5 '\r\n" \
    "10 DATA \"8EE7E0CE45001A50\",&H3D2\r\n" \
    "15 DATA \"4F5FED02CC03FFED\",&H458\r\n" \
    "20 DATA \"84CC043CED028D2F\",&H33B\r\n" \
    "25 DATA \"1F988D2B33CB8D27\",&H321\r\n" \
    "30 DATA \"1F988D23E7C04A26\",&H37E\r\n" \
    "35 DATA \"F9E684C42026FACC\",&H533\r\n" \
    "40 DATA \"0A02E700C6031E88\",&H262\r\n" \
    "45 DATA \"1F884A26F58D0827\",&H2C8\r\n" \
    "50 DATA \"D5E784ECC36ECB34\",&H55C\r\n" \
    "55 DATA \"02C601E784E6842B\",&H3C9\r\n" \
    "60 DATA \"FCE6842BF8C6801E\",&H4ED\r\n" \
    "65 DATA \"881F88A600485624\",&H297\r\n" \
    "70 DATA \"F63582\",&H1AD\r\n" \
    "75 '\r\n" \
    "80 LOCATE,,0:CLS:CONSOLE,,1\r\n" \
    "85 D=PEEK(&HFFF2)\r\n" \
    "90 IF D<128 THEN D=16384 ELSE D=0\r\n" \
    "95 A=D\r\n" \
    "100 FOR I=1 TO 13\r\n" \
    "105  READ A$,C:R=0\r\n" \
    "110  FOR J=1 TO LEN(A$)-1 STEP2\r\n" \
    "115   V=VAL(\"&H\"+MID$(A$,J,2))\r\n" \
    "120   R=R+V\r\n" \
    "125   POKE A,V\r\n" \
    "130   A=A+1\r\n" \
    "135  NEXTJ\r\n" \
    "140  IF R<>C THEN PRINT\"Error line\";I;\"of datas \r\n" \
    "(&H\";HEX$(R);\"<>&H\";HEX$(C);\")\":END\r\n" \
    "145 NEXTI\r\n" \
    "150 '\r\n" \
    "155 A=D/256\r\n" \
    "160 POKE D+1,&HA7+A:POKE D+4,&H05+A\r\n" \
    "165 EXEC D";

static HWND cText1;
static HWND cText2;
static HWND cText3;
static HWND cCancelButton;



/*
 * Procédure pour la boîte de dialogue
 */
static LRESULT CALLBACK install_Procedure (HWND hDlg, UINT uMsg,
                                           WPARAM wParam, LPARAM lParam)
{
    static HFONT hCourierFont;
    static HFONT hItalicFont;
    char *string = NULL;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            cText1 = GetDlgItem (hDlg, IDC_INSTALL_TEXT1);
            cText2 = GetDlgItem (hDlg, IDC_INSTALL_TEXT2);
            cText3 = GetDlgItem (hDlg, IDC_INSTALL_TEXT3);
            cCancelButton = GetDlgItem (hDlg, IDCANCEL);

            hCourierFont = CreateFont(16, 0, 0, 0, FW_NORMAL,
                               FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH, "Courier New");
            hItalicFont = CreateFont(14, 0, 0, 0, FW_NORMAL,
                               TRUE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
            string = std_strdup_printf ("%s",
                is_fr?"Tout d'abord, le programme INSTALL.BAS " \
                      "doit tourner sur le Thomson"
                     :"First of all, the INSTALL.BAS program must " \
                      "run on the Thomson");
            Static_SetText (cText1, string);
            string = std_free (string);
            SetWindowFont (cText1, hItalicFont , TRUE);
            SetWindowText(cText2, program_text);
            SetWindowFont (cText2, hCourierFont, TRUE);
            string = std_strdup_printf ("%s",
                is_fr?encode_String("Cliquez sur OK lorsque vous êtes prêt")
                     :"Click OK when you are ready");
            Static_SetText (cText3, string);
            string = std_free (string);
            SetWindowFont (cText3, hItalicFont , TRUE);
            Button_SetText (cCancelButton, is_fr?"Annuler":"Cancel");
            break;

        case WM_COMMAND :
            switch(LOWORD(wParam))
            {
                case IDC_OK_BUTTON :
                    (void)DeleteObject((HGDIOBJ)hCourierFont);
                    (void)DeleteObject((HGDIOBJ)hItalicFont);
                    EndDialog(hDlg, TRUE);
                    break;
                
                case IDCANCEL :
                    (void)DeleteObject((HGDIOBJ)hCourierFont);
                    (void)DeleteObject((HGDIOBJ)hItalicFont);
                    EndDialog(hDlg, FALSE);
                    break;
            }
            break;
    }
    return 0;

    (void)lParam;
}


/* ------------------------------------------------------------------------- */


void install_Prog (HINSTANCE hInst, HWND hDlg)
{
    int ret;

    ret = DialogBox(hInst, MAKEINTRESOURCE(ID_INSTALL),
                  hDlg, (DLGPROC)install_Procedure);

    if (ret == TRUE)
        if (cc90_Install() < 0)
            gui_ErrorDialog (error_msg);
}         
