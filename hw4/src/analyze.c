#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "analyze.h"

/*
 * Create a node with the given recipe 
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
 * @param : head   : Reference to the head of the list
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
    *state = NEEDED; /* 0 = NEEDED state */
    current_recipe->state = state;

    /* 
        Add the dependencies of the current recipe to
        its dependency list
    */
    RECIPE_LINK *r = current_recipe->this_depends_on;
    if(!r) 
        /* Add "leaves" to the list */
        add_to_list(current_recipe);
    while(r != NULL) { 
        /* Recursively traverse the dependencies */
        create_list(r->recipe);
        r = r->next;
    }
}

void
print(NODE *list) {
    while(list != NULL) {
        debug("%s : state : %d", list->recipe->name, *((unsigned *)list->recipe->state));
        list = list->next;
    }
}

/*
 * The function finds the main_recipe structure 
 * and traverses it's dependencies; marking them
 * as "needed" and created a list of all the "needed"
 * recipes that do not have their own dependencies
*/
void
analyize_recipe(const char *main_recipe, const COOKBOOK *cb) {
    /* ==================== FIND ROOT ==================== */
    RECIPE *r = cb->recipes; // Recipe list traversal pointer
    while(r != NULL) {
        if(!strcmp(r->name, main_recipe)) {
            /* ==================== "ANALYZE" & CREATE LIST ==================== */
            create_list(r);
            break;
        }
        r = r->next;
    } 

    print(list_head);
}
