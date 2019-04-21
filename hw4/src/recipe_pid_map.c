#include <stdlib.h>
#include "debug.h"
#include "cookbook.h"
#include "recipe_pid_map.h"

/*
 * Create a pair node; linking the Recipe with respective PID
 * 
 * @param : recipe : The recipe to be linked with the PID
 * @param : pid : PID used as the map key
 * 
 * @returns : PAIR * : A pointer to the newly created pair
*/
static PAIR *
create_pair(RECIPE *recipe, __pid_t pid) {
    PAIR *pair = (PAIR *)malloc(sizeof(PAIR));
    pair->key = pid;
    pair->recipe = recipe;
    pair->next = NULL;
    return pair;
}

/*
 * Maps the given recipe to the given pid.
*/
void
add_to_map(RECIPE *recipe, __pid_t pid) {
    PAIR *pair = create_pair(recipe, pid);
    PAIR *end = map_head;

    /* If map is empty */
    if(!map_head) {
        map_head = pair;
    } else {
        /* Go to end of map */
        while(end->next != NULL)
            end = end->next;
        
        /* Add new node to end of map */
        end->next = pair;
    }
}

/*
 * Gets the recipe associated with the given
 * pid.
*/
RECIPE *
get_recipe(__pid_t pid) {
    RECIPE *recipe = NULL;
    PAIR *pair = map_head; // Pointer to beginning of map
    PAIR *prev = NULL;     // Pointer to previous pair in map

    /* If map is empty */
    if(!pair)
        return NULL;

    /* Traverse map until the associated PID is found */
    while(pair->key != pid && pair->next != NULL) {
        prev = pair;
        pair = pair->next;
    }

    /* Pair was found - remove and retrieve Recipe */
    if(prev) {
        /* Pair was after beginning or it was the last pair */
        prev->next = pair->next;
    } else {
        /* Pair was the beginning of map */
        map_head = pair->next;
    }
    /* Get recipe before deleting pair */
    recipe = pair->recipe;
    free(pair);

    return recipe;
}