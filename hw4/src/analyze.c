#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "analyze.h"
#include "cook.h"

/*
 * Create a "leaf" list node with the given recipe 
 * 
 * @param : recipe : the recipe to be integrated in the node
 * 
 * @returns : NODE * : A pointer to the newly created node
*/
static NODE *
create_node(RECIPE *recipe) {
    NODE *node = (NODE *)malloc(sizeof(NODE));
    node->recipe = recipe;
    node->next = NULL;
    return node;
}

/*
 * Add next "leaf" recipe to the intitial list
 * to be used in the work queue
 * 
 * @param : recipe : Recipe to add to the "leaf" list
*/
static void
add_to_list(RECIPE *recipe) {
    NODE *node = create_node(recipe);
    NODE *end = list_head;

    /* If list is empty */
    if(!list_head) {
        list_head = node;
    } else {
        /* Go to end of list */
        while(end->next != NULL)
            end = end->next;
        
        /* Add new node to end of list */
        end->next = node;
    }
}

/*
 * Removes the entry at the beginning of the "leaf" list
 * 
 * @returns : RECIE * : Recipe from the removed entry
*/
RECIPE *
remove_head() {
    /* If list is empty */
    if(!list_head) {
        // Do nothing
    } else {
        NODE *prev = list_head;
        RECIPE *recipe = prev->recipe;
        
        /* Next entry in list is new head */
        list_head = list_head->next;

        free(prev); // Remove the old head

        return recipe;
    }

    return NULL;
}

/*
 * Mark the required recipe dependencies needed by the 
 * main recipe.
 * Create a list of all the "needed"
 * recipes that do not have their own dependencies
 * 
 * @param : current_recipe : Next recipe needed
 * by the main recipe
*/
static void
create_list(RECIPE *current_recipe) {
    /* Reached end of dependencies */
    if(!current_recipe)
        return;

    /* Mark recipe as "required" by the main recipe */
    unsigned *state = (unsigned *)malloc(sizeof(unsigned));
    *state = REQUIRED; /* 0 = NEEDED state */
    current_recipe->state = state;

    /* 
        Add the dependencies of the current recipe to
        its dependency list
    */
    RECIPE_LINK *r = current_recipe->this_depends_on;

    /* Add "leaves" (recipes with no dependencies) to the list */
    if(!r) 
        add_to_list(current_recipe);
    
    /* Recursively traverse the dependencies */
    while(r != NULL) { 
        create_list(r->recipe);
        r = r->next;
    }
}

/*
 * Checks to see if main_recipe is associated with
 * any infinite cycles in the cookbook with other
 * recipes
 * 
 * @param : recipe : main recipe
 * 
 * @return : 
 *     0           : No cycle
 *     CYCLE_ERROR : When an infinite cycle is found
*/
static int
cycle_check(RECIPE *recipe) {
    return 0;
}

/*
 * The function finds the main_recipe structure 
 * and traverses it's dependencies; marking them
 * as "needed" and created a list of all the "needed"
 * recipes that do not have their own dependencies
*/
int
analyize_recipe(const char *main_recipe, const COOKBOOK *cb) {
    /* ==================== FIND ROOT ==================== */
    RECIPE *r = cb->recipes; // Recipe list traversal pointer
    while(r != NULL) {
        if(!strcmp(r->name, main_recipe)) {
            /* ==================== ANALYZE & CREATE "LEAF" LIST ==================== */
            /* Check for infinite cycles */
            if(cycle_check(r)) {
                exit_code = CYCLE_ERROR;
                fprintf(stderr, "%sInfinite cycle found in cookbook\n%s%s", KRED, KNRM, USAGE);
                break;
            }
            create_list(r);
            break;
        }
        r = r->next;
    } 

    return exit_code;
}
