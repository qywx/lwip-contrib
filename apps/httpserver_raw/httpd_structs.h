#ifndef __HTTPD_STRUCTS_H__
#define __HTTPD_STRUCTS_H__

#include "httpd.h"

#ifndef HTTPD_SERVER_AGENT
#define HTTPD_SERVER_AGENT "lwIP/1.3.1 (http://savannah.nongnu.org/projects/lwip)"
#endif

#ifndef LWIP_HTTPD_DYNAMIC_HEADERS
#define LWIP_HTTPD_DYNAMIC_HEADERS 1
#endif


#if LWIP_HTTPD_DYNAMIC_HEADERS
/*****************************************************************************
 * HTTP header strings for various filename extensions.
 *
 *****************************************************************************/
typedef struct
{
  const char *pszExtension;
  unsigned long ulHeaderIndex;
} tHTTPHeader;

static const char *g_psHTTPHeaderStrings[] =
{
 "Content-type: text/html\r\n\r\n",
 "Content-type: text/html\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"    \
   "Pragma: no-cache\r\n\r\n",
 "Content-type: image/gif\r\n\r\n",
 "Content-type: image/png\r\n\r\n",
 "Content-type: image/jpeg\r\n\r\n",
 "Content-type: image/bmp\r\n\r\n",
 "Content-type: image/x-icon\r\n\r\n",
 "Content-type: application/octet-stream\r\n\r\n",
 "Content-type: application/x-javascript\r\n\r\n",
 "Content-type: application/x-javascript\r\n\r\n",
 "Content-type: text/css\r\n\r\n",
 "Content-type: application/x-shockwave-flash\r\n\r\n",
 "Content-type: text/xml\r\n\r\n",
 "Content-type: text/plain\r\n\r\n",
 "HTTP/1.0 200 OK\r\n",
 "HTTP/1.0 404 File not found\r\n",
 "Server: "HTTPD_SERVER_AGENT"\r\n",
 "\r\n<html><body><h2>404: The requested file cannot be found."             \
   "</h2></body></html>\r\n"
};

#define HTTP_HDR_HTML           0  /* text/html */
#define HTTP_HDR_SSI            1  /* text/html Expires... */
#define HTTP_HDR_GIF            2  /* image/gif */
#define HTTP_HDR_PNG            3  /* image/png */
#define HTTP_HDR_JPG            4  /* image/jpeg */
#define HTTP_HDR_BMP            5  /* image/bmp */
#define HTTP_HDR_ICO            6  /* image/x-icon */
#define HTTP_HDR_APP            7  /* application/octet-stream */
#define HTTP_HDR_JS             8  /* application/x-javascript */
#define HTTP_HDR_RA             9  /* application/x-javascript */
#define HTTP_HDR_CSS            10 /* text/css */
#define HTTP_HDR_SWF            11 /* application/x-shockwave-flash */
#define HTTP_HDR_XML            12 /* text/xml */
#define HTTP_HDR_DEFAULT_TYPE   13 /* text/plain */
#define HTTP_HDR_OK             14 /* 200 OK */
#define HTTP_HDR_NOT_FOUND      15 /* 404 File not found */
#define HTTP_HDR_SERVER         16 /* Server: HTTPD_SERVER_AGENT */
#define DEFAULT_404_HTML        17 /* default 404 body */

static tHTTPHeader g_psHTTPHeaders[] =
{
 { "html", HTTP_HDR_HTML},
 { "htm",  HTTP_HDR_HTML},
 { "shtml",HTTP_HDR_SSI},
 { "shtm", HTTP_HDR_SSI},
 { "ssi",  HTTP_HDR_SSI},
 { "gif",  HTTP_HDR_GIF},
 { "png",  HTTP_HDR_PNG},
 { "jpg",  HTTP_HDR_JPG},
 { "bmp",  HTTP_HDR_BMP},
 { "ico",  HTTP_HDR_ICO},
 { "class",HTTP_HDR_APP},
 { "cls",  HTTP_HDR_APP},
 { "js",   HTTP_HDR_JS},
 { "ram",  HTTP_HDR_RA},
 { "css",  HTTP_HDR_CSS},
 { "swf",  HTTP_HDR_SWF},
 { "xml",  HTTP_HDR_XML}
};

#define NUM_HTTP_HEADERS (sizeof(g_psHTTPHeaders) / sizeof(tHTTPHeader))

#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

#endif /* __HTTPD_STRUCTS_H__ */
