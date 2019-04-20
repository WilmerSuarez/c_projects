#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>
#include "debug.h"
#include "signal_handling.h"
#include "recipe_processing.h"
#include "recipe_pid_map.h"
#include "cook.h"

#include <string.h>
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

static void sio_ltoa(long v, char s[], int b) 
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {  
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}

ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];
    
    sio_ltoa(v, s, 10); /* Based on K&R itoa() */  //line:csapp:sioltoa
    return sio_puts(s);
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;
  
    if ((n = sio_puts(s)) < 0) {}

    return n;
}

ssize_t Sio_putl(long v)
{
    ssize_t n;
  
    if ((n = sio_putl(v)) < 0) {}

    return n;
}

/*
 * Check if any recipies that depends on the 
 * given recipe are ready to be processed. Add
 * them to the Queue if they are.
*/
static void
dependency_check(RECIPE *recipe) {
    RECIPE_LINK *temp = recipe->depend_on_this;
    RECIPE_LINK *temp2 = NULL;

    /* If main recipe - done */
    if(!temp)
        return;

    /* Traverse RECIPE_LINK lists */
    /* Test each of the recipes that depend on this one */
    while(temp != NULL) {
        temp2 = temp->recipe->this_depends_on;
        while(temp2 != NULL) {
            /* Has dependency been processed? */
            if(temp2->recipe->state && 
              *((unsigned *)temp2->recipe->state) == COMPLETED) {
                /* Test next dependency */
                temp2 = temp2->next;
            } else {
                /* Not ready to be process (has pending dependencies) */
                break;
            }

            /* Add recipe ready to be processed to Work Queue*/
            if(!temp2 && temp->recipe->state)
                add_to_work_queue(temp->recipe);
        }
        /* Check next recipe that depends on this one */
        temp = temp->next;
    }
}

/*
 * Catches SIGCHLD Signals when a recipe 
 * has finished processing. Examines the
 * exit status of the the process the finished 
 * process.
*/
void
sigchld_handler(int sig) {
    int olderrno = errno; // Save errno
    int status;           // Used to check if child processes exited normally
    __pid_t pid;          // Child processes ID

    /* Reap all child processes that completed a recipe */
    while((pid = waitpid(-1, &status, 0)) > 0) {
        if(WIFEXITED(status)) {  // Check if child process exited normally 
            current_num_cooks--; // Decrement number of running child processes

            /* Get the recipe from map */
            RECIPE *recipe = get_recipe(pid);

            /* Examine reaped process exit status */
            if(!WEXITSTATUS(status)) {
                /* Recipe cooked successfully */
                *(unsigned *)recipe->state = COMPLETED;
                /* Add now ready recipes to Work Queue */
                dependency_check(recipe);
            } else {
                /* Recipe failed to cook */
                *(unsigned *)recipe->state = FAILED;
            }
            Sio_puts("Num Cooks: ");
            Sio_putl((long)current_num_cooks);
            Sio_puts("\n");
            
            Sio_puts("Child returned normally : PID : ");
            Sio_putl((long)pid);
            Sio_puts(" : Exit Status :  ");
            Sio_putl((long)WEXITSTATUS(status));
            Sio_puts("\n");

            Sio_puts("Recipe: ");
            Sio_puts(recipe->name);
            Sio_puts("\n");
        } else {
            exit_code = FAILED_RECIPE;
        }
    }

    errno = olderrno; // Restore errno
}

/*
 * SIGFILLSET() wrapper
*/
void
sigfillset_wrapper(__sigset_t *set) {
    if(sigfillset(set) < 0) {
        fprintf(stderr, "%ssigfillset() error\n", KRED);
        exit(EXIT_FAILURE);
    }
}

/*
 * SIGEMPTYSET() wrapper
*/
void
sigemptyset_wrapper(__sigset_t *set) {
    if(sigemptyset(set) < 0) {
        fprintf(stderr, "%ssigemptyset() error\n", KRED);
        exit(EXIT_FAILURE);
    }
}

/*
 * SIGADDSET() wrapper
*/
void
sigaddset_wrapper(__sigset_t *set, int signum) {
    if(sigaddset(set, signum) < 0) {
        fprintf(stderr, "%ssigaddset() error\n", KRED);
        exit(EXIT_FAILURE);
    }
}

/*
 * SIGPROCMASK() wrapper
*/
void
sigprocmask_wrapper(int how, const __sigset_t *set, __sigset_t *oldest) {
    if(sigprocmask(how, set, oldest) < 0) {
        fprintf(stderr, "%ssigprocmask() error\n", KRED);
        exit(EXIT_FAILURE);
    }
}

/*
 * SIGACTION() wrapper
*/
void 
sigaction_wrapper(int signum, __sighandler_t handler) {
    struct sigaction action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); 
    action.sa_flags = SA_RESTART;

    if(sigaction(signum, &action, NULL)) {
        fprintf(stderr, "%ssigaddset() error\n", KRED);
        exit(EXIT_FAILURE);
    }
}