#ifndef MY_STDIO_H
#define MY_STDIO_H

// On the simulator, pd_api_network.h pulls in system <stdio.h> which conflicts
// with our macros below. Include it first so its declarations are already parsed.
#ifdef TARGET_SIMULATOR
#include <stdio.h>
#endif

#include "printf.h"

#define REMOVE_PRINTF

#ifdef REMOVE_PRINTF
#undef printf
#define printf(fmt, ...) do { (void)(fmt); __VA_ARGS__; } while (0)
#endif

#ifndef EOF
#define EOF (-1)
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

// Custom FILE type for WAD file I/O (overrides system FILE after it's been declared).
#define FILE FILE_
typedef struct FILE_ FILE_;

#define fopen fopen_
FILE_ *fopen_(const char *name, const char *mode);

#define fclose fclose_
int fclose_(FILE_ *file);

#define fread fread_
size_t fread_(void *ptr, size_t size, size_t nmemb, FILE_ *stream);

#define fwrite fwrite_
size_t fwrite_(void *ptr, size_t size, size_t nmemb, FILE_ *stream);

#define feof feof_
int feof_(FILE_ *stream);

#define fseek fseek_
int fseek_(FILE_ *stream, long offset, int whence);

#define ftell ftell_
long ftell_(FILE_ *stream);

#define fputc fputc_
int fputc_(int c, FILE_ *stream);

#define fgetc fgetc_
int fgetc_(FILE_ *stream);

#endif
