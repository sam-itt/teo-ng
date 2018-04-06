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
 *  Module     : option.h
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 30/05/2015
 *
 *  Management of the command line.
 */


#ifndef OPTION_H
#define OPTION_H 1

enum {
  OPTION_ARG_BOOL = 0,
  OPTION_ARG_INT,
  OPTION_ARG_FILENAME,
  OPTION_ARG_STRING,
  OPTION_ARG_HELP
};

struct OPTION_ENTRY {
    char   *long_name;
    char   short_name;
    int    type;
    void   *reg;
    char   *comment;
    char   *argument;
    int    min_value;
    int    max_value;
};

extern char *option_Parse (int argc, char *argv[],
                          char *internal_prog_name,
                          struct OPTION_ENTRY program_option[],
                          struct STRING_LIST **remain_option);
extern void option_Undefined (char *fname);

                          
#endif
