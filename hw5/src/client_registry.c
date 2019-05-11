#include <pthread.h>
#include <semaphore.h>
#include "client_registry.h"
#include "csapp.h"

/* Maximum number of file descriptros */
#define MAX_CLIENTS 100

/* Mutex used when registering and unregistering clients */
pthread_mutex_t mutex; 
/* Semaphore used to block threads in creg_wait_for_empty() */ 
sem_t semaphore;
int FD_COUNT = 0; // Number of registered clients

typedef struct client_registry {
    int FDS[MAX_CLIENTS];
} CLIENT_REGISTRY;

/*
 * Initialize a new client registry.
 */
CLIENT_REGISTRY *creg_init() {
    /* Initialize Locks */
    if(pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "Mutex Init Error\n");
    }
    Sem_init(&semaphore, 0, 1);

    /* Create new Client Registry */
    CLIENT_REGISTRY *cr = malloc(sizeof(CLIENT_REGISTRY));

    /* Make all FD's invalid */
    for(int i = 0; i < (MAX_CLIENTS-1); i++) {
        cr->FDS[i] = -1;
    }

    return cr;
}

/*
 * Finalize a client registry.
 */
void creg_fini(CLIENT_REGISTRY *cr) {
    /* Close all FD's */
    for(int i = 0; i < (FD_COUNT-1); i++) {
        Close(cr->FDS[i]);
    }

    /* Free Registry */
    Free(cr);
}

/*
 * Register a client file descriptor.
 */
void creg_register(CLIENT_REGISTRY *cr, int fd) {
    /* Critical Section Start */
    pthread_mutex_lock(&mutex);
    
    /* Check limits */
    if(FD_COUNT == (MAX_CLIENTS-1)) {
        fprintf(stderr, "Maximum number of clients reached!\n");
        shutdown(fd, SHUT_RD); // Shut down client read side of connected socket
        return;
    }

    /* Add the new client FD to Registry */
    cr->FDS[FD_COUNT] = fd;
    FD_COUNT++;

    /* Critical Section End */
    pthread_mutex_unlock(&mutex);
}

/*
 * Unregister a client file descriptor, alerting anybody waiting
 * for the registered set to become empty.
 */
void creg_unregister(CLIENT_REGISTRY *cr, int fd) {
    /* Critical Section Start */
    pthread_mutex_lock(&mutex);

    /* Close and shutdown fd */
    shutdown(fd, SHUT_RD);
    Close(fd);

    /* Remove client FD from Registry */
    /* Find fd to remove */
    int i = 0;
    for(i = 0; i < (MAX_CLIENTS-1); i ++) {
        if(cr->FDS[i] == fd)
            break;
    }

    /* Shift elemnts down */
    for(int j = i; j < (MAX_CLIENTS-1); j++) {
        cr->FDS[j] = cr->FDS[j+1];
    }

    FD_COUNT--;
    
    /* Alert creg_wait_for_empty() when Client Registry empty */
    if(FD_COUNT == 0)
        V(&semaphore);

    /* Critical Section End */
    pthread_mutex_unlock(&mutex);
}

/*
 * A thread calling this function will block in the call until
 * the number of registered clients has reached zero, at which
 * point the function will return.
 */
void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
    /* Block all threads until Client Register is empty */
    P(&semaphore);
    V(&semaphore);
}

/*
 * Shut down all the currently registered client file descriptors.
 */
void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    /* Shutdown all registered connected client socket fd's */
    for(int i = 0; i < (FD_COUNT-1); ++i) {
        shutdown(cr->FDS[i], SHUT_RD);
    }
}
