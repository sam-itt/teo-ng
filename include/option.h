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
 *  Copyright (C) 1997-2014 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : options.h
 *  Version    : 1.8.3
 *  Créé par   : François Mouret 07/10/2012
 *  Modifié par:
 *
 *  Lecture de la ligne de commande.
 */


#ifndef OPTIONS_H
#define OPTIONS_H

#include "std.h"

enum {
  OPTION_ARG_BOOL = 0,
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
};

extern char *option_Parse (int argc, char *argv[],
                          char *internal_prog_name,
                          struct OPTION_ENTRY program_option[],
                          struct STRING_LIST **remain_option);
extern int  option_Undefined (char *fname);

                          
#endif
