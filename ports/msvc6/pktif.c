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
 * pktif.c - This file is part of lwIPtest
 *
 ****************************************************************************
 *
 * This file is derived from an example in lwIP with the following license:
 *
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* get the windows definitions of the following 4 functions out of the way */
#include <stdlib.h>
#include <stdio.h>

#include "lwip/debug.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/ip.h"

#include "netif/etharp.h"

#undef NETIF_DEBUG

/* Define those to better describe your network interface. */
#define IFNAME0 'p'
#define IFNAME1 'k'

struct ethernetif {
	struct eth_addr *ethaddr;
	/* Add whatever per-interface state that is needed here. */
};

static const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};

/* Forward declarations. */
static void  ethernetif_input(struct netif *netif);
static err_t ethernetif_output(struct netif *netif, struct pbuf *p,
			       struct ip_addr *ipaddr);

static struct netif *pktif_netif;

extern unsigned char ethaddr[6];
extern unsigned char *cur_packet;
extern int cur_length;
extern int packet_send(void *buffer, int len);

/*-----------------------------------------------------------------------------------*/
static void
low_level_init(struct netif *netif)
{
	struct ethernetif *ethernetif;

	ethernetif = netif->state;

	memcpy(&ethaddr,ethernetif->ethaddr,6);

#ifdef NETIF_DEBUG
	LWIP_DEBUGF(NETIF_DEBUG, ("pktif: eth_addr %02X%02X%02X%02X%02X%02X\n",ethernetif->ethaddr->addr[0],ethernetif->ethaddr->addr[1],ethernetif->ethaddr->addr[2],ethernetif->ethaddr->addr[3],ethernetif->ethaddr->addr[4],ethernetif->ethaddr->addr[5]));
#endif /* NETIF_DEBUG */
	/* Do whatever else is needed to initialize interface. */

	pktif_netif=netif;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct netif *ethernetif, struct pbuf *p)
{
  struct pbuf *q;
  unsigned char buffer[1600];
	unsigned char *ptr;

  /* initiate transfer(); */
  if (p->tot_len>=1600)
 		return ERR_BUF;
	ptr=buffer;
  for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    /* send data from(q->payload, q->len); */
#ifdef NETIF_DEBUG
		LWIP_DEBUGF(NETIF_DEBUG, ("netif: send ptr %p q->payload %p q->len %i q->next %p\n", ptr, q->payload, (int)q->len, q->next));
#endif
		memcpy(ptr,q->payload,q->len);
		ptr+=q->len;
  }

  /* signal that packet should be sent(); */

  if (packet_send(buffer, p->tot_len) < 0)
	  return ERR_BUF;

#ifdef LINK_STATS
  lwip_stats.link.xmit++;
#endif /* LINK_STATS */
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct netif *ethernetif)
{
  struct pbuf *p, *q;
  int start, length;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  length = cur_length;
	if (length<=0)
		return NULL;

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_LINK, (u16_t)length, PBUF_POOL);
#ifdef NETIF_DEBUG
	LWIP_DEBUGF(NETIF_DEBUG, ("netif: recv length %i p->tot_len %i\n", length, (int)p->tot_len));
#endif
	
  if (p != NULL) {
    /* We iterate over the pbuf chain until we have read the entire
       packet into the pbuf. */
		start=0;
    for(q = p; q != NULL; q = q->next) {
      /* Read enough bytes to fill this pbuf in the chain. The
         available data in the pbuf is given by the q->len
         variable. */
      /* read data into(q->payload, q->len); */
#ifdef NETIF_DEBUG
			LWIP_DEBUGF(NETIF_DEBUG, ("netif: recv start %i length %i q->payload %p q->len %i q->next %p\n", start, length, q->payload, (int)q->len, q->next));
#endif
      memcpy(q->payload,&cur_packet[start],q->len);
			start+=q->len;
			length-=q->len;
			if (length<=0)
				break;
    }
    /* acknowledge that packet has been read(); */
    cur_length=0;
#ifdef LINK_STATS
    lwip_stats.link.recv++;
#endif /* LINK_STATS */
  } else {
    /* drop packet(); */
    cur_length=0;
#ifdef LINK_STATS
    lwip_stats.link.memerr++;
    lwip_stats.link.drop++;
#endif /* LINK_STATS */
  }

  return p;
}
/*-----------------------------------------------------------------------------------*/
/*
 * ethernetif_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actuall transmission of the packet.
 *
 */
/*-----------------------------------------------------------------------------------*/
static err_t
ethernetif_output(struct netif *netif, struct pbuf *p,
		  struct ip_addr *ipaddr)
{
  return etharp_output(netif, ipaddr, p);
}
/*-----------------------------------------------------------------------------------*/
/*
 * ethernetif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
ethernetif_input(struct netif *netif)
{
  struct ethernetif *ethernetif;
  struct eth_hdr *ethhdr;
  struct pbuf *p;


  ethernetif = netif->state;

  p = low_level_input(netif);

  if (p != NULL) {

#ifdef LINK_STATS
    lwip_stats.link.recv++;
#endif /* LINK_STATS */

    ethhdr = p->payload;
    
    switch (htons(ethhdr->type)) {
    case ETHTYPE_IP:
      etharp_ip_input(netif, p);
      pbuf_header(p, -14);
      netif->input(p, netif);
      break;
    case ETHTYPE_ARP:
      etharp_arp_input(netif, ethernetif->ethaddr, p);
      break;
    default:
      pbuf_free(p);
      break;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
static void
arp_timer(void *arg)
{
  etharp_tmr();
  sys_timeout(ARP_TMR_INTERVAL, (sys_timeout_handler)arp_timer, NULL);
}
/*-----------------------------------------------------------------------------------*/
/*
 * ethernetif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
ethernetif_init(struct netif *netif)
{
  struct ethernetif *ethernetif;

  ethernetif = mem_malloc(sizeof(struct ethernetif));
  netif->state = ethernetif;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->linkoutput = low_level_output;
  netif->output = ethernetif_output;

  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST;
  netif->hwaddr_len = 6;
  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

  low_level_init(netif);
  etharp_init();

  sys_timeout(ARP_TMR_INTERVAL, (sys_timeout_handler)arp_timer, NULL);
  
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/*
 * pktif_update():
 *
 * Needs to be called periodically to get new packets. This could
 * be done inside a thread.
 */
/*-----------------------------------------------------------------------------------*/
void process_input(void)
{
  ethernetif_input(pktif_netif);
}


