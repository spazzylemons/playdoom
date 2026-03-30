#ifndef _STUB_STDLIB_H
#define _STUB_STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void  free(void *ptr);

int    abs(int j);
long   labs(long j);
int    atoi(const char *nptr);
long   strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);

void   qsort(void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

_Noreturn void exit(int status);
_Noreturn void abort(void);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif
