#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wvariadic-macros"


#ifndef COMMON_H_
#define COMMON_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "logger.h"
#include "smemory.h"

// Source - https://dev.to/tomoyukiaota/exploring-c-equivalent-of-cs-nameof-operator-1p8c
#define nameof(name) #name

// Source - https://stackoverflow.com/a/2124385
// Posted by Kornel Kisielewicz, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-29, License - CC BY-SA 3.0

#define PP_NARG(...)  PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0


#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#elif defined(__APPLE__)
  #include <mach/mach_time.h>
#else
  #include <time.h>  // POSIX clock_gettime
#endif

// with snprintf — needs a helper macro
#define TMP_SPRINTF_SIZE 256
#define tmp_sprintf(fmt, ...) __extension__({ \
    char _buf[TMP_SPRINTF_SIZE]; \
    sprintf(_buf, fmt  ,__VA_ARGS__); \
    strdup(_buf); \
})


typedef enum {
    SV_STACK = 0, // points into a stack buffer, don't free
    SV_OWNED,     // heap allocated, we free it
    SV_BORROWED,  // slice into someone else's buffer, don't touch
} SVKind;

typedef struct {
    char* buffer;
    size_t len;
    SVKind kind;
} StringView;

typedef struct {
    char** items;
    size_t len;
    size_t capacity;
} StringBuilder;

typedef struct {
    bool   started;
    size_t ms;

#ifdef _WIN32
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
#elif defined(__APPLE__)
    uint64_t                  start;
    mach_timebase_info_data_t timebase;
#else
    struct timespec start;
#endif
} StopWatch;

void sw_start(StopWatch* sw);
void sw_stop(StopWatch* sw);

void sb_init(StringBuilder* sb);
void sb_to_sv_and_clear_sb(StringBuilder* sb, StringView* sv);
void sb_set_length(StringBuilder* sb, size_t len);
bool sb_push(StringBuilder* sb, const char* s);
void sb_free(StringBuilder* sb);

#define sb_fpush(sb,fmt,...) sb_push(sb, tmp_sprintf(fmt __VA_OPT__(,)__VA_ARGS__))
#define sb_fpushln(sb, fmt, ...) sb_push(sb, tmp_sprintf(fmt"\n" __VA_OPT__(,)__VA_ARGS__))

void sv_free(StringView* sv);

bool is_cstr_starts_with(const char *hay, const char *needle);
bool is_cstr_starts_with_i(const char *hay, const char *needle);
bool is_cstr_ends_with(const char *hay, const char *needle);
// bool is_cstr_contain(const char *hay, const char *needle);

#define SB_INIT_CAP 16

#ifdef COMMON_IMPL

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>




// -- platform now() -----------------------------------------------------------

static size_t sw_elapsed_ms(StopWatch* sw) {
#ifdef _WIN32
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (size_t)((now.QuadPart - sw->start.QuadPart) * 1000ULL
                    / sw->freq.QuadPart);

#elif defined(__APPLE__)
    uint64_t elapsed_ns = (mach_absolute_time() - sw->start)
                          * sw->timebase.numer / sw->timebase.denom;
    return (size_t)(elapsed_ns / 1000000ULL);

#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t elapsed_ns = (uint64_t)(now.tv_sec  - sw->start.tv_sec)  * 1000000000ULL
                        + (uint64_t)(now.tv_nsec - sw->start.tv_nsec);
    return (size_t)(elapsed_ns / 1000000ULL);
#endif
}

// -- public API ---------------------------------------------------------------

void sw_start(StopWatch* sw) {
    sw->ms      = 0;
    sw->started = true;

#ifdef _WIN32
    QueryPerformanceFrequency(&sw->freq);
    QueryPerformanceCounter(&sw->start);

#elif defined(__APPLE__)
    mach_timebase_info(&sw->timebase);
    sw->start = mach_absolute_time();

#else
    clock_gettime(CLOCK_MONOTONIC, &sw->start);
#endif
}

void sw_stop(StopWatch* sw) {
    if (!sw->started) return;
    sw->ms      = sw_elapsed_ms(sw);
    sw->started = false;
}

// -- init / free --------------------------------------------------------------

void sb_init(StringBuilder* sb) {
    sb->items    = MALLOC(SB_INIT_CAP * sizeof(char*));
    sb->len      = 0;
    sb->capacity = sb->items ? SB_INIT_CAP : 0;
}

void sb_free(StringBuilder* sb) {
    if (!sb || !sb->items) {
        return;
    }

    for (size_t i = 0; i < sb->len; i++) {
        if (sb->items[i]) {
            FREE(sb->items[i]);
        }
    }
    FREE(sb->items);
    sb->items    = NULL;
    sb->len      = 0;
    sb->capacity = 0;
}

// -- grow slot array ----------------------------------------------------------

