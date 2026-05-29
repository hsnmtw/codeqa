#ifndef IO_H_
#define IO_H_

#include "common.h"
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

void read_entire_file(const char* file_path, StringView* sv);
void write_entire_file(const char* file_path, StringView* sv);

bool io_is_file(const char* path);
bool io_is_directory(const char* path);

#ifdef IO_IMPL


bool io_is_file(const char* path) {
    FILE *f = fopen(path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

#ifdef _WIN32
  #define io_stat(p, s)   _stat((p), (s))
  #define io_stat_t       struct _stat
  #define IO_S_ISDIR(m)   (((m) & _S_IFMT) == _S_IFDIR)
#else
  #define io_stat(p, s)   stat((p), (s))
  #define io_stat_t       struct stat
  #define IO_S_ISDIR(m)   S_ISDIR(m)
#endif
bool io_is_directory(const char* path) {
    if (path == NULL || path[0] == '\0') return false;
    io_stat_t st;
    if (io_stat(path, &st) != 0) return false;
    return IO_S_ISDIR(st.st_mode);
}


void read_entire_file(const char *file_path, StringView *sv) {
    const char* f_name = nameof(read_entire_file);
    sv->buffer = NULL;
    sv->len    = 0;

    errno = 0;
    
    // --- open ---
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        err("%s: fopen failed: %s\n", f_name, strerror(errno));
        return;
    }

    // --- get size via stat ---
    struct stat st;
    if (stat(file_path, &st) != 0) {
        err("%s: stat failed: %s\n", f_name, strerror(errno));
        fclose(f);
        return;
    }
    size_t size = (size_t)st.st_size;

    // --- allocate (+1 for null terminator) ---
    char *buf = MALLOC(size + 1);
    if (!buf) {
        err("%s: out of memory: %s\n", f_name, strerror(errno));
        fclose(f);
        return;
    }

    // --- read ---
    size_t read = fread(buf, 1, size, f);
    if (read != size) {
        err("fread: expected %zu bytes, got %zu\n", size, read);
        FREE(buf);
        fclose(f);
        return;
    }

    buf[size] = '\0';   // null-terminate so buffer is also a valid cstr

    fclose(f);

    sv->buffer = buf;
    sv->len    = size;

    // strip UTF-8 BOM if present (EF BB BF)
    if (sv->len >= 3 &&
        (unsigned char)sv->buffer[0] == 0xEF &&
        (unsigned char)sv->buffer[1] == 0xBB &&
        (unsigned char)sv->buffer[2] == 0xBF)
    {
        memmove(sv->buffer, sv->buffer + 3, sv->len - 3 + 1);  // +1 for null terminator
        sv->len -= 3;
    }    

}

#endif//IO_IMPL

#endif//IO_H_
