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

#ifdef __cplusplus
extern "C" {
#endif

/* export MIB */
extern const struct mib_array_node mib_private;

void lwip_privmib_init(void);

#define SNMP_PRIVATE_MIB_INIT() lwip_privmib_init()

#ifdef __cplusplus
}
#endif

#endif /* SNMP_PRIVATE_MIB */

#endif
