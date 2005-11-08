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
 *
 */

#include <unistd.h>
#include <fcntl.h>

#include "lwip/opt.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"

#include "lwip/stats.h"


#include "lwip/tcpip.h"

#include "netif/tapif.h"
#include "netif/tunif.h"

#include "netif/unixif.h"
#include "netif/dropif.h"
#include "netif/pcapif.h"
#include "netif/loopif.h"

#include "netif/tcpdump.h"

#if PPP_SUPPORT
#include "netif/ppp/ppp.h"
#define PPP_PTY_TEST 1
#include <termios.h>
#endif

#include "lwip/ip_addr.h"

#include "arch/perf.h"

#include "httpd.h"
#include "udpecho.h"
#include "tcpecho.h"
#include "shell.h"

#if LWIP_RAW
#include "lwip/icmp.h"
#include "lwip/raw.h"
#include "lwip/sockets.h"
#endif

/*-----------------------------------------------------------------------------------*/
static void
tcp_timeout(void *data)
{
#if TCP_DEBUG
  tcp_debug_print_pcbs();
#endif /* TCP_DEBUG */
  sys_timeout(5000, tcp_timeout, NULL);
}
/*-----------------------------------------------------------------------------------*/
static void
tcpip_init_done(void *arg)
{
  sys_sem_t *sem;
  sem = arg;
  sys_sem_signal(*sem);
}

#if PPP_SUPPORT
void 
pppLinkStatusCallback(void *ctx, int errCode, void *arg)
{
    switch(errCode) {
	case PPPERR_NONE:               /* No error. */
	    {
		struct ppp_addrs *ppp_addrs = arg;

		printf("pppLinkStatusCallback: PPPERR_NONE");
		printf(" our_ipaddr=%s", _inet_ntoa(ppp_addrs->our_ipaddr.addr));
		printf(" his_ipaddr=%s", _inet_ntoa(ppp_addrs->his_ipaddr.addr));
		printf(" netmask=%s", _inet_ntoa(ppp_addrs->netmask.addr));
		printf(" dns1=%s", _inet_ntoa(ppp_addrs->dns1.addr));
		printf(" dns2=%s\n", _inet_ntoa(ppp_addrs->dns2.addr));
	    }
	    break;

	case PPPERR_PARAM:             /* Invalid parameter. */
	    printf("pppLinkStatusCallback: PPPERR_PARAM\n");
	    break;

	case PPPERR_OPEN:              /* Unable to open PPP session. */
	    printf("pppLinkStatusCallback: PPPERR_OPEN\n");
	    break;

	case PPPERR_DEVICE:            /* Invalid I/O device for PPP. */
	    printf("pppLinkStatusCallback: PPPERR_DEVICE\n");
	    break;

	case PPPERR_ALLOC:             /* Unable to allocate resources. */
	    printf("pppLinkStatusCallback: PPPERR_ALLOC\n");
	    break;

	case PPPERR_USER:              /* User interrupt. */
	    printf("pppLinkStatusCallback: PPPERR_USER\n");
	    break;

	case PPPERR_CONNECT:           /* Connection lost. */
	    printf("pppLinkStatusCallback: PPPERR_CONNECT\n");
	    break;

	case PPPERR_AUTHFAIL:          /* Failed authentication challenge. */
	    printf("pppLinkStatusCallback: PPPERR_AUTHFAIL\n");
	    break;

	case PPPERR_PROTOCOL:          /* Failed to meet protocol. */
	    printf("pppLinkStatusCallback: PPPERR_PROTOCOL\n");
	    break;

	default:
	    printf("pppLinkStatusCallback: unknown errCode %d\n", errCode);
	    break;
    }
}
#endif

/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
#if LWIP_RAW

static int seq_num;

#if 0
/* Ping using the raw ip */
static int
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, struct ip_addr *addr)
{
  printf("ping recv\n");
  return 1; /* eat the event */
}

static void
ping_send(struct raw_pcb *raw, struct ip_addr *addr)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;

  p = pbuf_alloc(PBUF_IP,sizeof(struct icmp_echo_hdr),PBUF_RAM);
  if (!p) return;

  iecho = p->payload;
  ICMPH_TYPE_SET(iecho,ICMP_ECHO);
  iecho->chksum = 0;
  iecho->seqno = htons(seq_num);

  iecho->chksum = inet_chksum(iecho, p->len);
  raw_send_to(raw,p,addr);

  pbuf_free(p);

  seq_num++;
}

static void
ping_thread(void *arg)
{
  struct raw_pcb *raw;
  struct ip_addr dest_addr;

  if (!(raw = raw_new(IP_PROTO_ICMP))) return;

  raw_recv(raw,ping_recv,NULL);

  IP4_ADDR(&dest_addr,192,168,2,1);

  while (1)
  {
    printf("ping send\n");
    ping_send(raw,&dest_addr);
    sleep(1);
  }
  /* Never reaches this */
  raw_remove(raw);
}
#else
/* Ping using the socket ip */

