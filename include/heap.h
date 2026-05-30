#ifndef HEAP_H_
#define HEAP_H_

#define MEMORY_SIZE 14*1024*1024
#define CHUNK_CAP 218*1024

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define UNIMPLEMENTED do {         \
    fprintf(stderr,                \
        "UNIMPLEMENTED: %s:%d %s", \
        __FILE__,                  \
        __LINE__,                  \
        __func__);                 \
    abort();                       \
} while(0)

typedef struct {
    uint8_t* ptr;
    size_t   size;
    char*    file;
    int      line;
    bool     used;      // ← add used flag
} Chunk;

typedef struct {
    Chunk  chunks[CHUNK_CAP];
    size_t count;
} Chunks;

static Chunks  used_chunks = {0};

static uint8_t _heap[MEMORY_SIZE] = {0};
static bool _heap_wrn = true;
static bool _heap_trc = false;

// interface
void reset_memory(void);
bool in_heap(void* ptr);
size_t heap_used_count(void);
size_t heap_free_count(void);
bool    free_no_overlap(void);
void*  ___memmove(void* dest, const void* src, size_t n, const char* file, int line);
void   ___print_memory();
void*  ___malloc(size_t size, const char* file, const int line);
char*  ___strdup(const char* str, const char* file, int line);
void   ___free(void *_Nullable_ptr, const char* file, const int line);
void   __heap_compact(void);
void*  ___calloc(size_t nmemb, size_t size, const char* file, const int line);
// implementation

#include "heap.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "logger.h"

// reset allocator state between tests
void reset_memory(void) {
    memset(&used_chunks, 0, sizeof(used_chunks));
    memset(_heap,   0, MEMORY_SIZE);
}

// check pointer is inside heap bounds
bool in_heap(void* ptr) {
    return (uint8_t*)ptr >= _heap &&
           (uint8_t*)ptr <  _heap + MEMORY_SIZE;
}

// count live (non-free) chunks
size_t heap_used_count(void) {
    size_t n = 0;
    for (size_t i = 0; i < used_chunks.count; i++)
        if (used_chunks.chunks[i].used) n++;
    return n;
}

// count free chunks
size_t heap_free_count(void) {
    size_t n = 0;
    for (size_t i = 0; i < used_chunks.count; i++)
        if (!used_chunks.chunks[i].used) n++;
    return n;
}

// check no two live chunks overlap
bool free_no_overlap(void) {
    for (size_t i = 0; i < used_chunks.count; i++) {
        if (!used_chunks.chunks[i].used) continue;
        for (size_t j = i + 1; j < used_chunks.count; j++) {
            if (!used_chunks.chunks[j].used) continue;
            uint8_t* a_start = used_chunks.chunks[i].ptr;
            uint8_t* a_end   = a_start + used_chunks.chunks[i].size;
            uint8_t* b_start = used_chunks.chunks[j].ptr;
            uint8_t* b_end   = b_start + used_chunks.chunks[j].size;
            if (a_start < b_end && b_start < a_end) return false;
        }
    }
    return true;
}




