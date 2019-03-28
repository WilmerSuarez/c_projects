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
    /* Teset Flushing */
    // void *a = sf_malloc(130);
    // void *b = sf_malloc(130);
    // void *c = sf_malloc(130);
    // void *d = sf_malloc(130);
    // void *e = sf_malloc(130);
    // void *f = sf_malloc(130);
    // sf_free(a);
    // sf_free(b);
    // sf_free(c);
    // sf_free(d);
    // sf_free(e);
    // sf_free(f);

    // sf_show_heap();

    /* Test Coalescing with Block going Directly to Main Free List */
    // void *a = sf_malloc(200);
    // void *b = sf_malloc(2000);
    // void *c = sf_malloc(200);
    // sf_malloc(320);

    // sf_show_heap();

    // sf_free(a);
    // sf_show_heap();
    // sf_free(c);
    // sf_show_heap();
    // sf_free(b);

    // sf_show_heap();

    // /* Test multiple Pages */
    // sf_malloc(12928);
    // sf_show_heap();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
