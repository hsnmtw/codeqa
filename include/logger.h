#ifndef LOGGER_H_
#define LOGGER_H_

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


#define todo(s) do { printf(B_RED""F_WHITE" TODO: "RESET" %s:%d <%s> ["F_AMBER"%s"RESET"]\n", __FILE__, __LINE__,__func__,s); exit(1); } while (0)
#define unused(x) (void)x

#endif//LOGGER_H_
