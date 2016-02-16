typedef unsigned char err_t;
typedef unsigned int u32_t;
typedef void sys_sem_t;
typedef void sys_mutex_t;
typedef size_t mem_size_t;
typedef size_t memp_t;
struct pbuf;
struct netif;

void* mem_malloc(mem_size_t size)
{
  __coverity_alloc__(size);
}
void mem_free(void* mem)
{
  __coverity_free__(mem);
}

void* memp_malloc(memp_t type)
{
  __coverity_alloc_nosize__();  
}
void memp_free(memp_t type, void* mem)
{
  __coverity_free__(mem);  
}

void sys_mutex_lock(sys_mutex_t* mutex)
{
  __coverity_exclusive_lock_acquire__(mutex);
}
void sys_mutex_unlock(sys_mutex_t* mutex)
{
  __coverity_exclusive_lock_release__(mutex);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  __coverity_recursive_lock_acquire__(sem);
}
void sys_sem_signal(sys_sem_t *sem)
{
  __coverity_recursive_lock_release__(sem);
}

err_t ethernet_input(struct pbuf *p, struct netif *inp)
{
  __coverity_tainted_data_sink__(p); 
}
err_t tcpip_input(struct pbuf *p, struct netif *inp)
{
  __coverity_tainted_data_sink__(p); 
}

void abort(void)
{
  __coverity_panic__();
}

int check_path(char* path, size_t size)
{
  if (size) {
    __coverity_tainted_data_sanitize__(path);
    return 1;
  } else {
    return 0;
  }
}