void* ___memmove(void* dest, const void* src, size_t n, const char* file, int line) {
    if (!dest || !src) {
        if (_heap_wrn) wrn("[mem/memmove] NULL pointer  dest=%p src=%p (%s:%d)",
                           dest, src, file, line);
        return dest;
    }

    if (n == 0) {
        if (_heap_wrn) wrn("[mem/memmove] zero size move (%s:%d)", file, line);
        return dest;
    }

    uint8_t* heap_start = _heap;
    uint8_t* heap_end   = _heap + MEMORY_SIZE;
    uint8_t* d          = (uint8_t*)dest;
    const uint8_t* s    = (const uint8_t*)src;

    // -- validate dest is inside heap -----------------------------------------
    if (d < heap_start || d + n > heap_end) {
        if (_heap_wrn) wrn("[mem/memmove] dest %p out of heap bounds (%s:%d)",
                           dest, file, line);
        return dest;
    }

    // -- validate src is inside heap ------------------------------------------
    if (s < heap_start || s + n > heap_end) {
        if (_heap_wrn) wrn("[mem/memmove] src %p out of heap bounds (%s:%d)",
                           src, file, line);
        return dest;
    }

    // -- validate dest chunk is live ------------------------------------------
    bool dest_live = false;
    for (size_t i = 0; i < used_chunks.count; ++i) {
        Chunk* c = &used_chunks.chunks[i];
        if (!c->used) continue;
        if (d >= c->ptr && d + n <= c->ptr + c->size) {
            dest_live = true;
            break;
        }
    }
    if (!dest_live) {
        if (_heap_wrn) wrn("[mem/memmove] dest %p is not inside a live chunk (%s:%d)",
                           dest, file, line);
        return dest;
    }

    // -- validate src chunk is live -------------------------------------------
    bool src_live = false;
    for (size_t i = 0; i < used_chunks.count; ++i) {
        Chunk* c = &used_chunks.chunks[i];
        if (!c->used) continue;
        if (s >= c->ptr && s + n <= c->ptr + c->size) {
            src_live = true;
            break;
        }
    }
    if (!src_live) {
        if (_heap_wrn) wrn("[mem/memmove] src %p is not inside a live chunk (%s:%d)",
                           src, file, line);
        return dest;
    }

    // -- move -----------------------------------------------------------------
    if (_heap_trc) trc("[mem/memmove] dest=%p src=%p n=%zu (%s:%d)",
                       dest, src, n, file, line);

    if (d == s)   return dest;   // no-op

    if (d < s || d >= s + n) {
        // no overlap or dest before src — safe forward copy
        while (n--) *d++ = *s++;
    } else {
        // overlap: dest inside src region — copy backward
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }

    return dest;
}

void ___print_memory () {
    dbg("[mem] ------------ MEMORY REPORT ----------------------------------");
    for (int i=0;i<used_chunks.count;++i) {
        Chunk c = used_chunks.chunks[i];
        if (c.used) dbg("[mem] %-5d ptr = %15p | %-4zu | %s:%d %s",i, c.ptr, c.size, c.file, c.line, (char*)c.ptr);
    }
    dbg("[mem] ------------ MEMORY CONTENT ----------------------------------");
    // for(size_t i=0;i<MEMORY_SIZE;++i){
    //     if (i>0 && i % 75 == 0) {
    //         puts("");
    //     }
    //     const char c = _heap[i] ? _heap[i] : '.'; 
    //     putc(c,stdout);
    // }
    // puts("");
    dbg("[mem] ------------ MEMORY REPORT ----------------------------------");
}


#define ALIGN     (8)
#define ALIGN_UP(n) (((n) + (ALIGN - 1)) & ~(ALIGN - 1))

void* ___malloc(size_t size_in_bytes, const char* file, const int line) {
    if (size_in_bytes == 0) {
        wrn("[mem/alloc] requested zero-sized alloc");
        return NULL;
    }
    if (used_chunks.count >= CHUNK_CAP) {
        wrn("[mem/alloc] OOM");
        return NULL;
    }

    // round up size to alignment boundary
    size_t size = ALIGN_UP(size_in_bytes);   // ← add this

    if (used_chunks.count > 0) {
        // scan for reusable freed slot
        for (size_t i = 0; i < used_chunks.count ; ++i) {
            if (!used_chunks.chunks[i].used && used_chunks.chunks[i].size >= size) {
                used_chunks.chunks[i].used = true;
                used_chunks.chunks[i].file = (char*)file;
                used_chunks.chunks[i].line = line;
                return (void*)used_chunks.chunks[i].ptr;
            }
        }
    }

    // fresh append — align the pointer too
    uint8_t* ptr = _heap;
    if (used_chunks.count > 0) {
        Chunk* last = &used_chunks.chunks[used_chunks.count - 1];
        ptr = last->ptr + last->size;
    }

    // align ptr to ALIGN boundary
    uintptr_t aligned = (uintptr_t)ptr;
    uintptr_t rem     = aligned % ALIGN;
    if (rem != 0) {
        aligned += ALIGN - rem;
        ptr      = (uint8_t*)aligned;
    }

    if (ptr + size > _heap + MEMORY_SIZE) {
        if (_heap_wrn) err("[mem] OOM (%s:%d)", file, line);
        return NULL;
    }

    used_chunks.chunks[used_chunks.count++] = (Chunk){ ptr, size, (char*)file, line, true };
    return (void*)ptr;
}

