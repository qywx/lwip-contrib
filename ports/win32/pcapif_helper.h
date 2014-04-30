#ifndef __PCAPIF_HELPER_H__
#define __PCAPIF_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

struct pcapifh_linkstate;

enum pcapifh_link_event {
  PCAPIF_LINKEVENT_UNKNOWN,
  PCAPIF_LINKEVENT_UP,
  PCAPIF_LINKEVENT_DOWN
};

struct pcapifh_linkstate* pcapifh_linkstate_init(const char *adapter_name);
enum pcapifh_link_event pcapifh_linkstate_get(struct pcapifh_linkstate* state);
void pcapifh_linkstate_close(struct pcapifh_linkstate* state);


#ifdef __cplusplus
}
#endif

#endif /* __PCAPIF_HELPER_H__ */