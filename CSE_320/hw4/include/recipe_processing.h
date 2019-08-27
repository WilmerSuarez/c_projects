#ifndef RECIPE_PROCESSING_H
#define RECIPE_PROCESSING_H

#include <signal.h>
#include "cookbook.h"

typedef struct wq_node {
    RECIPE *recipe;       // Recipe ready to be processed
    struct wq_node *next; // Pointer to next recipe to be processed
} WQ_NODE;

WQ_NODE *work_head;

/*
 * Processes each recipe needed to complete the 
 * main_recipe. Beginning with all the "leaf"
 * recipes.
 * 
 * @returns : 
 *     0 : Success : All recipes sucessfully processed
 *     1 : Failure : If any recipe fails to process
*/
int
processing();

/*
 * Adds a recipe to the Work Queue
 * 
 * @param : recipe : Recipe ready to be processed
*/
void
add_to_work_queue(RECIPE *recipe);

#endif /* RECIPE_PROCESSING_H */
