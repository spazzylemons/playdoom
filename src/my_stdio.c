#include "my_stdio.h"
#include "z_zone.h"

#include "playdate.h"

struct FILE_ {
    SDFile *fd;
    int eof;
};

void _putchar(char character) {
    static char printbuffer[256];
    static int printindex = 0;

    if (character == '\n') {
        playdate->system->logToConsole("%.*s", printindex, printbuffer);
        printindex = 0;
    } else if (printindex < sizeof(printbuffer)) {
        printbuffer[printindex++] = character;
    }
}

FILE_ *fopen_(const char *name, const char *mode) {
    FileOptions flags = 0;
    if (strstr(mode, "r")) {
        flags = kFileRead | kFileReadData;
    } else if (strstr(mode, "w")) {
        flags = kFileWrite;
    }

    SDFile *fd = playdate->file->open(name, flags);
    if (fd == NULL) {
        return NULL;
    }

    FILE_ *file = Z_Malloc(sizeof(FILE_), PU_STATIC, 0);
    file->fd = fd;
    file->eof = 0;
    return file;
}

int fclose_(FILE_ *file) {
    playdate->file->flush(file->fd);
    int result = playdate->file->close(file->fd);
    Z_Free(file);
    return result;
}

size_t fread_(void *ptr, size_t size, size_t nmemb, FILE_ *stream) {
    int result = playdate->file->read(stream->fd, ptr, size * nmemb);
    if (result < 0) {
        return 0;
    }

    if (result == 0) {
        stream->eof = 1;
        return 0;
    }

    int remainder = result % size;
    if (remainder != 0) {
        playdate->file->seek(stream->fd, -remainder, SEEK_CUR);
    }

    return result / size;
}

size_t fwrite_(void *ptr, size_t size, size_t nmemb, FILE_ *stream) {
    int result = playdate->file->write(stream->fd, ptr, size * nmemb);
    if (result < 0) {
        return 0;
    }

    return result / size;
}

int feof_(FILE_ *stream) {
    return stream->eof;
}

int fseek_(FILE_ *stream, long offset, int whence) {
    return playdate->file->seek(stream->fd, offset, whence);
}

long ftell_(FILE_ *stream) {
    return playdate->file->tell(stream->fd);
}

int fputc_(int c, FILE_ *stream) {
    if (fwrite_(&c, 1, 1, stream) == 1) {
        return 0;
    }
    return -1;
}

int fgetc_(FILE_ *stream) {
    char c;
    if (fread_(&c, 1, 1, stream) == 1) {
        return (unsigned) c;
    }
    return EOF;
}
