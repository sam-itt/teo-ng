/*
 *    TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *    TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EEEEEEEEEE      OO          OO
 *          TT        EEEEEEEEEE      OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *          TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *
 *                  L'émulateur Thomson TO8
 *
 *  Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret
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
 *  Module     : wsetting.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               19/09/2012
 *
 *  Interface utilisateur Windows native.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <windows.h>
   #include <shellapi.h>
   #include <commctrl.h>
#endif

#include "intern/image.h"
#include "alleg/sound.h"
#include "win/dialog.rh"
#include "win/gui.h"
#include "to8.h"



#define MIN_POS    0
#define MAX_POS    254
#define LINE_STEP  2
#define PAGE_STEP  16


static void init_bar(HWND volume_bar)
{
   SendMessage(volume_bar, TBM_SETRANGE, TRUE, MAKELONG(MIN_POS, MAX_POS));
   SendMessage(volume_bar, TBM_SETLINESIZE, TRUE, LINE_STEP);
   SendMessage(volume_bar, TBM_SETPAGESIZE, TRUE, PAGE_STEP);
   SendMessage(volume_bar, TBM_SETTICFREQ, PAGE_STEP, 0);

   SendMessage(volume_bar, TBM_SETPOS, TRUE,
      (teo.setting.sound_enabled && teo.setting.exact_speed)
                     ? asound_GetVolume()-1 : (MAX_POS-MIN_POS)/2);
   
   if (!teo.setting.sound_enabled || !teo.setting.exact_speed)
      EnableWindow(volume_bar, FALSE);
}


static void update_bar(HWND volume_bar, WPARAM wParam)
{
   int pos = asound_GetVolume()-1;

   switch(LOWORD(wParam))
   {
      case TB_TOP:
         pos = MIN_POS;
         break;

      case TB_LINEUP:
         pos -= LINE_STEP;
         break;

      case TB_PAGEUP:
         pos -= PAGE_STEP;
         if (pos<MIN_POS)
            pos = MIN_POS;
         break;

      case TB_BOTTOM:
         pos = MAX_POS;
         break;

      case TB_LINEDOWN:
         pos += LINE_STEP;
         break;

      case TB_PAGEDOWN:
         pos += PAGE_STEP;
         if (pos>MAX_POS)
            pos = MAX_POS;
         break;           

      case TB_THUMBPOSITION:
      case TB_THUMBTRACK:
         pos = HIWORD(wParam);
         break;
   }

   asound_SetVolume(pos+1);
}



static void update_sound_check(HWND hDlg, HWND volume_bar)
{
    int state;

    if (teo.sound_enabled)
    {
        state = (IsDlgButtonChecked(hDlg, SOUND_CHECK) == BST_CHECKED);
        teo.setting.sound_enabled = state ? TRUE : FALSE;
        EnableWindow(volume_bar, state ? TRUE : FALSE);
    }
    else
    {
        MessageBox(hDlg, is_fr? "Carte son non detectée"
                              : "Sound card not detected",
                              PROGNAME_STR, MB_OK | MB_ICONERROR);
        CheckDlgButton(hDlg, SOUND_CHECK, BST_UNCHECKED);
        EnableWindow(volume_bar, FALSE);
    }
}


/* ------------------------------------------------------------------------- */


/* wsetting_TabProc:
 *  Procédure pour l'onglet des réglages
 */
int CALLBACK wsetting_TabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static HWND sound_check;
   static HWND volume_bar;

   switch(uMsg)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANG
         SetWindowText(GetDlgItem(hDlg, SPEED_LTEXT), "Vitesse:");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exacte");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "rapide");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Mode vidéo entrelacé");
         SetWindowText(GetDlgItem(hDlg, SOUND_CHECK), "Son:");
#else
         SetWindowText(GetDlgItem(hDlg, SPEED_LTEXT), "Speed:");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exact");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "fast");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Interlaced video");
         SetWindowText(GetDlgItem(hDlg, SOUND_CHECK), "Sound:");
#endif
         /* initialisation des boutons radio de la vitesse */
         CheckRadioButton(hDlg, EXACT_SPEED_BUTTON, MAX_SPEED_BUTTON, 
               (teo.setting.exact_speed ? EXACT_SPEED_BUTTON : MAX_SPEED_BUTTON));

         /* initialisation du mode entrelacé */
         CheckDlgButton(hDlg, INTERLACED_CHECK, teo.setting.interlaced_video
                                                ? BST_CHECKED : BST_UNCHECKED);

         /* initialisation du checkbox de son */
         CheckDlgButton(hDlg, SOUND_CHECK, teo.setting.sound_enabled
                                                ? BST_CHECKED : BST_UNCHECKED);

         /* initialisation de la barre du volume */
         volume_bar = GetDlgItem(hDlg, VOLUME_BAR);
         sound_check = GetDlgItem(hDlg, SOUND_CHECK);
         init_bar(volume_bar);

         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case EXACT_SPEED_BUTTON:
               teo.setting.exact_speed = TRUE;
               EnableWindow(volume_bar, TRUE);
               EnableWindow(sound_check, TRUE);
               break;

            case MAX_SPEED_BUTTON:
               teo.setting.exact_speed = FALSE;
               EnableWindow(volume_bar, FALSE);
               EnableWindow(sound_check, FALSE);
               break;

            case INTERLACED_CHECK:
               teo.setting.interlaced_video =
                  (IsDlgButtonChecked(hDlg, INTERLACED_CHECK) == BST_CHECKED)
                      ? TRUE : FALSE;
               break;
               
            case SOUND_CHECK:
               update_sound_check(hDlg, volume_bar);
               break;
         }
         return TRUE;

      case WM_HSCROLL:
         if ((HWND)lParam == volume_bar)
         {
            if (teo.setting.sound_enabled && teo.setting.exact_speed)
               update_bar(volume_bar, wParam);
         }
         return TRUE;

      default:
         return FALSE;
   }
}
