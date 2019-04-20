#ifndef COOK_H
#define COOK_H

/* Cook program usage */
#define USAGE "Usage: cook [-f cookbook] [-c max_cooks] [main_recipe_name]\n"

/* ==================== EXIT CODES ==================== */
#define ARG_ERROR           2 // Invalid Argument Error
#define CYCLE_ERROR         3 // Infinite cycle in cookbook
#define FAILED_RECIPE       4 // Recipe Failed to Cook Error
#define INVALID_INPUT_FILE  5 // Input redirection file could not be open
#define INVALID_OUTPUT_FILE 6 // Ouput redirection file could not be open
#define PIPE_ERROR          7 // One of the pipe'd processes exited abnomally
#define COMMAND_ERROR       8 // Invalid command

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

