#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "cookbook.h"
#include "cook.h"

/* Maximum Number of Concurrent Processes is Initially 1 */
int max_cooks = 1;      
/* Default Cookbook File */
char *cookbook = "cookbook.ckb";
/* Name of Main Recipe */
char *main_recipe = NULL;

/*
 * The function performs the necessary tasks 
 * needed to cook the main recipe given.
*/
int 
cook() {
    /* ==================== PARSE COOKBOOK ==================== */
    COOKBOOK *cb;
    FILE *cb_file; // File pointer to given coobook
    int err = 0;   // Parser error flag
    if(!(cb_file = fopen(cookbook, "r"))) {
        fprintf(stderr, "Cannot open cookbook\n");
        return 1;
    }
    cb = parse_cookbook(cb_file, &err);
    if(err) {
        fprintf(stderr, "Error parsing cookbook\n");
        return 1;
    } else {
        /* ===== Validate Main Recipe ===== */
        if(!main_recipe) { /* If Main Recipe was omitted */
            /* Main Recipe assumed to be first recipe in cookbook */
            main_recipe = cb->recipes->name; 
        }

        int found = 0; // Main recipe found flag
        struct recipe *r = cb->recipes; // Recipe list traversal pointer
        while(r != NULL) {
            if(!strcmp(r->name, main_recipe)) {
                found = 1;
                break;
            }
            r = r->next;
        }
        
        if(!found) { /* Provided recipe is not in cookbook recipe list */
            fprintf(stderr, "Given recipe does not exist\n");
            return 1;
        }
        
        /* ==================== ANALYZE COOKBOOK ==================== */
        
    }

    /* ===== Free Cookbook ===== */

    return 0;
}
