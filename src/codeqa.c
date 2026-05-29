
#include <stdlib.h>

#define COMMON_IMPL
#include "common.h"
#define COLLECTIONS_IMPL
#include "collections.h"
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
                err("you need to provide path of a file/folder after argument -f");
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
    println("   "F_AMBER"# --------------------------------------------------------\n"RESET);
    println("   #  "B_GREEN"          CODE/QA                                      "RESET);
    println("   "F_AMBER"# --------------------------------------------------------\n");
    println("   "F_CYAN"# usage: scan a file or a directory (recursively if needed)\n");
    println("   "F_GREEN"$ ./codeqa (-f)ile/older (-p) *.* (-r)");
    println("   "F_CYAN"            -f"RESET"   file or folder path");
    println("   "F_CYAN"            -p"RESET"   pattern");
    println("   "F_CYAN"            -r"RESET"   recursive");
    println("   "RESET);
}


void print_options(Options* options) {
    println(" [%*s] = %s", 15, "file", options->file);
    println(" [%*s] = %s", 15, "directory", options->directory);
    println(" [%*s] = %s", 15, "pattern", options->pattern);
    println(" [%*s] = %s", 15, "is_recursive", options->is_recursive ? "true" : "false");
}



// bool even(const char* _, size_t i) { return i % 2 == 0; }

#include "../tests/test_collections.c"
#include "../tests/test_sb_sv.c"

int main(int argc, char** argv) {
    unused(argc);
    unused(argv);
    
    DynamicArray da = DA_OF("1","1","1");
    DynamicArray unique = {0};

    da_distinct(&da, &unique);

    dbg("------------------------------- da");
    inf("count = %zu", da.len);
    dbg("------------------------------- unique");
    inf("count = %zu", unique.len);
    
    wrn("");
    dbg("");
    trc("");

    test_map();
    test_sb_sv();
    printf("\n\n  [ UNIT TESTS ] %d passed, %d failed\n\n", passed, failed);


    err("------------------------ TESTING ERROR COLORS\nExit normally");
    return 0;
}
