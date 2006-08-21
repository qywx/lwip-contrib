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
 * Author: Adam Dunkels <adam@sics.se>
 * RT timer modifications by Christiaan Simons
 */

#include "lwip/debug.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"

#include "lwip/stats.h"

#include "lwip/ip.h"
#include "lwip/ip_frag.h"
#include "lwip/udp.h"
#include "lwip/snmp_msg.h"
#include "lwip/tcp.h"

#include "mintapif.h"
#include "netif/etharp.h"

#include "timer.h"
#include <signal.h>

#include "echo.h"

int
main(int argc, char **argv)
{
  struct ip_addr ipaddr, netmask, gw;
  struct netif netif;
  sigset_t mask, oldmask, empty;
        
#ifdef PERF
  perf_init("/tmp/minimal.perf");
#endif /* PERF */
#ifdef STATS
  stats_init();
#endif /* STATS */

  mem_init();
  memp_init();
  pbuf_init(); 
  netif_init();
  ip_init();
  udp_init();
  snmp_init();
  tcp_init();
  printf("TCP/IP initialized.\n");
  
  IP4_ADDR(&gw, 192,168,0,1);
  IP4_ADDR(&ipaddr, 192,168,0,2);
  IP4_ADDR(&netmask, 255,255,255,0);

  netif_add(&netif, &ipaddr, &netmask, &gw, NULL, mintapif_init, ip_input);
  
  netif_set_default(&netif);

  netif_set_up(&netif);

  echo_init();
  
  timer_init();
  timer_set_interval(TIMER_EVT_ETHARPTMR,2000);
  timer_set_interval(TIMER_EVT_TCPFASTTMR, TCP_FAST_INTERVAL / 10);
  timer_set_interval(TIMER_EVT_TCPSLOWTMR, TCP_SLOW_INTERVAL / 10);
#if IP_REASSEMBLY
  timer_set_interval(TIMER_EVT_IPREASSTMR,100);
#endif
  
  printf("Applications started.\n");
    

  while (1) {
    
      /* poll for input packet and ensure
         select() or read() arn't interrupted */
      sigemptyset(&mask);
      sigaddset(&mask, SIGALRM);
      sigprocmask(SIG_BLOCK, &mask, &oldmask);

      /* start of critical section,
         poll netif, pass packet to lwIP */
      if (mintapif_select(&netif) > 0)
      {
        /* work, immediatly end critical section 
           hoping lwIP ended quickly ... */
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
      }
      else
      {
        /* no work, wait a little (10 msec) for SIGALRM */
          sigemptyset(&empty);
          sigsuspend(&empty);
        /* ... end critical section */
          sigprocmask(SIG_SETMASK, &oldmask, NULL);
      }
    
      if(timer_testclr_evt(TIMER_EVT_TCPFASTTMR))
      {
        tcp_fasttmr();
      }
      if(timer_testclr_evt(TIMER_EVT_TCPSLOWTMR))
      {
        tcp_slowtmr();
      }      
#if IP_REASSEMBLY
      if(timer_testclr_evt(TIMER_EVT_IPREASSTMR))
      {
        ip_reass_tmr();
      }
#endif
      if(timer_testclr_evt(TIMER_EVT_ETHARPTMR))
      {
        etharp_tmr();
      }
      
  }
  
  return 0;
}
