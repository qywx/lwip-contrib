#ifndef __CC_H__
#define __CC_H__

/* memset(), memcpy() */
#include <string.h>
/* printf() and abort() */
#include <stdio.h>
#include <stdlib.h>
/* isdigit() */
#include <ctype.h>

#include "arch/cpu.h"

typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;

typedef u32_t mem_ptr_t;

/* LW: Supported in at least >=v7.5 r2, but lwIP worked without the "_packed" attribute already */
#define PACK_STRUCT_BEGIN _packed
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#ifdef LWIP_DEBUG

/* LW: forward declaration */
void debug_printf(char *format, ...);
void page_printf(char *format, ...);

/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)	{ debug_printf x; }
#define LWIP_PLATFORM_ASSERT(x) { debug_printf("\fline %d in %s\n", __LINE__, __FILE__);  while(1); }

#endif/* LWIP_DEBUG */

#endif /* __CC_H__ */
