/* Serial operations for SLIP */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/netif.h"

#include <cyg/io/io.h>
#include <cyg/io/config_keys.h>

static cyg_io_handle_t ser;

static int len;

void
sio_send(char c,void * dev)
{
	len = 1;
	cyg_io_write(*(cyg_io_handle_t*)dev, &c, &len);
}

char
sio_recv(void * dev)
{
	char c;
	len = 1;
	cyg_io_read(*(cyg_io_handle_t *)dev, &c, &len);
	return c;			
}

int
sio_write(void *dev, char *b, int size)
{
	int len = size;
	cyg_io_write(*(cyg_io_handle_t*)dev, b, &len);
	return len;
}
		
int
sio_read(void *dev, char *b, int size)
{
	int len = size;
	cyg_io_read(*(cyg_io_handle_t*)dev, b, &len);
	
	return len;
}

void * 
sio_open(int devnum)
{
	int res;
	cyg_uint32 nb = 0, len = 4;
	
#if LWIP_SLIP
	#define SIODEV SLIP_DEV
#elif PPP_SUPPORT
	#define SIODEV PPP_DEV
#endif
	res = cyg_io_lookup(SIODEV, &ser);
	if (res != ENOERR)
		diag_printf("Cannot open %s\n", SIODEV);

	res = cyg_io_set_config(ser, CYG_IO_SET_CONFIG_READ_BLOCKING, &nb, &len);
	return &ser; 
}

void 
sio_read_abort(void * dev)
{
   diag_printf("Abort called\n");
}
