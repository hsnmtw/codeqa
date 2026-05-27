#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>


// with snprintf — needs a helper macro
//snprintf(_buf, TMP_SPRINTF_SIZE, fmt, ##__VA_ARGS__); 
#define TMP_SPRINTF_SIZE 256
#define tmp_sprintf(fmt, ...) __extension__({ \
    char _buf[TMP_SPRINTF_SIZE]; \
    sprintf(_buf, fmt, ##__VA_ARGS__); \
    strdup(_buf); \
})

#define sdup(x) tmp_sprintf("%s",x)

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

#define todo(s) do { printf(B_RED""F_WHITE" TODO: "RESET" %s:%d <%s> ["F_AMBER"%s"RESET"]\n", __FILE__, __LINE__,__FUNCTION__,s); exit(1); } while (0)
#define unused(x) ((void)x) 

typedef struct {
    char* buffer;
    size_t len;
} StringView;

void log_error(const char* message);
void log_warning(const char* message);
void log_info(const char* message);

bool is_cstr_starts_with(const char *hay, const char *needle);
bool is_cstr_starts_with_i(const char *hay, const char *needle);
bool is_cstr_ends_with(const char *hay, const char *needle);
// bool is_cstr_contain(const char *hay, const char *needle);


#ifdef COMMON_IMPL



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

void log_error(const char* message) {
    printf(B_RED""F_WHITE" ERR: "RESET" %s\n", message);
}
void log_warning(const char* message) {
    printf(B_AMBER""F_RED" WRN: "RESET" %s\n", message);
}
void log_info(const char* message) {
    printf(B_GREEN""F_WHITE" INF: "RESET" %s\n", message);
}

#endif//COMMON_IMPL

#endif//COMMON_H_
