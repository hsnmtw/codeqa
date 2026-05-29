#ifndef COLLECTIONS_H_
#define COLLECTIONS_H_

static bool ___enable_debug = false;

// #define COMMON_IMPL
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

typedef struct {
    char* key;
    void* value;
    bool  taken;
} Pair;

typedef struct {
    Pair* items;
    size_t len;
    size_t capacity;
} Map;


#define DA_INITIAL_CAPACITY 8
#define DA_GROWTH_FACTOR    2

// ------------------------------------------------
//  counts variadic arguments at compile time
// ------------------------------------------------
#define _DA_NARGS(...) PP_NARG(__VA_ARGS__)
// #define _DA_NARGS(...) (sizeof((const char*[]){__VA_ARGS__}) / sizeof(const char*))

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

uint64_t hash(const char* key);

void map_init(Map* map);
void map_set(Map* map, const char* key, const void* value);
void* map_get(Map* map, const char* key);
void map_free(Map* map);
void map_keys(Map* map,DynamicArray *da);

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
char* da_dequeue(DynamicArray* da);


#ifdef COLLECTIONS_IMPL

// -------- Map ------------------------ 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// -- primes for capacity growth -----------------------------------------------

static const size_t PRIMES[] = {
    11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437,
    102877, 205759, 411527, 823117, 1646237, 3292489, 6584983, 13169977
};
static const size_t PRIMES_LEN = sizeof(PRIMES) / sizeof(PRIMES[0]);
static const double LOAD_FACTOR = 0.4;

static size_t next_prime(size_t min) {
    for (size_t i = 0; i < PRIMES_LEN; i++) {
        if (PRIMES[i] > min) {
            return PRIMES[i];
        }
    }
    return min * 2 + 1;  // fallback for very large maps
}





// -- wyhash (fastest modern non-crypto hash) ----------------------------------
// https://github.com/wangyi-fudan/wyhash

// static inline uint64_t wymix(uint64_t a, uint64_t b) {
//     __uint128_t r = (__uint128_t)a * b;
//     return (uint64_t)(r >> 64) ^ (uint64_t)r;
// }

// static uint64_t wyhash(const char* key) {
//     const uint8_t* p   = (const uint8_t*)key;
//     size_t         len = strlen(key);
//     uint64_t       h   = 0x9E3779B97F4A7C15ULL ^ len;

//     while (len >= 8) {
//         uint64_t word;
//         memcpy(&word, p, 8);
//         h = wymix(h, word);
//         p += 8; len -= 8;
//     }

//     uint64_t tail = 0;
//     switch (len) {
//         case 7: tail |= (uint64_t)p[6] << 48; // fallthrough
//         case 6: tail |= (uint64_t)p[5] << 40; // fallthrough
//         case 5: tail |= (uint64_t)p[4] << 32; // fallthrough
//         case 4: tail |= (uint64_t)p[3] << 24; // fallthrough
//         case 3: tail |= (uint64_t)p[2] << 16; // fallthrough
//         case 2: tail |= (uint64_t)p[1] <<  8; // fallthrough
//         case 1: tail |= (uint64_t)p[0];
//                 h = wymix(h, tail);
//     }

//     return wymix(h, 0x60bee2bee120fc15ULL);
// }

// -- tombstone sentinel -------------------------------------------------------

static uint64_t djb2(const char* key) {
    unsigned long result = 5381;
    const size_t l = strlen(key);    
    for (size_t c=0;c<l;c++) {
        result = ((result << 5) + result) + (unsigned long)key[c];
    }
    return result;
}

// #include <math.h>
// static uint64_t myhash(const char* key) {
//     const uint64_t l = strlen(key);
//     long sum = (long)pow(l,l); 
//     for (uint64_t i = 0; i < l; i++)
//     {
//         unsigned char c = (byte)key[i];
//         uint64_t p = next_prime(i/2);
//         sum =  (sum*p ^ c*(l-i)) << 3;
//     }
//     int64_t hash = (int64_t)sum;
//     return hash < 0 ? -hash : hash;
// }

uint64_t hash(const char* key) {
    // return myhash(key);
    return djb2(key);
    // return wyhash(key);
}

#define TOMB ((char*)-1)   // deleted slot marker
#define is_empty(p)  ((p).key == NULL)
#define is_tomb(p)   ((p).key == TOMB)
#define is_live(p)   ((p).key != NULL && (p).key != TOMB && p.taken)

// -- internal probe -----------------------------------------------------------

// double hashing: step = 1 + (hash / cap) % (cap - 1)
// eliminates primary clustering without a second hash function
static size_t probe_step(uint64_t hash, size_t cap) {
    return 1 + (hash / cap) % (cap - 1);
}

