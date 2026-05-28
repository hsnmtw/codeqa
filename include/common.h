#ifndef COMMON_H_
#define COMMON_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

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

#define sdup(x) tmp_sprintf("%s",x)
#define ANSI_ERR "\033[1;41;37m"
#define ANSI_WRN "\033[1;43;37m"
#define ANSI_INF "\033[1;42;37m"
#define ANSI_DBG "\033[1;44;37m"
#define ANSI_TRC "\033[1;44;40m"

#define F_RED   "\033[31m"
#define F_GREEN "\033[32m"
#define F_AMBER "\033[33m"
#define F_BLUE  "\033[34m"
#define F_PINK  "\033[35m"
#define F_CYAN  "\033[36m"
#define F_WHITE "\033[37m"
#define B_BLACK "\033[40m"
#define B_RED   "\033[41m"
#define B_GREEN "\033[42m"
#define B_AMBER "\033[43m"
#define B_BLUE  "\033[44m"
#define B_PINK  "\033[45m"
#define B_CYAN  "\033[46m"
#define B_WHITE "\033[47m"
#define RESET   "\033[0m"     

#define err(fmt,...) printf(ANSI_ERR" ERR: "RESET" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define wrn(fmt,...) printf(ANSI_WRN" WRN: "RESET" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define inf(fmt,...) printf(ANSI_INF" INF: "RESET" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define dbg(fmt,...) printf(ANSI_DBG" DBG: "RESET" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define trc(fmt,...) printf(ANSI_TRC" TRC: "RESET" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define println(fmt,...) printf(RESET""fmt"\n" __VA_OPT__(,)__VA_ARGS__)


#define todo(s) do { printf(B_RED""F_WHITE" TODO: "RESET" %s:%d <%s> ["F_AMBER"%s"RESET"]\n", __FILE__, __LINE__,__FUNCTION__,s); exit(1); } while (0)
#define unused(x) (void)x

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
bool sb_push(StringBuilder* sb, char* s);
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
    sb->items    = malloc(SB_INIT_CAP * sizeof(char*));
    sb->len      = 0;
    sb->capacity = sb->items ? SB_INIT_CAP : 0;
}

void sb_free(StringBuilder* sb) {
    for (size_t i = 0; i < sb->len; i++) free(sb->items[i]);
    free(sb->items);
    sb->items    = NULL;
    sb->len      = 0;
    sb->capacity = 0;
}

// -- grow slot array ----------------------------------------------------------

static bool sb_grow(StringBuilder* sb) {
    size_t new_cap = sb->capacity * 2;
    char** grown   = realloc(sb->items, new_cap * sizeof(char*));
    if (!grown) return false;
    sb->items    = grown;
    sb->capacity = new_cap;
    return true;
}

// -- core append (owns the formatted string) ----------------------------------

bool sb_push(StringBuilder* sb, char* s) {
    if (sb->len == sb->capacity && !sb_grow(sb)) { free(s); return false; }
    sb->items[sb->len++] = s;
    return true;
}

// -- public API ---------------------------------------------------------------


// -- flatten to StringView, clear sb ------------------------------------------

void sb_to_sv_and_clear_sb(StringBuilder* sb, StringView* sv) {
    if (!sb) {
        wrn("%s: attempt to convert to string view from a string builder that is NULL",__FUNCTION__);
        return;
    }
    if (!sv) {
        wrn("%s: target string view is NULL",__FUNCTION__);
        return;
    }
    // measure total length
    size_t total = 0;
    for (size_t i = 0; i < sb->len; i++) total += strlen(sb->items[i]);

    char* buf = malloc(total + 1);
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
    if (!sb) {
        wrn("%s: attempt to set length %zu on a string builder that is NULL",__FUNCTION__,len);
        return;
    }
    size_t total = 0;
    for (size_t i = 0; i < sb->len; i++) {
        size_t chunk = strlen(sb->items[i]);
        if (total + chunk >= len) {
            sb->items[i][len - total] = '\0';   // truncate this chunk
            for (size_t j = i + 1; j < sb->len; j++) free(sb->items[j]);
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
            char* resized = realloc((char*)sv->buffer, length + 1);
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
            char* heap = malloc(length + 1);
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
    free(sv->buffer);
    sv->buffer = NULL;
    sv->len    = 0;
}


#endif//COMMON_IMPL

#endif//COMMON_H_
