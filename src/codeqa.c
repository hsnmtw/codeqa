
#include <stdlib.h>

#define COMMON_IMPL
#include "common.h"
#define DA_IMPL
#include "dynamic_array.h"
#define IO_IMPL
#include "io.h"


typedef struct {
    char* file;
    char* directory;
    char* pattern;
    bool is_recursive;
} Options;


void parse_arguments(Options* options, int argc, char** argv);
void usage(void);
void print_options(Options* options);

void parse_arguments(Options* options, int argc, char** argv) {

    for (int i=1; i<argc; ++i) {
        if (is_cstr_starts_with(argv[i],"-f")) {
            i++;
            if (i >= argc) {
                ERR("you need to provide path of a file/folder after argument -f");
                usage();
                exit(1);
            }
            if (io_is_file(argv[i])) {
                options->file = argv[i];
            } else if (io_is_directory(argv[i])) {
                options->directory = argv[i];
            } 
        }
        if (is_cstr_starts_with(argv[i],"-p")) {
            
        }
        if (is_cstr_starts_with(argv[i],"-r")) {
            options->is_recursive = true;
        }
        // if (io_is_file(argv[i])) {
        //     options->file = argv[i];
        // } else if (io_is_directory(argv[i])) {
        //     options->directory = argv[i];
        // } else if (strcmp(argv[i],"-r")==0) {
        //     options->is_recursive = true;
        // } else {
        //     options->pattern = argv[i];
        // }
    }
}




void usage(void) {
    printf("   "F_AMBER"# --------------------------------------------------------\n"RESET);
    printf("   #  "B_GREEN"          CODE/QA                                      "RESET"\n");
    printf("   "F_AMBER"# --------------------------------------------------------\n\n");
    printf("   "F_CYAN"# usage: scan a file or a directory (recursively if needed)\n\n");
    printf("   "F_GREEN"$ ./codeqa (-f)ile/older (-p) *.* (-r)\n");
    printf("   "F_CYAN"            -f"RESET"   file or folder path\n");
    printf("   "F_CYAN"            -p"RESET"   pattern\n");
    printf("   "F_CYAN"            -r"RESET"   recursive\n");
    printf("   "RESET"\n");
}


void print_options(Options* options) {
    printf(" [%*s] = %s\n", 15, "file", options->file);
    printf(" [%*s] = %s\n", 15, "directory", options->directory);
    printf(" [%*s] = %s\n", 15, "pattern", options->pattern);
    printf(" [%*s] = %s\n", 15, "is_recursive", options->is_recursive ? "true" : "false");
}



bool even(const char* _, size_t i) { return i % 2 == 0; }

int main(int argc, char** argv) {
    unused(argc);
    unused(argv);
    
    DynamicArray da = DA_OF("1","1","1");
    DynamicArray unique = {0};

    da_distinct(&da, &unique);

    DBG("------------------------------- da");
    INF("count = %zu", da.len);
    DBG("------------------------------- unique");
    INF("count = %zu", unique.len);
    
    WRN("");
    DBG("");
    TRC("");

    ERR("------------------------ TESTING ERROR COLORS\nExit normally");
    return 0;
}
