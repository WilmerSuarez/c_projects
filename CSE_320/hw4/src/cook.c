#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "cookbook.h"
#include "cook.h"
#include "analyze.h"
#include "recipe_processing.h"

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
        fprintf(stderr, "%sGiven recipe does not exist\n%s%s", KRED, KNRM, USAGE);
        return 1;
    }

    return 0;
}

/*
 * Free the Cookbook structure
*/
static void
free_cookbook(COOKBOOK *cb) {

}

/*
 * Free unused Work Queue nodes 
 * after abnormal processing loop termination
*/
static void
free_work_queue(WQ_NODE *queue) {

}

/*
 * The function performs the necessary tasks 
 * needed to "cook" the main recipe given.
*/
int 
cook() {
    /* ==================== COOKBOOK PARSING ==================== */
    COOKBOOK *cb;
    FILE *cb_file;     // File pointer to given coobook
    int err = 0;       // Parser error flag

    if(!(cb_file = fopen(cookbook, "r"))) {
        fprintf(stderr, "%sCannot open cookbook\n%s%s", KRED, KNRM, USAGE);
        return 1;
    }
    cb = parse_cookbook(cb_file, &err);
    fclose(cb_file); // Finished parsing cookbook
    if(err) {
        fprintf(stderr, "%sError parsing cookbook\n%s%s", KRED, KNRM, USAGE);
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
        if(analyize_recipe(main_recipe, cb)) {
            return exit_code;
        }

        /* ==================== PROCESS RECIPES ==================== */
        processing();
    }

    /* ===== FREE USED DATA STRUCTURES ===== */
    free_cookbook(cb); // also free the states if they are not null
    /* 
        If Main Porcessing Loop exited abnormally, 
        free the unused Work Queue entries 
    */
    if(exit_code)
        free_work_queue(work_head);

    return exit_code;
}
