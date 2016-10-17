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

#include <unistd.h>
#include <getopt.h>

#include "lwip/sio.h"
#include "netif/sio.h"

#include "lwip/opt.h"
#include "lwip/init.h"

#include "lwip/debug.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"

#include "lwip/stats.h"

#include "lwip/ip.h"
#include "lwip/mld6.h"
#include "lwip/ip4_frag.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "netif/tapif.h"
#include "netif/etharp.h"
#include "netif/ppp/pppapi.h"
#include "lwip/netifapi.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/pppoe.h"
#include "netif/ppp/pppol2tp.h"

#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_mib2.h"

#include "apps/snmp_private_mib/private_mib.h"
#include "apps/udpecho_raw/udpecho_raw.h"
#include "apps/tcpecho_raw/tcpecho_raw.h"

#include "lwip/tcpip.h"

/* (manual) host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;

#if LWIP_SNMP
/* SNMP trap destination cmd option */
static unsigned char trap_flag;
static ip_addr_t trap_addr;

static const struct snmp_mib *mibs[] = {
  &mib2,
  &mib_private
};
#endif

/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

#if 0
int to_pppd[2], from_pppd[2];
#endif

struct netif netif;
struct netif netif2;

#if PPP_SUPPORT
const char *username = "essai", *password = "aon0viipheehooX";
#if PPPOE_SUPPORT
  ppp_pcb *pppoe;
#endif
#if PPPOL2TP_SUPPORT
  ppp_pcb *pppl2tp;
#endif
#if PPPOS_SUPPORT
  ppp_pcb *pppos;
  int pppos_fd;
#endif /* PPPOS_SUPPORT */
#endif /* PPP_SUPPORT */

#if LWIP_SNMP
/* enable == 1, disable == 2 */
u8_t snmpauthentraps_set = 2;
#endif /* LWIP_SNMP */

static struct option longopts[] = {
  /* turn on debugging output (if build with LWIP_DEBUG) */
  {"debug", no_argument,        NULL, 'd'},
  /* help */
  {"help", no_argument, NULL, 'h'},
  /* gateway address */
  {"gateway", required_argument, NULL, 'g'},
  /* ip address */
  {"ipaddr", required_argument, NULL, 'i'},
  /* netmask */
  {"netmask", required_argument, NULL, 'm'},
  /* ping destination */
  {"trap_destination", required_argument, NULL, 't'},
  /* new command line options go here! */
  {NULL,   0,                 NULL,  0}
};
#define NUM_OPTS ((sizeof(longopts) / sizeof(struct option)) - 1)

static void
usage(void)
{
  unsigned char i;
   
  printf("options:\n");
  for (i = 0; i < NUM_OPTS; i++) {
    printf("-%c --%s\n",longopts[i].val, longopts[i].name);
  }
}

/* Callback executed when the TCP/IP init is done. */
static void tcpip_init_done(void *arg)
{
  sys_sem_t sem = (sys_sem_t)arg;

#if LWIP_SNMP
  /* initialize our private example MIB */
  lwip_privmib_init();

  /* snmp_trap_dst_ip_set(0,&trap_addr); */
  /* snmp_trap_dst_enable(0,trap_flag); */

#if SNMP_LWIP_MIB2
#if SNMP_USE_NETCONN
  snmp_threadsync_init(&snmp_mib2_lwip_locks, snmp_mib2_lwip_synchronizer);
#endif
  snmp_mib2_set_syscontact_readonly((const u8_t*)"root", NULL);
  snmp_mib2_set_syslocation_readonly((const u8_t*)"lwIP development PC", NULL);
  snmp_mib2_set_sysdescr((const u8_t*)"minimal example", NULL);
#endif /* SNMP_LWIP_MIB2 */

  /* snmp_set_snmpenableauthentraps(&snmpauthentraps_set); */
  snmp_set_mibs(mibs, LWIP_ARRAYSIZE(mibs));
  snmp_init();
#endif /* LWIP_SNMP */

  udpecho_raw_init();
  tcpecho_raw_init();

  printf("Applications started.\n");

  sys_sem_signal(&sem); /* Signal the waiting thread that the TCP/IP init is done. */
}

