#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>
#include "debug.h"
#include "cook.h"
#include "analyze.h"
#include "recipe_processing.h"
#include "signal_handling.h"
#include "recipe_pid_map.h"

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
 * Executes the command from the given step
 * of the recipe
 * 
 * @param : step : Step to perform
*/
static void
execute_step(STEP *stepp) {
    int size = strlen("util/");
    
    /* Get size of all words + "util/" */
    for (char **words = stepp->words; *words; ++words) {
        size += strlen(*words);
    }
    /* Initialize Path */
    char path[size];
    memset(path, 0, size);
    strncpy(path, "util/", size);
    /* Attempt to execute from /util directory first */
    execvp(strcat(path, *stepp->words), stepp->words);
    
    /* Not in util directory */
    execvp(*stepp->words, stepp->words);    

    exit(COMMAND_ERROR);    
}   

/*
 * Waits for all the child processes in the pipe to complete
 * 
 * @param : c_count : Number of children to wait for
*/
static int
wait_for_pipe(int c_count) {
    int status; // Used to check if child processes exited normally

    while(c_count && wait(&status) > 0) {
        --c_count;
        if(WIFEXITED(status)) {  // Check if child process exited normally 
            /* Examine reaped process exit status */
            if(!WEXITSTATUS(status)) {
                // Pipe'd process exited normally
            } else {
                return WEXITSTATUS(status);
            }
            
        } else {
            return PIPE_ERROR;
        }
    }

    return EXIT_SUCCESS;
}

/*
 * Process the given recipe (perfrom the tasks needed to complete it)
*/
static void
process_recipe(RECIPE *recipe) {
    TASK *taskp = recipe->tasks; // Pointer to traverse task lists
    STEP *stepp = NULL;          // Pointer to traverse each step
    unsigned c_counter = 0;      // Keeps track of the number of reaped children in the pipeline
    int p_fd[2];                 // Pipe file descriptors
    int in_fd;                   // Input file descriptor 
    int out_fd = 0;              // Output file descriptor - Initially standard out

    /* Process each task of the current recipe */
    while(taskp) {
        stepp = taskp->steps;   
        /* Is there an input redirection? - Test if file exists */
        if(taskp->input_file) {
            if((in_fd = open(taskp->input_file, O_RDONLY)) == -1) {
                exit(INVALID_INPUT_FILE);
            } else {
                in_fd = STDIN_FILENO; // If no file redirection
            } 
        }
        /* Traverse each step of the task */
        while(stepp->next) {
            /* Setup pipe */
            pipe(p_fd);   // Create next pipe
        
            /* Process next step */
            c_counter++; // Increment number of piped processes
            switch(fork()) {
                case -1:
                    fprintf(stderr, "%sfork() error\n", KRED);
                    exit(EXIT_FAILURE);
                case 0:
                    /* Setup pipes inside child process */
                    close(p_fd[0]); // Closed unsused read end
                    dup2(in_fd, STDIN_FILENO);
                    dup2(p_fd[1], STDOUT_FILENO);
                    
                    execute_step(stepp);
                default:
                    /* In parent */
                    close(p_fd[1]);
                    close(in_fd);
                    in_fd = p_fd[0];// Next step reads from here
                    break;
            }
            stepp = stepp->next;
        }
        /* Finish last step */
        /* Handle output redirection (if any) */
        if(taskp->output_file) {
            /* If file doesnt exists, create it. If it does, truncate to zero length */
            if((out_fd = open(taskp->output_file, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU)) == -1) {
                debug("outputfile: %s", taskp->output_file);
                debug("errno %d", errno);
                exit(INVALID_OUTPUT_FILE);
            }
        } else {
            out_fd = STDOUT_FILENO;
        }

        c_counter++; // Increment number of piped processes
        switch(fork()) {
            case -1:
                fprintf(stderr, "%sfork() error\n", KRED);
                exit(EXIT_FAILURE);
            case 0:
                dup2(out_fd, STDOUT_FILENO);
                
                execute_step(stepp);
            default:
                break;
        }
        stepp = stepp->next;

        /* Wait for pipe to complete */
        if(wait_for_pipe(c_counter)) {
            exit(PIPE_ERROR);
        }
        c_counter = 0;
        taskp = taskp->next;
    }

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
        while(recipe && (*((unsigned *)recipe->state) == COMPLETED ||
              *((unsigned *)recipe->state) == PROCESSING)) {
            recipe = remove_from_work_queue();
        }

        if(recipe) {
            *(unsigned *)recipe->state = PROCESSING; 
            
            current_num_cooks++; // Increment number of child processes
            /* Process next recipe in Work Queue */
            switch((pid = fork())) {
                case -1:
                    fprintf(stderr, "%sfork() error\n", KRED);
                    exit(EXIT_FAILURE);
                case 0:
                    sigaction_wrapper(SIGCHLD, SIG_DFL); // Unistall signal handler for children processes
                    sigprocmask_wrapper(SIG_SETMASK, &prev, NULL); // Unblock SIGCHLD
                    process_recipe(recipe); // Complete tasks of current recipe
                default:
                    /* Map recipe to the most recent child process' PID */
                    add_to_map(recipe, pid);
                    break;
            }
        }     

        /* If max number of cooks reached */
        if(current_num_cooks == max_cooks || !work_head) {
            /* Wait for completion of one or more recipes (wait for a SIGCHLD Signal) */
            sigsuspend(&prev);
            if(errno != EINTR) {
                fprintf(stderr, "%ssigsuspend() error\n", KRED);
            }
        }
        
        /* 
            If an error ocurred,
            do not proceed. 
        */
        if(exit_code)
            break;
    }

    return exit_code;
}

