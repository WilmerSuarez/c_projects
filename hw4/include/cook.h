#ifndef COOK_H
#define COOK_H

#define USAGE "Usage: cook [-f cookbook] [-c max_cooks] [main_recipe_name]\n"

extern int max_cooks;     /* Maximum number of concurrent "cooks" (processes) */
extern char *cookbook;
extern char *main_recipe; /* Name of Main Recipe */

/*
 * The function performs the necessary tasks 
 * needed to cook the main recipe.
 * 
 * @returns : 
 *     0 : Success 
 *     1 : Failure
*/
int 
cook();

#endif /* COOK_H */

