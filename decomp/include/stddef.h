#ifndef _MSL_STDDEF_H
#define _MSL_STDDEF_H

#ifndef _SIZE_T_DEF
#define _SIZE_T_DEF
typedef unsigned long size_t;
#endif

typedef long ptrdiff_t;

#ifndef NULL
#define NULL 0
#endif

#define offsetof(type, member) ((size_t) & (((type*)0)->member))

#endif
