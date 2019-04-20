#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "debug.h"
#include "cook.h"
#include "analyze.h"
#include "recipe_processing.h"
#include "signal_handling.h"
#include "recipe_pid_map.h"
#include "piping.h"

// Free "leaf" list entries as you add them to the work queue
// Free work queue entries as you remove them for processing
// Any time a recipe is processed,go see what depends on it and
// add it (all the recipes that depend on it) to the work queue
//
// if more "cooks" still available, continue removing from the "leaf" list
// and keep doing the same as above ^

/*
 * Create a Work Queue node with the given recipe 
 * 
 * @param : recipe : the recipe to be integrated in the node
 * 
 * @returns : WQ_NODE * : A pointer to the newly created node
*/
static WQ_NODE *
create_wq_node(RECIPE *recipe) {
    WQ_NODE *node = (WQ_NODE *)malloc(sizeof(WQ_NODE));
    node->recipe = recipe;
    node->next = NULL;
    return node;
}

/*
 * Adds a recipe to the Work Queue
*/
void
add_to_work_queue(RECIPE *recipe) {
    WQ_NODE *node = create_wq_node(recipe);
    WQ_NODE *end = work_head;

    /* If list is empty */
    if(!work_head) {
        work_head = node;
    } else {
        /* Go to end of list */
        while(end->next != NULL)
            end = end->next;
        
        /* Add new node to end of list */
        end->next = node;
    }
}

/*
 * Removes the entry at the beginning of the Work Queue
 * 
 * @returns : RECIE * : Recipe from the removed entry
*/
static RECIPE *
remove_from_work_queue() {
    /* If Work Queue is empty */
    if(!work_head) {
        // Do nothing
    } else {
        WQ_NODE *prev = work_head;
        RECIPE *recipe = prev->recipe;
        
        /* Next entry in list is new head */
        work_head = work_head->next;

        free(prev); // Remove the old entry

        return recipe;
    }

    return NULL;
}

/*
 * Process the given recipe (perfrom the tasks needed to complete it)
*/
static void
process_recipes(RECIPE *recipe) {
    debug("Num cooks: %d", current_num_cooks);

    debug("(In new proc) Recipe added: %s : PID : %d", recipe->name, getpid());

    exit(EXIT_SUCCESS);
}

/*
 * Processes each recipe needed to complete the 
 * main_recipe. Beginning with all the "leaf"
 * recipes.
*/
int 
processing() {
    __pid_t pid;

    /* ==================== INITIALZE SIGNAL SETS FOR BLOCKING ==================== */
    sigset_t mask_child, prev; // Signal sets used for signal blocking 
    /* Initialize mask_child signal set to include the SIGCHLD signal */
    sigemptyset_wrapper(&mask_child); 
    sigaddset_wrapper(&mask_child, SIGCHLD);
    sigaction_wrapper(SIGCHLD, sigchld_handler); // Install a handler for SIGCHLD Signals

    /* ==================== INITIALIZE WORK QUEUE ==================== */
    /* Work Queue initially has all the leaf list entries */
    while(list_head) {
        add_to_work_queue(remove_head(list_head));
    }

    /* 
        Temporarily block SIGCHLD Signals while 
        working on Work Queue, to prevent errors from
        the SIGCHLD handler
    */
    sigprocmask_wrapper(SIG_BLOCK, &mask_child, &prev);

    /* ==================== MAIN PROCESSING LOOP ==================== */
    /* 
        Continue processing recipes in the Work Queue
        until the work queue is empty 
    */
    while(work_head) {
        /* Remove next recipe from the Work Queue */
        RECIPE *recipe = remove_from_work_queue();
        /* Make sure the same sub-recipe isn't processed multiple times */
        while(*((unsigned *)recipe->state) == COMPLETED) {
            recipe = remove_from_work_queue();
        }
        
        current_num_cooks++; // Increment number of child processes
        /* Process next recipe in Work Queue */
        switch((pid = fork())) {
            case -1:
                fprintf(stderr, "%sfork() error\n", KRED);
                exit(EXIT_FAILURE);
            case 0:
                sigaction_wrapper(SIGCHLD, SIG_DFL); // Unistall signal handler for children processes
                sigprocmask_wrapper(SIG_SETMASK, &prev, NULL); // Unblock SIGCHLD
                process_recipes(recipe); // Complete tasks of current recipe
            default:
                break;
        }
        /* Map recipe to the most recent child process' PID */
        add_to_map(recipe, pid);
        
        /* 
            If an error ocurred,
            do not proceed. 
        */
        if(exit_code)
            break;       

        /* If max number of cooks reached */
        if(current_num_cooks == max_cooks || !work_head) {
            /* Wait for completion of one or more recipes (wait for a SIGCHLD Signal) */
            sigsuspend(&prev);
            if(errno != EINTR) {
                fprintf(stderr, "%ssigsuspend() error\n", KRED);
            }
        }
    }

    return exit_code;
}

