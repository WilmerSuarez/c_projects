#ifndef ANALYZE_H
#define ANALYZE_H

#include "cookbook.h"

/* ==================== RECIPE STATES ==================== */
typedef enum states {
    NEEDED,
    PROCESSING,
    FAILED,
    COMPLETED
} STATES;

/* Linked list of the main_recipe dependencies */
typedef struct node {
    RECIPE *recipe;    // Recipe
    struct node *next; // Pointer to next recipe dependency
} NODE;

/* Head of the dependency list */
NODE *list_head;

/*
 * The function finds the main_recipe structure 
 * and traverses it's dependencies; marking them
 * as "needed" and created a list of all the "needed"
 * recipes that do not have their own dependencies
 * 
 * @param : main_recipe : Recipe used as the root of the tree
 * @param : cb : Cookbook being traversed to create the tree
*/
void
analyize_recipe(const char *main_recipe, const COOKBOOK *cb);

#endif /* ANALYZE_H */

