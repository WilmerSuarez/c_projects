#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "csapp.h"
#include "client_registry.h"
#include "maze.h"
#include "player.h"
#include "debug.h"
#include "server.h"

static void terminate(int status);

static char *default_maze[] = {
  "******************************",
  "***** %%%%%%%%% &&&&&&&&&&& **",
  "***** %%%%%%%%%        $$$$  *",
  "*           $$$$$$ $$$$$$$$$ *",
  "*##########                  *",
  "*########## @@@@@@@@@@@@@@@@@*",
  "*           @@@@@@@@@@@@@@@@@*",
  "******************************",
  NULL
};

CLIENT_REGISTRY *client_registry;

/*
 * Catches SIGHUP signal to shutdown the server.
*/
void
sighup_handler(int sig) {
    int olderrno = errno; // Save errno
    
    terminate(EXIT_SUCCESS);

    errno = olderrno; // Restore errno
}

int main(int argc, char* argv[]) {
    /* ========== PROCESS ARGUMENTS ========== */
    char *port_num = NULL, **template_file = NULL;
    int opt; 
    while((opt = getopt(argc, argv, "p:t:")) != -1) {
        switch(opt) {
            case 'p': // Port Number
                /* Make sure port is not already being used */
                port_num = optarg;
            break;
            case 't': // Template File
                template_file = &optarg;
            break;
            default:
                fprintf(stderr, "%sUSAGE: mazewar -p <portnum> -t <template>\n", KWHT);
                exit(EXIT_FAILURE);
        }
    }

    if(!port_num) {
        fprintf(stderr, "%sPort number not specified\n", KRED);
        exit(EXIT_FAILURE);
    }
        
    /* Perform required initializations of the client_registry, maze, and player modules */
    client_registry = creg_init();
    /* Handle template file selection */
    if(template_file)
        maze_init(template_file);
    else
        maze_init(default_maze);
    player_init();
    debug_show_maze = 1;  // Show the maze after each packet.

    int listenfd, connfd;
    socklen_t clinetlen = sizeof(struct sockaddr_storage);
    struct sockaddr_storage clientaddr;
    pthread_t tid; 

    /* INSTALL SIGHUP HANDLER */
    Signal(SIGHUP, sighup_handler);

    /* SET UP SERVER SOCKET & WAIT FOR CLIENT CONNECTION REQUESTS */
    listenfd = Open_listenfd(port_num); // Get FD of listening socket 
    while((connfd = Accept(listenfd, (SA *)&clientaddr, &clinetlen))) { /* Create new socket from connection request queue */
        /* ALlocate and initialize newly created socket connection */
        int *new_sock_conn = Malloc(sizeof(int));
        *new_sock_conn = connfd;

        /* Create new service thread */
        Pthread_create(&tid, NULL, mzw_client_service, new_sock_conn);
    }

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);
    
    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    player_fini();
    maze_fini();

    debug("MazeWar server terminating");
    exit(status);
}
