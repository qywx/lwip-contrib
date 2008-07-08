#ifndef __PING_H__
#define __PING_H__

void ping_init(void);

#if NO_SYS
void ping_send_now();
#endif /* NO_SYS */

#endif /* __PING_H__ */
