/*
*********************************************************************************************************
*                                              lwIP TCP/IP Stack
*                                    	 port for uC/OS-II RTOS on TIC6711 DSK
*
* File : tcp_ip.c
* By   : ZengMing @ DEP,Tsinghua University,Beijing,China
* Reference: YangYe's source code for SkyEye project
*********************************************************************************************************
*/
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>

#include "netif/etharp.h"

#include "netif/ne2kif.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 't'

#define		DELAY							0x590b2  //0.5s test by ming
#define		DELAY_2S						0xbf000  //2s test 
#define     DELAY_MS						0x38F4   //20ms test 	

struct ne2k_if {
  struct eth_addr *ethaddr; //MAC Address 
};

struct netif *ne2k_if_netif;   

static void low_level_init(struct netif * netif);
static err_t low_level_output(struct netif * netif,struct pbuf *p);
static struct pbuf * low_level_input(struct netif *netif);


/**
 * Initialize the ne2k ethernet chip, resetting the interface and getting the ethernet
 * address.
 */
static void low_level_init(struct netif * netif)
{
	u16_t i;
	
	struct ne2k_if *ne2k_if;
	
	ne2k_if = netif->state;
	// the meaning of "netif->state" can be defined in drivers, here for MAC address!
	
	netif->hwaddr_len=6;
	netif->mtu = 1500;	
  	netif->flags = NETIF_FLAG_BROADCAST;
  		
	// ---------- start -------------
	
    i = EN_RESET;//this instruction let NE2K chip soft reset

    for (i=0;i<DELAY_MS;i++); //wait

    EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP); 
    
    EN0_DCFG = (u8_t) 0x01;

    /* Clear the remote	byte count registers. */
    EN0_RCNTHI = (u8_t) 0x00; 								/* MSB remote byte count reg */
    EN0_RCNTLO = (u8_t) 0x00; 								/* LSB remote byte count reg */

	/* RX configuration reg    Monitor mode (no packet receive) */
	EN0_RXCR = (u8_t) ENRXCR_MON;
	/* TX configuration reg   set internal loopback mode  */
	EN0_TXCR = (u8_t) ENTXCR_LOOP;

    EN0_TPSR = (u8_t) 0x40;					//发送缓冲首地址 大小为6页，刚好是1个最大包
    										//为0x40-0x46 
    
    EN0_STARTPG = (u8_t) 0x46 ;					    /* 接收缓冲 47。Starting page of ring buffer. First page of Rx ring buffer 46h*/
    EN0_BOUNDARY = (u8_t) 0x46 ;						/* Boundary page of ring buffer 0x46*/
    EN0_STOPPG = (u8_t) 0x80 ;    					/* Ending page of ring buffer ,0x80*/

    EN0_ISR = (u8_t) 0xff; 								/* clear the all flag bits in EN0_ISR */
    EN0_IMR = (u8_t) 0x00; 								/* Disable all Interrupt */

    EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_STOP);
    EN1_CURR = (u8_t) 0x47; 							
    /* RX_CURR_PG; 47 (keep curr=boundary+1 means no new packet)   */
           
    EN1_PAR0 = (u8_t)0x12;// MAC_addr.addr[0];	//自定义的mac地址
    EN1_PAR1 = (u8_t)0x34;// MAC_addr.addr[1];
    EN1_PAR2 = (u8_t)0x56;// MAC_addr.addr[2];
    EN1_PAR3 = (u8_t)0x78;// MAC_addr.addr[3];
    EN1_PAR4 = (u8_t)0x9a;// MAC_addr.addr[4];
    EN1_PAR5 = (u8_t)0xe0;// MAC_addr.addr[5];
    
  	/* make up an address. */
  	ne2k_if->ethaddr->addr[0] = (u8_t) 0x12;//MAC_addr.addr[0];
  	ne2k_if->ethaddr->addr[1] = (u8_t) 0x34;//MAC_addr.addr[1];
  	ne2k_if->ethaddr->addr[2] = (u8_t) 0x56;//MAC_addr.addr[2];
  	ne2k_if->ethaddr->addr[3] = (u8_t) 0x78;//MAC_addr.addr[3];
  	ne2k_if->ethaddr->addr[4] = (u8_t) 0x9a;//MAC_addr.addr[4];
  	ne2k_if->ethaddr->addr[5] = (u8_t) 0xe0;//MAC_addr.addr[5];
    
    /* Initialize the multicast list to reject-all.  
       If we enable multicast the higher levels can do the filtering. 
       <multicast filter mask array (8 bytes)> */
    EN1_MAR0 = (u8_t) 0x00;  
    EN1_MAR1 = (u8_t) 0x00;
    EN1_MAR2 = (u8_t) 0x00;
    EN1_MAR3 = (u8_t) 0x00;
    EN1_MAR4 = (u8_t) 0x00;
    EN1_MAR5 = (u8_t) 0x00;
    EN1_MAR6 = (u8_t) 0x00;
    EN1_MAR7 = (u8_t) 0x00;
    
    EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);  	   	/* 00001010B: PS1 PS0 RD2 RD1 RD0 TXP STA STP */
 
    EN0_IMR = (u8_t) (ENISR_OVER + ENISR_RX + ENISR_RX_ERR);
    //(ENISR_OVER + ENISR_RX + ENISR_TX + ENISR_TX_ERR);

    EN0_TXCR = (u8_t) 0xE0;	//ENTXCR_TXCONFIG;			//TCR 				
	EN0_RXCR = (u8_t) 0xCC;	//without Multicast //0xcc 	//RCR
	    
    EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);
    
    EN0_ISR = (u8_t) 0xff; // clear the all flag bits in EN0_ISR
  
 	ne2k_if_netif = netif;
}


