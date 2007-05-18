/*
 * Copyright (c) 2001,2002 Florian Schulze.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * test.c - This file is part of lwIPtest
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <conio.h>

#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"

#include "lwip/stats.h"

#include "lwip/tcpip.h"

#include "netif/loopif.h"

#include "arch/perf.h"

#include "httpd.h"
//#include "ftpd.h"
//#include "fs.h"

/* index of the network adapter to use for lwIP */
#define PACKET_LIB_ADAPTER_NR   4

err_t ethernetif_init(struct netif *netif);
int init_adapter(int adapter_num, char *macaddr_out);
void shutdown_adapter(void);
void update_adapter(void);

int dbg_printf(const char *fmt, ...)
{
	va_list v;
	int r;

	va_start(v, fmt);
	r = vfprintf(stderr,fmt, v);
	va_end(v);
	return r;
}

#if LWIP_TCP
static err_t netio_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	if (err == ERR_OK && p != NULL)
	{
		tcp_recved(pcb, p->tot_len);
		pbuf_free(p);
	}
	else
		pbuf_free(p);

	if (err == ERR_OK && p == NULL)
	{
		tcp_arg(pcb, NULL);
		tcp_sent(pcb, NULL);
		tcp_recv(pcb, NULL);
		tcp_close(pcb);
	}

	return ERR_OK;
}

static err_t netio_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, netio_recv);
	return ERR_OK;
}

void netio_init(void)
{
	struct tcp_pcb *pcb;

	pcb = tcp_new();
	tcp_bind(pcb, IP_ADDR_ANY, 18767);
	pcb = tcp_listen(pcb);
	tcp_accept(pcb, netio_accept);
}
#endif /* LWIP_TCP */

struct netif netif;

void main_loop()
{
	struct ip_addr ipaddr, netmask, gw;
#if NO_SYS
	int last_time;
	int timer1;
	int timer2;
#endif /* NO_SYS */
  int done;
	char mac_addr[6];

	IP4_ADDR(&gw, 192,168,1,1);
	IP4_ADDR(&ipaddr, 192,168,1,200);
	IP4_ADDR(&netmask, 255,255,255,0);
	printf("Starting lwIP, local interface IP is %s\n", inet_ntoa(*(struct in_addr*)&ipaddr));
	
  if (init_adapter(PACKET_LIB_ADAPTER_NR, mac_addr) != 0) {
    printf("ERROR initializing network adapter %d!\n", PACKET_LIB_ADAPTER_NR);
		return;
  }

	memcpy(&netif.hwaddr, mac_addr, 6);
	/* increase the last octet so that lwIP netif has a similar but different MAC addr */
	netif.hwaddr[5]++;
#if NO_SYS
	netif_set_default(netif_add(&netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, ip_input));
#else /* NO_SYS */
	netif_set_default(netif_add(&netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_ethinput));
#endif /* NO_SYS */
	netif_set_up(&netif);

	/*
	IP4_ADDR(&gw, 127,0,0,1);
	IP4_ADDR(&ipaddr, 127,0,0,1);
	IP4_ADDR(&netmask, 255,0,0,0);
	
	netif_add(&ipaddr, &netmask, &gw, loopif_init,
		ip_input);
	*/

#if NO_SYS
	tcp_init();
	udp_init();
	ip_init();
#else /* NO_SYS */
  tcpip_init(0,0);
#endif /* NO_SYS */

	httpd_init();
	netio_init();
	//ftpd_init();

#if NO_SYS
	last_time=clock();
	timer1=0;
	timer2=0;
	done=0;
#endif /* NO_SYS */

	while (!done)
	{
#if NO_SYS
		int cur_time;
		int time_diff;

		cur_time=clock();
		time_diff=cur_time-last_time;
		if (time_diff>0)
		{
			last_time=cur_time;
			timer1+=time_diff;
			timer2+=time_diff;
		}

		if (timer1>10)
		{
			tcp_fasttmr();
			timer1=0;
		}

		if (timer2>45)
		{
			tcp_slowtmr();
			timer2=0;
			done=_kbhit();
		}
#endif /* NO_SYS */

		update_adapter();
	}

	shutdown_adapter();
}

void bcopy(const void *src, void *dest, int len)
{
  memcpy(dest,src,len);
}

void bzero(void *data, int n)
{
  memset(data,0,n);
}

int main(void)
{
	setvbuf(stdout,NULL,_IONBF,0);
#ifdef PERF
	perf_init("/tmp/lwip.perf");
#endif /* PERF */
#ifdef STATS
	stats_init();
#endif /* STATS */
	sys_init();
	mem_init();
	memp_init();
	pbuf_init();
#if !NO_SYS
	lwip_socket_init();
#endif /* !NO_SYS */

	//tcpdump_init();

	printf("System initialized.\n");

	main_loop();

	return 0;
}

