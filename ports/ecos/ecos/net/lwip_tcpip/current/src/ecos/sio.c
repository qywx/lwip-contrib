/* Serial operations for SLIP */

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "lwipopts.h"

#include <cyg/io/io.h>

static cyg_io_handle_t ser;

static int len;

void sio_send(char c,void * dev)
{
	len = 1;
	cyg_io_write(*(cyg_io_handle_t*)dev, &c, &len);

//	if (len!=1)
//		diag_printf("err send\n");

}

char sio_recv(void * dev)
{
	char c;
	len = 1;
	cyg_io_read(*(cyg_io_handle_t *)dev, &c, &len);

//	if (len!=1)
//		diag_printf("err recv\n");
		
	return c;			
}
	
void * sio_open(int devnum)
{
	int res;
	
//		diag_printf("fun\n");
	res = cyg_io_lookup(SLIP_DEV, &ser);
//	if (res != ENOERR)
//		diag_printf("shit\n");
	return &ser; 
}