/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
static err_t low_level_output(struct netif * netif, struct pbuf *p)
{
	struct pbuf *q;
	u16_t padLength,packetLength,Count,loop,rd_tmp;
	u8_t *buf,flag1=0 ;

	/*
	 * Set up to transfer the packet contents to the NIC RAM.
	 */
	padLength = 0;
	packetLength = p->tot_len - ETH_PAD_SIZE; //05 01 millin

    if ((packetLength) < 64)
   {
       padLength = 64 - (packetLength);
       packetLength = 64;
   }

	/* Wait for other transmit */
	while ( EN_CMD & EN_TRANS );
	
	/* We should already be in page 0, but to be safe... */
	EN_CMD = (u8_t) (EN_PAGE0 + EN_START + EN_NODMA);

	EN0_IMR = (u8_t) (ENISR_OVER);
	//(ENISR_OVER + ENISR_TX + ENISR_TX_ERR);
	// turn off RX int
	
	EN0_ISR = (u8_t) ENISR_RDC;	// clear the RDC bit
	
	/* Now the normal output. */
	EN0_RCNTLO = (u8_t) (packetLength & 0xff);
	EN0_RCNTHI = (u8_t) ((packetLength >> 8) & 0xff);
	EN0_RSARLO = (u8_t) ((TX_START_PG<<8) & 0xFF);
	EN0_RSARHI = (u8_t) (TX_START_PG & 0xFF);
	
	EN_CMD = (u8_t) (EN_RWRITE + EN_START + EN_PAGE0);
	
	/*
	 * Write packet to ring buffers.
	 */
   for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    	Count = q->len;
		buf = q->payload;

		if (q == p){
           	buf += ETH_PAD_SIZE;
		    Count -= ETH_PAD_SIZE;//Pad in Eth_hdr struct 
           	  }

		if ( (Count & 0x0001) ) flag1 = 1;
		Count = Count>>1;
		for (loop=0;loop < Count ;loop++){
				rd_tmp = (*buf++) + ((*buf++) << 8 );
				EN_DATA = (u16_t) rd_tmp;
	    	}
		if (flag1 == 1) *(unsigned char *)(Base_ADDR+0x10) = *buf++;
   	}

	while(padLength-- > 0){	
		*(unsigned char *)(Base_ADDR+0x10) = (u8_t)0x00; 	
		// Write padding for undersized packets
	}
    
	
	EN0_ISR = (u8_t) ENISR_RDC;	// clear the RDC bit

	/* Just send it, and does not check */
	EN0_TCNTLO = (u8_t) (packetLength& 0xff);
	EN0_TCNTHI = (u8_t) (packetLength>> 8);
	EN0_TPSR = (u8_t) TX_START_PG;

	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_TRANS + EN_START);
	
	EN0_IMR = (u8_t) (ENISR_OVER + ENISR_RX + ENISR_RX_ERR);
	//(ENISR_OVER + ENISR_RX + ENISR_TX + ENISR_TX_ERR);
	
	#if LINK_STATS
		lwip_stats.link.xmit++;
	#endif /* LINK_STATS */
		
	return ERR_OK;
}


