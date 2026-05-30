
// #ifndef STACK_MEMORY_H_
// #define STACK_MEMORY_H_

// #include <stddef.h>
// #include <stdint.h>
// #include <string.h>
// #include <stdbool.h>
// #include <ctype.h>
// #include "logger.h"

// // -----------------------------------------------------------------------------
// // debug macros
// // -----------------------------------------------------------------------------

// #ifdef MEMORY_DEBUG

//   #define POISON_BYTE    0xCD   // uninitialized marker (same as MSVC debug heap)
//   #define FREED_BYTE     0xDD   // freed memory marker
//   #define PADDING_BYTE   0xFD   // boundary pad marker

//   #define POISON_SIZE    8      // bytes of padding after each payload

// // -- write poison -------------------------------------------------------------

//   static inline void poison_block(void* ptr, size_t size) {
//       memset(ptr, POISON_BYTE, size);
//   }

//   static inline void poison_freed(void* ptr, size_t size) {
//       memset(ptr, FREED_BYTE, size);
//   }

// // -- check: returns true if any byte looks uninitialized ----------------------

//   static inline bool is_poisoned(const void* ptr, size_t size) {
//       const uint8_t* p = (const uint8_t*)ptr;
//       for (size_t i = 0; i < size; i++)
//           if (p[i] == POISON_BYTE) return true;
//       return false;
//   }

//   static inline bool is_freed_poison(const void* ptr, size_t size) {
//       const uint8_t* p = (const uint8_t*)ptr;
//       for (size_t i = 0; i < size; i++)
//           if (p[i] == FREED_BYTE) return true;
//       return false;
//   }

// // -- boundary pad: written after payload, checked on free ---------------------

//   static inline void pad_write(void* ptr, size_t size) {
//       memset((uint8_t*)ptr + size, PADDING_BYTE, POISON_SIZE);
//   }

//   static inline bool pad_check(const void* ptr, size_t size, const char* file, int line) {
//       const uint8_t* p = (const uint8_t*)ptr + size;
//       if (!p || sizeof(p) < POISON_SIZE) return false;
//       for (size_t i = 0; i < POISON_SIZE; i++) {
//           if (p[i] != PADDING_BYTE) {
//               wrn("[mem] OVERFLOW  %zu bytes past payload  ptr=%p  (%s:%d)",
//                   i + 1, ptr, file, line);
//               return false;
//           }
//       }
//       return true;
//   }

//   bool mem_check_ptr(const void* ptr, size_t size, const char* file, int line);

//   #define MALLOC(size)          my_malloc_dbg(size,        __FILE__, __LINE__)
//   #define CALLOC(count, size)   my_calloc_dbg(count, size, __FILE__, __LINE__)
//   #define REALLOC(ptr, size)    my_realloc_dbg(ptr, size,  __FILE__, __LINE__)
//   #define FREE(ptr)             my_free_dbg((void**)&(ptr),         __FILE__, __LINE__)
//   #define STRDUP(str)           my_strdup_dbg(str,          __FILE__, __LINE__)
//   #define MEMMOVE(dest, src, n) my_memmove_dbg(dest, src, n, __FILE__, __LINE__)
//   #define MEM_CHECK(ptr, size)  mem_check_ptr(ptr, size, __FILE__, __LINE__)
// #else
//   #define MEM_CHECK(ptr, size)  false
//   #define MALLOC(size)          my_malloc(size)
//   #define CALLOC(count, size)   my_calloc(count, size)
//   #define REALLOC(ptr, size)    my_realloc(ptr, size)
//   #define FREE(ptr)             my_free(ptr)
//   #define STRDUP(str)           my_strdup(str)
//   #define MEMMOVE(dest, src, n) my_memmove(dest, src, n)
// #endif

// // -----------------------------------------------------------------------------
// // arena config
// // -----------------------------------------------------------------------------

