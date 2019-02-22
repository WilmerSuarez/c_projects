#include <stdio.h>
#include <stdlib.h>
#include "const.h"
#include "huff.h"
#include "debug.h"

/* global_options shift amounts */
#define G_OP_H  0    
#define G_OP_C  1  
#define G_OP_D  2   
#define G_OP_BS 16

/* Max size of flag String length */
#define MAX_FLAG_LEN 2

#define DEFAULT_BLOCK_SIZE  0xFFFF 

/* Min and Max number of command line arguments */
#define MIN_ARGS    2
#define MAX_ARGS    4

/*
 * @brief Calculate length of str
 * @details Cycle through String with a temporary pointer and
 * then subtract the address of the temp pointer from the one 
 * passed in to get the number of characters in the String.
 *
 * @param *str Pointer to String
 * @return Length of String
*/
static int 
strlength(const char *str) {
    /* Point to start of String */
    const char *s = str;
    /* Cycle through String */
    for(; *s; ++s);
    /* Difference in String addresses = String length */
    return (s-str);
}

/*
 * @brief Convert from String to Integer
 * @details The function converts the Block Size String to an int.
 * 
 * @param argv The argument strings passed to the program from the CLI.
 * @return Block Size
*/ 
static int 
get_b_size(char **argv) {
    int bsize = 0, len;
    
    /* Get block size string length */
    len = strlength(*(argv+3));

    /* Convert String to Integer */
    for(int i = 0; i < len; ++i) {
        bsize = bsize * 10 + (*(*(argv+3)+i) - '0');
    }

    return bsize;
}

/*
 * @brief Evaluate Block size argument
 * @details Verifies the "-b" flag is present and that the Block Size argument is
 * a valid String of digits from 0-9. Converts the Block Size argument
 * to integer and verifies that number to be within range (1024 - 65536).
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return EXIT_SUCCESS if validation succeeds and EXIT_FAILURE if validation fails.
 * @modifies global_options variable
*/
static int 
checkb(int argc, char **argv) {
    int bsize;

    /* Verify String is "-b" */
    if(strlength(*(argv+2)) == MAX_FLAG_LEN 
       && *(*(argv+2)) == '-' && *(*(argv+2)+1) == 'b') {

        /* Test for digit (0-9) */
        const char *s = *(argv+3);
        for(; *s; ++s) {
            if(*s >= '0' && *s <= '9') {
                continue;    
            } else {
                return 1;
            }
        }
    
        /* Convert from String to Integer */
        bsize = get_b_size(argv);
        
        /* Test Block Size Boundaries */
        if(bsize >= MIN_BLOCK_SIZE && bsize <= MAX_BLOCK_SIZE) {
            /* Set block size in global_options */
            global_options |= ((bsize - 1) << G_OP_BS);
            return 0;
        }
    }

    return 1;
}

/*
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning EXIT_SUCCESS if validation succeeds and EXIT_FAILURE 
 * if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return EXIT_SUCCESS if validation succeeds and EXIT_FAILURE if validation fails.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int 
validargs(int argc, char **argv) {

    /* Test for too few or too many arguments */
    if(argc >= MIN_ARGS && argc <= MAX_ARGS) {
        /* Verify flag String length and verify hyphen is present */
        if(strlength(*(argv+1)) == MAX_FLAG_LEN && *(*(argv+1)) == '-') { 
            switch(*(*(argv+1)+1)) { /* Check char after hyphen */
                case 'h': // --------------------------- HELP --------------------------- //
                    /* Set global_options to display usage in main.c */
                    global_options |= (1 << G_OP_H); 
                    return 0;
                case 'c': // --------------------------- COMPRESS --------------------------- //
                    /* Set global_options to compress */
                    global_options |= (1 << G_OP_C); 
                    if(argc == MIN_ARGS) { // If optional flag "-b" not provided
                        /* Set default block size */
                        global_options |= (DEFAULT_BLOCK_SIZE << G_OP_BS);
                        return 0;
                    } else if(argc == MAX_ARGS) {
                        /* Test for "-b" and validate block size */
                        return checkb(argc, argv); 
                    }   
                    break;
                case 'd': // --------------------------- DECOMPRESS --------------------------- //
                    if(argc > MIN_ARGS) break; // No arguments after "-d" flag allowed
                    /* Set default block size */
                    global_options |= (DEFAULT_BLOCK_SIZE << G_OP_BS);
                    /* Set global_options to decompress */
                    global_options |= (1 << G_OP_D);
                    return 0;
            }
        }
    }

    return 1;
}
