#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();
    
    /* SF_MALLOC */
    // /* Test Allocating minimum Block size */
    // double* ptr = sf_malloc(sizeof(double));
    // sf_show_heap();

    // *ptr = 2423.324;

    // printf("%f\n", *ptr);

    // /* Test Allocating maximum Block size */
    // char *ptr = sf_malloc(4016);

    // ptr = "Hello, my name is Wilmer\n";

    // printf("%s\n", ptr);

    /* Test Allocating multiple pages */
    // double *ptr = sf_malloc(8116);

    // *ptr = 320320320e-320;

    // sf_show_heap();

    /* SF_FREE */
    void *ptr = sf_malloc(12);
    sf_free(ptr);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
