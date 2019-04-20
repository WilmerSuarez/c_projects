#ifndef ANALYZE_H
#define ANALYZE_H

#include "cookbook.h"

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
 * 
 * @return : 
 *     0           : No cycle
 *     CYCLE_ERROR : When an infinite cycle is found
*/
int
analyize_recipe(const char *main_recipe, const COOKBOOK *cb);

/*
 * Removes the entry at the beginning of the "leaf" list
 * 
 * @returns : RECIPE * : Pointer to the recipe in the removed,
 *                       list entry, or NULL if list if empty
*/
RECIPE *
remove_head();

#endif /* ANALYZE_H */