char* ___strdup(const char* str, const char* file, int line) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char*  buf = (char*)___malloc(len, file, line);
    if (!buf) return NULL;
    memcpy(buf, str, len);
    return buf;
}



void ___free(void *_Nullable_ptr, const char* file, const int line) {
    if (used_chunks.count == 0) {
        if (_heap_wrn) wrn("[mem/free] nothing is allocated yet %s:%d",file,line);
        return;
    }
    // after freeing a chunk, move all non-free chunks next to it to the left
    if (_Nullable_ptr == NULL) {
        if (_heap_wrn) wrn("[mem/free] attempting to free a NULL pointer from %s:%d",file,line);
        return;
    }
    const uint8_t* ptr = (uint8_t*)_Nullable_ptr;
    
    
    for (int i=used_chunks.count-1;i>=0;--i) {     
        if (used_chunks.chunks[i].ptr == ptr) {
            used_chunks.chunks[i].used = false;
            if(_heap_trc) trc("free %p", ptr);
            memset(used_chunks.chunks[i].ptr,0,used_chunks.chunks[i].size);
            return;
        }
    }
    if (_heap_wrn) wrn("[mem/free] failed to locate pointer (%p) from %s:%d",_Nullable_ptr,file,line);
}


void __heap_compact(void) {
    size_t heap_write = 0;   // tracks heap cursor
    size_t tbl_write  = 0;   // tracks chunk table cursor

    for (size_t i = 0; i < used_chunks.count; ++i) {
        if (!used_chunks.chunks[i].used) continue;

        uint8_t* src = used_chunks.chunks[i].ptr;
        size_t   sz  = used_chunks.chunks[i].size;

        // guard: ptr must be inside heap
        if (src < _heap || src + sz > _heap + MEMORY_SIZE) {
            wrn("[mem/compact] chunk %zu has out-of-bounds ptr %p — skipped", i, src);
            continue;
        }

        // shift heap data if there is a gap
        if (src != _heap + heap_write) {
            memmove(_heap + heap_write, src, sz);
        }

        // copy chunk record into packed position THEN update ptr
        used_chunks.chunks[tbl_write]      = used_chunks.chunks[i];   // copy full record
        used_chunks.chunks[tbl_write].ptr  = _heap + heap_write; // fix ptr AFTER copy

        heap_write += sz;
        tbl_write++;
    }

    // zero reclaimed heap region
    if (heap_write < MEMORY_SIZE)
        memset(_heap + heap_write, 0, MEMORY_SIZE - heap_write);

    // clear stale chunk slots
    for (size_t i = tbl_write; i < used_chunks.count; ++i)
        used_chunks.chunks[i] = (Chunk){0};

    used_chunks.count = tbl_write;
}

void* ___calloc(size_t nmemb, size_t size,
                             const char* file, const int line) {
    if (nmemb == 0 || size == 0) {
        if (_heap_wrn) wrn("[mem/calloc] zero nmemb or size (%s:%d)", file, line);
        return NULL;
    }

    // overflow-safe multiplication
    if (nmemb > MEMORY_SIZE / size) {
        if (_heap_wrn) wrn("[mem/calloc] nmemb * size overflow (%s:%d)", file, line);
        return NULL;
    }

    size_t total = nmemb * size;
    void*  ptr   = ___malloc(total, file, line);
    if (!ptr) return NULL;

    memset(ptr, 0, total);  // calloc guarantees zero-initialized memory
    return ptr;
}