static size_t find_slot(Pair* items, size_t cap, const char* key, uint64_t hash) {
    size_t i    = hash % cap;
    size_t step = probe_step(hash, cap);
    size_t first_tomb = SIZE_MAX;

    while (true) {
        if (is_empty(items[i])) {
            return (first_tomb != SIZE_MAX) ? first_tomb : i;
        }
        if (is_tomb(items[i])) {
            if (first_tomb == SIZE_MAX) first_tomb = i;
        } else if (strcmp(items[i].key, key) == 0) {
            return i;
        }
        i = (i + step) % cap;
    }
}

// -- rehash -------------------------------------------------------------------

static bool rehash(Map* map, size_t new_cap) {
    Pair* new_items = CALLOC(new_cap, sizeof(Pair));
    if (!new_items) return false;

    for (size_t i = 0; i < map->capacity; i++) {
        if (!is_live(map->items[i])) continue;
        uint64_t h    = hash(map->items[i].key);
        size_t   slot = find_slot(new_items, new_cap, map->items[i].key, h);
        new_items[slot] = map->items[i];  // key/value ownership transfers
    }

    FREE(map->items);
    map->items    = new_items;
    map->capacity = new_cap;
    return true;
}

// -- public API ---------------------------------------------------------------

void map_keys(Map* map,DynamicArray *da) {
    if (!map || !da) return;
    da_init(da);
    for(size_t i=0;i<map->capacity;++i) {
        if (map->items[i].taken) {
            da_push(da, sdup(map->items[i].key));
        }
    }
}

void map_init(Map* map) {
    map->len      = 0;
    map->capacity = PRIMES[0];  // start at 11
    map->items    = CALLOC(map->capacity, sizeof(Pair));
}

void map_set(Map* map, const char* key, const void* value) {
    if (!map->items) return;

    // grow before insert to keep load factor healthy
    if ((double)(map->len + 1) / map->capacity > LOAD_FACTOR) {
        size_t new_cap = next_prime(map->capacity);
        if (!rehash(map, new_cap)) return;
    }

    uint64_t h    = hash(key);
    size_t   slot = find_slot(map->items, map->capacity, key, h);

    if (is_live(map->items[slot])) {
        // update existing key — free old value if owned, here we just overwrite
        map->items[slot].value = (void*)value;
        return;
    }

    // new slot (empty or tombstone)
    map->items[slot].key   = strdup(key);
    map->items[slot].value = (void*)value;
    map->items[slot].taken = true;
    map->len++;
}

void* map_get(Map* map, const char* key) {
    if (!map->items || map->len == 0) return NULL;

    uint64_t h    = hash(key);
    size_t   slot = find_slot(map->items, map->capacity, key, h);

    return is_live(map->items[slot]) ? map->items[slot].value : NULL;
}

void map_free(Map* map) {
    if (!map->items) return;
    for (size_t i = 0; i < map->capacity; i++)
        if (is_live(map->items[i])) FREE(map->items[i].key);
    FREE(map->items);
    map->items    = NULL;
    map->len      = 0;
    map->capacity = 0;
}
// -------- Map ------------------------

void da_queue(DynamicArray* da, const char* value) {
    if (da == NULL) return;
    //push an item normally
    da_push(da,value);
}

char* da_dequeue(DynamicArray* da) {
    const char* f_name = nameof(da_dequeue);
    if (da == NULL || !da->items || da->len == 0) return NULL;
    // pop the item at top, and shift array to left
    char* value = sdup(da->items[0]);
    if (!da_remove(da,0)) {
        wrn("%s: failed to remove item from array", f_name);
    }
    return value;
}

