#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#pragma GCC diagnostic ignored "-Wformat-overflow"
#pragma CC diagnostic ignored "-Wformat-overflow"

#ifndef LOGGER_H_
#define LOGGER_H_


#define ANSI_ERR "\033[1;41;37m"
#define ANSI_WRN "\033[1;43;37m"
#define ANSI_INF "\033[1;42;37m"
#define ANSI_DBG "\033[1;44;37m"
#define ANSI_TRC "\033[1;44;40m"

#define F_RED    "\033[31m"
#define F_GREEN  "\033[32m"
#define F_AMBER  "\033[33m"
#define F_BLUE   "\033[34m"
#define F_PINK   "\033[35m"
#define F_CYAN   "\033[36m"
#define F_WHITE  "\033[37m"
#define B_BLACK  "\033[40m"
#define B_RED    "\033[41m"
#define B_GREEN  "\033[42m"
#define B_AMBER  "\033[43m"
#define B_BLUE   "\033[44m"
#define B_PINK   "\033[45m"
#define B_CYAN   "\033[46m"
#define B_WHITE  "\033[47m"
#define ANSI_RST "\033[0m"     


#define err(fmt,...) printf(ANSI_ERR" ERR: "ANSI_RST" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define wrn(fmt,...) printf(ANSI_WRN" WRN: "ANSI_RST" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define inf(fmt,...) printf(ANSI_INF" INF: "ANSI_RST" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define dbg(fmt,...) printf(ANSI_DBG" DBG: "ANSI_RST" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define trc(fmt,...) printf(ANSI_TRC" TRC: "ANSI_RST" "fmt"\n" __VA_OPT__(,)__VA_ARGS__)
#define println(fmt,...) printf(ANSI_RST""fmt"\n" __VA_OPT__(,)__VA_ARGS__)


#define todo(s) do { printf(B_RED""F_WHITE" TODO: "ANSI_RST" %s:%d <%s> ["F_AMBER"%s"ANSI_RST"]\n", __FILE__, __LINE__,__func__,s); exit(1); } while (0)
#define unused(x) (void)x

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static inline void trim(char* s, size_t l) {
    if (!s || l == 0) return;

    // find first non-whitespace
    size_t start = 0;
    while (start < l && (s[start] == '\0' || isspace((unsigned char)s[start])))
        start++;

    if (start == l) { s[0] = '\0'; return; }  // all whitespace

    // find last non-whitespace
    size_t end = l - 1;
    while (end > start && (s[end] == '\0' || isspace((unsigned char)s[end])))
        end--;

    // end is now the index of the last non-whitespace char
    size_t new_len = end - start;
    memmove(s, s + start, new_len);
    s[new_len] = '\0';
}

#endif//LOGGER_H_
