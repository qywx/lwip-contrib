/* Serial operations for SLIP */

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "lwipopts.h"

#include <cyg/io/io.h>
#include <cyg/io/config_keys.h>

static cyg_io_handle_t ser;

static int len;

void sio_send(char c,void * dev)
{
	len = 1;
	cyg_io_write(*(cyg_io_handle_t*)dev, &c, &len);
}

char sio_recv(void * dev)
{
	char c;
	len = 1;
	cyg_io_read(*(cyg_io_handle_t *)dev, &c, &len);
	return c;			
}

int sio_write(void *dev, char *b, int size)
{
	int len = size;
	cyg_io_write(*(cyg_io_handle_t*)dev, b, &len);
	return len;
}
		
int sio_read(void *dev, char *b, int size)
{
	int len = size;
	cyg_io_read(*(cyg_io_handle_t*)dev, b, &len);
	return len;
}
void * sio_open(int devnum)
{
	int res;
	cyg_uint32 nb = 0;
	
	char siodev[] = "/dev/serX";
	if (devnum < 0 || devnum >9)
		return NULL;
	siodev[8] = '0' + devnum;
	res = cyg_io_lookup(siodev, &ser);
	if (res != ENOERR)
		diag_printf("Cannot open %s\n", siodev);
	cyg_io_set_config(ser, CYG_IO_SET_CONFIG_READ_BLOCKING, nb, 4);
	return &ser; 
}

