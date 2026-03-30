#ifndef _STUB_STDIO_H
#define _STUB_STDIO_H

#include <stddef.h>
#include <stdarg.h>

#ifndef EOF
#define EOF (-1)
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

// Only define FILE if my_stdio.h hasn't already redefined it.
#ifndef MY_STDIO_H
typedef struct _stub_FILE { int unused; } FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int    fprintf(FILE *stream, const char *format, ...);
int    printf(const char *format, ...);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int    fclose(FILE *stream);
FILE  *fopen(const char *filename, const char *mode);
int    fseek(FILE *stream, long offset, int whence);
long   ftell(FILE *stream);
int    feof(FILE *stream);
int    fgetc(FILE *stream);
int    fputc(int c, FILE *stream);
char  *fgets(char *s, int size, FILE *stream);
int    fputs(const char *s, FILE *stream);
int    fflush(FILE *stream);
void   rewind(FILE *stream);
#endif

int    sprintf(char *str, const char *format, ...);
int    snprintf(char *str, size_t size, const char *format, ...);
int    vsnprintf(char *str, size_t size, const char *format, va_list ap);
int    sscanf(const char *str, const char *format, ...);
int    remove(const char *pathname);
int    rename(const char *oldpath, const char *newpath);

#endif
