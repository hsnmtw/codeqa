#ifndef HEAP_H_
#define HEAP_H_

#define MEMORY_SIZE 64*1024*1024
#define CHUNKS_SIZE 128*1000

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
    Chunk  chunks[CHUNKS_SIZE];
    size_t count;
} Memory;

static Memory  memory = {0};
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
    memset(&memory, 0, sizeof(memory));
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
    for (size_t i = 0; i < memory.count; i++)
        if (memory.chunks[i].used) n++;
    return n;
}

// count free chunks
size_t heap_free_count(void) {
    size_t n = 0;
    for (size_t i = 0; i < memory.count; i++)
        if (!memory.chunks[i].used) n++;
    return n;
}

// check no two live chunks overlap
bool free_no_overlap(void) {
    for (size_t i = 0; i < memory.count; i++) {
        if (!memory.chunks[i].used) continue;
        for (size_t j = i + 1; j < memory.count; j++) {
            if (!memory.chunks[j].used) continue;
            uint8_t* a_start = memory.chunks[i].ptr;
            uint8_t* a_end   = a_start + memory.chunks[i].size;
            uint8_t* b_start = memory.chunks[j].ptr;
            uint8_t* b_end   = b_start + memory.chunks[j].size;
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
    for (size_t i = 0; i < memory.count; ++i) {
        Chunk* c = &memory.chunks[i];
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
    for (size_t i = 0; i < memory.count; ++i) {
        Chunk* c = &memory.chunks[i];
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
    for (int i=0;i<memory.count;++i) {
        Chunk c = memory.chunks[i];
        if (c.used) dbg("[mem] ptr = %15p | %-4zu | %s:%d", c.ptr, c.size, c.file, c.line);
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


void *___malloc(size_t size, const char* file, const int line) {
    
    if (size == 0) {
        if (_heap_wrn) wrn("attempt to alloc %zu bytes in memory would not be accepted, %s:%d", size, file, line);
        return NULL;
    }
    
    // CORRECT
    if (memory.count >= CHUNKS_SIZE) {
        if (_heap_wrn) wrn("[mem/alloc] chunk table full (%s:%d)", file, line);
        return NULL;
    }

    if (memory.chunks[memory.count].used) {
        if (_heap_wrn) wrn("[mem/alloc] chunk table is overwritten !! (%s:%d) by previous call at (%s:%d)",
            file,
            line,
            memory.chunks[memory.count].file,
            memory.chunks[memory.count].line);
    }

    // scan for a freed slot that fits — reuse it
    for (size_t i = 0; i < memory.count; ++i) {
        if (!memory.chunks[i].used && memory.chunks[i].size >= size) {
            memory.chunks[i].used = true;
            memory.chunks[i].file = (char*)file;
            memory.chunks[i].line = line;
            memory.chunks[i].size = size;  // keep or trim to requested size
            if (_heap_trc) trc("[mem/alloc] reuse slot %zu  ptr=%p  size=%zu (%s:%d)",
                               i, memory.chunks[i].ptr, size, file, line);
            return (void*)memory.chunks[i].ptr;
        }
    }
    
    
    const Chunk last = memory.count == 0 ? ((Chunk){&_heap[0]}) : memory.chunks[memory.count-1];
    uint8_t* ptr = last.ptr + last.size;

    if (ptr + size > _heap + MEMORY_SIZE) {
        if (_heap_wrn) err("[mem/alloc] OOM (%s:%d)", file, line);
        return NULL;
    }

    memory.chunks[memory.count++] = (Chunk) { ptr, size, (char*)file, line, true };
    return (void*)ptr;
}

char* ___strdup(const char* str, const char* file, int line) {
    if (str == NULL) return NULL;
    size_t len = strlen(str) + 1;
    char*  buf = (char*)___malloc(len, file, line);
    if (!buf) return NULL;
    memcpy(buf, str, len);
    return buf;
}

void ___free(void *_Nullable_ptr, const char* file, const int line) {
    if (memory.count == 0) {
        if (_heap_wrn) wrn("[mem/free] nothing is allocated yet %s:%d",file,line);
        return;
    }
    // after freeing a chunk, move all non-free chunks next to it to the left
    if (_Nullable_ptr == NULL) {
        if (_heap_wrn) wrn("[mem/free] attempting to free a NULL pointer from %s:%d",file,line);
        return;
    }
    uint8_t* ptr = (uint8_t*)_Nullable_ptr;
    //find the index of the chunk containing this pointer
    // size_t sum = 0;
    for (size_t i=0;i<memory.count;++i) {     
        // sum += memory.chunks[i].size;   
        if (memory.chunks[i].ptr == ptr) {
            if (_heap_trc) trc("free %p", ptr);
            memory.chunks[i].used = false;
            memset(ptr,0,memory.chunks[i].size);
            return;
        }
    }
    if (_heap_wrn) wrn("[mem/free] failed to locate pointer (%p) from %s:%d",_Nullable_ptr,file,line);
}


// compact — call explicitly when you want to defragment
void __heap_compact(void) {
    size_t write = 0;
    for (size_t i = 0; i < memory.count; ++i) {
        if (!memory.chunks[i].used) continue;

        size_t read = (size_t)(memory.chunks[i].ptr - _heap);
        if (read != write) {
            memmove(_heap + write, _heap + read, memory.chunks[i].size);
            memory.chunks[i].ptr = _heap + write;
        }
        memory.chunks[write++] = memory.chunks[i];  // pack chunk table too
    }
    // zero out reclaimed space and clear stale chunk slots
    size_t new_used = 0;
    for (size_t i = 0; i < write; i++) new_used += memory.chunks[i].size;
    memset(_heap + new_used, 0, MEMORY_SIZE - new_used);
    for (size_t i = write; i < memory.count; i++) memory.chunks[i] = (Chunk){0};
    memory.count = write;
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
    // realloc(NULL, size) == malloc(size)
    if (!_Nullable_ptr) return ___malloc(size, file, line);

    // realloc(ptr, 0) == free(ptr)
    if (size == 0) {
        ___free(_Nullable_ptr, file, line);
        return NULL;
    }

    // find the existing chunk
    uint8_t* ptr = (uint8_t*)_Nullable_ptr;
    for (size_t i = 0; i < memory.count; ++i) {
        Chunk* c = &memory.chunks[i];
        if (c->ptr != ptr || !c->used) continue;

        // case 1: fits in existing slot — shrink in place
        if (size <= c->size) {
            if (_heap_trc) trc("[mem/realloc] shrink %p  %zu → %zu (%s:%d)",
                               ptr, c->size, size, file, line);
            memset(ptr + size, 0, c->size - size);  // zero released tail
            c->size = size;
            c->file = (char*)file;
            c->line = line;
            return (void*)ptr;
        }

        // case 2: needs to grow — alloc new, copy, free old
        void* new_ptr = ___malloc(size, file, line);
        if (!new_ptr) {
            if (_heap_wrn) wrn("[mem/realloc] OOM growing %p  %zu → %zu (%s:%d)",
                               ptr, c->size, size, file, line);
            return NULL;
        }

        memcpy(new_ptr, ptr, c->size);          // copy old payload
        ___free(_Nullable_ptr, file, line);        // release old slot
        return new_ptr;
    }

    if (_heap_wrn) wrn("[mem/realloc] pointer %p not found (%s:%d)",
                       _Nullable_ptr, file, line);
    return NULL;
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


#define MALLOC       malloc       // (p)             ___malloc(p,    __FILE__,__LINE__)
#define FREE         free         // (p)               ___free(p,    __FILE__,__LINE__)
#define CALLOC       calloc       // (n,s)           ___calloc(n,s,  __FILE__,__LINE__)
#define REALLOC      realloc      // (p,s)          ___realloc(p,s,  __FILE__,__LINE__)
#define REALLOCARRAY reallocarray // (p,n,s)   ___reallocarray(p,n,s,__FILE__,__LINE__)
#define STRDUP       strdup       // (s)             ___strdup(s,   __FILE__, __LINE__)
#define MEMMOVE      memmove      // (d,s,n)        ___memmove(d,s,n,__FILE__,__LINE__)
#define PRINT_MEMORY()        ___print_memory ()


#endif// HEAP_H_
