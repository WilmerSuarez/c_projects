#ifndef RECIPE_PID_MAP_H
#define RECIPE_PID_MAP_H

#include "cookbook.h"

typedef struct pair {
    RECIPE *recipe;
    __pid_t key;
    struct pair *next;
} PAIR;

/* Head pointer of Recipe-PID map */
PAIR *map_head;

/* 
 * Maps the given recipe to the given pid.
 * 
 * @param : recipe : Recipe being processed 
 * @param : pid : Pid of process working on the 
 *                given recipe's task list
 * 
 * @returns : Any one of the Error Codes
*/
void
add_to_map(RECIPE *recipe, __pid_t pid);

/*
 * Gets the recipe associated with the given
 * pid.
 * 
 * @param : pid : Map key
 * @return : RECIPE * : Recipe that finished processing
*/
RECIPE *
get_recipe(__pid_t pid);

#endif /* RECIPE_PID_MAP_H */