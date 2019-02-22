#include <stdio.h>
#include <stdlib.h>
#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv) {
    int ret;
    
    /* Validate program arguments */
    if(validargs(argc, argv)) {
        USAGE(*argv, EXIT_FAILURE);
    }
    
    /* Perform Operation based on global_options (set by validargs()) */
    if(global_options & 1) { 
        USAGE(*argv, EXIT_SUCCESS); /* PRINT UTILITY USAGE */
    } else if(global_options & 2) {
        return compress();          /* COMPRESS STDIN DATA */
    } else if (global_options & 4) {
        return decompress();        /* DECOMPRESS STDIN DATA */
    }

    return EXIT_SUCCESS;
}
