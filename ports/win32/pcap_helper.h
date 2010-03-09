#ifndef __PCAP_HELPER_H__
#define __PCAP_HELPER_H__

#include "arch/cc.h"

int get_adapter_index_from_addr(u32_t netaddr, char *guid, u32_t guid_len);

#endif /* __PCAP_HELPER_H__ */