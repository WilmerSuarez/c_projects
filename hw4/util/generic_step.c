#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

/*
 * Test program to be used as a "generic step" in a recipe task.
 * When invoked, it announces itself using the name by which it was invoked,
 * then delays for a randomly chosen interval.  If invoked with arguments,
 * and the first argument is '-', then it copies stdin to stdout until EOF
 * is seen, otherwise it does not do this.  Finally, then announces its
 * termination and exits.
 */

int main(int argc, char *argv[]) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srandom((unsigned int)ts.tv_nsec);
    int delay = random() % 10;

    fprintf(stderr, "START\t[%ld.%03ld,%6d,%2d] %s",
	    ts.tv_sec-1553700000, ts.tv_nsec % 1000,
	    getpid(), delay, argv[0]);
    for(int i = 1; i < argc; i++) {
	    fprintf(stderr, " %s", argv[i]);
    }
    fprintf(stderr, "\n");
    if(argc > 1 && !strcmp(argv[1], "-")) {
        int c;
	    while((c = getchar()) != EOF)
	        putchar(c);
    }
    sleep(delay);
    clock_gettime(CLOCK_REALTIME, &ts);
    fprintf(stderr, "END\t[%ld.%03ld,%6d,%2d]\n",
	    ts.tv_sec-1553700000, ts.tv_nsec % 1000, getpid(), delay);
}
