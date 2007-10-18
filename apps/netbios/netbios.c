#include "netbios.h"

#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/netif.h"

#if LWIP_UDP

/** default port number for "NetBIOS Name service */
#define NETBIOS_PORT     137

/** size of a NetBIOS name */
#define NETBIOS_NAME_LEN 16

/** NetBIOS name of LWIP device */
#define NETBIOS_LWIP_NAME "NETBIOSLWIPDEV"

/** NetBIOS message header */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_hdr {
  PACK_STRUCT_FIELD(u16_t trans_id);
  PACK_STRUCT_FIELD(u16_t flags);
  PACK_STRUCT_FIELD(u16_t questions);
  PACK_STRUCT_FIELD(u16_t answerRRs);
  PACK_STRUCT_FIELD(u16_t authorityRRs);
  PACK_STRUCT_FIELD(u16_t additionalRRs);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** NetBIOS message name part */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_name_hdr {
  PACK_STRUCT_FIELD(u8_t  nametype);
  PACK_STRUCT_FIELD(u8_t  encname[(NETBIOS_NAME_LEN*2)+1]);
  PACK_STRUCT_FIELD(u16_t type);
  PACK_STRUCT_FIELD(u16_t class);
  PACK_STRUCT_FIELD(u32_t ttl);
  PACK_STRUCT_FIELD(u16_t datalen);
  PACK_STRUCT_FIELD(u16_t flags);
  PACK_STRUCT_FIELD(u32_t addr);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** NetBIOS decoding name */
static int
netbios_name_decoding( char *name_enc, char *name_dec, int name_dec_len)
{
  char *pname;
  char  cname;
  char  cnbname;
  int   index = 0;
  
  /* Start decoding netbios name. */
  pname  = name_enc;
  for (;;) {
    /* Every two characters of the first level-encoded name
     * turn into one character in the decoded name. */
    cname = *pname;
    if (cname == '\0')
      break;    /* no more characters */
    if (cname == '.')
      break;    /* scope ID follows */
    if (cname < 'A' || cname > 'Z') {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname = cname << 4;
    pname++;

    cname = *pname;
    if (cname == '\0' || cname == '.') {
      /* No more characters in the name - but we're in
       * the middle of a pair.  Not legal. */
      return -1;
    }
    if (cname < 'A' || cname > 'Z') {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname |= cname;
    pname++;

    /* Do we have room to store the character? */
    if (index < NETBIOS_NAME_LEN) {
      /* Yes - store the character. */
      name_dec[index++] = (cnbname!=' '?cnbname:'\0');
    }
  }

  return 0;
}

/** NetBIOS encoding name */
static int
netbios_name_encoding(char *name_enc, char *name_dec, int name_dec_len)
{
  char         *pname;
  char          cname;
  unsigned char ucname;
  int           index = 0;
  
  /* Start encoding netbios name. */
  pname = name_enc;

  for (;;) {
    /* Every two characters of the first level-encoded name
     * turn into one character in the decoded name. */
    cname = *pname;
    if (cname == '\0')
      break;    /* no more characters */
    if (cname == '.')
      break;    /* scope ID follows */
    if ((cname < 'A' || cname > 'Z') && (cname < '0' || cname > '9')) {
      /* Not legal. */
      return -1;
    }

    /* Do we have room to store the character? */
    if (index >= name_dec_len) {
      return -1;
    }

    /* Yes - store the character. */
    ucname = cname;
    name_dec[index++] = ('A'+((ucname>>4) & 0x0F));
    name_dec[index++] = ('A'+( ucname     & 0x0F));
    pname++;
  }

  /* Fill with "space" coding */
  for (;index<name_dec_len-1;) {
    name_dec[index++] = 'C';
    name_dec[index++] = 'A';
  }

  /* Terminate string */
  name_dec[index]='\0';

  return 0;
}

/** NetBIOS Name service recv callback */
static void
netbios_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  /* if packet is valid */
  if (p != NULL) {
    char   netbios_name[NETBIOS_NAME_LEN+1];
    struct netbios_hdr*      netbios_hdr      = p->payload;
    struct netbios_name_hdr* netbios_name_hdr = (struct netbios_name_hdr*)(netbios_hdr+1);
    
    /* if the packet is a NetBIOS question */
    if (((ntohs(netbios_hdr->flags) & 0x8000/** @todo */)==0) && (ntohs(netbios_hdr->questions)==1)) {
      /* decode the NetBIOS name */
      netbios_name_decoding( netbios_name_hdr->encname, netbios_name, sizeof(netbios_name));
      /* if the packet is for us */
      if (strcmp( netbios_name, NETBIOS_LWIP_NAME)==0) {
        struct pbuf *q;
        struct {
          struct netbios_hdr      resp_hdr;
          struct netbios_name_hdr resp_name;
        }netbios_resp;

        /* prepare NetBIOS header response */
        netbios_resp.resp_hdr.trans_id      = netbios_hdr->trans_id;
        netbios_resp.resp_hdr.flags         = htons(0x8500);/** @todo */
        netbios_resp.resp_hdr.questions     = 0;
        netbios_resp.resp_hdr.answerRRs     = htons(1);
        netbios_resp.resp_hdr.authorityRRs  = 0;
        netbios_resp.resp_hdr.additionalRRs = 0;

        /* prepare NetBIOS header datas */
        memcpy( netbios_resp.resp_name.encname, netbios_name_hdr->encname, sizeof(netbios_name_hdr->encname));
        netbios_resp.resp_name.nametype = netbios_name_hdr->nametype;
        netbios_resp.resp_name.type     = netbios_name_hdr->type;
        netbios_resp.resp_name.class    = netbios_name_hdr->class;
        netbios_resp.resp_name.ttl      = 0xe0930400; /** @todo */
        netbios_resp.resp_name.datalen  = htons(sizeof(netbios_resp.resp_name.flags)+sizeof(netbios_resp.resp_name.addr));
        netbios_resp.resp_name.flags    = 0x0060;/** @todo */
        netbios_resp.resp_name.addr     = netif_default->ip_addr.addr;

        /* alloc a "reference" pbuf */
        q = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_REF);
        if (p != NULL) {
          /* initialize the "reference" pbuf on the NetBIOS response */
          q->payload = (void*)(&netbios_resp);
          q->len = q->tot_len = sizeof(netbios_resp);
          
          /* send the NetBIOS response */
          udp_sendto(upcb, q, addr, port);
          
          /* free the "reference" pbuf */
          pbuf_free(q);
        }
      }
    }
   /* free the pbuf */
    pbuf_free(p);
  }
}

void netbios_init(void)
{
  struct udp_pcb *pcb;

  pcb = udp_new();
  if (pcb != NULL) {
    udp_bind(pcb, IP_ADDR_ANY, NETBIOS_PORT);
    udp_recv(pcb, netbios_recv, pcb);
  }
}
#endif /* LWIP_UDP */
