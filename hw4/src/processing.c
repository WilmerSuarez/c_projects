#include <stdlib.h>
#include "debug.h"
#include "analyze.h"
#include "processing.h"

// Free "leaf" list entries as you add them to the work queue
// Free work queue entries as you remove them for processing
// remove from leaf list 1 by one. Any time a recipe is processed, 
// go see what depends on it and add it (all the recipes that depend on it) 
// to the work queue
//
// if more "cooks" still available, continue removing from the "leaf" list
// and keep doing the same as above ^

/*
 * Processes each recipe needed to complete the 
 * main_recipe. Beginning with all the "leaf"
 * recipes.
*/
int 
process_recipes() {


    return 0;
}