#if PPP_SUPPORT
static void ppp_notify_phase_cb(ppp_pcb *pcb, u8_t phase, void *ctx) {
	struct netif *pppif = ppp_netif(pcb);
	LWIP_UNUSED_ARG(ctx);

	switch(phase) {
		case PPP_PHASE_DEAD:
			printf("ppp_notify_phase_cb[%d]: DEAD(%d)\n", pppif->num, PPP_PHASE_DEAD);
			break;
		case PPP_PHASE_INITIALIZE:
			printf("ppp_notify_phase_cb[%d]: INITIALIZE(%d)\n", pppif->num, PPP_PHASE_INITIALIZE);
			break;
		case PPP_PHASE_SERIALCONN:
			printf("ppp_notify_phase_cb[%d]: SERIALCONN(%d)\n", pppif->num, PPP_PHASE_SERIALCONN);
			break;
		case PPP_PHASE_DORMANT:
			printf("ppp_notify_phase_cb[%d]: DORMANT(%d)\n", pppif->num, PPP_PHASE_DORMANT);
			break;
		case PPP_PHASE_ESTABLISH:
			printf("ppp_notify_phase_cb[%d]: ESTABLISH(%d)\n", pppif->num, PPP_PHASE_ESTABLISH);
			break;
		case PPP_PHASE_AUTHENTICATE:
			printf("ppp_notify_phase_cb[%d]: AUTHENTICATE(%d)\n", pppif->num, PPP_PHASE_AUTHENTICATE);
			break;
		case PPP_PHASE_CALLBACK:
			printf("ppp_notify_phase_cb[%d]: CALLBACK(%d)\n", pppif->num, PPP_PHASE_CALLBACK);
			break;
		case PPP_PHASE_NETWORK:
			printf("ppp_notify_phase_cb[%d]: NETWORK(%d)\n", pppif->num, PPP_PHASE_NETWORK);
			break;
		case PPP_PHASE_RUNNING:
			printf("ppp_notify_phase_cb[%d]: RUNNING(%d)\n", pppif->num, PPP_PHASE_RUNNING);
			break;
		case PPP_PHASE_TERMINATE:
			printf("ppp_notify_phase_cb[%d]: TERMINATE(%d)\n", pppif->num, PPP_PHASE_TERMINATE);
			break;
		case PPP_PHASE_DISCONNECT:
			printf("ppp_notify_phase_cb[%d]: DISCONNECT(%d)\n", pppif->num, PPP_PHASE_DISCONNECT);
			break;
		case PPP_PHASE_HOLDOFF:
			printf("ppp_notify_phase_cb[%d]: HOLDOFF(%d)\n", pppif->num, PPP_PHASE_HOLDOFF);
			break;
		case PPP_PHASE_MASTER:
			printf("ppp_notify_phase_cb[%d]: MASTER(%d)\n", pppif->num, PPP_PHASE_HOLDOFF);
			break;
		default:
			printf("ppp_notify_phase_cb[%d]: Unknown phase %d\n", pppif->num, phase);
			break;
	}
}

