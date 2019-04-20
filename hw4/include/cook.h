#ifndef COOK_H
#define COOK_H

/* Cook program usage */
#define USAGE "Usage: cook [-f cookbook] [-c max_cooks] [main_recipe_name]\n"

/* ==================== EXIT CODES ==================== */
#define ARG_ERROR     1 // Invalid Argument Error
#define FAILED_RECIPE 2 // Recipe Failed to Cook Error
unsigned exit_code;

/* ==================== RECIPE STATES ==================== */
#define COMPLETED  1 // Completed Recipe
#define FAILED     2 // Failed Recipe
#define REQUIRED   3 // Required Recipe
#define PROCESSING 4 // Processing Recipe

/* ==================== PROGRAM ARGUMENTS ==================== */
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

