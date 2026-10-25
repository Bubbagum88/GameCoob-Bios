#ifndef _MSL_STDIO_H
#define _MSL_STDIO_H

#include <stdarg.h>

int vprintf(const char* format, va_list arg);
int sprintf(char* s, const char* format, ...);

#endif
