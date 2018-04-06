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
 *  Module     : error.h
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou 22/11/2000
 *  Modifié par: François Mouret 01/11/2012 23/08/2015
 *
 *  Gestion des erreurs générées par l'émulateur.
 */


#ifndef ERROR_H
#define ERROR_H

enum {
    TEO_ERROR_ALLOC = -1000,
    TEO_ERROR_BMP_FORMAT,
    TEO_ERROR_DIRECTORY_FULL,
    TEO_ERROR_DISK_CREATE,
    TEO_ERROR_DISK_IO,
    TEO_ERROR_DISK_NONE,
    TEO_ERROR_DISK_PROTECT,
    TEO_ERROR_FILE_EMPTY,
    TEO_ERROR_FILE_FORMAT,
    TEO_ERROR_FILE_NOT_FOUND,
    TEO_ERROR_FILE_OPEN,
    TEO_ERROR_FILE_READ,
    TEO_ERROR_FILE_TOO_LARGE,
    TEO_ERROR_FILE_WRITE,
    TEO_ERROR_JOYSTICK_NUM,
    TEO_ERROR_MEDIA_ALREADY_SET,
    TEO_ERROR_MEMO_HEADER_CHECKSUM,
    TEO_ERROR_MEMO_HEADER_NAME,
    TEO_ERROR_MULTIPLE_INIT,
    TEO_ERROR_UNSUPPORTED_MODEL,
    TEO_ERROR_VALID_SAP,
    TEO_ERROR
};

extern char *teo_error_msg;
extern int error_Message(int error, const char moreinfo[]);

#endif

