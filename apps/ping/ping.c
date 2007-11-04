#include "ping.h"

/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

#if LWIP_RAW

/* lwIP ping identifier */
#define LWIP_PING_ID 0xAFAF

/* ping variables */
static int   ping_seq_num;
static u32_t ping_time;

#if NO_SYS
/* port-defined functions used for timer execution */
void sys_msleep(u32_t ms);
u32_t sys_now();
#endif /* NO_SYS */

#if LWIP_SOCKET
/* Ping using the socket ip */
static void
ping_send(int s, struct ip_addr *addr)
{
  struct icmp_echo_hdr *iecho;
  struct sockaddr_in to;

  if (!(iecho = mem_malloc(sizeof(struct icmp_echo_hdr)))) {
    return;
  }

  ICMPH_TYPE_SET(iecho,ICMP_ECHO);
  iecho->chksum = 0;
  iecho->id     = LWIP_PING_ID;
  iecho->seqno  = htons(++ping_seq_num);
  iecho->chksum = inet_chksum(iecho, sizeof(*iecho));

  to.sin_len = sizeof(to);
  to.sin_family = AF_INET;
  to.sin_addr.s_addr = addr->addr;

  lwip_sendto(s, iecho, sizeof(*iecho), 0, (struct sockaddr*)&to, sizeof(to));

  mem_free(iecho);
}

static void
ping_recv(int s, struct ip_addr *addr)
{
  char buf[256];
  int fromlen, len;
  struct sockaddr_in from;
  struct icmp_echo_hdr *iecho;

  len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromlen);

  if (len >= sizeof(struct icmp_echo_hdr)) {
    iecho = (struct icmp_echo_hdr *)buf;
    if ((iecho->id == LWIP_PING_ID) && (iecho->seqno == htons(ping_seq_num))) {
      LWIP_DEBUGF( LWIP_DBG_ON, ("ping recv "));
      ip_addr_debug_print(LWIP_DBG_ON, (struct ip_addr *)&(from.sin_addr));
      LWIP_DEBUGF( LWIP_DBG_ON, (" %lu ms\n", (sys_now()-ping_time)));
    }
  }
}

static void
ping_thread(void *arg)
{
  int s;

  if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
    return;
  }

  while (1) {
    LWIP_DEBUGF( LWIP_DBG_ON, ("ping send "));
    ip_addr_debug_print(LWIP_DBG_ON, &(netif_default->gw));
    LWIP_DEBUGF( LWIP_DBG_ON, ("\n"));
    ping_send(s, &(netif_default->gw));
    ping_time = sys_now();
    ping_recv(s, &(netif_default->gw));
    sys_msleep(1000);
  }
}

#else /* LWIP_SOCKET */

/* Ping using the raw ip */
static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, struct ip_addr *addr)
{
  struct icmp_echo_hdr *iecho;

  if (pbuf_header( p, -PBUF_IP_HLEN)==0) {
    iecho = p->payload;

    if ((iecho->id == LWIP_PING_ID) && (iecho->seqno == htons(ping_seq_num))) {
      LWIP_DEBUGF( LWIP_DBG_ON, ("ping recv "));
      ip_addr_debug_print(LWIP_DBG_ON, addr);
      LWIP_DEBUGF( LWIP_DBG_ON, (" %lu ms\n", (sys_now()-ping_time)));
    }
  }

  return 1; /* eat the event */
}

static void
ping_send(struct raw_pcb *raw, struct ip_addr *addr)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;

  if (!(p = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr), PBUF_RAM))) {
    return;
  }

  iecho = p->payload;
  ICMPH_TYPE_SET(iecho,ICMP_ECHO);
  iecho->chksum = 0;
  iecho->id     = LWIP_PING_ID;
  iecho->seqno  = htons(++ping_seq_num);
  iecho->chksum = inet_chksum(iecho, p->len);

  ping_time     = sys_now();

  raw_sendto(raw,p,addr);

  pbuf_free(p);
}

static void
ping_thread(void *arg)
{
  struct raw_pcb *raw;

  if (!(raw = raw_new(IP_PROTO_ICMP))) {
    return;
  }

  raw_recv(raw,ping_recv,NULL);
  raw_bind(raw, IP_ADDR_ANY);

  while (1) {
    LWIP_DEBUGF( LWIP_DBG_ON, ("ping send "));
    ip_addr_debug_print(LWIP_DBG_ON, &(netif_default->gw));
    LWIP_DEBUGF( LWIP_DBG_ON, ("\n"));
    ping_send(raw, &(netif_default->gw));
    sys_msleep(1000);
  }

  /* Never reaches this */
  raw_remove(raw);
}

#endif /* LWIP_SOCKET */

void
ping_init(void)
{ sys_thread_new("ping_thread", ping_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

#endif /* LWIP_RAW */
