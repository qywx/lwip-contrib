/* config.h for check-0.9.8 on win32 under MSVC */

typedef unsigned int pid_t;
typedef unsigned int uint32_t;

#define ssize_t size_t
#define snprintf _snprintf

#define HAVE_DECL_STRDUP 1
#define HAVE_DECL_FILENO 1

#include <io.h>
