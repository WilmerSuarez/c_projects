#ifndef SIGNAL_HANDLING_H
#define SIGNAL_HANDLING_H

#include <signal.h>

unsigned current_num_cooks; // Number of child processes working on recipes

/*
 * Catches SIGCHLD Signals when a recipe 
 * has finished processing. Examines the
 * exit status of the the process the finished 
 * process.
*/
void
sigchld_handler(int sig);

/*
 * SIGFILLSET() wrapper
*/
void
sigfillset_wrapper(sigset_t *set);

/*
 * SIGEMPTYSET() wrapper
*/
void
sigemptyset_wrapper(sigset_t *set);

/*
 * SIGADDSET() wrapper
*/
void
sigaddset_wrapper(sigset_t *set, int signum);

/*
 * SIGACTION() wrapper
*/
void
sigaction_wrapper(int signum, __sighandler_t handler);

/*
 * SIGPROCMASK() wrapper
*/
void
sigprocmask_wrapper(int how, const sigset_t *set, sigset_t *oldest);

#endif /* SIGNAL_HANDLING_H */