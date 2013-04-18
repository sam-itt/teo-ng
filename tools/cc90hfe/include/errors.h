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
 *  Module     : errors.h
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of errors.
 */
 
 
#ifndef ERRORS_H
#define ERRORS_H 1

enum {
    CC90HFE_ERROR_ALLOC = -1000,
    CC90HFE_ERROR_CC90_VERSION,
    CC90HFE_ERROR_CC90_BSTART,
    CC90HFE_ERROR_CC90_CRC,
    CC90HFE_ERROR_CC90_MEMORY,
    CC90HFE_ERROR_CC90_THMFC1,
    CC90HFE_ERROR_DISK_ACCESS,
    CC90HFE_ERROR_DISK_CONVERSION,
    CC90HFE_ERROR_DISK_IO,
    CC90HFE_ERROR_DISK_NONE,
    CC90HFE_ERROR_DISK_PROTECT,
    CC90HFE_ERROR_FILE_FORMAT,
    CC90HFE_ERROR_FILE_NOT_FOUND,
    CC90HFE_ERROR_FILE_OPEN,
    CC90HFE_ERROR_FILE_READ,
    CC90HFE_ERROR_FILE_WRITE,
    CC90HFE_ERROR_PORT_NONE,
    CC90HFE_ERROR_SERIAL_OPEN,
    CC90HFE_ERROR_SERIAL_IO,
    CC90HFE_ERROR
};

extern char *error_msg;
extern int error_Message(int error, const char moreinfo[]);

#endif

