#ifndef DA_H_
#define DA_H_

#define COMMON_IMPL
#include "common.h"
#include <stdbool.h>

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
void da_map(DynamicArray* da, FunctionStringToString func, DynamicArray* result);
void da_filter(DynamicArray* da, FunctionStringPredicate func, DynamicArray* result);
int  da_index_of(DynamicArray *da, const char *value);
void da_insert(DynamicArray* da, const char* value);
bool da_remove_at(DynamicArray* da, int index);
void da_clear(DynamicArray* da);
void da_free(DynamicArray* da);
void da_sort(DynamicArray* da);

#ifdef DA_IMPL
int da_index_of(DynamicArray *da, const char *value) {
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

void da_insert(DynamicArray *da, const char *value) {
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
                log_error(tmp_sprintf("%s: out of memory\n", __FUNCTION__));
                return;
            }
            memcpy(new_items, da->items, da->len * sizeof(char *));
        } else {
            da->stack = false;   // now heap-owned, safe to realloc from now on
            new_items = realloc(da->items, new_cap * sizeof(char *));
            if (!new_items) { 
                log_error(tmp_sprintf("%s: out of memory\n", __FUNCTION__));
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
    for (size_t i = 0; i < da->len; i++) {
        if (da->items[i] != NULL) {
            free(da->items[i]);
        }
    }
    if (da->items != NULL) {
        free(da->items);
    }
    da->items    = NULL;
    da->len      = 0;
    da->capacity = 0;
    da->stack    = false;
}

bool da_remove_at(DynamicArray *da, int index) {
    if (da == NULL || da->items == NULL) return false;
    if (index < 0 || (size_t)index >= da->len) return false;

    memmove(&da->items[index],
            &da->items[index + 1],
            (da->len - (size_t)index - 1) * sizeof(char *));

    da->len--;
    return true;
}

void da_clear(DynamicArray *da) {
    if (da == NULL) {
        return;
    }
    da->len = 0;
}

void da_filter(DynamicArray* da, FunctionStringPredicate pred, DynamicArray* result) {
    if (!da || !pred || !result) {
        return;
    }

    char** new_items = malloc(da->len * sizeof(char*));
    if (!new_items) {
        return;
    }

    size_t count = 0;
    for (size_t i = 0; i < da->len; i++) {
        if (pred(da->items[i],i)) {
            new_items[count] = sdup(da->items[i]);
            if (!new_items[count]) {
                for (size_t j = 0; j < count; j++) {
                    free(new_items[j]);
                }
                free(new_items);
                return;
            }
            count++;
        }
    }

    // free existing result contents before overwriting
    if (result->items) {
        for (size_t i = 0; i < result->len; i++) {
            free(result->items[i]);
        }
        free(result->items);
    }

    result->items    = new_items;
    result->len      = count;
    result->capacity = da->len;  // allocated da->len, used count
    result->stack    = da->stack;
}

void da_map(DynamicArray* da, FunctionStringToString func, DynamicArray* result) {
    if (!da || !func || !result) {
        return;
    }

    char** new_items = malloc(da->len * sizeof(char*));
    if (!new_items) {
        return;
    }

    for (size_t i = 0; i < da->len; i++) {
        new_items[i] = func(da->items[i],i);
        if (!new_items[i]) {
            for (size_t j = 0; j < i; j++) free(new_items[j]);
            free(new_items);
            return;
        }
    }

    // free existing result contents before overwriting
    if (result->items) {
        for (size_t i = 0; i < result->len; i++) {
            free(result->items[i]);
        }
        free(result->items);
    }

    result->items    = new_items;
    result->len      = da->len;
    result->capacity = da->len;
    result->stack    = da->stack;
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
