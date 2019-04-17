#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "cookbook.h"
#include "cook.h"
#include "analyze.h"
#include "processing.h"

#define RED "\x1B[31m"
#define WHT "\x1B[37m"

/* Maximum Number of Concurrent Processes is Initially 1 */
int max_cooks = 1;      
/* Default Cookbook File */
char *cookbook = "cookbook.ckb";
/* Name of Main Recipe */
char *main_recipe = NULL;

/*
 * The function validates the main recipe:
 *     - If main recipe is specified, it must exist in the Cookbook
 *     - If main recipe is not specified, first recipe is chosen
 * 
 * @param : cb : Current Cookbook
 * 
 * @return:
 *     0 : Valid main recipe
 *     1 : Recipe does no exist in cookbook
*/
static int
validate_main_recipe(COOKBOOK *cb) {
    if(!main_recipe) { /* If Main Recipe was omitted */
        /* Main Recipe assumed to be first recipe in cookbook */
        main_recipe = cb->recipes->name; 
    }

    int found = 0; // Main recipe found flag
    RECIPE *r = cb->recipes; // Recipe list traversal pointer
    while(r != NULL) {
        if(!strcmp(r->name, main_recipe)) {
            found = 1;
            break;
        }
        r = r->next;
    }
   
    if(!found) { /* Provided recipe is not in cookbook recipe list */
        fprintf(stderr, "%sGiven recipe does not exist\n%s%s", RED, WHT, USAGE);
        return 1;
    }

    return 0;
}

/*
 * Free the Cookbook structure
 * 
 * @param : cb : Cookbook being free'd
*/
static void
free_cookbook(COOKBOOK *cb) {

}

/*
 * The function performs the necessary tasks 
 * needed to "cook" the main recipe given.
*/
int 
cook() {
    /* ==================== COOKBOOK PARSING ==================== */
    COOKBOOK *cb;
    FILE *cb_file; // File pointer to given coobook
    int err = 0;   // Parser error flag
    if(!(cb_file = fopen(cookbook, "r"))) {
        fprintf(stderr, "%sCannot open cookbook\n%s%s", RED, WHT, USAGE);
        return 1;
    }
    cb = parse_cookbook(cb_file, &err);
    fclose(cb_file); // Finished parsing cookbook
    if(err) {
        fprintf(stderr, "%sError parsing cookbook\n%s%s", RED, WHT, USAGE);
        return 1;
    } else {
        /* ===== VALIDATE MAIN RECIPE ===== */
        if(validate_main_recipe(cb))
            return 1;

        /* ==================== RECIPE ANALYSIS ==================== */
        /* 
            Create initial list for work queue & 
            "mark" the needed recipes in the cookbook 
        */
        analyize_recipe(main_recipe, cb);

        /* ==================== PROCESS RECIPES ==================== */
        if(process_recipes())
            return 1;
    }

    /* ===== Free Cookbook & Lists ===== */
    free_cookbook(cb);
    //free_work_queue();
    //free_list(list_head); // There might be some entries left (duplicates)

    return 0;
}
