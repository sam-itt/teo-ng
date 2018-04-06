/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2018 Yves Charriau, François Mouret
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
 *  Module     : win/resource.h
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Windows resource file.
 */


#ifndef WIN_RESOURCE_H
#define WIN_RESOURCE_H 1

/* DialogBoxes */
#define ID_MAIN                   3020
#define ID_INSTALL                3021
#define ID_DETECT                 3022
#define ID_ABOUT                  3023
#define ID_PROGRESS               3024

#define IDC_MAIN_ARCHIVE_BUTTON   3050
#define IDC_MAIN_EXTRACT_BUTTON   3051
#define IDC_MAIN_INSTALL_BUTTON   3052
#define IDC_MAIN_ABOUT_BUTTON     3053
#define IDC_MAIN_OPTION_GROUPBOX  3054
#define IDC_MAIN_SIDE0_CHECKBOX   3055
#define IDC_MAIN_SIDE1_CHECKBOX   3056
#define IDC_MAIN_RETRY_RTEXT      3057
#define IDC_MAIN_RETRY_EDIT       3058
#define IDC_MAIN_PROGRESS_LTEXT   3059
#define IDC_MAIN_PROGRESS_BAR     3060
#define IDC_MAIN_CANCEL           3061

/* Buttons */
#define IDC_OK_BUTTON             3070

/* Fenêtre de progression */

/* Cc90/Install... */
#define IDC_INSTALL_TEXT1         3090
#define IDC_INSTALL_TEXT2         3091
#define IDC_INSTALL_TEXT3         3092

/* Cc90/Detect serial ports */
#define IDC_DETECT_LTEXT          3110
#define IDC_DETECT_COMBO          3111

/* Help/about... */
#define IDC_ABOUT_DIALOG          3130
#define IDC_ABOUT_CTEXT_TITLE     3131
#define IDC_ABOUT_CTEXT_COPYRIGHT 3132
#define IDC_ABOUT_STATIC_LINK     3133
#define IDC_ABOUT_STATIC_FORUM    3134
#define IDC_ABOUT_CTEXT_LICENSE   3135

#endif