static void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx) {
	struct netif *pppif = ppp_netif(pcb);
	LWIP_UNUSED_ARG(ctx);

	switch(err_code) {
		case PPPERR_NONE: {             /* No error. */
#if PPP_IPV4_SUPPORT && LWIP_DNS
			const ip_addr_t *ns;
#endif /* PPP_IPV4_SUPPORT && LWIP_DNS */
#if PPP_IPV4_SUPPORT
			printf("ppp_link_status_cb[%d]: PPPERR_NONE\n", pppif->num);
			printf("   our_ipaddr  = %s\n", ip4addr_ntoa(netif_ip4_addr(pppif)));
			printf("   his_ipaddr  = %s\n", ip4addr_ntoa(netif_ip4_gw(pppif)));
			printf("   netmask     = %s\n", ip4addr_ntoa(netif_ip4_netmask(pppif)));
#if LWIP_DNS
			ns = dns_getserver(0);
			printf("   dns1        = %s\n", ipaddr_ntoa(ns));
			ns = dns_getserver(1);
			printf("   dns2        = %s\n", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */
#if PPP_IPV6_SUPPORT
			printf("   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#if PPPOE_SUPPORT
			if (err_code == PPPERR_NONE && pcb == pppoe) {
				IP6_ADDR((ip6_addr_t*)&pppif->ip6_addr[1], PP_HTONL(0x20010000), PP_HTONL(0x00000000), PP_HTONL(0x00000000), PP_HTONL(0x00010002));
				netif_ip6_addr_set_state(pppif, 1, IP6_ADDR_PREFERRED);
			}
#endif /* PPPOE_SUPPORT */
#endif /* PPP_IPV6_SUPPORT */
			break;
		}
		case PPPERR_PARAM: {           /* Invalid parameter. */
			printf("ppp_link_status_cb[%d]: PPPERR_PARAM\n", pppif->num);
			break;
		}
		case PPPERR_OPEN: {            /* Unable to open PPP session. */
			printf("ppp_link_status_cb[%d]: PPPERR_OPEN\n", pppif->num);
			break;
		}
		case PPPERR_DEVICE: {          /* Invalid I/O device for PPP. */
			printf("ppp_link_status_cb[%d]: PPPERR_DEVICE\n", pppif->num);
			break;
		}
		case PPPERR_ALLOC: {           /* Unable to allocate resources. */
			printf("ppp_link_status_cb[%d]: PPPERR_ALLOC\n", pppif->num);
			break;
		}
		case PPPERR_USER: {            /* User interrupt. */
			printf("ppp_link_status_cb[%d]: PPPERR_USER\n", pppif->num);
			break;
		}
		case PPPERR_CONNECT: {         /* Connection lost. */
			printf("ppp_link_status_cb[%d]: PPPERR_CONNECT\n", pppif->num);
			break;
		}
		case PPPERR_AUTHFAIL: {        /* Failed authentication challenge. */
			printf("ppp_link_status_cb[%d]: PPPERR_AUTHFAIL\n", pppif->num);
			break;
		}
		case PPPERR_PROTOCOL: {        /* Failed to meet protocol. */
			printf("ppp_link_status_cb[%d]: PPPERR_PROTOCOL\n", pppif->num);
			break;
		}
		case PPPERR_PEERDEAD: {        /* Connection timeout. */
			printf("ppp_link_status_cb[%d]: PPPERR_PEERDEAD\n", pppif->num);
			break;
		}
		case PPPERR_IDLETIMEOUT: {      /* Idle Timeout. */
			printf("ppp_link_status_cb[%d]: PPPERR_IDLETIMEOUT\n", pppif->num);
			break;
		}
		case PPPERR_CONNECTTIME: {      /* Max connect time reache. */
			printf("ppp_link_status_cb[%d]: PPPERR_CONNECTTIME\n", pppif->num);
			break;
		}
		case PPPERR_LOOPBACK: {        /* Loopback detected. */
			printf("ppp_link_status_cb[%d]: PPPERR_LOOPBACK\n", pppif->num);
			break;
		}
		default: {
			printf("ppp_link_status_cb[%d]: unknown err code %d\n", pppif->num, err_code);
			break;
		}
	}

#if PPPOE_SUPPORT
	if(err_code == PPPERR_USER && pcb == pppoe) {
		printf("Destroying PPPoE and recreating\n");
		ppp_free(pcb);
		pcb = pppoe_create(pppif, &netif, NULL, NULL, ppp_link_status_cb, NULL);
		ppp_set_notify_phase_callback(pcb, ppp_notify_phase_cb);
#if PPP_AUTH_SUPPORT
		ppp_set_auth(pcb, PPPAUTHTYPE_EAP, username, password);
#endif /* PPP_AUTH_SUPPORT */
		ppp_connect(pcb, 5);
	}
#endif

	if (err_code != PPPERR_NONE) {
#if PPPOS_SUPPORT
		if (pcb == pppos) {
			sio_fd_t ser;
#if PPP_SERVER
			ser = sio_open(3);
#else /* PPP_SERVER */
			ser = sio_open(2);
#endif /* PPP_SERVER */
			if (!ser) {
				exit(-5);
			}
			pppos_fd = ser->fd;
			printf("PPPoS FD = %d\n", pppos_fd);
		}
#endif /* PPPOS_SUPPORT */

		ppp_connect(pcb, 5);
/*		printf("ppp_free(pcb) = %d\n", ppp_free(pcb)); */
/*		printf("ppp_delete(pcb) = %d\n", ppp_delete(pcb)); */
		/* printf("ppp_connect(pcb, 5) = %d\n", ppp_connect(pcb, 5)); */
	}

/*	if(errCode != PPPERR_NONE) {
		if(ppp_desc >= 0) {
			pppOverEthernetClose(ppp_desc);
			ppp_desc = -1;
		}
	} */
}

#endif /* PPP_SUPPORT */

#if PPPOS_SUPPORT
static u32_t pppos_out(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) {
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(ctx);
  ssize_t wl;
  wl = write(pppos_fd, data, len);
  return wl < 0 ? 0 : wl;
}
#endif/* PPPOS_SUPPORT */

int
main(int argc, char **argv)
{
  int ch;
  char ip_str[16] = {0}, nm_str[16] = {0}, gw_str[16] = {0};
  sys_sem_t sem;
#if PPP_SUPPORT
#if PPPOL2TP_SUPPORT || PPPOS_SUPPORT
#if PPP_AUTH_SUPPORT
  /* const char *username2 = "essai2", *password2 = "aon0viipheehooX"; */
  const char *username2 = "essai10", *password2 = "essai10pass";
#endif /* PPP_AUTH_SUPPORT */
#endif /* PPPOL2TP_SUPPORT || PPPOS_SUPPORT */
#if PPPOE_SUPPORT
  struct netif pppnetif;
#endif
#if PPPOL2TP_SUPPORT
  struct netif pppl2tpnetif;
#endif
#if PPPOS_SUPPORT
  struct netif pppsnetif;
#endif /* PPPOS_SUPPORT */
#if 0
  int coin = 0;
#endif
#endif /* PPP_SUPPORT */

  /* startup defaults (may be overridden by one or more opts) */
  IP4_ADDR(&gw, 192,168,0,1);
  IP4_ADDR(&ipaddr, 192,168,0,2);
  IP4_ADDR(&netmask, 255,255,255,0);

#if LWIP_SNMP
  trap_flag = 0;
#endif /* LWIP_SNMP */
  /* use debug flags defined by debug.h */
  debug_flags = LWIP_DBG_OFF;
  setlinebuf(stdout);

  while ((ch = getopt_long(argc, argv, "dhg:i:m:t:", longopts, NULL)) != -1) {
    switch (ch) {
      case 'd':
        debug_flags |= (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT);
        break;
      case 'h':
        usage();
        exit(0);
        break;
      case 'g':
        ip4addr_aton(optarg, &gw);
        break;
      case 'i':
        ip4addr_aton(optarg, &ipaddr);
        break;
      case 'm':
        ip4addr_aton(optarg, &netmask);
        break;
      case 't':
#if LWIP_SNMP
        trap_flag = !0;
        /* @todo: remove this authentraps tweak 
          when we have proper SET & non-volatile mem */
        snmpauthentraps_set = 1;
        ipaddr_aton(optarg, &trap_addr);
        strncpy(ip_str, ipaddr_ntoa(&trap_addr),sizeof(ip_str));
        printf("SNMP trap destination %s\n", ip_str);
#endif /* LWIP_SNMP */
        break;
      default:
        usage();
        break;
    }
  }

  argc -= optind;
  argv += optind;

  strncpy(ip_str, ip4addr_ntoa(&ipaddr), sizeof(ip_str));
  strncpy(nm_str, ip4addr_ntoa(&netmask), sizeof(nm_str));
  strncpy(gw_str, ip4addr_ntoa(&gw), sizeof(gw_str));
  printf("Host at %s mask %s gateway %s\n", ip_str, nm_str, gw_str);


#ifdef PERF
  perf_init("/tmp/minimal.perf");
#endif /* PERF */

  sys_sem_new(&sem, 0); /* Create a new semaphore. */
  tcpip_init(tcpip_init_done, sem);
  sys_sem_wait(&sem);    /* Block until the lwIP stack is initialized. */
  sys_sem_free(&sem);    /* Free the semaphore. */

/*  lwip_init(); */

  printf("TCP/IP initialized.\n");

  netifapi_netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, tcpip_input);
  /* netifapi_set_default(&netif); */

  {
#if LWIP_IPV6
	ip6_addr_t multicast_address;
	IP6_ADDR((ip6_addr_t*)&netif.ip6_addr[1], PP_HTONL(0x20010000), PP_HTONL(0x00000000), PP_HTONL(0x00000000), PP_HTONL(0x00000002));
	netif_ip6_addr_set_state(&netif, 1, IP6_ADDR_PREFERRED);
	ip6_addr_set_solicitednode(&multicast_address, netif_ip6_addr(&netif, 1)->addr[3]);
	mld6_joingroup(netif_ip6_addr(&netif, 1), &multicast_address);
/*	netif_ip6_addr_set_state(&netif, 1, IP6_ADDR_TENTATIVE); */
	/* netif_ip6_addr_set_state(&netif, 1, IP6_ADDR_TENTATIVE|IP6_ADDR_PREFERRED); */
	netif_create_ip6_linklocal_address(&netif, 1);
#endif /* LWIP_IPV6 */
  }

  netifapi_netif_set_up(&netif);
  printf("netif %d\n", netif.num);

#if 0
  IP4_ADDR(&gw, 192,168,1,1);
  IP4_ADDR(&ipaddr, 192,168,1,2);
  IP4_ADDR(&netmask, 255,255,255,0);
  netifapi_netif_add(&netif2, &ipaddr, &netmask, &gw, NULL, tapif_init, tcpip_input);
  netifapi_netif_set_up(&netif2);
#if LWIP_IPV6
  /* netif_create_ip6_linklocal_address(&netif, 1); */
#endif
  printf("netif2 %d\n", netif2.num);
#endif

#if PPP_SUPPORT
	printf("ppp_pcb sizeof(ppp) = %ld\n", sizeof(ppp_pcb));

#if PPPOE_SUPPORT
	memset(&pppnetif, 0, sizeof(struct netif));
	pppoe = pppapi_pppoe_create(&pppnetif, &netif, NULL, NULL, ppp_link_status_cb, NULL);
	ppp_set_notify_phase_callback(pppoe, ppp_notify_phase_cb);
#if PPP_AUTH_SUPPORT
	ppp_set_auth(pppoe, PPPAUTHTYPE_MSCHAP_V2, username, password);
#endif /* PPP_AUTH_SUPPORT */
#if MPPE_SUPPORT
	ppp_set_mppe(pppoe, PPP_MPPE_ENABLE|PPP_MPPE_REFUSE_128);
#endif /* MPPE_SUPPORT */
	#if PPP_DEBUG
	printf("PPPoE ID = %d\n", pppoe->netif->num);
#endif
	/* pppoe-server skip the first packet while it is forking pppd, wait a little
	 * before sending the first LCP packet.
	 */
	ppp_set_listen_time(pppoe, 100);
	pppapi_connect(pppoe, 0);
#endif

	/* ppp_set_auth(ppp2, PPPAUTHTYPE_MSCHAP, username2, password2);
	pppapi_pppoe_open(ppp2, &netif2, NULL, NULL, ppp_link_status_cb, NULL); */
#if PPPOS_SUPPORT
	memset(&pppsnetif, 0, sizeof(struct netif));

	{
		sio_fd_t ser;
#if PPP_SERVER
		ser = sio_open(3);
#else /* PPP_SERVER */
		ser = sio_open(2);
#endif /* PPP_SERVER */
		if (!ser) {
			exit(-5);
		}
		pppos_fd = ser->fd;
		printf("PPPoS FD = %d\n", pppos_fd);
	}

	pppos = pppapi_pppos_create(&pppsnetif, pppos_out, ppp_link_status_cb, NULL);
	ppp_set_notify_phase_callback(pppos, ppp_notify_phase_cb);

	ppp_set_listen_time(pppos, 100);
#if PPP_AUTH_SUPPORT
	ppp_set_auth(pppos, PPPAUTHTYPE_MSCHAP, username2, password2);
#endif /* PPP_AUTH_SUPPORT */
	pppapi_set_default(pppos);
#if PPP_DEBUG
	printf("PPPoS ID = %d\n", pppos->netif->num);
#endif
#if PPP_SERVER
	{
#if PPP_IPV4_SUPPORT
		ip4_addr_t addr;
		IP4_ADDR(&addr,  192,168,70,1);
		ppp_set_ipcp_ouraddr(pppos, &addr);
		IP4_ADDR(&addr,  192,168,70,2);
		ppp_set_ipcp_hisaddr(pppos, &addr);
#if LWIP_DNS
		IP4_ADDR(&addr,  192,168,70,20);
		ppp_set_ipcp_dnsaddr(pppos, 0, &addr);
		IP4_ADDR(&addr,  192,168,70,21);
		ppp_set_ipcp_dnsaddr(pppos, 1, &addr);
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */
#if PPP_AUTH_SUPPORT
		ppp_set_auth_required(pppos, 1);
#endif /* PPP_AUTH_SUPPORT */
		ppp_set_silent(pppos, 1);
		pppapi_listen(pppos);
	}
#else /* PPP_SERVER */
#if LWIP_DNS
	ppp_set_usepeerdns(pppos, 1);
#endif /* LWIP_DNS */
	pppapi_connect(pppos, 0);
#endif /* PPP_SERVER */
#endif /* PPPOS_SUPPORT */

#if PPPOL2TP_SUPPORT
	{
		ip_addr_t l2tpserv;
		memset(&pppl2tpnetif, 0, sizeof(struct netif));
		printf("L2TP Started\n");

#if 0
		IP_ADDR6(&l2tpserv, 0, 0x20,0x01,0x00,0x00);
		IP_ADDR6(&l2tpserv, 1, 0x00,0x00,0x00,0x00);
		IP_ADDR6(&l2tpserv, 2, 0x00,0x00,0x00,0x00);
		IP_ADDR6(&l2tpserv, 3, 0x00,0x00,0x00,0x01);
#endif
#if PPPOE_SUPPORT
		IP_ADDR4(&l2tpserv, 192,168,4,254);
/*		IP_ADDR4(&l2tpserv, 192,168,1,1); */
/* 		IP_ADDR4(&l2tpserv, 10,1,10,0); */
		pppl2tp = pppapi_pppol2tp_create(&pppl2tpnetif, ppp_netif(pppoe), &l2tpserv, 1701, (const u8_t*)"ahah", 4, ppp_link_status_cb, NULL);
#else /* PPPOE_SUPPORT */
		IP_ADDR4(&l2tpserv, 192,168,0,1);
		pppl2tp = pppapi_pppol2tp_create(&pppl2tpnetif, &netif, &l2tpserv, 1701, (const u8_t*)"ahah", 4, ppp_link_status_cb, NULL);
#endif /* PPPOE_SUPPORT */

		ppp_set_notify_phase_callback(pppl2tp, ppp_notify_phase_cb);
#if PPP_AUTH_SUPPORT
		ppp_set_auth(pppl2tp, PPPAUTHTYPE_MSCHAP_V2, username2, password2);
#endif /* PPP_AUTH_SUPPORT */
		pppapi_set_default(pppl2tp);
#if PPP_DEBUG
		printf("PPPoL2TP ID = %d\n", pppl2tp->netif->num);
#endif
#if MPPE_SUPPORT
		ppp_set_mppe(pppl2tp, PPP_MPPE_ENABLE);
#endif /* MPPE_SUPPORT */
		ppp_connect(pppl2tp, 0);
	}
#endif

#if 0
	/* start pppd */
	switch(fork()) {
		case -1:
			perror("fork");
			 exit(-1);
		/* child */
		case 0:
			pipe(to_pppd);
			pipe(from_pppd);
			dup2(to_pppd[0],0);
			dup2(from_pppd[1],1);
			execlp("pon", "pon", "test-dialup", NULL) ;
			break;
		/* parent */
		default:
			break;
	}
#endif

#endif /* PPP_SUPPORT */

#if 0
  while (1) {
	tapif_wait(&netif, 0xFFFF);
  } 
#endif
  while (1) {
    fd_set fdset;
    struct timeval tv;
#if 0
    struct tapif *tapif, *tapif2;
#endif
    int ret;
    int maxfd = -1;

    tv.tv_sec = 1;
    tv.tv_usec = 0; /* usec_to; */

    FD_ZERO(&fdset);

#if 0
    tapif = (struct tapif *)netif.state;
    FD_SET(tapif->fd, &fdset);

    tapif2 = (struct tapif *)netif2.state;
    FD_SET(tapif2->fd, &fdset);

    maxfd = LWIP_MAX(tapif->fd, tapif2->fd);
#endif

#if PPP_SUPPORT
#if 0
    FD_SET(from_pppd[0], &fdset);
#endif
#if PPPOS_SUPPORT
    if(pppos_fd >= 0) {
      FD_SET(pppos_fd, &fdset);
      maxfd = LWIP_MAX(maxfd, pppos_fd);
    }
#endif /* PPPOS_SUPPORT */
#endif /* PPP_SUPPORT */

    ret = select( maxfd + 1, &fdset, NULL, NULL, &tv);
    if(ret > 0) {
#if 0
      if( FD_ISSET(tapif->fd, &fdset) )
        tapif_input(&netif);
      if( FD_ISSET(tapif2->fd, &fdset) )
        tapif_input(&netif2);
#endif

#if PPP_SUPPORT
#if PPPOS_SUPPORT
      if(pppos && pppos_fd >= 0 && FD_ISSET(pppos_fd, &fdset) ) {
        u8_t buffer[1024];
        int len;
        len = read(pppos_fd, buffer, 1024);
	if(len < 0) {
	  close(pppos_fd);
	  pppos_fd = -1;
	  pppapi_close(pppos, 1);
	} else {
#if PPP_INPROC_IRQ_SAFE
	  pppos_input(pppos, buffer, len);
#else /* PPP_INPROC_IRQ_SAFE */
	  pppos_input_tcpip(pppos, buffer, len);
#endif /* PPP_INPROC_IRQ_SAFE */
	}
      }
#endif /* PPPOS_SUPPORT */
#endif /* PPP_SUPPORT */
    }

#if 0
	coin++;
        if(!(coin%1000)) printf("COIN %d\n", coin);
	if(coin == 2000) {
#if PPPOE_SUPPORT
		pppapi_close(pppoe, 0);
#endif
		/* pppapi_close(ppps, 0); */
		/* printf("pppapi_close(ppp, 0) = %d\n", pppapi_close(ppp, 0)); */
	}
#if 0
	if( !(coin % 2000)) {
		pppapi_close(ppp, 0);
	}
#endif

#endif
  }

#if (NO_SYS == 1)
  while (1) {
    /* poll netif, pass packet to lwIP */
    tapif_select(&netif);

    sys_check_timeouts();
  }
#endif

  return 0;
}