// #define HEAP_SIZE   (1024 * 1024 * 24)   // 24 MB static buffer
// #define ALIGN       (8)                   // 8-byte alignment for all platforms
// #define ALIGN_UP(n) (((n) + (ALIGN-1)) & ~(ALIGN-1))

// // -----------------------------------------------------------------------------
// // block header
// // -----------------------------------------------------------------------------

// typedef struct Block {
//     size_t        size;          // aligned payload size (used by allocator)
//     size_t        requested;     // original size before ALIGN_UP (used by debug)
//     bool          free;
//     struct Block* next;
// #ifdef MEMORY_DEBUG
//     const char*   file;
//     int           line;
//     bool          dead;
//     uint32_t      canary;
// #endif
// } Block;

// #define HEADER_SIZE ALIGN_UP(sizeof(Block))

// #ifdef MEMORY_DEBUG
//   #define CANARY_VALUE 0xDEADBEEF
// #endif

// // -----------------------------------------------------------------------------
// // static heap
// // -----------------------------------------------------------------------------

// static uint8_t heap[HEAP_SIZE] = {0};
// static Block*  heap_head       = NULL;

// // -----------------------------------------------------------------------------
// // allocation tracker (debug only)
// // -----------------------------------------------------------------------------

// #ifdef MEMORY_DEBUG

// #define MAX_ALLOCS 4096

// typedef struct {
//     void*       ptr;
//     size_t      size;
//     const char* file;
//     int         line;
//     bool        freed;
// } AllocRecord;

// static AllocRecord alloc_table[MAX_ALLOCS];
// static size_t      alloc_count = 0;

// static void track_alloc(void* ptr, size_t size, const char* file, int line) {
//     for (size_t i = 0; i < alloc_count; i++) {
//         if (alloc_table[i].freed) {
//             alloc_table[i] = (AllocRecord){ ptr, size, file, line, false };
//             return;
//         }
//     }
//     if (alloc_count < MAX_ALLOCS)
//         alloc_table[alloc_count++] = (AllocRecord){ ptr, size, file, line, false };
//     else
//         wrn("[mem] alloc_table full — increase MAX_ALLOCS");
// }

// static void track_free(void* ptr, const char* file, int line) {
//     for (size_t i = 0; i < alloc_count; i++) {
//         if (alloc_table[i].ptr == ptr) {
//             if (alloc_table[i].freed) {
//                 wrn("[mem] DOUBLE FREE  %p  (%s:%d)", ptr, file, line);
//                 return;
//             }
//             alloc_table[i].freed = true;
//             return;
//         }
//     }
//     wrn("[mem] FREE of UNKNOWN ptr  %p  (%s:%d)", ptr, file, line);
// }

// #endif // MEMORY_DEBUG

// // -----------------------------------------------------------------------------
// // diagnostics
// // -----------------------------------------------------------------------------

// static inline void mem_report(void) {
// #ifdef MEMORY_DEBUG
//     size_t leaks      = 0;
//     size_t leak_bytes = 0;

//     inf("[mem] ── memory report ────────────────────────────────");
//     for (size_t i = 0; i < alloc_count; i++) {
//         AllocRecord* r = &alloc_table[i];
//         if (r->freed) continue;
//         leaks++;
//         leak_bytes += r->size;
//         wrn("[mem] LEAK  %6zu bytes  ptr=%-14p  %s:%d",
//             r->size, r->ptr, r->file, r->line);

//         // was this allocation ever written to?
//         if (is_poisoned(r->ptr, r->size))
//             wrn("[mem]   └─ NEVER WRITTEN (uninitialized since alloc)");

//         // boundary pad intact?
//         pad_check(r->ptr, r->size, r->file, r->line);
//     }

//     if (leaks == 0) inf("[mem] no leaks detected ✓");
//     else wrn("[mem] %zu leak(s) — %zu bytes not freed", leaks, leak_bytes);

//     inf("[mem] ────────────────────────────────────────────────");
// #endif
// }

// static inline void mem_stats(void) {
//     size_t total_blocks = 0, free_blocks = 0;
//     size_t used_bytes   = 0, free_bytes  = 0;

