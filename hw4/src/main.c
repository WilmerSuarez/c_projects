#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "debug.h"
#include "cook.h"
#include "cookbook.h"

int main(int argc, char *argv[]) {
    /* ==================== VALIDATE ARGUMENTS ==================== */
    int opt; // Option character
    while((opt = getopt(argc, argv, "f:c:")) != -1) {
        switch(opt) {
            case 'f': /* Cookbook */
                cookbook = optarg;
            break;
            case 'c': /* Maximum Amount of Cooks */
                max_cooks = strtol(optarg, NULL, 10);
                if(max_cooks <= 0) { // Max cooks cannot be 0 or negative
                    fprintf(stderr, "Invalid Cook count: %d\n", max_cooks);
                    exit(ARG_ERROR);
                }
            break;
            case '?':
            default:
            fprintf(stderr, USAGE); // Usage of the program
            exit(ARG_ERROR);
        }
    }
    /* Get the Main Recipe Name (if supplied) */
    if(optind != argc) 
        main_recipe = argv[optind];

    /* ==================== START COOKING! ==================== */
    if(!(exit_code = cook())) {
      exit(EXIT_SUCCESS);
    }   

    /* Handle Error code */
    switch(exit_code) {
        case FAILED_RECIPE:
            fprintf(stderr, "%sRecipe failed to cook\n", KRED);
            break;
        case INVALID_INPUT_FILE:
            fprintf(stderr, "%sInvalid input redirection file\n", KRED);
            break;
        case INVALID_OUTPUT_FILE:
            fprintf(stderr, "%sInvalid output redirection file\n", KRED);
            break;
        case PIPE_ERROR:
            fprintf(stderr, "%sPipe'd process exited abnormally\n", KRED);
            break;
        case COMMAND_ERROR:
            fprintf(stderr, "%sInvalid Command\n", KRED);
            break;
        default:
            break;
    }

    exit(exit_code);
}
