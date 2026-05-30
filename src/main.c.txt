#include "logger.h"
#include "heap.h"

#include "../tests/test_heap.c"

int main(void) {
    // inf("hello heap!");
    // char* p1 = MALLOC(1);
    // char* p2 = MALLOC(2);
    // char* p3 = MALLOC(3);
    // char* p4 = MALLOC(4);
    // char* p5 = MALLOC(5);
    // // trc("p1=%p",p1);
    // // trc("p2=%p",p2);
    // // trc("p3=%p",p3);
    // // trc("p4=%p",p4);
    // // trc("p5=%p",p5);
    // PRINT_MEMORY();puts("\n");

    // p2[0] = 'H';
    // p2[1] = 'i';
    // sprintf(p5 , "%s", "54321");
    // sprintf(p4, "%s", "Ali"); // assignment changes the pointer (memory leak)
    // //PRINT_MEMORY();
    // inf("testing free ... ");
    // PRINT_MEMORY();puts("\n");
    // puts("free: p3");FREE(p3); PRINT_MEMORY();puts("\n");
    // puts("free: p1");FREE(p1); PRINT_MEMORY();puts("\n");
    // puts("free: p4");FREE(p4); PRINT_MEMORY();puts("\n");
    // // FREE(p3);
    // FREE(p2);
    // PRINT_MEMORY();
    
    // FREE(p1);
    // FREE(p5);
    // PRINT_MEMORY();

    test_heap();

    inf("done !");
    return 0;
}