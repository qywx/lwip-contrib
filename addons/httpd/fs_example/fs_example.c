/**
 * @file
 * HTTPD custom file system example
 *
 * This file demonstrates how to add support for an external file system to httpd.
 * It provides access to the specified root directory and uses stdio.h file functions
 * to read files.
 *
 * ATTENTION: This implementation is *not* secure: no checks are added to ensure
 * files are only read below the specified root directory!
 */
 
 /*
 * Copyright (c) 2017 Simon Goldschmidt
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

#include "lwip/opt.h"
#include "fs_example.h"

#include "lwip/apps/fs.h"

#include <stdio.h>
#include <string.h>

/** define LWIP_HTTPD_EXAMPLE_CUSTOMFILES to 1 to enable this file system*/
#ifndef LWIP_HTTPD_EXAMPLE_CUSTOMFILES
#define LWIP_HTTPD_EXAMPLE_CUSTOMFILES 0
#endif

#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES

const char* fs_ex_root_dir;

void
fs_ex_init(const char *httpd_root_dir)
{
  fs_ex_root_dir = strdup(httpd_root_dir);
}

#if LWIP_HTTPD_CUSTOM_FILES
int
fs_open_custom(struct fs_file *file, const char *name)
{
  char full_filename[256];
  FILE *f;

  snprintf(full_filename, 255, "%s%s", fs_ex_root_dir, name);
  full_filename[255] = 0;

  f = fopen(full_filename, "rb");
  if (f != NULL) {
    if (!fseek(f, 0, SEEK_END)) {
      int len = (int)ftell(f);
      if(!fseek(f, 0, SEEK_SET)) {
        memset(file, 0, sizeof(struct fs_file));
        file->len = len;
        file->pextension = f;
        return 1;
      }
    }
    fclose(f);
  }
  return 0;
}

void
fs_close_custom(struct fs_file *file)
{
  if (file && file->pextension) {
    fclose((FILE*)file->pextension);
    file->pextension = NULL;
  }
}

#if LWIP_HTTPD_FS_ASYNC_READ
u8_t
fs_canread_custom(struct fs_file *file)
{
  /* This function is only necessary for asynchronous I/O:
     If reading would block, return 0 and implement fs_wait_read_custom() to call the
     supplied callback if reading works. */
  LWIP_UNUSED_ARG(file);
  return 1;
}

u8_t
fs_wait_read_custom(struct fs_file *file, fs_wait_cb callback_fn, void *callback_arg)
{
  /* not implemented in this example */
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(callback_fn);
  LWIP_UNUSED_ARG(callback_arg);
  /* Return
     - 1 if ready to read (at least one byte)
     - 0 if reading should be delayed (call 'tcpip_callback(callback_fn, callback_arg)' when ready) */
  return 0;
}

int
fs_read_async_custom(struct fs_file *file, char *buffer, int count, fs_wait_cb callback_fn, void *callback_arg)
{
  FILE *f = (FILE*)file->pextension;
  int len = (int)fread(buffer, 1, count, f);

  LWIP_UNUSED_ARG(callback_fn);
  LWIP_UNUSED_ARG(callback_arg);

  file->index += len;

  /* Return
     - FS_READ_EOF if all bytes have been read
     - FS_READ_DELAYED if reading is delayed (call 'tcpip_callback(callback_fn, callback_arg)' when done) */
  return len;
}

#else /* LWIP_HTTPD_FS_ASYNC_READ */
int
fs_read_custom(struct fs_file *file, char *buffer, int count)
{
  FILE *f = (FILE*)file->pextension;
  int len = (int)fread(buffer, 1, count, f);

  file->index += len;

  /* Return FS_READ_EOF if all bytes have been read */
  return len;
}

#endif /* LWIP_HTTPD_FS_ASYNC_READ */
#endif /* LWIP_HTTPD_CUSTOM_FILES */

#endif /* LWIP_HTTPD_EXAMPLE_CUSTOMFILES */
