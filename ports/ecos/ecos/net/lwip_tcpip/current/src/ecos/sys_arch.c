/* 
 * This file implements the eCos specific sys_arch functions used by lwIP 
 */

#include "lwipopts.h"
#include "arch/sys_arch.h"
#include "lwip/sys.h"
#include "lwip/def.h"
#include "lwip/debug.h"

//FIXME use CYG_HWR_whatever for RTC 

/* lwIp timeouts are milliseconds, eCos time unit is clock tick */
//1s = 1000ms = 100ticks (eCos default is 100 tick per second for most(all?) platforms) 
#define tick_to_msec(tick)	((u16_t)((tick)*10+1)) 
#define msec_to_tick(msec)	((cyg_tick_count_t)(msec+9)/10)

/* We use a common var mempool for allocating semaphores, mboxes and threads... */
static char memvar[CYGNUM_LWIP_VARMEMPOOL_SIZE];
static cyg_mempool_var var_mempool;
static cyg_handle_t var_mempool_h;


#define SYS_THREADS	2	/* polling thread and tcpip_thread */

#define THREAD_COUNT	(CYGNUM_LWIP_APP_THREADS + SYS_THREADS)
static char memfix[CYGNUM_LWIP_THREAD_STACK_SIZE * THREAD_COUNT];

/* List of threads: associate eCos thread info with lwIP timeout info */
struct lwip_thread {
	struct lwip_thread * next;
	struct sys_timeouts to;
	cyg_handle_t th;
	cyg_thread t;		
} *threads;

/* 
 * Timeout for threads which were not created by sys_thread_new
 * usually "main"
 */ 
struct sys_timeouts to;

/* 
 * Set up memory pools and threads
 */
void sys_init(void)
{
	cyg_mempool_var_create(memvar, sizeof(memvar), &var_mempool_h, &var_mempool);	

	threads = NULL;
	to.next = NULL;
}

/*------------------------------------ Message boxes ----------------------------------------*/

/*
 * Create a new mbox.If no memory is available return NULL 
 */
sys_mbox_t sys_mbox_new(void)
{
	cyg_mbox * mbox;
	cyg_handle_t m;
	mbox = (cyg_mbox *)cyg_mempool_var_try_alloc(var_mempool_h, sizeof(cyg_mbox));
	
	/* out of memory? */
	if(!mbox) 
		return SYS_MBOX_NULL;
	
	cyg_mbox_create(&m, mbox);
	return m;
}

/*
 * Destroy the mbox and release the space it took up in the pool
 */
void sys_mbox_free(sys_mbox_t mbox)
{
	cyg_mbox_delete(mbox);
	cyg_mempool_var_free(var_mempool_h,(void*)mbox);
}

/* 
 * Post data to a mbox.
 */ 
void sys_mbox_post(sys_mbox_t mbox, void *data)
{
	while (cyg_mbox_put(mbox,data) == false);
}

#if 0
void
sys_mbox_fetch(sys_mbox_t mbox, void **msg){
	void *d;
	d = cyg_mbox_get(mbox);
	if (msg)
		*msg = d;
	
}
#endif

/* 
 * Fetch data from a mbox.Wait for at most timeout millisecs
 * Return 0 if timed out otherwise time spent waiting or 1 
 * if there was no wait.
 */ 
u32_t sys_arch_mbox_fetch(sys_mbox_t mbox, void **data, u32_t timeout)
{
	void *d;
	cyg_tick_count_t end_time = 0, start_time = 0;
	if (timeout) {
		start_time = cyg_current_time();
		d = cyg_mbox_timed_get(mbox, start_time + msec_to_tick(timeout));
		end_time = cyg_current_time();

		if (d == NULL)
			return 0;
	} else {
		d = cyg_mbox_get(mbox);
	}
	if (data)
		*data = d;
	return tick_to_msec(end_time - start_time);	
}

/*----------------------------------------- Semaphores ------------------------------------ */

/*
 * Create a new semaphore and initialize it.
 * If no memory is available return NULL 
 */
sys_sem_t sys_sem_new(u8_t count)
{
	sys_sem_t sem;

	sem = (cyg_sem_t *)cyg_mempool_var_try_alloc(var_mempool_h, sizeof(cyg_sem_t));
	/* out of memory? */
	if(!sem)
		return SYS_SEM_NULL;
	cyg_semaphore_init(sem, count);
	return sem;
}

#if 0
void
sys_sem_wait(sys_sem_t sem)
{
	cyg_semaphore_wait(sem);

}

void
sys_timeout(u16_t msecs, sys_timeout_handler h, void *arg)
{}
#endif
/* 
 * Wait on a semaphore for at most timeout millisecs
 * Return 0 if timed out otherwise time spent waiting or 1 
 * if there was no wait.
 */ 
u32_t sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)
{
	cyg_bool_t r;
	cyg_tick_count_t end_time = 0, start_time = 0;

	if (timeout) {
		start_time = cyg_current_time();
		r = cyg_semaphore_timed_wait(sem, start_time + msec_to_tick(timeout));
		end_time = cyg_current_time();

		if (r == false) {
			return 0;
		}	
	} else {
		cyg_semaphore_wait(sem);
	}

	return tick_to_msec(end_time - start_time);	
}

/*
 * Signal a semaphore
 */ 
void sys_sem_signal(sys_sem_t sem)
{
	cyg_semaphore_post(sem);
}

/* 
 * Destroy the semaphore and release the space it took up in the pool 
 */
void sys_sem_free(sys_sem_t sem)
{
	cyg_semaphore_destroy(sem);
	cyg_mempool_var_free(var_mempool_h,(void*)sem);
}

/*
 * Create new thread 
 */
sys_thread_t sys_thread_new(void (*function) (void *arg), void *arg,int prio)
{
	struct lwip_thread * nt;
	void * stack;
	static int thread_count = 0;
	nt = (struct lwip_thread *)cyg_mempool_var_alloc(var_mempool_h, sizeof(struct lwip_thread));

	nt->next = threads;
	nt->to.next = NULL;
	
	threads = nt;

	stack = (void *)(memfix+CYGNUM_LWIP_THREAD_STACK_SIZE*thread_count++);
	cyg_thread_create(prio, (cyg_thread_entry_t *)function, (cyg_addrword_t)arg,
			(char *)arg , stack, CYGNUM_LWIP_THREAD_STACK_SIZE, &(nt->th), &(nt->t) );

	cyg_thread_resume(nt->th);
	return NULL;
}

/* 
 * Return current thread's timeout info
 */
struct sys_timeouts *sys_arch_timeouts(void)
{
	cyg_handle_t ct;
	struct lwip_thread *t;

	ct = cyg_thread_self();
	for(t = threads; t; t = t->next)
		if (t->th == ct)
			return &(t->to);
	
	return &to;
}
