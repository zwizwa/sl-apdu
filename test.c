/* Test: drive APDU parser using trace on stdin.
   Usage:  bzcat sim-4MHz.bin.bz2 | ./test */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "apdu.h"

int main(int argc, char **argv) {
    char buf[4096];
    int rv;

    int fd = 0;
    if (1) {
        fd = open("sim-4MHz.bin", O_RDONLY);
    }

    struct apdu_state *apdu_sink =
        apdu_new(4000000,   // logic sample freq
                 3817000);  // card clock
    for(;;) {
        rv = read(fd, buf, sizeof(buf));
        if (rv <= 0) {
            fprintf(stderr, "read -> %d\n", rv);
            return rv;
        }
        apdu_push8(apdu_sink, buf, rv);
    }
}

