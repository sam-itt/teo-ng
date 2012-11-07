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
 *  Module     : intern/errors.h
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 22/11/2000
 *  Modifié par: François Mouret 01/11/2012
 *
 *  Gestion des erreurs générées par l'émulateur.
 */


#ifndef ERRORS_H
#define ERRORS_H

enum {
    TO8_MULTIPLE_INIT = -1000,
    TO8_BAD_ALLOC,
    TO8_CANNOT_FIND_FILE,
    TO8_CANNOT_OPEN_FILE,
    TO8_BAD_FILE_FORMAT,
    TO8_UNSUPPORTED_MODEL,
    TO8_BAD_JOYSTICK_NUM,
    TO8_BAD_MEMO_HEADER_CHECKSUM,
    TO8_BAD_MEMO_HEADER_NAME,
    TO8_MEDIA_ALREADY_SET,
    TO8_CANNOT_CREATE_DISK,
    TO8_SAP_NOT_VALID,
    TO8_FILE_TOO_LARGE,
    TO8_FILE_IS_EMPTY,
    TO8_SAP_DIRECTORY_FULL,
    ERR_ERROR
};

extern int error_Message(int error, const char moreinfo[]);

#endif
