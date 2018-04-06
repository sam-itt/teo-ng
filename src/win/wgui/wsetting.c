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
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 28/11/2000
 *  Modifié par: Eric Botcazou 28/10/2003
 *               François Mouret 17/09/2006 28/08/2011 18/03/2012
 *                               19/09/2012 18/09/2013 10/05/2014
 *
 *  Interface utilisateur Windows native.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
#endif

#include "teo.h"
#include "image.h"
#include "alleg/sound.h"
#include "win/gui.h"


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
    EnableWindow(GetDlgItem(hDlg, IDC_SOUND_CHECK), sound_state);
    EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_BAR), volume_state);
}



static void init_bar (HWND hDlg)
{
   HWND volume_bar = GetDlgItem(hDlg, IDC_VOLUME_BAR);

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
   int state;
   static int bank_range;

   switch(uMsg)
   {
      case WM_INITDIALOG:
#ifdef FRENCH_LANGUAGE
         SetWindowText(GetDlgItem(hDlg, IDC_SPEED_GROUP), "Vitesse");
         SetWindowText(GetDlgItem(hDlg, IDC_EXACT_SPEED_BUTTON), "exacte");
         SetWindowText(GetDlgItem(hDlg, IDC_MAX_SPEED_BUTTON), "rapide");
         SetWindowText(GetDlgItem(hDlg, IDC_SOUND_GROUP), "Son");
         SetWindowText(GetDlgItem(hDlg, IDC_SOUND_CHECK), "actif");
         SetWindowText(GetDlgItem(hDlg, IDC_VOLUME_LOW_LTEXT), "min");
         SetWindowText(GetDlgItem(hDlg, IDC_VOLUME_HIGH_LTEXT), "max");
         SetWindowText(GetDlgItem(hDlg, IDC_MEMORY_GROUP), "Mémoire");
         SetWindowText(GetDlgItem(hDlg, IDC_INTERLACED_CHECK), "vidéo entrelacée");
#else
         SetWindowText(GetDlgItem(hDlg, IDC_SPEED_GROUP), "Speed");
         SetWindowText(GetDlgItem(hDlg, IDC_EXACT_SPEED_BUTTON), "exact");
         SetWindowText(GetDlgItem(hDlg, IDC_MAX_SPEED_BUTTON), "fast");
         SetWindowText(GetDlgItem(hDlg, IDC_SOUND_GROUP), "Sound");
         SetWindowText(GetDlgItem(hDlg, IDC_SOUND_CHECK), "activated");
         SetWindowText(GetDlgItem(hDlg, IDC_VOLUME_LOW_LTEXT), "min");
         SetWindowText(GetDlgItem(hDlg, IDC_VOLUME_HIGH_LTEXT), "max");
         SetWindowText(GetDlgItem(hDlg, IDC_MEMORY_GROUP), "Memory");
         SetWindowText(GetDlgItem(hDlg, IDC_INTERLACED_CHECK), "interlaced video");
#endif
         if (teo.setting.bank_range == 32)
         {
            SetWindowText(GetDlgItem(hDlg, IDC_MEMORY_256K_RADIO), "256k (+reset)");
            SetWindowText(GetDlgItem(hDlg, IDC_MEMORY_512K_RADIO), "512k");
         }
         else
         {
            SetWindowText(GetDlgItem(hDlg, IDC_MEMORY_256K_RADIO), "256k");
            SetWindowText(GetDlgItem(hDlg, IDC_MEMORY_512K_RADIO), "512k (+reset)");
         }

         /* initialisation des boutons radio de la vitesse */
         CheckRadioButton(hDlg, IDC_EXACT_SPEED_BUTTON, IDC_MAX_SPEED_BUTTON, 
               (teo.setting.exact_speed ? IDC_EXACT_SPEED_BUTTON
                                        : IDC_MAX_SPEED_BUTTON));

         /* initialisation des boutons radio de l'extension mémoire */
         CheckRadioButton(hDlg, IDC_MEMORY_256K_RADIO, IDC_MEMORY_512K_RADIO, 
               ((teo.setting.bank_range == 32) ? IDC_MEMORY_512K_RADIO
                                               : IDC_MEMORY_256K_RADIO));

         /* initialisation du mode entrelacé */
         state = (teo.setting.interlaced_video) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hDlg, IDC_INTERLACED_CHECK, state);

         /* initialisation du checkbox de son */
         state = (teo.setting.sound_enabled) ? BST_CHECKED : BST_UNCHECKED;
         CheckDlgButton(hDlg, IDC_SOUND_CHECK, state);

         /* initialisation de la barre du volume */
         init_bar (hDlg);

         update_state (hDlg);
         bank_range = teo.setting.bank_range;
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDC_EXACT_SPEED_BUTTON:
               teo.setting.exact_speed = TRUE;
               update_state (hDlg);
               break;

            case IDC_MAX_SPEED_BUTTON:
               teo.setting.exact_speed = FALSE;
               update_state (hDlg);
               break;

            case IDC_INTERLACED_CHECK:
               state = IsDlgButtonChecked(hDlg, IDC_INTERLACED_CHECK);
               state = (state == BST_CHECKED) ? TRUE : FALSE;
               teo.setting.interlaced_video = state;
               break;
               
            case IDC_MEMORY_256K_RADIO:
               teo.setting.bank_range = 16;
               teo.command = (bank_range == 16) ? TEO_COMMAND_NONE
                                                : TEO_COMMAND_COLD_RESET;
               break;

            case IDC_MEMORY_512K_RADIO:
               teo.setting.bank_range = 32;
               teo.command = (bank_range == 32) ? TEO_COMMAND_NONE
                                                : TEO_COMMAND_COLD_RESET;
               break;

            case IDC_SOUND_CHECK:
               teo.setting.sound_enabled =
                    (IsDlgButtonChecked(hDlg, IDC_SOUND_CHECK) == BST_CHECKED)
                         ? TRUE : FALSE;
               update_state (hDlg);
               break;
         }
         return TRUE;

      case WM_HSCROLL:
         if ((HWND)lParam == GetDlgItem(hDlg, IDC_VOLUME_BAR))
               update_bar(wParam);
         return TRUE;

      default:
         return FALSE;
   }
   return FALSE;
}
