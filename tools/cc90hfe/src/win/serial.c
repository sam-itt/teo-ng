/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2013 Yves Charriau, François Mouret
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
 *  Module     : windows/serial.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of RS232 connection (Windows systems).
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <windows.h>
   #include <assert.h>
   #include <winioctl.h>
#endif

#include "defs.h"
#include "std.h"
#include "errors.h"
#include "serial.h"

/* The size of receiving/sending buffers
 * must be even to be compatible with
 * Windows NT, 2000, XP and Vista */
#define SERIAL_BUFFER_SIZE  1200    /* Size of receiving/sending buffer */

static HANDLE hCom = NULL;
static long timeout = SERIAL_TIME_OUT;


/* ------------------------------------------------------------------------- */


/* serial_Close:
 *  Close the serial port.
 */
void serial_Close (void)
{
    if (hCom != NULL)
    {
        CloseHandle(hCom);
        hCom = NULL;
    }
}



/* serial_RestoreTimeout:
 *  Set the normal timeout.
 */
void serial_RestoreTimeout (void)
{
    timeout = (long)gui.timeout;
}



/* serial_InfiniteTimeout:
 *  Set the infinite timeout.
 */
void serial_InfiniteTimeout (void)
{
    timeout = INFINITE;
}



/* serial_Open:
 *  Open the serial port.
 */
int serial_Open (char *port_name)
{
    DCB dcb;
    COMMTIMEOUTS cto;
    DWORD lpStatus;

    /* returns if port already open */
    if (hCom != NULL)
        serial_Close ();

    /* create the port */
    hCom = CreateFile (port_name,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       0,
                       OPEN_EXISTING,
                       FILE_FLAG_OVERLAPPED,
                       0);
    if (hCom == NULL)
        return error_Message (CC90HFE_ERROR_SERIAL_OPEN, port_name);

    /* set the port parameters */
    memset (&dcb, 0x00, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
    if (GetCommState (hCom, &dcb) == 0)
        return error_Message (CC90HFE_ERROR_SERIAL_OPEN, port_name);

    dcb.BaudRate          = CBR_38400;
    dcb.ByteSize          = 8;
    dcb.StopBits          = ONESTOPBIT;
    dcb.fParity           = NOPARITY;
    dcb.fBinary           = TRUE;
    dcb.fDtrControl       = DTR_CONTROL_ENABLE;
    dcb.fOutxDsrFlow      = FALSE;
    dcb.fDsrSensitivity   = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX             = FALSE;
    dcb.fInX              = FALSE;
    dcb.fNull             = FALSE;
    dcb.fAbortOnError     = FALSE;
    dcb.fOutxCtsFlow      = TRUE;
    dcb.fRtsControl       = RTS_CONTROL_HANDSHAKE;

    if (SetCommState (hCom, &dcb) == 0)
        return error_Message (CC90HFE_ERROR_SERIAL_OPEN, port_name);

    /* wait to be sure */ 
    Sleep(60);

    /* Set timeouts */
    cto.ReadIntervalTimeout         = 0;
    cto.ReadTotalTimeoutMultiplier  = 0;
    cto.ReadTotalTimeoutConstant    = 0;
    cto.WriteTotalTimeoutMultiplier = 0;
    cto.WriteTotalTimeoutConstant   = 0;
    if (SetCommTimeouts (hCom, &cto) == 0)
        return error_Message (CC90HFE_ERROR_SERIAL_OPEN, port_name);

    /* wait to be sure */ 
    Sleep(60);

    /* Set the size of buffers */
    if (SetupComm(hCom, SERIAL_BUFFER_SIZE, SERIAL_BUFFER_SIZE) == 0)
        return error_Message (CC90HFE_ERROR_SERIAL_OPEN, port_name);

    /* purge the buffers */
    PurgeComm (hCom,
                 PURGE_TXABORT |
                 PURGE_RXABORT |
                 PURGE_TXCLEAR |
                 PURGE_RXCLEAR);

    /* check if CTS activated */
    if ((GetCommModemStatus (hCom, &lpStatus) == 0)
     || ((lpStatus & MS_CTS_ON) == 0))
        return error_Message (CC90HFE_ERROR_SERIAL_IO, port_name);

    /* wait 1/4 of second */ 
    Sleep(250);

    /* check if CTS activated */
    if ((GetCommModemStatus (hCom, &lpStatus) == 0)
     || ((lpStatus & MS_CTS_ON) == 0))
        return error_Message (CC90HFE_ERROR_SERIAL_IO, port_name);

    return 0;
}



/* serial_Write:
 *  Send asynchronous datas to the serial port.
 */
int serial_Write (uint8 *buf, int size)
{
    int err = 0;
    DWORD nbytes = 0;
    OVERLAPPED overl;

    /* create the sending event */
    memset (&overl, 0, sizeof (OVERLAPPED));
    if ((overl.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL)) == NULL)
        return error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);

    /* send the data block */
    if (!WriteFile(hCom, buf, size, &nbytes, &overl))
    {
        if (GetLastError () != ERROR_IO_PENDING)
        {
            err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
        }
        else
        {
	       if (WaitForSingleObject (overl.hEvent, timeout) == WAIT_OBJECT_0)
	       {
               if (!GetOverlappedResult (hCom, &overl, &nbytes, FALSE))
                   err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
           }
           else
               err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
        }
    }

    /*close the sending event */
    CloseHandle(overl.hEvent);

    if (size != (int)nbytes)
        err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
        
    return err;
}



/* serial_Read:
 *  Receive asynchronous datas from the serial port.
 */
int serial_Read (uint8 *buf, int size)
{
    int err = 0;
    DWORD nbytes = 0;
    OVERLAPPED overl;

    /* create the receiving event */
    memset (&overl, 0, sizeof (OVERLAPPED));
    if ((overl.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL)) == NULL)
        return error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);

    /* receive the data block */
    if (!ReadFile(hCom, buf, size, &nbytes, &overl))
    {
        if (GetLastError () != ERROR_IO_PENDING)
        {
            err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
        }
        else
        {
	       if (WaitForSingleObject (overl.hEvent, timeout) == WAIT_OBJECT_0)
	       {
               if (!GetOverlappedResult (hCom, &overl, &nbytes, FALSE))
                   err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
           }
           else
               err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
        }
    }

    /*close the sending event */
    CloseHandle(overl.hEvent);

    if (size != (int)nbytes)
        err = error_Message (CC90HFE_ERROR_SERIAL_IO, NULL);
        
    return err;
}

