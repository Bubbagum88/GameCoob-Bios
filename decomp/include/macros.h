#ifndef _MACROS_H
#define _MACROS_H

#define ARRAY_SIZE(o) (sizeof((o)) / sizeof(*(o)))

// Unused local that reserves stack space, for frames the original code
// sized larger than its visible locals need.
#ifdef __MWERKS__
#define STACK_PAD_VAR(n)                                                                           \
    do {                                                                                           \
        int pad[n];                                                                                \
    } while (0)
#else
#define STACK_PAD_VAR(n) ((void)0)
#endif

#endif
