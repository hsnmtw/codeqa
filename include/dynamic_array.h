#ifndef DA_H_
#define DA_H_

#define COMMON_IMPL
#include "common.h"
#include <stdbool.h>
#include <errno.h>

#define HSET_EMPTY NULL
#define HSET_LOAD  0.7

typedef struct {
    char** slots;
    size_t capacity;
    size_t count;
} HashSet;

typedef struct {
    char** items;
    size_t len;
    size_t capacity;
    bool   stack;
} DynamicArray;

#define DA_INITIAL_CAPACITY 8
#define DA_GROWTH_FACTOR    2

// ------------------------------------------------
//  counts variadic arguments at compile time
// ------------------------------------------------
#define _DA_NARGS(...) (sizeof((const char*[]){__VA_ARGS__}) / sizeof(const char*))

// -----------------------------------------
//  constructs a stack baed dynamic array
// -----------------------------------------
#define DA_OF(...) ((DynamicArray){     \
    .items    = (char*[]){__VA_ARGS__}, \
    .len      = _DA_NARGS(__VA_ARGS__), \
    .capacity = _DA_NARGS(__VA_ARGS__), \
    .stack    = true                    \
})

// -------------------------------------------------------------
// Dynamic Array Operations
// -------------------------------------------------------------
typedef char* (*FunctionStringToString)(const char*, size_t index);
typedef bool (*FunctionStringPredicate)(const char*, size_t index);

void da_map(DynamicArray* src, DynamicArray* dest, FunctionStringToString func);
void da_filter(DynamicArray* src, DynamicArray* dest, FunctionStringPredicate func);
void da_distinct(DynamicArray* src, DynamicArray* dest);
size_t da_count(DynamicArray* da, FunctionStringPredicate func);

int  da_locate(DynamicArray *da, const char *value);
bool da_remove(DynamicArray* da, int index);
void da_init(DynamicArray* da);
void da_clear(DynamicArray* da);
void da_free(DynamicArray* da);
void da_sort(DynamicArray* da);
char* da_pop(DynamicArray* da);
void da_push(DynamicArray* da, const char* value);
void da_queue(DynamicArray* da, const char* value);


#ifdef DA_IMPL

void da_queue(DynamicArray* da, const char* value) {
    if (da == NULL) return;

    if (da->len >= da->capacity) {
        // extend the array by one item
        da_push(da,"");
        // move elements of array one item to right
        // void *memmove(void dest[.n], const void src[.n], size_t n);
        memmove(&da->items[1], // char* dest
                &da->items[0], // char* src
                (da->len-1) * sizeof(char *));        
    } else {
        // move elements of array one item to right
        memmove(&da->items[1],
                &da->items[0],
                (da->len++) * sizeof(char *));
    }

    //place the value at top of the list
    da->items[0] = sdup(value);
}

char* da_pop(DynamicArray* da) {
    if (da == NULL || da->items == NULL || da->len == 0 || !da->items[0]) return NULL;
    char* result = sdup(da->items[0]);
    if (!da_remove(da,0)) {
        WRN("%s: failed to pop an item from the list : %s",__FUNCTION__,strerror(errno));
        return NULL;
    }
    return result;
}

int da_locate(DynamicArray *da, const char *value) {
    if (da == NULL || da->items == NULL || value == NULL) {
        return -1;
    }

    for (size_t i = 0; i < da->len; i++) {
        if (da->items[i] == value) {             // 1. pointer equality (free)
            return (int)i;
        }
        if (strcmp(da->items[i], value) == 0) {  // 2. content equality
            return (int)i;
        }
    }

    return -1;
}

void da_push(DynamicArray *da, const char *value) {
    // --- grow if needed ---
    if (da->len >= da->capacity) {
        size_t new_cap = da->capacity == 0 || da->len == 0
            ? DA_INITIAL_CAPACITY
            : da->capacity * DA_GROWTH_FACTOR;

        char **new_items;

        if (da->stack) {
            // first grow from stack — must copy to heap manually
            new_items = malloc(new_cap * sizeof(char *));
            if (!new_items) { 
                ERR("%s: out of memory\n", __FUNCTION__);
                return;
            }
            memcpy(new_items, da->items, da->len * sizeof(char *));
        } else {
            da->stack = false;   // now heap-owned, safe to realloc from now on
            new_items = realloc(da->items, new_cap * sizeof(char *));
            if (!new_items) { 
                ERR("%s: out of memory\n", __FUNCTION__);
                return;
            }
        }


        da->items    = new_items;
        da->capacity = new_cap;
    }

    // heap copy — caller doesn't need to keep value alive
    da->items[da->len++] = sdup(value);
}

void da_free(DynamicArray *da) {
    if(!da || !da->items) {
        return;
    }

    if (!da->stack) {
        for (size_t i = 0; i < da->len; i++) {
            if (da->items[i] != NULL) {
                free(da->items[i]);
            }
        }
        if (da->items != NULL) {
            free(da->items);
        }
    }

    da->items    = NULL;
    da->len      = 0;
    da->capacity = 0;
    da->stack    = false;
}

bool da_remove(DynamicArray *da, int index) {
    if (da == NULL || da->items == NULL) return false;
    if (index < 0 || (size_t)index >= da->len) return false;
    
    memmove(&da->items[index],
            &da->items[index + 1],
            (da->len - (size_t)index - 1) * sizeof(char *));
    
    da->len--;
    return true;
}

void da_init(DynamicArray *da) {
    if (da == NULL) {
        *da = (DynamicArray){0};
    }
    if (da->len>0 && da->items) {
        da_free(da);
        return;
    }
    da->items    = NULL;
    da->len      = 0;
    da->capacity = 0;
    da->stack    = false;
}