/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
static struct pbuf * 
low_level_input(struct netif *netif)
{
	struct ne2k_if *ne2k_if = netif->state;
	u16_t  packetLength,len,Count,loop,rd_tmp;
	u8_t PDHeader[4];   // Temp storage for ethernet headers//18
	struct pbuf * p;
	struct pbuf * q;
	u8_t *payload,*buf,flag1=0;
	u8_t curr,this_frame,next_frame;
	
	EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_START);
	curr = (u8_t) EN1_CURR;
	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);
	this_frame = (u8_t) EN0_BOUNDARY + 1;//millin + 1
	
	if (this_frame >= RX_STOP_PG)
		this_frame = RX_START_PG;

	EN0_RCNTLO = (u8_t) 4;
	EN0_RCNTHI = (u8_t) 0;
	
	EN0_RSARLO = (u8_t) 0; /* See controller manual , use send packet command */
	EN0_RSARHI = (u8_t) this_frame; /* See controller manual , use send packet command */
	
	EN_CMD = (u8_t) (EN_PAGE0 + EN_RREAD + EN_START);
	
	//get the first 4 bytes from nic 
	Count = 2;  
	buf = PDHeader;
	while(Count--) {
 	    rd_tmp = EN_DATA ;
 	    *buf++ = (u8_t)(rd_tmp & 0x00ff) ;
 	    *buf++ = (u8_t)(rd_tmp >> 8) ;
  	}	
	
	EN0_ISR = (u8_t) ENISR_RDC;	/* clear the RDC bit */
	
	//  Store real length, set len to packet length - header
	packetLength = ((unsigned) PDHeader[2] | (PDHeader[3] << 8 ));
	
	next_frame = (u8_t) (this_frame + 1 + (packetLength >> 8));
	
	packetLength -= 4;
	
	if ((PDHeader[1] != next_frame) 
		&& (PDHeader[1] != next_frame + 1)
		&& (PDHeader[1] != next_frame - (RX_STOP_PG - RX_START_PG))
		&& (PDHeader[1] != next_frame + 1 - (RX_STOP_PG - RX_START_PG)))
		{
			EN0_BOUNDARY = (u8_t) (curr - 1); 
			//puts("Bad frame!\n");
			return NULL;
		}
	
	if (packetLength > MAX_PACKET_SIZE || packetLength < MIN_PACKET_SIZE)
		{
			next_frame = PDHeader[1];
			EN0_BOUNDARY = (u8_t) (next_frame-1);
			//printf("Bogus Packet Size \n");
			return NULL;
		}
	    	
	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);
	
	EN0_RCNTLO = (u8_t) (packetLength & 0xff);
	EN0_RCNTHI = (u8_t) (packetLength >> 8);
	
	EN0_RSARLO = (u8_t) 4; /* See controller manual , use send packet command */
	EN0_RSARHI = (u8_t) this_frame; /* See controller manual , use send packet command */
	
	EN_CMD = (u8_t) (EN_PAGE0 + EN_RREAD + EN_START);
	
	/* We allocate a pbuf chain of pbufs from the pool. */
	p = pbuf_alloc(PBUF_RAW, packetLength+ETH_PAD_SIZE, PBUF_POOL); /* length of buf */
	// change from PBUF_LINK
	
	if(p != NULL) {
		/* We iterate over the pbuf chain until we have read the entire
      		 packet into the pbuf. */
	
		for(q = p; q != NULL; q= q->next){
     		   /* Read enough bytes to fill this pbuf in the chain. The
         	  avaliable data in the pbuf is given by the q->len
         	  variable. */
      	
		  	payload = q->payload;
		  	len = q->len;

		  	if (q == p){ // if first buf...
           		payload += ETH_PAD_SIZE;
		    	len -= ETH_PAD_SIZE;//Pad in Eth_hdr struct 
			}
		
			Count = len;
			buf = payload;
		
			if ( (Count & 0x0001) ) flag1 = 1;
			Count = Count>>1;
			for(loop=0;loop < Count;loop++) {
 	    			rd_tmp = EN_DATA ;
 	    			*buf++ = (u8_t)(rd_tmp & 0x00ff) ;
 	    			*buf++ = (u8_t)(rd_tmp >> 8) ;
 	    	}
 	    	if ( flag1==1 )      *buf++ = *(unsigned char *)(Base_ADDR+0x10) ;
 	    	
 	    	#if LINK_STATS
    			lwip_stats.link.recv++;
			#endif /* LINK_STATS */  
		}//for
		
	} else { // p == NULL
    	/* no more PBUF resource, Discard packet in buffer. */
	     	Count = packetLength;
			if ( (Count & 0x0001) ) flag1 = 1; Count = Count>>1;
			for(loop=0;loop < Count;loop++) rd_tmp = EN_DATA;
 	    	if ( flag1==1 ) rd_tmp = *(unsigned char *)(Base_ADDR+0x10);
 	    	
 	    	#if LINK_STATS
    			lwip_stats.link.memerr++;
    			lwip_stats.link.drop++;
			#endif /* LINK_STATS */      
  	}
  	
  	next_frame = PDHeader[1];
	
	EN0_BOUNDARY = (u8_t) (next_frame-1);
	
	EN0_ISR = (u8_t) ENISR_RDC;
	
	return p;
}