void* ___realloc(void* _Nullable_ptr, size_t size,
                 const char* file, const int line) {
    if (!_Nullable_ptr) return ___malloc(size, file, line);
    if (size == 0) { ___free(_Nullable_ptr, file, line); return NULL; }

    int index = -1;
    for (int i = 0; i < (int)used_chunks.count; ++i) {
        if (used_chunks.chunks[i].ptr == (uint8_t*)_Nullable_ptr &&
            used_chunks.chunks[i].used) {          // ← must be live
            index = i;
            break;
        }
    }

    if (index == -1) {
        wrn("[mem/realloc] ptr %p not found or already freed (%s:%d)",
            _Nullable_ptr, file, line);
        return NULL;                          // ← don't malloc on unknown ptr
    }

    // -- shrink in place ------------------------------------------------------
    if (used_chunks.chunks[index].size >= size) {
        memset((uint8_t*)used_chunks.chunks[index].ptr + size, 0,
               used_chunks.chunks[index].size - size);
        used_chunks.chunks[index].size = size;
        used_chunks.chunks[index].file = (char*)file;
        used_chunks.chunks[index].line = line;
        return used_chunks.chunks[index].ptr;
    }

    // -- grow -----------------------------------------------------------------
    size_t  old_size = used_chunks.chunks[index].size;
    uint8_t* old_ptr = used_chunks.chunks[index].ptr;

    if (!old_ptr) {                           // ← guard NULL old_ptr
        wrn("[mem/realloc] chunk has NULL ptr (%s:%d)", file, line);
        return NULL;
    }

    uint8_t tmp[old_size];
    memcpy(tmp, old_ptr, old_size);           // snapshot before mutation

    used_chunks.chunks[index].used = false;
    memset(old_ptr, 0, old_size);

    void* new_ptr = ___malloc(size, file, line);
    if (!new_ptr) {                           // ← guard NULL new_ptr
        used_chunks.chunks[index].used = true;     // rollback
        memcpy(old_ptr, tmp, old_size);
        wrn("[mem/realloc] OOM growing %p %zu→%zu (%s:%d)",
            old_ptr, old_size, size, file, line);
        return NULL;
    }

    memcpy(new_ptr, tmp, old_size);
    return new_ptr;
}

void* ___reallocarray(void* _Nullable_ptr, size_t nmemb, size_t size,
                                   const char* file, const int line) {
    if (nmemb == 0 || size == 0) {
        if (_Nullable_ptr) ___free(_Nullable_ptr, file, line);
        return NULL;
    }

    // overflow-safe multiplication
    if (nmemb > MEMORY_SIZE / size) {
        if (_heap_wrn) wrn("[mem/reallocarray] nmemb * size overflow (%s:%d)", file, line);
        return NULL;
    }

    return ___realloc(_Nullable_ptr, nmemb * size, file, line);
}

// #ifdef MEMORY_DEBUG
#define MALLOC(p)            ___malloc(p,    __FILE__,__LINE__)
#define FREE(p)              ___free(p,    __FILE__,__LINE__)
#define CALLOC(n,s)          ___calloc(n,s,  __FILE__,__LINE__)
#define REALLOC(p,s)         ___realloc(p,s,  __FILE__,__LINE__)
#define REALLOCARRAY(p,n,s)  ___reallocarray(p,n,s,__FILE__,__LINE__)
#define STRDUP(s)            ___strdup(s,   __FILE__, __LINE__)
#define MEMMOVE(d,s,n)       ___memmove(d,s,n,__FILE__,__LINE__)
// #else
// #define MALLOC(p)            malloc(p)           //___malloc(p,    __FILE__,__LINE__)
// #define FREE(p)              free(p)             //___free(p,    __FILE__,__LINE__)
// #define CALLOC(n,s)          calloc(n,s)         //___calloc(n,s,  __FILE__,__LINE__)
// #define REALLOC(p,s)         realloc(p,s)        //___realloc(p,s,  __FILE__,__LINE__)
// #define REALLOCARRAY(p,n,s)  reallocarray(p,n,s) //___reallocarray(p,n,s,__FILE__,__LINE__)
// #define STRDUP(s)            strdup(s)           //___strdup(s,   __FILE__, __LINE__)
// #define MEMMOVE(d,s,n)       memmove(d,s,n)      //___memmove(d,s,n,__FILE__,__LINE__)
// #endif
#define PRINT_MEMORY()        ___print_memory ()


#endif// HEAP_H_