static void
ping_send(int s, struct ip_addr *addr)
{
  struct icmp_echo_hdr *iecho;
  struct sockaddr_in to;

  if (!(iecho = malloc(sizeof(struct icmp_echo_hdr))))
    return;

  ICMPH_TYPE_SET(iecho,ICMP_ECHO);
  iecho->chksum = 0;
  iecho->seqno = htons(seq_num);
  iecho->chksum = inet_chksum(iecho, sizeof(*iecho));

  to.sin_len = sizeof(to);
  to.sin_family = AF_INET;
  to.sin_addr.s_addr = addr->addr;

  lwip_sendto(s,iecho,sizeof(*iecho),0,(struct sockaddr*)&to,sizeof(to));

  free(iecho);
  seq_num++;
}

static void
ping_recv(int s, struct ip_addr *addr)
{
  char buf[200];
  int fromlen,len;
  struct sockaddr_in from;

  len = lwip_recvfrom(s, buf,sizeof(buf),0,(struct sockaddr*)&from,&fromlen);

  printf("Received %d bytes from %lx\n",len,ntohl(from.sin_addr.s_addr));
}

static void
ping_thread(void *arg)
{
  int s;
  struct ip_addr dest_addr;

  if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
    return;
  }

  IP4_ADDR(&dest_addr,192,168,2,1);

  while (1) {
    printf("sending ping\n");
    ping_send(s,&dest_addr);
    ping_recv(s,&dest_addr);
    sleep(1);
  }
}
#endif

#endif

struct netif netif;
#if LWIP_HAVE_LOOPIF
struct netif loopif;
#endif
/*-----------------------------------------------------------------------------------*/
static void
main_thread(void *arg)
{
  struct ip_addr ipaddr, netmask, gw;
  sys_sem_t sem;
#if PPP_SUPPORT
  sio_fd_t ppp_sio;
#endif

  netif_init();

  sem = sys_sem_new(0);
  tcpip_init(tcpip_init_done, &sem);
  sys_sem_wait(sem);
  sys_sem_free(sem);
  printf("TCP/IP initialized.\n");
#if PPP_SUPPORT
  pppInit();
#if PPP_PTY_TEST
  ppp_sio = sio_open(2);
#else
  ppp_sio = sio_open(0);
#endif
  if(!ppp_sio)
  {
      perror("Error opening device: ");
      exit(1);
  }

#ifdef LWIP_PPP_CHAP_TEST
  pppSetAuth(PPPAUTHTYPE_CHAP, "lwip", "mysecret");
#endif

  pppOpen(ppp_sio, pppLinkStatusCallback, NULL);
#endif /* PPP_SUPPORT */
  
#if LWIP_DHCP
  {
    IP4_ADDR(&gw, 0,0,0,0);
    IP4_ADDR(&ipaddr, 0,0,0,0);
    IP4_ADDR(&netmask, 0,0,0,0);

    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init,
		      tcpip_input);
    netif_set_default(&netif);
    dhcp_init();
    dhcp_start(&netif);
  }
#else
  IP4_ADDR(&gw, 192,168,0,1);
  IP4_ADDR(&ipaddr, 192,168,0,2);
  IP4_ADDR(&netmask, 255,255,255,0);
  
  netif_set_default(netif_add(&netif,&ipaddr, &netmask, &gw, NULL, tapif_init,
			      tcpip_input));

  netif_set_up(&netif);

#endif
  /* Only used for testing purposes: */
  /*  IP4_ADDR(&gw, 193,10,66,1);
  IP4_ADDR(&ipaddr, 193,10,66,107);
  IP4_ADDR(&netmask, 255,255,252,0);
  
  netif_add(&ipaddr, &netmask, &gw, NULL, pcapif_init,
  tcpip_input);*/

#if LWIP_HAVE_LOOPIF  
  IP4_ADDR(&gw, 127,0,0,1);
  IP4_ADDR(&ipaddr, 127,0,0,1);
  IP4_ADDR(&netmask, 255,0,0,0);
  
  netif_set_default(netif_add(&loopif, &ipaddr, &netmask, &gw, NULL, loopif_init,
	    tcpip_input));
#endif
  
#if LWIP_TCP  
  tcpecho_init();
  shell_init();
  httpd_init();
#endif
#if LWIP_UDP  
  udpecho_init();
#endif  

#if LWIP_RAW
  sys_thread_new(ping_thread, NULL, DEFAULT_THREAD_PRIO);
#endif

  printf("Applications started.\n");

  /*  sys_timeout(5000, tcp_timeout, NULL);*/

#ifdef MEM_PERF
  mem_perf_init("/tmp/memstats.client");
#endif /* MEM_PERF */
#if 0
	stats_display();
#endif
  /* Block for ever. */
  sem = sys_sem_new(0);
  sys_sem_wait(sem);
}
/*-----------------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
#ifdef PERF
  perf_init("/tmp/simhost.perf");
#endif /* PERF */
#if LWIP_STATS
  stats_init();
#endif /* STATS */
  sys_init();
  mem_init();
  memp_init();
  pbuf_init();
#ifdef LWIP_TCPDUMP
  tcpdump_init();
#endif
  
  printf("System initialized.\n");
    
  sys_thread_new(main_thread, NULL, DEFAULT_THREAD_PRIO);
  pause();
  return 0;
}
/*-----------------------------------------------------------------------------------*/