char* da_pop(DynamicArray* da) {
    const char* f_name = nameof(da_pop);
    if (da == NULL || da->items == NULL || da->len == 0 || !da->items[0]) return NULL;
    char* result = sdup(da->items[da->len-1]);
    if (!da_remove(da,(int)(da->len-1))) {
        wrn("%s: failed to pop an item from the list : %s",f_name,strerror(errno));
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
    const char* f_name = nameof(da_pop);
    // --- grow if needed ---
    if (da->len >= da->capacity) {
        size_t new_cap = da->capacity == 0 || da->len == 0
            ? DA_INITIAL_CAPACITY
            : da->capacity * DA_GROWTH_FACTOR;

        char **new_items;

        if (da->stack) {
            // first grow from stack — must copy to heap manually
            new_items = MALLOC(new_cap * sizeof(char *));
            if (!new_items) { 
                err("%s: out of memory\n", f_name);
                return;
            }
            memcpy(new_items, da->items, da->len * sizeof(char *));
        } else {
            da->stack = false;   // now heap-owned, safe to realloc from now on
            new_items = REALLOC(da->items, new_cap * sizeof(char *));
            if (!new_items) { 
                err("%s: out of memory\n", f_name);
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
                FREE(da->items[i]);
            }
        }
        if (da->items != NULL) {
            FREE(da->items);
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
    const char* f_name = nameof(da_filter);
    if (!src || !pred || !dest || !src->len) {
        return;
    }

    char** new_items = MALLOC(src->len * sizeof(char*));
    if (!new_items) {
        err("%s : failed to allocate some memory to perform filter on array ! : %s", f_name, strerror(errno));
        return;
    }

    size_t count = 0;
    for (size_t i = 0; i < src->len; i++) {
        if (pred(src->items[i],i)) {
            new_items[count] = sdup(src->items[i]);
            if (!new_items[count]) {
                for (size_t j = 0; j < count; j++) {
                    FREE(new_items[j]);
                }
                FREE(new_items);
                err("%s : failed to allocate some memory to perform filter on array ! : %s", f_name, strerror(errno));
                return;
            }
            count++;
        }
    }

    // free existing result contents before overwriting
    if (dest->items) {
        for (size_t i = 0; i < dest->len; i++) {
            FREE(dest->items[i]);
        }
        FREE(dest->items);
    }

    dest->items    = new_items;
    dest->len      = count;
    dest->capacity = src->len;  // allocated da->len, used count
    dest->stack    = src->stack;
}

// static size_t hset_hash(const char* s, size_t cap) {
//     size_t h = 14695981039346656037ULL;  // FNV-1a
//     while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
//     return h % cap;
// }

static bool hset_init(HashSet* hs, size_t cap) {
    hs->slots    = CALLOC(cap, sizeof(char*));  // NULL == empty
    hs->capacity = cap;
    hs->count    = 0;
    return hs->slots != NULL;
}

static bool hset_contains(HashSet* hs, const char* s) {
    // size_t i = hset_hash(s, hs->capacity);
    size_t i = hash(s) % (hs->capacity);
    while (hs->slots[i] != HSET_EMPTY) {
        if (strcmp(hs->slots[i], s) == 0) return true;
        i = (i + 1) % hs->capacity;  // linear probe
    }
    return false;
}

// returns false on alloc failure
static bool hset_insert(HashSet* hs, const char* s) {
    // size_t i = hset_hash(s, hs->capacity);
    size_t i = hash(s)%(hs->capacity);
    while (hs->slots[i] != HSET_EMPTY) {
        if (strcmp(hs->slots[i], s) == 0) return true;  // already present
        i = (i + 1) % hs->capacity;
    }
    hs->slots[i] = (char*)s;  // borrowed — hset does NOT own the string
    hs->count++;
    return true;
}

static void hset_free(HashSet* hs) {
    FREE(hs->slots);
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

    char** new_items = MALLOC(src->len * sizeof(char*));
    if (!new_items) { hset_free(&seen); return; }

    size_t count = 0;
    for (size_t i = 0; i < src->len; i++) {
        if (hset_contains(&seen, src->items[i])) continue;

        new_items[count] = strdup(src->items[i]);
        if (!new_items[count]) {
            for (size_t j = 0; j < count; j++) FREE(new_items[j]);
            FREE(new_items);
            hset_free(&seen);
            return;
        }

        hset_insert(&seen, src->items[i]);  // borrow original, not the copy
        count++;
    }

    hset_free(&seen);

    // free existing dest contents before overwriting
    if (dest->items) {
        for (size_t i = 0; i < dest->len; i++) FREE(dest->items[i]);
        FREE(dest->items);
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
    const char* f_name = nameof(da_map);

    if (!src || !func || !dest || !src->len) {
        return;
    }

    char** new_items = MALLOC(src->len * sizeof(char*));
    if (!new_items) {
        err("%s : failed to allocate some memory to perform filter on array ! : %s", f_name, strerror(errno));
        return;
    }

    for (size_t i = 0; i < src->len; i++) {
        new_items[i] = func(src->items[i],i);
        if (!new_items[i]) {
            for (size_t j = 0; j < i; j++) {
                FREE(new_items[j]);
            }
            FREE(new_items);
            err("%s : failed to allocate some memory to perform filter on array ! : %s", f_name, strerror(errno));
            return;
        }
    }

    // free existing result contents before overwriting
    if (dest->items) {
        for (size_t i = 0; i < dest->len; i++) {
            FREE(dest->items[i]);
        }
        FREE(dest->items);
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
#endif//COLLECTIONS_IMPL

#endif//COLLECTIONS_H_