//     Block* cur = heap_head;
//     while (cur) {
//         total_blocks++;
//         if (cur->free) { free_blocks++; free_bytes += cur->size; }
//         else           {                used_bytes += cur->size; }
//         cur = cur->next;
//     }

//     inf("[mem] ── heap stats ──────────────────────────────────");
//     inf("[mem] heap size   : %d bytes",   HEAP_SIZE);
//     inf("[mem] used        : %zu bytes across %zu block(s)",
//         used_bytes, total_blocks - free_blocks);
//     inf("[mem] free        : %zu bytes across %zu block(s)",
//         free_bytes, free_blocks);
//     inf("[mem] total blocks: %zu", total_blocks);
//     inf("[mem] ────────────────────────────────────────────────");
// }

// // -----------------------------------------------------------------------------
// // internal helpers
// // -----------------------------------------------------------------------------

// static void heap_init(void) {
//     heap_head        = (Block*)heap;
//     heap_head->size  = HEAP_SIZE - HEADER_SIZE;
//     heap_head->free  = true;
//     heap_head->next  = NULL;
// #ifdef MEMORY_DEBUG
//     heap_head->canary = CANARY_VALUE;
//     heap_head->dead   = false;
//     heap_head->file   = NULL;
//     heap_head->line   = 0;
// #endif
// }

// static void block_split(Block* b, size_t size) {
//     if (b->size < size + HEADER_SIZE + ALIGN) return;

//     Block* split  = (Block*)((uint8_t*)b + HEADER_SIZE + size);
//     split->size   = b->size - size - HEADER_SIZE;
//     split->free   = true;
//     split->next   = b->next;
// #ifdef MEMORY_DEBUG
//     split->canary = CANARY_VALUE;
//     split->dead   = false;
//     split->file   = NULL;
//     split->line   = 0;
// #endif
//     b->size = size;
//     b->next = split;
// }

// static void heap_coalesce(void) {
//     Block* cur = heap_head;
//     while (cur && cur->next) {
//         if (cur->free && cur->next->free) {
//             cur->size += HEADER_SIZE + cur->next->size;
//             cur->next  = cur->next->next;
//         } else {
//             cur = cur->next;
//         }
//     }
// }

// static Block* block_find(size_t size) {
//     Block* cur = heap_head;
//     while (cur) {
//         if (cur->free && cur->size >= size) return cur;
//         cur = cur->next;
//     }
//     return NULL;
// }

// // -----------------------------------------------------------------------------
// // core allocators
// // -----------------------------------------------------------------------------

// static inline void* my_malloc(size_t size) {
//     if (size == 0) { wrn("[mem] zero-size malloc"); return NULL; }
//     if (!heap_head) heap_init();
//     size = ALIGN_UP(size);
//     Block* b = block_find(size);
//     if (!b) { heap_coalesce(); b = block_find(size); }
//     if (!b) { wrn("[mem] OOM"); return NULL; }
//     block_split(b, size);
//     b->free = false;
//     return (uint8_t*)b + HEADER_SIZE;
// }


// static inline void my_free(void* ptr) {
//     if (!ptr) return;
//     Block* b = (Block*)((uint8_t*)ptr - HEADER_SIZE);
//     if (!b) return;
//     if(!MEM_CHECK(ptr,8)) return;
// #ifdef MEMORY_DEBUG
//     if (b->dead) { 
//         wrn("[mem] [3] USE AFTER FREE  %p", ptr); 
//         return; 
//     }
//     b->dead = true;
// #endif
//     b->free = true;
//     heap_coalesce();
// }

// static inline void* my_calloc(size_t count, size_t size) {
//     size_t total = count * size;
//     void*  ptr   = my_malloc(total);
//     if (ptr) memset(ptr, 0, total);
//     return ptr;
// }

