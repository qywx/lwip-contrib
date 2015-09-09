/**
 * @file
 * Exports Private lwIP MIB 
 */

#ifndef LWIP_HDR_PRIVATE_MIB_H
#define LWIP_HDR_PRIVATE_MIB_H

#include "arch/cc.h"
#include "lwip/opt.h"

#if SNMP_PRIVATE_MIB
#include "lwip/snmp_structs.h"
extern const struct mib_array_node mib_private;

/** @todo remove this?? */
struct private_msg
{
  u8_t dummy;
};

void lwip_privmib_init(void);

#define SNMP_PRIVATE_MIB_INIT() lwip_privmib_init()

#endif /* SNMP_PRIVATE_MIB */

#endif
