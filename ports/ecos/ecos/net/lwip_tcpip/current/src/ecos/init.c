/*
 * init.c - helper code for initing applications that use lwIP		
 */

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwipopts.h"

#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif


void inline IP_ADDR(struct ip_addr *ipaddr, char a, char b, char c, char d)
{
	IP4_ADDR(ipaddr,a,b,c,d);
}


void tcpip_init_done(void * arg)
{
	sys_sem_t *sem = arg;
	sys_sem_signal(*sem);
}	

struct netif mynetif;

extern err_t ecosif_init(struct netif *);	
/*
 * Called by the eCos application at startup
 * wraps various init calls
 */
void lwip_init(void)
{
	sys_sem_t sem;
	
	sys_init();	/* eCos specific initialization */
	mem_init();	/* heap based memory allocator */
	memp_init();	/* pool based memory allocator */
	pbuf_init();	/* packet buffer allocator */
	netif_init();	/* netif layer */
	
	/* Start the stack.It will spawn a new dedicated thread */
	sem = sys_sem_new(0);
	tcpip_init(tcpip_init_done,&sem);
	sys_sem_wait(sem);
	sys_sem_free(sem);
#if LWIP_SLIP	
	lwip_set_addr(&mynetif);
	slipif_init(&mynetif);
#else	
	ecosglue_init();		
#endif	
//	ecosif_init(&mynetif);
}

void lwip_set_addr(struct netif *netif)
{
	struct ip_addr ipaddr, netmask, gw;

	IP_ADDR(&gw, CYGPKG_LWIP_SERV_ADDR);
	IP_ADDR(&ipaddr, CYGPKG_LWIP_MY_ADDR);
	IP_ADDR(&netmask, CYGPKG_LWIP_NETMASK);
	netif_set_addr(netif, &ipaddr, &netmask, &gw);
	netif->next = NULL;
	netif_list = netif;
	
	netif->input = tcpip_input;
}
