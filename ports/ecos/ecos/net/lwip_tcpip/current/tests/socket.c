/* Simple test-case for the BSD socket API  : echo on TCP port 7 */

#include "lwip/sys.h"
#define LWIP_COMPAT_SOCKETS 1
#include "lwip/sockets.h"

char buf[400];
static void
socket_thread(void *arg)
{
	int sock,s;
	int len;
	struct sockaddr_in addr,rem;
	sock = socket(AF_INET,SOCK_STREAM,0);	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(sock,(struct sockaddr *)&addr,sizeof(addr));
			
	listen(sock,5);
	while(1) {	
		len = sizeof(rem);
		s = accept(sock,(struct sockaddr*)&rem,&len);
		while((len = read(s,buf,400)) > 0)
			write(s,buf,len);
		close(s);
	}	
}

void
tmain(void * p)
{
  lwip_init();	
  sys_thread_new(socket_thread, (void*)"socket",7);
}

#define STACK_SIZE 0x1000
static char stack[STACK_SIZE];
static cyg_thread thread_data;
static cyg_handle_t thread_handle;

void
cyg_user_start(void)
{
    // Create a main thread, so we can run the scheduler and have time 'pass'
    cyg_thread_create(10,                // Priority - just a number
                      tmain,          // entry
                      0,                 // entry parameter
                      "socket echo test",        // Name
                      &stack[0],         // Stack
                      STACK_SIZE,        // Size
                      &thread_handle,    // Handle
                      &thread_data       // Thread data structure
            );
    cyg_thread_resume(thread_handle);  // Start it
}