// static inline void* my_realloc(void* ptr, size_t size) {
//     if (!ptr)    return my_malloc(size);
//     if (!size) { my_free(ptr); return NULL; }
//     Block* b = (Block*)((uint8_t*)ptr - HEADER_SIZE);
//     if (b->size >= ALIGN_UP(size)) { block_split(b, ALIGN_UP(size)); return ptr; }
//     void* new_ptr = my_malloc(size);
//     if (!new_ptr) return NULL;
//     memcpy(new_ptr, ptr, b->size);
//     my_free(ptr);
//     return new_ptr;
// }

// static inline char* my_strdup(const char* str) {
//     if (!str) return NULL;
//     size_t len = strlen(str) + 1;
//     char*  buf = (char*)my_malloc(len);
//     if (!buf) return NULL;
//     memcpy(buf, str, len);
//     return buf;
// }

// static inline void* my_memmove(void* dest, const void* src, size_t n) {
//     if (!dest || !src || n == 0) return dest;
//     uint8_t*       d = (uint8_t*)dest;
//     const uint8_t* s = (const uint8_t*)src;
//     if (d == s) return dest;
//     if (d < s || d >= s + n) {
//         while (n--) *d++ = *s++;
//     } else {
//         d += n; s += n;
//         while (n--) *--d = *--s;
//     }
//     return dest;
// }

// // -----------------------------------------------------------------------------
// // debug wrappers
// // -----------------------------------------------------------------------------

// #ifdef MEMORY_DEBUG
// bool mem_check_ptr(const void* ptr, size_t size,
//                                   const char* file, int line) {
//     if (!ptr) {
//         wrn("[mem] NULL ptr access  (%s:%d)", file, line);
//         return false;
//     }

//     // bounds — is it inside our heap?
//     const uint8_t* heap_start = heap;
//     const uint8_t* heap_end   = heap + HEAP_SIZE;
//     if ((const uint8_t*)ptr < heap_start ||
//         (const uint8_t*)ptr >= heap_end) {
//         wrn("[mem] ptr=%p OUTSIDE heap  (%s:%d)", ptr, file, line);
//         return false;
//     }

//     Block* b = (Block*)((uint8_t*)ptr - HEADER_SIZE);

//     // uninitialized?
//     if (is_poisoned(ptr, size)) {
//         wrn("[mem] UNINITIALIZED READ  ptr=%p  size=%zu  (%s:%d)",
//             ptr, size,file, line);
//         return false;
//     }

//     // use-after-free?
//     if (is_freed_poison(ptr, size)) {
//         wrn("[mem] [1] USE AFTER FREE  ptr=%p  (%s:%d)", ptr, file, line);
//         return false;
//     }

//     // block marked dead?
//     if (b->dead) {
//         wrn("[mem] ACCESS TO DEAD BLOCK  ptr=%p  allocated at (%s:%d)  accessed at (%s:%d)",
//             ptr, b->file, b->line, file, line);
//         return false;
//     }

//     // canary intact?
//     if (b->canary != CANARY_VALUE) {
//         wrn("[mem] CANARY CORRUPT  ptr=%p  (%s:%d)", ptr, file, line);
//         return false;
//     }

//     // boundary pad intact?
//     return pad_check(ptr, size, file, line);
// }

  


// // -----------------------------------------------------------------------------
// // my_free_dbg  — check pad, then fill freed region with 0xDD
// // -----------------------------------------------------------------------------

// static inline void my_free_dbg(void** ptr, const char* file, int line) {
//     if (!ptr || !*ptr) return;

//     Block* b    = (Block*)((uint8_t*)*ptr - HEADER_SIZE);
//     //size_t size = b->size - POISON_SIZE;   // actual payload size
//     size_t size = b->requested;  // ← use original size, not aligned size

//     // 1. boundary overflow check
//     pad_check(*ptr, size, file, line);

//     // 2. use-after-free check — already freed?
//     if (is_freed_poison(*ptr, size))
//         wrn("[mem] [2] DOUBLE FREE or USE AFTER FREE  ptr=%p  (%s:%d)",
//             *ptr, file, line);

