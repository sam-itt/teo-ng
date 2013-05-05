/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 François Mouret
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
 *  Module     : main.h
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Main program.
 */


#ifndef MAIN_H
#define MAIN_H 1

extern int windowed_mode;
extern FILE *fd_debug;
extern int progress_on;

extern void  main_Console (void);
extern void  main_ConsoleReadCommandLine (int argc, char *argv[]);
extern int   main_ArchiveDisk (void);
extern int   main_ExtractDisk (void);
extern void  main_FreeAll (void);
extern void  main_InitAll (void);

extern char *main_DataFile (const char filename[]);

extern void  gui_ErrorDialog (char *message);
extern void  gui_InitLanguage (void);
extern void  gui_Main (int argc, char *argv[]);
extern void  gui_SetConsoleEncoding (void);
extern void  gui_ProgressUpdate (int percent);

#endif

