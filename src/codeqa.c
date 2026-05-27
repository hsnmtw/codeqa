
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
                log_error("you need to provide path of a file/folder after argument -f");
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
    
    DynamicArray da = DA_OF("five","2","5","4","3","four","1","two","three");

    for (size_t i=0;i<da.len;++i) {
        log_info(tmp_sprintf("da[%zu] = '%s'", i, da.items[i]));
    }

    log_warning(tmp_sprintf("da.len = %zu", da.len));
    log_warning(tmp_sprintf("da.capacity = %zu", da.capacity));

    printf("---------------------------------------------------\n");

    DynamicArray filtered = {0};

    da_filter(&da, even, &filtered);
    
    for (size_t i=0;i<filtered.len;++i) {
        log_info(tmp_sprintf("da[%zu] = '%s'", i, filtered.items[i]));
    }

    log_warning(tmp_sprintf("da.len = %zu", filtered.len));
    log_warning(tmp_sprintf("da.capacity = %zu", filtered.capacity));

    log_info("------------------------\nExit normally");
    return 0;
}
