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
                    exit(1);
                }
            break;
            case '?':
            default:
            fprintf(stderr, USAGE); // Usage of the program
            exit(1);
        }
    }
    /* Get the Main Recipe Name (if supplied) */
    if(optind != argc) 
        main_recipe = argv[optind];

    /* ==================== START COOKING! ==================== */
    if(!cook()) {
      exit(0);
    }   

    exit(1);
}
