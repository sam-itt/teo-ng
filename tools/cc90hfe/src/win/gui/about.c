/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 Yves Charriau, François Mouret
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
 *  Module     : windows/gui/about.c
 *  Version    : 0.5.0
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
   #include <shellapi.h>
   #include <commctrl.h>
#endif

#include "defs.h"
#include "main.h"
#include "win/resource.h"

/* ------------------------------------------------------------------------- */


/* AboutProc:
 *  Procédure pour la boîte "A propos"
 */
static LRESULT CALLBACK about_Procedure(HWND hDlg, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam)
{
   static HWND aboutLink;
   static HWND aboutTitle;
   static HWND aboutCopyright;
   static HWND aboutLicense;
   static HFONT hLinkStyle;
   static HFONT hTitleStyle;
   static HFONT hCopyrightStyle;
   static HFONT hLicenseStyle;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         aboutLink  = GetDlgItem (hDlg, IDC_ABOUT_STATIC_LINK);
         aboutTitle = GetDlgItem (hDlg, IDC_ABOUT_CTEXT_TITLE);
         aboutCopyright = GetDlgItem (hDlg, IDC_ABOUT_CTEXT_COPYRIGHT);
         aboutLicense = GetDlgItem (hDlg, IDC_ABOUT_CTEXT_LICENSE);
         hTitleStyle = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE,
                           FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
         hLinkStyle = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE,
                           TRUE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
         hCopyrightStyle = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE,
                           FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
         hLicenseStyle = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE,
                           FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
         SendMessage(aboutLink, WM_SETFONT, (WPARAM)hLinkStyle, TRUE);
         SendMessage(aboutTitle, WM_SETFONT, (WPARAM)hTitleStyle, TRUE);
         SendMessage(aboutLicense, WM_SETFONT, (WPARAM)hLicenseStyle, TRUE);
         SendMessage(aboutCopyright, WM_SETFONT, (WPARAM)hCopyrightStyle, TRUE);
                              
         SetWindowText(hDlg, is_fr?PROG_NAME" - A propos"
                                  :PROG_NAME" - About");
         SetWindowText(aboutLink, is_fr?PROG_NAME" sur SourceForge (Teo module)"
                                       :PROG_NAME" on SourceForge (Teo module)");
         SetWindowText(aboutTitle, "CC90HFE\n Version "PROG_VERSION_MAJOR"."PROG_VERSION_MINOR);
         SetWindowText(aboutCopyright, "Copyright © 1997-"PROG_CREATION_YEAR"\n Yves Charriau, François Mouret");

         SetClassLongPtr(aboutLink, GCLP_HCURSOR,
                         (ULONG_PTR)LoadCursor(NULL, IDC_HAND));
         return TRUE;

      case WM_CTLCOLORSTATIC :
          if ((HWND)lParam == aboutCopyright)
          {
              SetTextColor((HDC)wParam, RGB(0, 0, 0));
              SetBkMode((HDC)wParam, TRANSPARENT);
              return (BOOL)GetStockObject(HOLLOW_BRUSH);
          }
          if ((HWND)lParam == aboutTitle)
          {
              SetTextColor((HDC)wParam, RGB(0, 0, 0));
              SetBkMode((HDC)wParam, TRANSPARENT);
              return (BOOL)GetStockObject(HOLLOW_BRUSH);
          }
          if ((HWND)lParam == aboutLink)
          {
              SetTextColor((HDC)wParam, RGB(0, 0, 255));
              SetBkMode((HDC)wParam, TRANSPARENT);
              return (BOOL)GetStockObject(HOLLOW_BRUSH);
          }
          if ((HWND)lParam == aboutLicense)
          {
              SetTextColor((HDC)wParam, RGB(0, 0, 0));
              SetBkMode((HDC)wParam, TRANSPARENT);
              return (BOOL)GetStockObject(HOLLOW_BRUSH);
          }
          return TRUE;

      case WM_CTLCOLOREDIT :
          return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
             case IDCANCEL:
                (void)DeleteObject((HGDIOBJ)hLinkStyle);
                (void)DeleteObject((HGDIOBJ)hTitleStyle);
                (void)DeleteObject((HGDIOBJ)hCopyrightStyle);
                (void)DeleteObject((HGDIOBJ)hLicenseStyle);
                EndDialog(hDlg, TRUE);
                return TRUE;
          
            case IDC_ABOUT_STATIC_LINK :
               ShellExecute(NULL, "open", PROG_WEB_SITE, 0, 0, SW_SHOWNORMAL);
               break;

         }
         return TRUE;

      default:
         return FALSE;
   }
}


/* ------------------------------------------------------------------------- */


void about_Prog (HINSTANCE hInst, HWND hDlg)
{
    (void)DialogBox(hInst, MAKEINTRESOURCE(ID_ABOUT),
                    hDlg, (DLGPROC)about_Procedure);
}         
