#ifndef PROCESSING_H
#define PROCESSING_H

#include "cookbook.h"

typedef struct work_queue {
    RECIPE *recipe;          // Recipe ready to be processed
    struct work_queue *next; //
} WORK_QUEUE;

WORK_QUEUE work_head;

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
process_recipes();

#endif /* PROCESSING_H */