/*
 * ethernetif_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actual transmission of the packet.
 *
 */
static err_t 
ne2k_output(struct netif *netif, struct pbuf *p,
		  struct ip_addr *ipaddr)
{
	/* resolve hardware address, then send (or queue) packet */
	return etharp_output(netif, ipaddr, p);
}


/*
 * ethernetif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
static void 
ne2k_input(struct netif *netif)
{
  struct ne2k_if *ne2k_if;
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  ne2k_if = netif->state;
    
//p = low_level_input(ne2k_if);
  /* move received packet into a new pbuf */
  p = low_level_input(netif);
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = p->payload;

#if LINK_STATS
  lwip_stats.link.recv++;
#endif /* LINK_STATS */  

  switch(htons(ethhdr->type)) {
  /* IP packet? */
	case ETHTYPE_IP:
    	/* update ARP table */
    	etharp_ip_input(netif, p);
    	/* skip Ethernet header */
    	pbuf_header(p, -(14+ETH_PAD_SIZE));
    	/* pass to network layer */
    	netif->input(p, netif);
    	break;
  case ETHTYPE_ARP:
	    /* pass p to ARP module */
   		etharp_arp_input(netif, ne2k_if->ethaddr, p);
    	break;
  default:
		pbuf_free(p);
		p = NULL;
		break;
  }
}

/*-----------------------------------------------------------------------------------*/
static void
arp_timer(void *arg)
{
  etharp_tmr();
  sys_timeout(ARP_TMR_INTERVAL, (sys_timeout_handler)arp_timer, NULL);
}