static bool sb_grow(StringBuilder* sb) {
    size_t new_cap = sb->capacity * 2;
    char** grown   = REALLOC(sb->items, new_cap * sizeof(char*));
    if (!grown) return false;
    sb->items    = grown;
    sb->capacity = new_cap;
    return true;
}

// -- core append (owns the formatted string) ----------------------------------

bool sb_push(StringBuilder* sb, const char* s) {
    if (sb->len == sb->capacity && !sb_grow(sb)) { return false; }
    sb->items[sb->len++] = sdup(s);
    return true;
}

// -- public API ---------------------------------------------------------------


// -- flatten to StringView, clear sb ------------------------------------------

void sb_to_sv_and_clear_sb(StringBuilder* sb, StringView* sv) {
    const char* f_name = nameof(sb_to_sv_and_clear_sb);
    if (!sb) {
        wrn("%s: attempt to convert to string view from a string builder that is NULL",f_name);
        return;
    }
    if (!sv) {
        wrn("%s: target string view is NULL",f_name);
        return;
    }
    // measure total length
    size_t total = 0;
    for (size_t i = 0; i < sb->len; i++) total += strlen(sb->items[i]);

    char* buf = MALLOC(total + 1);
    if (!buf) { sv->buffer = NULL; sv->len = 0; return; }

    // single-pass memcpy join — no strlen called twice
    char* cur = buf;
    for (size_t i = 0; i < sb->len; i++) {
        size_t chunk = strlen(sb->items[i]);
        memcpy(cur, sb->items[i], chunk);
        cur += chunk;
    }
    *cur = '\0';

    sv->buffer = buf;
    sv->len    = total;

    sb_free(sb);
    sb_init(sb);   // ready for reuse
}

// -- truncate without realloc -------------------------------------------------

void sb_set_length(StringBuilder* sb, size_t len) {
    const char* f_name = nameof(sb_set_length);

    if (!sb) {
        wrn("%s: attempt to set length %zu on a string builder that is NULL",f_name,len);
        return;
    }
    size_t total = 0;
    for (size_t i = 0; i < sb->len; i++) {
        size_t chunk = strlen(sb->items[i]);
        if (total + chunk >= len) {
            sb->items[i][len - total] = '\0';   // truncate this chunk
            for (size_t j = i + 1; j < sb->len; j++) FREE(sb->items[j]);
            sb->len = i + 1;
            return;
        }
        total += chunk;
    }
    // len >= current total: no-op
}

bool is_cstr_starts_with(const char *hay, const char *needle) {
    if (hay == NULL || needle == NULL) return false;
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return true;           // empty needle always matches
    return strncmp(hay, needle, needle_len) == 0;
}

bool is_cstr_starts_with_i(const char *hay, const char *needle) {
    if (hay == NULL || needle == NULL) return false;
    while (*needle) {
        if (tolower((unsigned char)*hay) != tolower((unsigned char)*needle))
            return false;
        hay++;
        needle++;
    }
    return true;
}

// ends_with
bool is_cstr_ends_with(const char *hay, const char *needle) {
    if (hay == NULL || needle == NULL) return false;
    size_t hay_len    = strlen(hay);
    size_t needle_len = strlen(needle);
    if (needle_len == 0)              return true;
    if (needle_len > hay_len)         return false;
    return strcmp(hay + (hay_len - needle_len), needle) == 0;
}

// contains
bool is_cstr_contains(const char *hay, const char *needle) {
    if (hay == NULL || needle == NULL) return false;
    if (needle[0] == '\0')             return true;
    return strstr(hay, needle) != NULL;
}

void sv_realloc(StringView* sv, size_t length) {
    if (!sv) return;

    switch (sv->kind) {

        case SV_OWNED: {
            char* resized = REALLOC((char*)sv->buffer, length + 1);
            if (!resized) return;
            if (length > sv->len)
                memset(resized + sv->len, 0, length - sv->len + 1);  // zero new region
            resized[length] = '\0';
            sv->buffer = resized;
            sv->len    = length;
        }
        break;

        case SV_BORROWED:
        case SV_STACK: {
            // can't realloc a buffer we don't own — copy it first
            char* heap = MALLOC(length + 1);
            if (!heap) return;
            size_t copy_len = length < sv->len ? length : sv->len;
            memcpy(heap, sv->buffer, copy_len);
            if (length > sv->len)
                memset(heap + sv->len, 0, length - sv->len + 1);  // zero new region
            heap[length] = '\0';
            sv->buffer = heap;
            sv->len    = length;
            sv->kind   = SV_OWNED;  // promoted — we now own the new heap copy
        }
        break;
        default: 
            err("UNREACHABLE");
            break;
    }
}

void sv_free(StringView* sv) {
    if (!sv || !sv->buffer || !sv->kind) return;
    FREE(sv->buffer);
    sv->buffer = NULL;
    sv->len    = 0;
}


#endif//COMMON_IMPL

#endif//COMMON_H_
