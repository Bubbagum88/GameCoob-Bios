#ifndef _MSL_STRING_H
#define _MSL_STRING_H

#ifndef _SIZE_T_DEF
#define _SIZE_T_DEF
typedef unsigned long size_t;
#endif

void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* dst, int val, size_t n);
int memcmp(const void* a, const void* b, size_t n);
size_t strlen(const char* s);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);
int strcmp(const char* a, const char* b);
char* strchr(const char* s, int c);

#endif
