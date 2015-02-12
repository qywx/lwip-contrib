/*
 * Copyright (c) 2006 Christiaan Simons.
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
 * This file is part of and a contribution to the lwIP TCP/IP stack.
 *
 * Author: Christiaan Simons <christiaan.simons@axon.tv>
 */
 
/** 
 * @file
 * Simple pre-allocated interval (reload) timers.
 * @see timer.h
 */

#include <stddef.h>
#include <signal.h>
#include <sys/time.h>
#include "timer.h"
#include "lwip/snmp.h"

static struct itimerval tmr;

void sigalarm_handler(int sig);

/**
 * Initializes interval timers.
 */
void
timer_init(void)
{
  signal(SIGALRM,sigalarm_handler);

  /* timer reload is in 10msec steps */
  tmr.it_interval.tv_sec = 0;
  tmr.it_interval.tv_usec = 10000;
  /* set to half period (enables timer) */
  tmr.it_value.tv_sec = 0;
  tmr.it_value.tv_usec = 5000;

  setitimer(ITIMER_REAL,&tmr,NULL);
}

/**
 * interrupting (!) sigalarm handler
 */
void
sigalarm_handler(int sig)
{
  snmp_inc_sysuptime();
}

