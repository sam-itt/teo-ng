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
 *                          Jérémie Guillaume
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
 *  Module     : to8err.h
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou 23/11/2000
 *  Modifié par: Eric Botcazou 12/02/2001
 *
 *  Déclarations des types d'erreur renvoyés par l'émulateur.
 */


#ifndef TO8ERR_H
#define TO8ERR_H

enum {
    TO8_MULTIPLE_INIT,
    TO8_BAD_ALLOC,
    TO8_CANNOT_FIND_FILE,
    TO8_CANNOT_OPEN_FILE,
    TO8_BAD_FILE_FORMAT,
    TO8_UNSUPPORTED_MODEL,
    TO8_BAD_JOYSTICK_NUM,
    TO8_ERROR_MAX
};

extern void to8_RegisterErrorTable(const char *table[TO8_ERROR_MAX]);

#endif