void da_clear(DynamicArray *da) {
    if (da == NULL) {
        return;
    }
    da->len = 0;
}

void da_filter(DynamicArray* src, DynamicArray* dest, FunctionStringPredicate pred) {
    if (!src || !pred || !dest) {
        return;
    }

    char** new_items = malloc(src->len * sizeof(char*));
    if (!new_items) {
        ERR("%s : failed to allocate some memory to perform filter on array ! : %s", __FUNCTION__, strerror(errno));
        return;
    }

    size_t count = 0;
    for (size_t i = 0; i < src->len; i++) {
        if (pred(src->items[i],i)) {
            new_items[count] = sdup(src->items[i]);
            if (!new_items[count]) {
                for (size_t j = 0; j < count; j++) {
                    free(new_items[j]);
                }
                free(new_items);
                ERR("%s : failed to allocate some memory to perform filter on array ! : %s", __FUNCTION__, strerror(errno));
                return;
            }
            count++;
        }
    }

    // free existing result contents before overwriting
    if (dest->items) {
        for (size_t i = 0; i < dest->len; i++) {
            free(dest->items[i]);
        }
        free(dest->items);
    }

    dest->items    = new_items;
    dest->len      = count;
    dest->capacity = src->len;  // allocated da->len, used count
    dest->stack    = src->stack;
}

static size_t hset_hash(const char* s, size_t cap) {
    size_t h = 14695981039346656037ULL;  // FNV-1a
    while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
    return h % cap;
}

static bool hset_init(HashSet* hs, size_t cap) {
    hs->slots    = calloc(cap, sizeof(char*));  // NULL == empty
    hs->capacity = cap;
    hs->count    = 0;
    return hs->slots != NULL;
}

static bool hset_contains(HashSet* hs, const char* s) {
    size_t i = hset_hash(s, hs->capacity);
    while (hs->slots[i] != HSET_EMPTY) {
        if (strcmp(hs->slots[i], s) == 0) return true;
        i = (i + 1) % hs->capacity;  // linear probe
    }
    return false;
}

// returns false on alloc failure
static bool hset_insert(HashSet* hs, const char* s) {
    size_t i = hset_hash(s, hs->capacity);
    while (hs->slots[i] != HSET_EMPTY) {
        if (strcmp(hs->slots[i], s) == 0) return true;  // already present
        i = (i + 1) % hs->capacity;
    }
    hs->slots[i] = (char*)s;  // borrowed — hset does NOT own the string
    hs->count++;
    return true;
}

static void hset_free(HashSet* hs) {
    free(hs->slots);
}

// -- da_distinct --------------------------------------------------------------

void da_distinct(DynamicArray* src, DynamicArray* dest) {
    if (!src || !dest) return;
    if (src->len == 0) {
        dest->items    = NULL;
        dest->len      = 0;
        dest->capacity = 0;
        dest->stack    = src->stack;
        return;
    }

    // size capacity so load factor stays under HSET_LOAD
    size_t hcap = (size_t)(src->len / HSET_LOAD) + 1;
    HashSet seen;
    if (!hset_init(&seen, hcap)) return;

    char** new_items = malloc(src->len * sizeof(char*));
    if (!new_items) { hset_free(&seen); return; }

    size_t count = 0;
    for (size_t i = 0; i < src->len; i++) {
        if (hset_contains(&seen, src->items[i])) continue;

        new_items[count] = strdup(src->items[i]);
        if (!new_items[count]) {
            for (size_t j = 0; j < count; j++) free(new_items[j]);
            free(new_items);
            hset_free(&seen);
            return;
        }

        hset_insert(&seen, src->items[i]);  // borrow original, not the copy
        count++;
    }

    hset_free(&seen);

    // free existing dest contents before overwriting
    if (dest->items) {
        for (size_t i = 0; i < dest->len; i++) free(dest->items[i]);
        free(dest->items);
    }

    dest->items    = new_items;
    dest->len      = count;
    dest->capacity = src->len;
    dest->stack    = src->stack;
}

size_t da_count(DynamicArray* da, FunctionStringPredicate pred) {
    if (!da || !pred || !da->items || da->len == 0) {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < da->len; i++) {
        if (pred(da->items[i],i)) {
            count++;
        }
    }

    return count;
}

void da_map(DynamicArray* src, DynamicArray* dest, FunctionStringToString func) {
    if (!src || !func || !dest) {
        return;
    }

    char** new_items = malloc(src->len * sizeof(char*));
    if (!new_items) {
        ERR("%s : failed to allocate some memory to perform filter on array ! : %s", __FUNCTION__, strerror(errno));
        return;
    }

    for (size_t i = 0; i < src->len; i++) {
        new_items[i] = func(src->items[i],i);
        if (!new_items[i]) {
            for (size_t j = 0; j < i; j++) {
                free(new_items[j]);
            }
            free(new_items);
            ERR("%s : failed to allocate some memory to perform filter on array ! : %s", __FUNCTION__, strerror(errno));
            return;
        }
    }

    // free existing result contents before overwriting
    if (dest->items) {
        for (size_t i = 0; i < dest->len; i++) {
            free(dest->items[i]);
        }
        free(dest->items);
    }

    dest->items    = new_items;
    dest->len      = src->len;
    dest->capacity = src->len;
    dest->stack    = src->stack;
}

int cmp_strings(const void* a, const void* b) {
    if (!a || !b) {
        return -1;
    }
    return strcmp(*(const char**)a, *(const char**)b);
}

void da_sort(DynamicArray* da) {
    if (!da || !da->items || da->len < 2) {
        return;
    }
    qsort(da->items, da->len, sizeof(char*), cmp_strings);
}
#endif//DA_IMPL

#endif//DA_H_
