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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               19/09/2012 18/09/2013
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

#include "image.h"
#include "alleg/sound.h"
#include "win/dialog.rh"
#include "win/gui.h"
#include "teo.h"


#define MIN_POS    0
#define MAX_POS    254
#define LINE_STEP  2
#define PAGE_STEP  16




static void update_state (HWND hDlg)
{
    int volume_state = FALSE;
    int sound_state = FALSE;

    if (teo.sound_enabled)
    {
        if (teo.setting.exact_speed)
        {
            sound_state = TRUE;
            if (teo.setting.sound_enabled)
                volume_state = TRUE;
        }
    }
    EnableWindow(GetDlgItem(hDlg, SOUND_CHECK), sound_state);
    EnableWindow(GetDlgItem(hDlg, VOLUME_BAR), volume_state);
}



static void init_bar (HWND hDlg)
{
   HWND volume_bar = GetDlgItem(hDlg, VOLUME_BAR);

   SendMessage(volume_bar, TBM_SETRANGE, TRUE, MAKELONG(MIN_POS, MAX_POS));
   SendMessage(volume_bar, TBM_SETLINESIZE, TRUE, LINE_STEP);
   SendMessage(volume_bar, TBM_SETPAGESIZE, TRUE, PAGE_STEP);
   SendMessage(volume_bar, TBM_SETTICFREQ, PAGE_STEP, 0);

   SendMessage(volume_bar, TBM_SETPOS, TRUE, teo.setting.sound_volume);
   asound_SetVolume(teo.setting.sound_volume);
}


static void update_bar(WPARAM wParam)
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
   teo.setting.sound_volume = pos+1;
}



/* ------------------------------------------------------------------------- */


/* wsetting_TabProc:
 *  Procédure pour l'onglet des réglages
 */
int CALLBACK wsetting_TabProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static int bank_range;

   switch(uMsg)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANGUAGE
         SetWindowText(GetDlgItem(hDlg, SPEED_GROUP), "Vitesse");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exacte");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "rapide");
         SetWindowText(GetDlgItem(hDlg, SOUND_GROUP), "Son");
         SetWindowText(GetDlgItem(hDlg, SOUND_CHECK), "Actif");
         SetWindowText(GetDlgItem(hDlg, DISPLAY_GROUP), "Vidéo");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Entrelacée");
         SetWindowText(GetDlgItem(hDlg, MEMORY_GROUP), "Mémoire");
#else
         SetWindowText(GetDlgItem(hDlg, SPEED_GROUP), "Speed");
         SetWindowText(GetDlgItem(hDlg, EXACT_SPEED_BUTTON), "exact");
         SetWindowText(GetDlgItem(hDlg, MAX_SPEED_BUTTON), "fast");
         SetWindowText(GetDlgItem(hDlg, SOUND_GROUP), "Sound");
         SetWindowText(GetDlgItem(hDlg, SOUND_CHECK), "Activated");
         SetWindowText(GetDlgItem(hDlg, DISPLAY_GROUP), "Video");
         SetWindowText(GetDlgItem(hDlg, INTERLACED_CHECK), "Interlaced");
         SetWindowText(GetDlgItem(hDlg, MEMORY_GROUP), "Memory");
#endif
         if (teo.setting.bank_range == 32)
         {
            SetWindowText(GetDlgItem(hDlg, MEMORY_256K_RADIO), "256k (+reset)");
            SetWindowText(GetDlgItem(hDlg, MEMORY_512K_RADIO), "512k");
         }
         else
         {
            SetWindowText(GetDlgItem(hDlg, MEMORY_256K_RADIO), "256k");
            SetWindowText(GetDlgItem(hDlg, MEMORY_512K_RADIO), "512k (+reset)");
         }

         /* initialisation des boutons radio de la vitesse */
         CheckRadioButton(hDlg, EXACT_SPEED_BUTTON, MAX_SPEED_BUTTON, 
               (teo.setting.exact_speed ? EXACT_SPEED_BUTTON
                                        : MAX_SPEED_BUTTON));

         /* initialisation des boutons radio de l'extension mémoire */
         CheckRadioButton(hDlg, MEMORY_256K_RADIO, MEMORY_512K_RADIO, 
               ((teo.setting.bank_range == 32) ? MEMORY_512K_RADIO
                                               : MEMORY_256K_RADIO));

         /* initialisation du mode entrelacé */
         CheckDlgButton(hDlg, INTERLACED_CHECK, teo.setting.interlaced_video
                                                ? BST_CHECKED : BST_UNCHECKED);

         /* initialisation du checkbox de son */
         CheckDlgButton(hDlg, SOUND_CHECK, teo.setting.sound_enabled
                                                ? BST_CHECKED : BST_UNCHECKED);

         /* initialisation de la barre du volume */
         init_bar (hDlg);

         update_state (hDlg);
         bank_range = teo.setting.bank_range;
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case EXACT_SPEED_BUTTON:
               teo.setting.exact_speed = TRUE;
               update_state (hDlg);
               break;

            case MAX_SPEED_BUTTON:
               teo.setting.exact_speed = FALSE;
               update_state (hDlg);
               break;

            case INTERLACED_CHECK:
               teo.setting.interlaced_video =
                  (IsDlgButtonChecked(hDlg, INTERLACED_CHECK) == BST_CHECKED)
                      ? TRUE : FALSE;
               break;
               
            case MEMORY_256K_RADIO:
               teo.setting.bank_range = 16;
               teo.command = (bank_range == 16) ? TEO_COMMAND_NONE
                                                : TEO_COMMAND_COLD_RESET;
               break;

            case MEMORY_512K_RADIO:
               teo.setting.bank_range = 32;
               teo.command = (bank_range == 32) ? TEO_COMMAND_NONE
                                                : TEO_COMMAND_COLD_RESET;
               break;

            case SOUND_CHECK:
               teo.setting.sound_enabled =
                    (IsDlgButtonChecked(hDlg, SOUND_CHECK) == BST_CHECKED)
                         ? TRUE : FALSE;
               update_state (hDlg);
               break;
         }
         return TRUE;

      case WM_HSCROLL:
         if ((HWND)lParam == GetDlgItem(hDlg, VOLUME_BAR))
               update_bar(wParam);
         return TRUE;

      default:
         return FALSE;
   }
}
