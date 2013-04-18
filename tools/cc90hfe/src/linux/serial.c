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
 *  Module     : linux/serial.c
 *  Version    : 0.5.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of RS232 connection.
 */


#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "defs.h"
#include "error.h"
#include "std.h"
#include "errors.h"
#include "serial.h"

static struct termios gOriginalTTYAttrs;
static struct termios options;
static int fileDescriptor = -1;



/* serial_Close:
 *  Close the serial port.
 */
void serial_Close(void)
{
    if (fileDescriptor >= 0)
    {
        (void)tcdrain (fileDescriptor);
        (void)tcsetattr (fileDescriptor, TCSANOW, &gOriginalTTYAttrs);
        (void)close (fileDescriptor);
        fileDescriptor = -1;
    }
}



/* serial_RestoreTimeout:
 *  Set the normal timeout.
 */
void serial_RestoreTimeout(void)
{
    options.c_cc[VTIME] = (cc_t)gui.timeout;
    options.c_cc[VMIN] = (cc_t)0;
    (void)tcsetattr (fileDescriptor, TCSANOW, &options);
}



/* serial_InfiniteTimeout:
 *  Set the infinite timeout.
 */
void serial_InfiniteTimeout(void)
{
    options.c_cc[VTIME] = (cc_t)0;
    options.c_cc[VMIN] = (cc_t)1;
    (void)tcsetattr (fileDescriptor, TCSANOW, &options);
}



/* serial_Open:
 *  Open the serial port.
 */
int serial_Open (char *port_name)
{
    int handshake = 0;

    /* return if already open */
    if (fileDescriptor >= 0)
        return 0;

    /* open serial port */
    if (((fileDescriptor = open (port_name, O_RDWR | O_NOCTTY)) < 0)
     || (ioctl (fileDescriptor, (unsigned long int)TIOCEXCL) < 0)
     || (fcntl (fileDescriptor, F_SETFL,  NULL) < 0)
     || (tcgetattr (fileDescriptor, &gOriginalTTYAttrs) < 0))
        return error_Message (CC90HFE_ERROR_SERIAL_OPEN, NULL);

    /* set serial parameters */
    options = gOriginalTTYAttrs;
    cfmakeraw(&options);
    options.c_cc[VTIME] = (cc_t)gui.timeout;
    options.c_cc[VMIN] = (cc_t)0;
    (void)cfsetispeed (&options, B38400);
    (void)cfsetospeed (&options, B38400);
    options.c_cflag &= ~(PARENB | CSTOPB);
    options.c_cflag |= (CSIZE | CS8 | CRTSCTS | CREAD | CLOCAL);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag |= (IGNBRK | IGNPAR);
    options.c_oflag &= ~(OPOST);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    if (tcsetattr (fileDescriptor, TCSANOW, &options) < 0)
        return error_Message (CC90HFE_ERROR_SERIAL_OPEN, NULL);
    (void)usleep (150);

    /* check if CC90 already runs */
    if ((ioctl (fileDescriptor, (unsigned long int)TIOCMGET, &handshake) < 0)
     || ((handshake & TIOCM_CTS) == 0))
        return error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);

    (void)usleep (250000);

    if ((ioctl (fileDescriptor, (unsigned long int)TIOCMGET, &handshake) < 0)
     || ((handshake & TIOCM_CTS) == 0))
        return error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);

    return 0;
}



/* serial_Write:
 *  Send asynchronous datas to the serial port.
 */
int serial_Write (uint8 *buf, int size)
{
    int i;

    for (i=0; i<size; i++)
    {
        if (write (fileDescriptor, buf+i, 1) < 0)
            return error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
    }

    return 0;
}



/* serial_Read:
 *  Receive asynchronous datas from the serial port.
 */
int serial_Read (uint8 *buf, int size)
{
    int i;

    for (i=0; i<size; i++)
    {
        if (read (fileDescriptor, buf+i, 1) < 0)
            return error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
    }

    return 0;
}