/*
 * ethernetif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
err_t 
ne2k_init(struct netif *netif)
{
  struct ne2k_if *ne2k_if;

  ne2k_if = mem_malloc(sizeof(struct ne2k_if));//MAC Address
  
  if (ne2k_if == NULL)
  {
  		LWIP_DEBUGF(NETIF_DEBUG,("ne2k_init: out of memory!\n"));
  		return ERR_MEM;
  }
  
  netif->state = ne2k_if;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->output = ne2k_output;
  netif->linkoutput = low_level_output;
  
  ne2k_if->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  
  low_level_init(netif);
  
  etharp_init();
  
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
  
  return ERR_OK;
}


/*-----------------------------------------------------------------------------------*/

void ne2k_rx_err(void)
{
		u8_t  curr;
		EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_STOP);
		curr = (u8_t) EN1_CURR;
		EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
		EN0_BOUNDARY = (u8_t) curr-1;
}


/*-----------------------------------------------------------------------------------*/

void ne2k_rx(void)
{
		u8_t  curr,bnry;
		EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_STOP);
		curr = (u8_t) EN1_CURR;
		EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
		bnry = (u8_t) EN0_BOUNDARY + 1;//millin + 1
		
		if (bnry >= RX_STOP_PG)
			bnry = RX_START_PG;
		
		while(curr != bnry){
			ne2k_input(ne2k_if_netif);
			EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_STOP);
			curr = (u8_t) EN1_CURR;
			EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
			bnry = (u8_t) EN0_BOUNDARY + 1;			//millin +1
			}
}

/* -----------------------------
 *     void ne2k_isr(void)
 *    can be int 4 5 6 or 7 
 * ----------------------------*/
void
ne2k_isr(void)
{
		
	DSP_C6x_Save();

	OSIntEnter();
	
	if (OSIntNesting == 1)
		{
			OSTCBCur->OSTCBStkPtr = (OS_STK *) DSP_C6x_GetCurrentSP();
		}
			
	/* You can enable Interrupt again here, 
		if want to use nested interrupt..... */
	//------------------------------------------------------------

	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
	//outb(CMD_PAGE0 | CMD_NODMA | CMD_STOP,NE_CR);
	
	EN0_IMR = (u8_t) 0x00;//close
	
	// ram overflow interrupt
	if (EN0_ISR & ENISR_OVER) {
		EN0_ISR = (u8_t) ENISR_OVER;		// clear interrupt
	}
	
	// error transfer interrupt ,NIC abort tx due to excessive collisions	
	if (EN0_ISR & ENISR_TX_ERR) {
		EN0_ISR = (u8_t) ENISR_TX_ERR;		// clear interrupt
	 	//temporarily do nothing
	}

	// Rx error , reset BNRY pointer to CURR (use SEND PACKET mode)
	if (EN0_ISR & ENISR_RX_ERR) {
		EN0_ISR = (u8_t) ENISR_RX_ERR;		// clear interrupt
		ne2k_rx_err();
	}

	//got packet with no errors
	if (EN0_ISR & ENISR_RX) {
		EN0_ISR = (u8_t) ENISR_RX;
		ne2k_rx();		
	}
		
	//Transfer complelte, do nothing here
	if (EN0_ISR & ENISR_TX){
		EN0_ISR = (u8_t) ENISR_TX;		// clear interrupt
	}
	
	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
	
	EN0_ISR = (u8_t) 0xff;			// clear ISR	
	
	EN0_IMR = (u8_t) (ENISR_OVER + ENISR_RX + ENISR_RX_ERR);
	//(ENISR_OVER + ENISR_RX + ENISR_TX + ENISR_TX_ERR);
	
	//open nic for next packet
	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);
	
	if (led_stat & 0x04) {LED3_on;}
	else {LED3_off;}
		
	//--------------------------------------------------------
		
	OSIntExit();
	
	DSP_C6x_Resume();
	
	asm ("	nop	5"); //important! 
	// this can avoid a stack error when compile with the optimization!
}
