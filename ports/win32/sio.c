/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 */

#include <lwip/opt.h>
#include <lwip/sys.h>
#include <lwip/sio.h>

#include <stdio.h>
#include <stdarg.h>

#include <windows.h>

/* If this is 1, use COMx (not working yet), if 0, use a pipe (default) */
#if SIO_USE_COMPORT
#define SIO_DEVICENAME "\\\\.\\COM"
#else
#define SIO_DEVICENAME "\\\\.\\pipe\\lwip"
#endif

static int sio_abort=0;

/* \\.\pipe\lwip0 */
/* pppd /dev/ttyS0 logfile mylog debug nocrtscts local noauth noccp ms-dns 212.27.54.252 192.168.0.4:192.168.0.5
 */

/**
 * SIO_DEBUG: Enable debugging for SIO.
 */
#ifndef SIO_DEBUG
#define SIO_DEBUG    LWIP_DBG_OFF
#endif

/**
 * Opens a serial device for communication.
 * 
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
  HANDLE hPipe = INVALID_HANDLE_VALUE;
  CHAR   szPipeName[256];
  LWIP_DEBUGF(SIO_DEBUG, ("sio_open(%lu)\n", (DWORD)devnum));
  sprintf(szPipeName, SIO_DEVICENAME"%lu", (DWORD)(devnum));
  hPipe = CreateFile(szPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (hPipe != INVALID_HANDLE_VALUE) {
    sio_abort = 0;
    FlushFileBuffers(hPipe);
    return (sio_fd_t)(hPipe);
  }
  LWIP_DEBUGF(SIO_DEBUG, ("sio_open(%lu) failed. GetLastError() returns %d\n",
              (DWORD)devnum, GetLastError()));
  return NULL;
}

/**
 * Sends a single character to the serial device.
 * 
 * @param c character to send
 * @param fd serial device handle
 * 
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
  DWORD dwNbBytesWritten = 0;
  LWIP_DEBUGF(SIO_DEBUG, ("sio_send(%lu)\n", (DWORD)c));
  while ((!WriteFile((HANDLE)(fd), &c, 1, &dwNbBytesWritten, NULL)) || (dwNbBytesWritten < 1));
  return;
}

/**
 * Receives a single character from the serial device.
 * 
 * @param fd serial device handle
 * 
 * @note This function will block until a character is received.
 */
u8_t sio_recv(sio_fd_t fd)
{
  DWORD dwNbBytesReadden = 0;
  u8_t byte = 0;
  LWIP_DEBUGF(SIO_DEBUG, ("sio_recv()\n"));
  while ((sio_abort == 0) && ((!ReadFile((HANDLE)(fd), &byte, 1, &dwNbBytesReadden, NULL)) || (dwNbBytesReadden < 1)));
  LWIP_DEBUGF(SIO_DEBUG, ("sio_recv()=%lu\n", (DWORD)byte));
  return byte;
}

/**
 * Reads from the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 * 
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t* data, u32_t len)
{
  DWORD dwNbBytesReadden = 0;
  LWIP_DEBUGF(SIO_DEBUG, ("sio_read()...\n"));
  ReadFile((HANDLE)(fd), data, len, &dwNbBytesReadden, NULL);
  LWIP_DEBUGF(SIO_DEBUG, ("sio_read()=%lu bytes\n", dwNbBytesReadden));
  return dwNbBytesReadden;
}

/**
 * Writes to the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data to send
 * @param len length (in bytes) of data to send
 * @return number of bytes actually sent
 * 
 * @note This function will block until all data can be sent.
 */
u32_t sio_write(sio_fd_t fd, u8_t* data, u32_t len)
{
  DWORD dwNbBytesWritten = 0;
  LWIP_DEBUGF(SIO_DEBUG, ("sio_write()...\n"));
  WriteFile((HANDLE)(fd), data, len, &dwNbBytesWritten, NULL);
  LWIP_DEBUGF(SIO_DEBUG, ("sio_write()=%lu bytes\n", dwNbBytesWritten));
  return dwNbBytesWritten;
}

/**
 * Aborts a blocking sio_read() call.
 * @todo: This currently ignores fd and aborts all reads
 * 
 * @param fd serial device handle
 */
void sio_read_abort(sio_fd_t fd)
{
  LWIP_UNUSED_ARG(fd);
  LWIP_DEBUGF(SIO_DEBUG, ("sio_read_abort() !!!!!...\n"));
  sio_abort = 1;
  return;
}

/**
 * Used by pppdebug:
 * ppp_trace - a form of printf to send tracing information to stderr
 */
void ppp_trace( int level, const char *format, ...)
{
  int len;
  char buffer[1024];
  va_list argList;

  LWIP_UNUSED_ARG(level);

  va_start(argList, format);
  len = vsprintf(buffer, format, argList);
  buffer[len - 1] = '\0';
  va_end(argList);
  printf("%s\n", buffer);
}
