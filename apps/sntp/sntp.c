#include "sntp.h"

/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

/* C runtime includes */
#include "time.h"

/** This is an example of a "SNTP" client (with socket API).
 * 
 * For a list of some public NTP servers, see this link :
 * http://support.ntp.org/bin/view/Servers/NTPPoolServers
 *
 */

#if LWIP_SOCKET

/* SNTP server address as string in "dot format" - avoid to use DNS here */
#ifndef SNTP_SERVER_ADDRESS
#define SNTP_SERVER_ADDRESS "91.121.71.87" /* pool.ntp.org */
#endif

/* SNTP macro to change system time and/or the update the RTC clock */
#ifndef SNTP_SYSTEM_TIME
#define SNTP_SYSTEM_TIME(t)
#endif

/**
 * SNTP_DEBUG: Enable debugging for SNTP.
 */
#ifndef SNTP_DEBUG
#define SNTP_DEBUG LWIP_DBG_ON
#endif

/* SNTP protocol defines */
#define SNTP_PORT                   123
#define SNTP_RECV_TIMEOUT           3000
#define SNTP_UPDATE_DELAY           60000
#define SNTP_MAX_DATA_LEN           48
#define SNTP_RCV_TIME_OFS           32
#define SNTP_LI_NO_WARNING          0x00
#define SNTP_VERSION               (4/* NTP Version 4*/<<3) 
#define SNTP_MODE_CLIENT            0x03
#define SNTP_MODE_SERVER            0x04
#define SNTP_MODE_BROADCAST         0x05
#define SNTP_MODE_MASK              0x07

/* number of seconds between 1900 and 1970 */
#define DIFF_SEC_1900_1970         (2208988800)

/**
 * SNTP processing
 */
static void
sntp_process( time_t t)
{
  /* change system time and/or the update the RTC clock */
  SNTP_SYSTEM_TIME(t);

  /* display local time from GMT time */
  LWIP_DEBUGF( SNTP_DEBUG, ("sntp_process: %s\n", ctime(&t)));
}

/**
 * SNTP request
 */
static void
sntp_request()
{
  int                sock;
  struct sockaddr_in local;
  struct sockaddr_in to;
  int                tolen;
  int                size;
  int                timeout;
  u8_t               sntp_request [SNTP_MAX_DATA_LEN];
  u8_t               sntp_response[SNTP_MAX_DATA_LEN];
  u32_t              sntp_server_address;
  u32_t              timestamp;
  time_t             t;
  
  /* initialize SNTP server address */
  sntp_server_address = inet_addr(SNTP_SERVER_ADDRESS);

  /* if we got a valid SNTP server address... */
  if (sntp_server_address!=0) {
    /* create new socket */
    sock = socket( AF_INET, SOCK_DGRAM, 0);
    if (sock>=0) {
      /* prepare local address */
      memset(&local, 0, sizeof(local));
      local.sin_family      = AF_INET;
      local.sin_port        = htons(INADDR_ANY);
      local.sin_addr.s_addr = htonl(INADDR_ANY);

      /* bind to local address */
      if (bind( sock, (struct sockaddr *)&local, sizeof(local))==0) {
        /* set recv timeout */
        timeout = SNTP_RECV_TIMEOUT;
        setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

        /* prepare SNTP request */
        memset( sntp_request, 0, sizeof(sntp_request));
        sntp_request[0] = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;

        /* prepare SNTP server address */
        memset(&to, 0, sizeof(to));
        to.sin_family      = AF_INET;
        to.sin_port        = htons(SNTP_PORT);
        to.sin_addr.s_addr = sntp_server_address;
    
        /* send SNTP request to server */
        if (sendto( sock, sntp_request, sizeof(sntp_request), 0, (struct sockaddr *)&to, sizeof(to))>=0) {
          /* receive SNTP server response */
          tolen = sizeof(to);
          size  = recvfrom( sock, sntp_response, sizeof(sntp_response), 0, (struct sockaddr *)&to, (socklen_t *)&tolen);

          /* if the response size is good */
          if (size == SNTP_MAX_DATA_LEN) {
            /* if this is a SNTP response... */
            if (((sntp_response[0] & SNTP_MODE_MASK) == SNTP_MODE_SERVER) || ((sntp_response[0] & SNTP_MODE_MASK) == SNTP_MODE_BROADCAST)) {
              /* extract GMT time from response */
              SMEMCPY( &timestamp, (sntp_response+SNTP_RCV_TIME_OFS), sizeof(timestamp));
              t = (ntohl(timestamp) - DIFF_SEC_1900_1970);

              /* do time processing */
              sntp_process(t);

            } else {
              LWIP_DEBUGF( SNTP_DEBUG, ("sntp_request: not response frame code\n"));
            }
          } else {
            LWIP_DEBUGF( SNTP_DEBUG, ("sntp_request: not recvfrom==%i\n", errno));
          }
        } else {
          LWIP_DEBUGF( SNTP_DEBUG, ("sntp_request: not sendto==%i\n", errno));
        }
      }
      /* close the socket */
      closesocket(sock);
    }
  }
}

/**
 * SNTP thread
 */
static void
sntp_thread(void *arg)
{
  while(1) {
    sntp_request();
    sys_msleep(SNTP_UPDATE_DELAY);
  }
}

#endif /* LWIP_SOCKET */

void
sntp_init(void)
{ sys_thread_new("sntp_thread", sntp_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}
