#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#include "PowerPC_EABI_Support/MSL_C/MSL_Common/alloc.h"
#include "PowerPC_EABI_Support/MSL_C/MSL_Common/arith.h"
#include "PowerPC_EABI_Support/MSL_C/MSL_Common/mbstring.h"
#include "PowerPC_EABI_Support/MSL_C/MSL_Common/rand.h"
#include "PowerPC_EABI_Support/MSL_C/MSL_Common/strtold.h"
#include "PowerPC_EABI_Support/MSL_C/MSL_Common/strtoul.h"

BEGIN_SCOPE_EXTERN_C

void abort(void);
void atexit(void);
void exit(int);

typedef int (*_compare_function)(const void*, const void*);
void qsort(void* table_base, size_t num_members, size_t member_size, _compare_function compare_members);

END_SCOPE_EXTERN_C

#endif
