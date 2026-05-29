#ifndef STACK_MEMORY_H_
#define STACK_MEMORY_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "logger.h"

#define MALLOC  my_malloc
#define REALLOC my_realloc
#define CALLOC  my_calloc
#define FREE    my_free

// -- arena config -------\------------------------------------------------------

#define HEAP_SIZE   (1024 * 1024 * 24) // 24 MB static buffer
#define ALIGN       (8)                // 8-byte alignment for all platforms
#define ALIGN_UP(n) (((n) + (ALIGN-1)) & ~(ALIGN-1))

// -- block header -------------------------------------------------------------

typedef struct Block {
    size_t       size;   // payload size (excludes header)
    bool         free;
    struct Block* next;
} Block;

#define HEADER_SIZE ALIGN_UP(sizeof(Block))

// -- static heap --------------------------------------------------------------

static uint8_t  heap[HEAP_SIZE] = {0};
static Block*   heap_head = NULL;

// -- init (called once on first alloc) ----------------------------------------

static void heap_init(void) {
    heap_head        = (Block*)heap;
    heap_head->size  = HEAP_SIZE - HEADER_SIZE;
    heap_head->free  = true;
    heap_head->next  = NULL;
}

// -- split a block if remainder is large enough -------------------------------

static void block_split(Block* b, size_t size) {
    if (b->size < size + HEADER_SIZE + ALIGN) return;  // not worth splitting

    Block* split  = (Block*)((uint8_t*)b + HEADER_SIZE + size);
    split->size   = b->size - size - HEADER_SIZE;
    split->free   = true;
    split->next   = b->next;

    b->size = size;
    b->next = split;
}

// -- coalesce adjacent free blocks --------------------------------------------

static void heap_coalesce(void) {
    Block* cur = heap_head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += HEADER_SIZE + cur->next->size;
            cur->next  = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

// -- find first fitting free block (first-fit) --------------------------------

static Block* block_find(size_t size) {
    Block* cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size) return cur;
        cur = cur->next;
    }
    return NULL;
}

// -- public API ---------------------------------------------------------------

void* my_malloc(size_t size) {
    if (size == 0) {
        wrn("why would someone request to allocate a zero sized memory space ???");
        return NULL;
    }
    if (!heap_head) heap_init();

    size = ALIGN_UP(size);

    Block* b = block_find(size);
    if (!b) { heap_coalesce(); b = block_find(size); }
    if (!b) {
        wrn("running low on memory !!!");
        return NULL;  // OOM
    }

    block_split(b, size);
    b->free = false;

    return (uint8_t*)b + HEADER_SIZE;
}

void my_free(void* ptr) {
    if (!ptr) return;

    Block* b = (Block*)((uint8_t*)ptr - HEADER_SIZE);
    if (!b) {
        wrn("attempting to free memory of an invalid pointer !");
        return;
    }
    b->free  = true;

    heap_coalesce();  // merge immediately to prevent fragmentation
}

void* my_calloc(size_t count, size_t size) {
    size_t total = count * size;
    void*  ptr   = my_malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* my_realloc(void* ptr, size_t size) {
    if (!ptr)    return my_malloc(size);
    if (!size) { my_free(ptr); return NULL; }

    Block* b = (Block*)((uint8_t*)ptr - HEADER_SIZE);

    // already fits — try to split off the surplus
    if (b->size >= ALIGN_UP(size)) {
        block_split(b, ALIGN_UP(size));
        return ptr;
    }

    // need a bigger block — alloc, copy, free
    void* new_ptr = my_malloc(size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, b->size);  // copy only the old payload
    my_free(ptr);
    return new_ptr;
}


#endif//STACK_MEMORY_H_