//     // 3. poison freed region so dangling reads are caught
//     poison_freed(*ptr, b->size);

//     track_free(*ptr, file, line);
//     my_free(*ptr);
//     *ptr = NULL;
// }

// static inline void* my_malloc_dbg(size_t size, const char* file, int line) {
//     void* ptr = my_malloc(size);
//     if (!ptr) return NULL;
    
//     Block* b  = (Block*)((uint8_t*)ptr - HEADER_SIZE);
//     b->file   = file;
//     b->line   = line;
//     b->canary = CANARY_VALUE;
//     b->dead   = false;
//     b->requested  = size;        // ← store original size
    
//     poison_block(ptr, size);     // 0xCD fill — uninitialized marker
//     pad_write(ptr, size);        // 0xFD boundary pad after payload

//     track_alloc(ptr, size, file, line);
//     return ptr;
// }

// static inline void* my_calloc_dbg(size_t count, size_t size,
//                                    const char* file, int line) {
//     void* ptr = my_calloc(count, size);
//     if (ptr) {
//         Block* b  = (Block*)((uint8_t*)ptr - HEADER_SIZE);
//         b->file   = file;
//         b->line   = line;
//         b->canary = CANARY_VALUE;
//         b->dead   = false;
//         track_alloc(ptr, count * size, file, line);
//     }
//     return ptr;
// }

// static inline void* my_realloc_dbg(void* ptr, size_t size,
//                                     const char* file, int line) {
//     // case 1: no existing ptr
//     if (!ptr) return my_malloc_dbg(size, file, line);

//     // case 2: zero size — free and return NULL
//     if (!size) {
//         my_free_dbg(&ptr, file, line);
//         return NULL;
//     }

//     Block* b = (Block*)((uint8_t*)ptr - HEADER_SIZE);

//     // case 3: fits in place — update record, no alloc/free
//     if (b->size >= ALIGN_UP(size)) {
//         block_split(b, ALIGN_UP(size));
//         for (size_t i = 0; i < alloc_count; i++) {
//             if (alloc_table[i].ptr == ptr && !alloc_table[i].freed) {
//                 alloc_table[i].size = size;
//                 alloc_table[i].file = file;
//                 alloc_table[i].line = line;
//                 break;
//             }
//         }
//         return ptr;
//     }

//     // case 4: needs bigger block
//     void* new_ptr = my_malloc_dbg(size, file, line);
//     if (!new_ptr) return NULL;
//     memcpy(new_ptr, ptr, b->size);
//     my_free_dbg(&ptr, file, line);
//     return new_ptr;
// }



// static inline char* my_strdup_dbg(const char* str,
//                                    const char* file, int line) {
//     if (str == NULL) return NULL;
//     size_t len = strlen(str) + 1;
//     char*  buf = (char*)my_malloc_dbg(len, file, line);
//     if (!buf) return NULL;
//     memcpy(buf, str, len);
//     return buf;
// }

// static inline void* my_memmove_dbg(void* dest, const void* src, size_t n,
//                                     const char* file, int line) {
//     uint8_t* heap_start = heap;
//     uint8_t* heap_end   = heap + HEAP_SIZE;

//     if ((uint8_t*)dest < heap_start || (uint8_t*)dest + n > heap_end)
//         wrn("[mem] MEMMOVE dest %p out of heap bounds (%s:%d)", dest, file, line);

//     if ((uint8_t*)src < heap_start || (uint8_t*)src + n > heap_end)
//         wrn("[mem] MEMMOVE src %p out of heap bounds (%s:%d)", src, file, line);

//     Block* b = (Block*)((uint8_t*)dest - HEADER_SIZE);
//     if (b->dead)
//         wrn("[mem] MEMMOVE into dead block %p (%s:%d)", dest, file, line);

//     return my_memmove(dest, src, n);
// }
// #endif // MEMORY_DEBUG
// #endif // STACK_MEMORY_H_
